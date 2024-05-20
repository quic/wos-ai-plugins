#pragma once

#include <unordered_map>
#include <unordered_set>
#include <dlfcn.h>

#include "IBufferAlloc.hpp"
#include "QnnInterface.h"
#include "Log.hpp"

typedef void *(*RpcMemAllocFn_t)(int, uint32_t, int);
typedef void (*RpcMemFreeFn_t)(void *);
typedef int (*RpcMemToFdFn_t)(void *);

class RpcMem final : public IBufferAlloc {
 public:
  RpcMem(Qnn_ContextHandle_t contextHandle, QNN_INTERFACE_VER_TYPE *qnnInterface);

  // Disable copy constructors, r-value referencing, etc
  RpcMem(const RpcMem &) = delete;

  RpcMem &operator=(const RpcMem &) = delete;

  RpcMem(RpcMem &&) = delete;

  RpcMem &operator=(RpcMem &&) = delete;

  bool initialize() override;

  void *getBuffer(Qnn_Tensor_t *tensor) override;

  int getFd(Qnn_Tensor_t *tensor) override;

  size_t getBufferSize(Qnn_Tensor_t *tensor) override;

  bool allocateTensorBuffer(Qnn_Tensor_t *tensor, size_t tensorDataSize) override;

  bool freeTensorBuffer(Qnn_Tensor_t *tensor) override;

  bool useSameMemory(Qnn_Tensor_t* dest, Qnn_Tensor_t* src) override;

  virtual ~RpcMem();

 private:
  struct RpcMemTensorData {
    int fd;
    void *memPointer;
    size_t size;
    RpcMemTensorData() : fd(-1), memPointer(nullptr), size(0) {}
    RpcMemTensorData(int fdIn, void *memPointerIn, size_t sizeIn)
        : fd(fdIn), memPointer(memPointerIn), size(sizeIn) {}
  };

  // Pointer to the dlopen'd libcdsprpc.so shared library which contains
  // rpcmem_alloc, rpcmem_free, rpcmem_to_fd APIs
  void *m_libCdspRpc;
  // Function pointer to rpcmem_alloc
  RpcMemAllocFn_t m_rpcMemAlloc;
  // Function pointer to rpcmem_free
  RpcMemFreeFn_t m_rpcMemFree;
  // Function pointer to rpcmem_to_fd
  RpcMemToFdFn_t m_rpcMemToFd;
  QNN_INTERFACE_VER_TYPE *m_qnnInterface;
  Qnn_ContextHandle_t m_contextHandle;

  std::unordered_map<Qnn_Tensor_t*, RpcMemTensorData> m_tensorToRpcMem;
  std::unordered_set<Qnn_Tensor_t*> m_sameMemoryFreeTensors;
};
