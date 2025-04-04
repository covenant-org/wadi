// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_TASK_SCHEDULER_SEQUENCE_H_
#define BASE_TASK_TASK_SCHEDULER_SEQUENCE_H_

#include <stddef.h>

#include "base/base_export.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/sequence_token.h"
#include "base/task/task_scheduler/scheduler_parallel_task_runner.h"
#include "base/task/task_scheduler/sequence_sort_key.h"
#include "base/task/task_scheduler/task.h"
#include "base/task/task_scheduler/task_source.h"
#include "base/task/task_traits.h"
#include "base/threading/sequence_local_storage_map.h"

namespace base {
namespace internal {

// A Sequence holds slots each containing up to a single Task that must be
// executed in posting order.
//
// In comments below, an "empty Sequence" is a Sequence with no slot.
//
// Note: there is a known refcounted-ownership cycle in the Scheduler
// architecture: Sequence -> Task -> TaskRunner -> Sequence -> ...
// This is okay so long as the other owners of Sequence (PriorityQueue and
// SchedulerWorker in alternation and
// SchedulerWorkerPoolImpl::SchedulerWorkerDelegateImpl::GetWork()
// temporarily) keep running it (and taking Tasks from it as a result). A
// dangling reference cycle would only occur should they release their reference
// to it while it's not empty. In other words, it is only correct for them to
// release it after PopTask() returns false to indicate it was made empty by
// that call (in which case the next PushTask() will return true to indicate to
// the caller that the Sequence should be re-enqueued for execution).
//
// This class is thread-safe.
class BASE_EXPORT Sequence : public TaskSource {
 public:
  // A Transaction can perform multiple operations atomically on a
  // Sequence. While a Transaction is alive, it is guaranteed that nothing
  // else will access the Sequence; the Sequence's lock is held for the
  // lifetime of the Transaction.
  class BASE_EXPORT Transaction : public TaskSource::Transaction {
   public:
    Transaction(Transaction&& other);
    ~Transaction();

    // Adds |task| in a new slot at the end of the Sequence. Returns true if the
    // Sequence needs to be enqueued again.
    bool PushTask(Task task);

    Sequence* sequence() const { return static_cast<Sequence*>(task_source()); }

   private:
    friend class Sequence;

    explicit Transaction(Sequence* sequence);

    DISALLOW_COPY_AND_ASSIGN(Transaction);
  };

  // |traits| is metadata that applies to all Tasks in the Sequence.
  // |scheduler_parallel_task_runner| is a reference to the
  // SchedulerParallelTaskRunner that created this Sequence, if any.
  Sequence(const TaskTraits& traits,
           scoped_refptr<SchedulerParallelTaskRunner>
               scheduler_parallel_task_runner = nullptr);

  // Begins a Transaction. This method cannot be called on a thread which has an
  // active Sequence::Transaction.
  Transaction BeginTransaction();

  ExecutionEnvironment GetExecutionEnvironment() override;

  // Returns a token that uniquely identifies this Sequence.
  const SequenceToken& token() const { return token_; }

  SequenceLocalStorageMap* sequence_local_storage() {
    return &sequence_local_storage_;
  }

 private:
  ~Sequence() override;

  // TaskSource:
  Optional<Task> TakeTask() override;
  SequenceSortKey GetSortKey() const override;
  bool IsEmpty() const override;
  void Clear() override;

  const SequenceToken token_ = SequenceToken::Create();

  // Queue of tasks to execute.
  base::queue<Task> queue_;

  // Holds data stored through the SequenceLocalStorageSlot API.
  SequenceLocalStorageMap sequence_local_storage_;

  // A reference to the SchedulerParallelTaskRunner that created this Sequence,
  // if any. Used to remove Sequence from the TaskRunner's list of Sequence
  // references when Sequence is deleted.
  const scoped_refptr<SchedulerParallelTaskRunner>
      scheduler_parallel_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(Sequence);
};

struct BASE_EXPORT SequenceAndTransaction {
  scoped_refptr<Sequence> sequence;
  Sequence::Transaction transaction;
  SequenceAndTransaction(scoped_refptr<Sequence> sequence_in,
                         Sequence::Transaction transaction_in);
  SequenceAndTransaction(SequenceAndTransaction&& other);
  static SequenceAndTransaction FromSequence(scoped_refptr<Sequence> sequence);
  ~SequenceAndTransaction();
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_TASK_SCHEDULER_SEQUENCE_H_
