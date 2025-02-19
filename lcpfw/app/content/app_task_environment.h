// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/task/lazy_thread_pool_task_runner.h"
#include "base/task/sequence_manager/sequence_manager.h"
#include "app_scoped_run_loop_timeout.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/traits_bag.h"
#include "build/build_config.h"

namespace base {

class Clock;
class FileDescriptorWatcher;
class SimpleTaskExecutor;
class TickClock;

namespace subtle {
class ScopedTimeClockOverrides;
}

// This header exposes SingleThreadTaskEnvironment and AppTaskEnvironment.
//
// SingleThreadTaskEnvironment enables the following APIs within its scope:
//  - (Thread|Sequenced)TaskRunnerHandle on the main thread
//  - RunLoop on the main thread
//
// AppTaskEnvironment additionally enables:
//  - posting to base::ThreadPool through base/task/thread_pool.h.
//
// Hint: For content::BrowserThreads, use content::BrowserTaskEnvironment.
//
// Tests should prefer SingleThreadTaskEnvironment over AppTaskEnvironment when the
// former is sufficient.
//
// Tasks posted to the (Thread|Sequenced)TaskRunnerHandle run synchronously when
// RunLoop::Run(UntilIdle) or AppTaskEnvironment::RunUntilIdle is called on the
// main thread.
//
// The AppTaskEnvironment requires TestTimeouts::Initialize() to be called in order
// to run posted tasks, so that it can watch for problematic long-running tasks.
//
// The TimeSource trait can be used to request that delayed tasks be under the
// manual control of RunLoop::Run() and AppTaskEnvironment::FastForward*() methods.
//
// If a AppTaskEnvironment's ThreadPoolExecutionMode is QUEUED, ThreadPool tasks
// run when RunUntilIdle() or ~AppTaskEnvironment is called. If
// ThreadPoolExecutionMode is ASYNC, they run as they are posted.
//
// All AppTaskEnvironment methods must be called from the main thread.
//
// Usage:
//
//   class MyTestFixture : public testing::Test {
//    public:
//     (...)
//
//    // protected rather than private visibility will allow controlling the
//    // task environment (e.g. RunUntilIdle(), FastForwardBy(), etc.). from the
//    // test body.
//    protected:
//     // Must generally be the first member to be initialized first and
//     // destroyed last (some members that require single-threaded
//     // initialization and tear down may need to come before -- e.g.
//     // base::test::ScopedFeatureList). Extra traits, like TimeSource, are
//     // best provided inline when declaring the AppTaskEnvironment, as
//     // such:
//     base::test::AppTaskEnvironment task_environment_{
//         base::test::AppTaskEnvironment::TimeSource::MOCK_TIME};
//
//     // Other members go here (or further below in private section.)
//   };
class AppTaskEnvironment {
 protected:
  // This enables a two-phase initialization for sub classes such as
  // content::BrowserTaskEnvironment which need to provide the default task
  // queue because they instantiate a scheduler on the same thread. Subclasses
  // using this trait must invoke DeferredInitFromSubclass() before running the
  // task environment.
  struct SubclassCreatesDefaultTaskRunner {};

 public:
  enum class TimeSource {
    // Delayed tasks and Time/TimeTicks::Now() use the real-time system clock.
    SYSTEM_TIME,

    // Delayed tasks use a mock clock which only advances when reaching "idle"
    // during a RunLoop::Run() call on the main thread or a FastForward*() call
    // to this AppTaskEnvironment. "idle" is defined as the main thread and thread
    // pool being out of ready tasks. In that situation : time advances to the
    // soonest delay between main thread and thread pool delayed tasks,
    // according to the semantics of the current Run*() or FastForward*() call.
    //
    // This also mocks Time/TimeTicks::Now() with the same mock clock.
    //
    // Warning some platform APIs are still real-time, e.g.:
    //   * PlatformThread::Sleep
    //   * WaitableEvent::TimedWait
    //   * ConditionVariable::TimedWait
    //   * Delayed tasks on unmanaged base::Thread's and other custom task
    //     runners.
    MOCK_TIME,

    DEFAULT = SYSTEM_TIME
  };

  // This type will determine what types of messages will get pumped by the main
  // thread.
  // Note: If your test needs to use a custom MessagePump you should
  // consider using a SingleThreadTaskExecutor instead.
  enum class MainThreadType {
    // The main thread doesn't pump system messages.
    DEFAULT,
    // The main thread pumps UI messages.
    UI,
    // The main thread pumps asynchronous IO messages and supports the
    // FileDescriptorWatcher API on POSIX.
    IO,
  };

