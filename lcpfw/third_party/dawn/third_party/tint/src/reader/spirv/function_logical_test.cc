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

#include "gmock/gmock.h"
#include "src/reader/spirv/function.h"
#include "src/reader/spirv/parser_impl_test_helper.h"
#include "src/reader/spirv/spirv_tools_helpers_test.h"

namespace tint {
namespace reader {
namespace spirv {
namespace {

using ::testing::HasSubstr;

std::string Preamble() {
  return R"(
  OpCapability Shader
  OpMemoryModel Logical Simple
  OpEntryPoint Vertex %100 "main"

  %void = OpTypeVoid
  %voidfn = OpTypeFunction %void

  %bool = OpTypeBool
  %true = OpConstantTrue %bool
  %false = OpConstantFalse %bool

  %uint = OpTypeInt 32 0
  %int = OpTypeInt 32 1
  %float = OpTypeFloat 32

  %uint_10 = OpConstant %uint 10
  %uint_20 = OpConstant %uint 20
  %int_30 = OpConstant %int 30
  %int_40 = OpConstant %int 40
  %float_50 = OpConstant %float 50
  %float_60 = OpConstant %float 60

  %ptr_uint = OpTypePointer Function %uint
  %ptr_int = OpTypePointer Function %int
  %ptr_float = OpTypePointer Function %float

  %v2bool = OpTypeVector %bool 2
  %v2uint = OpTypeVector %uint 2
  %v2int = OpTypeVector %int 2
  %v2float = OpTypeVector %float 2

  %v2bool_t_f = OpConstantComposite %v2bool %true %false
  %v2bool_f_t = OpConstantComposite %v2bool %false %true
  %v2uint_10_20 = OpConstantComposite %v2uint %uint_10 %uint_20
  %v2uint_20_10 = OpConstantComposite %v2uint %uint_20 %uint_10
  %v2int_30_40 = OpConstantComposite %v2int %int_30 %int_40
  %v2int_40_30 = OpConstantComposite %v2int %int_40 %int_30
  %v2float_50_60 = OpConstantComposite %v2float %float_50 %float_60
  %v2float_60_50 = OpConstantComposite %v2float %float_60 %float_50
)";
}

// Returns the AST dump for a given SPIR-V assembly constant.
std::string AstFor(std::string assembly) {
  if (assembly == "v2bool_t_f") {
    return R"(TypeConstructor[not set]{
          __vec_2__bool
          ScalarConstructor[not set]{true}
          ScalarConstructor[not set]{false}
        })";
  }
  if (assembly == "v2bool_f_t") {
    return R"(TypeConstructor[not set]{
          __vec_2__bool
          ScalarConstructor[not set]{false}
          ScalarConstructor[not set]{true}
        })";
  }
  if (assembly == "v2uint_10_20") {
    return R"(TypeConstructor[not set]{
          __vec_2__u32
          ScalarConstructor[not set]{10u}
          ScalarConstructor[not set]{20u}
        })";
  }
  if (assembly == "cast_uint_10") {
    return R"(Bitcast[not set]<__i32>{
          ScalarConstructor[not set]{10u}
        })";
  }
  if (assembly == "cast_uint_20") {
    return R"(Bitcast[not set]<__i32>{
          ScalarConstructor[not set]{20u}
        })";
  }
  if (assembly == "cast_v2uint_10_20") {
    return R"(Bitcast[not set]<__vec_2__i32>{
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{10u}
            ScalarConstructor[not set]{20u}
          }
        })";
  }
  if (assembly == "v2uint_20_10") {
    return R"(TypeConstructor[not set]{
          __vec_2__u32
          ScalarConstructor[not set]{20u}
          ScalarConstructor[not set]{10u}
        })";
  }
  if (assembly == "cast_v2uint_20_10") {
    return R"(Bitcast[not set]<__vec_2__i32>{
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{20u}
            ScalarConstructor[not set]{10u}
          }
        })";
  }
  if (assembly == "cast_int_30") {
    return R"(Bitcast[not set]<__u32>{
          ScalarConstructor[not set]{30}
        })";
  }
  if (assembly == "cast_int_40") {
    return R"(Bitcast[not set]<__u32>{
          ScalarConstructor[not set]{40}
        })";
  }
  if (assembly == "v2int_30_40") {
    return R"(TypeConstructor[not set]{
          __vec_2__i32
          ScalarConstructor[not set]{30}
          ScalarConstructor[not set]{40}
        })";
  }
  if (assembly == "cast_v2int_30_40") {
    return R"(Bitcast[not set]<__vec_2__u32>{
          TypeConstructor[not set]{
            __vec_2__i32
            ScalarConstructor[not set]{30}
            ScalarConstructor[not set]{40}
          }
        })";
  }
  if (assembly == "v2int_40_30") {
    return R"(TypeConstructor[not set]{
          __vec_2__i32
          ScalarConstructor[not set]{40}
          ScalarConstructor[not set]{30}
        })";
  }
  if (assembly == "cast_v2int_40_30") {
    return R"(Bitcast[not set]<__vec_2__u32>{
          TypeConstructor[not set]{
            __vec_2__i32
            ScalarConstructor[not set]{40}
            ScalarConstructor[not set]{30}
          }
        })";
  }
  if (assembly == "v2int_40_30") {
    return R"(TypeConstructor[not set]{
          __vec_2__i32
          ScalarConstructor[not set]{40}
          ScalarConstructor[not set]{30}
        })";
  }
  if (assembly == "v2float_50_60") {
    return R"(TypeConstructor[not set]{
          __vec_2__f32
          ScalarConstructor[not set]{50.000000}
          ScalarConstructor[not set]{60.000000}
        })";
  }
  if (assembly == "v2float_60_50") {
    return R"(TypeConstructor[not set]{
          __vec_2__f32
          ScalarConstructor[not set]{60.000000}
          ScalarConstructor[not set]{50.000000}
        })";
  }
  return "bad case";
}

