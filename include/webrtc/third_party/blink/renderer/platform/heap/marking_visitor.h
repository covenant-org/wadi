// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VISITOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VISITOR_H_

#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/heap_buildflags.h"
#include "third_party/blink/renderer/platform/heap/heap_page.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

class BasePage;

// Visitor used to mark Oilpan objects.
class PLATFORM_EXPORT MarkingVisitor : public Visitor {
 public:
  enum MarkingMode {
    // This is a default visitor. This is used for MarkingType=kAtomicMarking
    // and MarkingType=kIncrementalMarking.
    kGlobalMarking,
    // This visitor just marks objects and ignores weak processing.
    // This is used for MarkingType=kTakeSnapshot.
    kSnapshotMarking,
    // Perform global marking along with preparing for additional sweep
    // compaction of heap arenas afterwards. Compared to the GlobalMarking
    // visitor, this visitor will also register references to objects
    // that might be moved during arena compaction -- the compaction
    // pass will then fix up those references when the object move goes
    // ahead.
    kGlobalMarkingWithCompaction,
  };

  // Write barrier that adds |value| to the set of marked objects. The barrier
  // bails out if marking is off or the object is not yet marked.
  ALWAYS_INLINE static void WriteBarrier(void* value);

  // Eagerly traces an already marked backing store ensuring that all its
  // children are discovered by the marker. The barrier bails out if marking
  // is off and on individual objects reachable if they are already marked. The
  // barrier uses the callback function through GcInfo, so it will not inline
  // any templated type-specific code.
  ALWAYS_INLINE static void TraceMarkedBackingStore(void* value);

  MarkingVisitor(ThreadState*, MarkingMode);
  ~MarkingVisitor() override;

  // Marking implementation.

  // Conservatively marks an object if pointed to by Address. The object may
  // be in construction as the scan is conservative without relying on a
  // Trace method.
  void ConservativelyMarkAddress(BasePage*, Address);

  // Marks an object dynamically using any address within its body and adds a
  // tracing callback for processing of the object. The object is not allowed
  // to be in construction.
  void DynamicallyMarkAddress(Address);

  // Marks an object and adds a tracing callback for processing of the object.
  inline void MarkHeader(HeapObjectHeader*, TraceCallback);

  // Try to mark an object without tracing. Returns true when the object was not
  // marked upon calling.
  inline bool MarkHeaderNoTracing(HeapObjectHeader*);

  // Implementation of the visitor interface. See above for descriptions.

  void Visit(void* object, TraceDescriptor desc) final {
    DCHECK(object);
    if (desc.base_object_payload == BlinkGC::kNotFullyConstructedObject) {
      // This means that the objects are not-yet-fully-constructed. See comments
      // on GarbageCollectedMixin for how those objects are handled.
      not_fully_constructed_worklist_.Push(object);
      return;
    }
    MarkHeader(HeapObjectHeader::FromPayload(desc.base_object_payload),
               desc.callback);
  }

  void VisitWithWrappers(void*, TraceDescriptor) final {
    // Ignore as the object is also passed to Visit(void*, TraceDescriptor).
  }

  void VisitWeak(void* object,
                 void** object_slot,
                 TraceDescriptor desc,
                 WeakCallback callback) final {
    // Filter out already marked values. The write barrier for WeakMember
    // ensures that any newly set value after this point is kept alive and does
    // not require the callback.
    if (desc.base_object_payload != BlinkGC::kNotFullyConstructedObject &&
        HeapObjectHeader::FromPayload(desc.base_object_payload)->IsMarked())
      return;
    RegisterWeakCallback(object_slot, callback);
  }

  void VisitBackingStoreStrongly(void* object,
                                 void** object_slot,
                                 TraceDescriptor desc) final {
    RegisterBackingStoreReference(object_slot);
    if (!object)
      return;
    Visit(object, desc);
  }

  // All work is registered through RegisterWeakCallback.
  void VisitBackingStoreWeakly(void* object,
                               void** object_slot,
                               TraceDescriptor desc,
                               WeakCallback callback,
                               void* parameter) final {
    RegisterBackingStoreReference(object_slot);
    if (!object)
      return;
    RegisterWeakCallback(parameter, callback);
  }

  // Used to only mark the backing store when it has been registered for weak
  // processing. In this case, the contents are processed separately using
  // the corresponding traits but the backing store requires marking.
  void VisitBackingStoreOnly(void* object, void** object_slot) final {
    RegisterBackingStoreReference(object_slot);
    if (!object)
      return;
    MarkHeaderNoTracing(HeapObjectHeader::FromPayload(object));
  }

  void RegisterBackingStoreCallback(void** slot,
                                    MovingObjectCallback,
                                    void* callback_data) final;
  bool RegisterWeakTable(const void* closure,
                         EphemeronCallback iteration_callback) final;
  void RegisterWeakCallback(void* closure, WeakCallback) final;

  // Unused cross-component visit methods.
  void Visit(const TraceWrapperV8Reference<v8::Value>&) override {}

 private:
  // Exact version of the marking write barriers.
  static void WriteBarrierSlow(void*);
  static void TraceMarkedBackingStoreSlow(void*);

  void RegisterBackingStoreReference(void** slot);

  MarkingWorklist::View marking_worklist_;
  NotFullyConstructedWorklist::View not_fully_constructed_worklist_;
  WeakCallbackWorklist::View weak_callback_worklist_;
  const MarkingMode marking_mode_;
};

inline bool MarkingVisitor::MarkHeaderNoTracing(HeapObjectHeader* header) {
  DCHECK(header);
  DCHECK(State()->InAtomicMarkingPause() || State()->IsIncrementalMarking());
  // A GC should only mark the objects that belong in its heap.
  DCHECK_EQ(State(),
            PageFromObject(header->Payload())->Arena()->GetThreadState());
  // Never mark free space objects. This would e.g. hint to marking a promptly
  // freed backing store.
  DCHECK(!header->IsFree());

  return header->TryMark();
}

inline void MarkingVisitor::MarkHeader(HeapObjectHeader* header,
                                       TraceCallback callback) {
  DCHECK(header);
  DCHECK(callback);

  if (header->IsInConstruction()) {
    not_fully_constructed_worklist_.Push(header->Payload());
  } else if (MarkHeaderNoTracing(header)) {
    marking_worklist_.Push(
        {reinterpret_cast<void*>(header->Payload()), callback});
  }
}

ALWAYS_INLINE void MarkingVisitor::WriteBarrier(void* value) {
  if (!ThreadState::IsAnyIncrementalMarking())
    return;

  // Avoid any further checks and dispatch to a call at this point. Aggressive
  // inlining otherwise pollutes the regular execution paths.
  WriteBarrierSlow(value);
}

ALWAYS_INLINE void MarkingVisitor::TraceMarkedBackingStore(void* value) {
  if (!ThreadState::IsAnyIncrementalMarking())
    return;

  // Avoid any further checks and dispatch to a call at this point. Aggressive
  // inlining otherwise pollutes the regular execution paths.
  TraceMarkedBackingStoreSlow(value);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_MARKING_VISITOR_H_
