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
#include "ecmascript/tooling/agent/tracing_impl.h"
#include "ecmascript/tooling/protocol_channel.h"

namespace panda::ecmascript::tooling {
DispatchRequest::DispatchRequest(const std::string &message)
{
    std::unique_ptr<PtJson> json = PtJson::Parse(message);
    if (json == nullptr || !json->IsObject()) {
        code_ = RequestCode::JSON_PARSE_ERROR;
        LOG(ERROR, DEBUGGER) << "json parse error";
        return;
    }

    Result ret;
    int32_t callId;
    ret = json->GetInt("id", &callId);
    if (ret != Result::SUCCESS) {
        code_ = RequestCode::PARSE_ID_ERROR;
        LOG(ERROR, DEBUGGER) << "parse id error";
        return;
    }
    callId_ = callId;

    std::string wholeMethod;
    ret = json->GetString("method", &wholeMethod);
    if (ret != Result::SUCCESS) {
        code_ = RequestCode::PARSE_METHOD_ERROR;
        LOG(ERROR, DEBUGGER) << "parse method error";
        return;
    }
    std::string::size_type length = wholeMethod.length();
    std::string::size_type indexPoint;
    indexPoint = wholeMethod.find_first_of('.', 0);
    if (indexPoint == std::string::npos || indexPoint == 0 || indexPoint == length - 1) {
        code_ = RequestCode::METHOD_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "method format error: " << wholeMethod;
        return;
    }
    domain_ = wholeMethod.substr(0, indexPoint);
    method_ = wholeMethod.substr(indexPoint + 1, length);

    LOG(DEBUG, DEBUGGER) << "id: " << callId_;
    LOG(DEBUG, DEBUGGER) << "domain: " << domain_;
    LOG(DEBUG, DEBUGGER) << "method: " << method_;

    std::unique_ptr<PtJson> params;
    ret = json->GetObject("params", &params);
    if (ret == Result::NOT_EXIST) {
        return;
    }
    if (ret == Result::TYPE_ERROR) {
        code_ = RequestCode::PARAMS_FORMAT_ERROR;
        LOG(ERROR, DEBUGGER) << "params format error";
        return;
    }
    params_ = std::move(params);
}

DispatchRequest::~DispatchRequest()
{
    params_->ReleaseRoot();
}

DispatchResponse DispatchResponse::Create(ResponseCode code, const std::string &msg)
{
    DispatchResponse response;
    response.code_ = code;
    response.errorMsg_ = msg;
    return response;
}

DispatchResponse DispatchResponse::Create(std::optional<std::string> error)
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

DispatchResponse DispatchResponse::Fail(const std::string &message)
{
    DispatchResponse response;
    response.code_ = ResponseCode::NOK;
    response.errorMsg_ = message;
    return response;
}

void DispatcherBase::SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                                  const PtBaseReturns &result)
{
    if (channel_ != nullptr) {
        channel_->SendResponse(request, response, result);
    }
}

Dispatcher::Dispatcher(const EcmaVM *vm, ProtocolChannel *channel)
{
    // profiler
    auto profiler = std::make_unique<ProfilerImpl>(vm, channel);
    auto heapProfiler = std::make_unique<HeapProfilerImpl>(vm, channel);
    auto tracing = std::make_unique<TracingImpl>(vm, channel);
    dispatchers_["Profiler"] =
        std::make_unique<ProfilerImpl::DispatcherImpl>(channel, std::move(profiler));
    dispatchers_["HeapProfiler"] =
        std::make_unique<HeapProfilerImpl::DispatcherImpl>(channel, std::move(heapProfiler));
    dispatchers_["Tracing"] =
        std::make_unique<TracingImpl::DispatcherImpl>(channel, std::move(tracing));

    // debugger
    auto runtime = std::make_unique<RuntimeImpl>(vm, channel);
    auto debugger = std::make_unique<DebuggerImpl>(vm, channel, runtime.get());
    dispatchers_["Runtime"] =
        std::make_unique<RuntimeImpl::DispatcherImpl>(channel, std::move(runtime));
    dispatchers_["Debugger"] =
        std::make_unique<DebuggerImpl::DispatcherImpl>(channel, std::move(debugger));
}

void Dispatcher::Dispatch(const DispatchRequest &request)
{
    if (!request.IsValid()) {
        LOG(ERROR, DEBUGGER) << "Unknown request";
        return;
    }
    const std::string &domain = request.GetDomain();
    auto dispatcher = dispatchers_.find(domain);
    if (dispatcher != dispatchers_.end()) {
        dispatcher->second->Dispatch(request);
    } else {
        LOG(ERROR, DEBUGGER) << "unknown domain: " << domain;
    }
}
}  // namespace panda::ecmascript::tooling
