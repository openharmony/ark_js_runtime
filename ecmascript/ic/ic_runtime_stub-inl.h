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

#ifndef ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H_
#define ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H_

#include "ecmascript/base/config.h"
#include "ic_runtime_stub.h"
#include "ic_handler.h"
#include "ic_runtime.h"
#include "profile_type_info.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/ic/proto_change_details.h"

#include "ecmascript/vmstat/runtime_stat.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
JSTaggedValue ICRuntimeStub::LoadGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                JSTaggedValue globalValue, JSTaggedValue key, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, LoadGlobalICByName);
    JSTaggedValue handler = profileTypeInfo->Get(slotId);
    if (handler.IsHeapObject()) {
        auto result = LoadGlobal(handler);
        if (!result.IsHole()) {
            return result;
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, globalValue);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId,
                            ICKind::NamedGlobalLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

JSTaggedValue ICRuntimeStub::StoreGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                 JSTaggedValue globalValue, JSTaggedValue key,
                                                 JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreGlobalICByName);
    JSTaggedValue handler = profileTypeInfo->Get(slotId);
    if (handler.IsHeapObject()) {
        auto result = StoreGlobal(thread, value, handler);
        if (!result.IsHole()) {
            return result;
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, globalValue);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId,
                             ICKind::NamedGlobalStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle);
}

JSTaggedValue ICRuntimeStub::CheckPolyHClass(JSTaggedValue cachedValue, JSHClass* hclass)
{
    if (!cachedValue.IsWeak()) {
        ASSERT(cachedValue.IsTaggedArray());
        TaggedArray *array = TaggedArray::Cast(cachedValue.GetHeapObject());
        uint32_t length = array->GetLength();
        for (uint32_t i = 0; i < length; i += 2) {  // 2 means one ic, two slot
            auto result = array->Get(i);
            if (result != JSTaggedValue::Undefined() && result.GetWeakReferent() == hclass) {
                return array->Get(i + 1);
            }
        }
    }
    return JSTaggedValue::Hole();
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryLoadICByName(JSThread *thread, JSTaggedValue receiver,
                                                        JSTaggedValue firstValue, JSTaggedValue secondValue)
{
    INTERPRETER_TRACE(thread, TryLoadICByName);
    if (LIKELY(receiver.IsHeapObject())) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (LIKELY(firstValue.GetWeakReferentUnChecked() == hclass)) {
            return LoadICWithHandler(thread, receiver, receiver, secondValue);
        }
        JSTaggedValue cachedHandler = CheckPolyHClass(firstValue, hclass);
        if (!cachedHandler.IsHole()) {
            return LoadICWithHandler(thread, receiver, receiver, cachedHandler);
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::LoadICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                       JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, LoadICByName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<ProfileTypeInfo>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, profileInfoHandle, slotId, ICKind::NamedLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryLoadICByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                         JSTaggedValue firstValue, JSTaggedValue secondValue)
{
    INTERPRETER_TRACE(thread, TryLoadICByValue);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            ASSERT(HandlerBase::IsElement(secondValue.GetInt()));
            return LoadElement(JSObject::Cast(receiver.GetHeapObject()), key);
        }
        // Check key
        if (firstValue == key) {
            JSTaggedValue cachedHandler = CheckPolyHClass(secondValue, hclass);
            if (!cachedHandler.IsHole()) {
                return LoadICWithHandler(thread, receiver, receiver, cachedHandler);
            }
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::LoadICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                        JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, LoadICByValue);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::LoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryStoreICByValue(JSThread *thread, JSTaggedValue receiver,
                                                          JSTaggedValue key, JSTaggedValue firstValue,
                                                          JSTaggedValue secondValue, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryStoreICByValue);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            auto handlerInfo = static_cast<uint32_t>(secondValue.GetInt());
            return StoreElement(thread, JSObject::Cast(receiver.GetHeapObject()), key, value, handlerInfo);
        }
        // Check key
        if (firstValue == key) {
            JSTaggedValue cachedHandler = CheckPolyHClass(secondValue, hclass);
            if (!cachedHandler.IsHole()) {
                return StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
            }
        }
    }

    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::StoreICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                         JSTaggedValue receiver, JSTaggedValue key,
                                                         JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreICByValue);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::StoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::TryStoreICByName(JSThread *thread, JSTaggedValue receiver,
                                                         JSTaggedValue firstValue, JSTaggedValue secondValue,
                                                         JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryStoreICByName);
    if (receiver.IsHeapObject()) {
        auto hclass = receiver.GetTaggedObject()->GetClass();
        if (firstValue.GetWeakReferentUnChecked() == hclass) {
            return StoreICWithHandler(thread, receiver, receiver, value, secondValue);
        }
        JSTaggedValue cachedHandler = CheckPolyHClass(firstValue, hclass);
        if (!cachedHandler.IsHole()) {
            return StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
        }
    }
    return JSTaggedValue::Hole();
}

