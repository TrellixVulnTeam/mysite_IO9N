// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_task_environment.h"

#include <algorithm>
#include <memory>

#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_pump.h"
#include "base/message_loop/message_pump_type.h"
#include "base/no_destructor.h"
#include "base/process/process.h"
#include "base/run_loop.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/task/post_task.h"
#include "base/task/sequence_manager/sequence_manager_impl.h"
#include "base/task/sequence_manager/time_domain.h"
#include "base/task/simple_task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"
//#include "base/test/bind.h"
//#include "base/test/test_mock_time_task_runner.h"
//#include "base/test/test_timeouts.h"
#include "base/thread_annotations.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequence_local_storage_map.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/time/time_override.h"
//#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "base/files/file_descriptor_watcher_posix.h"
#endif

namespace base {

namespace {

//// The timeout values should increase in the order they appear in this block.
//// static
//base::TimeDelta TestTimeouts::tiny_timeout_ =
//  base::TimeDelta::FromMilliseconds(100);
//base::TimeDelta TestTimeouts::action_timeout_ =
//  base::TimeDelta::FromSeconds(10);
//base::TimeDelta TestTimeouts::action_max_timeout_ =
//  base::TimeDelta::FromSeconds(30);
//base::TimeDelta TestTimeouts::test_launcher_timeout_ =
//  base::TimeDelta::FromSeconds(45);
    const int action_max_timeout_in_sec = 30;
    const int action_timeout_in_sec = 10;

ObserverList<AppTaskEnvironment::DestructionObserver>& GetDestructionObservers() {
  static NoDestructor<ObserverList<AppTaskEnvironment::DestructionObserver>>
      instance;
  return *instance;
}

base::MessagePumpType GetMessagePumpTypeForMainThreadType(
    AppTaskEnvironment::MainThreadType main_thread_type) {
  switch (main_thread_type) {
    case AppTaskEnvironment::MainThreadType::DEFAULT:
      return MessagePumpType::DEFAULT;
    case AppTaskEnvironment::MainThreadType::UI:
      return MessagePumpType::UI;
    case AppTaskEnvironment::MainThreadType::IO:
      return MessagePumpType::IO;
  }
  NOTREACHED();
  return MessagePumpType::DEFAULT;
}

std::unique_ptr<sequence_manager::SequenceManager>
CreateSequenceManagerForMainThreadType(
    AppTaskEnvironment::MainThreadType main_thread_type) {
  auto type = GetMessagePumpTypeForMainThreadType(main_thread_type);
  return sequence_manager::CreateSequenceManagerOnCurrentThreadWithPump(
      MessagePump::Create(type),
      base::sequence_manager::SequenceManager::Settings::Builder()
          .SetMessagePumpType(type)
          .Build());
}

class TickClockBasedClock : public Clock {
 public:
  explicit TickClockBasedClock(const TickClock* tick_clock)
      : tick_clock_(*tick_clock),
        start_ticks_(tick_clock_.NowTicks()),
        start_time_(Time::UnixEpoch()) {}

  Time Now() const override {
    return start_time_ + (tick_clock_.NowTicks() - start_ticks_);
  }

