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

#include "ic_accessor.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ic_accessor-inl.h"

namespace panda::ecmascript {
JSTaggedValue ICAccessor::LoadMiss(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                   uint16_t slotId)
{
    if (obj->IsTypedArray()) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key).GetValue().GetTaggedValue();
    }
    ObjectOperator op(thread, obj, key);
    // ic-switch
    if (!thread->GetEcmaVM()->ICEnable()) {
        return JSObject::GetProperty(thread, &op);
    }
    // do not cache element
    if (!op.IsFastMode() || op.IsElement()) {
        return JSObject::GetProperty(thread, &op);
    }
    JSTaggedValue handler;
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    if (!op.IsFound()) {
        handler = NonExistentHandler::LoadNonExistent(thread, dynclass);
    } else if (!op.IsOnPrototype()) {
        handler = LoadHandler::LoadProperty(thread, op);
    } else {
        handler = PrototypeHandler::LoadPrototype(thread, op, dynclass);
    }
    // add handler to ic slot
    FunctionCache::AddLoadHandler(thread, key, dynclass, JSHandle<JSTaggedValue>(thread, handler), slotId);
    return JSObject::GetProperty(thread, &op);
}

JSTaggedValue ICAccessor::LoadMissByName(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &key, uint16_t slotId)
{
    if (obj->IsTypedArray()) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key).GetValue().GetTaggedValue();
    }
    ObjectOperator op(thread, obj, key);
    // ic-switch
    if (!thread->GetEcmaVM()->ICEnable()) {
        return JSObject::GetProperty(thread, &op);
    }
    // do not cache element
    if (!op.IsFastMode() || op.IsElement()) {
        return JSObject::GetProperty(thread, &op);
    }
    JSTaggedValue handler;
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    if (!op.IsFound()) {
        handler = NonExistentHandler::LoadNonExistent(thread, dynclass);
    } else if (!op.IsOnPrototype()) {
        handler = LoadHandler::LoadProperty(thread, op);
    } else {
        handler = PrototypeHandler::LoadPrototype(thread, op, dynclass);
    }
    // add handler to ic slot
    FunctionCache::AddLoadHandler(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()), dynclass,
                                  JSHandle<JSTaggedValue>(thread, handler), slotId);
    return JSObject::GetProperty(thread, &op);
}

