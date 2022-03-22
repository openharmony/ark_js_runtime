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

#include "ecmascript/js_serializer.h"

#include <malloc.h>
#include <vector>

#include "ecmascript/base/array_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "libpandabase/mem/mem.h"
#include "securec.h"

namespace panda::ecmascript {
constexpr size_t INITIAL_CAPACITY = 64;
constexpr int CAPACITY_INCREASE_RATE = 2;

bool JSSerializer::WriteType(SerializationUID id)
{
    uint8_t rawId = static_cast<uint8_t>(id);
    return WriteRawData(&rawId, sizeof(rawId));
}

// Write JSTaggedValue could be either a pointer to a HeapObject or a value
bool JSSerializer::SerializeJSTaggedValue(const JSHandle<JSTaggedValue> &value)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    if (!value->IsHeapObject()) {
        if (!WritePrimitiveValue(value)) {
            return false;
        }
    } else {
        if (!WriteTaggedObject(value)) {
            return false;
        }
    }
    return true;
}

// Write JSTaggedValue that is pure value
bool JSSerializer::WritePrimitiveValue(const JSHandle<JSTaggedValue> &value)
{
    if (value->IsNull()) {
        return WriteType(SerializationUID::JS_NULL);
    }
    if (value->IsUndefined()) {
        return WriteType(SerializationUID::JS_UNDEFINED);
    }
    if (value->IsTrue()) {
        return WriteType(SerializationUID::JS_TRUE);
    }
    if (value->IsFalse()) {
        return WriteType(SerializationUID::JS_FALSE);
    }
    if (value->IsInt()) {
        return WriteInt(value->GetInt());
    }
    if (value->IsDouble()) {
        return WriteDouble(value->GetDouble());
    }
    if (value->IsHole()) {
        return WriteType(SerializationUID::HOLE);
    }
    return false;
}

bool JSSerializer::WriteInt(int32_t value)
{
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::INT32)) {
        return false;
    }
    if (!WriteRawData(&value, sizeof(value))) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteDouble(double value)
{
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::DOUBLE)) {
        return false;
    }
    if (!WriteRawData(&value, sizeof(value))) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteBoolean(bool value)
{
    if (value) {
        return WriteType(SerializationUID::C_TRUE);
    }
    return WriteType(SerializationUID::C_FALSE);
}

bool JSSerializer::WriteRawData(const void *data, size_t length)
{
    if (length <= 0) {
        return false;
    }
    if ((bufferSize_ + length) > bufferCapacity_) {
        if (!AllocateBuffer(length)) {
            return false;
        }
    }
    errno_t rc;
    rc = memcpy_s(buffer_ + bufferSize_, bufferCapacity_ - bufferSize_, data, length);
    if (rc != EOK) {
        LOG(ERROR, RUNTIME) << "Failed to memcpy_s Data";
        return false;
    }
    bufferSize_ += length;
    return true;
}

bool JSSerializer::AllocateBuffer(size_t bytes)
{
    // Get internal heap size
    if (sizeLimit_ == 0) {
        uint64_t heapSize = thread_->GetEcmaVM()->GetJSOptions().GetInternalMemorySizeLimit();
        sizeLimit_ = heapSize;
    }
    size_t oldSize = bufferSize_;
    size_t newSize = oldSize + bytes;
    if (newSize > sizeLimit_) {
        return false;
    }
    if (bufferCapacity_ == 0) {
        if (bytes < INITIAL_CAPACITY) {
            buffer_ = reinterpret_cast<uint8_t *>(malloc(INITIAL_CAPACITY));
            if (buffer_ != nullptr) {
                bufferCapacity_ = INITIAL_CAPACITY;
                return true;
            } else {
                return false;
            }
        } else {
            buffer_ = reinterpret_cast<uint8_t *>(malloc(bytes));
            if (buffer_ != nullptr) {
                bufferCapacity_ = bytes;
                return true;
            } else {
                return false;
            }
        }
    }
    if (newSize > bufferCapacity_) {
        if (!ExpandBuffer(newSize)) {
            return false;
        }
    }
    return true;
}

bool JSSerializer::ExpandBuffer(size_t requestedSize)
{
    size_t newCapacity = bufferCapacity_ * CAPACITY_INCREASE_RATE;
    newCapacity = std::max(newCapacity, requestedSize);
    if (newCapacity > sizeLimit_) {
        return false;
    }
    uint8_t *newBuffer = reinterpret_cast<uint8_t *>(malloc(newCapacity));
    if (newBuffer == nullptr) {
        return false;
    }
    errno_t rc;
    rc = memcpy_s(newBuffer, newCapacity, buffer_, bufferSize_);
    if (rc != EOK) {
        LOG(ERROR, RUNTIME) << "Failed to memcpy_s Data";
        free(newBuffer);
        return false;
    }
    free(buffer_);
    buffer_ = newBuffer;
    bufferCapacity_ = newCapacity;
    return true;
}

// Transfer ownership of buffer, should not use this Serializer after release
std::pair<uint8_t *, size_t> JSSerializer::ReleaseBuffer()
{
    auto res = std::make_pair(buffer_, bufferSize_);
    buffer_ = nullptr;
    bufferSize_ = 0;
    bufferCapacity_ = 0;
    objectId_ = 0;
    return res;
}

bool JSSerializer::IsSerialized(uintptr_t addr) const
{
    if (referenceMap_.find(addr) != referenceMap_.end()) {
        return true;
    }
    return false;
}

