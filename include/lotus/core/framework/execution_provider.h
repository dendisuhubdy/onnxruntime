#pragma once

#include <unordered_map>
#include "core/common/status.h"
#include "core/framework/arena.h"
#include "core/framework/tensor.h"

namespace LotusIR {
class GraphTransformer;
class Node;
}  // namespace LotusIR
namespace Lotus {
class KernelRegistry;
class OpKernelContext;

// Logical device representation.
typedef std::map<MemType, AllocatorPtr> AllocatorMap;

class IExecutionProvider {
 public:
  virtual ~IExecutionProvider() = default;

  // Get all IAllocators for <*this> execution provider.
  const AllocatorMap& GetAllocatorMap() const {
    return allocators_;
  }

  // Get allocator with specified MemType
  virtual AllocatorPtr GetAllocator(MemType mem_type = kMemTypeDefault) const {
    if (allocators_.count(mem_type) > 0)
      return allocators_.at(mem_type);
    else
      return nullptr;
  }

  // Get kernel registry per execution provider type.
  // The KernelRegistry share pointer returned is shared across sessions.
  //
  // NOTE: this is a tricky but final solution to acheive following goals,
  // 1. The execution provider type based kernel registry should be shared across sessions.
  //    Only one copy of this kind of kernel registry exists in lotus runtime with multiple sessions/models.
  // 2. Adding an execution provider into lotus does not need to touch lotus frameowrk/session code.
  // 3. Lotus runtime (framework/session) does not depend on any specific execution provider lib.
  virtual std::shared_ptr<KernelRegistry> GetKernelRegistry() const = 0;

  // Copy tensor between execution providers
  virtual Common::Status CopyTensor(const Tensor& src, Tensor& dst) const = 0;

  // Copy tensor between execution providers on specified exec queue
  virtual Common::Status CopyTensor(const Tensor& src, Tensor& dst, int exec_queue_id) const {
    // execution provider may override this to support different exec queues
    LOTUS_ENFORCE(exec_queue_id == 0);
    return CopyTensor(src, dst);
  }

  // Returns an opaque handle whose exact type varies based on the provider
  // and is interpreted accordingly by the corresponding kernel implementation.
  // For Direct3D operator kernels, this may return an IUnknown supporting
  // QueryInterface to ID3D12GraphicsCommandList1.
  virtual const void* GetExecutionHandle() const noexcept = 0;

  // @return type of the execution provider; should match that set in the node through
  // the SetExecutionProvider API.
  // Example valid return values are: kCpuExecutionProvider, kCudaExecutionProvider
  virtual std::string Type() const = 0;

  /**
  * Blocks until the device has completed all preceding requested tasks.
  * Currently this is primarily used by the IOBinding object to ensure that all inputs have been
  * copied to the device before execution begins.
  */
  virtual Common::Status Sync() {
    return Status::OK();
  };

  // Called when InferenceSession::Run started
  // NOTE that due to async execution in provider, the actual work of previous Run may not be finished on device
  // This function should be regarded as the point after which a new Run would start to submit commands from CPU
  virtual Common::Status OnRunStart() {
    return Status::OK();
  }

  // Called when InferenceSession::Run ended
  // NOTE that due to async execution in provider, the actual work of this Run may not be finished on device
  // This function should be regarded as the point that all commands of current Run has been submmited by CPU
  virtual Common::Status OnRunEnd() {
    return Status::OK();
  }

  void InsertAllocator(MemType mem_type, AllocatorPtr allocator) {
    allocators_.insert({mem_type, allocator});
  }

 private:
  AllocatorMap allocators_;
};
}  // namespace Lotus