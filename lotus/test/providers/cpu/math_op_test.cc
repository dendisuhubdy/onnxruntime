#include "core/providers/cpu/math/clip.h"
#include "core/providers/cpu/math/element_wise_ops.h"
#include "core/providers/cpu/math/gemm.h"
#include "gtest/gtest.h"
#include "test/test_utils.h"

namespace Lotus {
namespace Test {
typedef std::vector<LotusIR::NodeArg*> ArgMap;

void SetupState(SessionState& state,
                const std::vector<LotusIR::NodeArg*>& input_defs,
                const std::vector<LotusIR::NodeArg*>& output_defs) {
  int idx = 0;
  for (auto& elem : input_defs) {
    state.AddMLValueNameIdx(elem->Name(), idx++);
  }
  for (auto& elem : output_defs) {
    state.AddMLValueNameIdx(elem->Name(), idx++);
  }
}

void FillFeedsAndOutputNames(const std::vector<LotusIR::NodeArg*>& input_defs,
                             const std::vector<LotusIR::NodeArg*>& output_defs,
                             std::unordered_map<std::string, MLValue>& feeds,
                             std::vector<std::string>& output_names) {
  for (auto& elem : input_defs) {
    feeds.insert(std::make_pair(elem->Name(), MLValue()));
  }
  for (auto& elem : output_defs) {
    output_names.push_back(elem->Name());
  }
}

TEST(MathOpTest, Clip) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg input_def("X", &tensor_float), output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&input_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Clip", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("min", -10.0f));
  EXPECT_TRUE(node->AddAttribute("max", 10.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Clip<float> kernel(info);

  std::vector<float> input_vals = {11.0f, 4.4f, 432.3f, -1.3f, 3.5f, 64.0f, -5.4f, 9.3f, 82.4f};
  std::vector<int64_t> dims = {3, 3};
  std::vector<float> expected_vals = {10.0f, 4.4f, 10.0f, -1.3f, 3.5f, 10.0f, -5.4f, 9.3f, 10.0f};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, dims, &input_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

TEST(MathOpTest, GemmNoTrans) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg x_def("X", &tensor_float),
      w_def("W", &tensor_float),
      b_def("B", &tensor_float),
      output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&x_def, &w_def, &b_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Gemm", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("transA", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("transB", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("broadcast", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("alpha", 1.0f));
  EXPECT_TRUE(node->AddAttribute("beta", 1.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Gemm<float, float, float, float> kernel(info);

  std::vector<float> x_vals = {1.0f, 2.0f, 3.0f, 4.0f, -1.0f, -2.0f, -3.0f, -4.0f};
  std::vector<int64_t> x_dims = {2, 4};
  std::vector<float> y_vals(12, 1.0f);
  std::vector<int64_t> y_dims = {4, 3};
  std::vector<float> b_vals(6, 1.0f);
  std::vector<int64_t> b_dims = {2, 3};
  std::vector<float> expected_vals = {11.0f, 11.0f, 11.0f, -9.0f, -9.0f, -9.0f};
  std::vector<int64_t> expected_dims = {2, 3};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, x_dims, &x_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 1, frame, y_dims, &y_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 2, frame, b_dims, &b_vals);
  EXPECT_TRUE(status.IsOK());

  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, expected_dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(expected_dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

TEST(MathOpTest, GemmBroadcast) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg x_def("X", &tensor_float),
      w_def("W", &tensor_float),
      b_def("B", &tensor_float),
      output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&x_def, &w_def, &b_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Gemm", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("transA", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("transB", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("broadcast", (int64_t)1));
  EXPECT_TRUE(node->AddAttribute("alpha", 1.0f));
  EXPECT_TRUE(node->AddAttribute("beta", 1.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Gemm<float, float, float, float> kernel(info);

  std::vector<float> x_vals = {1.0f, 2.0f, 3.0f, 4.0f, -1.0f, -2.0f, -3.0f, -4.0f};
  std::vector<int64_t> x_dims = {2, 4};
  std::vector<float> y_vals(12, 1.0f);
  std::vector<int64_t> y_dims = {4, 3};
  std::vector<float> b_vals(3, 1.0f);
  std::vector<int64_t> b_dims = {3};
  std::vector<float> expected_vals = {11.0f, 11.0f, 11.0f, -9.0f, -9.0f, -9.0f};
  std::vector<int64_t> expected_dims = {2, 3};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, x_dims, &x_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 1, frame, y_dims, &y_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 2, frame, b_dims, &b_vals);
  EXPECT_TRUE(status.IsOK());

  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, expected_dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(expected_dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

TEST(MathOpTest, GemmTrans) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg x_def("X", &tensor_float),
      w_def("W", &tensor_float),
      b_def("B", &tensor_float),
      output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&x_def, &w_def, &b_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Gemm", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("transA", (int64_t)1));
  EXPECT_TRUE(node->AddAttribute("transB", (int64_t)1));
  EXPECT_TRUE(node->AddAttribute("broadcast", (int64_t)1));
  EXPECT_TRUE(node->AddAttribute("alpha", 1.0f));
  EXPECT_TRUE(node->AddAttribute("beta", 1.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Gemm<float, float, float, float> kernel(info);

  std::vector<float> x_vals = {1.0f, -1.0f, 2.0f, -2.0f, 3.0f, -3.0f, 4.0f, -4.0f};
  std::vector<int64_t> x_dims = {4, 2};
  std::vector<float> y_vals(12, 1.0f);
  std::vector<int64_t> y_dims = {3, 4};
  std::vector<float> b_vals(3, 1.0f);
  std::vector<int64_t> b_dims = {3};
  std::vector<float> expected_vals = {11.0f, 11.0f, 11.0f, -9.0f, -9.0f, -9.0f};
  std::vector<int64_t> expected_dims = {2, 3};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, x_dims, &x_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 1, frame, y_dims, &y_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 2, frame, b_dims, &b_vals);
  EXPECT_TRUE(status.IsOK());

  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, expected_dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(expected_dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

TEST(MathOpTest, GemmAlphaBeta) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg x_def("X", &tensor_float),
      w_def("W", &tensor_float),
      b_def("B", &tensor_float),
      output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&x_def, &w_def, &b_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Gemm", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("transA", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("transB", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("broadcast", (int64_t)1));
  EXPECT_TRUE(node->AddAttribute("alpha", 0.5f));
  EXPECT_TRUE(node->AddAttribute("beta", 2.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Gemm<float, float, float, float> kernel(info);

  std::vector<float> x_vals = {1.0f, 2.0f, 3.0f, 4.0f, -1.0f, -2.0f, -3.0f, -4.0f};
  std::vector<int64_t> x_dims = {2, 4};
  std::vector<float> y_vals(12, 1.0f);
  std::vector<int64_t> y_dims = {4, 3};
  std::vector<float> b_vals(3, 1.0f);
  std::vector<int64_t> b_dims = {3};
  std::vector<float> expected_vals = {7.0f, 7.0f, 7.0f, -3.0f, -3.0f, -3.0f};
  std::vector<int64_t> expected_dims = {2, 3};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, x_dims, &x_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 1, frame, y_dims, &y_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 2, frame, b_dims, &b_vals);
  EXPECT_TRUE(status.IsOK());

  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, expected_dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(expected_dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

TEST(MathOpTest, GemmNaN) {
  TypeProto tensor_float;
  tensor_float.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  LotusIR::NodeArg x_def("X", &tensor_float),
      w_def("W", &tensor_float),
      b_def("B", &tensor_float),
      output_def("Y", &tensor_float);
  std::vector<LotusIR::NodeArg*> input_defs{&x_def, &w_def, &b_def};
  std::vector<LotusIR::NodeArg*> output_defs{&output_def};
  CREATE_NODE("Gemm", input_defs, output_defs);

  EXPECT_TRUE(node->AddAttribute("transA", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("transB", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("broadcast", (int64_t)0));
  EXPECT_TRUE(node->AddAttribute("alpha", 1.0f));
  EXPECT_TRUE(node->AddAttribute("beta", 0.0f));

  AllocatorInfo allocator_info("CPUAllocator", Lotus::AllocatorType::ArenaAllocator);
  KernelDef kernel_def;
  OpKernelInfo info(*node, allocator_info, kernel_def);
  Gemm<float, float, float, float> kernel(info);

  std::vector<float> x_vals = {1.0f, 2.0f, 3.0f, 4.0f, -1.0f, -2.0f, -3.0f, -4.0f};
  std::vector<int64_t> x_dims = {2, 4};
  std::vector<float> y_vals(12, 1.0f);
  std::vector<int64_t> y_dims = {4, 3};
  std::vector<float> b_vals(6, 1.0f);
  std::vector<int64_t> b_dims = {2, 3};
  std::vector<float> expected_vals = {10.0f, 10.0f, 10.0f, -10.0f, -10.0f, -10.0f};
  std::vector<int64_t> expected_dims = {2, 3};

  SessionState state;
  state.SetGraph(graph);
  SetupState(state, input_defs, output_defs);

  std::unordered_map<std::string, MLValue> feeds;
  std::vector<std::string> output_names;
  FillFeedsAndOutputNames(input_defs, output_defs, feeds, output_names);

  auto frame = TestUtils::CreateSingleNodeCPUExecutionFrame(state, feeds, output_names);
  auto status = TestUtils::PrepareIthInput<float>(*node, 0, frame, x_dims, &x_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 1, frame, y_dims, &y_vals);
  EXPECT_TRUE(status.IsOK());
  status = TestUtils::PrepareIthInput<float>(*node, 2, frame, b_dims, &b_vals);
  EXPECT_TRUE(status.IsOK());

  status = TestUtils::PrepareIthOutput<float>(*node, 0, frame, expected_dims);
  EXPECT_TRUE(status.IsOK());

  OpKernelContext kernel_ctx(frame.get(), static_cast<OpKernel*>(&kernel));
  //set Y to Nan to making sure NaN does not propagate when beta == 0
  float nan = static_cast<float>(std::nan("1"));
  float* out_buffer = kernel_ctx.output(0, TensorShape(expected_dims))->mutable_data<float>();
  for (int i = 0; i < expected_vals.size(); ++i) {
    out_buffer[i] = nan;
  }

  kernel.compute(&kernel_ctx);
  auto output = kernel_ctx.output(0, TensorShape(expected_dims));
  const float* res = output->data<float>();

  for (int i = 0; i < expected_vals.size(); ++i) {
    EXPECT_EQ(expected_vals[i], res[i]);
  }
}

struct TestGraph {
  TestGraph(const char* szName, const std::vector<LotusIR::NodeArg*>& inputDefs, const std::vector<LotusIR::NodeArg*>& outputDefs) {
    graph_->AddNode("node1", szName, szName, inputDefs, outputDefs);
  }

  operator LotusIR::Graph*() const { return graph_; }
  LotusIR::Graph* operator->() const { return graph_; }

 private:
  LotusIR::Model model_{"test", true};
  LotusIR::Graph* graph_{model_.MainGraph()};
};

template <template <typename> typename Op>
struct SimpleFloatTest {
  SimpleFloatTest(const char* szName, const std::vector<LotusIR::NodeArg*>& inputDefs, const std::vector<LotusIR::NodeArg*>& outputDefs)
      : graph_(szName, inputDefs, outputDefs) {
    graph_->Resolve();
    state_.SetGraph(graph_);
    SetupState(state_, inputDefs, outputDefs);

    std::unordered_map<std::string, MLValue> feeds;
    std::vector<std::string> output_names;
    FillFeedsAndOutputNames(inputDefs, outputDefs, feeds, output_names);
    frame_ = TestUtils::CreateSingleNodeCPUExecutionFrame(state_, feeds, output_names);
  }

  template <size_t count>
  void Run(const std::vector<int64_t>& expectedDims, const float (&expected_vals)[count]) {
    OpKernelContext kernel_ctx(frame_.get(), &kernel_);
    kernel_.compute(&kernel_ctx);
    auto& output = *kernel_ctx.output(0, TensorShape(expectedDims));
    Check(output, expected_vals);
  }

  template <size_t count>
  static void Check(Tensor& output, const float (&expected_vals)[count]) {
    LOTUS_ENFORCE(output.shape().Size() == count);
    const float* res = output.data<float>();
    for (int i = 0; i < count; ++i) {
      EXPECT_NEAR(expected_vals[i], res[i], 0.001f);
    }
  }

  void AddInput(const std::vector<int64_t>& dims, const std::vector<float>& values) {
    auto status = TestUtils::PrepareIthInput<float>(node_, inputCount_++, frame_, dims, &values);
    EXPECT_TRUE(status.IsOK());
  }

  void AddOutput(const std::vector<int64_t>& dims) {
    auto status = TestUtils::PrepareIthOutput<float>(node_, 0, frame_, dims, nullptr);
    EXPECT_TRUE(status.IsOK());
  }

  TestGraph graph_;
  LotusIR::Node& node_{*graph_->GetNode(graph_->NumberOfNodes() - 1)};
  AllocatorInfo allocator_info_{"CPUAllocator", Lotus::AllocatorType::ArenaAllocator};
  KernelDef kernel_def_;
  OpKernelInfo info_{node_, allocator_info_, kernel_def_};
  Op<float> kernel_{info_};
  SessionState state_;
  std::shared_ptr<ExecutionFrame> frame_;

  unsigned inputCount_{};
};

static struct TypeProto_Float : TypeProto {
  TypeProto_Float() {
    mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);
  }
} s_tensor_float;

TEST(MathOpTest, Add) {
  LotusIR::NodeArg input1_def("A", &s_tensor_float), input2_def("B", &s_tensor_float), output_def("C", &s_tensor_float);
  SimpleFloatTest<Add> test("Add", {&input1_def, &input2_def}, {&output_def});

  std::vector<int64_t> dims{3, 3};
  test.AddInput(dims, {1.0f, 2.0f, -1.0f, 0.0f, 1.5f, -100.0f, -5.4f, 9.3f, -10'000.0f});
  test.AddInput(dims, {-1.0f, 4.4f, 432.3f, 0.0f, 3.5f, 64.0f, -5.4f, 9.3f, 10'000.0f});
  test.AddOutput(dims);
  float expected_vals[] = {0.0f, 6.4f, 431.3f, 0.0f, 5.0f, -36.0f, -10.8f, 18.6f, 0.0f};
  test.Run(dims, expected_vals);
}

TEST(MathOpTest, Sum) {
  LotusIR::NodeArg input1_def("data_0", &s_tensor_float), input2_def("data_1", &s_tensor_float), input3_def("data_3", &s_tensor_float), output_def("sum", &s_tensor_float);
  SimpleFloatTest<Sum> test("Sum", {&input1_def, &input2_def, &input3_def}, {&output_def});

  std::vector<int64_t> dims{3, 3};
  test.AddInput(dims, {1.0f, 0.0f, 1.0f, -1.0f, 1.1f, -100.0f, -5.4f, 0.01f, -10'000.0f});
  test.AddInput(dims, {1.0f, 0.0f, 2.0f, -2.0f, 2.2f, 64.0f, -1.0f, 0.02f, 0.1f});
  test.AddInput(dims, {1.0f, 0.0f, 3.0f, -3.0f, 3.3f, 64.0f, 5.4f, 0.03f, 10'000.0f});
  test.AddOutput(dims);
  float expected_vals[] = {3.0f, 0.0f, 6.0f, -6.0f, 6.6f, 28.0f, -1.0f, 0.06f, 0.1f};
  test.Run(dims, expected_vals);
}

TEST(MathOpTest, Sub) {
  LotusIR::NodeArg input1_def("A", &s_tensor_float), input2_def("B", &s_tensor_float), output_def("C", &s_tensor_float);
  SimpleFloatTest<Sub> test("Sub", {&input1_def, &input2_def}, {&output_def});

  std::vector<int64_t> dims{3, 3};
  test.AddInput(dims, {1.0f, 2.0f, -1.0f, 0.0f, 1.5f, -100.0f, -5.4f, 9.3f, -10'000.0f});
  test.AddInput(dims, {-1.0f, 4.4f, 432.3f, 0.0f, 3.5f, 64.0f, -5.4f, 9.3f, 10'000.0f});
  test.AddOutput(dims);
  float expected_vals[] = {2.0f, -2.4f, -433.3f, 0.0f, -2.0f, -164.0f, 0.0f, 0.0f, -20'000.0f};
  test.Run(dims, expected_vals);
}

TEST(MathOpTest, Mul) {
  LotusIR::NodeArg input1_def("A", &s_tensor_float), input2_def("B", &s_tensor_float), output_def("C", &s_tensor_float);
  SimpleFloatTest<Mul> test("Mul", {&input1_def, &input2_def}, {&output_def});

  std::vector<int64_t> dims{3, 3};
  test.AddInput(dims, {1.0f, 2.0f, -1.0f, 0.0f, 1.5f, -100.0f, -5.4f, 9.30f, -10'000.0f});
  test.AddInput(dims, {-1.0f, 4.4f, 432.3f, 0.0f, 3.5f, 64.0f, -5.4f, 9.30f, 10'000.0f});
  test.AddOutput(dims);
  float expected_vals[] = {-1.0f, 8.8f, -432.3f, 0.0f, 5.25f, -6'400.0f, 29.16f, 86.49f, -100'000'000.0f};
  test.Run(dims, expected_vals);
}

TEST(MathOpTest, Reciprocal) {
  LotusIR::NodeArg input_def("X", &s_tensor_float), output_def("Y", &s_tensor_float);
  SimpleFloatTest<Reciprocal> test("Reciprocal", {&input_def}, {&output_def});

  std::vector<int64_t> dims{2, 2};
  test.AddInput(dims, {1.0f, 2.0f, -1.0f, -2.0f});
  test.AddOutput(dims);
  float expected_vals[] = {1.0f, 0.5f, -1.0f, -0.5f};
  test.Run(dims, expected_vals);
}
}  // namespace Test
}  // namespace Lotus