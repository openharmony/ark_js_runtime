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

#ifndef ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
#define ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H

#include "ecmascript/mem/semi_space_collector.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/semi_space_worker.h"

namespace panda::ecmascript {
void SemiSpaceCollector::UpdatePromotedSlot(TaggedObject *object, ObjectSlot slot)
{
#ifndef NDEBUG
    JSTaggedValue value(slot.GetTaggedType());
    ASSERT(value.IsHeapObject());
    ASSERT(Region::ObjectAddressToRange(value.GetTaggedObject())->InYoungGeneration());
#endif
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(!objectRegion->InYoungGeneration());
    objectRegion->InsertOldToNewRememberedSet(slot.SlotAddress());
}

void SemiSpaceCollector::RecordWeakReference(uint32_t threadId, JSTaggedType *ref)
{
    auto value = JSTaggedValue(*ref);
    Region *objectRegion = Region::ObjectAddressToRange(value.GetTaggedWeakRef());
    if (objectRegion->InYoungGeneration()) {
        workList_->PushWeakReference(threadId, ref);
    }
}

uintptr_t SemiSpaceCollector::AllocateOld(size_t size)
{
    os::memory::LockHolder lock(allocatorLock_);
    uintptr_t result = oldSpaceAllocator_.Allocate(size);
    if (UNLIKELY(result == 0)) {
        if (!heap_->FillOldSpaceAndTryGC(&oldSpaceAllocator_, false)) {
            return 0;
        }
        result = oldSpaceAllocator_.Allocate(size);
        if (UNLIKELY(result == 0)) {
            return 0;
        }
    }
    return result;
}

uintptr_t SemiSpaceCollector::AllocateYoung(size_t size)
{
    os::memory::LockHolder lock(allocatorLock_);
    uintptr_t result = fromSpaceAllocator_.Allocate(size);
    if (UNLIKELY(result == 0)) {
        if (!heap_->FillNewSpaceAndTryGC(&fromSpaceAllocator_, false)) {
            return 0;
        }
        result = fromSpaceAllocator_.Allocate(size);
        if (UNLIKELY(result == 0)) {
            return 0;
        }
    }
    return result;
}

bool SemiSpaceCollector::BlowAgeMark(uintptr_t address)
{
    return address < ageMark_;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
