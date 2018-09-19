// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "gtest/gtest.h"
#include "test/providers/provider_test_utils.h"
using namespace std;
namespace onnxruntime {
namespace Test {

TEST(LRNTest, LRN_1) {
  OpTester test("LRN");
  test.AddAttribute("alpha", .001f);
  test.AddAttribute("beta", .75f);
  test.AddAttribute("bias", 2.0f);
  test.AddAttribute("size", int64_t(5));

  vector<float> X = {0.93150997f, 0.22777775f, 0.0239904f, 0.68148804f, 0.38188118f,
                     0.37675565f, 0.56248045f, 0.38941276f, 0.80423731f, 0.06973697f,
                     0.22548851f, 0.92550498f, 0.99807096f, 0.13079064f, 0.56333995f,
                     0.68052572f, 0.40238085f, 0.36750308f, 0.21684977f, 0.34298277f,
                     0.41941738f, 0.09853589f, 0.67301852f, 0.17712493f, 0.12575327f,
                     0.25551194f, 0.99063486f, 0.55772477f, 0.25830251f, 0.65443498f,
                     0.19408275f, 0.47776002f, 0.50554532f, 0.7642616f, 0.39078581f,
                     0.5375315f, 0.02074649f, 0.05360929f, 0.79922867f, 0.66012126f,
                     0.76942754f, 0.14644335f, 0.09823465f, 0.79470968f, 0.60738826f,
                     0.19192833f, 0.53565669f, 0.57413113f, 0.27340019f, 0.17495774f};
  vector<int64_t> shape = {1, 2, 5, 5};
  auto expected_output = {0.5538404f, 0.13542697f, 0.01426445f, 0.40519908f, 0.22705813f,
                          0.22401723f, 0.33443925f, 0.23153915f, 0.47815821f, 0.04146536f,
                          0.13407286f, 0.55027318f, 0.5934121f, 0.07776476f, 0.33494502f,
                          0.40461099f, 0.23925379f, 0.21851626f, 0.12893309f, 0.20393133f,
                          0.24938308f, 0.05858848f, 0.40015569f, 0.10531828f, 0.07477307f,
                          0.15191767f, 0.58898938f, 0.33161741f, 0.15358147f, 0.38911262f,
                          0.11540074f, 0.28406623f, 0.30058989f, 0.4543907f, 0.2323599f,
                          0.31961f, 0.01233515f, 0.03187389f, 0.47520086f, 0.3924883f,
                          0.45746815f, 0.08707454f, 0.05841004f, 0.47251317f, 0.36114204f,
                          0.11411944f, 0.31849629f, 0.34136036f, 0.16256343f, 0.10403012f};
  test.AddInput<float>("X", shape, X);
  test.AddOutput<float>("Y", shape, expected_output);
  test.RunOnCpuAndCuda();
}

TEST(LRNTest, LRN_2) {
  OpTester test("LRN");
  test.AddAttribute("alpha", .0001f);
  test.AddAttribute("beta", .75f);
  // default bias attribute value is 1.0f
  test.AddAttribute("size", int64_t(5));

  vector<float> X = {0.97540224f, 0.76555133f, 0.44334042f, 0.81262767f,
                     0.80408305f, 0.45893553f, 0.39771056f, 0.34420514f,
                     0.94965851f, 0.93253171f, 0.42878076f, 0.85962552f,
                     0.14810622f, 0.89759219f, 0.34574565f, 0.70201623f,
                     0.15821661f, 0.48984697f, 0.94124645f, 0.32628751f,
                     0.15926595f, 0.59950596f, 0.88879001f, 0.98674315f,
                     0.80416244f, 0.71297693f, 0.94821811f, 0.48053992f,
                     0.16935933f, 0.11691149f, 0.22425655f, 0.89018708f,
                     0.75404555f, 0.68191183f, 0.31341696f, 0.86113745f,
                     0.11833113f, 0.67812026f, 0.92196965f, 0.2754015f,
                     0.80969357f, 0.06198973f, 0.01612644f, 0.12104732f,
                     0.26516402f, 0.55688673f, 0.88051248f, 0.02725011f,
                     0.05843787f, 0.75477105f, 0.14542344f, 0.97296566f,
                     0.90956807f, 0.43174571f, 0.76335925f, 0.45990494f,
                     0.40780696f, 0.29402575f, 0.54538655f, 0.28858703f,
                     0.56942707f, 0.75392908f, 0.66404897f, 0.0093868f,
                     0.6678884f, 0.60093129f, 0.54297262f, 0.16187565f,
                     0.81088668f, 0.93738687f, 0.17667364f, 0.61121237f,
                     0.46496955f, 0.11731055f, 0.09468836f, 0.80042875f,
                     0.63450789f, 0.6306234f, 0.01279899f, 0.06116414f,
                     0.61256766f, 0.5070824f, 0.60149539f, 0.84250051f,
                     0.99870878f, 0.08305351f, 0.25943553f, 0.58913916f,
                     0.19776763f, 0.29520017f, 0.21649282f, 0.46786523f,
                     0.93280208f, 0.87324554f, 0.63763618f, 0.58810955f};
  vector<int64_t> shape = {2, 3, 4, 4};
  auto expected_output = {0.97537965f, 0.76553655f, 0.44333258f, 0.81260926f,
                          0.80407488f, 0.45892847f, 0.39769977f, 0.34419912f,
                          0.94962716f, 0.93251234f, 0.42877379f, 0.85961282f,
                          0.14810593f, 0.89757699f, 0.34574077f, 0.7020027f,
                          0.15821294f, 0.48983753f, 0.94122976f, 0.32628012f,
                          0.15926433f, 0.59949672f, 0.88876593f, 0.98672587f,
                          0.80413586f, 0.71296215f, 0.94820267f, 0.48053282f,
                          0.169359f, 0.1169095f, 0.22425337f, 0.89016992f,
                          0.75402808f, 0.68189865f, 0.31341141f, 0.86111796f,
                          0.11832992f, 0.67810982f, 0.92194468f, 0.27539667f,
                          0.80966681f, 0.06198845f, 0.01612618f, 0.12104553f,
                          0.26516351f, 0.55687732f, 0.88050002f, 0.02724958f,
                          0.05843714f, 0.75475758f, 0.14542197f, 0.9729411f,
                          0.90953422f, 0.43173879f, 0.76335144f, 0.45989853f,
                          0.40780437f, 0.29402491f, 0.54538363f, 0.28858295f,
                          0.56941342f, 0.75390953f, 0.66404057f, 0.00938675f,
                          0.66788006f, 0.60092056f, 0.54296708f, 0.16187157f,
                          0.81085652f, 0.93737185f, 0.17667183f, 0.61120385f,
                          0.46496657f, 0.11731022f, 0.09468785f, 0.80041742f,
                          0.6344927f, 0.63060707f, 0.01279883f, 0.06116382f,
                          0.61256003f, 0.50707334f, 0.60148925f, 0.84247929f,
                          0.99867165f, 0.08305217f, 0.25943288f, 0.58913094f,
                          0.19776636f, 0.29519933f, 0.21649165f, 0.46785861f,
                          0.93277973f, 0.87322289f, 0.63762808f, 0.58810645f};

  test.AddInput<float>("X", shape, X);
  test.AddOutput<float>("Y", shape, expected_output);
  test.RunOnCpuAndCuda();
}

}  // namespace Test
}  // namespace onnxruntime