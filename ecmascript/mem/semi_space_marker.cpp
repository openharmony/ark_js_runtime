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

#include "ecmascript/free_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/semi_space_collector-inl.h"
#include "ecmascript/mem/semi_space_marker.h"
#include "ecmascript/mem/semi_space_worker.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
constexpr int HEAD_SIZE = TaggedObject::ObjectHeaderSize();

SemiSpaceMarker::SemiSpaceMarker(SemiSpaceCollector *semiSpaceCollector) : collector_(semiSpaceCollector) {}

SlotStatus SemiSpaceMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungGeneration()) {
        return SlotStatus::CLEAR_SLOT;
    }
    return MarkYoungGenerationObject(threadId, object, slot);
}

SlotStatus SemiSpaceMarker::MarkYoungGenerationObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        Region *valueRegion = Region::ObjectAddressToRange(dst);
        return valueRegion->InYoungGeneration() ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

SlotStatus SemiSpaceMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
                                           ObjectSlot slot)
{
    auto klass = markWord.GetJSHClass();
    JSType jsType = klass->GetObjectType();
    auto size = klass->SizeFromJSHClass(jsType, object);
    bool isPromoted = ShouldBePromoted(object);
    uintptr_t forwardAddress;
    if (isPromoted) {
        forwardAddress = collector_->workList_->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::OLD_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            forwardAddress = collector_->workList_->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::YOUNG_SPACE);
            if (UNLIKELY(forwardAddress == 0)) {
                LOG_ECMA_MEM(FATAL) << "alloc failed";
                UNREACHABLE();
            }
            isPromoted = false;
        }
    } else {
        forwardAddress = collector_->workList_->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::YOUNG_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            forwardAddress = collector_->workList_->GetTlabAllocator(threadId)->Allocate(size, SpaceAlloc::OLD_SPACE);
            if (UNLIKELY(forwardAddress == 0)) {
                LOG_ECMA_MEM(FATAL) << "alloc failed";
                UNREACHABLE();
            }
            isPromoted = true;
        }
    }

    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage, hicpp-signed-bitwise)
    bool result = Barriers::AtomicSetDynPrimitive(object, 0, markWord.GetValue(),
                                                  MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        CopyObjectWithoutHeader(object, forwardAddress, size);
        *reinterpret_cast<MarkWordType *>(forwardAddress) = markWord.GetValue();
        collector_->workList_->AddAliveSize(threadId, size);
        if (isPromoted) {
            collector_->workList_->AddPromoteSize(threadId, size);
        }
        collector_->heap_->OnMoveEvent(reinterpret_cast<intptr_t>(object), forwardAddress);
        if (klass->HasReferenceField(jsType)) {
            collector_->workList_->Push(threadId, reinterpret_cast<TaggedObject *>(forwardAddress));
        }
        slot.Update(reinterpret_cast<TaggedObject *>(forwardAddress));
        return isPromoted ? SlotStatus::CLEAR_SLOT : SlotStatus::KEEP_SLOT;
    }
    FreeObject::FillFreeObject(collector_->heap_->GetEcmaVM(), forwardAddress,
                               AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
    auto dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
    return Region::ObjectAddressToRange(dst)->InYoungGeneration() ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
}

void SemiSpaceMarker::CopyObjectWithoutHeader(TaggedObject *object, uintptr_t address, size_t size)
{
    if (memcpy_s(ToVoidPtr(address + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(ToUintPtr(object) + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
}

bool SemiSpaceMarker::ShouldBePromoted(TaggedObject *object)
{
    Region *region = Region::ObjectAddressToRange(object);
    return (region->BelowAgeMark() || (region->HasAgeMark() && collector_->BlowAgeMark(ToUintPtr(object))));
}
}  // namespace panda::ecmascript
