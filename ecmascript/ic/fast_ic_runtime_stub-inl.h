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

#ifndef PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_INL_H

#include "fast_ic_runtime_stub.h"
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

namespace panda::ecmascript {
JSTaggedValue ICRuntimeStub::LoadGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                JSTaggedValue globalValue, JSTaggedValue key, uint32_t slotId)
{
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
        array_size_t length = array->GetLength();
        for (array_size_t i = 0; i < length; i += 2) {  // 2 means one ic, two slot
            if (array->Get(i).GetWeakReferent() == hclass) {
                return array->Get(i + 1);
            }
        }
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue ICRuntimeStub::LoadICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                          JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    if (receiver.IsHeapObject()) {
        JSTaggedValue cachedValue = profileTypeInfo->Get(slotId);
        if (cachedValue.IsHeapObject()) {
            auto hclass = receiver.GetTaggedObject()->GetClass();
            if (cachedValue.GetWeakReferentUnChecked() == hclass) {
                auto cachedHandler = profileTypeInfo->Get(slotId + 1);
                auto result = LoadICWithHandler(thread, receiver, receiver, cachedHandler);
                if (!result.IsHole()) {
                    return result;
                }
            }
            JSTaggedValue cachedHandler = CheckPolyHClass(cachedValue, hclass);
            if (!cachedHandler.IsHole()) {
                auto result = LoadICWithHandler(thread, receiver, receiver, cachedHandler);
                if (!result.IsHole()) {
                    return result;
                }
            }
        } else if (cachedValue.IsHole()) {
            JSTaggedValue result = FastRuntimeStub::GetPropertyByName(thread, receiver, key);
            if (!result.IsHole()) {
                return result;
            }
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::NamedLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

JSTaggedValue ICRuntimeStub::LoadICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                           JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId)
{
    if (receiver.IsHeapObject()) {
        JSTaggedValue cachedValue = profileTypeInfo->Get(slotId);
        if (cachedValue.IsHeapObject()) {
            auto hclass = receiver.GetTaggedObject()->GetClass();
            if (cachedValue.GetWeakReferentUnChecked() == hclass) {
                ASSERT(HandlerBase::IsElement(profileTypeInfo->Get(slotId + 1).GetInt()));
                auto result = LoadElement(JSObject::Cast(receiver.GetHeapObject()), key);
                if (!result.IsHole()) {
                    return result;
                }
            }
            // Check key
            if (cachedValue == key) {
                cachedValue = profileTypeInfo->Get(slotId + 1);
                JSTaggedValue cachedHandler = CheckPolyHClass(cachedValue, hclass);
                if (!cachedHandler.IsHole()) {
                    auto result = LoadICWithHandler(thread, receiver, receiver, cachedHandler);
                    if (!result.IsHole()) {
                        return result;
                    }
                }
            }
        } else if (cachedValue.IsHole()) {
            JSTaggedValue result = FastRuntimeStub::GetPropertyByValue(thread, receiver, key);
            if (!result.IsHole()) {
                return result;
            }
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::LoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle);
}

JSTaggedValue ICRuntimeStub::StoreICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                            JSTaggedValue key, JSTaggedValue value, uint32_t slotId)
{
    if (receiver.IsHeapObject()) {
        JSTaggedValue cachedValue = profileTypeInfo->Get(slotId);
        if (cachedValue.IsHeapObject()) {
            auto hclass = receiver.GetTaggedObject()->GetClass();
            if (cachedValue.GetWeakReferentUnChecked() == hclass) {
                auto handlerInfo = static_cast<uint32_t>(profileTypeInfo->Get(slotId + 1).GetInt());
                auto result = StoreElement(thread, JSObject::Cast(receiver.GetHeapObject()), key, value, handlerInfo);
                if (!result.IsHole()) {
                    return result;
                }
            }
            // Check key
            if (cachedValue == key) {
                cachedValue = profileTypeInfo->Get(slotId + 1);
                JSTaggedValue cachedHandler = CheckPolyHClass(cachedValue, hclass);
                if (!cachedHandler.IsHole()) {
                    auto result = StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
                    if (!result.IsHole()) {
                        return result;
                    }
                }
            }
        } else if (cachedValue.IsHole()) {
            JSTaggedValue result = FastRuntimeStub::SetPropertyByValue(thread, receiver, key, value);
            if (!result.IsHole()) {
                return result;
            }
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::StoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle);
}

JSTaggedValue ICRuntimeStub::StoreICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo, JSTaggedValue receiver,
                                           JSTaggedValue key, JSTaggedValue value, uint32_t slotId)
{
    if (receiver.IsHeapObject()) {
        JSTaggedValue cachedValue = profileTypeInfo->Get(slotId);
        if (cachedValue.IsHeapObject()) {
            auto hclass = receiver.GetTaggedObject()->GetClass();
            if (cachedValue.GetWeakReferentUnChecked() == hclass) {
                auto cachedHandler = profileTypeInfo->Get(slotId + 1);
                auto result = StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
                if (!result.IsHole()) {
                    return result;
                }
            }
            JSTaggedValue cachedHandler = CheckPolyHClass(cachedValue, hclass);
            if (!cachedHandler.IsHole()) {
                auto result = StoreICWithHandler(thread, receiver, receiver, value, cachedHandler);
                if (!result.IsHole()) {
                    return result;
                }
            }
        } else if (cachedValue.IsHole()) {
            JSTaggedValue result = FastRuntimeStub::SetPropertyByName(thread, receiver, key, value);
            if (!result.IsHole()) {
                return result;
            }
        }
    }

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, key);
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, receiver);
    auto valueHandle = JSHandle<JSTaggedValue>(thread, value);
    auto profileInfoHandle = JSHandle<JSTaggedValue>(thread, profileTypeInfo);
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileInfoHandle), slotId, ICKind::NamedStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle);
}

