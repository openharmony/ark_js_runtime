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

#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler-inl.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory-inl.h"

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
            handlerValue = PrototypeHandler::LoadPrototype(thread_, op, hclass);
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
    if (op.IsElement()) {
        handlerValue = StoreHandler::StoreElement(thread_, receiver);
    } else {
        ASSERT(op.IsFound());
        if (op.IsOnPrototype()) {
            // do not support global prototype ic
            if (IsGlobalStoreIC(GetICKind())) {
                return;
            }
            JSHandle<JSHClass> hclass(thread_, JSHandle<JSObject>::Cast(receiver)->GetClass());
            handlerValue = PrototypeHandler::StorePrototype(thread_, op, hclass);
        } else if (op.IsTransition()) {
            handlerValue = TransitionHandler::StoreTransition(thread_, op);
        } else {
            handlerValue = StoreHandler::StoreProperty(thread_, op);
        }
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
        LOG(ERROR, RUNTIME) << kind << " miss key is: " << JSHandle<EcmaString>::Cast(key)->GetCString().get()
                            << ", receiver is " << receiver->GetHeapObject()->GetClass()->IsDictionaryMode()
                            << ", state is " << state;
    } else {
        LOG(ERROR, RUNTIME) << kind << " miss " << ", state is "
                            << ", receiver is " << receiver->GetHeapObject()->GetClass()->IsDictionaryMode()
                            << state;
    }
#endif
}

JSTaggedValue LoadICRuntime::LoadMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key)
{
    if (receiver->IsTypedArray() || !receiver->IsJSObject()) {
        return JSTaggedValue::GetProperty(GetThread(), receiver, key).GetValue().GetTaggedValue();
    }
    ObjectOperator op(GetThread(), receiver, key);
    auto result = JSHandle<JSTaggedValue>(thread_, JSObject::GetProperty(GetThread(), &op));
    if (!op.IsFound() && GetICKind() == ICKind::NamedGlobalLoadIC) {
        return SlowRuntimeStub::ThrowReferenceError(GetThread(), key.GetTaggedValue(), " is not defined");
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(GetThread());
    // ic-switch
    if (!GetThread()->GetEcmaVM()->ICEnable()) {
        icAccessor_.SetAsMega();
        return result.GetTaggedValue();
    }
#ifndef NDEBUG
    TraceIC(receiver, key);
#endif
    // do not cache element
    if (!op.IsFastMode() && op.IsFound()) {
        icAccessor_.SetAsMega();
        return result.GetTaggedValue();
    }

    UpdateLoadHandler(op, key, receiver);
    return result.GetTaggedValue();
}

JSTaggedValue StoreICRuntime::StoreMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key,
                                        JSHandle<JSTaggedValue> value)
{
    if (receiver->IsTypedArray() || !receiver->IsJSObject()) {
        bool success = JSTaggedValue::SetProperty(GetThread(), receiver, key, value, true);
        return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
    }
    UpdateReceiverHClass(JSHandle<JSTaggedValue>(GetThread(), JSHandle<JSObject>::Cast(receiver)->GetClass()));
    ObjectOperator op(GetThread(), receiver, key);
    bool success = JSObject::SetProperty(&op, value, true);
    if (!success && GetICKind() == ICKind::NamedGlobalStoreIC) {
        return SlowRuntimeStub::ThrowReferenceError(GetThread(), key.GetTaggedValue(), " is not defined");
    }
    // ic-switch
    if (!GetThread()->GetEcmaVM()->ICEnable()) {
        icAccessor_.SetAsMega();
        return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
    }
#ifndef NDEBUG
    TraceIC(receiver, key);
#endif
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
