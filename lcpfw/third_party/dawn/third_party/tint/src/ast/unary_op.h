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

#ifndef SRC_AST_UNARY_OP_H_
#define SRC_AST_UNARY_OP_H_

#include <ostream>

namespace tint {
namespace ast {

/// The unary op
enum class UnaryOp {
  kAddressOf,    // &EXPR
  kIndirection,  // *EXPR
  kNegation,     // -EXPR
  kNot,          // !EXPR
};

/// @param out the std::ostream to write to
/// @param mod the UnaryOp
/// @return the std::ostream so calls can be chained
std::ostream& operator<<(std::ostream& out, UnaryOp mod);

}  // namespace ast
}  // namespace tint

#endif  // SRC_AST_UNARY_OP_H_
