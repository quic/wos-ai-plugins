// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include "StableDiffusionHelper.hpp"

///////////////// Custom Helpers /////////////////
static int toInt(const std::string& str)
{
  char dummyChar;
  int i;
  std::stringstream ss(str);
  ss >> i;
  if ( ss.fail() || ss.get(dummyChar))
  {
     throw std::runtime_error("Failed to parse string " + str + " to int");
  }
  return i;
}

static std::vector<uint8_t> toInt(const std::vector<std::string>& strs)
{
  std::vector<uint8_t> ints(strs.size());
  for (size_t i = 0; i < strs.size(); i++)
  {
     // ints[i] = std::stoi(strs[i]); // not available on Android???
     ints[i] = (uint8_t)toInt(strs[i]);
  }
  return ints;
}

static void PrintMap(std::unordered_map<std::string, cv::Mat> const map)
{
  for (auto const pair: map) {
      DEMO_DEBUG("Key (layer name) = %s", pair.first.c_str());
  }
}

static void PrintVector(std::vector<std::string> const str_vector)
{
  for(int index=0; index < str_vector.size(); ++index) {
    DEMO_DEBUG("Element = %d, value = %s",
                        index, str_vector.at(index).c_str());
  }
}

static std::string toHexString(const uint8_t& num)
{
  char dummyChar;
  std::stringstream ss;
  std::string res;
  ss << std::hex << (int)num;
  ss >> res;
  if ( ss.fail() || ss.get(dummyChar))
  {
     throw std::runtime_error("Failed to parse int to string");
  }
  return res;
}

static std::string rgb2hex(const std::vector<uint8_t>& ints, int8_t len=-1)
{
  if (len==-1)
    len = ints.size();

  std::string hex_string = "";
  for (uint8_t i = 0; i < len; i++)
  {
     if (ints[i] < 16)
         hex_string += "0" + toHexString(ints[i]);
     else 
         hex_string += toHexString(ints[i]);
  }
  return hex_string;
}

StableDiffusionHelper::StableDiffusionHelper() {
    m_PixelMapMat = cv::Mat(1, 256, CV_8UC3, cv::Scalar(0, 0, 0));

    m_allowedClasses = std::vector<bool>(NUM_CLASSES, false);

    g_lastNFramesClassAppearance = std::vector<std::vector<bool>>(NUM_ACC_FRAMES, std::vector<bool>(NUM_CLASSES, false));
}

StableDiffusionHelper::~StableDiffusionHelper() {

}