 private:
  const TickClock& tick_clock_;
  const TimeTicks start_ticks_;
  const Time start_time_;
};

}  // namespace

class AppTaskEnvironment::AppTaskTracker
    : public internal::ThreadPoolImpl::TaskTrackerImpl {
 public:
  AppTaskTracker();

  // Allow running tasks. Returns whether tasks were previously allowed to run.
  bool AllowRunTasks();

  // Disallow running tasks. Returns true on success; success requires there to
  // be no tasks currently running. Returns false if >0 tasks are currently
  // running. Prior to returning false, it will attempt to block until at least
  // one task has completed (in an attempt to avoid callers busy-looping
  // DisallowRunTasks() calls with the same set of slowly ongoing tasks). This
  // block attempt will also have a short timeout (in an attempt to prevent the
  // fallout of blocking: if the only task remaining is blocked on the main
  // thread, waiting for it to complete results in a deadlock...).
  bool DisallowRunTasks();

  // Returns true if tasks are currently allowed to run.
  bool TasksAllowedToRun() const;

  // For debugging purposes. Returns a string with information about all the
  // currently running tasks on the thread pool.
  std::string DescribeRunningTasks() const;
  std::string DescribePendingTasks() const;  

 private:
  friend class AppTaskEnvironment;

  // internal::ThreadPoolImpl::TaskTrackerImpl:
  bool WillPostTask(internal::Task* task, TaskShutdownBehavior shutdown_behavior) override;
  void RunTask(internal::Task task,
               internal::TaskSource* sequence,
               const TaskTraits& traits) override;
  void BeginCompleteShutdown(base::WaitableEvent& shutdown_event) override;

  // Synchronizes accesses to members below.
  mutable Lock lock_;

  // True if running tasks is allowed.
  bool can_run_tasks_ GUARDED_BY(lock_) = true;

  // Signaled when |can_run_tasks_| becomes true.
  ConditionVariable can_run_tasks_cv_ GUARDED_BY(lock_);

  // Signaled when a task is completed.
  ConditionVariable task_completed_cv_ GUARDED_BY(lock_);

  // Next task number so that each task has some unique-ish id.
  int64_t next_task_number_ GUARDED_BY(lock_) = 1;
  // The set of tasks currently running, keyed by the id from
  // |next_task_number_|.
  base::flat_map<int64_t, Location> running_tasks_ GUARDED_BY(lock_);

  // pending tasks
  base::flat_map<int, Location> pending_tasks_ GUARDED_BY(lock_);

  DISALLOW_COPY_AND_ASSIGN(AppTaskTracker);
};

class AppTaskEnvironment::MockTimeDomain : public sequence_manager::TimeDomain,
                                        public TickClock {
 public:
  explicit MockTimeDomain(sequence_manager::SequenceManager* sequence_manager)
      : sequence_manager_(sequence_manager) {
    DCHECK_EQ(nullptr, current_mock_time_domain_);
    current_mock_time_domain_ = this;
  }

  ~MockTimeDomain() override {
    DCHECK_EQ(this, current_mock_time_domain_);
    current_mock_time_domain_ = nullptr;
  }

  static MockTimeDomain* current_mock_time_domain_;

  static Time GetTime() {
    return Time::UnixEpoch() + (current_mock_time_domain_->Now() - TimeTicks());
  }

  static TimeTicks GetTimeTicks() { return current_mock_time_domain_->Now(); }

  using TimeDomain::NextScheduledRunTime;

  Optional<TimeTicks> NextScheduledRunTime() const {
    // The TimeDomain doesn't know about immediate tasks, check if we have any.
    if (!sequence_manager_->IsIdleForTesting())
      return Now();
    return TimeDomain::NextScheduledRunTime();
  }

  void AdvanceClock(TimeDelta delta) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    {
      AutoLock lock(now_ticks_lock_);
      now_ticks_ += delta;
    }
    if (thread_pool_)
      thread_pool_->ProcessRipeDelayedTasksForTesting();
  }

  static std::unique_ptr<AppTaskEnvironment::MockTimeDomain> CreateAndRegister(
      sequence_manager::SequenceManager* sequence_manager) {
    auto mock_time_domain =
        std::make_unique<AppTaskEnvironment::MockTimeDomain>(sequence_manager);
    sequence_manager->RegisterTimeDomain(mock_time_domain.get());
    return mock_time_domain;
  }

  void SetThreadPool(internal::ThreadPoolImpl* thread_pool,
                     const AppTaskTracker* thread_pool_task_tracker) {
    DCHECK(!thread_pool_);
    DCHECK(!thread_pool_task_tracker_);
    thread_pool_ = thread_pool;
    thread_pool_task_tracker_ = thread_pool_task_tracker;
  }

  // sequence_manager::TimeDomain:

  sequence_manager::LazyNow CreateLazyNow() const override {
    AutoLock lock(now_ticks_lock_);
    return sequence_manager::LazyNow(now_ticks_);
  }

  TimeTicks Now() const override {
    // This can be called from any thread.
    AutoLock lock(now_ticks_lock_);
    return now_ticks_;
  }

  Optional<TimeDelta> DelayTillNextTask(
      sequence_manager::LazyNow* lazy_now) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    // Make sure TimeDomain::NextScheduledRunTime has taken canceled tasks into
    // account, ReclaimMemory sweeps canceled delayed tasks.
    sequence_manager()->ReclaimMemory();
    Optional<TimeTicks> run_time = NextScheduledRunTime();
    // Check if we've run out of tasks.
    if (!run_time)
      return base::nullopt;

    // Check if we have a task that should be running now. Reading |now_ticks_|
    // from the main thread doesn't require the lock.
    if (run_time <= TS_UNCHECKED_READ(now_ticks_))
      return base::TimeDelta();