bool JSSerializer::WriteIfSerialized(uintptr_t addr)
{
    size_t oldSize = bufferSize_;
    auto iter = referenceMap_.find(addr);
    if (iter == referenceMap_.end()) {
        return false;
    }
    uint64_t id = iter->second;
    if (!WriteType(SerializationUID::TAGGED_OBJECT_REFERNCE)) {
        return false;
    }
    if (!WriteRawData(&id, sizeof(uint64_t))) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

// Write HeapObject
bool JSSerializer::WriteTaggedObject(const JSHandle<JSTaggedValue> &value)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(value.GetTaggedValue().GetTaggedObject());
    bool serialized = IsSerialized(addr);
    if (serialized) {
        return WriteIfSerialized(addr);
    }
    referenceMap_.insert(std::pair(addr, objectId_));
    objectId_++;

    TaggedObject *taggedObject = value->GetTaggedObject();
    JSType type = taggedObject->GetClass()->GetObjectType();
    switch (type) {
        case JSType::JS_ERROR:
        case JSType::JS_EVAL_ERROR:
        case JSType::JS_RANGE_ERROR:
        case JSType::JS_REFERENCE_ERROR:
        case JSType::JS_TYPE_ERROR:
        case JSType::JS_URI_ERROR:
        case JSType::JS_SYNTAX_ERROR:
            return WriteJSError(value);
        case JSType::JS_DATE:
            return WriteJSDate(value);
        case JSType::JS_ARRAY:
            return WriteJSArray(value);
        case JSType::JS_MAP:
            return WriteJSMap(value);
        case JSType::JS_SET:
            return WriteJSSet(value);
        case JSType::JS_REG_EXP:
            return WriteJSRegExp(value);
        case JSType::JS_INT8_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_INT8_ARRAY);
        case JSType::JS_UINT8_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_UINT8_ARRAY);
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_UINT8_CLAMPED_ARRAY);
        case JSType::JS_INT16_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_INT16_ARRAY);
        case JSType::JS_UINT16_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_UINT16_ARRAY);
        case JSType::JS_INT32_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_INT32_ARRAY);
        case JSType::JS_UINT32_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_UINT32_ARRAY);
        case JSType::JS_FLOAT32_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_FLOAT32_ARRAY);
        case JSType::JS_FLOAT64_ARRAY:
            return WriteJSTypedArray(value, SerializationUID::JS_FLOAT64_ARRAY);
        case JSType::JS_ARRAY_BUFFER:
            return WriteJSArrayBuffer(value);
        case JSType::STRING:
            return WriteEcmaString(value);
        case JSType::JS_OBJECT:
            return WritePlainObject(value);
        default:
            break;
    }
    return false;
}

bool JSSerializer::WriteJSError(const JSHandle<JSTaggedValue> &value)
{
    size_t oldSize = bufferSize_;
    TaggedObject *taggedObject = value->GetTaggedObject();
    JSType errorType = taggedObject->GetClass()->GetObjectType();
    if (!WriteJSErrorHeader(errorType)) {
        return false;
    }
    auto globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> handleMsg = globalConst->GetHandledMessageString();
    JSHandle<JSTaggedValue> msg = JSObject::GetProperty(thread_, value, handleMsg).GetValue();
    // Write error message
    if (!SerializeJSTaggedValue(msg)) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteJSErrorHeader(JSType type)
{
    switch (type) {
        case JSType::JS_ERROR:
            return WriteType(SerializationUID::JS_ERROR);
        case JSType::JS_EVAL_ERROR:
            return WriteType(SerializationUID::EVAL_ERROR);
        case JSType::JS_RANGE_ERROR:
            return WriteType(SerializationUID::RANGE_ERROR);
        case JSType::JS_REFERENCE_ERROR:
            return WriteType(SerializationUID::REFERENCE_ERROR);
        case JSType::JS_TYPE_ERROR:
            return WriteType(SerializationUID::TYPE_ERROR);
        case JSType::JS_URI_ERROR:
            return WriteType(SerializationUID::URI_ERROR);
        case JSType::JS_SYNTAX_ERROR:
            return WriteType(SerializationUID::SYNTAX_ERROR);
        default:
            UNREACHABLE();
    }
    return false;
}

bool JSSerializer::WriteJSDate(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSDate> date = JSHandle<JSDate>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_DATE)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    double timeValue = date->GetTimeValue().GetDouble();
    if (!WriteDouble(timeValue)) {
        bufferSize_ = oldSize;
        return false;
    }
    double localOffset = date->GetLocalOffset().GetDouble();
    if (!WriteDouble(localOffset)) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteJSArray(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_ARRAY)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    uint32_t arrayLength = static_cast<uint32_t>(array->GetLength().GetInt());
    if (!WriteInt(arrayLength)) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteEcmaString(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<EcmaString> string = JSHandle<EcmaString>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::ECMASTRING)) {
        return false;
    }
    bool isUtf8 = string->IsUtf8();
    // write utf encode flag
    if (!WriteBoolean(isUtf8)) {
        bufferSize_ = oldSize;
        return false;
    }
    if (isUtf8) {
        size_t length = string->GetLength();
        if (!WriteInt(static_cast<int32_t>(length))) {
            bufferSize_ = oldSize;
            return false;
        }
        // skip writeRawData for empty EcmaString
        if (length == 0) {
            return true;
        }
        const uint8_t *data = string->GetDataUtf8();
        const uint8_t strEnd = '\0';
        if (!WriteRawData(data, length) || !WriteRawData(&strEnd, sizeof(uint8_t))) {
            bufferSize_ = oldSize;
            return false;
        }
    } else {
        size_t length = string->GetUtf16Length();
        ASSERT(length != 0);
        if (!WriteInt(static_cast<int32_t>(length))) {
            bufferSize_ = oldSize;
            return false;
        }
        const uint16_t *data = string->GetDataUtf16();
        if (!WriteRawData(data, length * sizeof(uint16_t))) {
            bufferSize_ = oldSize;
            return false;
        }
    }
    return true;
}