bool ICAccessor::StoreMiss(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value, uint16_t slotId)
{
    if (obj->IsTypedArray()) {
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key, value);
    }
    ObjectOperator op(thread, obj, key);
    bool success = JSObject::SetProperty(&op, value, false);
    if (!thread->GetEcmaVM()->ICEnable()) {
        return success;
    }

    if (!success) {
        return false;
    }
    // do not cache element and proxy
    if (!op.IsFastMode() || op.IsElement() || op.GetHolder()->IsJSProxy()) {
        return true;
    }
    // if !op.IsElement && !op.IsFound, target will be added in function AddPropertyInternal(), at the same time op will
    // be set found()
    ASSERT(op.IsFound());
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    JSTaggedValue handler;
    if (op.IsOnPrototype()) {
        // if SetProperty successfully, op.IsOnPrototype will be reset if op.IsAccessorDescriptor is false;
        ASSERT(op.IsAccessorDescriptor());
        handler = PrototypeHandler::StorePrototype(thread, op, dynclass);
    } else if (*dynclass == obj->GetJSHClass()) {
        handler = StoreHandler::StoreProperty(thread, op);
    } else {
        handler = TransitionHandler::StoreTransition(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddStoreHandler(thread, key, dynclass, JSHandle<JSTaggedValue>(thread, handler), slotId);
    return true;
}

bool ICAccessor::StoreMissByName(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                 const JSHandle<JSTaggedValue> &value, uint16_t slotId)
{
    if (obj->IsTypedArray()) {
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key, value);
    }
    ObjectOperator op(thread, obj, key);
    bool success = JSObject::SetProperty(&op, value, false);
    if (!thread->GetEcmaVM()->ICEnable()) {
        return success;
    }

    if (!success) {
        return false;
    }
    // do not cache element and proxy
    if (!op.IsFastMode() || op.IsElement() || op.GetHolder()->IsJSProxy()) {
        return true;
    }
    // if !op.IsElement && !op.IsFound, target will be added in function AddPropertyInternal(), at the same time op will
    // be set found()
    ASSERT(op.IsFound());
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    JSTaggedValue handler;
    if (op.IsOnPrototype()) {
        // if SetProperty successfully, op.IsOnPrototype will be reset if op.IsAccessorDescriptor is false;
        ASSERT(op.IsAccessorDescriptor());
        handler = PrototypeHandler::StorePrototype(thread, op, dynclass);
    } else if (*dynclass == obj->GetJSHClass()) {
        handler = StoreHandler::StoreProperty(thread, op);
    } else {
        handler = TransitionHandler::StoreTransition(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddStoreHandler(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()), dynclass,
                                   JSHandle<JSTaggedValue>(thread, handler), slotId);
    return true;
}

JSTaggedValue ICAccessor::LoadGlobalMiss(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint16_t slotId)
{
    ObjectOperator op(thread, key);
    // ic-switch
    if (!thread->GetEcmaVM()->ICEnable()) {
        return JSObject::GetProperty(thread, &op);
    }
    // do not cache element
    if (op.IsElement()) {
        return JSObject::GetProperty(thread, &op);
    }
    JSTaggedValue handler;
    if (!op.IsFound()) {
        handler = NonExistentHandler::LoadGlobalNonExistent(thread);
    } else if (!op.IsOnPrototype()) {
        handler = LoadHandler::LoadGlobalProperty(thread, op);
    } else {
        handler = PrototypeHandler::LoadGlobalPrototype(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddGlobalHandler(thread, key, JSHandle<JSTaggedValue>(thread, handler), slotId);
    return JSObject::GetProperty(thread, &op);
}

JSTaggedValue ICAccessor::LoadGlobalMissByName(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint16_t slotId)
{
    ObjectOperator op(thread, key);
    // ic-switch
    if (!thread->GetEcmaVM()->ICEnable()) {
        return JSObject::GetProperty(thread, &op);
    }
    // do not cache element
    if (op.IsElement()) {
        return JSObject::GetProperty(thread, &op);
    }
    JSTaggedValue handler;
    if (!op.IsFound()) {
        handler = NonExistentHandler::LoadGlobalNonExistent(thread);
    } else if (!op.IsOnPrototype()) {
        handler = LoadHandler::LoadGlobalProperty(thread, op);
    } else {
        handler = PrototypeHandler::LoadGlobalPrototype(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddGlobalHandler(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()),
                                    JSHandle<JSTaggedValue>(thread, handler), slotId);
    return JSObject::GetProperty(thread, &op);
}

bool ICAccessor::StoreGlobalMissByName(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                       const JSHandle<JSTaggedValue> &value, uint16_t slotId)
{
    ObjectOperator op(thread, key);
    bool success = JSObject::SetProperty(&op, value, false);
    if (!thread->GetEcmaVM()->ICEnable()) {
        return success;
    }

    if (!success) {
        return false;
    }
    // do not cache element and proxy
    if (op.IsElement() || op.GetHolder()->IsJSProxy()) {
        return true;
    }
    ASSERT(op.IsFound());
    JSTaggedValue handler;
    if (op.IsOnPrototype()) {
        // if SetProperty successfully, op.IsOnPrototype will be reset if op.IsAccessorDescriptor is false;
        ASSERT(op.IsAccessorDescriptor());
        handler = PrototypeHandler::StoreGlobalPrototype(thread, op);
        FunctionCache::AddGlobalHandler(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()),
                                        JSHandle<JSTaggedValue>(thread, handler), slotId);
        return true;
    }
    JSHandle<JSObject> obj(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    if (*dynclass == obj->GetJSHClass()) {
        handler = StoreHandler::StoreGlobalProperty(thread, op);
    } else {
        handler = TransitionHandler::StoreTransition(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddGlobalHandler(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null()),
                                    JSHandle<JSTaggedValue>(thread, handler), slotId);
    return true;
}

bool ICAccessor::StoreGlobalMissByValue(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                        const JSHandle<JSTaggedValue> &value, uint16_t slotId)
{
    ObjectOperator op(thread, key);
    bool success = JSObject::SetProperty(&op, value, false);
    if (!thread->GetEcmaVM()->ICEnable()) {
        return success;
    }

    if (!success) {
        return false;
    }
    // do not cache element and proxy
    if (op.IsElement() || op.GetHolder()->IsJSProxy()) {
        return true;
    }
    ASSERT(op.IsFound());
    JSTaggedValue handler;
    if (op.IsOnPrototype()) {
        // if SetProperty successfully, op.IsOnPrototype will be reset if op.IsAccessorDescriptor is false;
        ASSERT(op.IsAccessorDescriptor());
        handler = PrototypeHandler::StoreGlobalPrototype(thread, op);
        FunctionCache::AddGlobalHandler(thread, key, JSHandle<JSTaggedValue>(thread, handler), slotId);
        return true;
    }
    JSHandle<JSObject> obj(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    if (*dynclass == obj->GetJSHClass()) {
        handler = StoreHandler::StoreGlobalProperty(thread, op);
    } else {
        handler = TransitionHandler::StoreTransition(thread, op);
    }
    // add handler to ic slot
    FunctionCache::AddGlobalHandler(thread, key, JSHandle<JSTaggedValue>(thread, handler), slotId);
    return true;
}

JSTaggedValue ICAccessor::LoadIC(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, uint16_t slotId)
{
    if (receiver.IsUndefined() || receiver.IsNull() || receiver.IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "LoadIC--receiver is not valid", JSTaggedValue::Exception());
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(JSHandle<JSTaggedValue>(thread, key)), "Key is not a property key");
    if (!receiver.IsECMAObject()) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key))
            .GetValue()
            .GetTaggedValue();
    }
    if (key.IsNumber()) {
        if (!receiver.IsJSPrimitiveRef() && !receiver.IsTypedArray()) {
            JSTaggedValue val = FastRuntimeStub::FastGetPropertyByIndex(thread, receiver, key.GetArrayLength());
            if (UNLIKELY(val.IsAccessorData())) {
                return JSObject::CallGetter(thread, AccessorData::Cast(val.GetTaggedObject()),
                                            JSHandle<JSTaggedValue>(thread, receiver));
            }
            return val;
        }
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key))
            .GetValue()
            .GetTaggedValue();
    }
    JSObject *obj = JSObject::Cast(receiver.GetTaggedObject());
    JSTaggedValue dynclass(obj->GetJSHClass());
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetLoadHandler(thread, key, dynclass, slotId);
    if (!handler.IsNull()) {
        JSTaggedValue ret = LoadWithHandler(thread, obj, receiver, handler);
        if (!ret.IsHole()) {
            return ret;
        }
    }
    return LoadMiss(thread, JSHandle<JSObject>(thread, obj), JSHandle<JSTaggedValue>(thread, key), slotId);
}

