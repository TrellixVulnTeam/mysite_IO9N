// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_scoped_run_loop_timeout.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
//#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

bool g_add_gtest_failure_on_timeout = false;

std::string TimeoutMessage(const RepeatingCallback<std::string()>& get_log,
                           const Location& timeout_enabled_from_here) {
  std::string message = "RunLoop::Run() timed out. Timeout set at ";
  message += timeout_enabled_from_here.ToString() + ".";
  if (get_log)
    StrAppend(&message, {"\n", get_log.Run()});
  return message;
}

}  // namespace

AppScopedRunLoopTimeout::AppScopedRunLoopTimeout(const Location& from_here,
                                           TimeDelta timeout)
    : AppScopedRunLoopTimeout(from_here, timeout, NullCallback()) {}

AppScopedRunLoopTimeout::~AppScopedRunLoopTimeout() {
  RunLoop::SetTimeoutForCurrentThread(nested_timeout_);
}

AppScopedRunLoopTimeout::AppScopedRunLoopTimeout(
    const Location& timeout_enabled_from_here,
    TimeDelta timeout,
    RepeatingCallback<std::string()> on_timeout_log)
    : nested_timeout_(RunLoop::GetTimeoutForCurrentThread()) {
  DCHECK_GT(timeout, TimeDelta());
  run_timeout_.timeout = timeout;

  if (g_add_gtest_failure_on_timeout) {
    run_timeout_.on_timeout = BindRepeating(
        [](const Location& timeout_enabled_from_here,
           RepeatingCallback<std::string()> on_timeout_log,
           const Location& run_from_here) {
          /*GTEST_FAIL_AT(run_from_here.file_name(), run_from_here.line_number())
              << TimeoutMessage(on_timeout_log, timeout_enabled_from_here);*/
        },
        timeout_enabled_from_here, std::move(on_timeout_log));
  } else {
    run_timeout_.on_timeout = BindRepeating(
        [](const Location& timeout_enabled_from_here,
           RepeatingCallback<std::string()> on_timeout_log,
           const Location& run_from_here) {
          std::string message =
              TimeoutMessage(on_timeout_log, timeout_enabled_from_here);
          logging::LogMessage(run_from_here.file_name(),
                              run_from_here.line_number(), message.data());
        },
        timeout_enabled_from_here, std::move(on_timeout_log));
  }

  RunLoop::SetTimeoutForCurrentThread(&run_timeout_);
}

// static
bool AppScopedRunLoopTimeout::ExistsForCurrentThread() {
  return RunLoop::GetTimeoutForCurrentThread() != nullptr;
}

// static
void AppScopedRunLoopTimeout::SetAddGTestFailureOnTimeout() {
  g_add_gtest_failure_on_timeout = true;
}

// static
const RunLoop::RunLoopTimeout*
AppScopedRunLoopTimeout::GetTimeoutForCurrentThread() {
  return RunLoop::GetTimeoutForCurrentThread();
}

AppScopedDisableRunLoopTimeout::AppScopedDisableRunLoopTimeout()
    : nested_timeout_(RunLoop::GetTimeoutForCurrentThread()) {
  RunLoop::SetTimeoutForCurrentThread(nullptr);
}

AppScopedDisableRunLoopTimeout::~AppScopedDisableRunLoopTimeout() {
  RunLoop::SetTimeoutForCurrentThread(nested_timeout_);
}

}  // namespace base
