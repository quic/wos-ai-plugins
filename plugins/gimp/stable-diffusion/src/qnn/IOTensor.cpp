// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include <cstring>
#include <fstream>
#include <iostream>

#include "ClientBuffer.hpp"
#include "IBufferAlloc.hpp"
#include "IOTensor.hpp"
#include "RpcMem.hpp"
#include "QnnTypeMacros.hpp"

IOTensor::IOTensor(BufferAlloc bufferAllocIn, QNN_INTERFACE_VER_TYPE* qnnInterface)
    : m_bufferAlloc(bufferAllocIn),
      m_qnnInterface(qnnInterface),
      m_bufferManager(new ClientBuffer()) {}

bool IOTensor::initialize(Qnn_ContextHandle_t contextHandle) {
  if (m_bufferAlloc == BufferAlloc::SHARED_BUFFER) {
    m_bufferManager = std::unique_ptr<IBufferAlloc>(new RpcMem(contextHandle, m_qnnInterface));
  }

  if (true != m_bufferManager->initialize()) {
    QNN_ERROR("Failed to initialize buffer manager");
    return false;
  }

  return true;
}

// Setup details for Qnn_Tensor_t for execution
// based on information in TensorWrapper provided by model.so.
bool IOTensor::setupTensors(
    Qnn_Tensor_t** tensors,
    std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
    uint32_t tensorCount,
    TensorWrapper* tensorWrappers,
    std::unordered_map<std::string, size_t>& tensorsSize) {

  if (nullptr == tensorWrappers) {
    QNN_ERROR("tensorWrappers is nullptr");
    return false;
  }
  if (0 == tensorCount) {
    QNN_DEBUG("tensor count is 0. Nothing to setup.");
    return true;
  }

  *tensors          = (Qnn_Tensor_t*)calloc(1, tensorCount * sizeof(Qnn_Tensor_t));
  if (nullptr == *tensors) {
    QNN_ERROR("mem alloc failed for *tensors");
    return false;
  }

  auto returnStatus = true;
  for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
    Qnn_Tensor_t wrapperTensor = GET_TENSOR_WRAPPER_TENSOR(tensorWrappers[tensorIdx]);
    auto wrapperTensorName = std::string(GET_TENSOR_WRAPPER_NAME(tensorWrappers[tensorIdx]));
    if (true == returnStatus) {
      (*tensors)[tensorIdx] = QNN_TENSOR_INIT;
      returnStatus = deepCopyQnnTensorInfo(((*tensors) + tensorIdx), &wrapperTensor);
    }
    if (true == returnStatus) {
      size_t tensorDataSize = tensorsSize[wrapperTensorName];
      returnStatus = m_bufferManager->allocateTensorBuffer(((*tensors) + tensorIdx), tensorDataSize);
    }
    if (true != returnStatus) {
      QNN_ERROR("Failure in setupTensors, cleaning up resources");
      tearDownTensors(*tensors, tensorIdx);
      *tensors     = nullptr;
      QNN_ERROR("Failure in setupTensors, done cleaning up resources");
      return false;
    } else {
      tensorNameToTensorPointer.insert({wrapperTensorName, ((*tensors) + tensorIdx)});
      QNN_DEBUG("allocateBuffer successful");      
    }
  }

  return returnStatus;
}

// Setup details for all input tensors for graph execution.
bool IOTensor::setupInputTensors(
    Qnn_Tensor_t** inputs,
    std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
    const GraphInfo_t& graphInfo, std::unordered_map<std::string, size_t>& inputTensorsSize) {

  if (true !=
      setupTensors(inputs, tensorNameToTensorPointer, graphInfo.numInputTensors,
        (graphInfo.inputTensors), inputTensorsSize)) {
    QNN_ERROR("Failure in setupInputTensors, cleaning up resources");
    if (nullptr != *inputs) {
      QNN_DEBUG("cleaning up input tensors");
      tearDownTensors(*inputs, graphInfo.numInputTensors);
      *inputs = nullptr;
    }
    QNN_ERROR("Failure in setupInputTensors, done cleaning up resources");

    return false;
  }

  return true;
}

// Setup details for all output tensors for graph execution.
bool IOTensor::setupOutputTensors(
    Qnn_Tensor_t** outputs,
    std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
    const GraphInfo_t& graphInfo, std::unordered_map<std::string, size_t>& outputTensorsSize) {

  if (true !=
      setupTensors(outputs, tensorNameToTensorPointer, graphInfo.numOutputTensors,
        (graphInfo.outputTensors), outputTensorsSize)) {
    QNN_ERROR("Failure in setupOutputTensors, cleaning up resources");
    if (nullptr != *outputs) {
      QNN_DEBUG("cleaning up output tensors");
      tearDownTensors(*outputs, graphInfo.numOutputTensors);
      *outputs = nullptr;
    }
    QNN_ERROR("Failure in setupOutputTensors, done cleaning up resources");

    return false;
  }

  return true;
}

