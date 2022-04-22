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


namespace panda::ecmascript::tooling {
HeapProfilerImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<HeapProfilerImpl> heapprofiler)
    : DispatcherBase(frontend), heapprofiler_(std::move(heapprofiler))
{
    dispatcherTable_["addInspectedHeapObject"] = &HeapProfilerImpl::DispatcherImpl::AddInspectedHeapObject;
    dispatcherTable_["collectGarbage"] = &HeapProfilerImpl::DispatcherImpl::CollectGarbage;
    dispatcherTable_["enable"] = &HeapProfilerImpl::DispatcherImpl::Enable;
    dispatcherTable_["disable"] = &HeapProfilerImpl::DispatcherImpl::Disable;
    dispatcherTable_["getHeapObjectId"] = &HeapProfilerImpl::DispatcherImpl::GetHeapObjectId;
    dispatcherTable_["getObjectByHeapObjectId"] = &HeapProfilerImpl::DispatcherImpl::GetObjectByHeapObjectId;
    dispatcherTable_["getSamplingProfile"] = &HeapProfilerImpl::DispatcherImpl::GetSamplingProfile;
    dispatcherTable_["startSampling"] = &HeapProfilerImpl::DispatcherImpl::StartSampling;
    dispatcherTable_["startTrackingHeapObjects"] = &HeapProfilerImpl::DispatcherImpl::StartTrackingHeapObjects;
    dispatcherTable_["stopSampling"] = &HeapProfilerImpl::DispatcherImpl::StopSampling;
    dispatcherTable_["stopTrackingHeapObjects"] = &HeapProfilerImpl::DispatcherImpl::StopTrackingHeapObjects;
    dispatcherTable_["takeHeapSnapshot"] = &HeapProfilerImpl::DispatcherImpl::TakeHeapSnapshot;
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

void HeapProfilerImpl::DispatcherImpl::AddInspectedHeapObject(const DispatchRequest &request)
{
    std::unique_ptr<AddInspectedHeapObjectParams> params =
        AddInspectedHeapObjectParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = heapprofiler_->AddInspectedHeapObject(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void HeapProfilerImpl::DispatcherImpl::CollectGarbage(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->CollectGarbage();
    SendResponse(request, response, nullptr);
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

void HeapProfilerImpl::DispatcherImpl::GetHeapObjectId(const DispatchRequest &request)
{
    std::unique_ptr<GetHeapObjectIdParams> params =
        GetHeapObjectIdParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }

    HeapSnapshotObjectId objectId;
    DispatchResponse response = heapprofiler_->GetHeapObjectId(std::move(params), &objectId);
    std::unique_ptr<GetHeapObjectIdReturns> result = std::make_unique<GetHeapObjectIdReturns>(std::move(objectId));
    SendResponse(request, response, std::move(result));
}

void HeapProfilerImpl::DispatcherImpl::GetObjectByHeapObjectId(const DispatchRequest &request)
{
    std::unique_ptr<GetObjectByHeapObjectIdParams> params =
        GetObjectByHeapObjectIdParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }

    std::unique_ptr<RemoteObject> remoteObjectResult;
    DispatchResponse response = heapprofiler_->GetObjectByHeapObjectId(std::move(params), &remoteObjectResult);
    std::unique_ptr<GetObjectByHeapObjectIdReturns> result =
        std::make_unique<GetObjectByHeapObjectIdReturns>(std::move(remoteObjectResult));
    SendResponse(request, response, std::move(result));
}

void HeapProfilerImpl::DispatcherImpl::GetSamplingProfile(const DispatchRequest &request)
{
    std::unique_ptr<SamplingHeapProfile> profile;
    DispatchResponse response = heapprofiler_->GetSamplingProfile(&profile);
    // The return value type of GetSamplingProfile is the same as of StopSampling.
    std::unique_ptr<StopSamplingReturns> result = std::make_unique<StopSamplingReturns>(std::move(profile));
    SendResponse(request, response, std::move(result));
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

void HeapProfilerImpl::DispatcherImpl::TakeHeapSnapshot(const DispatchRequest &request)
{
    std::unique_ptr<StopTrackingHeapObjectsParams> params =
        StopTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("HeapProfiler got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = heapprofiler_->TakeHeapSnapshot(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

DispatchResponse HeapProfilerImpl::AddInspectedHeapObject(
    [[maybe_unused]] std::unique_ptr<AddInspectedHeapObjectParams> params)
{
    LOG(ERROR, DEBUGGER) << "AddInspectedHeapObject not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::CollectGarbage()
{
    LOG(ERROR, DEBUGGER) << "CollectGarbage not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::Enable()
{
    LOG(ERROR, DEBUGGER) << "Enable not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::Disable()
{
    LOG(ERROR, DEBUGGER) << "Disable not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::GetHeapObjectId([[maybe_unused]] std::unique_ptr<GetHeapObjectIdParams> params,
    HeapSnapshotObjectId *objectId)
{
    ASSERT(objectId != nullptr);
    *objectId = 0;
    LOG(ERROR, DEBUGGER) << "GetHeapObjectId not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::GetObjectByHeapObjectId(
    [[maybe_unused]] std::unique_ptr<GetObjectByHeapObjectIdParams> params,
    [[maybe_unused]] std::unique_ptr<RemoteObject> *remoteObjectResult)
{
    LOG(ERROR, DEBUGGER) << "GetObjectByHeapObjectId not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::GetSamplingProfile([[maybe_unused]]std::unique_ptr<SamplingHeapProfile> *profile)
{
    LOG(ERROR, DEBUGGER) << "GetSamplingProfile not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StartSampling([[maybe_unused]]std::unique_ptr<StartSamplingParams> params)
{
    LOG(ERROR, DEBUGGER) << "StartSampling not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StartTrackingHeapObjects(
    [[maybe_unused]] std::unique_ptr<StartTrackingHeapObjectsParams> params)
{
    auto ecmaVm = (panda::EcmaVM *)static_cast<ProtocolHandler *>(frontend_)->GetEcmaVM();
    bool result = panda::DFXJSNApi::StartHeapTracking(ecmaVm, 0.05, true);
    if (result == true) {
        return DispatchResponse::Ok();
    } else {
        return DispatchResponse::Fail("StartHeapTracking fail");
    }
}


DispatchResponse HeapProfilerImpl::StopSampling([[maybe_unused]]std::unique_ptr<SamplingHeapProfile> *profile)
{
    LOG(ERROR, DEBUGGER) << "StopSampling not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::StopTrackingHeapObjects(
    [[maybe_unused]] std::unique_ptr<StopTrackingHeapObjectsParams> params)
{
    return DispatchResponse::Ok();
}

DispatchResponse HeapProfilerImpl::TakeHeapSnapshot(
    [[maybe_unused]] std::unique_ptr<StopTrackingHeapObjectsParams> params)
{
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
