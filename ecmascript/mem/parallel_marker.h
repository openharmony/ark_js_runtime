/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_MEM_PARALLEL_MARKER_H
#define ECMASCRIPT_MEM_PARALLEL_MARKER_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/slots.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
class Heap;
class TaggedObject;
class Region;

static constexpr uint32_t MAIN_THREAD_INDEX = 0;

class Marker {
public:
    explicit Marker(Heap *heap);
    virtual ~Marker() = default;

    virtual void Initialized()
    {
        ECMA_GC_LOG() << "Marker::Initialize do nothing";
    }

    void MarkRoots(uint32_t threadId);
    void ProcessOldToNew(uint32_t threadId);                  // for HPPGC only semi mode
    void ProcessOldToNew(uint32_t threadId, Region *region);  // for SemiGC
    void ProcessSnapshotRSet(uint32_t threadId);              // for SemiGC

    virtual void ProcessMarkStack([[maybe_unused]] uint32_t threadId)
    {
        LOG(FATAL, ECMASCRIPT) << "can not call this method";
    }

protected:
    // non move
    virtual inline void MarkObject([[maybe_unused]] uint32_t threadId, [[maybe_unused]] TaggedObject *object)
    {
        LOG(FATAL, ECMASCRIPT) << "can not call this method";
    }

    virtual inline SlotStatus MarkObject([[maybe_unused]] uint32_t threadId, [[maybe_unused]] TaggedObject *object,
                                         [[maybe_unused]] ObjectSlot slot)  // move
    {
        LOG(FATAL, ECMASCRIPT) << "can not call this method";
        return SlotStatus::KEEP_SLOT;
    }

    virtual inline void HandleOldToNewRSet(uint32_t threadId, Region *region) = 0;
    virtual inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot) = 0;
    virtual inline void HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                         ObjectSlot end) = 0;
    virtual inline void RecordWeakReference([[maybe_unused]] uint32_t threadId, [[maybe_unused]] JSTaggedType *ref)
    {
        LOG(FATAL, ECMASCRIPT) << "can not call this method";
    }

    Heap *heap_;
    ObjectXRay objXRay_;
};

class NonMovableMarker : public Marker {
public:
    explicit NonMovableMarker(Heap *heap) : Marker(heap) {}

protected:
    void ProcessMarkStack(uint32_t threadId) override;
    inline void MarkObject(uint32_t threadId, TaggedObject *object) override;
    inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot) override;
    inline void HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                 ObjectSlot end) override;

    inline void HandleOldToNewRSet(uint32_t threadId, Region *region) override;
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref) override;
};

class MovableMarker : public Marker {
public:
    explicit MovableMarker(Heap *heap) : Marker(heap) {}

protected:
    inline void HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot) override;
    inline void HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
                                 ObjectSlot end) override;
    virtual inline SlotStatus EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
                                             ObjectSlot slot) = 0;

    inline void HandleOldToNewRSet(uint32_t threadId, Region *region) override;

    inline uintptr_t AllocateDstSpace(uint32_t threadId, size_t size, bool &shouldPromote);
    inline void UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
                                              uintptr_t toAddress, size_t size, const MarkWord &markWord,
                                              ObjectSlot slot);
    inline bool UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size, ObjectSlot slot);
};

class SemiGcMarker : public MovableMarker {
public:
    explicit SemiGcMarker(Heap *heap) : MovableMarker(heap) {}

    void Initialized() override;

protected:
    void ProcessMarkStack(uint32_t threadId) override;
    inline SlotStatus MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot) override;
    inline SlotStatus EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
                                     ObjectSlot slot) override;
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref) override;

private:
    inline bool ShouldBePromoted(TaggedObject *object);

    uintptr_t waterLine_ {0};
};

class CompressGcMarker : public MovableMarker {
public:
    explicit CompressGcMarker(Heap *heap) : MovableMarker(heap) {}

protected:
    void ProcessMarkStack(uint32_t threadId) override;
    inline SlotStatus MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot) override;

    inline SlotStatus EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
                                     ObjectSlot slot) override;
    inline void RecordWeakReference(uint32_t threadId, JSTaggedType *ref) override;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_MARKER_H