bool JSSerializer::WriteJSMap(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSMap> map = JSHandle<JSMap>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_MAP)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    int size = map->GetSize();
    if (!WriteInt(size)) {
        bufferSize_ = oldSize;
        return false;
    }
    for (int i = 0; i < size; i++) {
        JSHandle<JSTaggedValue> key(thread_, map->GetKey(i));
        if (!SerializeJSTaggedValue(key)) {
            bufferSize_ = oldSize;
            return false;
        }
        JSHandle<JSTaggedValue> val(thread_, map->GetValue(i));
        if (!SerializeJSTaggedValue(val)) {
            bufferSize_ = oldSize;
            return false;
        }
    }
    return true;
}

bool JSSerializer::WriteJSSet(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSSet> set = JSHandle<JSSet>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_SET)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    int size = set->GetSize();
    if (!WriteInt(size)) {
        bufferSize_ = oldSize;
        return false;
    }
    for (int i = 0; i < size; i++) {
        JSHandle<JSTaggedValue> val(thread_, set->GetValue(i));
        if (!SerializeJSTaggedValue(val)) {
            bufferSize_ = oldSize;
            return false;
        }
    }
    return true;
}

bool JSSerializer::WriteJSRegExp(const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSRegExp> regExp = JSHandle<JSRegExp>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_REG_EXP)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    uint32_t bufferSize = regExp->GetLength();
    if (!WriteInt(static_cast<int32_t>(bufferSize))) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write Accessor(ByteCodeBuffer) which is a pointer to a Dynbuffer
    JSHandle<JSTaggedValue> bufferValue(thread_, regExp->GetByteCodeBuffer());
    JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(bufferValue);
    void *dynBuffer = np->GetExternalPointer();
    if (!WriteRawData(dynBuffer, bufferSize)) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write Accessor(OriginalSource)
    JSHandle<JSTaggedValue> originalSource(thread_, regExp->GetOriginalSource());
    if (!SerializeJSTaggedValue(originalSource)) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write Accessor(OriginalFlags)
    JSHandle<JSTaggedValue> originalFlags(thread_, regExp->GetOriginalFlags());
    if (!SerializeJSTaggedValue(originalFlags)) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteJSTypedArray(const JSHandle<JSTaggedValue> &value, SerializationUID uId)
{
    JSHandle<JSTypedArray> typedArray = JSHandle<JSTypedArray>::Cast(value);
    size_t oldSize = bufferSize_;
    if (!WriteType(uId)) {
        return false;
    }
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write ACCESSORS(ViewedArrayBuffer) which is a pointer to an ArrayBuffer
    JSHandle<JSTaggedValue> viewedArrayBuffer(thread_, typedArray->GetViewedArrayBuffer());
    if (!WriteJSArrayBuffer(viewedArrayBuffer)) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write ACCESSORS(TypedArrayName)
    JSHandle<JSTaggedValue> typedArrayName(thread_, typedArray->GetTypedArrayName());
    if (!SerializeJSTaggedValue(typedArrayName)) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write ACCESSORS(ByteLength)
    JSTaggedValue byteLength = typedArray->GetByteLength();
    if (!WriteRawData(&byteLength, sizeof(JSTaggedValue))) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write ACCESSORS(ByteOffset)
    JSTaggedValue byteOffset = typedArray->GetByteOffset();
    if (!WriteRawData(&byteOffset, sizeof(JSTaggedValue))) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write ACCESSORS(ArrayLength)
    JSTaggedValue arrayLength = typedArray->GetArrayLength();
    if (!WriteRawData(&arrayLength, sizeof(JSTaggedValue))) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteNativeFunctionPointer(const JSHandle<JSTaggedValue> &value)
{
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::NATIVE_FUNCTION_POINTER)) {
        return false;
    }
    JSTaggedValue pointer = value.GetTaggedValue();
    if (!WriteRawData(&pointer, sizeof(JSTaggedValue))) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

bool JSSerializer::WriteJSArrayBuffer(const JSHandle<JSTaggedValue> &value)
{
    size_t oldSize = bufferSize_;
    JSHandle<JSArrayBuffer> arrayBuffer = JSHandle<JSArrayBuffer>::Cast(value);

    if (arrayBuffer->IsDetach()) {
        return false;
    }

    if (!WriteType(SerializationUID::JS_ARRAY_BUFFER)) {
        return false;
    }

    // Write Accessors(ArrayBufferByteLength)
    uint32_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    if (!WriteInt(arrayLength)) {
        bufferSize_ = oldSize;
        return false;
    }

    // write Accessor shared which indicate the C memory is shared
    bool shared = arrayBuffer->GetShared();
    if (!WriteBoolean(shared)) {
        bufferSize_ = oldSize;
        return false;
    }

    if (shared) {
        JSHandle<JSNativePointer> np(thread_, arrayBuffer->GetArrayBufferData());
        void *buffer = np->GetExternalPointer();
        uint64_t bufferAddr = (uint64_t)buffer;
        if (!WriteRawData(&bufferAddr, sizeof(uint64_t))) {
            bufferSize_ = oldSize;
            return false;
        }
    } else {
        // Write Accessors(ArrayBufferData) which is a pointer to a DynBuffer
        JSHandle<JSNativePointer> np(thread_, arrayBuffer->GetArrayBufferData());
        void *buffer = np->GetExternalPointer();
        if (!WriteRawData(buffer, arrayLength)) {
            bufferSize_ = oldSize;
            return false;
        }
    }

    // write obj properties
    if (!WritePlainObject(value)) {
        bufferSize_ = oldSize;
        return false;
    }

    return true;
}