    // The next task is a future delayed task. Since we're using mock time, we
    // don't want an actual OS level delayed wake up scheduled, so pretend we
    // have no more work. This will result in appearing idle, AppTaskEnvironment
    // will decide what to do based on that (return to caller or fast-forward
    // time).
    return base::nullopt;
  }

  // This method is called when the underlying message pump has run out of
  // non-delayed work. Advances time to the next task unless
  // |quit_when_idle_requested| or AppTaskEnvironment controls mock time.
  bool MaybeFastForwardToNextTask(bool quit_when_idle_requested) override {
    if (quit_when_idle_requested)
      return false;

    return FastForwardToNextTaskOrCap(TimeTicks::Max()) ==
           NextTaskSource::kMainThread;
  }

  const char* GetName() const override { return "MockTimeDomain"; }

  // TickClock implementation:
  TimeTicks NowTicks() const override { return Now(); }

  // Used by FastForwardToNextTaskOrCap() to return which task source time was
  // advanced to.
  enum class NextTaskSource {
    // Out of tasks under |fast_forward_cap|.
    kNone,
    // There's now >=1 immediate task on the main thread.
    kMainThread,
    // There's now >=1 immediate task in the thread pool.
    kThreadPool,
  };

  // Advances time to the first of : next main thread task, next thread pool
  // task, or |fast_forward_cap| (if it's not Max()).
  NextTaskSource FastForwardToNextTaskOrCap(TimeTicks fast_forward_cap) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    // We don't need to call ReclaimMemory here because
    // DelayTillNextTask will have dealt with cancelled delayed tasks for us.
    Optional<TimeTicks> next_main_thread_task_time = NextScheduledRunTime();

    // Consider the next thread pool tasks iff they're running.
    Optional<TimeTicks> next_thread_pool_task_time;
    if (thread_pool_ && thread_pool_task_tracker_->TasksAllowedToRun()) {
      next_thread_pool_task_time =
          thread_pool_->NextScheduledRunTimeForTesting();
    }

    // Custom comparison logic to consider nullopt the largest rather than
    // smallest value. Could consider using TimeTicks::Max() instead of nullopt
    // to represent out-of-tasks?
    Optional<TimeTicks> next_task_time;
    if (!next_main_thread_task_time) {
      next_task_time = next_thread_pool_task_time;
    } else if (!next_thread_pool_task_time) {
      next_task_time = next_main_thread_task_time;
    } else {
      next_task_time =
          std::min(*next_main_thread_task_time, *next_thread_pool_task_time);
    }

    if (next_task_time && *next_task_time <= fast_forward_cap) {
      {
        AutoLock lock(now_ticks_lock_);
        // It's possible for |next_task_time| to be in the past in the following
        // scenario:
        // Start with Now() == 100ms
        // Thread A : Post 200ms delayed task T (construct and enqueue)
        // Thread B : Construct 20ms delayed task U
        //              => |delayed_run_time| == 120ms.
        // Thread A : FastForwardToNextTaskOrCap() => fast-forwards to T @
        //            300ms (task U is not yet in queue).
        // Thread B : Complete enqueue of task U.
        // Thread A : FastForwardToNextTaskOrCap() => must stay at 300ms and run
        //            U, not go back to 120ms.
        // Hence we need std::max() to protect against this because construction
        // and enqueuing isn't atomic in time (LazyNow support in
        // base/task/thread_pool could help).
        now_ticks_ = std::max(now_ticks_, *next_task_time);
      }

      if (next_task_time == next_thread_pool_task_time) {
        // Let the thread pool know that it should post its now ripe delayed
        // tasks.
        thread_pool_->ProcessRipeDelayedTasksForTesting();
        return NextTaskSource::kThreadPool;
      }
      return NextTaskSource::kMainThread;
    }

    if (!fast_forward_cap.is_max()) {
      AutoLock lock(now_ticks_lock_);
      // It's possible that Now() is already beyond |fast_forward_cap| when the
      // caller nests multiple FastForwardBy() calls.
      now_ticks_ = std::max(now_ticks_, fast_forward_cap);
    }

    return NextTaskSource::kNone;
  }

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  sequence_manager::SequenceManager* const sequence_manager_;

  internal::ThreadPoolImpl* thread_pool_ = nullptr;
  const AppTaskTracker* thread_pool_task_tracker_ = nullptr;

  // Protects |now_ticks_|
  mutable Lock now_ticks_lock_;

  // Only ever written to from the main sequence. Start from real Now() instead
  // of zero to give a more realistic view to tests.
  TimeTicks now_ticks_ GUARDED_BY(now_ticks_lock_){
      base::subtle::TimeTicksNowIgnoringOverride()};
};

AppTaskEnvironment::MockTimeDomain*
    AppTaskEnvironment::MockTimeDomain::current_mock_time_domain_ = nullptr;

