/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#ifndef Helpers_HPP_
#define Helpers_HPP_

#include <vector>
#include <map>
#include <istream>
#include <iterator>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstring>
#include <string>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <vector>
#include <thread>
#include <cstdio>
#include <unordered_map>
#include <dlfcn.h>
#include <dirent.h>
#include <uchar.h>
#include <sys/stat.h>
#include <direct.h>
#include <opencv2/core/core.hpp>
#include <opencv2/core/types_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/persistence.hpp>

// For recording error and debug info

#ifdef DEBUG_ENABLE
#define DEMO_ERROR(fmt, ...) printf("INFERENCE_ERROR: "#fmt "\n", ##__VA_ARGS__)
#define DEMO_WARN(fmt, ...) printf("INFERENCE_WARN: "#fmt "\n", ##__VA_ARGS__)
#define DEMO_DEBUG(fmt, ...) printf("INFERENCE_DEBUG: "#fmt "\n", ##__VA_ARGS__)
#else
#define DEMO_ERROR(fmt, ...)
#define DEMO_WARN(fmt, ...)
#define DEMO_DEBUG(fmt, ...)
#endif

class Helpers {
public:

    // To be used to return post-process output back to the Java Layer
    struct InferenceReturn{
        std::vector<std::string> m_Scenes = {};
        void* m_ImageData = nullptr;
        size_t m_PostProcessedImageSize = 0;
        int m_loop = 0;
    };

    struct ImageDims {
      int32_t height;
      int32_t width;
      float channel;
      int32_t bitWidth;

      ImageDims() : height(0), width(0), channel(0.0), bitWidth(0) {}

      ImageDims(int32_t height, int32_t width, float channel, int32_t bitWidth)
          : height(height), width(width), channel(channel), bitWidth(bitWidth) {}

      bool operator==(const ImageDims& rhs) const
      {
        return (height == rhs.height) && (width == rhs.width) &&
               (channel == rhs.channel) && (bitWidth == rhs.bitWidth);
      }

      bool operator!=(const ImageDims& rhs) const
      {
        return !(operator==(rhs));
      }

      size_t getImageSize() const
      {
        return (size_t)(height*width*channel*bitWidth);
      }

      ImageDims T() const
      {
        return ImageDims(width, height, channel, bitWidth);
      }
    };

    struct GenericMemory {
      typedef enum {HEAP, QNN, DMA} TENSOR_TYPES;

      TENSOR_TYPES tensorType;
      void* tensorMemPointer;
      void* memPointer;
      size_t memSize;
      int32_t memFd;

      GenericMemory()
          : tensorType(HEAP), tensorMemPointer(nullptr), memPointer(nullptr), memSize(0), memFd(-1) {}
      GenericMemory(void* memPointer, size_t memSize, int32_t memFd)
          : memPointer(memPointer), memSize(memSize), memFd(memFd) {}
      GenericMemory(void* memPointer, size_t memSize)
          : memPointer(memPointer), memSize(memSize), memFd(-1) {}
    };

    // Structure to keep track of data needed to be transferred across
    // stages
    struct StagesData {
        bool overlayOnImage = false;
        uint32_t loopNum = 0;
    };

    // Structure to store quantization parameters
    struct QuantParameters {
        bool active = false;
        double scale = 1.0; 
        int32_t offset = 0;
    };

    /**
     * @brief Class to calculate execution time statistics
     */
    class RunningGauge;

   /**
    * @brief For debugging Model Output, since we need to convert to string for android logcat
    */
    template <typename T> static std::string ConvertToString (T value){
        std::ostringstream buff;
        buff<<value;
        return buff.str();
    }

    template <typename T> static T toNum(const std::string& str)
    {
      char dummyChar;
      T i;
      std::stringstream ss(str);
      ss >> i;
      if ( ss.fail() || ss.get(dummyChar))
      {
         throw std::runtime_error("Failed to parse string " + str + " to a number");
      }

      return i;
    }

    template <typename T> static std::vector<T> toNums(const std::vector<std::string>& strs)
    {
      std::vector<T> nums;
      for (size_t i = 0; i < strs.size(); i++)
      {
         nums.push_back(Helpers::toNum<T>(strs[i]));
      }

      return nums;
    }

    /**
     * @brief Function to convert CV matrix to array
     */
    static std::vector<uchar> CvMatToArray(cv::Mat imageMat);

    static std::string stripPath(const std::string& path);

    static std::string joinPath(const std::string& path1, const std::string& path2);

    static bool fillDims(std::vector<size_t>& dims, uint32_t* inDimensions, uint32_t rank);

    static std::string getTimeBasedFileName(std::string basepath, std::string extension);

    /**
     * @brief Helper function to have the pull path for the files listed in the config file.
     */
    static std::string makePathAbsolute(const std::string& parentPath,
                                        const std::string& path
                                       );

