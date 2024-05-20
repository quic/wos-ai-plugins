// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include "ClientBuffer.hpp"
#include "QnnTypeMacros.hpp"

void* ClientBuffer::getBuffer(Qnn_Tensor_t* tensor) {
  if (!tensor) {
    QNN_WARN("getBuffer: received a null pointer to a tensor");
    return nullptr;
  }
  return QNN_TENSOR_GET_CLIENT_BUF(tensor).data;
}

size_t ClientBuffer::getBufferSize(Qnn_Tensor_t *tensor) {
  if (!tensor) {
    QNN_WARN("getBufferSize: received a null pointer to a tensor");
    return 0;
  }
  return QNN_TENSOR_GET_CLIENT_BUF(tensor).dataSize;
};

bool ClientBuffer::allocateTensorBuffer(Qnn_Tensor_t* tensor, size_t tensorDataSize) {
  if (!tensor) {
    QNN_ERROR("Received nullptr for tensors");
    return false;
  }
  QNN_TENSOR_SET_MEM_TYPE(tensor, QNN_TENSORMEMTYPE_RAW);
  Qnn_ClientBuffer_t clientBuffer;
  clientBuffer.data = malloc(tensorDataSize);
  if (nullptr == clientBuffer.data) {
    QNN_ERROR("mem alloc failed for clientBuffer.data");
    return false;
  }
  clientBuffer.dataSize = tensorDataSize;
  QNN_TENSOR_SET_CLIENT_BUF(tensor, clientBuffer);
  return true;
}

bool ClientBuffer::freeTensorBuffer(Qnn_Tensor_t* tensor) {
  if (!tensor) {
    QNN_ERROR("Received nullptr for tensors");
    return false;
  }
  if (QNN_TENSOR_GET_CLIENT_BUF(tensor).data) {
    free(QNN_TENSOR_GET_CLIENT_BUF(tensor).data);
    QNN_TENSOR_SET_CLIENT_BUF(tensor, Qnn_ClientBuffer_t({nullptr, 0u}));
    QNN_TENSOR_SET_MEM_TYPE(tensor, QNN_TENSORMEMTYPE_UNDEFINED);
  }
  return true;
}

bool ClientBuffer::useSameMemory(Qnn_Tensor_t* dest, Qnn_Tensor_t* src) {
  if (nullptr == dest || nullptr == src) {
    QNN_ERROR("Received nullptr");
    return false;
  }
  if (false == freeTensorBuffer(dest)) {
    return false;
  }

  QNN_TENSOR_SET_MEM_TYPE(dest, QNN_TENSOR_GET_MEM_TYPE(src));
  QNN_TENSOR_SET_CLIENT_BUF(dest, QNN_TENSOR_GET_CLIENT_BUF(src));
  return true;
}
