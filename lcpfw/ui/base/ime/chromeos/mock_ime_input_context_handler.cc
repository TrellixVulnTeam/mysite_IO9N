// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/chromeos/mock_ime_input_context_handler.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/input_method.h"
#include "ui/gfx/range/range.h"

namespace ui {

MockIMEInputContextHandler::MockIMEInputContextHandler()
    : commit_text_call_count_(0),
      set_selection_range_call_count_(0),
      update_preedit_text_call_count_(0),
      delete_surrounding_text_call_count_(0) {}

MockIMEInputContextHandler::~MockIMEInputContextHandler() = default;

void MockIMEInputContextHandler::CommitText(
    const std::string& text,
    TextInputClient::InsertTextCursorBehavior cursor_behavior) {
  ++commit_text_call_count_;
  last_commit_text_ = text;
}

void MockIMEInputContextHandler::UpdateCompositionText(
    const CompositionText& text,
    uint32_t cursor_pos,
    bool visible) {
  ++update_preedit_text_call_count_;
  last_update_composition_arg_.composition_text = text;
  last_update_composition_arg_.selection = gfx::Range(cursor_pos);
  last_update_composition_arg_.is_visible = visible;
}

bool MockIMEInputContextHandler::SetCompositionRange(
    uint32_t before,
    uint32_t after,
    const std::vector<ui::ImeTextSpan>& text_spans) {
  // TODO(shend): Make this work with before, after and different text contents.
  last_update_composition_arg_.composition_text.text =
      base::UTF8ToUTF16(last_commit_text_);
  return true;
}

bool MockIMEInputContextHandler::SetComposingRange(
    uint32_t start,
    uint32_t end,
    const std::vector<ui::ImeTextSpan>& text_spans) {
  // TODO(shend): Make this work with start, end and different text contents.
  last_update_composition_arg_.composition_text.text =
      base::UTF8ToUTF16(last_commit_text_);
  return true;
}

gfx::Range MockIMEInputContextHandler::GetAutocorrectRange() {
  return autocorrect_range_;
}

gfx::Rect MockIMEInputContextHandler::GetAutocorrectCharacterBounds() {
  return gfx::Rect();
}

bool MockIMEInputContextHandler::SetAutocorrectRange(const gfx::Range& range) {
  autocorrect_range_ = range;
  return true;
}

bool MockIMEInputContextHandler::SetSelectionRange(uint32_t start,
                                                   uint32_t end) {
  ++set_selection_range_call_count_;
  last_update_composition_arg_.selection = gfx::Range(start, end);
  return true;
}

void MockIMEInputContextHandler::DeleteSurroundingText(int32_t offset,
                                                       uint32_t length) {
  ++delete_surrounding_text_call_count_;
  last_delete_surrounding_text_arg_.offset = offset;
  last_delete_surrounding_text_arg_.length = length;
}

SurroundingTextInfo MockIMEInputContextHandler::GetSurroundingTextInfo() {
  return SurroundingTextInfo();
}

void MockIMEInputContextHandler::Reset() {
  commit_text_call_count_ = 0;
  set_selection_range_call_count_ = 0;
  update_preedit_text_call_count_ = 0;
  delete_surrounding_text_call_count_ = 0;
  last_commit_text_.clear();
  sent_key_events_.clear();
}

void MockIMEInputContextHandler::SendKeyEvent(KeyEvent* event) {
  sent_key_events_.emplace_back(*event);
}

InputMethod* MockIMEInputContextHandler::GetInputMethod() {
  return nullptr;
}

void MockIMEInputContextHandler::ConfirmCompositionText(bool reset_engine,
                                                        bool keep_selection) {
  // TODO(b/134473433) Modify this function so that when keep_selection is
  // true, the selection is not changed when text committed
  if (keep_selection) {
    NOTIMPLEMENTED_LOG_ONCE();
  }
  if (!HasCompositionText())
    return;

  CommitText(
      base::UTF16ToUTF8(last_update_composition_arg_.composition_text.text),
      TextInputClient::InsertTextCursorBehavior::kMoveCursorAfterText);
  last_update_composition_arg_.composition_text.text = base::string16();
}

bool MockIMEInputContextHandler::HasCompositionText() {
  return !last_update_composition_arg_.composition_text.text.empty();
}

}  // namespace ui
