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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_SERIALIZE_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_SERIALIZE_H

#include <iostream>
#include <fstream>
#include <sstream>

#include "libpandabase/macros.h"
#include "ecmascript/snapshot/mem/encode_bit.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/object_xray.h"

namespace panda::ecmascript {
class EcmaVM;
class JSPandaFile;

enum class SnapShotType {
    VM_ROOT,
    GLOBAL_CONST,
    TS_LOADER
};

class SnapShotSerialize final {
public:
    explicit SnapShotSerialize(EcmaVM *vm) : vm_(vm), objXRay_(vm) {}
    ~SnapShotSerialize() = default;

    void Serialize(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                   std::unordered_map<uint64_t, std::pair<uint64_t, ecmascript::EncodeBit>> *data);
    void Relocate(SnapShotType type, const JSPandaFile *jsPandaFile, uint64_t rootObjSize);
    void SerializePandaFileMethod();
    EncodeBit EncodeTaggedObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, std::pair<uint64_t, ecmascript::EncodeBit>> *data);
    void EncodeTaggedObjectRange(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, std::pair<uint64_t, ecmascript::EncodeBit>> *data);

    void SetProgramSerializeStart()
    {
        programSerialize_ = true;
    }

    void GeneratedNativeMethod();
    size_t GetNativeTableSize() const;

private:
    void SetObjectEncodeField(uintptr_t obj, size_t offset, uint64_t value);

    EncodeBit HandleObjectHeader(TaggedObject *objectHeader, uint8_t objectType, CQueue<TaggedObject *> *queue,
                                 std::unordered_map<uint64_t, std::pair<uint64_t, ecmascript::EncodeBit>> *data);
    uint64_t HandleTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                               std::unordered_map<uint64_t, std::pair<uint64_t, ecmascript::EncodeBit>> *data);
    void DeserializeHandleField(TaggedObject *objectHeader);
    void DeserializeHandleTaggedField(uint64_t *value);
    void DeserializeHandleNativePointer(uint64_t *value);
    void DeserializeHandleClassWord(TaggedObject *object);
    void DeserializeHandleRootObject(SnapShotType type, uintptr_t rootObjectAddr, uint8_t objType, size_t objIndex);

    EncodeBit NativePointerToEncodeBit(void *nativePointer);
    void *NativePointerEncodeBitToAddr(EncodeBit nativeBit);
    uint16_t SearchNativeMethodIndex(void *nativePointer);
    uintptr_t TaggedObjectEncodeBitToAddr(EncodeBit taggedBit);

    EcmaVM *vm_ {nullptr};
    ObjectXRay objXRay_;
    bool programSerialize_ {false};
    CVector<uintptr_t> pandaMethod_;

    NO_COPY_SEMANTIC(SnapShotSerialize);
    NO_MOVE_SEMANTIC(SnapShotSerialize);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_SERIALIZE_H
