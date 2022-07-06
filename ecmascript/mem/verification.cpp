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

#include "verification.h"

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
// Verify the object body
void VerifyObjectVisitor::VisitAllObjects(TaggedObject *obj)
{
    auto jsHclass = obj->GetClass();
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(
        obj, jsHclass, [this]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end,
                              [[maybe_unused]] bool isNative) {
            for (ObjectSlot slot = start; slot < end; slot++) {
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsWeak()) {
                    if (!heap_->IsAlive(value.GetTaggedWeakRef())) {
                        LOG_GC(ERROR) << "Heap verify detected a dead weak object " << value.GetTaggedObject()
                                            << " at object:" << slot.SlotAddress();
                        ++(*failCount_);
                    }
                } else if (value.IsHeapObject()) {
                    if (!heap_->IsAlive(value.GetTaggedObject())) {
                        LOG_GC(ERROR) << "Heap verify detected a dead object at " << value.GetTaggedObject()
                                            << " at object:" << slot.SlotAddress();
                        ++(*failCount_);
                    }
                }
            }
        });
}

size_t Verification::VerifyRoot() const
{
    size_t failCount = 0;
    RootVisitor visit1 = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsWeak()) {
            VerifyObjectVisitor(heap_, &failCount)(value.GetTaggedWeakRef());
        } else if (value.IsHeapObject()) {
            VerifyObjectVisitor(heap_, &failCount)(value.GetTaggedObject());
        }
    };
    RootRangeVisitor visit2 = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsWeak()) {
                VerifyObjectVisitor(heap_, &failCount)(value.GetTaggedWeakRef());
            } else if (value.IsHeapObject()) {
                VerifyObjectVisitor(heap_, &failCount)(value.GetTaggedObject());
            }
        }
    };
    objXRay_.VisitVMRoots(visit1, visit2);
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyRoot detects deadObject count is " << failCount;
    }

    return failCount;
}

size_t Verification::VerifyHeap() const
{
    size_t failCount = heap_->VerifyHeapObjects();
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyHeap detects deadObject count is " << failCount;
    }
    return failCount;
}
}  // namespace panda::ecmascript