JSTaggedValue ICAccessor::LoadICByName(JSThread *thread, JSTaggedValue receiver, uint32_t stringId, uint16_t slotId)
{
    if (receiver.IsUndefined() || receiver.IsNull() || receiver.IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "LoadIC--receiver is not valid", JSTaggedValue::Exception());
    }
    if (UNLIKELY(!receiver.IsECMAObject())) {
        JSHandle<JSTaggedValue> receiverHandle(thread, receiver);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSTaggedValue key(factory->ResolveString(stringId));
        return JSTaggedValue::GetProperty(thread, receiverHandle, JSHandle<JSTaggedValue>(thread, key))
            .GetValue()
            .GetTaggedValue();
    }
    JSObject *obj = JSObject::Cast(receiver.GetTaggedObject());
    JSTaggedValue dynclass(obj->GetJSHClass());
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetHandlerByIndex(dynclass, slotId);
    if (!handler.IsNull()) {
        JSTaggedValue ret = LoadWithHandler(thread, obj, receiver, handler);
        if (!ret.IsHole()) {
            return ret;
        }
    }
    JSHandle<JSObject> objHandle(thread, obj);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue key(factory->ResolveString(stringId));
    return LoadMissByName(thread, objHandle, JSHandle<JSTaggedValue>(thread, key), slotId);
}

