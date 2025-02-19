// Copyright 2020 The Tint Authors.
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

#include "src/writer/msl/generator.h"

namespace tint {
namespace writer {
namespace msl {

Generator::Generator(const Program* program)
    : impl_(std::make_unique<GeneratorImpl>(program)) {}

Generator::~Generator() = default;

bool Generator::Generate() {
  auto ret = impl_->Generate();
  if (!ret) {
    error_ = impl_->error();
  }
  return ret;
}

std::string Generator::result() const {
  return impl_->result();
}

std::string Generator::error() const {
  return impl_->error();
}

}  // namespace msl
}  // namespace writer
}  // namespace tint
