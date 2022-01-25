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

#ifndef ECMASCRIPT_IC_IC_HANDLER_INL_H
#define ECMASCRIPT_IC_IC_HANDLER_INL_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_object-inl.h"
#include "ic_handler.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> LoadHandler::LoadElement(const JSThread *thread)
{
    uint32_t handler = 0;
    KindBit::Set<uint32_t>(HandlerKind::ELEMENT, &handler);
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
}

JSHandle<JSTaggedValue> LoadHandler::LoadProperty(const JSThread *thread, const ObjectOperator &op)
{
    uint32_t handler = 0;
    ASSERT(!op.IsElement());
    if (!op.IsFound()) {
        KindBit::Set<uint32_t>(HandlerKind::NON_EXIST, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }
    ASSERT(op.IsFastMode());

    JSTaggedValue val = op.GetValue();
    if (val.IsPropertyBox()) {
        return JSHandle<JSTaggedValue>(thread, val);
    }
    bool hasAccessor = op.IsAccessorDescriptor();
    AccessorBit::Set<uint32_t>(hasAccessor, &handler);
    if (!hasAccessor) {
        KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);
    }

    if (op.IsInlinedProps()) {
        InlinedPropsBit::Set<uint32_t>(true, &handler);
        JSHandle<JSObject> holder = JSHandle<JSObject>::Cast(op.GetHolder());
        auto index = holder->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
        OffsetBit::Set<uint32_t>(index, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }
    if (op.IsFastMode()) {
        OffsetBit::Set<uint32_t>(op.GetIndex(), &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }
    UNREACHABLE();
}

JSHandle<JSTaggedValue> PrototypeHandler::LoadPrototype(const JSThread *thread,
                                                        const ObjectOperator &op,
                                                        const JSHandle<JSHClass> &hclass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handlerInfo = LoadHandler::LoadProperty(thread, op);
    JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
    handler->SetHandlerInfo(thread, handlerInfo);
    if (op.IsFound()) {
        handler->SetHolder(thread, op.GetHolder());
    }
    auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
    handler->SetProtoCell(thread, result);
    return JSHandle<JSTaggedValue>::Cast(handler);
}

JSHandle<JSTaggedValue> PrototypeHandler::StorePrototype(const JSThread *thread,
                                                         const ObjectOperator &op,
                                                         const JSHandle<JSHClass> &hclass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PrototypeHandler> handler = factory->NewPrototypeHandler();
    JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
    handler->SetHandlerInfo(thread, handlerInfo);
    handler->SetHolder(thread, op.GetHolder());
    auto result = JSHClass::EnableProtoChangeMarker(thread, hclass);
    handler->SetProtoCell(thread, result);
    return JSHandle<JSTaggedValue>::Cast(handler);
}

JSHandle<JSTaggedValue> StoreHandler::StoreElement(const JSThread *thread, JSHandle<JSTaggedValue> receiver)
{
    uint32_t handler = 0;
    KindBit::Set<uint32_t>(HandlerKind::ELEMENT, &handler);

    if (receiver->IsJSArray()) {
        IsJSArrayBit::Set<uint32_t>(true, &handler);
    }
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
}

JSHandle<JSTaggedValue> StoreHandler::StoreProperty(const JSThread *thread, const ObjectOperator &op)
{
    if (op.IsElement()) {
        return StoreElement(thread, op.GetReceiver());
    }
    uint32_t handler = 0;
    JSTaggedValue val = op.GetValue();
    if (val.IsPropertyBox()) {
        return JSHandle<JSTaggedValue>(thread, val);
    }
    bool hasSetter = op.IsAccessorDescriptor();
    AccessorBit::Set<uint32_t>(hasSetter, &handler);
    if (!hasSetter) {
        KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);
    }
    if (op.IsInlinedProps()) {
        InlinedPropsBit::Set<uint32_t>(true, &handler);
        JSHandle<JSObject> receiver = JSHandle<JSObject>::Cast(op.GetReceiver());
        auto index = receiver->GetJSHClass()->GetInlinedPropertiesIndex(op.GetIndex());
        OffsetBit::Set<uint32_t>(index, &handler);
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
    }
    ASSERT(op.IsFastMode());
    OffsetBit::Set<uint32_t>(op.GetIndex(), &handler);
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue(handler));
}

JSHandle<JSTaggedValue> TransitionHandler::StoreTransition(const JSThread *thread, const ObjectOperator &op)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TransitionHandler> handler = factory->NewTransitionHandler();
    JSHandle<JSTaggedValue> handlerInfo = StoreHandler::StoreProperty(thread, op);
    handler->SetHandlerInfo(thread, handlerInfo);
    auto hclass = JSObject::Cast(op.GetReceiver()->GetHeapObject())->GetJSHClass();
    handler->SetTransitionHClass(thread, JSTaggedValue(hclass));
    return JSHandle<JSTaggedValue>::Cast(handler);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_HANDLER_INL_H
