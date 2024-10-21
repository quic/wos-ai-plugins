/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#ifndef _RUNTIMEAPIHELPERS_HPP_
#define _RUNTIMEAPIHELPERS_HPP_

#include <sstream>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <numeric>
#include <map>
#include <cstdlib>

#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <algorithm>

#include <chrono>
#include <limits>
#include <windows.h>

#include "Helpers.hpp"


// Structure to store measurement statistics
struct RuntimeGauge {
  // Maximum value
  uint32_t minval;
  // Minimum value
  uint32_t maxval;

  // Constructor to initialize gauge
  RuntimeGauge() {
    minval = std::numeric_limits<uint32_t>::max();
    maxval = 0;
  }

  // Function to update the gauge based on sampled start and stop times
  void Update(std::chrono::steady_clock::time_point start,
              std::chrono::steady_clock::time_point stop) {
    // Execution time
    auto execution_time = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    // Update minimum of all measured times
    minval = std::min(minval, (uint32_t) execution_time);
    // Update maximum of all measured times
    maxval = std::max(maxval, (uint32_t) execution_time);
  }

  void Display(void) {
    DEMO_DEBUG("Gauge values: max = %d, min = %d", maxval, minval);
  }
};


class RuntimeApiHelpers {
public:

    RuntimeApiHelpers();
    virtual ~RuntimeApiHelpers();

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // These definitions are needed to cast dlsym with it for optional custom functions.            //
    // These signatures should be common across runtimes and/or different apps                      //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    typedef int32_t (*m_CustInitPtr)(std::string& configFilePath,
                                     Helpers::ImageDims& inputImageDims,
                                     std::unordered_map<std::string, Helpers::ImageDims>& modelInputImageDims,
                                     Helpers::QuantParameters& inputQuantParam,
                                     Helpers::QuantParameters& outputDequantParam,
                                     std::unordered_map<std::string, Helpers::ImageDims>& modelOutputImageDims,
                                     Helpers::ImageDims& outputImageDims
                                    );

    typedef bool (*m_CustPrePtr)(const void* inputData,
                                 bool isFlipped,
                                 void* preprocessedData
                                );

    typedef bool (*m_CustPostPtr)(std::unordered_map<std::string, void*>& modelOutputData,
                                  const void* orgInputData,
                                  Helpers::StagesData& postStageData,
                                  bool showLabels,
                                  bool showConfScores,
                                  Helpers::InferenceReturn &returnVal,
                                  bool dumpPostOutput,
                                  std::string& outputLocation
                                 );

    // CustomHelper lib function pointers
    m_CustInitPtr m_PtrCustInit{nullptr};
    m_CustPrePtr m_PtrCustPre{nullptr};
    m_CustPostPtr m_PtrCustPost{nullptr};

    // variable to open custom library
    void* m_CustomLibHandle{nullptr};

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // The following functions are declared as virtual to make it an abstract class.                //
    // All derived class/runrime should implement them. The signature should be sufficient for all  //
    // use cases.                                                                                   //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    /**
    * @brief Parse config file and assign initial values for class variables below, such as: the
          runtime instance, model width and height...
    * @param configFilePath: the injected.properties configuration file to be used to set initial values
    * @param nativeLibPath:  the location for the dsp library files. This is used to set the ADSP_LIBRARY_PATH environment
                             variable which is needed to execute runtime on dsp.
    * @param inputWidth:     the size of the input image width
    * @param inputHeight:    the size of the input image height
    * @param inputChannel:   the size of the input image channel
    * @param outputWidth:    the size of the preview image width
    * @param outputHeight:   the size of the preview image height
    * @param outputChannel:  the size of the preview image channel
    * @param debugModeRequested: boolean value to determine if runtime should run in debug mode. If true, then if the buffers
                              used are tensors (instead of userbuffers) then inference will return the output of all layers.
    * @param runtimeNameOrFile: what processor to execute runtime on. Supported thus far are: DSP, GPU, CPU

    * @return: 0 if no error, -1 otherwise
    */
    virtual int32_t Init (std::string configFilePath, std::string nativeLibPath,
                          int32_t inputWidth, int32_t inputHeight, float inputChannel,
                          int32_t outputWidth, int32_t outputHeight, float outputChannel,
                          bool debugModeRequested=false, std::string runtimeNameOrFile="QnnHtp.dll"
                         ) = 0;

    /**
    * @brief runs image preprocessing steps. It first checks for custom preprocessing is defined by user and does a
          dynamic call if it exists. If user hasn't specified a custom preprocess, by default function does:
            - color convert images from NV21 to BGR as needed. Camera input from Android Devices is
              always in nv21 format. If the framework APIs get run for other usecases such as the
              commandline app, it first checks if nv21 or bgr image_encoding is set by user.
            - Flips the image if camera is rotated
            - scales down or up the input image to the size of the model
            - Converts the data to  32 bit float as that is requirement with the framework setup
    * @param image: the input frame from camera
    * @param isFlipped: variable to see if we need to flip images before passing to model. For eg: This
                     can happen if someone has their phone flipped.
    * @param overlayOnImage: boolean value to determine whether user wants to overlay the output of
                          the model on top in the input image.
    * @param playOrPause variable to pause the playback.(0 is pause, 1 is play)
    * @return: true if no error, False otherwise
    */
    virtual bool PreProcessInput(void* image,
                                 uint32_t imageSize,
                                 bool isFlipped,
                                 bool overlayOnImage
                                ) = 0;

    /**
    * @brief wrapper function to run the inference
    * @param runVAE: boolean to determine if inference should run the VAE
    * @param dumpOutput: boolean to determine if the layer output(s) of inference should be written to disk
    * @param outputLocation: string value of where the location of inference output should be. Default: current dir.

    * @return: true if no error, False otherwise
    */
    virtual bool RunInference(bool runVAE,
                              bool dumpOutput=false,
                              std::string outputLocation="."
                             ) = 0;

    /**
    * @brief wrapper function to run custom postprocess function that is defined by user
    * @param showLabels: boolean value to determine whether user wants to see the labels of a detection.
    * @param showConfScores: boolean value to determine whether user wants to see the confidence score for the
                            detections predicted.
    * @param returnVal: the final output object. This is a struct with the following variables:
    * @param dumpPostOutput: boolean to determine whether the postprocess results should be written to disk
    * @param outputLocation: string value of where the location of inference output should be. Default: current dir.

    * @return: true if no error, False otherwise
    */
    virtual bool PostProcessOutput(bool showLabels,
                                   bool showConfScores,
                                   Helpers::InferenceReturn &returnVal,
                                   bool dumpPostOutput=false,
                                   std::string outputLocation="."
                                  ) = 0;


    //////////////////////////////////////////////////////////////////////////////////////////////////
    // The following functions are the common functions and can be used across all runtimes.        //
    //////////////////////////////////////////////////////////////////////////////////////////////////

    /**
    * @brief dynamically loads the custom library provided by user
    * @param customLibName: path to custom library name

    * @return: 0 if no error, -1 otherwise
    */
    int32_t LoadLibraries(std::string customLibName);

    /**
    * @brief parses a key=value config value and sets the relevant class level variables
    * @param configFilePath: the path to the configuration file

    * @return: 0 if no errors, -1 otherwise
    */
    int32_t setEnvVariable(std::string envVariable, std::string value);

    void lockPostProcessBufferAccess() { m_PostProcessBufferAccess.lock(); }
    void unlockPostProcessBufferAccess() { m_PostProcessBufferAccess.unlock(); }

    /**
    * @brief free the memory for all tensors from the bank. 
    * @param tensorsBufBank: vector of multiple buffers.
    */
    void tearDownTensorsBufBank(
        std::vector<std::unordered_map<std::string, std::unordered_map<std::string, void*>>>& tensorsBufBank);

    /**
    * @brief free the memory for all buffers from the bank. 
    * @param tensorsBufBank: vector of buffers.
    */
    void tearDownOrgDataBuf(std::vector<void*>& orgDataBuf);


    // Variable to store model path
    std::vector<std::string> m_ModelsPathVec;

    // Vector to store the execution order of multi-model demo
    std::vector<std::string> m_modelsExecOrder;

    // Variables to store input and output image's dimension
    Helpers::ImageDims m_InputDims, m_OutputDims;

    // Variable to keep track of number of input and output tensors for each graph
    std::unordered_map<std::string, uint32_t> m_numInputTensorsMap, m_numOutputTensorsMap;

    // Store one input and one output tensor name with their corresponding graph
    std::pair<std::string, std::string> m_InputTensorName, m_OutputTensorName;

    // Variable to keep track of dimensions of all input and output tensors of each model/graph
    std::unordered_map<std::string, std::unordered_map<std::string, Helpers::ImageDims>>
        m_ModelInputImageDims, m_ModelOutputImageDims;

    // Bank of all input and output tensors buffer memory with their graph
    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, void*>>>
        m_InputTensorsBufBank, m_OutputTensorsBufBank;

    // Set to hold of the memory pointers which has been freed
    std::unordered_set<void*> m_FreeTensorsPointerSet;

    // Bank to save original image data for overlay
    std::vector<void*> m_OrgDataPre, m_OrgDataPost;

    // pingpong index variables for each stages
    uint8_t m_pre_pingpong_index;
    uint8_t m_infer_in_pingpong_index, m_infer_out_pingpong_index;
    uint8_t m_post_pingpong_index;

    // Variables to keep track of number of runs of each preprocess, inference and postprocess
    int32_t m_preprocess_count, m_inference_count, m_postprocess_count;

    // Bank size of pre and post processing buffers
    uint8_t m_preProcessBankSize, m_postProcessBankSize;
private:
    // buffer control variables for post-processing function
    std::mutex m_PostProcessBufferAccess;
};

#endif