void StableDiffusionHelper::sort_based_on_last_forty_occurance(const std::vector<bool>& segmentedClasses)
{
  //Update g_lastNFramesClassAppearance to have current-frame-classes appearance
  for (uint8_t i = 0; i < NUM_CLASSES; i++)
     g_lastNFramesClassAppearance[g_lastNFramesCount][i] = m_allowedClasses[i] && segmentedClasses[i];

  //Accumulate classes appearance over frames
  std::vector<uint8_t> classesAccOverNFrames(NUM_CLASSES, 0);
  uint8_t classAccValue;
  for (uint8_t i = 0; i < NUM_CLASSES; i++)
  {
     classAccValue = 0;
     for (uint8_t frame = 0; frame < NUM_ACC_FRAMES; frame++)
        classAccValue += g_lastNFramesClassAppearance[frame][i];
     classesAccOverNFrames[i] = classAccValue;
  }

  //Add weighted number into classesAccOverNFrames w.r.t. classes appearance-order
  //in last frame to break tie in classesAccOverNFrames
  uint8_t sortedClassesLen = g_sortedClasses.size();
  for (uint8_t i = 0; i < sortedClassesLen; i++)
     classesAccOverNFrames[g_sortedClasses[i]] += (sortedClassesLen-i-1);

  //Incremenet frame count
  g_lastNFramesCount = (g_lastNFramesCount + 1) % NUM_ACC_FRAMES;

  //Check classes presence and absence in last NUM_ACC_FRAMES_FOR_CURR_CLASS_TO_APPEAR frames
  std::vector<bool> classesPresenceOverMFrames(NUM_CLASSES, false);
  std::vector<bool> classesAbsenceOverMFrames(NUM_CLASSES, false);
  bool isClassPresent;
  bool isClassAbsent;
  for (uint8_t i = 0; i < NUM_CLASSES; i++)
  {
     isClassPresent = true;
     isClassAbsent = true;
     for (uint8_t frame = g_lastNFramesCount+NUM_ACC_FRAMES-NUM_ACC_FRAMES_FOR_CURR_CLASS_TO_APPEAR;
             frame < g_lastNFramesCount+NUM_ACC_FRAMES; frame++) {
        isClassPresent &= g_lastNFramesClassAppearance[frame%NUM_ACC_FRAMES][i];
        isClassAbsent &= !g_lastNFramesClassAppearance[frame%NUM_ACC_FRAMES][i];
     }
     classesPresenceOverMFrames[i] = isClassPresent;
     classesAbsenceOverMFrames[i] = isClassAbsent;
  }

  //Store classes which made to text overlays last frame
  std::vector<bool> classesDisplayedLastFrame(NUM_CLASSES, false);
  for(uint8_t i=0; i<g_sortedClasses.size(); i++)
     classesDisplayedLastFrame[g_sortedClasses[i]] = true;

  //Clear out classes displayed last frame
  g_sortedClasses.clear();

  //Now sort classesAccOverNFrames
  bool nonZeroPresent;
  uint8_t highOccurenceValue;
  uint8_t highOccurenceClass;

  nonZeroPresent = true;
  while (nonZeroPresent)
  {
     highOccurenceValue = 0;
     highOccurenceClass = 0;
     nonZeroPresent = false;

     //find the class with high occurence
     for (uint8_t i = 0; i < NUM_CLASSES; i++)
     {
        if (classesAccOverNFrames[i] > highOccurenceValue)
        {
           highOccurenceValue = classesAccOverNFrames[i];
           highOccurenceClass = i;
           nonZeroPresent = true;
        }
     }

     //if we have at least one class with (+)ve occurence
     if (nonZeroPresent)
     {
        //OFF->ON: if class couldn't made to text overlays last frame but present in last M Frames
        if (!classesDisplayedLastFrame[highOccurenceClass] && classesPresenceOverMFrames[highOccurenceClass])
           g_sortedClasses.push_back(highOccurenceClass);
        //OFF->OFF: if class couldn't made to text overlays last frame but not present in all last M Frames
        //ON->OFF: if class made to text overlays last frame but absent in all last M Frames
        //ON->ON: if class made to text overlays last frame but present in at least one of last M Frames
        else if (classesDisplayedLastFrame[highOccurenceClass] && !classesAbsenceOverMFrames[highOccurenceClass])
           g_sortedClasses.push_back(highOccurenceClass);

        //Reset the counter of highOccurenceClass, so next highOccurenceClass can appear
        classesAccOverNFrames[highOccurenceClass] = 0;
     }
  }
}

void StableDiffusionHelper::ReadPixelMapFile(const std::string& classPath)
{
  std::ifstream infile(classPath);
  std::string line;

  if ( infile.peek() == std::ifstream::traits_type::eof() )
  {
      DEMO_WARN("ClassFile is empty, something went wrong when copying!" );

  }
  // we will need to first throw away the comments
  while (std::getline(infile, line)){
      // this is the exit condition: an empty line
      if (line == "") { break; }
  }

  // now we can start to populate the map
  while (std::getline(infile, line)) {
      //just incase we have trailing new lines :)
      if (line != "") {
          //Checking if line has been commented
          if (line[0] != '#') {
              
              // DEMO_DEBUG("%s: Parsing line %s", __FUNCTION__, line.c_str() );
              // this will return the index, class name and the colors
              std::vector<std::string> index = Helpers::split(line, '=');
              std::vector<std::string> colors = Helpers::split(index[2], ',');
              // colors.push_back("255"); // -> this for the alpha value to be used in post processing

              // populate map with index as key and the vector of int32_t as value
              // DEMO_DEBUG("%s: toInt %s", __FUNCTION__, index[0].c_str() );
              uint8_t classIndex = toInt(index[0]);
              std::vector<uint8_t> colorsInt = toInt(colors);

              m_PixelMapMat.at<cv::Vec3b>(0,classIndex) = cv::Vec3b(colorsInt[0], colorsInt[1], colorsInt[2]);
              m_ClassesName[classIndex] = index[1];
              m_ClassesHexColor[classIndex] = rgb2hex(colorsInt);
              m_allowedClasses[classIndex] = true;
          }
      }
  }

  cv::multiply(m_PixelMapMat, 0.65, m_PixelMapMat65Percent);
}