AppTaskEnvironment::AppTaskEnvironment(
    TimeSource time_source,
    MainThreadType main_thread_type,
    ThreadPoolExecutionMode thread_pool_execution_mode,
    ThreadingMode threading_mode,
    ThreadPoolCOMEnvironment thread_pool_com_environment,
    bool subclass_creates_default_taskrunner,
    trait_helpers::NotATraitTag)
    : main_thread_type_(main_thread_type),
      thread_pool_execution_mode_(thread_pool_execution_mode),
      threading_mode_(threading_mode),
      thread_pool_com_environment_(thread_pool_com_environment),
      subclass_creates_default_taskrunner_(subclass_creates_default_taskrunner),
      sequence_manager_(
          CreateSequenceManagerForMainThreadType(main_thread_type)),
      mock_time_domain_(
          time_source != TimeSource::SYSTEM_TIME
              ? MockTimeDomain::CreateAndRegister(sequence_manager_.get())
              : nullptr),
      time_overrides_(time_source == TimeSource::MOCK_TIME
                          ? std::make_unique<subtle::ScopedTimeClockOverrides>(
                                &MockTimeDomain::GetTime,
                                &MockTimeDomain::GetTimeTicks,
                                nullptr)
                          : nullptr),
      mock_clock_(mock_time_domain_ ? std::make_unique<TickClockBasedClock>(
                                          mock_time_domain_.get())
                                    : nullptr),
      scoped_lazy_task_runner_list_for_testing_(
          std::make_unique<internal::ScopedLazyTaskRunnerListForTesting>())
      // RunLoopTimeout的作用是给定一个超时时间，当前线程的RunLoop在Run的时候通过TLS发现有RunLoopTimeout实例设置的话
      // 就会直接向当前线程的TaskRunner投递前述给定超时时间的延迟任务，该延迟任务做两件事：
      //    第一是直接RunLoop::Quit；
      //    第二是立即调用SequenceManager::DescribeAllPendingTasks将任务队列中尚未得到执行的任务信息都打印出来；
      // 运行程序的通常情况是我们不会给程序指定一个最长运行时长让其到时间自动退出，所以我们不需要创建RunLoopTimeout实例，
      // 但是我们可能会在乎程序在退出时其任务队列里是否还有未执行的任务并获知其投递信息，所以我们需要监听RunLoop的执行状态，
      // 在其Quit之后我们主动调用一下SequenceManager::DescribeAllPendingTasks。
      // 这个事情放在AppMainRunner中去做，因为SequenceManager的实例是由AppMainRunner维护的，由它来调用比较方便。
      /*run_loop_timeout_(
          mock_time_domain_
              ? nullptr
              : std::make_unique<AppScopedRunLoopTimeout>(
                    FROM_HERE,
                    TimeDelta::FromSeconds(action_timeout_in_sec),
                    BindRepeating(&sequence_manager::SequenceManager::
                                      DescribeAllPendingTasks,
                                  Unretained(sequence_manager_.get())))) */
{
  CHECK(!base::ThreadTaskRunnerHandle::IsSet());
  // If |subclass_creates_default_taskrunner| is true then initialization is
  // deferred until DeferredInitFromSubclass().
  if (!subclass_creates_default_taskrunner) {
    task_queue_ = sequence_manager_->CreateTaskQueue(
        sequence_manager::TaskQueue::Spec("lcpfw_task_environment_default")
            .SetTimeDomain(mock_time_domain_.get()));
    task_runner_ = task_queue_->task_runner();
    sequence_manager_->SetDefaultTaskRunner(task_runner_);
    simple_task_executor_ = std::make_unique<SimpleTaskExecutor>(task_runner_);
    CHECK(base::ThreadTaskRunnerHandle::IsSet())
        << "ThreadTaskRunnerHandle should've been set now.";
    CompleteInitialization();
  }

  if (threading_mode_ != ThreadingMode::MAIN_THREAD_ONLY)
    InitializeThreadPool();

  if (thread_pool_execution_mode_ == ThreadPoolExecutionMode::QUEUED &&
      task_tracker_) {
    CHECK(task_tracker_->DisallowRunTasks());
  }
}

// static
AppTaskEnvironment::AppTaskTracker* AppTaskEnvironment::CreateThreadPool() {
  CHECK(!ThreadPoolInstance::Get())
      << "Someone has already installed a ThreadPoolInstance. If nothing in "
         "your test does so, then a test that ran earlier may have installed "
         "one and leaked it. base::TestSuite will trap leaked globals, unless "
         "someone has explicitly disabled it with "
         "DisableCheckForLeakedGlobals().";

  auto task_tracker = std::make_unique<AppTaskTracker>();
  AppTaskTracker* raw_task_tracker = task_tracker.get();
  auto thread_pool = std::make_unique<internal::ThreadPoolImpl>(
      "app_task_env_thread_pool", std::move(task_tracker));
  ThreadPoolInstance::Set(std::move(thread_pool));

  return raw_task_tracker;
}

