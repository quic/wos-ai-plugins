// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include "Scheduler.hpp"
#include <cmath>

#define INPUT_WIDTH 64
#define INPUT_HEIGHT 64
#define INPUT_CHANNEL 4
#define TOTAL_LENGTH INPUT_WIDTH*INPUT_HEIGHT*INPUT_CHANNEL

#ifdef BUILD_RELEASE
#define MY_LOGV(format, ...)
#define MY_LOGD(format, ...)
#define MY_LOGE(format, ...)
#else
#ifdef __ANDROID__
#include <android/log.h>
#define LOGTAG "Scheduler"
#define MY_LOGV(format, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOGTAG, format, ##__VA_ARGS__)
#define MY_LOGD(format, ...) __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, format, ##__VA_ARGS__)
#define MY_LOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOGTAG, format, ##__VA_ARGS__)
#else
#define MY_LOGV(format, ...) printf("Scheduler INFO: "#format "\n", ##__VA_ARGS__)
#define MY_LOGD(format, ...) printf("Scheduler DEBUG: "#format "\n", ##__VA_ARGS__)
#define MY_LOGE(format, ...) printf("Scheduler ERROR: "#format "\n", ##__VA_ARGS__)
#endif
#endif

inline float32_t torch_exp(float32_t x) {
    return std::pow(2.71828185, x);
}

inline double alphaBar(double timestep) {
    const double pi = std::atan(static_cast<double>(1)) * 4;
    return std::pow(std::cos((timestep + 0.008) / 1.008 * pi / 2), 2);
}

std::vector<float32_t> linspace(double start, double end, int64_t num_points) {
    std::vector<float32_t> rval(num_points);

    const double step = (end - start) / (num_points - 1);
    for (int64_t i = 0; i < num_points - 1; ++i) {
        rval[i] = static_cast<float32_t>(start + i * step);
    }
    rval[num_points - 1] = static_cast<float32_t>(end);

    return rval;
}

std::vector<float32_t> betasForAlphaBar(int64_t num_diffusion_timesteps, double max_beta=0.999) {
    std::vector<float32_t> betas(num_diffusion_timesteps);

    for (int64_t i = 0; i < num_diffusion_timesteps; ++i) {
        double t1 = static_cast<double>(i) / num_diffusion_timesteps;
        double t2 = static_cast<double>(i + 1) / num_diffusion_timesteps;
        betas[i]  = static_cast<float32_t>(std::min(1 - alphaBar(t2) / alphaBar(t1), max_beta));
    }

    return betas;
}

DPMSolverMultistepScheduler::~DPMSolverMultistepScheduler() {
    for (auto& model_output_data : m_ModelOutputs) {
        if (nullptr != model_output_data) {
            std::free(model_output_data);
            model_output_data = nullptr;
        }
    }
}

DPMSolverMultistepScheduler::DPMSolverMultistepScheduler(int32_t num_train_timesteps, double beta_start,
                                                         double beta_end, std::string beta_schedule,
                                                         std::vector<float32_t> trained_betas, std::vector<float32_t> trained_lambdas,
                                                         int32_t solver_order, std::string prediction_type,
                                                         bool thresholding, double dynamic_thresholding_ratio,
                                                         double sample_max_value, std::string algorithm_type,
                                                         std::string solver_type, bool lower_order_final) {
    if (!trained_betas.empty()) {
        m_Betas = trained_betas;
    } else if (beta_schedule == "linear") {
        m_Betas = linspace(beta_start, beta_end, num_train_timesteps);
    } else if (beta_schedule == "scaled_linear") {
        m_Betas = linspace(std::sqrt(beta_start), std::sqrt(beta_end), num_train_timesteps);
        for (int32_t i = 0; i < m_Betas.size(); ++i) {
            m_Betas[i] = m_Betas[i] * m_Betas[i];
        }
    } else if (beta_schedule == "squaredcos_cap_v2") {
        m_Betas = betasForAlphaBar(num_train_timesteps);
    }

    double alphaCumprod = 1.0;
    for (int32_t i = 0; i < m_Betas.size(); ++i) {
        m_Alphas.push_back(1.0 - m_Betas[i]);

        alphaCumprod *= m_Alphas[i];
        m_AlphasCumprod.push_back(static_cast<float32_t>(alphaCumprod));

        m_Alpha_t.push_back(std::sqrt(m_AlphasCumprod[i]));
        m_Sigma_t.push_back(std::sqrt(1 - m_AlphasCumprod[i]));
    }

    if (!trained_lambdas.empty()) {
        m_Lambda_t = trained_lambdas;
    } else {
        for (int32_t i = 0; i < m_Betas.size(); ++i) {
            m_Lambda_t.push_back(std::log(m_Alpha_t[i]) - std::log(m_Sigma_t[i]));
        }
    }

    m_InitNoiseSigma = 1.0;

    auto timesteps = linspace(0, num_train_timesteps - 1, num_train_timesteps);
    m_Timesteps.resize(timesteps.size());
    for (int32_t i = 0; i < timesteps.size(); ++i) {
        m_Timesteps[i] = static_cast<int32_t>(timesteps[i]);
    }
    std::reverse(m_Timesteps.begin(), m_Timesteps.end());

    m_ModelOutputs = std::vector<void*>(solver_order, nullptr);
    for (int32_t i = 0; i < solver_order; i++) {
        m_ModelOutputs[i] = (void*) std::malloc(TOTAL_LENGTH * sizeof(float32_t));
    }

    m_LowerOrderNums = 0;
    m_AlgorithmType = algorithm_type;
    m_PredictionType = prediction_type;
    m_SolverOrder = solver_order;
    m_Thresholding = thresholding;
    m_SolverType = solver_type;
    m_LowerOrderFinal = lower_order_final;
    m_NumTrainTimesteps = num_train_timesteps;
}

