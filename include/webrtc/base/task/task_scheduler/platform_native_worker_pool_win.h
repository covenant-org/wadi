// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_TASK_SCHEDULER_PLATFORM_NATIVE_WORKER_POOL_WIN_H_
#define BASE_TASK_TASK_SCHEDULER_PLATFORM_NATIVE_WORKER_POOL_WIN_H_

#include <windows.h>

#include "base/base_export.h"
#include "base/logging.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task/task_scheduler/scheduler_worker_pool.h"

namespace base {
namespace internal {

// A SchedulerWorkerPool implementation backed by the Windows Thread Pool API.
//
// Windows Thread Pool API official documentation:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686766(v=vs.85).aspx
//
// Blog posts on the Windows Thread Pool API:
// https://msdn.microsoft.com/magazine/hh335066.aspx
// https://msdn.microsoft.com/magazine/hh394144.aspx
// https://msdn.microsoft.com/magazine/hh456398.aspx
// https://msdn.microsoft.com/magazine/hh547107.aspx
// https://msdn.microsoft.com/magazine/hh580731.aspx
class BASE_EXPORT PlatformNativeWorkerPoolWin : public SchedulerWorkerPool {
 public:
  PlatformNativeWorkerPoolWin(TrackedRef<TaskTracker> task_tracker,
                              TrackedRef<Delegate> delegate);

  // Destroying a PlatformNativeWorkerPoolWin is not allowed in
  // production; it is always leaked. In tests, it can only be destroyed after
  // JoinForTesting() has returned.
  ~PlatformNativeWorkerPoolWin() override;

  // Starts the worker pool and allows tasks to begin running.
  void Start();

  // SchedulerWorkerPool:
  void JoinForTesting() override;
  size_t GetMaxConcurrentNonBlockedTasksDeprecated() const override;
  void ReportHeartbeatMetrics() const override;

 private:
  class ScopedWorkersExecutor;

  // Callback that gets run by |pool_|. It runs a task off the next sequence on
  // the |priority_queue_|.
  static void CALLBACK RunNextSequence(PTP_CALLBACK_INSTANCE,
                                       void* scheduler_worker_pool_windows_impl,
                                       PTP_WORK);
  // SchedulerWorkerPool:
  void UpdateSortKey(SequenceAndTransaction sequence_and_transaction) override;
  void PushSequenceAndWakeUpWorkers(
      SequenceAndTransaction sequence_and_transaction) override;
  void EnsureEnoughWorkersLockRequired(BaseScopedWorkersExecutor* executor)
      override EXCLUSIVE_LOCKS_REQUIRED(lock_);

  // Returns the top Sequence off the |priority_queue_|. Returns nullptr
  // if the |priority_queue_| is empty.
  scoped_refptr<Sequence> GetWork();

  // Thread pool object that |work_| gets executed on.
  PTP_POOL pool_ = nullptr;

  // Callback environment. |pool_| is associated with |environment_| so that
  // work objects using this environment run on |pool_|.
  TP_CALLBACK_ENVIRON environment_ = {};

  // Work object that executes RunNextSequence. It has a pointer to the current
  // |PlatformNativeWorkerPoolWin| and a pointer to |environment_| bound to
  // it.
  PTP_WORK work_ = nullptr;

  // Indicates whether the pool has been started yet.
  bool started_ GUARDED_BY(lock_) = false;

  // Number of threadpool work submitted to the pool which haven't popped a
  // Sequence from the PriorityQueue yet.
  size_t num_pending_threadpool_work_ GUARDED_BY(lock_) = 0;

#if DCHECK_IS_ON()
  // Set once JoinForTesting() has returned.
  AtomicFlag join_for_testing_returned_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PlatformNativeWorkerPoolWin);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_TASK_SCHEDULER_PLATFORM_NATIVE_WORKER_POOL_WIN_H_
