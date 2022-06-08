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

#include "ecmascript/tooling/agent/runtime_impl.h"

#include <iomanip>

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/protocol_channel.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
void RuntimeImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    static std::unordered_map<std::string, AgentHandler> dispatcherTable {
        { "enable", &RuntimeImpl::DispatcherImpl::Enable },
        { "getProperties", &RuntimeImpl::DispatcherImpl::GetProperties },
        { "runIfWaitingForDebugger", &RuntimeImpl::DispatcherImpl::RunIfWaitingForDebugger },
        { "callFunctionOn", &RuntimeImpl::DispatcherImpl::CallFunctionOn },
        { "getHeapUsage", &RuntimeImpl::DispatcherImpl::GetHeapUsage }
    };

    const std::string &method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to RuntimeImpl";

    auto entry = dispatcherTable.find(method);
    if (entry != dispatcherTable.end()) {
        (this->*(entry->second))(request);
    } else {
        LOG(ERROR, DEBUGGER) << "unknown method: " << method;
        SendResponse(request, DispatchResponse::Fail("unknown method: " + method));
    }
}

void RuntimeImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->Enable();
    SendResponse(request, response);
}

void RuntimeImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->Disable();
    SendResponse(request, response);
}

void RuntimeImpl::DispatcherImpl::RunIfWaitingForDebugger(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->RunIfWaitingForDebugger();
    SendResponse(request, response);
}

void RuntimeImpl::DispatcherImpl::GetProperties(const DispatchRequest &request)
{
    std::unique_ptr<GetPropertiesParams> params = GetPropertiesParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    std::vector<std::unique_ptr<PropertyDescriptor>> outPropertyDesc;
    std::optional<std::vector<std::unique_ptr<InternalPropertyDescriptor>>> outInternalDescs;
    std::optional<std::vector<std::unique_ptr<PrivatePropertyDescriptor>>> outPrivateProperties;
    std::optional<std::unique_ptr<ExceptionDetails>> outExceptionDetails;
    DispatchResponse response = runtime_->GetProperties(std::move(params), &outPropertyDesc, &outInternalDescs,
        &outPrivateProperties, &outExceptionDetails);
    if (outExceptionDetails) {
        LOG(WARNING, DEBUGGER) << "GetProperties thrown an exception";
    }
    GetPropertiesReturns result(std::move(outPropertyDesc),
        std::move(outInternalDescs),
        std::move(outPrivateProperties),
        std::move(outExceptionDetails));
    SendResponse(request, response, result);
}

void RuntimeImpl::DispatcherImpl::CallFunctionOn(const DispatchRequest &request)
{
    std::unique_ptr<CallFunctionOnParams> params =
        CallFunctionOnParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    std::unique_ptr<RemoteObject> outRemoteObject;
    std::optional<std::unique_ptr<ExceptionDetails>> outExceptionDetails;
    DispatchResponse response = runtime_->CallFunctionOn(std::move(params), &outRemoteObject, &outExceptionDetails);
    if (outExceptionDetails) {
        LOG(WARNING, DEBUGGER) << "CallFunctionOn thrown an exception";
    }
    CallFunctionOnReturns result(std::move(outRemoteObject), std::move(outExceptionDetails));
    SendResponse(request, response, result);
}

void RuntimeImpl::DispatcherImpl::GetHeapUsage(const DispatchRequest &request)
{
    double usedSize = 0;
    double totalSize = 0;
    DispatchResponse response = runtime_->GetHeapUsage(&usedSize, &totalSize);
    GetHeapUsageReturns result(usedSize, totalSize);
    SendResponse(request, response, result);
}

bool RuntimeImpl::Frontend::AllowNotify() const
{
    return channel_ != nullptr;
}

void RuntimeImpl::Frontend::RunIfWaitingForDebugger()
{
    if (!AllowNotify()) {
        return;
    }

    channel_->RunIfWaitingForDebugger();
}

