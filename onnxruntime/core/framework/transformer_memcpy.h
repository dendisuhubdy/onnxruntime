// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/common/common.h"
#include "core/framework/op_kernel.h"
#include "core/framework/kernel_registry_manager.h"
#include "core/graph/graph_transformer.h"

namespace onnxruntime {

// implements MemCpy node insertion in graph transform
class MemcpyTransformer : public GraphTransformer {
 public:
  MemcpyTransformer(const std::vector<std::string>& provider_types, const KernelRegistryManager& registry_manager)
      : GraphTransformer("MemcpyTransformer", "Insert nodes to copy memory between devices when needed"),
        provider_types_{provider_types},
        registry_manager_{registry_manager} {}

 private:
  common::Status ApplyImpl(Graph& graph, bool& modified, int graph_level) const override;

  const std::vector<std::string> provider_types_;
  const KernelRegistryManager& registry_manager_;
};

}  // namespace onnxruntime