cv::Mat StableDiffusionHelper::CalculateQuantPixelMapping(double scale, int32_t offset,
                                   std::vector<double> image_mean, std::vector<double> image_std)
{
    cv::Mat pixelMapMat(1, 256, CV_8UC3, cv::Scalar(0, 0, 0));

    double processedValue[3];
    // Since 8 bits can range from 0 to 255
    for (int i=0; i<256; i++) {
        // 3 channels
        for (uint8_t c=0; c<3; c++) {
            // Normalizing each possible value
            processedValue[c] = (double(i)/255.0 - image_mean[c])/image_std[c];
            // Quantizing each possible value
            processedValue[c] = processedValue[c]/scale - offset;
            processedValue[c] = processedValue[c] < 0.0   ?   0.0 :
                                processedValue[c] > 255.0 ? 255.0 : processedValue[c];
        }
        pixelMapMat.at<cv::Vec3b>(0,i) = cv::Vec3b(processedValue[0], processedValue[1], processedValue[2]);
    }

    return pixelMapMat;
}

cv::Mat StableDiffusionHelper::CalculateDequantPixelMapping(double scale, int32_t offset)
{
    cv::Mat pixelMapMat(1, 65536, CV_8UC1, cv::Scalar(0));

    double processedValue;
    // Since 16 bits can range from 0 to 65535
    for (int i=0; i<65536; i++) {
        processedValue = (double(i) + offset) * scale * 255.0;
        pixelMapMat.at<uchar>(0, i) = processedValue < 0.0   ? 0 :
                                      processedValue > 255.0 ? 255 : (uint8_t)processedValue;
    }

    return pixelMapMat;
}

////////////// End of Custom Helpers //////////////

int32_t StableDiffusionHelper::Init
(
  std::string& configFilePath,
  Helpers::ImageDims& inputImageDims,
  std::unordered_map<std::string, Helpers::ImageDims>& modelInputsImageDims,
  Helpers::QuantParameters& inputQuantParam,
  Helpers::QuantParameters& outputDequantParam, 
  std::unordered_map<std::string, Helpers::ImageDims>& modelOutputsImageDims,
  Helpers::ImageDims& outputImageDims
)
{
    std::map<std::string, std::string> kvpMap;
    std::vector<std::string> requiredKvp;

    // Lets first parse out config_file.txt file to its a key/value pair for processing
    Helpers::readKeyValueTxtFile(configFilePath, kvpMap);
    
    // Populate the requiredKvp with the items we require from the injected.properties file
    requiredKvp.push_back("output_tensor_name");

    // Lets check if our model.txt file had all the requirements
    if (!Helpers::checkKeysPresent(kvpMap, requiredKvp)){
        DEMO_ERROR("Injected.properties file is missing required key");
        return -1;
    }

    m_OutputTensorName = Helpers::split(kvpMap["output_tensor_name"], ':')[1];

    if (modelOutputsImageDims.find(m_OutputTensorName) == modelOutputsImageDims.end()) {
        DEMO_ERROR("model output %s is not found in modelOutputsImageDims", m_OutputTensorName.c_str());
        return -1;
    }

    m_inputImageDims = inputImageDims;
    // m_modelInputImageDims = modelInputsImageDims[m_InputTensorName];
    m_modelOutputImageDims = modelOutputsImageDims[m_OutputTensorName];
    m_outputImageDims = outputImageDims;

    // Lets check if input and/or output qunatization/dequantization required.
    // If yes, calculate pixel mapping map
    m_inputQuantActive = false;
    if (inputQuantParam.active) {
        m_inputQuantActive = true;
        m_inputQuantMapMat = CalculateQuantPixelMapping(inputQuantParam.scale, inputQuantParam.offset,
                                                        {0.485, 0.456, 0.406}, {0.229, 0.224, 0.225});
    }
    m_outputDequantActive = false;
    if (outputDequantParam.active) {
        m_outputDequantActive = true;
        m_outputDequantMapMat = CalculateDequantPixelMapping(outputDequantParam.scale, outputDequantParam.offset);
    }

    // Mats for custom post-processing
    m_outputImagePost = cv::Mat(m_outputImageDims.height, m_outputImageDims.width, 
                                CV_8UC((uint32_t)m_outputImageDims.channel));

    return 0;
}

// bool PreProcessInput (const void* inputData,
//                       bool isFlipped,
//                       void* preprocessedData
//                      )
// {
//     m_inputImageMatPre.data = (unsigned char*)inputData;
//     m_processedImageMatPre.data = (unsigned char*)preprocessedData;

