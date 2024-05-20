#pragma once

#include "IBufferAlloc.hpp"
#include "Log.hpp"
#include <stdlib.h>

class ClientBuffer final : public IBufferAlloc {
 public:
  ClientBuffer(){};

  // Disable copy constructors, r-value referencing, etc
  ClientBuffer(const ClientBuffer &) = delete;

  ClientBuffer &operator=(const ClientBuffer &) = delete;

  ClientBuffer(ClientBuffer &&) = delete;

  ClientBuffer &operator=(ClientBuffer &&) = delete;

  bool initialize() override { return true; };

  void *getBuffer(Qnn_Tensor_t *tensor) override;

  int getFd(Qnn_Tensor_t *tensor) override { QNN_WARN("getFd: This is not ION memory"); return -1;};

  size_t getBufferSize(Qnn_Tensor_t *tensor) override;

  bool allocateTensorBuffer(Qnn_Tensor_t *tensor, size_t tensorDataSize) override;

  bool freeTensorBuffer(Qnn_Tensor_t *tensor) override;

  bool useSameMemory(Qnn_Tensor_t* dest, Qnn_Tensor_t* src) override;

  virtual ~ClientBuffer(){};
};