ARK_NOINLINE JSTaggedValue ICRuntimeStub::StoreICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                        JSTaggedValue receiver, JSTaggedValue key,
                                                        JSTaggedValue value, uint32_t slotId)
{
    INTERPRETER_TRACE(thread, StoreICByName);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::NamedStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::StoreICWithHandler(JSThread *thread, JSTaggedValue receiver,
                                                           JSTaggedValue holder,
                                                           JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreICWithHandler);
    if (handler.IsInt()) {
        auto handlerInfo = static_cast<uint32_t>(handler.GetInt());
        if (HandlerBase::IsField(handlerInfo)) {
            StoreField(thread, JSObject::Cast(receiver.GetHeapObject()), value, handlerInfo);
            return JSTaggedValue::Undefined();
        }
        ASSERT(HandlerBase::IsAccessor(handlerInfo) || HandlerBase::IsInternalAccessor(handlerInfo));
        auto accessor = LoadFromField(JSObject::Cast(holder.GetHeapObject()), handlerInfo);
        return FastRuntimeStub::CallSetter(thread, JSTaggedValue(receiver), value, accessor);
    }
    if (handler.IsTransitionHandler()) {
        StoreWithTransition(thread, JSObject::Cast(receiver.GetHeapObject()), value, handler);
        return JSTaggedValue::Undefined();
    }
    if (handler.IsPrototypeHandler()) {
        return StorePrototype(thread, receiver, value, handler);
    }
    if (handler.IsPropertyBox()) {
        return StoreGlobal(thread, value, handler);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ICRuntimeStub::StorePrototype(JSThread *thread, JSTaggedValue receiver,
                                            JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StorePrototype);
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell();
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetHeapObject());
    if (cell->GetHasChanged()) {
        return JSTaggedValue::Hole();
    }
    auto holder = prototypeHandler->GetHolder();
    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    return StoreICWithHandler(thread, receiver, holder, value, handlerInfo);
}

void ICRuntimeStub::StoreWithTransition(JSThread *thread, JSObject *receiver, JSTaggedValue value,
                                        JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreWithTransition);
    TransitionHandler *transitionHandler = TransitionHandler::Cast(handler.GetTaggedObject());
    JSHClass *newHClass = JSHClass::Cast(transitionHandler->GetTransitionHClass().GetTaggedObject());
    receiver->SetClass(newHClass);
    uint32_t handlerInfo = transitionHandler->GetHandlerInfo().GetInt();
    ASSERT(HandlerBase::IsField(handlerInfo));

    if (!HandlerBase::IsInlinedProps(handlerInfo)) {
        TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetHeapObject());
        int capacity = array->GetLength();
        int index = HandlerBase::GetOffset(handlerInfo);
        if (index >= capacity) {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> properties;
            JSHandle<JSObject> objHandle(thread, receiver);
            JSHandle<JSTaggedValue> valueHandle(thread, value);
            if (capacity == 0) {
                capacity = JSObject::MIN_PROPERTIES_LENGTH;
                properties = factory->NewTaggedArray(capacity);
            } else {
                auto arrayHandle = JSHandle<TaggedArray>(thread, array);
                properties = factory->CopyArray(arrayHandle, capacity,
                                                JSObject::ComputePropertyCapacity(capacity));
            }
            properties->Set(thread, index, valueHandle);
            objHandle->SetProperties(thread, properties);
            return;
        }
        array->Set(thread, index, value);
        return;
    }
    StoreField(thread, receiver, value, handlerInfo);
}