void DPMSolverMultistepScheduler::setTimesteps(int32_t num_inference_steps) {
    m_NumInferenceSteps = num_inference_steps;
    m_InitNoiseSigma = 1.0;

    auto timesteps = linspace(0, m_NumTrainTimesteps - 1, m_NumInferenceSteps + 1);
    m_Timesteps.resize(timesteps.size());
    for (int32_t i = 0; i < timesteps.size(); ++i) {
        m_Timesteps[i] = static_cast<int32_t>(std::round(timesteps[i]));
    }
    std::reverse(m_Timesteps.begin(), m_Timesteps.end());
    m_Timesteps.pop_back();

    m_LowerOrderNums = 0;
}

bool DPMSolverMultistepScheduler::convertModelOutput(void* model_data, int32_t timestep, void* sample_data) {
    auto alpha_t = m_Alpha_t[timestep];
    auto sigma_t = m_Sigma_t[timestep];
    
    auto sample = (float32_t*)sample_data;
    auto model_output = (float32_t*)model_data;
    if (m_AlgorithmType == "dpmsolver++") {
        if (m_PredictionType == "epsilon") {
            for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, model_output++) {
                *model_output = (*sample - (sigma_t * (*model_output))) * (1 / alpha_t);
            }
        } else if (m_PredictionType == "sample") {
            // Do nothing since output is same as model_data
        } else if (m_PredictionType == "v_prediction") {
            for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, model_output++) {
                *model_output = alpha_t * (*sample) - sigma_t * (*model_output);
            }
        } else {
            MY_LOGE("prediction_type given as %s must be one of `epsilon`, `sample`, or `v_prediction` for the DPMSolverMultistepScheduler.",
                    m_PredictionType.c_str()); 
            return false;
        }
    } else if (m_AlgorithmType == "dpmsolver") {
        if (m_PredictionType == "epsilon") {
            // Do nothing since output is same as model_data
        } else if (m_PredictionType == "sample") {
            for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, model_output++) {
                *model_output = (*sample - (alpha_t * (*model_output))) * (1 / sigma_t);
            }
        } else if (m_PredictionType == "v_prediction") {
            for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, model_output++) {
                *model_output = alpha_t * (*model_output) + sigma_t * (*sample);
            }
        } else {
            MY_LOGE("prediction_type given as %s must be one of `epsilon`, `sample`, or `v_prediction` for the DPMSolverMultistepScheduler.",
                    m_PredictionType.c_str()); 
            return false;
        }
    } else {
        MY_LOGE("algorithm_type given as %s must be one of `dpmsolver++`, or `dpmsolver`.",
                m_AlgorithmType.c_str()); 
        return false;
    }

    return true;
}

void DPMSolverMultistepScheduler::dpmSolverFirstOrderUpdate(const std::vector<int32_t>& timestep_list,
                                                            int32_t prev_timestep, void* sample_data, void* curr_output) {
    auto t  = prev_timestep;
    auto s0 = timestep_list[0];

    auto lambda_t = m_Lambda_t[t];
    auto lambda_s = m_Lambda_t[s0];
    auto alpha_t  = m_Alpha_t[t];
    auto alpha_s  = m_Alpha_t[s0];
    auto sigma_t  = m_Sigma_t[t];
    auto sigma_s  = m_Sigma_t[s0];
    auto h        = lambda_t - lambda_s;

    auto m0 = m_ModelOutputs[m_SolverOrder - 1];

    auto D0 = (float32_t*)m0;

    float32_t w0, w1;
    if (m_AlgorithmType == "dpmsolver++") {
        w0 = (sigma_t / sigma_s);
        w1 = (alpha_t * (torch_exp(-h) - 1));
    } else if (m_AlgorithmType == "dpmsolver") {
        w0 = (alpha_t / alpha_s);
        w1 = (sigma_t * (torch_exp(h) - 1));
    }

    auto sample = (float32_t*)sample_data;
    auto x_t    = (float32_t*)curr_output;
    for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, D0++, x_t++) {
        *x_t = w0 * (*sample) - w1 * (*D0);
    }
}

