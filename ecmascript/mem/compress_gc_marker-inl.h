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

#ifndef ECMASCRIPT_MEM_COMPRESS_GC_MARKER_INL_H
#define ECMASCRIPT_MEM_COMPRESS_GC_MARKER_INL_H

#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/compress_gc_marker.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
static constexpr int HEAD_SIZE = TaggedObject::TaggedObjectSize();

inline CompressGCMarker::CompressGCMarker(CompressCollector *compressCollector) : collector_(compressCollector) {}

inline void CompressGCMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungAndOldGeneration()) {
        auto markBitmap = objectRegion->GetMarkBitmap();
        if (!markBitmap->AtomicTestAndSet(object)) {
            collector_->workList_->Push(threadId, object);
        }
        return;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        return;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

inline void CompressGCMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
                                             ObjectSlot slot)
{
    auto klass = markWord.GetJSHClass();
    JSType jsType = klass->GetObjectType();
    auto size = klass->SizeFromJSHClass(jsType, object);
    uintptr_t forwardAddress = collector_->workList_->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::OLD_SPACE);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "alloc failed";
        UNREACHABLE();
    }

    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage, hicpp-signed-bitwise)
    bool result = Barriers::AtomicSetDynPrimitive(object, 0, markWord.GetValue(),
                                                  MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        CopyObjectWithoutHeader(object, forwardAddress, size);
        collector_->workList_->AddAliveSize(threadId, size);
        *reinterpret_cast<uintptr_t *>(forwardAddress) = markWord.GetValue();
        if (klass->HasReferenceField(jsType)) {
            collector_->workList_->Push(threadId, reinterpret_cast<TaggedObject *>(forwardAddress));
        }
        slot.Update(reinterpret_cast<TaggedObject *>(forwardAddress));
        return;
    }
    FreeObject::FillFreeObject(collector_->heap_->GetEcmaVM(), forwardAddress,
                               AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    auto dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
}

inline void CompressGCMarker::CopyObjectWithoutHeader(TaggedObject *object, uintptr_t address, size_t size)
{
    if (memcpy_s(ToVoidPtr(address + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(ToUintPtr(object) + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_COMPRESS_GC_MARKER_INL_H
