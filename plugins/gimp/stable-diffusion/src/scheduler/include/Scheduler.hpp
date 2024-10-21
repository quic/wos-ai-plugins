/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#include <string>
#include <vector>

#ifndef float32_t
using float32_t = float;
#endif

class DPMSolverMultistepScheduler {
public:
    
    ~DPMSolverMultistepScheduler();

    
    DPMSolverMultistepScheduler(int32_t num_train_timesteps=1000, double beta_start=0.0001,
                                double beta_end=0.02, std::string beta_schedule="linear",
                                std::vector<float32_t> trained_betas={}, std::vector<float32_t> trained_lambdas={},
                                int32_t solver_order=2, std::string prediction_type="epsilon",
                                bool thresholding=false, double dynamic_thresholding_ratio=0.995,
                                double sample_max_value=1.0, std::string algorithm_type="dpmsolver++",
                                std::string solver_type="midpoint", bool lower_order_final=true);

    
    void setGuidanceScale(double guidanceScale) { m_GuidanceScale = static_cast<float32_t>(guidanceScale); };
    
    
    void setTimesteps(int32_t num_inference_steps);

    
    bool step(void* model_output, int32_t timestep, void* prev_output, void* curr_output);

private:
    std::vector<float32_t> m_Betas;
    std::vector<float32_t> m_Alphas;
    std::vector<float32_t> m_AlphasCumprod;
    std::vector<float32_t> m_Alpha_t;
    std::vector<float32_t> m_Sigma_t;
    std::vector<float32_t> m_Lambda_t;
    float32_t m_InitNoiseSigma;
    std::vector<int32_t> m_Timesteps;
    std::vector<void*> m_ModelOutputs;
    int32_t m_LowerOrderNums;
    int32_t m_NumInferenceSteps;
    int32_t m_NumTrainTimesteps;
    float32_t m_GuidanceScale;
    std::string m_AlgorithmType;
    std::string m_PredictionType;
    std::string m_SolverType;
    int32_t m_SolverOrder;
    bool m_Thresholding;
    bool m_LowerOrderFinal;

    bool convertModelOutput(void* model_data, int32_t timestep, void* sample_data);

    void dpmSolverFirstOrderUpdate(const std::vector<int32_t>& timestep,
                                   int32_t prev_timestep, void* sample_data, void* curr_output);

    void dpmSolverSecondOrderUpdate(const std::vector<int32_t>& timestep,
                                    int32_t prev_timestep, void* sample_data, void* curr_output);
    
    void dpmSolverThirdOrderUpdate(const std::vector<int32_t>& timestep,
                                   int32_t prev_timestep, void* sample_data, void* curr_output);

}; // DPMSolverMultistepScheduler class
