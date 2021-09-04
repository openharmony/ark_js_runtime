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

#include "ecmascript/snapshot/mem/slot_bit.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
/* static */
uint8_t SerializeHelper::GetObjectType(TaggedObject *objectHeader)
{
    auto hclass = objectHeader->GetClass();
    return static_cast<uint8_t>(JSHClass::Cast(hclass)->GetObjectType());
}

/* static */
SlotBit SerializeHelper::AddObjectHeaderToData(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                               std::unordered_map<uint64_t, ecmascript::SlotBit> *data, size_t index)
{
    queue->emplace(objectHeader);
    SlotBit slotBits(data->size());
    slotBits.SetObjectInConstantsIndex(index);
    data->emplace(ToUintPtr(objectHeader), slotBits);
    return slotBits;
}

void SerializeHelper::AddTaggedObjectRangeToData(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                                 std::unordered_map<uint64_t, ecmascript::SlotBit> *data)
{
    size_t index = 0;
    while (start < end) {
        JSTaggedValue object(start.GetTaggedType());
        index++;
        start++;
        if (object.IsHeapObject()) {
            SlotBit slotBit(0);
            if (data->find(object.GetRawData()) == data->end()) {
                slotBit = AddObjectHeaderToData(object.GetTaggedObject(), queue, data, index);
            }
        }
    }
}
}  // namespace panda::ecmascript