using SpvUnaryLogicalTest = SpvParserTestBase<::testing::Test>;

TEST_F(SpvUnaryLogicalTest, LogicalNot_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpLogicalNot %bool %true
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        ScalarConstructor[not set]{true}
      }
    }
  })"));
}

TEST_F(SpvUnaryLogicalTest, LogicalNot_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpLogicalNot %v2bool %v2bool_t_f
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        TypeConstructor[not set]{
          __vec_2__bool
          ScalarConstructor[not set]{true}
          ScalarConstructor[not set]{false}
        }
      }
    }
  })"));
}

struct BinaryData {
  const std::string res_type;
  const std::string lhs;
  const std::string op;
  const std::string rhs;
  const std::string ast_type;
  const std::string ast_lhs;
  const std::string ast_op;
  const std::string ast_rhs;
};
inline std::ostream& operator<<(std::ostream& out, BinaryData data) {
  out << "BinaryData{" << data.res_type << "," << data.lhs << "," << data.op
      << "," << data.rhs << "," << data.ast_type << "," << data.ast_lhs << ","
      << data.ast_op << "," << data.ast_rhs << "}";
  return out;
}

using SpvBinaryLogicalTest =
    SpvParserTestBase<::testing::TestWithParam<BinaryData>>;

TEST_P(SpvBinaryLogicalTest, EmitExpression) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = )" + GetParam().op +
                        " %" + GetParam().res_type + " %" + GetParam().lhs +
                        " %" + GetParam().rhs + R"(
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions())
      << p->error() << "\n"
      << assembly;
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  std::ostringstream ss;
  ss << R"(VariableConst{
    x_1
    none
    )"
     << GetParam().ast_type << "\n    {\n      Binary[not set]{"
     << "\n        " << GetParam().ast_lhs << "\n        " << GetParam().ast_op
     << "\n        " << GetParam().ast_rhs;
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(ss.str()))
      << assembly;
}

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_IEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // uint uint
        BinaryData{"bool", "uint_10", "OpIEqual", "uint_20", "__bool",
                   "ScalarConstructor[not set]{10u}", "equal",
                   "ScalarConstructor[not set]{20u}"},
        // int int
        BinaryData{"bool", "int_30", "OpIEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "equal",
                   "ScalarConstructor[not set]{40}"},
        // uint int
        BinaryData{"bool", "uint_10", "OpIEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "equal",
                   R"(Bitcast[not set]<__u32>{
          ScalarConstructor[not set]{40}
        })"},
        // int uint
        BinaryData{"bool", "int_40", "OpIEqual", "uint_10", "__bool",
                   "ScalarConstructor[not set]{40}", "equal",
                   R"(Bitcast[not set]<__i32>{
          ScalarConstructor[not set]{10u}
        })"},
        // v2uint v2uint
        BinaryData{"v2bool", "v2uint_10_20", "OpIEqual", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "equal",
                   AstFor("v2uint_20_10")},
        // v2int v2int
        BinaryData{"v2bool", "v2int_30_40", "OpIEqual", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2int_30_40"), "equal",
                   AstFor("v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdEqual", "float_60", "__bool",
                   "ScalarConstructor[not set]{50.000000}", "equal",
                   "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdEqual", "v2float_60_50",
                   "__vec_2__bool", AstFor("v2float_50_60"), "equal",
                   AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_INotEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both uint
        BinaryData{"bool", "uint_10", "OpINotEqual", "uint_20", "__bool",
                   "ScalarConstructor[not set]{10u}", "not_equal",
                   "ScalarConstructor[not set]{20u}"},
        // Both int
        BinaryData{"bool", "int_30", "OpINotEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "not_equal",
                   "ScalarConstructor[not set]{40}"},
        // uint int
        BinaryData{"bool", "uint_10", "OpINotEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "not_equal",
                   R"(Bitcast[not set]<__u32>{
          ScalarConstructor[not set]{40}
        })"},
        // int uint
        BinaryData{"bool", "int_40", "OpINotEqual", "uint_10", "__bool",
                   "ScalarConstructor[not set]{40}", "not_equal",
                   R"(Bitcast[not set]<__i32>{
          ScalarConstructor[not set]{10u}
        })"},
        // Both v2uint
        BinaryData{"v2bool", "v2uint_10_20", "OpINotEqual", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "not_equal",
                   AstFor("v2uint_20_10")},
        // Both v2int
        BinaryData{"v2bool", "v2int_30_40", "OpINotEqual", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2int_30_40"), "not_equal",
                   AstFor("v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdNotEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdNotEqual", "float_60", "__bool",
                   "ScalarConstructor[not set]{50.000000}", "not_equal",
                   "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdNotEqual", "v2float_60_50",
                   "__vec_2__bool", AstFor("v2float_50_60"), "not_equal",
                   AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdLessThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdLessThan", "float_60", "__bool",
                   "ScalarConstructor[not set]{50.000000}", "less_than",
                   "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdLessThan", "v2float_60_50",
                   "__vec_2__bool", AstFor("v2float_50_60"), "less_than",
                   AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdLessThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdLessThanEqual", "float_60",
                   "__bool", "ScalarConstructor[not set]{50.000000}",
                   "less_than_equal", "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdLessThanEqual",
                   "v2float_60_50", "__vec_2__bool", AstFor("v2float_50_60"),
                   "less_than_equal", AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdGreaterThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdGreaterThan", "float_60",
                   "__bool", "ScalarConstructor[not set]{50.000000}",
                   "greater_than", "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdGreaterThan",
                   "v2float_60_50", "__vec_2__bool", AstFor("v2float_50_60"),
                   "greater_than", AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_FOrdGreaterThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        BinaryData{"bool", "float_50", "OpFOrdGreaterThanEqual", "float_60",
                   "__bool", "ScalarConstructor[not set]{50.000000}",
                   "greater_than_equal",
                   "ScalarConstructor[not set]{60.000000}"},
        BinaryData{"v2bool", "v2float_50_60", "OpFOrdGreaterThanEqual",
                   "v2float_60_50", "__vec_2__bool", AstFor("v2float_50_60"),
                   "greater_than_equal", AstFor("v2float_60_50")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_LogicalAnd,
    SpvBinaryLogicalTest,
    ::testing::Values(BinaryData{"bool", "true", "OpLogicalAnd", "false",
                                 "__bool", "ScalarConstructor[not set]{true}",
                                 "logical_and",
                                 "ScalarConstructor[not set]{false}"},
                      BinaryData{"v2bool", "v2bool_t_f", "OpLogicalAnd",
                                 "v2bool_f_t", "__vec_2__bool",
                                 AstFor("v2bool_t_f"), "logical_and",
                                 AstFor("v2bool_f_t")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_LogicalOr,
    SpvBinaryLogicalTest,
    ::testing::Values(BinaryData{"bool", "true", "OpLogicalOr", "false",
                                 "__bool", "ScalarConstructor[not set]{true}",
                                 "logical_or",
                                 "ScalarConstructor[not set]{false}"},
                      BinaryData{"v2bool", "v2bool_t_f", "OpLogicalOr",
                                 "v2bool_f_t", "__vec_2__bool",
                                 AstFor("v2bool_t_f"), "logical_or",
                                 AstFor("v2bool_f_t")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_LogicalEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(BinaryData{"bool", "true", "OpLogicalEqual", "false",
                                 "__bool", "ScalarConstructor[not set]{true}",
                                 "equal", "ScalarConstructor[not set]{false}"},
                      BinaryData{"v2bool", "v2bool_t_f", "OpLogicalEqual",
                                 "v2bool_f_t", "__vec_2__bool",
                                 AstFor("v2bool_t_f"), "equal",
                                 AstFor("v2bool_f_t")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_LogicalNotEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(BinaryData{"bool", "true", "OpLogicalNotEqual", "false",
                                 "__bool", "ScalarConstructor[not set]{true}",
                                 "not_equal",
                                 "ScalarConstructor[not set]{false}"},
                      BinaryData{"v2bool", "v2bool_t_f", "OpLogicalNotEqual",
                                 "v2bool_f_t", "__vec_2__bool",
                                 AstFor("v2bool_t_f"), "not_equal",
                                 AstFor("v2bool_f_t")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_UGreaterThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both unsigned
        BinaryData{"bool", "uint_10", "OpUGreaterThan", "uint_20", "__bool",
                   "ScalarConstructor[not set]{10u}", "greater_than",
                   "ScalarConstructor[not set]{20u}"},
        // First arg signed
        BinaryData{"bool", "int_30", "OpUGreaterThan", "uint_20", "__bool",
                   AstFor("cast_int_30"), "greater_than",
                   "ScalarConstructor[not set]{20u}"},
        // Second arg signed
        BinaryData{"bool", "uint_10", "OpUGreaterThan", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "greater_than",
                   AstFor("cast_int_40")},
        // Vector, both unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpUGreaterThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "greater_than",
                   AstFor("v2uint_20_10")},
        // First arg signed
        BinaryData{"v2bool", "v2int_30_40", "OpUGreaterThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("cast_v2int_30_40"), "greater_than",
                   AstFor("v2uint_20_10")},
        // Second arg signed
        BinaryData{"v2bool", "v2uint_10_20", "OpUGreaterThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "greater_than",
                   AstFor("cast_v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_UGreaterThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both unsigned
        BinaryData{"bool", "uint_10", "OpUGreaterThanEqual", "uint_20",
                   "__bool", "ScalarConstructor[not set]{10u}",
                   "greater_than_equal", "ScalarConstructor[not set]{20u}"},
        // First arg signed
        BinaryData{"bool", "int_30", "OpUGreaterThanEqual", "uint_20", "__bool",
                   AstFor("cast_int_30"), "greater_than_equal",
                   "ScalarConstructor[not set]{20u}"},
        // Second arg signed
        BinaryData{"bool", "uint_10", "OpUGreaterThanEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "greater_than_equal",
                   AstFor("cast_int_40")},
        // Vector, both unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpUGreaterThanEqual",
                   "v2uint_20_10", "__vec_2__bool", AstFor("v2uint_10_20"),
                   "greater_than_equal", AstFor("v2uint_20_10")},
        // First arg signed
        BinaryData{"v2bool", "v2int_30_40", "OpUGreaterThanEqual",
                   "v2uint_20_10", "__vec_2__bool", AstFor("cast_v2int_30_40"),
                   "greater_than_equal", AstFor("v2uint_20_10")},
        // Second arg signed
        BinaryData{"v2bool", "v2uint_10_20", "OpUGreaterThanEqual",
                   "v2int_40_30", "__vec_2__bool", AstFor("v2uint_10_20"),
                   "greater_than_equal", AstFor("cast_v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_ULessThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both unsigned
        BinaryData{"bool", "uint_10", "OpULessThan", "uint_20", "__bool",
                   "ScalarConstructor[not set]{10u}", "less_than",
                   "ScalarConstructor[not set]{20u}"},
        // First arg signed
        BinaryData{"bool", "int_30", "OpULessThan", "uint_20", "__bool",
                   AstFor("cast_int_30"), "less_than",
                   "ScalarConstructor[not set]{20u}"},
        // Second arg signed
        BinaryData{"bool", "uint_10", "OpULessThan", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "less_than",
                   AstFor("cast_int_40")},
        // Vector, both unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpULessThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "less_than",
                   AstFor("v2uint_20_10")},
        // First arg signed
        BinaryData{"v2bool", "v2int_30_40", "OpULessThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("cast_v2int_30_40"), "less_than",
                   AstFor("v2uint_20_10")},
        // Second arg signed
        BinaryData{"v2bool", "v2uint_10_20", "OpULessThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "less_than",
                   AstFor("cast_v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_ULessThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both unsigned
        BinaryData{"bool", "uint_10", "OpULessThanEqual", "uint_20", "__bool",
                   "ScalarConstructor[not set]{10u}", "less_than_equal",
                   "ScalarConstructor[not set]{20u}"},
        // First arg signed
        BinaryData{"bool", "int_30", "OpULessThanEqual", "uint_20", "__bool",
                   AstFor("cast_int_30"), "less_than_equal",
                   "ScalarConstructor[not set]{20u}"},
        // Second arg signed
        BinaryData{"bool", "uint_10", "OpULessThanEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{10u}", "less_than_equal",
                   AstFor("cast_int_40")},
        // Vector, both unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpULessThanEqual", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "less_than_equal",
                   AstFor("v2uint_20_10")},
        // First arg signed
        BinaryData{"v2bool", "v2int_30_40", "OpULessThanEqual", "v2uint_20_10",
                   "__vec_2__bool", AstFor("cast_v2int_30_40"),
                   "less_than_equal", AstFor("v2uint_20_10")},
        // Second arg signed
        BinaryData{"v2bool", "v2uint_10_20", "OpULessThanEqual", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2uint_10_20"), "less_than_equal",
                   AstFor("cast_v2int_40_30")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_SGreaterThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both signed
        BinaryData{"bool", "int_30", "OpSGreaterThan", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "greater_than",
                   "ScalarConstructor[not set]{40}"},
        // First arg unsigned
        BinaryData{"bool", "uint_10", "OpSGreaterThan", "int_40", "__bool",
                   AstFor("cast_uint_10"), "greater_than",
                   "ScalarConstructor[not set]{40}"},
        // Second arg unsigned
        BinaryData{"bool", "int_30", "OpSGreaterThan", "uint_20", "__bool",
                   "ScalarConstructor[not set]{30}", "greater_than",
                   AstFor("cast_uint_20")},
        // Vector, both signed
        BinaryData{"v2bool", "v2int_30_40", "OpSGreaterThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2int_30_40"), "greater_than",
                   AstFor("v2int_40_30")},
        // First arg unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpSGreaterThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("cast_v2uint_10_20"), "greater_than",
                   AstFor("v2int_40_30")},
        // Second arg unsigned
        BinaryData{"v2bool", "v2int_30_40", "OpSGreaterThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2int_30_40"), "greater_than",
                   AstFor("cast_v2uint_20_10")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_SGreaterThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both signed
        BinaryData{"bool", "int_30", "OpSGreaterThanEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "greater_than_equal",
                   "ScalarConstructor[not set]{40}"},
        // First arg unsigned
        BinaryData{"bool", "uint_10", "OpSGreaterThanEqual", "int_40", "__bool",
                   AstFor("cast_uint_10"), "greater_than_equal",
                   "ScalarConstructor[not set]{40}"},
        // Second arg unsigned
        BinaryData{"bool", "int_30", "OpSGreaterThanEqual", "uint_20", "__bool",
                   "ScalarConstructor[not set]{30}", "greater_than_equal",
                   AstFor("cast_uint_20")},
        // Vector, both signed
        BinaryData{"v2bool", "v2int_30_40", "OpSGreaterThanEqual",
                   "v2int_40_30", "__vec_2__bool", AstFor("v2int_30_40"),
                   "greater_than_equal", AstFor("v2int_40_30")},
        // First arg unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpSGreaterThanEqual",
                   "v2int_40_30", "__vec_2__bool", AstFor("cast_v2uint_10_20"),
                   "greater_than_equal", AstFor("v2int_40_30")},
        // Second arg unsigned
        BinaryData{"v2bool", "v2int_30_40", "OpSGreaterThanEqual",
                   "v2uint_20_10", "__vec_2__bool", AstFor("v2int_30_40"),
                   "greater_than_equal", AstFor("cast_v2uint_20_10")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_SLessThan,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both signed
        BinaryData{"bool", "int_30", "OpSLessThan", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "less_than",
                   "ScalarConstructor[not set]{40}"},
        // First arg unsigned
        BinaryData{"bool", "uint_10", "OpSLessThan", "int_40", "__bool",
                   AstFor("cast_uint_10"), "less_than",
                   "ScalarConstructor[not set]{40}"},
        // Second arg unsigned
        BinaryData{"bool", "int_30", "OpSLessThan", "uint_20", "__bool",
                   "ScalarConstructor[not set]{30}", "less_than",
                   AstFor("cast_uint_20")},
        // Vector, both signed
        BinaryData{"v2bool", "v2int_30_40", "OpSLessThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2int_30_40"), "less_than",
                   AstFor("v2int_40_30")},
        // First arg unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpSLessThan", "v2int_40_30",
                   "__vec_2__bool", AstFor("cast_v2uint_10_20"), "less_than",
                   AstFor("v2int_40_30")},
        // Second arg unsigned
        BinaryData{"v2bool", "v2int_30_40", "OpSLessThan", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2int_30_40"), "less_than",
                   AstFor("cast_v2uint_20_10")}));

INSTANTIATE_TEST_SUITE_P(
    SpvParserTest_SLessThanEqual,
    SpvBinaryLogicalTest,
    ::testing::Values(
        // Both signed
        BinaryData{"bool", "int_30", "OpSLessThanEqual", "int_40", "__bool",
                   "ScalarConstructor[not set]{30}", "less_than_equal",
                   "ScalarConstructor[not set]{40}"},
        // First arg unsigned
        BinaryData{"bool", "uint_10", "OpSLessThanEqual", "int_40", "__bool",
                   AstFor("cast_uint_10"), "less_than_equal",
                   "ScalarConstructor[not set]{40}"},
        // Second arg unsigned
        BinaryData{"bool", "int_30", "OpSLessThanEqual", "uint_20", "__bool",
                   "ScalarConstructor[not set]{30}", "less_than_equal",
                   AstFor("cast_uint_20")},
        // Vector, both signed
        BinaryData{"v2bool", "v2int_30_40", "OpSLessThanEqual", "v2int_40_30",
                   "__vec_2__bool", AstFor("v2int_30_40"), "less_than_equal",
                   AstFor("v2int_40_30")},
        // First arg unsigned
        BinaryData{"v2bool", "v2uint_10_20", "OpSLessThanEqual", "v2int_40_30",
                   "__vec_2__bool", AstFor("cast_v2uint_10_20"),
                   "less_than_equal", AstFor("v2int_40_30")},
        // Second arg unsigned
        BinaryData{"v2bool", "v2int_30_40", "OpSLessThanEqual", "v2uint_20_10",
                   "__vec_2__bool", AstFor("v2int_30_40"), "less_than_equal",
                   AstFor("cast_v2uint_20_10")}));

using SpvFUnordTest = SpvParserTestBase<::testing::Test>;

TEST_F(SpvFUnordTest, FUnordEqual_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordEqual %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          not_equal
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordEqual_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordEqual %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          not_equal
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordNotEqual_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordNotEqual %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          equal
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordNotEqual_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordNotEqual %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          equal
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordLessThan_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordLessThan %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          greater_than_equal
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordLessThan_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordLessThan %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          greater_than_equal
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordLessThanEqual_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordLessThanEqual %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          greater_than
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordLessThanEqual_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordLessThanEqual %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          greater_than
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordGreaterThan_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordGreaterThan %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          less_than_equal
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordGreaterThan_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordGreaterThan %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          less_than_equal
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordGreaterThanEqual_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordGreaterThanEqual %bool %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          ScalarConstructor[not set]{50.000000}
          less_than
          ScalarConstructor[not set]{60.000000}
        }
      }
    }
  })"));
}

TEST_F(SpvFUnordTest, FUnordGreaterThanEqual_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpFUnordGreaterThanEqual %v2bool %v2float_50_60 %v2float_60_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()), HasSubstr(R"(
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      UnaryOp[not set]{
        not
        Binary[not set]{
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
          less_than
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{60.000000}
            ScalarConstructor[not set]{50.000000}
          }
        }
      }
    }
  })"));
}