// #ifdef PRE_PROCESS_OLD_IMPL
//     if (m_inputQuantActive) {
//         cv::LUT(m_inputImageMatPre, m_inputQuantMapMat, m_processedImageMatPre);
//     } else if (inputData != preprocessedData) {
//         std::memcpy(m_processedImageMatPre.data, m_inputImageMatPre.data,
//                     m_inputImageMatPre.total() * m_inputImageMatPre.elemSize());
//     }
// #else
//     if (inputData != preprocessedData) {
//         std::memcpy(m_processedImageMatPre.data, m_inputImageMatPre.data,
//                     m_inputImageMatPre.total() * m_inputImageMatPre.elemSize());
//     }
//     if (m_inputQuantActive) {
//         m_processedImageMatPre.forEach<cv::Vec3b>([&](cv::Vec3b& orgPixel, const int * position) -> void {
//             orgPixel[0] = m_inputQuantMapMat.at<cv::Vec3b>(0, orgPixel[0])[0];
//             orgPixel[1] = m_inputQuantMapMat.at<cv::Vec3b>(0, orgPixel[1])[1];
//             orgPixel[2] = m_inputQuantMapMat.at<cv::Vec3b>(0, orgPixel[2])[2];
//         });
//     }
// #endif

//     return true;
// }


bool StableDiffusionHelper::PostProcessOutput
(
    std::unordered_map<std::string, void*>& modelOutputData,
    const void* orgInputData,
    Helpers::StagesData& postStageData,
    bool showLabels,
    bool showConfScores,
    Helpers::InferenceReturn &returnVal,
    bool dumpPostOutput,
    std::string& outputLocation
)
{
    // Let's get the result of our layer
    // Check whether the layer exists
    if (modelOutputData.find(m_OutputTensorName) == modelOutputData.end()) {
        returnVal.m_PostProcessedImageSize = 0;
        DEMO_ERROR("%s: Unable to find output layer %s in userBufferOutputMap. Aborting...",
                            __FUNCTION__, m_OutputTensorName.c_str());
        return false;
    }

    {
        auto start = std::chrono::steady_clock::now();
        cv::Mat modelOutputImagePost = cv::Mat(m_modelOutputImageDims.height,
                                               m_modelOutputImageDims.width,
                                               CV_16UC3, (char*)modelOutputData[m_OutputTensorName]);
        if (m_outputDequantActive) {
            m_outputImagePost.forEach<cv::Vec4b>([&](cv::Vec4b& orgPixel, const int * position) -> void {
                orgPixel[0] = m_outputDequantMapMat.at<uchar>(0, modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[0]);
                orgPixel[1] = m_outputDequantMapMat.at<uchar>(0, modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[1]);
                orgPixel[2] = m_outputDequantMapMat.at<uchar>(0, modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[2]);
                orgPixel[3] = 255;
            });
        } else {
            m_outputImagePost.forEach<cv::Vec4b>([&](cv::Vec4b& orgPixel, const int * position) -> void {
                orgPixel[0] = (uchar)(modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[0]);
                orgPixel[1] = (uchar)(modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[1]);
                orgPixel[2] = (uchar)(modelOutputImagePost.at<cv::Vec3w>(position[0], position[1])[2]);
                orgPixel[3] = 255;
            });
        }

        auto stop = std::chrono::steady_clock::now();
        Helpers::logProfile("PostProcessOutput: conversion from 16UC3 to 8UC4 (cpp) took", start, stop);
    }

    static std::vector<std::string> legend_strs(4);
    if (showLabels) {
        int index = 0;
        // legend_strs[index++] = "<font size=6 color=#FFFFFF><b><u>Bit Rate:</u></b></font> <br>";
        // legend_strs[index++] = "<font color=#FFFFFF>" + std::string(bitrate_str) +" Mbps" + "</font> <br><br><br>";
        // legend_strs[index++] = "<font size=6 color=#FFFFFF><b><u>Average bits/pixel:</u></b></font> <br>";
        // legend_strs[index++] = "<font color=#FFFFFF>" + std::string(bbp_str) + "</font> <br><br><br>";
    }

    returnVal.m_Scenes = legend_strs;
    returnVal.m_ImageData = m_outputImagePost.data;
    returnVal.m_PostProcessedImageSize = m_outputImagePost.total() * m_outputImagePost.elemSize();

    return true;
}
