
#pragma once

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "IBufferAlloc.hpp"
#include "QnnTypeDef.hpp"
#include "Log.hpp"
#include "QnnBackend.h"
#include "QnnCommon.h"
#include "QnnContext.h"
#include "QnnGraph.h"
#include "QnnInterface.h"
#include "QnnProperty.h"
#include "QnnTensor.h"
#include "QnnTypes.h"

enum class BufferAlloc {
  DEFAULT,        // malloc based allocator
  SHARED_BUFFER,  // shared buffer allocator; actual allocator depends on the platform
  INVALID
};

class IBufferAlloc;

class IOTensor {
 public:
  IOTensor(BufferAlloc bufferAllocIn            = BufferAlloc::DEFAULT,
           QNN_INTERFACE_VER_TYPE *qnnInterface = nullptr);

  bool initialize(Qnn_ContextHandle_t contextHandle = nullptr);

  bool setupInputTensors(Qnn_Tensor_t **inputs,
                         std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
                         const GraphInfo_t& graphInfo,
                         std::unordered_map<std::string, size_t>& inputTensorsSize);

  bool setupOutputTensors(Qnn_Tensor_t **outputs,
                          std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
                          const GraphInfo_t& graphInfo,
                          std::unordered_map<std::string, size_t>& outputTensorsSize);

  bool tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount);

  bool tearDownTensors(std::vector<Qnn_Tensor_t*>& tensors, uint32_t tensorCount);

  bool tearDownTensors(std::unordered_map<std::string, Qnn_Tensor_t*>& tensors,
                       std::unordered_map<std::string, uint32_t>& tensorCountMap);

  bool tearDownTensors(std::vector<std::unordered_map<std::string, Qnn_Tensor_t*>>& tensors,
                       std::unordered_map<std::string, uint32_t>& tensorCountMap);

  void* getBuffer(Qnn_Tensor_t *tensor) { return m_bufferManager->getBuffer(tensor); };

  int getFd(Qnn_Tensor_t *tensor) { return m_bufferManager->getFd(tensor); };

  size_t getBufferSize(Qnn_Tensor_t *tensor) { return m_bufferManager->getBufferSize(tensor); };

  bool useSameMemory(Qnn_Tensor_t* dest, Qnn_Tensor_t* src) { return m_bufferManager->useSameMemory(dest, src); }

  std::unordered_set<void*>& getFreeTensorsPointerSet() { return m_freeTensorsPointerSet; }

 private:
  BufferAlloc m_bufferAlloc;
  QNN_INTERFACE_VER_TYPE *m_qnnInterface;
  std::unique_ptr<IBufferAlloc> m_bufferManager;
  std::unordered_set<void*> m_freeTensorsPointerSet;

  bool deepCopyQnnTensorInfo(Qnn_Tensor_t *dest, Qnn_Tensor_t *src);

  bool setupTensors(Qnn_Tensor_t **tensors,
                    std::unordered_map<std::string, void*>& tensorNameToTensorPointer,
                    uint32_t tensorCount,
                    TensorWrapper *tensorsInfo,
                    std::unordered_map<std::string, size_t>& tensorsSize);
};
