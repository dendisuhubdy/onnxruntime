// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/graph/slice_elimination.h"
#include "core/graph/graph.h"
#include "core/graph/graph_utils.h"
#include "core/graph/op.h"

namespace onnxruntime {

Status EliminateSlice::Apply(Graph& graph, Node& node, bool& modified, bool& removed) {
  if (utils::RemoveSingleInSingleOutNode(graph, node)) {
    removed = modified = true;
  }

  return Status::OK();
}

bool EliminateSlice::SatisfyCondition(const Node& node) {
  // At the moment, we eliminate a slice operator only if it has a single input and a single output.
  if (!utils::IsSingleInSingleOutNode(node)) {
    return false;
  }

  std::vector<int64_t> starts;
  std::vector<int64_t> ends;
  if (!utils::GetRepeatedNodeAttributeValues(node, "starts", starts) ||
      !utils::GetRepeatedNodeAttributeValues(node, "ends", ends) ||
      starts.size() != ends.size()) {
    return false;
  }
  std::vector<int64_t> axes;
  if (!utils::GetRepeatedNodeAttributeValues(node, "axes", axes)) {
    for (int i = 0; (size_t)i < starts.size(); ++i) {
      axes.push_back(i);
    }
  } else if (axes.size() != starts.size() || axes.size() != ends.size()) {
    return false;
  }

  // For now eliminate slice operators if starts=0 and ends=MAX_INT or -1.
  // TODO: Take into account the input's shape to get a tighter bound for the ends.
  for (int i = 0; i < axes.size(); ++i) {
    if (starts[i] > 0 || starts[i] < 0 ||
        (ends[i] > 0 && ends[i] < INT64_MAX)) {
      return false;
    }
  }

  return true;
}

}  // namespace onnxruntime
