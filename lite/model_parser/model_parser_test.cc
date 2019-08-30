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

#include "lite/model_parser/model_parser.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "lite/core/scope.h"

DEFINE_string(model_dir, "", "");

namespace paddle {
namespace lite {

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

TEST(ModelParser, LoadProgram) {
  CHECK(!FLAGS_model_dir.empty());
  auto program = LoadProgram(FLAGS_model_dir + "/__model__");
}

TEST(ModelParser, LoadParam) {
  Scope scope;
  auto* v = scope.Var("xxx");
  LoadParam(FLAGS_model_dir + "/fc_0.b_0", v);
  const auto& t = v->Get<Tensor>();
  LOG(INFO) << "loaded\n";
  LOG(INFO) << t;
}

TEST(ModelParser, LoadModelPb) {
  CHECK(!FLAGS_model_dir.empty());
  cpp::ProgramDesc prog;
  Scope scope;
  LoadModelPb(FLAGS_model_dir, "", "", &scope, &prog);
}

TEST(ModelParser, SaveModelPb) {
  CHECK(!FLAGS_model_dir.empty());
  cpp::ProgramDesc prog;
  Scope scope;
  LoadModelPb(FLAGS_model_dir, "", "", &scope, &prog);
  const std::string save_pb_model_path = FLAGS_model_dir + ".saved.pb";
  SaveModelPb(save_pb_model_path, scope, prog);
}

TEST(ModelParser, SaveModelCombinedPb) {
  CHECK(!FLAGS_model_dir.empty());
  cpp::ProgramDesc prog;
  Scope scope;
  LoadModelPb(FLAGS_model_dir, "", "", &scope, &prog);
  const std::string save_pb_model_path = FLAGS_model_dir + ".saved.pb.combined";
  SaveModelPb(save_pb_model_path, scope, prog, true);
}

TEST(ModelParser, LoadModelCombinedPb) {
  CHECK(!FLAGS_model_dir.empty());
  const std::string model_path = FLAGS_model_dir + ".saved.pb.combined";
  cpp::ProgramDesc prog;
  Scope scope;
  std::string model_file_path = FLAGS_model_dir + ".saved.pb.combined/model";
  std::string param_file_path = FLAGS_model_dir + ".saved.pb.combined/params";
  LoadModelPb(
      model_path, model_file_path, param_file_path, &scope, &prog, true);
}

TEST(ModelParser, SaveParamNaive) {
  Scope scope;
  auto* tensor = scope.Var("xxx")->GetMutable<lite::Tensor>();
  auto& lod = *tensor->mutable_lod();
  lod.resize(2);
  lod[0] = {1, 2, 3};
  lod[1] = {4, 5};
  std::vector<int64_t> dim({1, 2, 5});
  tensor->Resize(lite::DDim(dim));
  auto* data = tensor->mutable_data<float>();
  size_t size = tensor->data_size();
  for (size_t i = 0; i < size; ++i) {
    data[i] = i / static_cast<float>(size);
  }
  SaveParamNaive("./fc_0.w", scope, "xxx");
}

TEST(ModelParser, LoadParamNaive) {
  Scope scope;
  LoadParamNaive("./fc_0.w", &scope, "xxx");
  auto& tensor = scope.Var("xxx")->Get<lite::Tensor>();
  std::vector<int64_t> bg_dim({1, 2, 5});
  size_t size = 10;
  std::vector<std::vector<uint64_t>> bg_lod({{1, 2, 3}, {4, 5}});
  std::vector<float> bg_data(size);
  for (size_t i = 0; i < size; ++i) {
    bg_data[i] = i / static_cast<float>(size);
  }

  ASSERT_EQ(bg_dim, tensor.dims().Vectorize());
  ASSERT_EQ(bg_lod, tensor.lod());
  ASSERT_EQ(tensor.data_size(), size);
  auto* data = tensor.data<float>();
  for (int i = 0; i < size; ++i) {
    EXPECT_NEAR(bg_data[i], data[i], 1e-6);
  }
}

TEST(ModelParser, SaveModelNaive) {
  CHECK(!FLAGS_model_dir.empty());
  cpp::ProgramDesc prog;
  Scope scope;
  LoadModelPb(FLAGS_model_dir, "", "", &scope, &prog);
  const std::string save_pb_model_path = FLAGS_model_dir + ".saved.naive";
  SaveModelNaive(save_pb_model_path, scope, prog);
}

TEST(ModelParser, LoadModelNaiveFromMemory) {
  CHECK(!FLAGS_model_dir.empty());
  cpp::ProgramDesc prog;
  Scope scope;

  auto model_path = std::string(FLAGS_model_dir) + ".saved.naive/__model__.nb";
  auto params_path = std::string(FLAGS_model_dir) + ".saved.naive/param.nb";
  char* bufModel = nullptr;
  size_t sizeBuf = ReadBuffer(model_path.c_str(), &bufModel);
  char* bufParams = nullptr;
  std::cout << "sizeBuf: " << sizeBuf << std::endl;
  size_t sizeParams = ReadBuffer(params_path.c_str(), &bufParams);
  std::cout << "sizeParams: " << sizeParams << std::endl;

  LoadModelNaiveFromMemory(bufModel, bufParams, &scope, &prog);
}

}  // namespace lite
}  // namespace paddle
