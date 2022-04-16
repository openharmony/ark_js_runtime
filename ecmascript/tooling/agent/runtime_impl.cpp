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

#include "ecmascript/tooling/base/pt_returns.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
RuntimeImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<RuntimeImpl> runtime)
    : DispatcherBase(frontend), runtime_(std::move(runtime))
{
    dispatcherTable_["enable"] = &RuntimeImpl::DispatcherImpl::Enable;
    dispatcherTable_["getProperties"] = &RuntimeImpl::DispatcherImpl::GetProperties;
    dispatcherTable_["runIfWaitingForDebugger"] = &RuntimeImpl::DispatcherImpl::RunIfWaitingForDebugger;
    dispatcherTable_["callFunctionOn"] = &RuntimeImpl::DispatcherImpl::CallFunctionOn;
}

void RuntimeImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    CString method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to RuntimeImpl";

    auto entry = dispatcherTable_.find(method);
    if (entry != dispatcherTable_.end()) {
        (this->*(entry->second))(request);
    } else {
        LOG(ERROR, DEBUGGER) << "unknown method: " << method;
        SendResponse(request, DispatchResponse::Fail("unknown method: " + method), nullptr);
    }
}

void RuntimeImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->Enable();
    SendResponse(request, response, nullptr);
}

void RuntimeImpl::DispatcherImpl::RunIfWaitingForDebugger(const DispatchRequest &request)
{
    DispatchResponse response = runtime_->RunIfWaitingForDebugger();
    SendResponse(request, response, nullptr);
}

void RuntimeImpl::DispatcherImpl::GetProperties(const DispatchRequest &request)
{
    std::unique_ptr<GetPropertiesParams> params = GetPropertiesParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }

    CVector<std::unique_ptr<PropertyDescriptor>> outPropertyDesc;
    std::optional<CVector<std::unique_ptr<InternalPropertyDescriptor>>> outInternalDescs;
    std::optional<CVector<std::unique_ptr<PrivatePropertyDescriptor>>> outPrivateProperties;
    std::optional<std::unique_ptr<ExceptionDetails>> outExceptionDetails;
    DispatchResponse response = runtime_->GetProperties(std::move(params), &outPropertyDesc, &outInternalDescs,
        &outPrivateProperties, &outExceptionDetails);
    if (outExceptionDetails) {
        LOG(WARNING, DEBUGGER) << "GetProperties thrown an exception";
    }
    std::unique_ptr<GetPropertiesReturns> result = std::make_unique<GetPropertiesReturns>(std::move(outPropertyDesc),
        std::move(outInternalDescs),
        std::move(outPrivateProperties),
        std::move(outExceptionDetails));
    SendResponse(request, response, std::move(result));
}

void RuntimeImpl::DispatcherImpl::CallFunctionOn(const DispatchRequest &request)
{
    std::unique_ptr<CallFunctionOnParams> params =
        CallFunctionOnParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }

    std::unique_ptr<RemoteObject> outRemoteObject;
    std::optional<std::unique_ptr<ExceptionDetails>> outExceptionDetails;
    DispatchResponse response = runtime_->CallFunctionOn(std::move(params), &outRemoteObject, &outExceptionDetails);
    if (outExceptionDetails) {
        LOG(WARNING, DEBUGGER) << "CallFunctionOn thrown an exception";
    }
    std::unique_ptr<CallFunctionOnReturns> result = std::make_unique<CallFunctionOnReturns>(std::move(outRemoteObject),
        std::move(outExceptionDetails));
    SendResponse(request, response, std::move(result));
}

DispatchResponse RuntimeImpl::Enable()
{
    auto ecmaVm = const_cast<EcmaVM *>(backend_->GetEcmaVm());
    ecmaVm->SetDebugMode(true);
    backend_->NotifyAllScriptParsed();
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::RunIfWaitingForDebugger()
{
    return DispatchResponse::Create(backend_->Resume());
}

DispatchResponse RuntimeImpl::GetProperties(std::unique_ptr<GetPropertiesParams> params,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc,
    [[maybe_unused]] std::optional<CVector<std::unique_ptr<InternalPropertyDescriptor>>> *outInternalDescs,
    [[maybe_unused]] std::optional<CVector<std::unique_ptr<PrivatePropertyDescriptor>>> *outPrivateProps,
    [[maybe_unused]] std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails)
{
    backend_->GetProperties(params->GetObjectId(),
        params->GetOwnProperties(),
        params->GetAccessPropertiesOnly(),
        outPropertyDesc);
    return DispatchResponse::Ok();
}

DispatchResponse RuntimeImpl::CallFunctionOn(std::unique_ptr<CallFunctionOnParams> params,
    std::unique_ptr<RemoteObject> *outRemoteObject,
    [[maybe_unused]] std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails)
{
    backend_->CallFunctionOn(params->GetFunctionDeclaration(), outRemoteObject);
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