bool JSSerializer::WritePlainObject(const JSHandle<JSTaggedValue> &objValue)
{
    JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(objValue);
    size_t oldSize = bufferSize_;
    if (!WriteType(SerializationUID::JS_PLAIN_OBJECT)) {
        return false;
    }
    // Get the number of elements stored in obj
    uint32_t elementsLength = obj->GetNumberOfElements();
    if (!WriteInt(static_cast<int32_t>(elementsLength))) {
        bufferSize_ = oldSize;
        return false;
    }
    std::vector<JSTaggedValue> keyVector;
    JSObject::GetALLElementKeysIntoVector(thread_, obj, keyVector);
    // Write elements' description attributes and value
    if (keyVector.size() != elementsLength) {
        bufferSize_ = oldSize;
        return false;
    }
    for (uint32_t i = 0; i < elementsLength; i++) {
        JSHandle<JSTaggedValue> key(thread_, keyVector[i]);
        if (!SerializeJSTaggedValue(key)) {
            bufferSize_ = oldSize;
            return false;
        }
        PropertyDescriptor desc(thread_);
        JSObject::OrdinaryGetOwnProperty(thread_, obj, key, desc);
        if (!WriteDesc(desc)) {
            bufferSize_ = oldSize;
            return false;
        }
        JSHandle<JSTaggedValue> value = desc.GetValue();
        if (!SerializeJSTaggedValue(value)) {
            bufferSize_ = oldSize;
            return false;
        }
    }
    // Get the number of k-v form properties stored in obj
    keyVector.clear();
    uint32_t propertiesLength = obj->GetNumberOfKeys();
    if (!WriteInt(static_cast<int32_t>(propertiesLength))) {
        bufferSize_ = oldSize;
        return false;
    }
    JSObject::GetAllKeys(thread_, obj, keyVector);
    if (keyVector.size() != propertiesLength) {
        bufferSize_ = oldSize;
        return false;
    }
    // Write keys' description attributes and related values
    for (uint32_t i = 0; i < propertiesLength; i++) {
        if (keyVector.empty()) {
            bufferSize_ = oldSize;
            return false;
        }
        JSHandle<JSTaggedValue> key(thread_, keyVector[i]);
        if (!SerializeJSTaggedValue(key)) {
            bufferSize_ = oldSize;
            return false;
        }
        PropertyDescriptor desc(thread_);
        JSObject::OrdinaryGetOwnProperty(thread_, obj, key, desc);
        if (!WriteDesc(desc)) {
            bufferSize_ = oldSize;
            return false;
        }
        JSHandle<JSTaggedValue> value = desc.GetValue();
        if (!SerializeJSTaggedValue(value)) {
            bufferSize_ = oldSize;
            return false;
        }
    }
    return true;
}

bool JSSerializer::WriteDesc(const PropertyDescriptor &desc)
{
    size_t oldSize = bufferSize_;
    bool isWritable = desc.IsWritable();
    if (!WriteBoolean(isWritable)) {
        bufferSize_ = oldSize;
        return false;
    }
    bool isEnumerable = desc.IsEnumerable();
    if (!WriteBoolean(isEnumerable)) {
        bufferSize_ = oldSize;
        return false;
    }
    bool isConfigurable = desc.IsConfigurable();
    if (!WriteBoolean(isConfigurable)) {
        bufferSize_ = oldSize;
        return false;
    }
    bool hasWritable = desc.HasWritable();
    if (!WriteBoolean(hasWritable)) {
        bufferSize_ = oldSize;
        return false;
    }
    bool hasEnumerable = desc.HasEnumerable();
    if (!WriteBoolean(hasEnumerable)) {
        bufferSize_ = oldSize;
        return false;
    }
    bool hasConfigurable = desc.HasConfigurable();
    if (!WriteBoolean(hasConfigurable)) {
        bufferSize_ = oldSize;
        return false;
    }
    return true;
}

SerializationUID JSDeserializer::ReadType()
{
    SerializationUID uid;
    if (position_ >= end_) {
        return SerializationUID::UNKNOWN;
    }
    uid = static_cast<SerializationUID>(*position_);
    if (uid < SerializationUID::JS_NULL || uid > SerializationUID::NATIVE_FUNCTION_POINTER) {
        return SerializationUID::UNKNOWN;
    }
    position_++;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return uid;
}

bool JSDeserializer::ReadInt(int32_t *value)
{
    size_t len = sizeof(int32_t);
    if (len > static_cast<size_t>(end_ - position_)) {
        return false;
    }
    if (memcpy_s(value, len, position_, len) != EOK) {
        UNREACHABLE();
    }
    position_ += len;
    return true;
}

bool JSDeserializer::ReadObjectId(uint64_t *objectId)
{
    size_t len = sizeof(uint64_t);
    if (len > static_cast<size_t>(end_ - position_)) {
        return false;
    }
    if (memcpy_s(objectId, len, position_, len) != EOK) {
        UNREACHABLE();
    }
    position_ += len;
    return true;
}

