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
#include "ecmascript/mem/free_object_kind.h"
#include "ecmascript/mem/free_object_list-inl.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
FreeObjectList::FreeObjectList() : kinds_(new FreeObjectKind *[NUMBER_OF_KINDS](), NUMBER_OF_KINDS),
    lastKinds_(new FreeObjectKind *[NUMBER_OF_KINDS](), NUMBER_OF_KINDS)
{
    for (int i = 0; i < NUMBER_OF_KINDS; i++) {
        kinds_[i] = nullptr;
        lastKinds_[i] = nullptr;
    }
    noneEmptyKindBitMap_ = 0;
}

FreeObjectList::~FreeObjectList()
{
    delete[] kinds_.data();
    delete[] lastKinds_.data();
    noneEmptyKindBitMap_ = 0;
}

FreeObject *FreeObjectList::Allocate(size_t size)
{
    if (noneEmptyKindBitMap_ == 0) {
        return nullptr;
    }
    // find from suitable
    KindType type = SelectKindType(size);
    if (type == FreeObjectKind::INVALID_KIND_TYPE) {
        return nullptr;
    }

    KindType lastType = type - 1;
    for (type = CalcNextNoneEmptyIndex(type); type > lastType && type < NUMBER_OF_KINDS;
        type = CalcNextNoneEmptyIndex(type + 1)) {
        lastType = type;
        FreeObjectKind *current = kinds_[type];
        while (current != nullptr) {
            if (current->Available() < size) {
                current = current->next_;
                continue;
            }
            FreeObjectKind *next = nullptr;
            FreeObject *object = nullptr;
            if (type <= SMALL_KIND_MAX_INDEX) {
                object = current->ObtainSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->ObtainLargeFreeObject(size);
            }
            if (current->Empty()) {
                RemoveKind(current);
                current->Rebuild();
            }
            if (object != nullptr) {
#ifndef NDEBUG
                available_ -= object->Available();
#endif
                return object;
            }
            current = next;
        }
    }
    return nullptr;
}

FreeObject *FreeObjectList::LookupSuitableFreeObject(size_t size)
{
    if (noneEmptyKindBitMap_ == 0) {
        return nullptr;
    }
    // find a suitable type
    KindType type = SelectKindType(size);
    if (type == FreeObjectKind::INVALID_KIND_TYPE) {
        return nullptr;
    }

    KindType lastType = type - 1;
    for (type = CalcNextNoneEmptyIndex(type); type > lastType && type < NUMBER_OF_KINDS;
        type = CalcNextNoneEmptyIndex(type + 1)) {
        lastType = type;
        FreeObjectKind *current = kinds_[type];
        while (current != nullptr) {
            FreeObjectKind *next = nullptr;
            FreeObject *object = nullptr;
            if (type <= SMALL_KIND_MAX_INDEX) {
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
#ifndef NDEBUG
        Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
        region->IncrementWasted(size);
        if (isAdd) {
            wasted_ += size;
        }
#endif
        return;
    }
    KindType type = SelectKindType(size);
    if (type == FreeObjectKind::INVALID_KIND_TYPE) {
        return;
    }

    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(start));
    auto kind = region->GetFreeObjectKind(type);
    if (kind == nullptr) {
        LOG_ECMA(FATAL) << "The kind of region is nullptr";
        return;
    }
    kind->Free(start, size);

    if (isAdd && !kind->isAdded_) {
        AddKind(kind);
    }
}

void FreeObjectList::Rebuild()
{
    EnumerateKinds([](FreeObjectKind *kind) { kind->Rebuild(); });
    for (int i = 0; i < NUMBER_OF_KINDS; i++) {
        kinds_[i] = nullptr;
        lastKinds_[i] = nullptr;
    }
#ifndef NDEBUG
    available_ = 0;
    wasted_ = 0;
#endif
    noneEmptyKindBitMap_ = 0;
}

bool FreeObjectList::AddKind(FreeObjectKind *kind)
{
    if (kind == nullptr || kind->Empty() || kind->isAdded_) {
        return false;
    }
    KindType type = kind->kindType_;
    FreeObjectKind *top = kinds_[type];
    if (kind == top) {
        return false;
    }
    if (top != nullptr) {
        top->prev_ = kind;
    }
    kind->isAdded_ = true;
    kind->next_ = top;
    kind->prev_ = nullptr;
    if (lastKinds_[type] == nullptr) {
        lastKinds_[type] = kind;
    }
    if (kinds_[type] == nullptr) {
        SetNoneEmptyBit(type);
    }
    kinds_[type] = kind;
#ifndef NDEBUG
    available_ += kind->Available();
#endif
    return true;
}

void FreeObjectList::RemoveKind(FreeObjectKind *kind)
{
    if (kind == nullptr) {
        return;
    }
    KindType type = kind->kindType_;
    FreeObjectKind *top = kinds_[type];
    FreeObjectKind *end = lastKinds_[type];
    if (top == kind) {
        kinds_[type] = top->next_;
    }
    if (end == kind) {
        lastKinds_[type] = end->prev_;
    }
    if (kind->prev_ != nullptr) {
        kind->prev_->next_ = kind->next_;
    }
    if (kind->next_ != nullptr) {
        kind->next_->prev_ = kind->prev_;
    }
    kind->isAdded_ = false;
    kind->prev_ = nullptr;
    kind->next_ = nullptr;
    if (kinds_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
#ifndef NDEBUG
    available_ -= kind->Available();
#endif
}

void FreeObjectList::Merge(FreeObjectList *list)
{
    list->EnumerateTopAndLastKinds([this](FreeObjectKind *kind, FreeObjectKind *end) {
        if (kind == nullptr || kind->Empty()) {
            return;
        }
        KindType type = kind->kindType_;
        FreeObjectKind *top = kinds_[type];
        if (top == nullptr) {
            top = kind;
        } else {
            lastKinds_[type]->next_ = kind;
            kind->prev_ = lastKinds_[type];
        }
        lastKinds_[type] = end;
        SetNoneEmptyBit(type);
    });
#ifndef NDEBUG
    available_ += list->available_;
#endif
    list->Rebuild();
}

template<class Callback>
void FreeObjectList::EnumerateKinds(const Callback &cb) const
{
    for (KindType i = 0; i < NUMBER_OF_KINDS; i++) {
        EnumerateKinds(i, cb);
    }
}

template<class Callback>
void FreeObjectList::EnumerateKinds(KindType type, const Callback &cb) const
{
    FreeObjectKind *current = kinds_[type];
    while (current != nullptr) {
        // maybe reset
        FreeObjectKind *next = current->next_;
        cb(current);
        current = next;
    }
}

template<class Callback>
void FreeObjectList::EnumerateTopAndLastKinds(const Callback &cb) const
{
    for (KindType i = 0; i < NUMBER_OF_KINDS; i++) {
        cb(kinds_[i], lastKinds_[i]);
    }
}
}  // namespace panda::ecmascript