  // Note that this is irrelevant (and ignored) under
  // ThreadingMode::MAIN_THREAD_ONLY
  enum class ThreadPoolExecutionMode {
    // Thread pool tasks are queued and only executed when RunUntilIdle(),
    // FastForwardBy(), or FastForwardUntilNoTasksRemain() are explicitly
    // called. Note: RunLoop::Run() does *not* unblock the ThreadPool in this
    // mode (it strictly runs only the main thread).
    QUEUED,
    // Thread pool tasks run as they are posted. RunUntilIdle() can still be
    // used to block until done.
    // Note that regardless of this trait, delayed tasks are always "queued"
    // under TimeSource::MOCK_TIME mode.
    ASYNC,
    DEFAULT = ASYNC
  };

  enum class ThreadingMode {
    // ThreadPool will be initialized, thus adding support for multi-threaded
    // tests.
    MULTIPLE_THREADS,
    // No thread pool will be initialized. Useful for tests that want to run
    // single threaded. Prefer using SingleThreadTaskEnvironment over this
    // trait.
    MAIN_THREAD_ONLY,
    DEFAULT = MULTIPLE_THREADS
  };

  // On Windows, sets the COM environment for the ThreadPoolInstance. Ignored
  // on other platforms.
  enum class ThreadPoolCOMEnvironment {
    // Do not initialize COM for the pool's workers.
    NONE,

    // Place the pool's workers in a COM MTA.
    COM_MTA,

    // Enable the MTA by default in unit tests to match the browser process's
    // ThreadPoolInstance configuration.
    //
    // This has the adverse side-effect of enabling the MTA in non-browser unit
    // tests as well but the downside there is not as bad as not having it in
    // browser unit tests. It just means some COM asserts may pass in unit
    // tests where they wouldn't in integration tests or prod. That's okay
    // because unit tests are already generally very loose on allowing I/O,
    // waits, etc. Such misuse will still be caught in later phases (and COM
    // usage should already be pretty much inexistent in sandboxed processes).
    DEFAULT = COM_MTA,
  };

  // List of traits that are valid inputs for the constructor below.
  struct ValidTraits {
    ValidTraits(TimeSource);
    ValidTraits(MainThreadType);
    ValidTraits(ThreadPoolExecutionMode);
    ValidTraits(SubclassCreatesDefaultTaskRunner);
    ValidTraits(ThreadingMode);
    ValidTraits(ThreadPoolCOMEnvironment);
  };

  // Constructor accepts zero or more traits which customize the testing
  // environment.
  template <typename... TaskEnvironmentTraits,
            class CheckArgumentsAreValid = std::enable_if_t<
                trait_helpers::AreValidTraits<ValidTraits,
                                              TaskEnvironmentTraits...>::value>>
  NOINLINE explicit AppTaskEnvironment(TaskEnvironmentTraits... traits)
      : AppTaskEnvironment(
            trait_helpers::GetEnum<TimeSource, TimeSource::DEFAULT>(traits...),
            trait_helpers::GetEnum<MainThreadType, MainThreadType::DEFAULT>(
                traits...),
            trait_helpers::GetEnum<ThreadPoolExecutionMode,
                                   ThreadPoolExecutionMode::DEFAULT>(traits...),
            trait_helpers::GetEnum<ThreadingMode, ThreadingMode::DEFAULT>(
                traits...),
            trait_helpers::GetEnum<ThreadPoolCOMEnvironment,
                                   ThreadPoolCOMEnvironment::DEFAULT>(
                traits...),
            trait_helpers::HasTrait<SubclassCreatesDefaultTaskRunner,
                                    TaskEnvironmentTraits...>(),
            trait_helpers::NotATraitTag()) {}

  // Waits until no undelayed ThreadPool tasks remain. Then, unregisters the
  // ThreadPoolInstance and the (Thread|Sequenced)TaskRunnerHandle.
  virtual ~AppTaskEnvironment();

  // Returns a TaskRunner that schedules tasks on the main thread.
  scoped_refptr<base::SingleThreadTaskRunner> GetMainThreadTaskRunner();

  // Returns whether the main thread's TaskRunner has pending tasks. This will
  // always return true if called right after RunUntilIdle.
  bool MainThreadIsIdle() const;

  // Runs tasks until both the (Thread|Sequenced)TaskRunnerHandle and the
  // ThreadPool's non-delayed queues are empty.
  // While RunUntilIdle() is quite practical and sometimes even necessary -- for
  // example, to flush all tasks bound to Unretained() state before destroying
  // test members -- it should be used with caution per the following warnings:
  //
  // WARNING #1: This may run long (flakily timeout) and even never return! Do
  //             not use this when repeating tasks such as animated web pages
  //             are present.
  // WARNING #2: This may return too early! For example, if used to run until an
  //             incoming event has occurred but that event depends on a task in
  //             a different queue -- e.g. a standalone base::Thread or a system
  //             event.
  //
  // As such, prefer RunLoop::Run() with an explicit RunLoop::QuitClosure() when
  // possible.
  void RunUntilIdle();

