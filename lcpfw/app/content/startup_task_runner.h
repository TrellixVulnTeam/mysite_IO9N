// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <list>

#include "base/callback.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"

#include "build/build_config.h"

// A startup task is a void function returning the status on completion.
// a status of > 0 indicates a failure, and that no further startup tasks should
// be run.
using StartupTask = base::OnceCallback<int(void)>;

// This class runs startup tasks. The tasks are either run immediately inline,
// or are queued one at a time on the UI thread's message loop. If the events
// are queued, UI events that are received during startup will be acted upon
// between startup tasks. The motivation for this is that, on targets where the
// UI is already started, it allows us to keep the UI responsive during startup.
//
// Note that this differs from a SingleThreadedTaskRunner in that there may be
// no opportunity to handle UI events between the tasks of a
// SingleThreadedTaskRunner.

class StartupTaskRunner {

 public:
  // Constructor: Note that |startup_complete_callback| is optional. If it is
  // not null it will be called once all the startup tasks have run.
  StartupTaskRunner(base::OnceCallback<void(int)> startup_complete_callback,
                    scoped_refptr<base::SingleThreadTaskRunner> proxy);

  ~StartupTaskRunner();

  // Add a task to the queue of startup tasks to be run.
  void AddTask(StartupTask callback);

  // Start running the tasks asynchronously.
  void StartRunningTasksAsync();

  // Run all tasks, or all remaining tasks, synchronously
  void RunAllTasksNow();

 private:
  friend class base::RefCounted<StartupTaskRunner>;

  std::list<StartupTask> task_list_;
  void WrappedTask();

  base::OnceCallback<void(int)> startup_complete_callback_;
  scoped_refptr<base::SingleThreadTaskRunner> proxy_;

  DISALLOW_COPY_AND_ASSIGN(StartupTaskRunner);
};
