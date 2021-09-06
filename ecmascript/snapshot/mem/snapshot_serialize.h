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

#ifndef PANDA_ECMASCRIPT_MEM_SNAPSHOT_SERIALIZE_H
#define PANDA_ECMASCRIPT_MEM_SNAPSHOT_SERIALIZE_H

#include <iostream>
#include <fstream>
#include <sstream>

#include "libpandabase/macros.h"
#include "ecmascript/snapshot/mem/slot_bit.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class EcmaVM;

class SnapShotSerialize final {
public:
    explicit SnapShotSerialize(EcmaVM *vm, bool serialize);
    ~SnapShotSerialize();

    void Serialize(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                   std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void RedirectSlot(const panda_file::File *pf);
    void SerializePandaFileMethod();

    void SetProgramSerializeStart()
    {
        programSerialize_ = true;
    }

    void RegisterNativeMethod();
    void GeneratedNativeMethod();

private:
    void SetObjectSlotField(uintptr_t obj, size_t offset, uint64_t value);
    void SetObjectSlotFieldUint32(uintptr_t obj, size_t offset, uint32_t value);

    void NativePointerSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj);
    void JSObjectSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                           CQueue<TaggedObject *> *queue,
                           std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void JSFunctionBaseSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                                 CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void JSProxySerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                          CQueue<TaggedObject *> *queue,
                          std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void DynClassSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                           CQueue<TaggedObject *> *queue,
                           std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void DynArraySerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, CQueue<TaggedObject *> *queue,
                           std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void DynStringSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj);
    void DynProgramSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, CQueue<TaggedObject *> *queue,
                             std::unordered_map<uint64_t, ecmascript::SlotBit> *data);

    void NativePointerDeserialize(uint64_t *objectHeader);
    void JSObjectDeserialize(uint64_t *objectHeader, size_t objectSize);
    void JSFunctionBaseDeserialize(uint64_t *objectHeader, size_t objectSize);
    void JSProxyDeserialize(uint64_t *objectHeader, size_t objectSize);
    void DynClassDeserialize(uint64_t *objectHeader);
    void DynStringDeserialize(uint64_t *objectHeader);
    void DynArrayDeserialize(uint64_t *objectHeader);
    void DynProgramDeserialize(uint64_t *objectHeader, size_t objectSize);

    SlotBit HandleObjectHeader(TaggedObject *objectHeader, uint8_t objectType, size_t objectSize,
                               CQueue<TaggedObject *> *queue,
                               std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    uint64_t HandleTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                               std::unordered_map<uint64_t, ecmascript::SlotBit> *data);
    void DeserializeHandleTaggedField(uint64_t *value);
    void DeserializeHandleClassWord(TaggedObject *object);
    void ExtendObjectArray();

    void SetAddressToSlot(size_t index, uintptr_t value)
    {
        auto addr = reinterpret_cast<uintptr_t *>(addressSlot_ + index * ADDRESS_SIZE);
        *addr = value;
    }

    template <typename T>
    T GetAddress(size_t index)
    {
        return *reinterpret_cast<T *>(addressSlot_ + index * ADDRESS_SIZE);
    }

    SlotBit NativePointerToSlotBit(void *nativePointer);
    void *NativePointerSlotBitToAddr(SlotBit native);
    void DeserializeRangeTaggedField(size_t beginAddr, int numOfFields);

    EcmaVM *vm_{nullptr};
    bool serialize_{false};
    bool programSerialize_{false};
    int count_{0};
    int objectArraySize_{0};
    uintptr_t addressSlot_;
    CVector<uintptr_t> pandaMethod_;

    NO_COPY_SEMANTIC(SnapShotSerialize);
    NO_MOVE_SEMANTIC(SnapShotSerialize);
};
}  // namespace panda::ecmascript

#endif  // PANDA_ECMASCRIPT_MEM_SNAPSHOT_SERIALIZE_H