void AppTaskEnvironment::InitializeThreadPool() {
  task_tracker_ = CreateThreadPool();
  internal::ThreadPoolImpl* thread_pool = static_cast<internal::ThreadPoolImpl*>(ThreadPoolInstance::Get());
  if (mock_time_domain_) {
    mock_time_domain_->SetThreadPool(
        thread_pool,
        task_tracker_);
  }

  ThreadPoolInstance::InitParams init_params(kNumForegroundThreadPoolThreads);
  init_params.suggested_reclaim_time = TimeDelta::Max();
#if defined(OS_WIN)
  if (thread_pool_com_environment_ == ThreadPoolCOMEnvironment::COM_MTA) {
    init_params.common_thread_pool_environment =
        ThreadPoolInstance::InitParams::CommonThreadPoolEnvironment::COM_MTA;
  }
#endif
  ThreadPoolInstance::Get()->Start(init_params);
}

void AppTaskEnvironment::CompleteInitialization() {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
  if (main_thread_type() == MainThreadType::IO) {
    file_descriptor_watcher_ =
        std::make_unique<FileDescriptorWatcher>(GetMainThreadTaskRunner());
  }
#endif  // defined(OS_POSIX) || defined(OS_FUCHSIA)
}

AppTaskEnvironment::AppTaskEnvironment(AppTaskEnvironment&& other) = default;

AppTaskEnvironment::~AppTaskEnvironment() {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  // If we've been moved then bail out.
  if (!owns_instance_)
    return;
  for (auto& observer : GetDestructionObservers())
    observer.WillDestroyCurrentTaskEnvironment();
  DestroyThreadPool();
  task_queue_ = nullptr;
  NotifyDestructionObserversAndReleaseSequenceManager();
}

void AppTaskEnvironment::DestroyThreadPool() {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  if (threading_mode_ == ThreadingMode::MAIN_THREAD_ONLY)
    return;

  // Ideally this would RunLoop().RunUntilIdle() here to catch any errors or
  // infinite post loop in the remaining work but this isn't possible right now
  // because base::~MessageLoop() didn't use to do this and adding it here would
  // make the migration away from MessageLoop that much harder.

  // Without FlushForTesting(), DeleteSoon() and ReleaseSoon() tasks could be
  // skipped, resulting in memory leaks.
  task_tracker_->AllowRunTasks();
  ThreadPoolInstance::Get()->FlushForTesting();
  ThreadPoolInstance::Get()->Shutdown();
  ThreadPoolInstance::Get()->JoinForTesting();
  // Destroying ThreadPoolInstance state can result in waiting on worker
  // threads. Make sure this is allowed to avoid flaking tests that have
  // disallowed waits on their main thread.
  ScopedAllowBaseSyncPrimitivesForTesting allow_waits_to_destroy_task_tracker;
  ThreadPoolInstance::Set(nullptr);
}

sequence_manager::TimeDomain* AppTaskEnvironment::GetTimeDomain() const {
  return mock_time_domain_ ? mock_time_domain_.get()
                           : sequence_manager_->GetRealTimeDomain();
}

sequence_manager::SequenceManager* AppTaskEnvironment::sequence_manager() const {
  DCHECK(subclass_creates_default_taskrunner_);
  return sequence_manager_.get();
}

void AppTaskEnvironment::DeferredInitFromSubclass(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  task_runner_ = std::move(task_runner);
  sequence_manager_->SetDefaultTaskRunner(task_runner_);
  CompleteInitialization();
}

void AppTaskEnvironment::NotifyDestructionObserversAndReleaseSequenceManager() {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  // A derived classes may call this method early.
  if (!sequence_manager_)
    return;

  if (mock_time_domain_)
    sequence_manager_->UnregisterTimeDomain(mock_time_domain_.get());

  sequence_manager_.reset();
}

scoped_refptr<base::SingleThreadTaskRunner>
AppTaskEnvironment::GetMainThreadTaskRunner() {
  DCHECK(task_runner_);
  return task_runner_;
}

bool AppTaskEnvironment::MainThreadIsIdle() const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  sequence_manager::internal::SequenceManagerImpl* sequence_manager_impl =
      static_cast<sequence_manager::internal::SequenceManagerImpl*>(
          sequence_manager_.get());
  // ReclaimMemory sweeps canceled delayed tasks.
  sequence_manager_impl->ReclaimMemory();
  return sequence_manager_impl->IsIdleForTesting();
}