DispatchResponse RuntimeImpl::Enable()
{
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::Disable()
{
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::RunIfWaitingForDebugger()
{
    frontend_.RunIfWaitingForDebugger();
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::CallFunctionOn([[maybe_unused]] std::unique_ptr<CallFunctionOnParams> params,
    std::unique_ptr<RemoteObject> *outRemoteObject,
    [[maybe_unused]] std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails)
{
    // Return EvalError temporarily.
    auto error = Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, "Unsupport eval now"));
    *outRemoteObject = RemoteObject::FromTagged(vm_, error);
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::GetHeapUsage(double *usedSize, double *totalSize)
{
    *totalSize = static_cast<double>(DFXJSNApi::GetHeapTotalSize(vm_));
    *usedSize = static_cast<double>(DFXJSNApi::GetHeapUsedSize(vm_));
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::GetProperties(std::unique_ptr<GetPropertiesParams> params,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc,
    [[maybe_unused]] std::optional<std::vector<std::unique_ptr<InternalPropertyDescriptor>>> *outInternalDescs,
    [[maybe_unused]] std::optional<std::vector<std::unique_ptr<PrivatePropertyDescriptor>>> *outPrivateProps,
    [[maybe_unused]] std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails)
{
    RemoteObjectId objectId = params->GetObjectId();
    bool isOwn = params->GetOwnProperties();
    bool isAccessorOnly = params->GetAccessPropertiesOnly();
    auto iter = properties_.find(objectId);
    if (iter == properties_.end()) {
        LOG(ERROR, DEBUGGER) << "RuntimeImpl::GetProperties Unknown object id: " << objectId;
        return DispatchResponse::Fail("Unknown object id");
    }
    Local<JSValueRef> value = Local<JSValueRef>(vm_, iter->second);
    if (value.IsEmpty() || !value->IsObject()) {
        LOG(ERROR, DEBUGGER) << "RuntimeImpl::GetProperties should a js object";
        return DispatchResponse::Fail("Not a object");
    }
    if (value->IsArrayBuffer()) {
        Local<ArrayBufferRef> arrayBufferRef(value);
        AddTypedArrayRefs(arrayBufferRef, outPropertyDesc);
    }
    Local<ArrayRef> keys = Local<ObjectRef>(value)->GetOwnPropertyNames(vm_);
    int32_t length = keys->Length(vm_);
    Local<JSValueRef> name = JSValueRef::Undefined(vm_);
    for (int32_t i = 0; i < length; ++i) {
        name = keys->Get(vm_, i);
        PropertyAttribute jsProperty = PropertyAttribute::Default();
        if (!Local<ObjectRef>(value)->GetOwnProperty(vm_, name, jsProperty)) {
            continue;
        }
        std::unique_ptr<PropertyDescriptor> debuggerProperty =
            PropertyDescriptor::FromProperty(vm_, name, jsProperty);
        if (isAccessorOnly && !jsProperty.HasGetter() && !jsProperty.HasSetter()) {
            continue;
        }
        if (debuggerProperty->HasGet()) {
            debuggerProperty->GetGet()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsProperty.GetGetter(vm_));
        }
        if (debuggerProperty->HasSet()) {
            debuggerProperty->GetSet()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsProperty.GetSetter(vm_));
        }
        if (debuggerProperty->HasValue()) {
            Local<JSValueRef> vValue = jsProperty.GetValue(vm_);
            if (vValue->IsObject() && !vValue->IsProxy()) {
                debuggerProperty->GetValue()->SetObjectId(curObjectId_);
                properties_[curObjectId_++] = Global<JSValueRef>(vm_, vValue);
            }
        }
        if (debuggerProperty->HasSymbol()) {
            debuggerProperty->GetSymbol()->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, name);
        }
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
    GetAdditionalProperties(value, outPropertyDesc);

    return DispatchResponse::Ok();
}

void RuntimeImpl::AddTypedArrayRefs(Local<ArrayBufferRef> arrayBufferRef,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    int32_t arrayBufferByteLength = arrayBufferRef->ByteLength(vm_);
    int32_t typedArrayLength = arrayBufferByteLength;
    AddTypedArrayRef<Int8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ClampedArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8ClampedArray]]", outPropertyDesc);

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_16BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_16BITS;
        AddTypedArrayRef<Int16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int16Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint16Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_32BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_32BITS;
        AddTypedArrayRef<Int32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int32Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint32Array]]", outPropertyDesc);
        AddTypedArrayRef<Float32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float32Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_64BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_64BITS;
        AddTypedArrayRef<Float64ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigInt64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigInt64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigUint64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigUint64Array]]", outPropertyDesc);
    }
}

