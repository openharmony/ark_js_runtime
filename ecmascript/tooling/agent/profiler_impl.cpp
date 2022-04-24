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
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/front_end.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
ProfilerImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<ProfilerImpl> profiler)
    : DispatcherBase(frontend), profiler_(std::move(profiler))
{
    dispatcherTable_["disable"] = &ProfilerImpl::DispatcherImpl::Disable;
    dispatcherTable_["enable"] = &ProfilerImpl::DispatcherImpl::Enable;
    dispatcherTable_["start"] = &ProfilerImpl::DispatcherImpl::Start;
    dispatcherTable_["stop"] = &ProfilerImpl::DispatcherImpl::Stop;
    dispatcherTable_["getBestEffortCoverage"] = &ProfilerImpl::DispatcherImpl::GetBestEffortCoverage;
    dispatcherTable_["stopPreciseCoverage"] = &ProfilerImpl::DispatcherImpl::StopPreciseCoverage;
    dispatcherTable_["takePreciseCoverage"] = &ProfilerImpl::DispatcherImpl::TakePreciseCoverage;
    dispatcherTable_["startPreciseCoverage"] = &ProfilerImpl::DispatcherImpl::StartPreciseCoverage;
    dispatcherTable_["startTypeProfile"] = &ProfilerImpl::DispatcherImpl::StartTypeProfile;
    dispatcherTable_["stopTypeProfile"] = &ProfilerImpl::DispatcherImpl::StopTypeProfile;
    dispatcherTable_["takeTypeProfile"] = &ProfilerImpl::DispatcherImpl::TakeTypeProfile;
}

void ProfilerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    CString method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to ProfilerImpl";
    auto entry = dispatcherTable_.find(method);
    if (entry != dispatcherTable_.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method), nullptr);
    }
}

void ProfilerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Disable();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Enable();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Start(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Start();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Stop(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Stop();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::GetBestEffortCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->GetBestEffortCoverage();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::StopPreciseCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StopPreciseCoverage();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::TakePreciseCoverage(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->TakePreciseCoverage();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::StartPreciseCoverage(const DispatchRequest &request)
{
    std::unique_ptr<StartPreciseCoverageParam> params =
        StartPreciseCoverageParam::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Profiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = profiler_->StartPreciseCoverage(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::StartTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StartTypeProfile();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::StopTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->StopTypeProfile();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::TakeTypeProfile(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->TakeTypeProfile();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
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
    LOG(ERROR, DEBUGGER) << "Start not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Stop()
{
    LOG(ERROR, DEBUGGER) << "Stop not support now.";
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

