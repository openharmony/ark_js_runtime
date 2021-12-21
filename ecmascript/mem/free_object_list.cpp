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

FreeObject *FreeObjectList::Allocator(size_t size)
{
    if (noneEmptyKindBitMap_ == 0) {
        return nullptr;
    }
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
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
                object = current->SearchSmallFreeObject(size);
            } else {
                next = current->next_;
                object = current->SearchLargeFreeObject(size);
            }
            if (current->Empty()) {
                RemoveKind(current);
            }
            if (object != nullptr) {
                size_t objectSize = object->Available();
                available_ -= objectSize;
                if (objectSize >= size) {
                    return object;
                }
            }
            current = next;
        }
    }
    return nullptr;
}

void FreeObjectList::Free(uintptr_t start, size_t size, bool isAdd)
{
    if (start == 0 || size == 0) {
        return;
    }
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
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

    if (isAdd) {
        if (kind->isAdded_) {
            available_ += size;
        } else {
            AddKind(kind);
        }
    }
}

void FreeObjectList::Rebuild()
{
    EnumerateKinds([](FreeObjectKind *kind) { kind->Rebuild(); });
    for (int i = 0; i < NUMBER_OF_KINDS; i++) {
        kinds_[i] = nullptr;
        lastKinds_[i] = nullptr;
    }
    available_ = 0;
    noneEmptyKindBitMap_ = 0;
}

size_t FreeObjectList::GetFreeObjectSize() const
{
    return available_;
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
    if (lastKinds_[type] == nullptr) {
        lastKinds_[type] = kind;
    }
    kinds_[type] = kind;
    SetNoneEmptyBit(type);
    available_ += kind->Available();
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
    if (kinds_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
    available_ -= kind->Available();
    kind->Rebuild();
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
    available_ += list->available_;
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
