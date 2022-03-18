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

#ifndef ECMASCRIPT_MEM_REGION_INL_H
#define ECMASCRIPT_MEM_REGION_INL_H

#include "ecmascript/mem/region.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
inline RememberedSet *Region::CreateRememberedSet()
{
    auto setSize = RememberedSet::GetSizeInByte(GetCapacity());
    auto setAddr = const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->Allocate(setSize);
    uintptr_t setData = ToUintPtr(setAddr);
    auto ret = new RememberedSet(ToUintPtr(this), GetCapacity(), setData);
    ret->ClearAllBits();
    return ret;
}

inline RememberedSet *Region::GetOrCreateCrossRegionRememberedSet()
{
    if (UNLIKELY(crossRegionSet_ == nullptr)) {
        os::memory::LockHolder lock(lock_);
        if (crossRegionSet_ == nullptr) {
            crossRegionSet_ = CreateRememberedSet();
        }
    }
    return crossRegionSet_;
}

inline RememberedSet *Region::GetOrCreateOldToNewRememberedSet()
{
    if (UNLIKELY(oldToNewSet_ == nullptr)) {
        os::memory::LockHolder lock(lock_);
        if (oldToNewSet_ == nullptr) {
            oldToNewSet_ = CreateRememberedSet();
        }
    }
    return oldToNewSet_;
}

inline void Region::InsertCrossRegionRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateCrossRegionRememberedSet();
    set->Insert(addr);
}

inline void Region::AtomicInsertCrossRegionRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateCrossRegionRememberedSet();
    set->AtomicInsert(addr);
}

inline void Region::InsertOldToNewRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateOldToNewRememberedSet();
    set->Insert(addr);
}

inline void Region::AtomicInsertOldToNewRememberedSet(uintptr_t addr)
{
    auto set = GetOrCreateOldToNewRememberedSet();
    set->AtomicInsert(addr);
}

inline WorkerHelper *Region::GetWorkList() const
{
    return heap_->GetWorkList();
}

inline void Region::DeleteMarkBitmap()
{
    if (markBitmap_ != nullptr) {
        auto size = RangeBitmap::GetBitMapSizeInByte(GetCapacity());
        const_cast<NativeAreaAllocator *>(
            heap_->GetNativeAreaAllocator())->Free(markBitmap_->GetBitMap().Data(), size);
        delete markBitmap_;
        markBitmap_ = nullptr;
    }
}

inline void Region::DeleteCrossRegionRememberedSet()
{
    if (crossRegionSet_ != nullptr) {
        auto size = RememberedSet::GetSizeInByte(GetCapacity());
        const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->Free(
            crossRegionSet_->GetBitMap().Data(), size);
        delete crossRegionSet_;
        crossRegionSet_ = nullptr;
    }
}

inline void Region::DeleteOldToNewRememberedSet()
{
    if (oldToNewSet_ != nullptr) {
        auto size = RememberedSet::GetSizeInByte(GetCapacity());
        const_cast<NativeAreaAllocator *>(heap_->GetNativeAreaAllocator())->Free(
            oldToNewSet_->GetBitMap().Data(), size);
        delete oldToNewSet_;
        oldToNewSet_ = nullptr;
    }
}

inline void Region::ClearMarkBitmap()
{
    if (markBitmap_ != nullptr) {
        markBitmap_->ClearAllBits();
    }
}

inline void Region::ClearCrossRegionRememberedSet()
{
    if (crossRegionSet_ != nullptr) {
        crossRegionSet_->ClearAllBits();
    }
}

inline bool Region::IsMarking() const
{
    return !heap_->GetJSThread()->IsReadyToMark();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_REGION_INL_H