bool JSDeserializer::ReadDouble(double *value)
{
    size_t len = sizeof(double);
    if (len > static_cast<size_t>(end_ - position_)) {
        return false;
    }
    if (memcpy_s(value, len, position_, len) != EOK) {
        UNREACHABLE();
    }
    position_ += len;
    return true;
}

JSDeserializer::~JSDeserializer()
{
    free(begin_);
    begin_ = nullptr;
}

JSHandle<JSTaggedValue> JSDeserializer::DeserializeJSTaggedValue()
{
    SerializationUID uid = ReadType();
    if (uid == SerializationUID::UNKNOWN) {
        return JSHandle<JSTaggedValue>();
    }
    switch (uid) {
        case SerializationUID::JS_NULL:
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Null());
        case SerializationUID::JS_UNDEFINED:
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        case SerializationUID::JS_TRUE:
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::True());
        case SerializationUID::JS_FALSE:
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::False());
        case SerializationUID::HOLE:
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Hole());
        case SerializationUID::INT32: {
            int32_t value;
            if (!ReadInt(&value)) {
                return JSHandle<JSTaggedValue>();
            }
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue(value));
        }
        case SerializationUID::DOUBLE: {
            double value;
            if (!ReadDouble(&value)) {
                return JSHandle<JSTaggedValue>();
            }
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue(value));
        }
        case SerializationUID::JS_ERROR:
        case SerializationUID::EVAL_ERROR:
        case SerializationUID::RANGE_ERROR:
        case SerializationUID::REFERENCE_ERROR:
        case SerializationUID::TYPE_ERROR:
        case SerializationUID::URI_ERROR:
        case SerializationUID::SYNTAX_ERROR:
            return ReadJSError(uid);
        case SerializationUID::JS_DATE:
            return ReadJSDate();
        case SerializationUID::JS_PLAIN_OBJECT:
            return ReadPlainObject();
        case SerializationUID::JS_ARRAY:
            return ReadJSArray();
        case SerializationUID::ECMASTRING:
            return ReadEcmaString();
        case SerializationUID::JS_MAP:
            return ReadJSMap();
        case SerializationUID::JS_SET:
            return ReadJSSet();
        case SerializationUID::JS_REG_EXP:
            return ReadJSRegExp();
        case SerializationUID::JS_INT8_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_INT8_ARRAY);
        case SerializationUID::JS_UINT8_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_UINT8_ARRAY);
        case SerializationUID::JS_UINT8_CLAMPED_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_UINT8_CLAMPED_ARRAY);
        case SerializationUID::JS_INT16_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_INT16_ARRAY);
        case SerializationUID::JS_UINT16_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_UINT16_ARRAY);
        case SerializationUID::JS_INT32_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_INT32_ARRAY);
        case SerializationUID::JS_UINT32_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_UINT32_ARRAY);
        case SerializationUID::JS_FLOAT32_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_FLOAT32_ARRAY);
        case SerializationUID::JS_FLOAT64_ARRAY:
            return ReadJSTypedArray(SerializationUID::JS_FLOAT64_ARRAY);
        case SerializationUID::NATIVE_FUNCTION_POINTER:
            return ReadNativeFunctionPointer();
        case SerializationUID::JS_ARRAY_BUFFER:
            return ReadJSArrayBuffer();
        case SerializationUID::TAGGED_OBJECT_REFERNCE:
            return ReadReference();
        default:
            return JSHandle<JSTaggedValue>();
    }
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSError(SerializationUID uid)
{
    base::ErrorType errorType;
    switch (uid) {
        case SerializationUID::JS_ERROR:
            errorType = base::ErrorType::ERROR;
            break;
        case SerializationUID::EVAL_ERROR:
            errorType = base::ErrorType::EVAL_ERROR;
            break;
        case SerializationUID::RANGE_ERROR:
            errorType = base::ErrorType::RANGE_ERROR;
            break;
        case SerializationUID::REFERENCE_ERROR:
            errorType = base::ErrorType::REFERENCE_ERROR;
            break;
        case SerializationUID::TYPE_ERROR:
            errorType = base::ErrorType::TYPE_ERROR;
            break;
        case SerializationUID::URI_ERROR:
            errorType = base::ErrorType::URI_ERROR;
            break;
        case SerializationUID::SYNTAX_ERROR:
            errorType = base::ErrorType::URI_ERROR;
            break;
        default:
            UNREACHABLE();
    }
    JSHandle<JSTaggedValue> msg = DeserializeJSTaggedValue();
    JSHandle<EcmaString> handleMsg(msg);
    JSHandle<JSTaggedValue> errorTag = JSHandle<JSTaggedValue>::Cast(factory_->NewJSError(errorType, handleMsg));
    referenceMap_.insert(std::pair(objectId_++, errorTag));
    return errorTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSDate()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = env->GetDateFunction();
    JSHandle<JSDate> date =
        JSHandle<JSDate>::Cast(factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    JSHandle<JSTaggedValue> dateTag = JSHandle<JSTaggedValue>::Cast(date);
    referenceMap_.insert(std::pair(objectId_++, dateTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(dateTag)) {
        return JSHandle<JSTaggedValue>();
    }
    double timeValue;
    if (!JudgeType(SerializationUID::DOUBLE) || !ReadDouble(&timeValue)) {
        return JSHandle<JSTaggedValue>();
    }
    date->SetTimeValue(thread_, JSTaggedValue(timeValue));
    double localOffset;
    if (!JudgeType(SerializationUID::DOUBLE) || !ReadDouble(&localOffset)) {
        return JSHandle<JSTaggedValue>();
    }
    date->SetLocalOffset(thread_, JSTaggedValue(localOffset));
    return dateTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSArray()
{
    JSHandle<JSArray> jsArray = thread_->GetEcmaVM()->GetFactory()->NewJSArray();
    JSHandle<JSTaggedValue> arrayTag = JSHandle<JSTaggedValue>::Cast(jsArray);
    referenceMap_.insert(std::pair(objectId_++, arrayTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(arrayTag)) {
        return JSHandle<JSTaggedValue>();
    }
    int32_t arrLength;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&arrLength)) {
        return JSHandle<JSTaggedValue>();
    }
    jsArray->SetLength(thread_, JSTaggedValue(arrLength));
    return arrayTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadEcmaString()
{
    int32_t stringLength;
    bool isUtf8;
    if (!ReadBoolean(&isUtf8)) {
        return JSHandle<JSTaggedValue>();
    }
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&stringLength)) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> stringTag;
    if (isUtf8) {
        if (stringLength == 0) {
            JSHandle<JSTaggedValue> emptyString = JSHandle<JSTaggedValue>::Cast(factory_->GetEmptyString());
            referenceMap_.insert(std::pair(objectId_++, emptyString));
            return emptyString;
        }

        uint8_t *string = reinterpret_cast<uint8_t*>(GetBuffer(stringLength + 1));
        if (string == nullptr) {
            return JSHandle<JSTaggedValue>();
        }

        JSHandle<EcmaString> ecmaString = factory_->NewFromUtf8(string, stringLength);
        stringTag = JSHandle<JSTaggedValue>(ecmaString);
        referenceMap_.insert(std::pair(objectId_++, stringTag));
    } else {
        uint16_t *string = reinterpret_cast<uint16_t*>(GetBuffer(stringLength * sizeof(uint16_t)));
        if (string == nullptr) {
            return JSHandle<JSTaggedValue>();
        }
        JSHandle<EcmaString> ecmaString = factory_->NewFromUtf16(string, stringLength);
        stringTag = JSHandle<JSTaggedValue>(ecmaString);
        referenceMap_.insert(std::pair(objectId_++, stringTag));
    }
    return stringTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadPlainObject()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSObject> jsObject =
        thread_->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSHandle<JSTaggedValue> objTag = JSHandle<JSTaggedValue>::Cast(jsObject);
    referenceMap_.insert(std::pair(objectId_++, objTag));
    if (!DefinePropertiesAndElements(objTag)) {
        return JSHandle<JSTaggedValue>();
    }
    return objTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSMap()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> mapFunction = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(mapFunction), mapFunction));
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    referenceMap_.insert(std::pair(objectId_++, mapTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(mapTag)) {
        return JSHandle<JSTaggedValue>();
    }
    int32_t size;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&size)) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread_);
    jsMap->SetLinkedMap(thread_, linkedMap);
    for (int32_t i = 0; i < size; i++) {
        JSHandle<JSTaggedValue> key = DeserializeJSTaggedValue();
        if (key.IsEmpty()) {
            return JSHandle<JSTaggedValue>();
        }
        JSHandle<JSTaggedValue> value = DeserializeJSTaggedValue();
        if (value.IsEmpty()) {
            return JSHandle<JSTaggedValue>();
        }
        JSMap::Set(thread_, jsMap, key, value);
    }
    return mapTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSSet()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> setFunction = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> jsSet =
        JSHandle<JSSet>::Cast(factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(setFunction), setFunction));
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(jsSet);
    referenceMap_.insert(std::pair(objectId_++, setTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(setTag)) {
        return JSHandle<JSTaggedValue>();
    }
    int32_t size;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&size)) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<LinkedHashSet> linkedSet = LinkedHashSet::Create(thread_);
    jsSet->SetLinkedSet(thread_, linkedSet);
    for (int32_t i = 0; i < size; i++) {
        JSHandle<JSTaggedValue> key = DeserializeJSTaggedValue();
        if (key.IsEmpty()) {
            return JSHandle<JSTaggedValue>();
        }
        JSSet::Add(thread_, jsSet, key);
    }
    return setTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSRegExp()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> regexpFunction = env->GetRegExpFunction();
    JSHandle<JSObject> obj = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(regexpFunction), regexpFunction);
    JSHandle<JSRegExp> regExp = JSHandle<JSRegExp>::Cast(obj);
    JSHandle<JSTaggedValue> regexpTag = JSHandle<JSTaggedValue>::Cast(regExp);
    referenceMap_.insert(std::pair(objectId_++, regexpTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(regexpTag)) {
        return JSHandle<JSTaggedValue>();
    }
    int32_t bufferSize;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&bufferSize)) {
        return JSHandle<JSTaggedValue>();
    }
    void *buffer = GetBuffer(bufferSize);
    if (buffer == nullptr) {
        return JSHandle<JSTaggedValue>();
    }
    factory_->NewJSRegExpByteCodeData(regExp, buffer, bufferSize);
    JSHandle<JSTaggedValue> originalSource = DeserializeJSTaggedValue();
    regExp->SetOriginalSource(thread_, originalSource);
    JSHandle<JSTaggedValue> originalFlags = DeserializeJSTaggedValue();
    regExp->SetOriginalFlags(thread_, originalFlags);
    return regexpTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSTypedArray(SerializationUID uid)
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target;
    JSHandle<JSObject> obj;
    JSHandle<JSTaggedValue> objTag;
    switch (uid) {
        case SerializationUID::JS_INT8_ARRAY: {
            target = env->GetInt8ArrayFunction();
            break;
        }
        case SerializationUID::JS_UINT8_ARRAY: {
            target = env->GetUint8ArrayFunction();
            break;
        }
        case SerializationUID::JS_UINT8_CLAMPED_ARRAY: {
            target = env->GetUint8ClampedArrayFunction();
            break;
        }
        case SerializationUID::JS_INT16_ARRAY: {
            target = env->GetInt16ArrayFunction();
            break;
        }
        case SerializationUID::JS_UINT16_ARRAY: {
            target = env->GetUint16ArrayFunction();
            break;
        }
        case SerializationUID::JS_INT32_ARRAY: {
            target = env->GetInt32ArrayFunction();
            break;
        }
        case SerializationUID::JS_UINT32_ARRAY: {
            target = env->GetUint32ArrayFunction();
            break;
        }
        case SerializationUID::JS_FLOAT32_ARRAY: {
            target = env->GetFloat32ArrayFunction();
            break;
        }
        case SerializationUID::JS_FLOAT64_ARRAY: {
            target = env->GetFloat64ArrayFunction();
            break;
        }
        default:
            UNREACHABLE();
    }
    JSHandle<JSTypedArray> typedArray =
        JSHandle<JSTypedArray>::Cast(factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    obj = JSHandle<JSObject>::Cast(typedArray);
    objTag = JSHandle<JSTaggedValue>::Cast(obj);
    referenceMap_.insert(std::pair(objectId_++, objTag));
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(objTag)) {
        return JSHandle<JSTaggedValue>();
    }

    JSHandle<JSTaggedValue> viewedArrayBuffer = DeserializeJSTaggedValue();
    if (viewedArrayBuffer.IsEmpty()) {
        return JSHandle<JSTaggedValue>();
    }
    typedArray->SetViewedArrayBuffer(thread_, viewedArrayBuffer);

    JSHandle<JSTaggedValue> typedArrayName = DeserializeJSTaggedValue();
    if (typedArrayName.IsEmpty()) {
        return JSHandle<JSTaggedValue>();
    }
    typedArray->SetTypedArrayName(thread_, typedArrayName);

    JSTaggedValue byteLength;
    if (!ReadJSTaggedValue(&byteLength) || !byteLength.IsNumber()) {
        return JSHandle<JSTaggedValue>();
    }
    typedArray->SetByteLength(thread_, byteLength);

    JSTaggedValue byteOffset;
    if (!ReadJSTaggedValue(&byteOffset) || !byteOffset.IsNumber()) {
        return JSHandle<JSTaggedValue>();
    }
    typedArray->SetByteOffset(thread_, byteOffset);

    JSTaggedValue arrayLength;
    if (!ReadJSTaggedValue(&arrayLength) || !byteOffset.IsNumber()) {
        return JSHandle<JSTaggedValue>();
    }
    typedArray->SetArrayLength(thread_, arrayLength);
    return objTag;
}