void AppTaskEnvironment::RunUntilIdle() {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  if (threading_mode_ == ThreadingMode::MAIN_THREAD_ONLY) {
    RunLoop(RunLoop::Type::kNestableTasksAllowed).RunUntilIdle();
    return;
  }

  // TODO(gab): This can be heavily simplified to essentially:
  //     bool HasMainThreadTasks() {
  //      if (message_loop_)
  //        return !message_loop_->IsIdleForTesting();
  //      return mock_time_task_runner_->NextPendingTaskDelay().is_zero();
  //     }
  //     while (task_tracker_->HasIncompleteTasks() || HasMainThreadTasks()) {
  //       base::RunLoop().RunUntilIdle();
  //       // Avoid busy-looping.
  //       if (task_tracker_->HasIncompleteTasks())
  //         PlatformThread::Sleep(TimeDelta::FromMilliSeconds(1));
  //     }
  // Update: This can likely be done now that MessageLoop::IsIdleForTesting()
  // checks all queues.
  //
  // Other than that it works because once |task_tracker_->HasIncompleteTasks()|
  // is false we know for sure that the only thing that can make it true is a
  // main thread task (AppTaskEnvironment owns all the threads). As such we can't
  // racily see it as false on the main thread and be wrong as if it the main
  // thread sees the atomic count at zero, it's the only one that can make it go
  // up. And the only thing that can make it go up on the main thread are main
  // thread tasks and therefore we're done if there aren't any left.
  //
  // This simplification further allows simplification of DisallowRunTasks().
  //
  // This can also be simplified even further once TaskTracker becomes directly
  // aware of main thread tasks. https://crbug.com/660078.

  const bool could_run_tasks = task_tracker_->AllowRunTasks();

  for (;;) {
    task_tracker_->AllowRunTasks();

    // First run as many tasks as possible on the main thread in parallel with
    // tasks in ThreadPool. This increases likelihood of TSAN catching
    // threading errors and eliminates possibility of hangs should a
    // ThreadPool task synchronously block on a main thread task
    // (ThreadPoolInstance::FlushForTesting() can't be used here for that
    // reason).
    RunLoop(RunLoop::Type::kNestableTasksAllowed).RunUntilIdle();

    // Then halt ThreadPool. DisallowRunTasks() failing indicates that there
    // were ThreadPool tasks currently running. In that case, try again from
    // top when DisallowRunTasks() yields control back to this thread as they
    // may have posted main thread tasks.
    if (!task_tracker_->DisallowRunTasks())
      continue;

    // Once ThreadPool is halted. Run any remaining main thread tasks (which
    // may have been posted by ThreadPool tasks that completed between the
    // above main thread RunUntilIdle() and ThreadPool DisallowRunTasks()).
    // Note: this assumes that no main thread task synchronously blocks on a
    // ThreadPool tasks (it certainly shouldn't); this call could otherwise
    // hang.
    RunLoop(RunLoop::Type::kNestableTasksAllowed).RunUntilIdle();

    // The above RunUntilIdle() guarantees there are no remaining main thread
    // tasks (the ThreadPool being halted during the last RunUntilIdle() is
    // key as it prevents a task being posted to it racily with it determining
    // it had no work remaining). Therefore, we're done if there is no more work
    // on ThreadPool either (there can be ThreadPool work remaining if
    // DisallowRunTasks() preempted work and/or the last RunUntilIdle() posted
    // more ThreadPool tasks).
    // Note: this last |if| couldn't be turned into a |do {} while();|. A
    // conditional loop makes it such that |continue;| results in checking the
    // condition (not unconditionally loop again) which would be incorrect for
    // the above logic as it'd then be possible for a ThreadPool task to be
    // running during the DisallowRunTasks() test, causing it to fail, but then
    // post to the main thread and complete before the loop's condition is
    // verified which could result in HasIncompleteUndelayedTasksForTesting()
    // returning false and the loop erroneously exiting with a pending task on
    // the main thread.
    if (!task_tracker_->HasIncompleteTaskSourcesForTesting())
      break;
  }

  // The above loop always ends with running tasks being disallowed. Re-enable
  // parallel execution before returning if it was allowed at the beginning of
  // this call.
  if (could_run_tasks)
    task_tracker_->AllowRunTasks();
}

void AppTaskEnvironment::FastForwardBy(TimeDelta delta) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  DCHECK(mock_time_domain_);
  DCHECK_GE(delta, TimeDelta());

  const bool could_run_tasks = task_tracker_ && task_tracker_->AllowRunTasks();

  const TimeTicks fast_forward_until = mock_time_domain_->NowTicks() + delta;
  do {
    RunUntilIdle();
  } while (mock_time_domain_->FastForwardToNextTaskOrCap(fast_forward_until) !=
           MockTimeDomain::NextTaskSource::kNone);

  if (task_tracker_ && !could_run_tasks)
    task_tracker_->DisallowRunTasks();
}

