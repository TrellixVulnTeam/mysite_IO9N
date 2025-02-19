// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/debug/invalid_access_win.h"

#include <intrin.h>
#include <stdlib.h>
#include <windows.h>

#include "base/check.h"
#include "base/win/windows_version.h"
#include "build/build_config.h"

namespace base {
namespace debug {
namespace win {

namespace {

#if defined(ARCH_CPU_X86_FAMILY)
// On x86/x64 systems, nop instructions are generally 1 byte.
static constexpr int kNopInstructionSize = 1;
#elif defined(ARCH_CPU_ARM64)
// On Arm systems, all instructions are 4 bytes, fixed size.
static constexpr int kNopInstructionSize = 4;
#else
#error "Unsupported architecture"
#endif

// Function that can be jumped midway into safely.
#ifdef COMPILER_GCC
__attribute__((naked)) int nop_sled() {
  asm("nop\n"
      "nop\n"
      "ret\n");
}
#else
int nop_sled() {
  __asm{
    nop
    nop
    ret
    };
}
#endif

using FuncType = decltype(&nop_sled);

void IndirectCall(FuncType* func) {
  // This code always generates CFG guards.
  (*func)();
}

void CreateSyntheticHeapCorruption() {
  EXCEPTION_RECORD record = {};
  record.ExceptionCode = STATUS_HEAP_CORRUPTION;
  RaiseFailFastException(&record, nullptr,
                         FAIL_FAST_GENERATE_EXCEPTION_ADDRESS);
}

}  // namespace

void TerminateWithHeapCorruption() {
  __try {
    // Pre-Windows 10, it's hard to trigger a heap corruption fast fail, so
    // artificially create one instead.
    if (base::win::GetVersion() < base::win::Version::WIN10)
      CreateSyntheticHeapCorruption();
    HANDLE heap = ::HeapCreate(0, 0, 0);
    CHECK(heap);
    CHECK(HeapSetInformation(heap, HeapEnableTerminationOnCorruption, nullptr,
                             0));
    void* addr = ::HeapAlloc(heap, 0, 0x1000);
    CHECK(addr);
    // Corrupt heap header.
    char* addr_mutable = reinterpret_cast<char*>(addr);
    memset(addr_mutable - sizeof(addr), 0xCC, sizeof(addr));

    HeapFree(heap, 0, addr);
    HeapDestroy(heap);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    // Heap corruption exception should never be caught.
    CHECK(false);
  }
  // Should never reach here.
  abort();
}

void TerminateWithControlFlowViolation() {
  // Call into the middle of the NOP sled.
  FuncType func = reinterpret_cast<FuncType>(
      (reinterpret_cast<uintptr_t>(nop_sled)) + kNopInstructionSize);
  __try {
    // Generates a STATUS_STACK_BUFFER_OVERRUN exception if CFG triggers.
    IndirectCall(&func);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    // CFG fast fail should never be caught.
    CHECK(false);
  }
  // Should only reach here if CFG is disabled.
  abort();
}

}  // namespace win
}  // namespace debug
}  // namespace base