    /**
     * @brief helper function to validate that all required are present in the app
     */
    static bool checkKeysPresent(std::map<std::string, std::string>& kvpMap,
                                 std::vector<std::string>& requiredKeys
                                );

    /**
     * @brief Helper Function for reading the Configuration file for the app
     * @param path path to the configuration file
     * @param kvpMap the key/value pair map for holding the configs
     */
    static void readKeyValueTxtFile(std::string &path,
                             std::map<std::string,
                             std::string>& kvpMap);

    /**
     * @brief The following two methods are used to split a string with a character delimiter
     */
    static std::vector<std::string> split(const std::string &s,
                                          char delim
                                         );
    static void split(const std::string &s,
                      char delim,
                      std::vector<std::string> &elems
                     );

    /**
     * @brief Helper function that reads threshold file and store it's values in a map for post-processing computation
     */
    static std::map<std::string,float> read_threshold_file(std::string threshold_path);

    static std::vector<std::string> readTextFile(const std::string& filename);
    static bool writeTextFile(const std::string& filename, const std::vector<std::string>& v_Text);

    /**
     * @brief Helper function to print log output of profiled code segment
    */
    template <typename T> static void logProfile(const char * message, const T startTime, const T endTime)
    {
         DEMO_DEBUG("%s: %lldus", message,
                              std::chrono::duration_cast<std::chrono::microseconds>(endTime-startTime).count());
    }

    enum class cpuCoreAffinity {
        ALL_CORES,
        BIG_CORES,
        LITTLE_CORES,

    };
    // sets the affinity for app using symphony library API
    // note: this only actually works when app is first launched per current testing
    static void SetCpuCoreAffinity(cpuCoreAffinity affinity);

    // get current CPU affinity on device
    static Helpers::cpuCoreAffinity GetCpuCoreAffinity();

    // creates all the dir and subdirs found in the path
    static bool CreateDirsIfNotExist(const std::string& path);

    // gets the filename component without extension from a given path
    static std::string GetFileNameFromPath(std::string filePath);

    // gets the extension of the filename from the given path
    static std::string GetFileExtensionFromPath(std::string filePath);

    // gets all the files in a given directory with optional extension
    static std::vector<std::string> FindAllFilesInDir(std::string dirPath, std::string extension="");

    // gets all the directories in a given directory
    static std::vector<std::string> FindAllDirsInDir(std::string dirPath);

    /**
     * @brief Helper function to print vector to file or stdout
    */
    template <typename T> static void logProfile(std::vector<T>, std::ostream_iterator<T> filename)
    {

    }
    
    // gets the file size
    static unsigned long GetFileSize(const std::string& filename);

    static bool writeRawData(void* tensorData, uint32_t tensorSize, const std::string& filename);
    static bool readRawData(void* tensorData, size_t tensorSize, const std::string& filename);
};


// Class to compute running statistics
// Reference: https://www.johndcook.com/blog/standard_deviation/
// TODO: This does not work properly and needs to be debugged
class Helpers::RunningGauge {
public:
  RunningGauge() {
    m_count = 0;
  }

  void Clear() {
    m_count = 0;
  }

  void Push(double newElem) {
    m_count++;

    // See Knuth TAOCP vol 2, 3rd edition, page 232
    if (m_count == 1) {
        m_oldMean = m_newMean = newElem;
        m_oldVarSum = 0.0;
        m_min = newElem;
        m_max = newElem;
    }
    else {
        m_min = std::min(newElem, m_min);
        m_max = std::max(newElem, m_max);
        m_newMean = m_oldMean + (newElem - m_oldMean)/m_count;
        m_newVarSum = m_oldVarSum + (newElem - m_oldMean)*(newElem - m_newMean);

        // set up for next iteration
        m_oldMean = m_newMean; 
        m_oldVarSum = m_newVarSum;
    }
    if ((m_count == 0) || ((m_count % 10) == 0)) {
      DEMO_DEBUG("STATS: newElem = %f, m_count=%d, m_min=%f, m_max=%f, m_newMean=%f", newElem, m_count, m_min, m_max, m_newMean);
    }
  }

  int NumDataValues() const {
    return m_count;
  }

  double Min() const {
    return m_min;
  }

  double Max() const {
    return m_max;
  }

  double Mean() const {
    return (m_count > 0) ? m_newMean : 0.0;
  }

  double Variance() const {
    return ( (m_count > 1) ? m_newVarSum/(m_count - 1) : 0.0 );
  }

  double StandardDeviation() const {
    return sqrt( Variance() );
  }

private:
  int m_count;
  double m_min;
  double m_max;
  double m_oldMean;
  double m_newMean;
  double m_oldVarSum;
  double m_newVarSum;
};

#endif //Helpers_H