template <typename TypedArrayRef>
void RuntimeImpl::AddTypedArrayRef(Local<ArrayBufferRef> arrayBufferRef, int32_t length, const char* name,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRefTypedArray(TypedArrayRef::New(vm_, arrayBufferRef, 0, length));
    std::unique_ptr<RemoteObject> remoteObjectTypedArray = RemoteObject::FromTagged(vm_, jsValueRefTypedArray);
    remoteObjectTypedArray->SetObjectId(curObjectId_);
    properties_[curObjectId_++] = Global<JSValueRef>(vm_, jsValueRefTypedArray);
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName(name)
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(remoteObjectTypedArray));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void RuntimeImpl::CacheObjectIfNeeded(Local<JSValueRef> valRef, RemoteObject *remoteObj)
{
    if (valRef->IsObject() && !valRef->IsProxy()) {
        remoteObj->SetObjectId(curObjectId_);
        properties_[curObjectId_++] = Global<JSValueRef>(vm_, valRef);
    }
}

void RuntimeImpl::GetProtoOrProtoType(const Local<JSValueRef> &value, bool isOwn, bool isAccessorOnly,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    if (!isAccessorOnly && isOwn && !value->IsProxy()) {
        return;
    }
    // Get Function ProtoOrDynClass
    if (value->IsConstructor()) {
        Local<JSValueRef> prototype = Local<FunctionRef>(value)->GetFunctionPrototype(vm_);
        std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(vm_, prototype);
        CacheObjectIfNeeded(prototype, protoObj.get());
        std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
        debuggerProperty->SetName("prototype")
            .SetWritable(false)
            .SetConfigurable(false)
            .SetEnumerable(false)
            .SetIsOwn(true)
            .SetValue(std::move(protoObj));
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    // Get __proto__
    Local<JSValueRef> proto = Local<ObjectRef>(value)->GetPrototype(vm_);
    std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(vm_, proto);
    CacheObjectIfNeeded(proto, protoObj.get());
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName("__proto__")
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(protoObj));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void RuntimeImpl::GetAdditionalProperties(const Local<JSValueRef> &value,
    std::vector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    // The length of the TypedArray have to be limited(less than or equal to lengthTypedArrayLimit) until we construct
    // the PropertyPreview class. Let lengthTypedArrayLimit be 10000 temporarily.
    static const uint32_t lengthTypedArrayLimit = 10000;

    // The width of the string-expression for JSTypedArray::MAX_TYPED_ARRAY_INDEX which is euqal to
    // JSObject::MAX_ELEMENT_INDEX which is equal to std::numeric_limits<uint32_t>::max(). (42,9496,7295)
    static const int32_t widthStrExprMaxElementIndex = 10;

    if (value->IsTypedArray()) {
        Local<TypedArrayRef> localTypedArrayRef(value);
        uint32_t lengthTypedArray = localTypedArrayRef->ArrayLength(vm_);
        if (lengthTypedArray > lengthTypedArrayLimit) {
            LOG(ERROR, DEBUGGER) << "The length of the TypedArray is non-compliant or unsupported.";
            return;
        }
        for (uint32_t i = 0; i < lengthTypedArray; i++) {
            Local<JSValueRef> localValRefElement = localTypedArrayRef->Get(vm_, i);
            std::unique_ptr<RemoteObject> remoteObjElement = RemoteObject::FromTagged(vm_, localValRefElement);
            remoteObjElement->SetObjectId(curObjectId_);
            properties_[curObjectId_++] = Global<JSValueRef>(vm_, localValRefElement);
            std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();

            std::ostringstream osNameElement;
            osNameElement << std::right << std::setw(widthStrExprMaxElementIndex) << i;
            std::string cStrNameElement = osNameElement.str();
            debuggerProperty->SetName(cStrNameElement)
                .SetWritable(true)
                .SetConfigurable(true)
                .SetEnumerable(false)
                .SetIsOwn(true)
                .SetValue(std::move(remoteObjElement));
            outPropertyDesc->emplace_back(std::move(debuggerProperty));
        }
    }
}
}  // namespace panda::ecmascript::tooling