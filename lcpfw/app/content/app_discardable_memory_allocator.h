// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/discardable_memory_allocator.h"

namespace base {

// AppDiscardableMemoryAllocator is a simple DiscardableMemoryAllocator
// implementation that can be used for testing. It allocates one-shot
// DiscardableMemory instances backed by heap memory.
class AppDiscardableMemoryAllocator : public DiscardableMemoryAllocator {
 public:
  AppDiscardableMemoryAllocator() = default;

  // Overridden from DiscardableMemoryAllocator:
  std::unique_ptr<DiscardableMemory> AllocateLockedDiscardableMemory(
      size_t size) override;

  size_t GetBytesAllocated() const override;

  void ReleaseFreeMemory() override {
    // Do nothing since it is backed by heap memory.
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AppDiscardableMemoryAllocator);
};

}  // namespace base