using SpvLogicalTest = SpvParserTestBase<::testing::Test>;

TEST_F(SpvLogicalTest, Select_BoolCond_BoolParams) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpSelect %bool %true %true %false
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __bool
    {
      Call[not set]{
        Identifier[not set]{select}
        (
          ScalarConstructor[not set]{true}
          ScalarConstructor[not set]{false}
          ScalarConstructor[not set]{true}
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, Select_BoolCond_IntScalarParams) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpSelect %uint %true %uint_10 %uint_20
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __u32
    {
      Call[not set]{
        Identifier[not set]{select}
        (
          ScalarConstructor[not set]{10u}
          ScalarConstructor[not set]{20u}
          ScalarConstructor[not set]{true}
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, Select_BoolCond_FloatScalarParams) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpSelect %float %true %float_50 %float_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __f32
    {
      Call[not set]{
        Identifier[not set]{select}
        (
          ScalarConstructor[not set]{50.000000}
          ScalarConstructor[not set]{60.000000}
          ScalarConstructor[not set]{true}
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, Select_BoolCond_VectorParams) {
  // Prior to SPIR-V 1.4, the condition must be a vector of bools
  // when the value operands are vectors.
  // "Before version 1.4, results are only computed per component."
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpSelect %v2uint %true %v2uint_10_20 %v2uint_20_10
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __vec_2__u32
    {
      Call[not set]{
        Identifier[not set]{select}
        (
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{10u}
            ScalarConstructor[not set]{20u}
          }
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{20u}
            ScalarConstructor[not set]{10u}
          }
          ScalarConstructor[not set]{true}
        )
      }
    }
  }
})"));

  // Fails validation prior to SPIR-V 1.4: If the value operands are vectors,
  // then the condition must be a vector.
  // "Expected vector sizes of Result Type and the condition to be equal:
  // Select"
  p->DeliberatelyInvalidSpirv();
}

TEST_F(SpvLogicalTest, Select_VecBoolCond_VectorParams) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpSelect %v2uint %v2bool_t_f %v2uint_10_20 %v2uint_20_10
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __vec_2__u32
    {
      Call[not set]{
        Identifier[not set]{select}
        (
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{10u}
            ScalarConstructor[not set]{20u}
          }
          TypeConstructor[not set]{
            __vec_2__u32
            ScalarConstructor[not set]{20u}
            ScalarConstructor[not set]{10u}
          }
          TypeConstructor[not set]{
            __vec_2__bool
            ScalarConstructor[not set]{true}
            ScalarConstructor[not set]{false}
          }
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, Any) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpAny %bool %v2bool_t_f
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __bool
    {
      Call[not set]{
        Identifier[not set]{any}
        (
          TypeConstructor[not set]{
            __vec_2__bool
            ScalarConstructor[not set]{true}
            ScalarConstructor[not set]{false}
          }
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, All) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpAll %bool %v2bool_t_f
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __bool
    {
      Call[not set]{
        Identifier[not set]{all}
        (
          TypeConstructor[not set]{
            __vec_2__bool
            ScalarConstructor[not set]{true}
            ScalarConstructor[not set]{false}
          }
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, IsNan_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpIsNan %bool %float_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __bool
    {
      Call[not set]{
        Identifier[not set]{isNan}
        (
          ScalarConstructor[not set]{50.000000}
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, IsNan_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpIsNan %v2bool %v2float_50_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      Call[not set]{
        Identifier[not set]{isNan}
        (
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, IsInf_Scalar) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpIsInf %bool %float_50
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __bool
    {
      Call[not set]{
        Identifier[not set]{isInf}
        (
          ScalarConstructor[not set]{50.000000}
        )
      }
    }
  }
})"));
}

TEST_F(SpvLogicalTest, IsInf_Vector) {
  const auto assembly = Preamble() + R"(
     %100 = OpFunction %void None %voidfn
     %entry = OpLabel
     %1 = OpIsInf %v2bool %v2float_50_60
     OpReturn
     OpFunctionEnd
  )";
  auto p = parser(test::Assemble(assembly));
  ASSERT_TRUE(p->BuildAndParseInternalModuleExceptFunctions());
  auto fe = p->function_emitter(100);
  EXPECT_TRUE(fe.EmitBody()) << p->error();
  EXPECT_THAT(ToString(p->builder(), fe.ast_body()),
              HasSubstr(R"(VariableDeclStatement{
  VariableConst{
    x_1
    none
    __vec_2__bool
    {
      Call[not set]{
        Identifier[not set]{isInf}
        (
          TypeConstructor[not set]{
            __vec_2__f32
            ScalarConstructor[not set]{50.000000}
            ScalarConstructor[not set]{60.000000}
          }
        )
      }
    }
  }
})"));
}

// TODO(dneto): Kernel-guarded instructions.
// TODO(dneto): OpSelect over more general types, as in SPIR-V 1.4

}  // namespace
}  // namespace spirv
}  // namespace reader
}  // namespace tint