  // Only valid for instances using TimeSource::MOCK_TIME. Fast-forwards
  // virtual time by |delta|, causing all tasks on the main thread and thread
  // pool with a remaining delay less than or equal to |delta| to be executed in
  // their natural order before this returns. |delta| must be non-negative. Upon
  // returning from this method, NowTicks() will be >= the initial |NowTicks() +
  // delta|. It is guaranteed to be == iff tasks executed in this
  // FastForwardBy() didn't result in nested calls to time-advancing-methods.
  void FastForwardBy(TimeDelta delta);

  // Only valid for instances using TimeSource::MOCK_TIME.
  // Short for FastForwardBy(TimeDelta::Max()).
  //
  // WARNING: This has the same caveat as RunUntilIdle() and is even more likely
  // to spin forever (any RepeatingTimer will cause this).
  void FastForwardUntilNoTasksRemain();

  // Only valid for instances using TimeSource::MOCK_TIME. Advances virtual time
  // by |delta|. Unlike FastForwardBy, this does not run tasks. Prefer
  // FastForwardBy() when possible but this can be useful when testing blocked
  // pending tasks where being idle (required to fast-forward) is not possible.
  //
  // Delayed tasks that are ripe as a result of this will be scheduled.
  // RunUntilIdle() can be used after this call to ensure those tasks have run.
  // Note: AdvanceClock(delta) + RunUntilIdle() is slightly different from
  // FastForwardBy(delta) in that time passes instantly before running any task
  // (whereas FastForwardBy() will advance the clock in the smallest increments
  // possible at a time). Hence FastForwardBy() is more realistic but
  // AdvanceClock() can be useful when testing edge case scenarios that
  // specifically handle more time than expected to have passed.
  void AdvanceClock(TimeDelta delta);

  // Only valid for instances using TimeSource::MOCK_TIME. Returns a
  // TickClock whose time is updated by FastForward(By|UntilNoTasksRemain).
  const TickClock* GetMockTickClock() const;

  // Only valid for instances using TimeSource::MOCK_TIME. Returns a
  // Clock whose time is updated by FastForward(By|UntilNoTasksRemain). The
  // initial value is implementation defined and should be queried by tests that
  // depend on it.
  // TickClock should be used instead of Clock to measure elapsed time in a
  // process. See time.h.
  const Clock* GetMockClock() const;

  // Only valid for instances using TimeSource::MOCK_TIME. Returns the current
  // virtual tick time (based on a realistic Now(), sampled when this
  // AppTaskEnvironment was created, and manually advanced from that point on).
  // This is always equivalent to base::TimeTicks::Now() under
  // TimeSource::MOCK_TIME.
  base::TimeTicks NowTicks() const;

  // Only valid for instances using TimeSource::MOCK_TIME. Returns the number of
  // pending tasks (delayed and non-delayed) of the main thread's TaskRunner.
  // When debugging, you can use DescribeCurrentTasks() to see what those are.
  size_t GetPendingMainThreadTaskCount() const;

  // Only valid for instances using TimeSource::MOCK_TIME.
  // Returns the delay until the next pending task of the main thread's
  // TaskRunner if there is one, otherwise it returns TimeDelta::Max().
  TimeDelta NextMainThreadPendingTaskDelay() const;

  // Only valid for instances using TimeSource::MOCK_TIME.
  // Returns true iff the next task is delayed. Returns false if the next task
  // is immediate or if there is no next task.
  bool NextTaskIsDelayed() const;

  // For debugging purposes: Dumps information about pending tasks on the main
  // thread, and currently running tasks on the thread pool.
  void DescribeCurrentTasks() const;

  class AppTaskTracker;
  // Callers outside of AppTaskEnvironment may not use the returned pointer. They
  // should just use base::ThreadPoolInstance::Get().
  static AppTaskTracker* CreateThreadPool();

  class DestructionObserver : public CheckedObserver {
   public:
    DestructionObserver() = default;
    ~DestructionObserver() override = default;

    DestructionObserver(const DestructionObserver&) = delete;
    DestructionObserver& operator=(const DestructionObserver&) = delete;

    virtual void WillDestroyCurrentTaskEnvironment() = 0;
  };

