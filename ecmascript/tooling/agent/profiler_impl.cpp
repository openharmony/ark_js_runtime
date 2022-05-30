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
    std::unique_ptr<Profile> profile;
    DispatchResponse response = profiler_->Stop(profile);
    std::unique_ptr<StopReturns> result = std::make_unique<StopReturns>(std::move(profile));
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
    auto ecmaVm = const_cast<EcmaVM *>(backend_->GetEcmaVm());
    panda::DFXJSNApi::StartCpuProfilerForInfo(ecmaVm);
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Stop(std::unique_ptr<Profile> &profile)
{
    auto profileInfo = panda::DFXJSNApi::StopCpuProfilerForInfo();
    if (profileInfo == nullptr) {
        LOG(ERROR, DEBUGGER) << "Transfer DFXJSNApi::StopCpuProfilerImpl is failure";
        return DispatchResponse::Fail("Stop is failure");
    }
    profile = FromCpuProfiler(profileInfo);
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

std::unique_ptr<RuntimeCallFrame> ProfilerImpl::FromCpuFrameInfo(const std::unique_ptr<FrameInfo> &cpuFrameInfo)
{
    auto runtimeCallFrame = std::make_unique<RuntimeCallFrame>();
    runtimeCallFrame->SetFunctionName(cpuFrameInfo->functionName.c_str());
    runtimeCallFrame->SetScriptId(std::to_string(cpuFrameInfo->scriptId).c_str());
    runtimeCallFrame->SetColumnNumber(cpuFrameInfo->columnNumber);
    runtimeCallFrame->SetLineNumber(cpuFrameInfo->lineNumber);
    runtimeCallFrame->SetUrl(cpuFrameInfo->url.c_str());
    return runtimeCallFrame;
}

std::unique_ptr<ProfileNode> ProfilerImpl::FromCpuProfileNode(const std::unique_ptr<CpuProfileNode> &cpuProfileNode)
{
    auto profileNode = std::make_unique<ProfileNode>();
    profileNode->SetId(cpuProfileNode->id);

    size_t childrenLen = cpuProfileNode->children.size();
    CVector<int32_t> tmpChildren;
    tmpChildren.reserve(childrenLen);
    for (uint32_t i = 0; i < childrenLen; ++i) {
        tmpChildren.push_back(cpuProfileNode->children[i]);
    }
    profileNode->SetChildren(tmpChildren);
    auto frameInfo = std::make_unique<FrameInfo>(cpuProfileNode->codeEntry);
    profileNode->SetCallFrame(FromCpuFrameInfo(frameInfo));
    return profileNode;
}

std::unique_ptr<Profile> ProfilerImpl::FromCpuProfiler(const std::unique_ptr<ProfileInfo> &profileInfo)
{
    auto profile = std::make_unique<Profile>();
    profile->SetStartTime(static_cast<int64_t>(profileInfo->startTime));
    profile->SetEndTime(static_cast<int64_t>(profileInfo->stopTime));
    size_t samplesLen = profileInfo->samples.size();
    CVector<int32_t> tmpSamples;
    tmpSamples.reserve(samplesLen);
    for (uint32_t i = 0; i < samplesLen; ++i) {
        tmpSamples.push_back(profileInfo->samples[i]);
    }
    profile->SetSamples(tmpSamples);

    size_t timeDeltasLen = profileInfo->timeDeltas.size();
    CVector<int32_t> tmpTimeDeltas;
    tmpTimeDeltas.reserve(timeDeltasLen);
    for (uint32_t i = 0; i < timeDeltasLen; ++i) {
        tmpTimeDeltas.push_back(profileInfo->timeDeltas[i]);
    }
    profile->SetTimeDeltas(tmpTimeDeltas);
    
    CVector<std::unique_ptr<ProfileNode>> profileNode;
    size_t nodesLen = profileInfo->nodes.size();
    for (uint32_t i = 0; i < nodesLen; ++i) {
        auto cpuProfileNode = std::make_unique<CpuProfileNode>(profileInfo->nodes[i]);
        profileNode.push_back(FromCpuProfileNode(cpuProfileNode));
    }
    profile->SetNodes(std::move(profileNode));
    return profile;
}
}  // namespace panda::ecmascript::tooling