JSHandle<JSTaggedValue> JSDeserializer::ReadNativeFunctionPointer()
{
    JSTaggedValue pointer;
    if (!ReadJSTaggedValue(&pointer)) {
        return JSHandle<JSTaggedValue>();
    }
    return JSHandle<JSTaggedValue>(thread_, pointer);
}

JSHandle<JSTaggedValue> JSDeserializer::ReadJSArrayBuffer()
{
    // read access length
    int32_t arrayLength;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&arrayLength)) {
        return JSHandle<JSTaggedValue>();
    }
    // read access shared
    bool shared = false;
    if (!ReadBoolean(&shared)) {
        return JSHandle<JSTaggedValue>();
    }
    // create jsarraybuffer
    JSHandle<JSTaggedValue> arrayBufferTag;
    if (shared) {
        uint64_t *bufferAddr = reinterpret_cast<uint64_t*>(GetBuffer(sizeof(uint64_t)));
        void* bufferData = ToVoidPtr(*bufferAddr);
        JSHandle<JSArrayBuffer> arrayBuffer = factory_->NewJSArrayBuffer(bufferData, arrayLength, nullptr, nullptr);
        arrayBufferTag = JSHandle<JSTaggedValue>::Cast(arrayBuffer);
        referenceMap_.insert(std::pair(objectId_++, arrayBufferTag));
    } else {
        void *fromBuffer = GetBuffer(arrayLength);
        if (fromBuffer == nullptr) {
            return arrayBufferTag;
        }
        JSHandle<JSArrayBuffer> arrayBuffer = factory_->NewJSArrayBuffer(arrayLength);
        arrayBufferTag = JSHandle<JSTaggedValue>::Cast(arrayBuffer);
        referenceMap_.insert(std::pair(objectId_++, arrayBufferTag));
        JSHandle<JSNativePointer> np(thread_, arrayBuffer->GetArrayBufferData());
        void *toBuffer = np->GetExternalPointer();
        if (memcpy_s(toBuffer, arrayLength, fromBuffer, arrayLength) != EOK) {
            UNREACHABLE();
        }
    }
    // read jsarraybuffer properties
    if (!JudgeType(SerializationUID::JS_PLAIN_OBJECT) || !DefinePropertiesAndElements(arrayBufferTag)) {
        return JSHandle<JSTaggedValue>();
    }

    return arrayBufferTag;
}