  // Adds/removes a DestructionObserver to any AppTaskEnvironment. Observers are
  // notified when any AppTaskEnvironment goes out of scope (other than with a move
  // operation). Must be called on the main thread.
  static void AddDestructionObserver(DestructionObserver* observer);
  static void RemoveDestructionObserver(DestructionObserver* observer);

  // The number of foreground workers in the ThreadPool managed by a
  // AppTaskEnvironment instance. This can be used to determine the maximum
  // parallelism in tests that require each parallel task it spawns to be
  // running at once. Having multiple threads prevents deadlocks should some
  // blocking APIs not use ScopedBlockingCall. It also allows enough concurrency
  // to allow TSAN to spot data races.
  static constexpr int kNumForegroundThreadPoolThreads = 4;

 protected:
  explicit AppTaskEnvironment(AppTaskEnvironment&& other);

  constexpr MainThreadType main_thread_type() const {
    return main_thread_type_;
  }

  constexpr ThreadPoolExecutionMode thread_pool_execution_mode() const {
    return thread_pool_execution_mode_;
  }

  // Returns the TimeDomain driving this AppTaskEnvironment.
  sequence_manager::TimeDomain* GetTimeDomain() const;

  sequence_manager::SequenceManager* sequence_manager() const;

  void DeferredInitFromSubclass(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // Derived classes may need to control when the sequence manager goes away.
  void NotifyDestructionObserversAndReleaseSequenceManager();

 private:
  class MockTimeDomain;

  void InitializeThreadPool();
  void DestroyThreadPool();

  void CompleteInitialization();

  // The template constructor has to be in the header but it delegates to this
  // constructor to initialize all other members out-of-line.
  AppTaskEnvironment(TimeSource time_source,
                  MainThreadType main_thread_type,
                  ThreadPoolExecutionMode thread_pool_execution_mode,
                  ThreadingMode threading_mode,
                  ThreadPoolCOMEnvironment thread_pool_com_environment,
                  bool subclass_creates_default_taskrunner,
                  trait_helpers::NotATraitTag tag);

  const MainThreadType main_thread_type_;
  const ThreadPoolExecutionMode thread_pool_execution_mode_;
  const ThreadingMode threading_mode_;
  const ThreadPoolCOMEnvironment thread_pool_com_environment_;
  const bool subclass_creates_default_taskrunner_;

  std::unique_ptr<sequence_manager::SequenceManager> sequence_manager_;

  // Manages the clock under TimeSource::MOCK_TIME modes. Null in
  // TimeSource::SYSTEM_TIME mode.
  std::unique_ptr<MockTimeDomain> mock_time_domain_;

  // Overrides Time/TimeTicks::Now() under TimeSource::MOCK_TIME_AND_NOW mode.
  // Null in other modes.
  std::unique_ptr<subtle::ScopedTimeClockOverrides> time_overrides_;

  scoped_refptr<sequence_manager::TaskQueue> task_queue_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Only set for instances using TimeSource::MOCK_TIME.
  std::unique_ptr<Clock> mock_clock_;

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  // Enables the FileDescriptorWatcher API iff running a MainThreadType::IO.
  std::unique_ptr<FileDescriptorWatcher> file_descriptor_watcher_;
#endif

  // Owned by the ThreadPoolInstance.
  AppTaskTracker* task_tracker_ = nullptr;

  // Ensures destruction of lazy TaskRunners when this is destroyed.
  std::unique_ptr<internal::ScopedLazyTaskRunnerListForTesting>
      scoped_lazy_task_runner_list_for_testing_;

  // Sets RunLoop::Run() to LOG(FATAL) if not Quit() in a timely manner.
  //std::unique_ptr<AppScopedRunLoopTimeout> run_loop_timeout_;

  std::unique_ptr<bool> owns_instance_ = std::make_unique<bool>(true);

  // To support base::CurrentThread().
  std::unique_ptr<SimpleTaskExecutor> simple_task_executor_;

  // Used to verify thread-affinity of operations that must occur on the main
  // thread. This is the case for anything that modifies or drives the
  // |sequence_manager_|.
  THREAD_CHECKER(main_thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(AppTaskEnvironment);
};

// SingleThreadTaskEnvironment takes the same traits as AppTaskEnvironment and is
// used the exact same way. It's a short-form for
//   AppTaskEnvironment{AppTaskEnvironment::ThreadingMode::MAIN_THREAD_ONLY, ...};
class AppSingleThreadTaskEnvironment : public AppTaskEnvironment {
 public:
  template <class... ArgTypes>
  AppSingleThreadTaskEnvironment(ArgTypes... args)
      : AppTaskEnvironment(ThreadingMode::MAIN_THREAD_ONLY, args...) {}
};

}  // namespace base