void DPMSolverMultistepScheduler::dpmSolverSecondOrderUpdate(const std::vector<int32_t>& timestep_list,
                                                             int32_t prev_timestep, void* sample_data, void* curr_output) {
    auto t  = prev_timestep;
    auto s0 = timestep_list[1];
    auto s1 = timestep_list[0];

    auto lambda_t  = m_Lambda_t[t];
    auto lambda_s0 = m_Lambda_t[s0];
    auto lambda_s1 = m_Lambda_t[s1];
    auto alpha_t   = m_Alpha_t[t];
    auto alpha_s0  = m_Alpha_t[s0];
    auto sigma_t   = m_Sigma_t[t];
    auto sigma_s0  = m_Sigma_t[s0];
    auto h         = lambda_t  - lambda_s0;
    auto h_0       = lambda_s0 - lambda_s1;
    auto r0        = h_0 / h;

    auto m0 = m_ModelOutputs[m_SolverOrder - 1];
    auto m1 = m_ModelOutputs[m_SolverOrder - 2];

    auto D0 = (float32_t*)m0;
    std::vector<float32_t> D1(TOTAL_LENGTH);
    auto m0_data = (float32_t*)m0;
    auto m1_data = (float32_t*)m1;
    for (int64_t i = 0; i < TOTAL_LENGTH; i++, m0_data++, m1_data++) {
        D1[i] = (1 / r0) * (*m0_data - *m1_data);
    }

    float32_t w0, w1, w2;
    if (m_AlgorithmType == "dpmsolver++") {
        if (m_SolverType == "midpoint") {
            w0 =       (sigma_t / sigma_s0);
            w1 =       (alpha_t * (torch_exp(-h) - 1));
            w2 = 0.5 * (alpha_t * (torch_exp(-h) - 1));
        } else if (m_SolverType == "heun") {
            w0 =  (sigma_t / sigma_s0);
            w1 =  (alpha_t * (torch_exp(-h) - 1));
            w2 = -(alpha_t * ((torch_exp(-h) - 1) / h + 1));
        }
    } else if (m_AlgorithmType == "dpmsolver") {
        if (m_SolverType == "midpoint") {
            w0 =       (alpha_t / alpha_s0);
            w1 =       (sigma_t * (torch_exp(h) - 1));
            w2 = 0.5 * (sigma_t * (torch_exp(h) - 1));
        } else if (m_SolverType == "heun") {
            w0 = (alpha_t / alpha_s0);
            w1 = (sigma_t * (torch_exp(h) - 1));
            w2 = (sigma_t * ((torch_exp(h) - 1) / h - 1));
        }
    }

    auto sample = (float32_t*)sample_data;
    auto x_t    = (float32_t*)curr_output;
    for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, D0++, x_t++) {
        *x_t = w0 * (*sample) - w1 * (*D0) - w2 * D1[i];
    }
}

