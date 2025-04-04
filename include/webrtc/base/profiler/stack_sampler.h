// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROFILER_STACK_SAMPLER_H_
#define BASE_PROFILER_STACK_SAMPLER_H_

#include <memory>

#include "base/base_export.h"
#include "base/macros.h"
#include "base/threading/platform_thread.h"

namespace base {

class ModuleCache;
class ProfileBuilder;
class StackSamplerTestDelegate;

// StackSampler is an implementation detail of StackSamplingProfiler. It
// abstracts the native implementation required to record a set of stack frames
// for a given thread.
class BASE_EXPORT StackSampler {
 public:
  // This class contains a buffer for stack copies that can be shared across
  // multiple instances of StackSampler.
  class BASE_EXPORT StackBuffer {
   public:
    StackBuffer(size_t buffer_size);
    ~StackBuffer();

    uintptr_t* buffer() const { return buffer_.get(); }
    size_t size() const { return size_; }

   private:
    // The word-aligned buffer.
    const std::unique_ptr<uintptr_t[]> buffer_;

    // The size of the buffer.
    const size_t size_;

    DISALLOW_COPY_AND_ASSIGN(StackBuffer);
  };

  virtual ~StackSampler();

  // Creates a stack sampler that records samples for thread with |thread_id|.
  // Returns null if this platform does not support stack sampling.
  static std::unique_ptr<StackSampler> Create(
      PlatformThreadId thread_id,
      ModuleCache* module_cache,
      StackSamplerTestDelegate* test_delegate);

  // Gets the required size of the stack buffer.
  static size_t GetStackBufferSize();

  // Creates an instance of the a stack buffer that can be used for calls to
  // any StackSampler object.
  static std::unique_ptr<StackBuffer> CreateStackBuffer();

  // The following functions are all called on the SamplingThread (not the
  // thread being sampled).

  // Records a set of frames and returns them.
  virtual void RecordStackFrames(StackBuffer* stackbuffer,
                                 ProfileBuilder* profile_builder) = 0;

 protected:
  StackSampler();

 private:
  DISALLOW_COPY_AND_ASSIGN(StackSampler);
};

// StackSamplerTestDelegate provides seams for test code to execute during stack
// collection.
class BASE_EXPORT StackSamplerTestDelegate {
 public:
  virtual ~StackSamplerTestDelegate();

  // Called after copying the stack and resuming the target thread, but prior to
  // walking the stack. Invoked on the SamplingThread.
  virtual void OnPreStackWalk() = 0;

 protected:
  StackSamplerTestDelegate();

 private:
  DISALLOW_COPY_AND_ASSIGN(StackSamplerTestDelegate);
};

}  // namespace base

#endif  // BASE_PROFILER_STACK_SAMPLER_H_
