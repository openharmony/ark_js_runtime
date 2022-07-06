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

#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tagged_dictionary.h"
namespace panda::ecmascript {
#define TRACE_IC 0  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

void ICRuntime::UpdateLoadHandler(const ObjectOperator &op, JSHandle<JSTaggedValue> key,
                                  JSHandle<JSTaggedValue> receiver)
{
    if (icAccessor_.GetICState() == ProfileTypeAccessor::ICState::MEGA) {
        return;
    }
    if (IsNamedIC(GetICKind())) {
        key = JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> handlerValue;
    JSHandle<JSHClass> hclass(GetThread(), JSHandle<JSObject>::Cast(receiver)->GetClass());
    if (op.IsElement()) {
        if (!op.IsFound() && hclass->IsDictionaryElement()) {
            return;
        }
        handlerValue = LoadHandler::LoadElement(thread_);
    } else {
        if (!op.IsFound()) {
            JSTaggedValue proto = hclass->GetPrototype();
            if (!proto.IsECMAObject()) {
                handlerValue = LoadHandler::LoadProperty(thread_, op);
            } else {
                handlerValue = PrototypeHandler::LoadPrototype(thread_, op, hclass);
            }
        } else if (!op.IsOnPrototype()) {
            handlerValue = LoadHandler::LoadProperty(thread_, op);
        } else {
            // do not support global prototype ic
            if (IsGlobalLoadIC(GetICKind())) {
                return;
            }
            handlerValue = PrototypeHandler::LoadPrototype(thread_, op, hclass);
        }
    }

    if (key.IsEmpty()) {
        icAccessor_.AddHandlerWithoutKey(JSHandle<JSTaggedValue>::Cast(hclass), handlerValue);
    } else if (op.IsElement()) {
        // do not support global element ic
        if (IsGlobalLoadIC(GetICKind())) {
            return;
        }
        icAccessor_.AddElementHandler(JSHandle<JSTaggedValue>::Cast(hclass), handlerValue);
    } else {
        icAccessor_.AddHandlerWithKey(key, JSHandle<JSTaggedValue>::Cast(hclass), handlerValue);
    }
}

void ICRuntime::UpdateStoreHandler(const ObjectOperator &op, JSHandle<JSTaggedValue> key,
                                   JSHandle<JSTaggedValue> receiver)
{
    if (icAccessor_.GetICState() == ProfileTypeAccessor::ICState::MEGA) {
        return;
    }
    if (IsNamedIC(GetICKind())) {
        key = JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> handlerValue;
    ASSERT(op.IsFound());
    if (op.IsOnPrototype()) {
        // do not support global prototype ic
        if (IsGlobalStoreIC(GetICKind())) {
            return;
        }
        JSHandle<JSHClass> hclass(thread_, JSHandle<JSObject>::Cast(receiver)->GetClass());
        handlerValue = PrototypeHandler::StorePrototype(thread_, op, hclass);
    } else if (op.IsTransition()) {
        ASSERT(!op.IsElement());
        handlerValue = TransitionHandler::StoreTransition(thread_, op);
    } else {
        handlerValue = StoreHandler::StoreProperty(thread_, op);
    }

    if (key.IsEmpty()) {
        icAccessor_.AddHandlerWithoutKey(receiverHClass_, handlerValue);
    } else if (op.IsElement()) {
        // do not support global element ic
        if (IsGlobalStoreIC(GetICKind())) {
            return;
        }
        icAccessor_.AddElementHandler(receiverHClass_, handlerValue);
    } else {
        icAccessor_.AddHandlerWithKey(key, receiverHClass_, handlerValue);
    }
}

void ICRuntime::TraceIC([[maybe_unused]] JSHandle<JSTaggedValue> receiver,
                        [[maybe_unused]] JSHandle<JSTaggedValue> key) const
{
#if TRACE_IC
    auto kind = ICKindToString(GetICKind());
    auto state = ProfileTypeAccessor::ICStateToString(icAccessor_.GetICState());
    if (key->IsString()) {
        LOG_ECMA(ERROR) << kind << " miss key is: " << JSHandle<EcmaString>::Cast(key)->GetCString().get()
                            << ", receiver is " << receiver->GetTaggedObject()->GetClass()->IsDictionaryMode()
                            << ", state is " << state;
    } else {
        LOG_ECMA(ERROR) << kind << " miss " << ", state is "
                            << ", receiver is " << receiver->GetTaggedObject()->GetClass()->IsDictionaryMode()
                            << state;
    }
#endif
}

JSTaggedValue LoadICRuntime::LoadMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key)
{
    if (receiver->IsTypedArray() || !receiver->IsJSObject() || receiver->IsSpecialContainer()) {
        icAccessor_.SetAsMega();
        return JSTaggedValue::GetProperty(thread_, receiver, key).GetValue().GetTaggedValue();
    }

    // global variable find from global record firstly
    if (GetICKind() == ICKind::NamedGlobalLoadIC) {
        JSTaggedValue box = SlowRuntimeStub::LdGlobalRecord(thread_, key.GetTaggedValue());
        if (!box.IsUndefined()) {
            ASSERT(box.IsPropertyBox());
            icAccessor_.AddGlobalRecordHandler(JSHandle<JSTaggedValue>(thread_, box));
            return PropertyBox::Cast(box.GetTaggedObject())->GetValue();
        }
    }

    ObjectOperator op(GetThread(), receiver, key);
    auto result = JSHandle<JSTaggedValue>(thread_, JSObject::GetProperty(GetThread(), &op));
    if (!op.IsFound() && GetICKind() == ICKind::NamedGlobalLoadIC) {
        return SlowRuntimeStub::ThrowReferenceError(GetThread(), key.GetTaggedValue(), " is not definded");
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(GetThread());
    // ic-switch
    if (!GetThread()->GetEcmaVM()->ICEnabled()) {
        icAccessor_.SetAsMega();
        return result.GetTaggedValue();
    }
    TraceIC(receiver, key);
    // do not cache element
    if (!op.IsFastMode()) {
        icAccessor_.SetAsMega();
        return result.GetTaggedValue();
    }

    UpdateLoadHandler(op, key, receiver);
    return result.GetTaggedValue();
}

JSTaggedValue StoreICRuntime::StoreMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key,
                                        JSHandle<JSTaggedValue> value)
{
    if (receiver->IsTypedArray() || !receiver->IsJSObject() || receiver->IsSpecialContainer()) {
        icAccessor_.SetAsMega();
        bool success = JSTaggedValue::SetProperty(GetThread(), receiver, key, value, true);
        return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
    }

    // global variable find from global record firstly
    if (GetICKind() == ICKind::NamedGlobalStoreIC) {
        JSTaggedValue box = SlowRuntimeStub::LdGlobalRecord(thread_, key.GetTaggedValue());
        if (!box.IsUndefined()) {
            ASSERT(box.IsPropertyBox());
            SlowRuntimeStub::TryUpdateGlobalRecord(thread_, key.GetTaggedValue(), value.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
            icAccessor_.AddGlobalRecordHandler(JSHandle<JSTaggedValue>(thread_, box));
            return JSTaggedValue::Undefined();
        }
    }
    UpdateReceiverHClass(JSHandle<JSTaggedValue>(GetThread(), JSHandle<JSObject>::Cast(receiver)->GetClass()));

    ObjectOperator op(GetThread(), receiver, key);
    bool success = JSObject::SetProperty(&op, value, true);
    if (!success && GetICKind() == ICKind::NamedGlobalStoreIC) {
        return SlowRuntimeStub::ThrowReferenceError(GetThread(), key.GetTaggedValue(), " is not defined");
    }
    // ic-switch
    if (!GetThread()->GetEcmaVM()->ICEnabled()) {
        icAccessor_.SetAsMega();
        return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
    }
    TraceIC(receiver, key);
    // do not cache element
    if (!op.IsFastMode()) {
        icAccessor_.SetAsMega();
        return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
    }
    if (success) {
        UpdateStoreHandler(op, key, receiver);
        return JSTaggedValue::Undefined();
    }
    return JSTaggedValue::Exception();
}
}  // namespace panda::ecmascript
