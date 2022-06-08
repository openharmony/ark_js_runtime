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

#include "ecmascript/tooling/agent/profiler_impl.h"

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/protocol_channel.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
void ProfilerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    static std::unordered_map<std::string, AgentHandler> dispatcherTable {
        { "disable", &ProfilerImpl::DispatcherImpl::Disable },
        { "enable", &ProfilerImpl::DispatcherImpl::Enable },
        { "start", &ProfilerImpl::DispatcherImpl::Start },
        { "stop", &ProfilerImpl::DispatcherImpl::Stop },
        { "SetSamplingInterval", &ProfilerImpl::DispatcherImpl::SetSamplingInterval },
        { "getBestEffortCoverage", &ProfilerImpl::DispatcherImpl::GetBestEffortCoverage },
        { "stopPreciseCoverage", &ProfilerImpl::DispatcherImpl::StopPreciseCoverage },
        { "takePreciseCoverage", &ProfilerImpl::DispatcherImpl::TakePreciseCoverage },
        { "startPreciseCoverage", &ProfilerImpl::DispatcherImpl::StartPreciseCoverage },
        { "startTypeProfile", &ProfilerImpl::DispatcherImpl::StartTypeProfile },
        { "stopTypeProfile", &ProfilerImpl::DispatcherImpl::StopTypeProfile },
        { "takeTypeProfile", &ProfilerImpl::DispatcherImpl::TakeTypeProfile }
    };

    const std::string &method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to ProfilerImpl";
    auto entry = dispatcherTable.find(method);
    if (entry != dispatcherTable.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method));
    }
}

void ProfilerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Disable();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Enable();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::Start(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Start();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::Stop(const DispatchRequest &request)
{
    std::unique_ptr<Profile> profile;
    DispatchResponse response = profiler_->Stop(&profile);
    StopReturns result(std::move(profile));
    SendResponse(request, response, result);
}

void ProfilerImpl::DispatcherImpl::SetSamplingInterval(const DispatchRequest &request)
{
    std::unique_ptr<SetSamplingIntervalParams> params =
        SetSamplingIntervalParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = profiler_->SetSamplingInterval(std::move(params));
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::GetBestEffortCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->GetBestEffortCoverage();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::StopPreciseCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StopPreciseCoverage();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::TakePreciseCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->TakePreciseCoverage();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::StartPreciseCoverage(const DispatchRequest &request)
{
    std::unique_ptr<StartPreciseCoverageParam> params =
        StartPreciseCoverageParam::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = profiler_->StartPreciseCoverage(std::move(params));
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::StartTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StartTypeProfile();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::StopTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StopTypeProfile();
    SendResponse(request, response);
}

void ProfilerImpl::DispatcherImpl::TakeTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->TakeTypeProfile();
    SendResponse(request, response);
}

bool ProfilerImpl::Frontend::AllowNotify() const
{
    return channel_ != nullptr;
}

void ProfilerImpl::Frontend::ConsoleProfileFinished()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::ConsoleProfileFinished consoleProfileFinished;
    channel_->SendNotification(consoleProfileFinished);
}

void ProfilerImpl::Frontend::ConsoleProfileStarted()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::ConsoleProfileStarted consoleProfileStarted;
    channel_->SendNotification(consoleProfileStarted);
}

void ProfilerImpl::Frontend::PreciseCoverageDeltaUpdate()
{
    if (!AllowNotify()) {
        return;
    }

    tooling::PreciseCoverageDeltaUpdate preciseCoverageDeltaUpdate;
    channel_->SendNotification(preciseCoverageDeltaUpdate);
}

DispatchResponse ProfilerImpl::Disable()
{
    LOG(ERROR, DEBUGGER) << "Disable not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Enable()
{
    LOG(ERROR, DEBUGGER) << "Enable not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Start()
{
    panda::DFXJSNApi::StartCpuProfilerForInfo(vm_);
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Stop(std::unique_ptr<Profile> *profile)
{
    auto profileInfo = panda::DFXJSNApi::StopCpuProfilerForInfo();
    if (profileInfo == nullptr) {
        LOG(ERROR, DEBUGGER) << "Transfer DFXJSNApi::StopCpuProfilerImpl is failure";
        return DispatchResponse::Fail("Stop is failure");
    }
    *profile = Profile::FromProfileInfo(*profileInfo);
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::SetSamplingInterval(std::unique_ptr<SetSamplingIntervalParams> params)
{
    panda::DFXJSNApi::SetCpuSamplingInterval(params->GetInterval());
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::GetBestEffortCoverage()
{
    LOG(ERROR, DEBUGGER) << "GetBestEffortCoverage not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::StopPreciseCoverage()
{
    LOG(ERROR, DEBUGGER) << "StopPreciseCoverage not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::TakePreciseCoverage()
{
    LOG(ERROR, DEBUGGER) << "TakePreciseCoverage not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::StartPreciseCoverage([[maybe_unused]] std::unique_ptr<StartPreciseCoverageParam> params)
{
    LOG(ERROR, DEBUGGER) << "StartPreciseCoverage not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::StartTypeProfile()
{
    LOG(ERROR, DEBUGGER) << "StartTypeProfile not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::StopTypeProfile()
{
    LOG(ERROR, DEBUGGER) << "StopTypeProfile not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::TakeTypeProfile()
{
    LOG(ERROR, DEBUGGER) << "TakeTypeProfile not support now.";
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
