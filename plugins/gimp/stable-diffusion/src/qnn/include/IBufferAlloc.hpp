/*
**************************************************************************************************
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
**************************************************************************************************
*/

#pragma once

#include "QnnTypes.h"

class IBufferAlloc {
 public:
  virtual ~IBufferAlloc() {}
  IBufferAlloc() {}
  virtual bool initialize()                                = 0;
  virtual void* getBuffer(Qnn_Tensor_t* tensor)            = 0;
  virtual int getFd(Qnn_Tensor_t *tensor)                  = 0;
  virtual size_t getBufferSize(Qnn_Tensor_t *tensor)       = 0;
  virtual bool allocateTensorBuffer(Qnn_Tensor_t* tensor,
                                    size_t tensorDataSize) = 0;
  virtual bool freeTensorBuffer(Qnn_Tensor_t* tensor)      = 0;
  virtual bool useSameMemory(Qnn_Tensor_t* dest,
                             Qnn_Tensor_t* src)            = 0;
};