bool JSDeserializer::ReadJSTaggedValue(JSTaggedValue *value)
{
    size_t len = sizeof(JSTaggedValue);
    if (len > static_cast<size_t>(end_ - position_)) {
        return false;
    }
    if (memcpy_s(value, len, position_, len) != EOK) {
        UNREACHABLE();
    }
    position_ += len;
    return true;
}

void *JSDeserializer::GetBuffer(uint32_t bufferSize)
{
    const uint8_t *buffer = nullptr;
    if (bufferSize > static_cast<size_t>(end_ - position_)) {
        return nullptr;
    }
    buffer = position_;
    position_ += bufferSize;
    uint8_t *retBuffer = const_cast<uint8_t *>(buffer);
    return static_cast<void *>(retBuffer);
}

JSHandle<JSTaggedValue> JSDeserializer::ReadReference()
{
    uint64_t objId;
    if (!ReadObjectId(&objId)) {
        return JSHandle<JSTaggedValue>();
    }
    auto objIter = referenceMap_.find(objId);
    if (objIter == referenceMap_.end()) {
        return JSHandle<JSTaggedValue>();
    }
    return objIter->second;
}

bool JSDeserializer::JudgeType(SerializationUID targetUid)
{
    if (ReadType() != targetUid) {
        return false;
    }
    return true;
}

