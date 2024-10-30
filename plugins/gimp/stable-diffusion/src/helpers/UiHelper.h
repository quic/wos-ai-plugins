// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include <dlfcn.h>

#include <iostream>
#include <filesystem>
#include <memory>
#include <string>
#include <chrono>
#include <ctime> 
#include "GetOpt.hpp"
#include "QnnApiHelpers.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "Client.hpp"


#define VAE_START_POINT 10 
#define VAE_FREQ 5
#define VERSION_2_1 "SD_2_1"
#define VERSION_1_5 "SD_1_5"
static void* sg_backendHandle{ nullptr };
static void* sg_modelHandle{ nullptr };


class UiHelper {
public:
	UiHelper(std::string config_path, std::string backend, std::string model_version);
	~UiHelper();
	bool init();
	bool executeStableDiffusion(int seed, int step, float scale, std::string input_text);
	void convertOutputImageToCV();
	cv::Mat getOutputImageCV();
	cv::Mat outputModelImage;	
	Helpers::InferenceReturn getInferenceReturn();
	int getStepNumber();
	void reinit();
	bool getImageUpdateStatus();
	

private:
	int step_number;
	std::string config_file_path;
	std::string backend_path;
	std::string model_version;
	RuntimeApiHelpers* app = new QnnApiHelpers;
	Helpers::InferenceReturn inferenceReturn;
	bool imageUpdateStatus;

};