void DPMSolverMultistepScheduler::dpmSolverThirdOrderUpdate(const std::vector<int32_t>& timestep_list,
                                                            int32_t prev_timestep, void* sample_data, void* curr_output) {
    auto t  = prev_timestep;
    auto s0 = timestep_list[2];
    auto s1 = timestep_list[1];
    auto s2 = timestep_list[0];

    auto lambda_t  = m_Lambda_t[t];
    auto lambda_s0 = m_Lambda_t[s0];
    auto lambda_s1 = m_Lambda_t[s1];
    auto lambda_s2 = m_Lambda_t[s2];
    auto alpha_t   = m_Alpha_t[t];
    auto alpha_s0  = m_Alpha_t[s0];
    auto sigma_t   = m_Sigma_t[t];
    auto sigma_s0  = m_Sigma_t[s0];
    auto h         = lambda_t - lambda_s0;
    auto h_0       = lambda_s0 - lambda_s1;
    auto h_1       = lambda_s1 - lambda_s2;
    auto r0        = h_0 / h;
    auto r1        = h_1 / h;

    auto m0 = m_ModelOutputs[m_SolverOrder - 1];
    auto m1 = m_ModelOutputs[m_SolverOrder - 2];
    auto m2 = m_ModelOutputs[m_SolverOrder - 3];

    auto D0 = (float32_t*)m0;
    std::vector<float32_t> D1(TOTAL_LENGTH);
    std::vector<float32_t> D2(TOTAL_LENGTH);
    auto m0_data = (float32_t*)m0;
    auto m1_data = (float32_t*)m1;
    auto m2_data = (float32_t*)m2;
    for (int64_t i = 0; i < TOTAL_LENGTH; i++, m0_data++, m1_data++, m2_data++) {
        auto D1_0 = (1 / r0) * (*m0_data - *m1_data);
        auto D1_1 = (1 / r1) * (*m1_data - *m2_data);

        D1[i] = D1_0 + (r0 / (r0 + r1)) * (D1_0 - D1_1);
        D2[i] = (1 / (r0 + r1)) * (D1_0 - D1_1);
    }

    float32_t w0, w1, w2, w3;
    if (m_AlgorithmType == "dpmsolver++") {
        w0 =  (sigma_t / sigma_s0);
        w1 =  (alpha_t * (torch_exp(-h) - 1));
        w2 = -(alpha_t * ((torch_exp(-h) - 1) / h + 1));
        w3 =  (alpha_t * ((torch_exp(-h) - 1 + h) / std::pow(h, 2) - 0.5));
    } else if (m_AlgorithmType == "dpmsolver") {
        w0 = (alpha_t / alpha_s0);
        w1 = (sigma_t * (torch_exp(h) - 1));
        w2 = (sigma_t * ((torch_exp(h) - 1) / h - 1));
        w3 = (sigma_t * ((torch_exp(h) - 1 - h) / std::pow(h, 2) - 0.5));
    }

    auto sample = (float32_t*)sample_data;
    auto x_t    = (float32_t*)curr_output;
    for (int64_t i = 0; i < TOTAL_LENGTH; i++, sample++, D0++, x_t++) {
        *x_t = w0 * (*sample) - w1 * (*D0) - w2 * D1[i] - w3 * D2[i];
    }
}

bool DPMSolverMultistepScheduler::step(void* model_output, int32_t timestep, void* prev_output, void* curr_output) {

    auto uncond_ptr = (float32_t*) model_output;
    auto cond_ptr   = uncond_ptr + TOTAL_LENGTH;
    auto model_data = (float32_t*) m_ModelOutputs[0];
    for (int32_t i = 0; i < TOTAL_LENGTH; i++, cond_ptr++, uncond_ptr++) {
        model_data[i] = *uncond_ptr + m_GuidanceScale * (*cond_ptr - *uncond_ptr);
    }

    if (true != convertModelOutput((void*) model_data, timestep, prev_output)) {
        MY_LOGE("Error in running convertModelOutput");
        return false;
    }

    for (int32_t i = 0; i < m_SolverOrder - 1; ++i) {
        m_ModelOutputs[i] = m_ModelOutputs[i + 1];
    }
    m_ModelOutputs[m_SolverOrder - 1] = (void*) model_data;

    auto it = std::find(m_Timesteps.begin(), m_Timesteps.end(), timestep);
    auto step_index = m_NumInferenceSteps - 1;
    if (it != m_Timesteps.end()) {
        step_index = it - m_Timesteps.begin();
    }

    int32_t prev_timestep = 0;
    if (step_index != (m_NumInferenceSteps - 1)) {
        prev_timestep = m_Timesteps[step_index + 1];
    }

    bool lower_order_final  = ((step_index == (m_NumInferenceSteps - 1)) && m_LowerOrderFinal && (m_NumInferenceSteps < 15));
    bool lower_order_second = ((step_index == (m_NumInferenceSteps - 2)) && m_LowerOrderFinal && (m_NumInferenceSteps < 15));

    if (m_SolverOrder == 1 || m_LowerOrderNums < 1 || lower_order_final) {
        dpmSolverFirstOrderUpdate({timestep},
                                  prev_timestep, prev_output, curr_output);
    } else if (m_SolverOrder == 2 || m_LowerOrderNums < 2 || lower_order_second) {
        dpmSolverSecondOrderUpdate({m_Timesteps[(m_NumInferenceSteps + step_index - 1) % m_NumInferenceSteps],
                                    timestep},
                                   prev_timestep, prev_output, curr_output);
    } else {
        dpmSolverThirdOrderUpdate({m_Timesteps[(m_NumInferenceSteps + step_index - 2) % m_NumInferenceSteps],
                                   m_Timesteps[(m_NumInferenceSteps + step_index - 1) % m_NumInferenceSteps],
                                   timestep},
                                  prev_timestep, prev_output, curr_output);
    }

    if (m_LowerOrderNums < m_SolverOrder) {
        m_LowerOrderNums++;
    }

    return true;
}
