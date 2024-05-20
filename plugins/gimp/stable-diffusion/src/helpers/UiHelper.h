// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include <dlfcn.h>

#include <iostream>
#include <filesystem>
#include <memory>
#include <string>

#include "GetOpt.hpp"
#include "QnnApiHelpers.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "Client.hpp"


#define VAE_START_POINT 10 
#define VAE_FREQ 5

static void* sg_backendHandle{ nullptr };
static void* sg_modelHandle{ nullptr };


class UiHelper {
public:
	UiHelper(std::string config_path, std::string backend);
	~UiHelper();
	bool init();
	bool executeStableDiffusion(int seed, int step, float scale, std::string input_text);
	void convertOutputImageToCV();
	cv::Mat getOutputImageCV();
	Helpers::InferenceReturn getInferenceReturn();
	int getStepNumber();
	void reinit();
	bool getImageUpdateStatus();
	

private:
	int step_number;
	std::string config_file_path;
	std::string backend_path;
	cv::Mat outputModelImage;
	RuntimeApiHelpers* app = new QnnApiHelpers;
	Helpers::InferenceReturn inferenceReturn;
	bool imageUpdateStatus;

};
