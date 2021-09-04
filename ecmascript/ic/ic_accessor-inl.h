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

#ifndef PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_INL_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/object_factory.h"
#include "function_cache.h"
#include "ic_accessor.h"
#include "ic_handler-inl.h"

namespace panda::ecmascript {
inline JSTaggedValue ICAccessor::LoadGlobalWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler)
{
    if (handler.IsPrototypeHandler()) {
        return LoadGlobalPrototype(thread, receiver, handler);
    }
    if (handler.IsGlobalHandler()) {
        return LoadGlobal(thread, receiver, handler);
    }
    ASSERT(handler.IsNonExistentHandler());
    return LoadNonExistent(handler);
}

inline JSTaggedValue ICAccessor::LoadGlobalPrototype(JSThread *thread, JSTaggedValue obj, JSTaggedValue handler)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
    if (JSHClass::HasProtoChainChanged(prototypeHandler->GetProtoCell())) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue handlerInfo = prototypeHandler->GetHandlerInfo();
    ASSERT(handlerInfo.IsGlobalHandler());
    return LoadGlobal(thread, obj, handlerInfo);
}

inline JSTaggedValue ICAccessor::LoadNonExistent(JSTaggedValue handler)
{
    NonExistentHandler *nonExistentHandler = NonExistentHandler::Cast(handler.GetTaggedObject());
    if (JSHClass::HasProtoChainChanged(nonExistentHandler->GetProtoCell())) {
        return JSTaggedValue::Hole();
    }
    return JSTaggedValue::Undefined();
}

inline bool ICAccessor::StoreGlobalICByName(JSThread *thread, uint32_t stringId, JSTaggedValue value, uint16_t slotId)
{
    FunctionCache *cache = FunctionCache::GetCurrent(thread);
    JSTaggedValue handler = cache->GetGlobalHandlerByIndex(slotId);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    bool success = false;
    if (!handler.IsNull()) {
        JSTaggedValue globalValue = thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject();
        success = StoreGlobalWithHandlerByName(thread, globalValue, stringId, value, handler, slotId);
    } else {
        JSTaggedValue key(factory->ResolveString(stringId));
        success = StoreGlobalMissByName(thread, JSHandle<JSTaggedValue>(thread, key),
                                        JSHandle<JSTaggedValue>(thread, value), slotId);
    }
    return success;
}

inline bool ICAccessor::StoreGlobalWithHandlerByName(JSThread *thread, JSTaggedValue obj, uint32_t stringId,
                                                     JSTaggedValue value, JSTaggedValue handler, uint16_t slotId)
{
    if (handler.IsPrototypeHandler()) {
        return StoreGlobalPrototypeByName(thread, obj, stringId, value, handler, slotId);
    }
    if (handler.IsGlobalHandler()) {
        return StoreGlobalByName(thread, JSObject::Cast(obj.GetTaggedObject()), stringId, value, handler, slotId);
    }
    ASSERT(handler.IsTransitionHandler());
    StoreWithTransition(thread, JSObject::Cast(obj.GetTaggedObject()), value, handler);
    return true;
}

inline bool ICAccessor::StoreGlobalWithHandlerByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                      JSTaggedValue value, JSTaggedValue handler, uint16_t slotId)
{
    if (handler.IsPrototypeHandler()) {
        return StoreGlobalPrototypeByValue(thread, obj, key, value, handler, slotId);
    }
    if (handler.IsGlobalHandler()) {
        return StoreGlobalByValue(thread, JSObject::Cast(obj.GetTaggedObject()), key, value, handler, slotId);
    }
    ASSERT(handler.IsTransitionHandler());
    StoreWithTransition(thread, JSObject::Cast(obj.GetTaggedObject()), value, handler);
    return true;
}

inline bool ICAccessor::StoreGlobalPrototypeByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                    JSTaggedValue value, JSTaggedValue handler, uint16_t slotId)
{
    ASSERT(handler.IsPrototypeHandler());
    PrototypeHandler *prototypeHandler = PrototypeHandler::Cast(handler.GetTaggedObject());
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
    JSHandle<JSTaggedValue> value_handle(thread, value);
    return StoreGlobalMissByValue(thread, JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value),
                                  slotId);
}
inline void ICAccessor::StoreWithTransition(const JSThread *thread, JSObject *obj, JSTaggedValue value,
                                            JSTaggedValue handler)
{
    TransitionHandler *transitionHandler = TransitionHandler::Cast(handler.GetTaggedObject());
    JSHClass *newDynclass = JSHClass::Cast(transitionHandler->GetTransitionHClass().GetTaggedObject());
    obj->SetClass(newDynclass);
    uint32_t handlerInfo = transitionHandler->GetHandlerInfo().GetInt();
    ASSERT(HandlerBase::IsField(handlerInfo));
    StoreField(thread, obj, value, handlerInfo);
}
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_INL_H
