#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "QnnInterface.h"
#include "QnnTypes.h"
#include "System/QnnSystemInterface.h"

#ifndef QNN_ENABLE_API_2x
#include "SampleAppContextCaching_generated.h"
#endif

#include "QnnTypeDef.hpp"
#include "Log.hpp"

/**
 * @brief Frees all memory allocated tensor attributes.
 *
 * @param[in] tensorWrapper tensor object to free
 *
 * @return Error code
 */
bool freeQnnTensorWrapper(TensorWrapper &tensorWrapper);

/**
 * @brief Loops through and frees all memory allocated tensor attributes for each tensorWrapper
 * object.
 *
 * @param[in] tensorWrappers array of tensor objects to free
 *
 * @param[in] numTensors length of the above tensorWrappers array
 *
 * @return Error code
 */
bool freeQnnTensorWrappers(TensorWrapper *&tensorWrappers, uint32_t numTensors);

/**
 * @brief A helper function to free memory malloced for communicating the Graph for a model(s)
 *
 * @param[in] graphsInfo Pointer pointing to location of graph objects
 *
 * @param[in] numGraphs The number of graph objects the above pointer is pointing to
 *
 * @return Error code
 *
 */
bool freeGraphsInfo(GraphInfoPtr_t **graphsInfo, uint32_t numGraphs);

bool freeGraphInfo(GraphInfo_t *graphInfo);

// Not needed for API 2x, tensor names are already part of Qnn_Tensor_t
#ifndef QNN_ENABLE_API_2x
bool populateTensorNamesFromMetadata(
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>>
        &graphTensorIdToNamesMap,
    GraphInfo_t **&graphsInfo,
    const uint32_t graphsCount);
#endif

bool copyMetadataToGraphsInfo(const QnnSystemContext_BinaryInfo_t *binaryInfo,
                              GraphInfo_t **&graphsInfo,
                              uint32_t &graphsCount);

bool copyGraphsInfo(const QnnSystemContext_GraphInfo_t *graphsInput,
                    const uint32_t numGraphs,
                    GraphInfo_t **&graphsInfo);

bool copyGraphsInfoV1(const QnnSystemContext_GraphInfoV1_t *graphInfoSrc,
                      GraphInfo_t *graphInfoDst);

bool copyTensorsInfo(const Qnn_Tensor_t *tensorsInfoSrc,
                     TensorWrapper *&tensorWrappers,
                     uint32_t tensorsCount);

#ifndef QNN_ENABLE_API_2x
bool deserializeData(
    std::string filePath,
    std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>>
        &graphTensorIdToNamesMap,
    uint32_t *graphsCount,
    std::shared_ptr<uint8_t> &binaryCache,
    uint64_t &binaryCacheSize);
#endif

bool fillDims(std::vector<size_t>& dims, uint32_t* inDimensions, uint32_t rank);
size_t getFileSize(std::string filePath);
bool readBinaryFromFile(std::string filePath, uint8_t* buffer, size_t bufferSize);

