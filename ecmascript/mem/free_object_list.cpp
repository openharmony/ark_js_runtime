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

#include "ecmascript/mem/free_object_list.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
FreeObjectList::FreeObjectList() : sets_(new FreeObjectSet *[NUMBER_OF_SETS](), NUMBER_OF_SETS),
    lastSets_(new FreeObjectSet *[NUMBER_OF_SETS](), NUMBER_OF_SETS)
{
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    noneEmptySetBitMap_ = 0;
}

FreeObjectList::~FreeObjectList()
{
    delete[] sets_.data();
    delete[] lastSets_.data();
    noneEmptySetBitMap_ = 0;
}

FreeObject *FreeObjectList::Allocate(size_t size)
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find from suitable
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet::INVALID_SET_TYPE) {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
        FreeObjectSet *current = sets_[type];
        while (current != nullptr) {
            if (current->Available() < size) {
                current = current->next_;
                continue;
            }
            FreeObjectSet *next = nullptr;
            FreeObject *object = nullptr;
            if (type <= SMALL_SET_MAX_INDEX) {
                object = current->ObtainSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->ObtainLargeFreeObject(size);
            }
            if (current->Empty()) {
                RemoveSet(current);
                current->Rebuild();
            }
            if (object != nullptr) {
                available_ -= object->Available();
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}

FreeObject *FreeObjectList::LookupSuitableFreeObject(size_t size)
{
    if (noneEmptySetBitMap_ == 0) {
        return nullptr;
    }
    // find a suitable type
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet::INVALID_SET_TYPE) {
        return nullptr;
    }

    SetType lastType = type - 1;
    for (type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type)); type > lastType && type < NUMBER_OF_SETS;
        type = static_cast<int32_t>(CalcNextNoneEmptyIndex(type + 1))) {
        lastType = type;
        FreeObjectSet *current = sets_[type];
        while (current != nullptr) {
            FreeObjectSet *next = nullptr;
            FreeObject *object = nullptr;
            if (type <= SMALL_SET_MAX_INDEX) {
                object = current->LookupSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->LookupLargeFreeObject(size);
            }
            if (object != nullptr) {
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}

void FreeObjectList::Free(uintptr_t start, size_t size, bool isAdd)
{
    if (UNLIKELY(start == 0)) {
        return;
    }
    if (UNLIKELY(size < MIN_SIZE)) {
        Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
        region->IncrementWasted(size);
        if (isAdd) {
            wasted_ += size;
        }
        return;
    }
    SetType type = SelectSetType(size);
    if (type == FreeObjectSet::INVALID_SET_TYPE) {
        return;
    }

    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
    auto set = region->GetFreeObjectSet(type);
    if (set == nullptr) {
        LOG_ECMA(FATAL) << "The set of region is nullptr";
        return;
    }
    set->Free(start, size);

    if (isAdd) {
        if (!set->isAdded_) {
            AddSet(set);
        } else {
            available_ += size;
        }
    }
}

void FreeObjectList::Rebuild()
{
    EnumerateSets([](FreeObjectSet *set) { set->Rebuild(); });
    for (int i = 0; i < NUMBER_OF_SETS; i++) {
        sets_[i] = nullptr;
        lastSets_[i] = nullptr;
    }
    available_ = 0;
    wasted_ = 0;
    noneEmptySetBitMap_ = 0;
}

bool FreeObjectList::AddSet(FreeObjectSet *set)
{
    if (set == nullptr || set->Empty() || set->isAdded_) {
        return false;
    }
    SetType type = set->setType_;
    FreeObjectSet *top = sets_[type];
    if (set == top) {
        return false;
    }
    if (top != nullptr) {
        top->prev_ = set;
    }
    set->isAdded_ = true;
    set->next_ = top;
    set->prev_ = nullptr;
    if (lastSets_[type] == nullptr) {
        lastSets_[type] = set;
    }
    if (sets_[type] == nullptr) {
        SetNoneEmptyBit(type);
    }
    sets_[type] = set;
    available_ += set->Available();
    return true;
}

void FreeObjectList::RemoveSet(FreeObjectSet *set)
{
    if (set == nullptr) {
        return;
    }
    SetType type = set->setType_;
    FreeObjectSet *top = sets_[type];
    FreeObjectSet *end = lastSets_[type];
    if (top == set) {
        sets_[type] = top->next_;
    }
    if (end == set) {
        lastSets_[type] = end->prev_;
    }
    if (set->prev_ != nullptr) {
        set->prev_->next_ = set->next_;
    }
    if (set->next_ != nullptr) {
        set->next_->prev_ = set->prev_;
    }
    set->isAdded_ = false;
    set->prev_ = nullptr;
    set->next_ = nullptr;
    if (sets_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
    available_ -= set->Available();
}

void FreeObjectList::Merge(FreeObjectList *list)
{
    list->EnumerateTopAndLastSets([this](FreeObjectSet *set, FreeObjectSet *end) {
        if (set == nullptr || set->Empty()) {
            return;
        }
        SetType type = set->setType_;
        FreeObjectSet *top = sets_[type];
        if (top == nullptr) {
            top = set;
        } else {
            lastSets_[type]->next_ = set;
            set->prev_ = lastSets_[type];
        }
        lastSets_[type] = end;
        SetNoneEmptyBit(type);
    });
    available_ += list->available_;
    list->Rebuild();
}

template<class Callback>
void FreeObjectList::EnumerateSets(const Callback &cb) const
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        EnumerateSets(i, cb);
    }
}

template<class Callback>
void FreeObjectList::EnumerateSets(SetType type, const Callback &cb) const
{
    FreeObjectSet *current = sets_[type];
    while (current != nullptr) {
        // maybe reset
        FreeObjectSet *next = current->next_;
        cb(current);
        current = next;
    }
}

template<class Callback>
void FreeObjectList::EnumerateTopAndLastSets(const Callback &cb) const
{
    for (SetType i = 0; i < NUMBER_OF_SETS; i++) {
        cb(sets_[i], lastSets_[i]);
    }
}
}  // namespace panda::ecmascript