JSTaggedValue ICRuntimeStub::StoreICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                JSTaggedValue value, JSTaggedValue handler)
{
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
            if (capacity == 0) {
                capacity = JSObject::MIN_PROPERTIES_LENGTH;
                properties = factory->NewTaggedArray(capacity);
            } else {
                properties = factory->CopyArray(JSHandle<TaggedArray>(thread, array), capacity,
                                                JSObject::ComputePropertyCapacity(capacity));
            }
            properties->Set(thread, index, value);
            objHandle->SetProperties(thread, properties);
            return;
        }
        array->Set(thread, index, value);
        return;
    }
    StoreField(thread, receiver, value, handlerInfo);
}

void ICRuntimeStub::StoreField(JSThread *thread, JSObject *receiver, JSTaggedValue value, uint32_t handler)
{
    int index = HandlerBase::GetOffset(handler);
    if (HandlerBase::IsInlinedProps(handler)) {
        SET_VALUE_WITH_BARRIER(thread, receiver, index * JSTaggedValue::TaggedTypeSize(), value);
        return;
    }
    TaggedArray *array = TaggedArray::Cast(receiver->GetProperties().GetHeapObject());
    ASSERT(index < static_cast<int>(array->GetLength()));
    array->Set(thread, index, value);
}

JSTaggedValue ICRuntimeStub::LoadFromField(JSObject *receiver, uint32_t handlerInfo)
{
    int index = HandlerBase::GetOffset(handlerInfo);
    if (HandlerBase::IsInlinedProps(handlerInfo)) {
        return JSTaggedValue(GET_VALUE(receiver, index * JSTaggedValue::TaggedTypeSize()));
    }
    return TaggedArray::Cast(receiver->GetProperties().GetHeapObject())->Get(index);
}

JSTaggedValue ICRuntimeStub::LoadGlobal(JSTaggedValue handler)
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

JSTaggedValue ICRuntimeStub::StoreGlobal(JSThread *thread, JSTaggedValue value, JSTaggedValue handler)
{
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

JSTaggedValue ICRuntimeStub::LoadICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                               JSTaggedValue handler)
{
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

JSTaggedValue ICRuntimeStub::LoadElement(JSObject *receiver, JSTaggedValue key)
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
    // TaggedArray
    return value;
}

JSTaggedValue ICRuntimeStub::StoreElement(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                          JSTaggedValue value, uint32_t handlerInfo)
{
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
        elements = *JSObject::GrowElementsCapacity(thread, receiverHandle,
                                                   JSObject::ComputeElementCapacity(elementIndex + 1));
        receiver->SetElements(thread, JSTaggedValue(elements));
    }
    elements->Set(thread, elementIndex, value);
    receiver->GetJSHClass()->UpdateRepresentation(value);
    return JSTaggedValue::Undefined();
}

int32_t ICRuntimeStub::TryToElementsIndex(JSTaggedValue key)
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

#endif  // PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_INL_H