// dispatch
JSTaggedValue ICAccessor::LoadWithHandler(JSThread *thread, JSObject *obj, JSTaggedValue receiver,
                                          JSTaggedValue handler)
{
    if (handler.IsInt()) {
        return LoadProperty(thread, obj, receiver, handler);
    }
    if (handler.IsPrototypeHandler()) {
        return LoadPrototype(thread, obj, handler);
    }
    if (handler.IsGlobalHandler()) {
        return LoadGlobal(thread, receiver, handler);
    }
    ASSERT(handler.IsNonExistentHandler());
    return LoadNonExistent(handler);
}

JSTaggedValue ICAccessor::LoadProperty(JSThread *thread, JSObject *obj, JSTaggedValue receiver, JSTaggedValue handler)
{
    ASSERT(handler.IsInt());
    int32_t handlerInfo = handler.GetInt();
    JSTaggedValue ret;

    if (HandlerBase::IsInlinedProps(handlerInfo)) {
        int index = HandlerBase::GetOffset(handlerInfo);
        ret = obj->GetPropertyInlinedProps(index);
    } else if (HandlerBase::IsNonInlinedProps(handlerInfo)) {
        int index = HandlerBase::GetOffset(handlerInfo);
        ret = TaggedArray::Cast(obj->GetProperties().GetTaggedObject())->Get(index);
    } else {
        UNREACHABLE();
    }
    if (HandlerBase::IsField(handlerInfo)) {
        return ret;
    }

    if (HandlerBase::IsAccessor(handlerInfo)) {
        return JSObject::CallGetter(thread, AccessorData::Cast(ret.GetTaggedObject()),
                                    JSHandle<JSTaggedValue>(thread, receiver));
    }
    ASSERT(HandlerBase::IsInternalAccessor(handlerInfo));
    return AccessorData::Cast(ret.GetTaggedObject())->CallInternalGet(thread, JSHandle<JSObject>(thread, receiver));
}

JSTaggedValue ICAccessor::LoadGlobal(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler)
{
    ASSERT(handler.IsGlobalHandler());
    GlobalHandler *globalHandler = GlobalHandler::Cast(handler.GetTaggedObject());
    PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
    if (cell->IsInvalid()) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue ret = cell->GetValue();
    if (ret.IsAccessorData()) {
        return JSObject::CallGetter(thread, AccessorData::Cast(ret.GetTaggedObject()),
                                    JSHandle<JSTaggedValue>(thread, receiver));
    }
    return ret;
}

JSTaggedValue ICAccessor::LoadPrototype(JSThread *thread, JSObject *obj, JSTaggedValue handler)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    if (JSHClass::HasProtoChainChanged(prototypeHandler->GetProtoCell())) {
        return JSTaggedValue::Hole();
    }
    JSObject *holder = JSObject::Cast(prototypeHandler->GetHolder().GetTaggedObject());
    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    ASSERT(handlerInfo.IsInt() || handlerInfo.IsGlobalHandler());
    if (handlerInfo.IsInt()) {
        return LoadProperty(thread, holder, JSTaggedValue(obj), handlerInfo);
    }
    if (handlerInfo.IsGlobalHandler()) {
        return LoadGlobal(thread, JSTaggedValue(obj), handlerInfo);
    }
    UNREACHABLE();
}

