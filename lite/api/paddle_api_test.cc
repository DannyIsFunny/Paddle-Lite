// Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lite/api/paddle_api.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "lite/api/paddle_use_kernels.h"
#include "lite/api/paddle_use_ops.h"
#include "lite/api/paddle_use_passes.h"
#include "lite/utils/cp_logging.h"

DEFINE_string(model_dir, "", "");
DEFINE_string(optimized_model, "", "");

namespace paddle {
namespace lite_api {
// subFunction to read buffer from file for testing
static size_t ReadBuffer(const char* file_name, char** out) {
  FILE* fp;
  fp = fopen(file_name, "rb");
  CHECK(fp != nullptr) << " %s open failed !";
  fseek(fp, 0, SEEK_END);
  auto size = static_cast<size_t>(ftell(fp));
  rewind(fp);
  LOG(INFO) << "model size: " << size;
  *out = reinterpret_cast<char*>(malloc(size));
  size_t cur_len = 0;
  size_t nread;
  while ((nread = fread(*out + cur_len, 1, size - cur_len, fp)) != 0) {
    cur_len += nread;
  }
  fclose(fp);
  return cur_len;
}

// Demo1 for Mobile Devices :Load model from file and run
#ifdef LITE_WITH_LIGHT_WEIGHT_FRAMEWORK
TEST(LightApi, run) {
  lite_api::MobileConfig config;
  config.set_model_dir(FLAGS_model_dir + ".opt2.naive");

  auto predictor = lite_api::CreatePaddlePredictor(config);

  auto input_tensor = predictor->GetInput(0);
  input_tensor->Resize(std::vector<int64_t>({100, 100}));
  auto* data = input_tensor->mutable_data<float>();
  for (int i = 0; i < 100 * 100; i++) {
    data[i] = i;
  }

  predictor->Run();

  auto output = predictor->GetOutput(0);
  auto* out = output->data<float>();
  LOG(INFO) << out[0];
  LOG(INFO) << out[1];

  EXPECT_NEAR(out[0], 50.2132, 1e-3);
  EXPECT_NEAR(out[1], -28.8729, 1e-3);
}

// Demo2 for Load model from memory
TEST(MobileConfig, LoadfromMemory) {
  // Get naive buffer
  auto model_path = std::string(FLAGS_optimized_model) + "/__model__.nb";
  auto params_path = std::string(FLAGS_optimized_model) + "/param.nb";
  char* bufModel = nullptr;
  size_t sizeBuf = ReadBuffer(model_path.c_str(), &bufModel);
  char* bufParams = nullptr;
  size_t sizeParams = ReadBuffer(params_path.c_str(), &bufParams);

  // set model buffer and run model
  lite_api::MobileConfig config;
  config.set_model_buffer(bufModel, sizeBuf, bufParams, sizeParams);

  auto predictor = lite_api::CreatePaddlePredictor(config);
  auto input_tensor = predictor->GetInput(0);
  input_tensor->Resize(std::vector<int64_t>({100, 100}));
  auto* data = input_tensor->mutable_data<float>();
  for (int i = 0; i < 100 * 100; i++) {
    data[i] = i;
  }

  predictor->Run();

  const auto output = predictor->GetOutput(0);
  const float* raw_output = output->data<float>();

  for (int i = 0; i < 10; i++) {
    LOG(INFO) << "out " << raw_output[i];
  }
}

#endif

TEST(CxxApi, run) {
  lite_api::CxxConfig config;
  config.set_model_dir(FLAGS_model_dir);
  config.set_preferred_place(Place{TARGET(kX86), PRECISION(kFloat)});
  config.set_valid_places({
      Place{TARGET(kX86), PRECISION(kFloat)},
      Place{TARGET(kARM), PRECISION(kFloat)},
  });

  auto predictor = lite_api::CreatePaddlePredictor(config);

  auto input_tensor = predictor->GetInput(0);
  input_tensor->Resize(std::vector<int64_t>({100, 100}));
  auto* data = input_tensor->mutable_data<float>();
  for (int i = 0; i < 100 * 100; i++) {
    data[i] = i;
  }

  predictor->Run();

  auto output = predictor->GetOutput(0);
  auto* out = output->data<float>();
  LOG(INFO) << out[0];
  LOG(INFO) << out[1];

  EXPECT_NEAR(out[0], 50.2132, 1e-3);
  EXPECT_NEAR(out[1], -28.8729, 1e-3);

  predictor->SaveOptimizedModel(FLAGS_model_dir + ".opt2");
  predictor->SaveOptimizedModel(FLAGS_model_dir + ".opt2.naive",
                                LiteModelType::kNaiveBuffer);
}

}  // namespace lite_api
}  // namespace paddle
