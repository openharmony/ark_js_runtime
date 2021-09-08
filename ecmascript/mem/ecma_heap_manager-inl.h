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

#ifndef ECMASCRIPT_MEM_HEAP_MANAGER_INL_H
#define ECMASCRIPT_MEM_HEAP_MANAGER_INL_H

#include "ecmascript/mem/ecma_heap_manager.h"

#include <ctime>

#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript {
TaggedObject *EcmaHeapManager::AllocateYoungGenerationOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateYoungGenerationOrHugeObject(hclass, size);
}

TaggedObject *EcmaHeapManager::AllocateYoungGenerationOrHugeObject(JSHClass *hclass, size_t size)
{
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    // regular objects
    auto object = reinterpret_cast<TaggedObject *>(newSpaceAllocator_.Allocate(size));
    if (LIKELY(object != nullptr)) {
        SetClass(object, hclass);
        heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
        return object;
    }

    // hclass must nonmovable
    if (!heap_->FillNewSpaceAndTryGC(&newSpaceAllocator_)) {
        LOG_ECMA_MEM(FATAL) << "OOM : extend failed";
        UNREACHABLE();
    }
    object = reinterpret_cast<TaggedObject *>(newSpaceAllocator_.Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        heap_->CollectGarbage(TriggerGCType::SEMI_GC);
        object = reinterpret_cast<TaggedObject *>(newSpaceAllocator_.Allocate(size));
        if (UNLIKELY(object == nullptr)) {
            heap_->CollectGarbage(TriggerGCType::OLD_GC);
            object = reinterpret_cast<TaggedObject *>(newSpaceAllocator_.Allocate(size));
            if (UNLIKELY(object == nullptr)) {
                heap_->ThrowOutOfMemoryError(size);
                UNREACHABLE();
            }
        }
    }

    SetClass(object, hclass);
    heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *EcmaHeapManager::TryAllocateYoungGeneration(size_t size)
{
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    return reinterpret_cast<TaggedObject *>(newSpaceAllocator_.Allocate(size));
}

TaggedObject *EcmaHeapManager::AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size)
{
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(nonMovableAllocator_.Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        if (heap_->CheckAndTriggerNonMovableGC()) {
            object = reinterpret_cast<TaggedObject *>(nonMovableAllocator_.Allocate(size));
        }
        if (UNLIKELY(object == nullptr)) {
            // hclass must be nonmovable
            if (!heap_->FillNonMovableSpaceAndTryGC(&nonMovableAllocator_)) {
                LOG_ECMA_MEM(FATAL) << "OOM : extend failed";
                UNREACHABLE();
            }
            object = reinterpret_cast<TaggedObject *>(nonMovableAllocator_.Allocate(size));
            if (UNLIKELY(object == nullptr)) {
                heap_->ThrowOutOfMemoryError(size);
                UNREACHABLE();
            }
        }
    }
    SetClass(object, hclass);
    heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

uintptr_t EcmaHeapManager::AllocateSnapShotSpace(size_t size)
{
    uintptr_t object = snapshotSpaceAllocator_.Allocate(size);
    if (UNLIKELY(object == 0)) {
        if (!heap_->FillSnapShotSpace(&snapshotSpaceAllocator_)) {
            LOG_ECMA_MEM(FATAL) << "alloc failed";
            UNREACHABLE();
        }
        object = snapshotSpaceAllocator_.Allocate(size);
        if (UNLIKELY(object == 0)) {
            LOG_ECMA_MEM(FATAL) << "alloc failed";
            UNREACHABLE();
        }
    }
    return object;
}

void EcmaHeapManager::SetClass(TaggedObject *header, JSHClass *hclass)
{
    header->SetClass(hclass);
}

TaggedObject *EcmaHeapManager::AllocateNonMovableOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateNonMovableOrHugeObject(hclass, size);
}

TaggedObject *EcmaHeapManager::AllocateOldGenerationOrHugeObject(JSHClass *hclass, size_t size)
{
    ASSERT(size > 0);
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(oldSpaceAllocator_.Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        if (heap_->CheckAndTriggerOldGC()) {
            object = reinterpret_cast<TaggedObject *>(oldSpaceAllocator_.Allocate(size));
        }
        if (UNLIKELY(object == nullptr)) {
            // hclass must nonmovable
            if (!heap_->FillOldSpaceAndTryGC(&oldSpaceAllocator_)) {
                LOG_ECMA_MEM(FATAL) << "OOM : extend failed";
                UNREACHABLE();
            }
            object = reinterpret_cast<TaggedObject *>(oldSpaceAllocator_.Allocate(size));
            if (UNLIKELY(object == nullptr)) {
                heap_->ThrowOutOfMemoryError(size);
                UNREACHABLE();
            }
        }
    }
    SetClass(object, hclass);
    heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *EcmaHeapManager::AllocateHugeObject(JSHClass *hclass, size_t size)
{
    ASSERT(size > MAX_REGULAR_HEAP_OBJECT_SIZE);
    // large objects
    heap_->CheckAndTriggerOldGC();
    auto *object =
        reinterpret_cast<TaggedObject *>(const_cast<HugeObjectSpace *>(heap_->GetHugeObjectSpace())->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        heap_->CollectGarbage(TriggerGCType::HUGE_GC);
        object = reinterpret_cast<TaggedObject *>(
                    const_cast<HugeObjectSpace *>(heap_->GetHugeObjectSpace())->Allocate(size));
        if (UNLIKELY(object == nullptr)) {
            heap_->ThrowOutOfMemoryError(size);
            UNREACHABLE();
        }
    }
    SetClass(object, hclass);
    heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}

TaggedObject *EcmaHeapManager::AllocateMachineCodeSpaceObject(JSHClass *hclass, size_t size)
{
    auto object = reinterpret_cast<TaggedObject *>(machineCodeSpaceAllocator_.Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        if (!heap_->FillMachineCodeSpaceAndTryGC(&machineCodeSpaceAllocator_)) {
            return nullptr;
        }
        object = reinterpret_cast<TaggedObject *>(machineCodeSpaceAllocator_.Allocate(size));
        if (UNLIKELY(object == nullptr)) {
            heap_->ThrowOutOfMemoryError(size);
            return nullptr;
        }
    }
    SetClass(object, hclass);
    heap_->OnAllocateEvent(reinterpret_cast<uintptr_t>(object));
    return object;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_MANAGER_INL_H