ARK_INLINE void ICRuntimeStub::StoreField(JSThread *thread, JSObject *receiver, JSTaggedValue value, uint32_t handler)
{
    INTERPRETER_TRACE(thread, StoreField);
    int index = HandlerBase::GetOffset(handler);
    if (HandlerBase::IsInlinedProps(handler)) {
        SET_VALUE_WITH_BARRIER(thread, receiver, index * JSTaggedValue::TaggedTypeSize(), value);
        return;
    }
    TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetHeapObject());
    ASSERT(index < static_cast<int>(array->GetLength()));
    array->Set(thread, index, value);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadFromField(JSObject *receiver, uint32_t handlerInfo)
{
    int index = HandlerBase::GetOffset(handlerInfo);
    if (HandlerBase::IsInlinedProps(handlerInfo)) {
        return JSTaggedValue(GET_VALUE(receiver, index * JSTaggedValue::TaggedTypeSize()));
    }
    return TaggedArray::Cast(receiver->GetProperties().GetHeapObject())->Get(index);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadGlobal(JSTaggedValue handler)
{
    ASSERT(handler.IsPropertyBox());
    PropertyBox *cell = PropertyBox::Cast(handler.GetHeapObject());
    if (cell->IsInvalid()) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue ret = cell->GetValue();
    ASSERT(!ret.IsAccessorData());
    return ret;
}

ARK_INLINE JSTaggedValue ICRuntimeStub::StoreGlobal(JSThread *thread, JSTaggedValue value, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, StoreGlobal);
    ASSERT(handler.IsPropertyBox());
    PropertyBox *cell = PropertyBox::Cast(handler.GetHeapObject());
    if (cell->IsInvalid()) {
        return JSTaggedValue::Hole();
    }
    ASSERT(!cell->GetValue().IsAccessorData());
    cell->SetValue(thread, value);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ICRuntimeStub::LoadPrototype(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, LoadPrototype);
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    auto cellValue = prototypeHandler->GetProtoCell();
    ASSERT(cellValue.IsProtoChangeMarker());
    ProtoChangeMarker *cell = ProtoChangeMarker::Cast(cellValue.GetHeapObject());
    if (cell->GetHasChanged()) {
        return JSTaggedValue::Hole();
    }
    auto holder = prototypeHandler->GetHolder();
    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    return LoadICWithHandler(thread, receiver, holder, handlerInfo);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadICWithHandler(JSThread *thread, JSTaggedValue receiver,
                                                          JSTaggedValue holder, JSTaggedValue handler)
{
    INTERPRETER_TRACE(thread, LoadICWithHandler);
    if (LIKELY(handler.IsInt())) {
        auto handlerInfo = static_cast<uint32_t>(handler.GetInt());
        if (LIKELY(HandlerBase::IsField(handlerInfo))) {
            return LoadFromField(JSObject::Cast(holder.GetHeapObject()), handlerInfo);
        }
        if (HandlerBase::IsNonExist(handlerInfo)) {
            return JSTaggedValue::Undefined();
        }
        ASSERT(HandlerBase::IsAccessor(handlerInfo) || HandlerBase::IsInternalAccessor(handlerInfo));
        auto accessor = LoadFromField(JSObject::Cast(holder.GetHeapObject()), handlerInfo);
        return FastRuntimeStub::CallGetter(thread, receiver, holder, accessor);
    }

    if (handler.IsPrototypeHandler()) {
        return LoadPrototype(thread, receiver, handler);
    }

    return LoadGlobal(handler);
}

ARK_INLINE JSTaggedValue ICRuntimeStub::LoadElement(JSObject *receiver, JSTaggedValue key)
{
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    uint32_t elementIndex = index;
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetHeapObject());
    if (elements->GetLength() <= elementIndex) {
        return JSTaggedValue::Hole();
    }

    JSTaggedValue value = elements->Get(elementIndex);
    // TaggedArray elements
    return value;
}

JSTaggedValue ICRuntimeStub::StoreElement(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                          JSTaggedValue value, uint32_t handlerInfo)
{
    INTERPRETER_TRACE(thread, StoreElement);
    ASSERT(HandlerBase::IsElement(handlerInfo));
    auto index = TryToElementsIndex(key);
    if (index < 0) {
        return JSTaggedValue::Hole();
    }
    uint32_t elementIndex = index;
    if (HandlerBase::IsJSArray(handlerInfo)) {
        JSArray *arr = JSArray::Cast(receiver);
        uint32_t oldLength = arr->GetArrayLength();
        if (elementIndex >= oldLength) {
            arr->SetArrayLength(thread, elementIndex + 1);
        }
    }
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetHeapObject());
    uint32_t capacity = elements->GetLength();
    if (elementIndex >= capacity) {
        if (JSObject::ShouldTransToDict(capacity, elementIndex)) {
            return JSTaggedValue::Hole();
        }
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        JSHandle<JSObject> receiverHandle(thread, receiver);
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        elements = *JSObject::GrowElementsCapacity(thread, receiverHandle,
                                                   JSObject::ComputeElementCapacity(elementIndex + 1));
        receiverHandle->SetElements(thread, JSTaggedValue(elements));
        elements->Set(thread, elementIndex, valueHandle);
        return JSTaggedValue::Undefined();
    }
    elements->Set(thread, elementIndex, value);
    return JSTaggedValue::Undefined();
}

ARK_INLINE int32_t ICRuntimeStub::TryToElementsIndex(JSTaggedValue key)
{
    if (LIKELY(key.IsInt())) {
        return key.GetInt();
    }

    if (key.IsString()) {
        uint32_t index = 0;
        if (JSTaggedValue::StringToElementIndex(key, &index)) {
            return static_cast<int32_t>(index);
        }
    }

    if (key.IsDouble()) {
        double number = key.GetDouble();
        auto integer = static_cast<int32_t>(number);
        if (number == integer) {
            return integer;
        }
    }

    return -1;
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_RUNTIME_STUB_INL_H_
