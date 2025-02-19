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

#include "gtest/gtest-spi.h"
#include "src/ast/test_helper.h"

namespace tint {
namespace ast {
namespace {

using IdentifierExpressionTest = TestHelper;

TEST_F(IdentifierExpressionTest, Creation) {
  auto* i = Expr("ident");
  EXPECT_EQ(i->symbol(), Symbol(1, ID()));
}

TEST_F(IdentifierExpressionTest, Creation_WithSource) {
  auto* i = Expr(Source{Source::Location{20, 2}}, "ident");
  EXPECT_EQ(i->symbol(), Symbol(1, ID()));

  auto src = i->source();
  EXPECT_EQ(src.range.begin.line, 20u);
  EXPECT_EQ(src.range.begin.column, 2u);
}

TEST_F(IdentifierExpressionTest, IsIdentifier) {
  auto* i = Expr("ident");
  EXPECT_TRUE(i->Is<IdentifierExpression>());
}

TEST_F(IdentifierExpressionTest, Assert_InvalidSymbol) {
  EXPECT_FATAL_FAILURE(
      {
        ProgramBuilder b;
        b.Expr("");
      },
      "internal compiler error");
}

TEST_F(IdentifierExpressionTest, Assert_DifferentProgramID_Symbol) {
  EXPECT_FATAL_FAILURE(
      {
        ProgramBuilder b1;
        ProgramBuilder b2;
        b1.Expr(b2.Sym(""));
      },
      "internal compiler error");
}

TEST_F(IdentifierExpressionTest, ToStr) {
  auto* i = Expr("ident");
  EXPECT_EQ(str(i), R"(Identifier[not set]{ident}
)");
}

}  // namespace
}  // namespace ast
}  // namespace tint
