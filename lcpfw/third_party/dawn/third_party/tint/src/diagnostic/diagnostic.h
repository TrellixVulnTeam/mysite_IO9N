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

#ifndef SRC_DIAGNOSTIC_DIAGNOSTIC_H_
#define SRC_DIAGNOSTIC_DIAGNOSTIC_H_

#include <string>
#include <utility>
#include <vector>

#include "src/source.h"

namespace tint {
namespace diag {

/// Severity is an enumerator of diagnostic severities.
enum class Severity { Note, Warning, Error, InternalCompilerError, Fatal };

/// @return true iff `a` is more than, or of equal severity to `b`
inline bool operator>=(Severity a, Severity b) {
  return static_cast<int>(a) >= static_cast<int>(b);
}

/// Diagnostic holds all the information for a single compiler diagnostic
/// message.
class Diagnostic {
 public:
  /// severity is the severity of the diagnostic message.
  Severity severity = Severity::Error;
  /// source is the location of the diagnostic.
  Source source;
  /// message is the text associated with the diagnostic.
  std::string message;
  /// code is the error code, for example a validation error might have the code
  /// `"v-0001"`.
  const char* code = nullptr;
};

/// List is a container of Diagnostic messages.
class List {
 public:
  /// iterator is the type used for range based iteration.
  using iterator = std::vector<Diagnostic>::const_iterator;

  /// Constructs the list with no elements.
  List();

  /// Copy constructor. Copies the diagnostics from `list` into this list.
  /// @param list the list of diagnostics to copy into this list.
  List(std::initializer_list<Diagnostic> list);

  /// Copy constructor. Copies the diagnostics from `list` into this list.
  /// @param list the list of diagnostics to copy into this list.
  List(const List& list);

  /// Move constructor. Moves the diagnostics from `list` into this list.
  /// @param list the list of diagnostics to move into this list.
  List(List&& list);
  ~List();

  /// Assignment operator. Copies the diagnostics from `list` into this list.
  /// @param list the list to copy into this list.
  /// @return this list.
  List& operator=(const List& list);

  /// Assignment move operator. Moves the diagnostics from `list` into this
  /// list.
  /// @param list the list to move into this list.
  /// @return this list.
  List& operator=(List&& list);

  /// adds a diagnostic to the end of this list.
  /// @param diag the diagnostic to append to this list.
  void add(Diagnostic&& diag) {
    if (diag.severity >= Severity::Error) {
      error_count_++;
    }
    entries_.emplace_back(std::move(diag));
  }

  /// adds a list of diagnostics to the end of this list.
  /// @param list the diagnostic to append to this list.
  void add(const List& list) {
    for (auto diag : list) {
      add(std::move(diag));
    }
  }

  /// adds the note message with the given Source to the end of this list.
  /// @param note_msg the note message
  /// @param source the source of the note diagnostic
  void add_note(const std::string& note_msg, const Source& source) {
    diag::Diagnostic error{};
    error.severity = diag::Severity::Note;
    error.source = source;
    error.message = note_msg;
    add(std::move(error));
  }

  /// adds the warning message with the given Source to the end of this list.
  /// @param warning_msg the warning message
  /// @param source the source of the warning diagnostic
  void add_warning(const std::string& warning_msg, const Source& source) {
    diag::Diagnostic error{};
    error.severity = diag::Severity::Warning;
    error.source = source;
    error.message = warning_msg;
    add(std::move(error));
  }

  /// adds the error message without a source to the end of this list.
  /// @param err_msg the error message
  void add_error(std::string err_msg) {
    diag::Diagnostic error{};
    error.severity = diag::Severity::Error;
    error.message = std::move(err_msg);
    add(std::move(error));
  }

  /// adds the error message with the given Source to the end of this list.
  /// @param err_msg the error message
  /// @param source the source of the error diagnostic
  void add_error(std::string err_msg, const Source& source) {
    diag::Diagnostic error{};
    error.severity = diag::Severity::Error;
    error.source = source;
    error.message = std::move(err_msg);
    add(std::move(error));
  }

  /// adds the error message with the given code and Source to the end of this
  /// list.
  /// @param code the error code
  /// @param err_msg the error message
  /// @param source the source of the error diagnostic
  void add_error(const char* code, std::string err_msg, const Source& source) {
    diag::Diagnostic error{};
    error.code = code;
    error.severity = diag::Severity::Error;
    error.source = source;
    error.message = std::move(err_msg);
    add(std::move(error));
  }

  /// adds an internal compiler error message to the end of this list.
  /// @param err_msg the error message
  /// @param source the source of the internal compiler error
  void add_ice(const std::string& err_msg, const Source& source) {
    diag::Diagnostic ice{};
    ice.severity = diag::Severity::InternalCompilerError;
    ice.source = source;
    ice.message = err_msg;
    add(std::move(ice));
  }

  /// @returns true iff the diagnostic list contains errors diagnostics (or of
  /// higher severity).
  bool contains_errors() const { return error_count_ > 0; }
  /// @returns the number of error diagnostics (or of higher severity).
  size_t error_count() const { return error_count_; }
  /// @returns the number of entries in the list.
  size_t count() const { return entries_.size(); }
  /// @returns the first diagnostic in the list.
  iterator begin() const { return entries_.begin(); }
  /// @returns the last diagnostic in the list.
  iterator end() const { return entries_.end(); }

  /// @returns a formatted string of all the diagnostics in this list.
  std::string str() const;

 private:
  std::vector<Diagnostic> entries_;
  size_t error_count_ = 0;
};

}  // namespace diag
}  // namespace tint

#endif  // SRC_DIAGNOSTIC_DIAGNOSTIC_H_