bool ICAccessor::StoreIC(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                         uint16_t slotId)
{
    if (receiver.IsUndefined() || receiver.IsNull() || receiver.IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "StoreIC--receiver is not valid", false);
    }
    if (!receiver.IsECMAObject()) {
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value));
    }

    if (key.IsNumber()) {
        if (!receiver.IsArray(thread) && !receiver.IsTypedArray()) {
            return FastRuntimeStub::FastSetPropertyByIndex(thread, receiver, key.GetArrayLength(), value);
        }
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value));
    }
    JSObject *obj = JSObject::Cast(receiver.GetTaggedObject());
    JSTaggedValue dynclass(obj->GetJSHClass());
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetStoreHandler(thread, key, dynclass, slotId);
    bool success = false;
    if (!handler.IsNull()) {
        success = StoreWithHandler(thread, obj, key, value, handler, slotId);
    } else {
        JSHandle<JSTaggedValue> prop = JSTaggedValue::ToPropertyKey(thread, JSHandle<JSTaggedValue>(thread, key));
        success =
            StoreMiss(thread, JSHandle<JSObject>(thread, obj), prop, JSHandle<JSTaggedValue>(thread, value), slotId);
    }
    return success;
}

bool ICAccessor::StoreICByName(JSThread *thread, JSTaggedValue receiver, uint32_t stringId, JSTaggedValue value,
                               uint16_t slotId)
{
    if (receiver.IsUndefined() || receiver.IsNull() || receiver.IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "StoreIC--receiver is not valid", false);
    }
    if (!receiver.IsECMAObject()) {
        JSHandle<JSTaggedValue> receiverHandle(thread, receiver);
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSTaggedValue key(factory->ResolveString(stringId));
        return JSTaggedValue::SetProperty(thread, receiverHandle, JSHandle<JSTaggedValue>(thread, key), valueHandle);
    }
    JSObject *obj = JSObject::Cast(receiver.GetTaggedObject());
    JSTaggedValue dynclass(obj->GetJSHClass());
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetHandlerByIndex(dynclass, slotId);
    bool success = false;
    if (!handler.IsNull()) {
        success = StoreWithHandlerByName(thread, obj, stringId, value, handler, slotId);
    } else {
        JSHandle<JSObject> objHandle(thread, obj);
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSTaggedValue key(factory->ResolveString(stringId));
        success = StoreMissByName(thread, objHandle, JSHandle<JSTaggedValue>(thread, key), valueHandle, slotId);
    }
    return success;
}

bool ICAccessor::StoreWithHandler(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                                  JSTaggedValue handler, uint16_t slotId)
{
    if (handler.IsInt()) {
        StoreProperty(thread, obj, value, handler.GetInt());
        return true;
    }
    if (handler.IsGlobalHandler()) {
        return StoreGlobal(thread, obj, key, value, handler, slotId);
    }
    if (handler.IsTransitionHandler()) {
        StoreWithTransition(thread, obj, value, handler);
        return true;
    }
    if (handler.IsPrototypeHandler()) {
        return StorePrototype(thread, obj, key, value, handler, slotId);
    }
    UNREACHABLE();
}

bool ICAccessor::StoreWithHandlerByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                        JSTaggedValue handler, uint16_t slotId)
{
    if (handler.IsInt()) {
        StoreProperty(thread, obj, value, handler.GetInt());
        return true;
    }
    if (handler.IsGlobalHandler()) {
        return StoreGlobalByName(thread, obj, stringId, value, handler, slotId);
    }
    if (handler.IsTransitionHandler()) {
        StoreWithTransition(thread, obj, value, handler);
        return true;
    }
    if (handler.IsPrototypeHandler()) {
        return StorePrototypeByName(thread, obj, stringId, value, handler, slotId);
    }
    UNREACHABLE();
}

void ICAccessor::StoreProperty(JSThread *thread, JSObject *obj, JSTaggedValue value, uint32_t handler)
{
    if (StoreHandler::IsField(handler)) {
        StoreField(thread, obj, value, handler);
    } else if (StoreHandler::IsAccessor(handler)) {
        auto *setter = GetAccessor(thread, obj, handler);
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
    } else {
        ASSERT(HandlerBase::IsInternalAccessor(handler));
        AccessorData *accessor = GetAccessor(thread, obj, handler);
        accessor->CallInternalSet(thread, JSHandle<JSObject>(thread, obj), JSHandle<JSTaggedValue>(thread, value));
    }
}