bool JSDeserializer::DefinePropertiesAndElements(const JSHandle<JSTaggedValue> &obj)
{
    int32_t elementLength;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&elementLength)) {
        return false;
    }
    for (int32_t i = 0; i < elementLength; i++) {
        JSHandle<JSTaggedValue> key = DeserializeJSTaggedValue();
        if (key.IsEmpty()) {
            return false;
        }
        PropertyDescriptor desc(thread_);
        if (!ReadDesc(&desc)) {
            return false;
        }
        JSHandle<JSTaggedValue> value = DeserializeJSTaggedValue();
        if (value.IsEmpty()) {
            return false;
        }
        desc.SetValue(value);
        if (!JSTaggedValue::DefineOwnProperty(thread_, obj, key, desc)) {
            return false;
        }
    }

    int32_t propertyLength;
    if (!JudgeType(SerializationUID::INT32) || !ReadInt(&propertyLength)) {
        return false;
    }
    for (int32_t i = 0; i < propertyLength; i++) {
        JSHandle<JSTaggedValue> key = DeserializeJSTaggedValue();
        if (key.IsEmpty()) {
            return false;
        }
        PropertyDescriptor desc(thread_);
        if (!ReadDesc(&desc)) {
            return false;
        }
        JSHandle<JSTaggedValue> value = DeserializeJSTaggedValue();
        if (value.IsEmpty()) {
            return false;
        }
        desc.SetValue(value);
        if (!JSTaggedValue::DefineOwnProperty(thread_, obj, key, desc)) {
            return false;
        }
    }
    return true;
}

bool JSDeserializer::ReadDesc(PropertyDescriptor *desc)
{
    bool isWritable = false;
    if (!ReadBoolean(&isWritable)) {
        return false;
    }
    bool isEnumerable = false;
    if (!ReadBoolean(&isEnumerable)) {
        return false;
    }
    bool isConfigurable = false;
    if (!ReadBoolean(&isConfigurable)) {
        return false;
    }
    bool hasWritable = false;
    if (!ReadBoolean(&hasWritable)) {
        return false;
    }
    bool hasEnumerable = false;
    if (!ReadBoolean(&hasEnumerable)) {
        return false;
    }
    bool hasConfigurable = false;
    if (!ReadBoolean(&hasConfigurable)) {
        return false;
    }
    if (hasWritable) {
        desc->SetWritable(isWritable);
    }
    if (hasEnumerable) {
        desc->SetEnumerable(isEnumerable);
    }
    if (hasConfigurable) {
        desc->SetConfigurable(isConfigurable);
    }
    return true;
}

bool JSDeserializer::ReadBoolean(bool *value)
{
    SerializationUID uid = ReadType();
    if (uid == SerializationUID::C_TRUE) {
        *value = true;
        return true;
    }
    if (uid == SerializationUID::C_FALSE) {
        *value = false;
        return true;
    }
    return false;
}

bool Serializer::WriteValue(
    JSThread *thread, const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &transfer)
{
    if (data_ != nullptr) {
        return false;
    }
    data_.reset(new SerializationData);
    if (!PrepareTransfer(thread, transfer)) {
        return false;
    }
    if (!valueSerializer_.SerializeJSTaggedValue(value)) {
        return false;
    }
    if (!FinalizeTransfer(thread, transfer)) {
        return false;
    }
    std::pair<uint8_t*, size_t> pair = valueSerializer_.ReleaseBuffer();
    data_->value_.reset(pair.first);
    data_->dataSize_ = pair.second;
    return true;
}

std::unique_ptr<SerializationData> Serializer::Release()
{
    return std::move(data_);
}

bool Serializer::PrepareTransfer(JSThread *thread, const JSHandle<JSTaggedValue> &transfer)
{
    if (transfer->IsUndefined()) {
        return true;
    }
    if (!transfer->IsJSArray()) {
        return false;
    }
    int len = base::ArrayHelper::GetArrayLength(thread, transfer);
    int k = 0;
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, transfer, k);
        if (exists) {
            JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, transfer, k);
            if (!element->IsArrayBuffer()) {
                return false;
            }
            arrayBufferIdxs_.emplace_back(k);
        }
        k++;
    }
    return true;
}

bool Serializer::FinalizeTransfer(JSThread *thread, const JSHandle<JSTaggedValue> &transfer)
{
    for (int idx : arrayBufferIdxs_) {
        JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, transfer, idx);
        JSArrayBuffer::Cast(element->GetHeapObject())->Detach(thread);
    }
    return true;
}

JSHandle<JSTaggedValue> Deserializer::ReadValue()
{
    return valueDeserializer_.DeserializeJSTaggedValue();
}
}  // namespace panda::ecmascript
