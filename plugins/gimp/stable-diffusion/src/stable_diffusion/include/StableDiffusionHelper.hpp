#ifndef CUSTOMSHELPERS_H
#define CUSTOMSHELPERS_H

#include "Helpers.hpp"

static const int NUM_CLASSES = 19;
static int NUM_ACC_FRAMES = 10;
static int NUM_ACC_FRAMES_FOR_CURR_CLASS_TO_APPEAR = 1;

// #define PRE_PROCESS_OLD_IMPL
// #define PRE_PROCESS_NEW_IMPL

class StableDiffusionHelper {
public :

    StableDiffusionHelper();
    ~StableDiffusionHelper();

    int32_t Init(std::string& configFilePath,
        Helpers::ImageDims& inputImageDims,
        std::unordered_map<std::string, Helpers::ImageDims>& modelInputsImageDims,
        Helpers::QuantParameters& inputQuantParam,
        Helpers::QuantParameters& outputDequantParam,
        std::unordered_map<std::string, Helpers::ImageDims>& modelOutputsImageDims,
        Helpers::ImageDims& outputImageDims
    );

    // 
    // bool PreProcessInput (const void* inputData,
    //                       bool isFlipped,
    //                       void* preprocessedData
    //                      );


    bool PostProcessOutput(std::unordered_map<std::string, void*>& modelOutputData,
        const void* orgInputData,
        Helpers::StagesData& postStageData,
        bool showLabels,
        bool showConfScores,
        Helpers::InferenceReturn& returnVal,
        bool dumpPostOutput,
        std::string& outputLocation
    );

private:
    // Mapping from org Pixel to quant/dequant pixels
    cv::Mat m_inputQuantMapMat, m_outputDequantMapMat;
    bool m_inputQuantActive, m_outputDequantActive;

    // Overlays mapping from class num to color
    cv::Mat m_PixelMapMat;
    cv::Mat m_PixelMapMat65Percent;

    std::string m_InputTensorName, m_OutputTensorName;

    // cv::Mat m_inputImageMatPre, m_processedImageMatPre;

    cv::Mat m_outputImagePost;

    Helpers::ImageDims m_inputImageDims, m_modelInputImageDims, m_modelOutputImageDims, m_outputImageDims;

    bool m_outputTransposeRequested = false;

    std::unordered_map<uint8_t, std::string> m_ClassesName;
    std::unordered_map<uint8_t, std::string> m_ClassesHexColor;

    std::vector<bool> m_allowedClasses;

    //Keeping track of classes appearance in last NUM_ACC_FRAMES frames to reduce text-overlays flickering
    std::vector<std::vector<bool>> g_lastNFramesClassAppearance;
    uint8_t g_lastNFramesCount = 0;
    //Classes to show as Text Overlays
    std::vector<uint8_t> g_sortedClasses;

    void writeCvMatrix(const cv::Mat& myMat, const std::string& file_name);

    void ReadPixelMapFile(const std::string& classPath);

    // Function to pre-compute the mapping from org Pixel to pre-processed pixel
    cv::Mat CalculateQuantPixelMapping(double scale, int32_t offset, std::vector<double> image_mean, std::vector<double> image_std);
    cv::Mat CalculateDequantPixelMapping(double scale, int32_t offset);

    void sort_based_on_last_forty_occurance(const std::vector<bool>& segmentedClasses);

};
#endif  