void AppTaskEnvironment::FastForwardUntilNoTasksRemain() {
  // TimeTicks::operator+(TimeDelta) uses saturated arithmetic so it's safe to
  // pass in TimeDelta::Max().
  FastForwardBy(TimeDelta::Max());
}

void AppTaskEnvironment::AdvanceClock(TimeDelta delta) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  DCHECK(mock_time_domain_);
  DCHECK_GE(delta, TimeDelta());
  mock_time_domain_->AdvanceClock(delta);
}

const TickClock* AppTaskEnvironment::GetMockTickClock() const {
  DCHECK(mock_time_domain_);
  return mock_time_domain_.get();
}

base::TimeTicks AppTaskEnvironment::NowTicks() const {
  DCHECK(mock_time_domain_);
  return mock_time_domain_->Now();
}

const Clock* AppTaskEnvironment::GetMockClock() const {
  DCHECK(mock_clock_);
  return mock_clock_.get();
}

size_t AppTaskEnvironment::GetPendingMainThreadTaskCount() const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  // ReclaimMemory sweeps canceled delayed tasks.
  sequence_manager_->ReclaimMemory();
  return sequence_manager_->GetPendingTaskCountForTesting();
}

TimeDelta AppTaskEnvironment::NextMainThreadPendingTaskDelay() const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  // ReclaimMemory sweeps canceled delayed tasks.
  sequence_manager_->ReclaimMemory();
  DCHECK(mock_time_domain_);
  Optional<TimeTicks> run_time = mock_time_domain_->NextScheduledRunTime();
  if (run_time)
    return *run_time - mock_time_domain_->Now();
  return TimeDelta::Max();
}

bool AppTaskEnvironment::NextTaskIsDelayed() const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  TimeDelta delay = NextMainThreadPendingTaskDelay();
  return !delay.is_zero() && !delay.is_max();
}

void AppTaskEnvironment::DescribeCurrentTasks() const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  LOG(INFO) << task_tracker_->DescribeRunningTasks();
  LOG(INFO) << task_tracker_->DescribePendingTasks();

  std::string main_msg("Main Thread pending tasks: ");
  if (sequence_manager_->GetPendingTaskCountForTesting() == 0)
  {
      main_msg += "none.";
  }
  else
  {
      main_msg += "\n" + sequence_manager_->DescribeAllPendingTasks();
  }
  LOG(INFO) << main_msg;
}

// static
void AppTaskEnvironment::AddDestructionObserver(DestructionObserver* observer) {
  GetDestructionObservers().AddObserver(observer);
}

// static
void AppTaskEnvironment::RemoveDestructionObserver(DestructionObserver* observer) {
  GetDestructionObservers().RemoveObserver(observer);
}

AppTaskEnvironment::AppTaskTracker::AppTaskTracker()
    : can_run_tasks_cv_(&lock_), task_completed_cv_(&lock_) {
  // Consider threads blocked on these as idle (avoids instantiating
  // ScopedBlockingCalls and confusing some //base internals tests).
  can_run_tasks_cv_.declare_only_used_while_idle();
  task_completed_cv_.declare_only_used_while_idle();
}

bool AppTaskEnvironment::AppTaskTracker::AllowRunTasks() {
  AutoLock auto_lock(lock_);
  const bool could_run_tasks = can_run_tasks_;
  can_run_tasks_ = true;
  can_run_tasks_cv_.Broadcast();
  return could_run_tasks;
}

bool AppTaskEnvironment::AppTaskTracker::TasksAllowedToRun() const {
  AutoLock auto_lock(lock_);
  return can_run_tasks_;
}

bool AppTaskEnvironment::AppTaskTracker::DisallowRunTasks() {
  AutoLock auto_lock(lock_);

  // Can't disallow run task if there are tasks running.
  if (!running_tasks_.empty()) {
    // Attempt to wait a bit so that the caller doesn't busy-loop with the same
    // set of pending work. A short wait is required to avoid deadlock
    // scenarios. See DisallowRunTasks()'s declaration for more details.
    task_completed_cv_.TimedWait(TimeDelta::FromMilliseconds(1));
    return false;
  }

  can_run_tasks_ = false;
  return true;
}

bool AppTaskEnvironment::AppTaskTracker::WillPostTask(internal::Task* task, TaskShutdownBehavior shutdown_behavior)
{
    bool ok = __super::WillPostTask(task, shutdown_behavior);
    if (ok)
    {
        AutoLock auto_lock(lock_);

        pending_tasks_.emplace(task->sequence_num, task->posted_from);
    }

    return ok;
}

