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

#include "ecmascript/tooling/agent/heapprofiler_impl.h"

#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/front_end.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
HeapProfilerImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<HeapProfilerImpl> heapprofiler)
    : DispatcherBase(frontend), heapprofiler_(std::move(heapprofiler))
{
    dispatcherTable_["enable"] = &HeapProfilerImpl::DispatcherImpl::Enable;
    dispatcherTable_["disable"] = &HeapProfilerImpl::DispatcherImpl::Disable;
    dispatcherTable_["startSampling"] = &HeapProfilerImpl::DispatcherImpl::StartSampling;
    dispatcherTable_["startTrackingHeapObjects"] = &HeapProfilerImpl::DispatcherImpl::StartTrackingHeapObjects;
    dispatcherTable_["stopSampling"] = &HeapProfilerImpl::DispatcherImpl::StopSampling;
    dispatcherTable_["stopTrackingHeapObjects"] = &HeapProfilerImpl::DispatcherImpl::StopTrackingHeapObjects;
}

void HeapProfilerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    CString method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to HeapProfilerImpl";
    auto entry = dispatcherTable_.find(method);
    if (entry != dispatcherTable_.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method), nullptr);
    }
}

void HeapProfilerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->Enable();
    SendResponse(request, response, nullptr);
}

void HeapProfilerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->Disable();
    SendResponse(request, response, nullptr);
}


void HeapProfilerImpl::DispatcherImpl::StartSampling(const DispatchRequest &request)
{
    std::unique_ptr<StartSamplingParams> params =
        StartSamplingParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = heapprofiler_->StartSampling(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void HeapProfilerImpl::DispatcherImpl::StartTrackingHeapObjects(const DispatchRequest &request)
{
    std::unique_ptr<StartTrackingHeapObjectsParams> params =
        StartTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = heapprofiler_->StartTrackingHeapObjects(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}


void HeapProfilerImpl::DispatcherImpl::StopSampling(const DispatchRequest &request)
{
    std::unique_ptr<SamplingHeapProfile> profile;
    DispatchResponse response = heapprofiler_->StopSampling(&profile);
    std::unique_ptr<StopSamplingReturns> result = std::make_unique<StopSamplingReturns>(std::move(profile));
    SendResponse(request, response, std::move(result));
}

void HeapProfilerImpl::DispatcherImpl::StopTrackingHeapObjects(const DispatchRequest &request)
{
    std::unique_ptr<StopTrackingHeapObjectsParams> params =
        StopTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = heapprofiler_->StopTrackingHeapObjects(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

DispatchResponse HeapProfilerImpl::Enable()
{
    return DispatchResponse::Ok();
}


DispatchResponse HeapProfilerImpl::Disable()
{
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StartSampling([[maybe_unused]]std::unique_ptr<StartSamplingParams> params)
{
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StartTrackingHeapObjects(
    [[maybe_unused]] std::unique_ptr<StartTrackingHeapObjectsParams> params)
{
    return DispatchResponse::Ok();
}


DispatchResponse HeapProfilerImpl::StopSampling([[maybe_unused]]std::unique_ptr<SamplingHeapProfile> *profile)
{
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StopTrackingHeapObjects(
    [[maybe_unused]] std::unique_ptr<StopTrackingHeapObjectsParams> params)
{
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