// Clean up all tensors related data after execution.
bool IOTensor::tearDownTensors(Qnn_Tensor_t* tensors, uint32_t tensorCount) {

  if (nullptr != tensors) {
    QNN_DEBUG("cleaning up resources for tensors");
    for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
      QNN_DEBUG("freeing resources for tensor: %zu", tensorIdx);
#ifdef QNN_ENABLE_API_2x
      if (nullptr != QNN_TENSOR_GET_DIMENSIONS(&tensors[tensorIdx])) {
        QNN_DEBUG("freeing maxDimensions");
        free(QNN_TENSOR_GET_DIMENSIONS(&tensors[tensorIdx]));
      }
#else
      if (nullptr != tensors[tensorIdx].maxDimensions) {
        QNN_DEBUG("freeing maxDimensions");
        free(tensors[tensorIdx].maxDimensions);
      }
      if (nullptr != tensors[tensorIdx].currentDimensions) {
        QNN_DEBUG("freeing currentDimensions");
        free(tensors[tensorIdx].currentDimensions);
      }
#endif
      m_bufferManager->freeTensorBuffer(&(tensors[tensorIdx]));
      m_freeTensorsPointerSet.insert(&(tensors[tensorIdx]));
    }
    free(tensors);
    tensors = nullptr;
  }

  return true;
}

// Clean up all tensors after execution.
bool IOTensor::tearDownTensors(std::vector<Qnn_Tensor_t*>& tensors,
                               uint32_t numTensors) {

  for (Qnn_Tensor_t* tensor : tensors) {
    tearDownTensors(tensor, numTensors);
  }

  return true;
}

// Clean up all tensors after execution.
bool IOTensor::tearDownTensors(std::unordered_map<std::string, Qnn_Tensor_t*>& tensors,
                               std::unordered_map<std::string, uint32_t>& tensorCountMap) {

  for (auto& tensor : tensors) {
    tearDownTensors(tensor.second, tensorCountMap[tensor.first]);
  }

  return true;
}

// Clean up all tensors after execution.
bool IOTensor::tearDownTensors(std::vector<std::unordered_map<std::string, Qnn_Tensor_t*>>& tensors,
                               std::unordered_map<std::string, uint32_t>& tensorCountMap) {

  for (auto& tensor : tensors) {
    tearDownTensors(tensor, tensorCountMap);
  }

  return true;
}

bool IOTensor::deepCopyQnnTensorInfo(Qnn_Tensor_t* dest, Qnn_Tensor_t* src) {

  if (nullptr == dest || nullptr == src) {
    QNN_ERROR("Received nullptr");
    return false;
  }

#ifdef QNN_ENABLE_API_2x
  // set tensor.version before using QNN_TENSOR_SET macros, as they require the version to be set
  // to correctly assign values
  dest->version = src->version;
#endif
  QNN_TENSOR_SET_ID(dest, QNN_TENSOR_GET_ID(src));
  QNN_TENSOR_SET_TYPE(dest, QNN_TENSOR_GET_TYPE(src));
  QNN_TENSOR_SET_DATA_FORMAT(dest, QNN_TENSOR_GET_DATA_FORMAT(src));
  QNN_TENSOR_SET_DATA_TYPE(dest, QNN_TENSOR_GET_DATA_TYPE(src));
  Qnn_QuantizeParams_t qParams = QNN_QUANTIZE_PARAMS_INIT;
  qParams.encodingDefinition   = QNN_TENSOR_GET_QUANT_PARAMS(src).encodingDefinition;
  qParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
  if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
      QNN_QUANTIZATION_ENCODING_SCALE_OFFSET) {
    qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
    qParams.scaleOffsetEncoding  = QNN_TENSOR_GET_QUANT_PARAMS(src).scaleOffsetEncoding;
  } else if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
             QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
    qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
    qParams.axisScaleOffsetEncoding.axis =
        QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.axis;
    qParams.axisScaleOffsetEncoding.numScaleOffsets =
        QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
    if (QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets > 0) {
      qParams.axisScaleOffsetEncoding.scaleOffset = (Qnn_ScaleOffset_t*)malloc(
          QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets *
          sizeof(Qnn_ScaleOffset_t));
      if (qParams.axisScaleOffsetEncoding.scaleOffset) {
        for (size_t idx = 0;
             idx < QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
             idx++) {
          qParams.axisScaleOffsetEncoding.scaleOffset[idx].scale =
              QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.scaleOffset[idx].scale;
          qParams.axisScaleOffsetEncoding.scaleOffset[idx].offset =
              QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.scaleOffset[idx].offset;
        }
      }
    }
  }
  QNN_TENSOR_SET_QUANT_PARAMS(dest, qParams);
  QNN_TENSOR_SET_RANK(dest, QNN_TENSOR_GET_RANK(src));
#ifdef QNN_ENABLE_API_2x
  QNN_TENSOR_SET_DIMENSIONS(dest, nullptr);
  if (QNN_TENSOR_GET_RANK(src) > 0) {
    QNN_TENSOR_SET_DIMENSIONS(dest, (uint32_t*)malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t)));
    if (QNN_TENSOR_GET_DIMENSIONS(dest)) {
      memcpy(QNN_TENSOR_GET_DIMENSIONS(dest),
             QNN_TENSOR_GET_DIMENSIONS(src),
             QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    }
  }
#else
  dest->maxDimensions     = nullptr;
  dest->currentDimensions = nullptr;
  if (QNN_TENSOR_GET_RANK(src) > 0) {
    dest->maxDimensions = (uint32_t*)malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    if (dest->maxDimensions) {
      memcpy(dest->maxDimensions,
             src->maxDimensions,
             QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    }
    dest->currentDimensions = (uint32_t*)malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    if (dest->currentDimensions) {
      memcpy(dest->currentDimensions,
             src->currentDimensions,
             QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    }
  }
#endif

  return true;
}
