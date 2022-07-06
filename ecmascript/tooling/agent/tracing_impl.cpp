/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/tooling/agent/tracing_impl.h"

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/protocol_channel.h"

namespace panda::ecmascript::tooling {
void TracingImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    static std::unordered_map<std::string, AgentHandler> dispatcherTable {
        { "end", &TracingImpl::DispatcherImpl::End },
        { "getCategories", &TracingImpl::DispatcherImpl::GetCategories },
        { "recordClockSyncMarker", &TracingImpl::DispatcherImpl::RecordClockSyncMarker },
        { "requestMemoryDump", &TracingImpl::DispatcherImpl::RequestMemoryDump },
        { "start", &TracingImpl::DispatcherImpl::Start }
    };

    const std::string &method = request.GetMethod();
    LOG_DEBUGGER(DEBUG) << "dispatch [" << method << "] to TracingImpl";
    auto entry = dispatcherTable.find(method);
    if (entry != dispatcherTable.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method));
    }
}

void TracingImpl::DispatcherImpl::End(const DispatchRequest &request)
{
    DispatchResponse response = tracing_->End();
    SendResponse(request, response);
}

void TracingImpl::DispatcherImpl::GetCategories(const DispatchRequest &request)
{
    std::vector<std::string> categories;
    DispatchResponse response = tracing_->GetCategories(categories);
    SendResponse(request, response);
}

void TracingImpl::DispatcherImpl::RecordClockSyncMarker(const DispatchRequest &request)
{
    std::string syncId;
    DispatchResponse response = tracing_->RecordClockSyncMarker(syncId);
    SendResponse(request, response);
}

void TracingImpl::DispatcherImpl::RequestMemoryDump(const DispatchRequest &request)
{
    std::unique_ptr<RequestMemoryDumpParams> params =
        RequestMemoryDumpParams::Create(request.GetParams());
    std::string dumpGuid;
    bool success = false;
    DispatchResponse response = tracing_->RequestMemoryDump(std::move(params), dumpGuid, success);
    SendResponse(request, response);
}

void TracingImpl::DispatcherImpl::Start(const DispatchRequest &request)
{
    std::unique_ptr<StartParams> params =
        StartParams::Create(request.GetParams());
    DispatchResponse response = tracing_->Start(std::move(params));
    SendResponse(request, response);
}

bool TracingImpl::Frontend::AllowNotify() const
{
    return channel_ != nullptr;
}

void TracingImpl::Frontend::BufferUsage()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::BufferUsage bufferUsage;
    channel_->SendNotification(bufferUsage);
}

void TracingImpl::Frontend::DataCollected()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::DataCollected dataCollected;
    channel_->SendNotification(dataCollected);
}

void TracingImpl::Frontend::TracingComplete()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::TracingComplete tracingComplete;
    channel_->SendNotification(tracingComplete);
}

DispatchResponse TracingImpl::End()
{
    return DispatchResponse::Fail("End not support now.");
}

DispatchResponse TracingImpl::GetCategories([[maybe_unused]] std::vector<std::string> categories)
{
    return DispatchResponse::Fail("GetCategories not support now.");
}

DispatchResponse TracingImpl::RecordClockSyncMarker([[maybe_unused]] std::string syncId)
{
    return DispatchResponse::Fail("RecordClockSyncMarker not support now.");
}

DispatchResponse TracingImpl::RequestMemoryDump([[maybe_unused]] std::unique_ptr<RequestMemoryDumpParams> params,
                                                [[maybe_unused]] std::string dumpGuid, [[maybe_unused]] bool success)
{
    return DispatchResponse::Fail("RequestMemoryDump not support now.");
}

DispatchResponse TracingImpl::Start([[maybe_unused]] std::unique_ptr<StartParams> params)
{
    return DispatchResponse::Fail("Start not support now.");
}
}  // namespace panda::ecmascript::tooling