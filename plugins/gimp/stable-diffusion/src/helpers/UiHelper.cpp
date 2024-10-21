/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/


#include "UiHelper.h"


UiHelper::UiHelper(std::string config_path, std::string backend) {
    outputModelImage = cv::Mat(512, 512, CV_8UC4);
    step_number = 0;
    imageUpdateStatus = false;
    config_file_path = config_path;
    backend_path = backend;
    std::ifstream isFilePath(config_file_path);
    if (!isFilePath) {
        std::cerr << "Config file is not a valid one. Input: " << config_file_path << std::endl;
    }
    std::ifstream isFilePathBackend(backend);
    if (!isFilePathBackend) {
        std::cerr << "Backend file is not a valid one. Input: " << backend_path << std::endl;
    }
}



UiHelper::~UiHelper() {
    delete app;

    if (sg_backendHandle) {
        dlclose(sg_backendHandle);
    }
    if (sg_modelHandle) {
        dlclose(sg_modelHandle);
    }
}

bool UiHelper::init() {
    // Load libraries method not required as custom helper is now statically linked
    /*if (0 != app->LoadLibraries("sd_custom_dll.dll")) {
        printf("LoadLibraries failure");
        return false;
    }*/

    if (0 != app->Init(config_file_path, Helpers::stripPath(config_file_path),
        768, 77, 1.0,
        512, 512, 4.0,
        false, backend_path)) {
        printf("Initialization failure");
        return false;
    }
    std::cout << "UI Initialisation complete" << std::endl;
    return true;
}

void UiHelper::reinit() {
    outputModelImage = cv::Mat(512, 512, CV_8UC4);
    step_number = 0;
    imageUpdateStatus = false;
}



bool UiHelper::executeStableDiffusion(int seed, int step, float scale, std::string input_text) {
    std::string text = input_text.substr(1);
    char buffer[1024]; sprintf(buffer, "%010d%010d%010f%s", seed, step, scale, text.c_str());

    std::string full_text(buffer);
    auto start = std::chrono::steady_clock::now();
    
    if (true != app->PreProcessInput((void*)full_text.c_str(), full_text.length(), false, false)) {
        printf("PreProcessInput failure");
        return false;
    }

    for (int mStepIdx = 0; mStepIdx < step; mStepIdx++) {
        
        bool runVAE = (((mStepIdx + 1) % VAE_FREQ == 0) ||
            ((mStepIdx + 1) == step) || ((mStepIdx + 1) == VAE_START_POINT)) && ((mStepIdx + 1) >= VAE_START_POINT);
        printf("\n");
        if (true != app->RunInference(runVAE)) {
            printf("RunInference failure");
            return false;
        }

        if (true == runVAE) {
            printf("\n");
            if (true != app->PostProcessOutput(false, false, inferenceReturn)) {
                printf("PostProcessOutput failure");
                return false;
            }
            app->unlockPostProcessBufferAccess();
            convertOutputImageToCV();
        }
        step_number = mStepIdx;
    }
    auto stop = std::chrono::steady_clock::now();
    Helpers::logProfile("Overall Inference time: ", start, stop);

    return true;
}

void UiHelper::convertOutputImageToCV() {
    std::memcpy(outputModelImage.data, (unsigned char*)inferenceReturn.m_ImageData, 512 * 512 * 4);
    cv::cvtColor(outputModelImage, outputModelImage, cv::COLOR_BGRA2RGBA);
    imageUpdateStatus = true;
}

cv::Mat UiHelper::getOutputImageCV() {
    imageUpdateStatus = false;
    return outputModelImage;
}

bool UiHelper::getImageUpdateStatus() {
    return imageUpdateStatus;
}

Helpers::InferenceReturn UiHelper::getInferenceReturn() {
    return inferenceReturn;
}

int UiHelper::getStepNumber() {
    return step_number;
}