void AppTaskEnvironment::AppTaskTracker::RunTask(internal::Task task,
                                               internal::TaskSource* sequence,
                                               const TaskTraits& traits) {
  int task_number;
  int sequence_num = task.sequence_num;
  {
    AutoLock auto_lock(lock_);

    while (!can_run_tasks_)
      can_run_tasks_cv_.Wait();

    task_number = next_task_number_++;
    auto pair = running_tasks_.emplace(task_number, task.posted_from);
    CHECK(pair.second);  // If false, the |task_number| was already present.
  }

  {
    // Using TimeTicksNowIgnoringOverride() because in tests that mock time,
    // Now() can advance very far very fast, and that's not a problem. This is
    // watching for tests that have actually long running tasks which cause our
    // test suites to run slowly.
    base::TimeTicks before = base::subtle::TimeTicksNowIgnoringOverride();
    const Location posted_from = task.posted_from;
    internal::ThreadPoolImpl::TaskTrackerImpl::RunTask(std::move(task),
                                                       sequence, traits);
    base::TimeTicks after = base::subtle::TimeTicksNowIgnoringOverride();

    const TimeDelta kTimeout = TimeDelta::FromSeconds(action_max_timeout_in_sec);
    if ((after - before) > kTimeout) {
      /*ADD_FAILURE() << "AppTaskEnvironment: RunTask took more than "
                    << kTimeout.InSeconds() << " seconds. Posted from "
                    << posted_from.ToString();*/
      LOG(FATAL) << "AppTaskEnvironment: RunTask took more than "
          << kTimeout.InSeconds() << " seconds. Posted from "
          << posted_from.ToString();
      DLOG(WARNING) << "AppTaskEnvironment: RunTask took more than "
          << kTimeout.InSeconds() << " seconds. Posted from "
          << posted_from.ToString();
    }
  }

  {
    AutoLock auto_lock(lock_);
    CHECK(can_run_tasks_);
    size_t found = running_tasks_.erase(task_number);
    CHECK_EQ(1u, found);

    found = pending_tasks_.erase(sequence_num);
    CHECK_EQ(1u, found);

    task_completed_cv_.Broadcast();
  }
}

std::string AppTaskEnvironment::AppTaskTracker::DescribeRunningTasks() const {
  base::flat_map<int64_t, Location> running_tasks_copy;
  {
    AutoLock auto_lock(lock_);
    running_tasks_copy = running_tasks_;
  }
  std::string running_tasks_str = "ThreadPool currently running tasks:";
  if (running_tasks_copy.empty()) {
    running_tasks_str += " none.";
  } else {
    for (auto& pair : running_tasks_copy)
      running_tasks_str += "\n  Task posted from: " + pair.second.ToString();
  }
  return running_tasks_str;
}

std::string AppTaskEnvironment::AppTaskTracker::DescribePendingTasks() const
{
    base::flat_map<int, Location> pending_tasks_copy;
    {
        AutoLock auto_lock(lock_);
        pending_tasks_copy = pending_tasks_;
    }
    std::string running_tasks_str = "ThreadPool pending tasks:";
    if (pending_tasks_copy.empty())
    {
        running_tasks_str += " none.";
    }
    else
    {
        for (auto& pair : pending_tasks_copy)
            running_tasks_str += "\n  Task posted from: " + pair.second.ToString();
    }
    return running_tasks_str;
}

void AppTaskEnvironment::AppTaskTracker::BeginCompleteShutdown(
    base::WaitableEvent& shutdown_event) {
  const TimeDelta kTimeout = TimeDelta::FromSeconds(action_max_timeout_in_sec);
  if (shutdown_event.TimedWait(kTimeout))
    return;  // All tasks completed in time, yay! Yield back to shutdown.

  // If we had to wait too long for the shutdown tasks to complete, then we
  // should fail the test and report which tasks are currently running.
  std::string failure_tasks = DescribeRunningTasks();
  std::string pending_tasks = DescribePendingTasks();

  /*ADD_FAILURE() << "AppTaskEnvironment: CompleteShutdown took more than "
                << kTimeout.InSeconds() << " seconds.\n"
                << failure_tasks;*/
  LOG(FATAL) << "AppTaskEnvironment: CompleteShutdown took more than "
      << kTimeout.InSeconds() << " seconds.\n"
      << failure_tasks << "\n" << pending_tasks;
  base::Process::TerminateCurrentProcessImmediately(-1);
}

//}  // namespace test
}  // namespace base