void ICAccessor::StoreField(const JSThread *thread, JSObject *obj, JSTaggedValue value, uint32_t handler)
{
    int index = HandlerBase::GetOffset(handler);
    if (HandlerBase::IsInlinedProps(handler)) {
        obj->SetPropertyInlinedProps(thread, index, value);
        return;
    }
    ASSERT(HandlerBase::IsNonInlinedProps(handler));
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    int capacity = array->GetLength();
    // grow capacity
    if (UNLIKELY(index >= capacity)) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> properties;
        JSHandle<JSObject> objHandle(thread, obj);
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
}

bool ICAccessor::StoreGlobal(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                             JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsGlobalHandler());
    GlobalHandler *globalHandler = GlobalHandler::Cast(handler.GetTaggedObject());
    PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
    JSTaggedValue val = cell->GetValue();
    if (cell->IsInvalid()) {
        return StoreMiss(thread, JSHandle<JSObject>(thread, obj), JSHandle<JSTaggedValue>(thread, key),
                         JSHandle<JSTaggedValue>(thread, value), slotId);
    }
    if (UNLIKELY(val.IsAccessorData())) {
        auto *setter = AccessorData::Cast(val.GetTaggedObject());
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    cell->SetValue(thread, value);
    return true;
}

bool ICAccessor::StoreGlobalByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                   JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsGlobalHandler());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    GlobalHandler *globalHandler = GlobalHandler::Cast(handler.GetTaggedObject());
    PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
    JSTaggedValue val = cell->GetValue();
    if (cell->IsInvalid()) {
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        JSTaggedValue key(factory->ResolveString(stringId));
        return StoreGlobalMissByName(thread, JSHandle<JSTaggedValue>(thread, key), valueHandle, slotId);
    }
    if (UNLIKELY(val.IsAccessorData())) {
        auto *setter = AccessorData::Cast(val.GetTaggedObject());
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    cell->SetValue(thread, value);
    return true;
}

bool ICAccessor::StoreGlobalByValue(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                                    JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsGlobalHandler());
    GlobalHandler *globalHandler = GlobalHandler::Cast(handler.GetTaggedObject());
    PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
    JSTaggedValue val = cell->GetValue();
    if (cell->IsInvalid()) {
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        JSHandle<JSTaggedValue> keyHandle(thread, key);
        return StoreGlobalMissByValue(thread, keyHandle, valueHandle, slotId);
    }
    if (UNLIKELY(val.IsAccessorData())) {
        auto *setter = AccessorData::Cast(val.GetTaggedObject());
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    cell->SetValue(thread, value);
    return true;
}

