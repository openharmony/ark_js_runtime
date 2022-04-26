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

#include "ecmascript/tooling/dispatcher.h"

#include "libpandabase/utils/logger.h"

#include "ecmascript/tooling/agent/debugger_impl.h"
#include "ecmascript/tooling/agent/runtime_impl.h"
#include "ecmascript/tooling/agent/heapprofiler_impl.h"
#include "ecmascript/tooling/agent/profiler_impl.h"
#include "ecmascript/tooling/front_end.h"

namespace panda::ecmascript::tooling {
DispatchRequest::DispatchRequest(const EcmaVM *ecmaVm, const CString &message) : ecmaVm_(ecmaVm)
{
    Local<JSValueRef> msgValue = JSON::Parse(ecmaVm, StringRef::NewFromUtf8(ecmaVm, message.c_str()));
    if (msgValue->IsException()) {
        DebuggerApi::ClearException(ecmaVm);
        LOG(ERROR, DEBUGGER) << "json parse throw exception";
        return;
    }
    if (!msgValue->IsObject()) {
        code_ = RequestCode::JSON_PARSE_ERROR;
        LOG(ERROR, DEBUGGER) << "json parse error";
        return;
    }
    ObjectRef *msgObj = ObjectRef::Cast(*msgValue);

    Local<StringRef> idStr = StringRef::NewFromUtf8(ecmaVm, "id");
    Local<JSValueRef> idResult = msgObj->Get(ecmaVm, idStr);
    if (idResult.IsEmpty()) {
        code_ = RequestCode::PARSE_ID_ERROR;
        LOG(ERROR, DEBUGGER) << "parse id error";
        return;
    }
    if (!idResult->IsNumber()) {
        code_ = RequestCode::ID_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "id format error";
        return;
    }
    callId_ = static_cast<int32_t>(Local<NumberRef>(idResult)->Value());

    Local<StringRef> methodStr = StringRef::NewFromUtf8(ecmaVm, "method");
    Local<JSValueRef> methodResult = msgObj->Get(ecmaVm, methodStr);
    if (methodResult.IsEmpty()) {
        code_ = RequestCode::PARSE_METHOD_ERROR;
        LOG(ERROR, DEBUGGER) << "parse method error";
        return;
    }
    if (!methodResult->IsString()) {
        code_ = RequestCode::METHOD_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "method format error";
        return;
    }
    CString wholeMethod = DebuggerApi::ToCString(methodResult);
    CString::size_type length = wholeMethod.length();
    CString::size_type indexPoint;
    indexPoint = wholeMethod.find_first_of('.', 0);
    if (indexPoint == CString::npos || indexPoint == 0 || indexPoint == length - 1) {
        code_ = RequestCode::METHOD_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "method format error: " << wholeMethod;
        return;
    }
    domain_ = wholeMethod.substr(0, indexPoint);
    method_ = wholeMethod.substr(indexPoint + 1, length);

    LOG(DEBUG, DEBUGGER) << "id: " << callId_;
    LOG(DEBUG, DEBUGGER) << "domain: " << domain_;
    LOG(DEBUG, DEBUGGER) << "method: " << method_;

    Local<StringRef> paramsStr = StringRef::NewFromUtf8(ecmaVm, "params");
    Local<JSValueRef> paramsValue = msgObj->Get(ecmaVm, paramsStr);
    if (paramsValue.IsEmpty()) {
        return;
    }
    if (!paramsValue->IsObject()) {
        code_ = RequestCode::PARAMS_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "params format error";
        return;
    }
    params_ = paramsValue;
}

DispatchResponse DispatchResponse::Create(ResponseCode code, const CString &msg)
{
    DispatchResponse response;
    response.code_ = code;
    response.errorMsg_ = msg;
    return response;
}

DispatchResponse DispatchResponse::Create(std::optional<CString> error)
{
    DispatchResponse response;
    if (error.has_value()) {
        response.code_ = ResponseCode::NOK;
        response.errorMsg_ = error.value();
    }
    return response;
}

DispatchResponse DispatchResponse::Ok()
{
    return DispatchResponse();
}

DispatchResponse DispatchResponse::Fail(const CString &message)
{
    DispatchResponse response;
    response.code_ = ResponseCode::NOK;
    response.errorMsg_ = message;
    return response;
}

void DispatcherBase::SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                                  std::unique_ptr<PtBaseReturns> result)
{
    if (frontend_ != nullptr) {
        frontend_->SendResponse(request, response, std::move(result));
    }
}

Dispatcher::Dispatcher(FrontEnd *front)
{
    std::unique_ptr<JSBackend> backend = std::make_unique<JSBackend>(front);
    dispatchers_["Runtime"] =
        std::make_unique<RuntimeImpl::DispatcherImpl>(front, std::make_unique<RuntimeImpl>(backend.get()));
    dispatchers_["HeapProfiler"] =
        std::make_unique<HeapProfilerImpl::DispatcherImpl>(front, std::make_unique<HeapProfilerImpl>(front));
    dispatchers_["Profiler"] =
        std::make_unique<ProfilerImpl::DispatcherImpl>(front, std::make_unique<ProfilerImpl>());
    dispatchers_["Debugger"] =
        std::make_unique<DebuggerImpl::DispatcherImpl>(front, std::make_unique<DebuggerImpl>(std::move(backend)));
}

void Dispatcher::Dispatch(const DispatchRequest &request)
{
    if (!request.IsValid()) {
        LOG(ERROR, DEBUGGER) << "Unknown request";
        return;
    }
    CString domain = request.GetDomain();
    auto dispatcher = dispatchers_.find(domain);
    if (dispatcher != dispatchers_.end()) {
        dispatcher->second->Dispatch(request);
    } else {
        LOG(ERROR, DEBUGGER) << "unknown domain: " << domain;
    }
}
}  // namespace panda::ecmascript::tooling
