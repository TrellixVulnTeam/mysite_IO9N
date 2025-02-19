// Copyright 2021 The Tint Authors.
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

#ifndef SRC_AST_DISABLE_VALIDATION_DECORATION_H_
#define SRC_AST_DISABLE_VALIDATION_DECORATION_H_

#include <string>

#include "src/ast/internal_decoration.h"

namespace tint {
namespace ast {

/// Enumerator of validation features that can be disabled with a
/// DisableValidationDecoration decoration.
enum class DisabledValidation {
  /// When applied to a function, the validator will not complain there is no
  /// body to a function.
  kFunctionHasNoBody,
  /// When applied to a module-scoped variable, the validator will not complain
  /// if two resource variables have the same binding points.
  kBindingPointCollision,
};

/// An internal decoration used to tell the validator to ignore specific
/// violations. Typically generated by transforms that need to produce ASTs that
/// would otherwise cause validation errors.
class DisableValidationDecoration
    : public Castable<DisableValidationDecoration, InternalDecoration> {
 public:
  /// Constructor
  /// @param program_id the identifier of the program that owns this node
  /// @param validation the validation to disable
  explicit DisableValidationDecoration(ProgramID program_id,
                                       DisabledValidation validation);

  /// Destructor
  ~DisableValidationDecoration() override;

  /// @return the validation that this decoration disables
  DisabledValidation Validation() const { return validation_; }

  /// @return a short description of the internal decoration which will be
  /// displayed in WGSL as `[[internal(<name>)]]` (but is not parsable).
  std::string Name() const override;

  /// Performs a deep clone of this object using the CloneContext `ctx`.
  /// @param ctx the clone context
  /// @return the newly cloned object
  DisableValidationDecoration* Clone(CloneContext* ctx) const override;

 private:
  DisabledValidation const validation_;
};

}  // namespace ast
}  // namespace tint

#endif  // SRC_AST_DISABLE_VALIDATION_DECORATION_H_