bool ICAccessor::StorePrototype(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                                JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    if (JSHClass::HasProtoChainChanged(prototypeHandler->GetProtoCell())) {
        JSObject *holder = JSObject::Cast(prototypeHandler->GetHolder().GetTaggedObject());
        JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
        AccessorData *setter = nullptr;
        if (handlerInfo.IsInt()) {
            setter = GetAccessor(thread, holder, handlerInfo.GetInt());
        } else {
            ASSERT(handlerInfo.IsGlobalHandler());
            GlobalHandler *globalHandler = GlobalHandler::Cast(handlerInfo.GetTaggedObject());
            PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
            setter = AccessorData::Cast(cell->GetValue().GetTaggedObject());
        }
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    return StoreMiss(thread, JSHandle<JSObject>(thread, obj), JSHandle<JSTaggedValue>(thread, key),
                     JSHandle<JSTaggedValue>(thread, value), slotId);
}

bool ICAccessor::StorePrototypeByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                      JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    if (JSHClass::HasProtoChainChanged(prototypeHandler->GetProtoCell())) {
        JSObject *holder = JSObject::Cast(prototypeHandler->GetHolder().GetTaggedObject());
        JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
        AccessorData *setter;
        if (handlerInfo.IsInt()) {
            setter = GetAccessor(thread, holder, handlerInfo.GetInt());
        } else {
            ASSERT(handlerInfo.IsGlobalHandler());
            GlobalHandler *globalHandler = GlobalHandler::Cast(handlerInfo.GetTaggedObject());
            PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
            setter = AccessorData::Cast(cell->GetValue().GetTaggedObject());
        }
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    JSHandle<JSObject> objHandle(thread, obj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSTaggedValue key(factory->ResolveString(stringId));
    return StoreMissByName(thread, JSHandle<JSObject>(thread, obj), JSHandle<JSTaggedValue>(thread, key),
                           JSHandle<JSTaggedValue>(thread, value), slotId);
}

bool ICAccessor::StoreGlobalPrototypeByName(JSThread *thread, JSTaggedValue obj, uint32_t stringId, JSTaggedValue value,
                                            JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (JSHClass::HasProtoChainChanged(prototypeHandler->GetProtoCell())) {
        JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
        ASSERT(handlerInfo.IsGlobalHandler());
        GlobalHandler *globalHandler = GlobalHandler::Cast(handlerInfo.GetTaggedObject());
        PropertyBox *cell = PropertyBox::Cast(globalHandler->GetPropertyBox().GetTaggedObject());
        auto *setter = AccessorData::Cast(cell->GetValue().GetTaggedObject());
        JSObject::CallSetter(thread, *setter, JSHandle<JSTaggedValue>(thread, obj),
                             JSHandle<JSTaggedValue>(thread, value));
        return true;
    }
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSTaggedValue key(factory->ResolveString(stringId));
    return StoreGlobalMissByName(thread, JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value),
                                 slotId);
}

AccessorData *ICAccessor::GetAccessor([[maybe_unused]] JSThread *thread, JSObject *obj, uint32_t handler)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue accessor = JSTaggedValue::Undefined();
    int index = HandlerBase::GetOffset(handler);
    if (HandlerBase::IsInlinedProps(handler)) {
        accessor = obj->GetPropertyInlinedProps(index);
    } else if (HandlerBase::IsNonInlinedProps(handler)) {
        TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
        accessor = array->Get(index);
    }
    ASSERT(!accessor.IsUndefined());
    return AccessorData::Cast(accessor.GetTaggedObject());
}

bool ICAccessor::StoreGlobalICByValue(JSThread *thread, JSTaggedValue key, JSTaggedValue value, uint16_t slotId)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(JSHandle<JSTaggedValue>(thread, key)), "Key is not a property key");

    if (key.IsNumber()) {
        JSTaggedValue globalValue = thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject();
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, globalValue),
                                          JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value));
    }
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetGlobalHandlerByValue(key, slotId);
    bool success = false;
    if (!handler.IsNull()) {
        JSTaggedValue globalValue = thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject();
        success = StoreGlobalWithHandlerByValue(thread, globalValue, key, value, handler, slotId);
    } else {
        success = StoreGlobalMissByValue(thread, JSHandle<JSTaggedValue>(thread, key),
                                         JSHandle<JSTaggedValue>(thread, value), slotId);
    }
    return success;
}

JSTaggedValue ICAccessor::LoadGlobalIC(JSThread *thread, JSTaggedValue key, uint16_t slotId)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(JSHandle<JSTaggedValue>(thread, key)), "Key is not a property key");
    JSTaggedValue globalValue = thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject();
    if (key.IsNumber()) {
        return FastRuntimeStub::FastGetPropertyByIndex(thread, globalValue, key.GetArrayLength());
    }
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetGlobalHandlerByValue(key, slotId);
    if (!handler.IsNull()) {
        JSTaggedValue ret = LoadGlobalWithHandler(thread, globalValue, handler);
        if (!ret.IsHole()) {
            return ret;
        }
    }
    return LoadGlobalMiss(thread, JSHandle<JSTaggedValue>(thread, key), slotId);
}

JSTaggedValue ICAccessor::LoadGlobalICByName(JSThread *thread, uint32_t stringId, uint16_t slotId)
{
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetGlobalHandlerByIndex(slotId);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!handler.IsNull()) {
        JSTaggedValue globalValue = thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject();
        JSTaggedValue ret = LoadGlobalWithHandler(thread, globalValue, handler);
        if (!ret.IsHole()) {
            return ret;
        }
    }
    JSTaggedValue key(factory->ResolveString(stringId));
    return LoadGlobalMissByName(thread, JSHandle<JSTaggedValue>(thread, key), slotId);
}
}  // namespace panda::ecmascript
