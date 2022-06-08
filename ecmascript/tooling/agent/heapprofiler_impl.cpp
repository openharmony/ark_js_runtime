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
void HeapProfilerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    static std::unordered_map<std::string, AgentHandler> dispatcherTable {
        { "addInspectedHeapObject", &HeapProfilerImpl::DispatcherImpl::AddInspectedHeapObject },
        { "collectGarbage", &HeapProfilerImpl::DispatcherImpl::CollectGarbage },
        { "enable", &HeapProfilerImpl::DispatcherImpl::Enable },
        { "disable", &HeapProfilerImpl::DispatcherImpl::Disable },
        { "getHeapObjectId", &HeapProfilerImpl::DispatcherImpl::GetHeapObjectId },
        { "getObjectByHeapObjectId", &HeapProfilerImpl::DispatcherImpl::GetObjectByHeapObjectId },
        { "getSamplingProfile", &HeapProfilerImpl::DispatcherImpl::GetSamplingProfile },
        { "startSampling", &HeapProfilerImpl::DispatcherImpl::StartSampling },
        { "startTrackingHeapObjects", &HeapProfilerImpl::DispatcherImpl::StartTrackingHeapObjects },
        { "stopSampling", &HeapProfilerImpl::DispatcherImpl::StopSampling },
        { "stopTrackingHeapObjects", &HeapProfilerImpl::DispatcherImpl::StopTrackingHeapObjects },
        { "takeHeapSnapshot", &HeapProfilerImpl::DispatcherImpl::TakeHeapSnapshot }
    };

    const std::string &method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to HeapProfilerImpl";
    auto entry = dispatcherTable.find(method);
    if (entry != dispatcherTable.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method));
    }
}

void HeapProfilerImpl::DispatcherImpl::AddInspectedHeapObject(const DispatchRequest &request)
{
    std::unique_ptr<AddInspectedHeapObjectParams> params =
        AddInspectedHeapObjectParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = heapprofiler_->AddInspectedHeapObject(std::move(params));
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::CollectGarbage(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->CollectGarbage();
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->Enable();
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = heapprofiler_->Disable();
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::GetHeapObjectId(const DispatchRequest &request)
{
    std::unique_ptr<GetHeapObjectIdParams> params =
        GetHeapObjectIdParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    HeapSnapshotObjectId objectId;
    DispatchResponse response = heapprofiler_->GetHeapObjectId(std::move(params), &objectId);
    GetHeapObjectIdReturns result(std::move(objectId));
    SendResponse(request, response, result);
}

void HeapProfilerImpl::DispatcherImpl::GetObjectByHeapObjectId(const DispatchRequest &request)
{
    std::unique_ptr<GetObjectByHeapObjectIdParams> params =
        GetObjectByHeapObjectIdParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    std::unique_ptr<RemoteObject> remoteObjectResult;
    DispatchResponse response = heapprofiler_->GetObjectByHeapObjectId(std::move(params), &remoteObjectResult);
    GetObjectByHeapObjectIdReturns result(std::move(remoteObjectResult));
    SendResponse(request, response, result);
}

void HeapProfilerImpl::DispatcherImpl::GetSamplingProfile(const DispatchRequest &request)
{
    std::unique_ptr<SamplingHeapProfile> profile;
    DispatchResponse response = heapprofiler_->GetSamplingProfile(&profile);
    // The return value type of GetSamplingProfile is the same as of StopSampling.
    StopSamplingReturns result(std::move(profile));
    SendResponse(request, response, result);
}

void HeapProfilerImpl::DispatcherImpl::StartSampling(const DispatchRequest &request)
{
    std::unique_ptr<StartSamplingParams> params =
        StartSamplingParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = heapprofiler_->StartSampling(std::move(params));
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::StartTrackingHeapObjects(const DispatchRequest &request)
{
    std::unique_ptr<StartTrackingHeapObjectsParams> params =
        StartTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = heapprofiler_->StartTrackingHeapObjects(std::move(params));
    SendResponse(request, response);
}


void HeapProfilerImpl::DispatcherImpl::StopSampling(const DispatchRequest &request)
{
    std::unique_ptr<SamplingHeapProfile> profile;
    DispatchResponse response = heapprofiler_->StopSampling(&profile);
    StopSamplingReturns result(std::move(profile));
    SendResponse(request, response, result);
}

void HeapProfilerImpl::DispatcherImpl::StopTrackingHeapObjects(const DispatchRequest &request)
{
    std::unique_ptr<StopTrackingHeapObjectsParams> params =
        StopTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = heapprofiler_->StopTrackingHeapObjects(std::move(params));
    SendResponse(request, response);
}

void HeapProfilerImpl::DispatcherImpl::TakeHeapSnapshot(const DispatchRequest &request)
{
    std::unique_ptr<StopTrackingHeapObjectsParams> params =
        StopTrackingHeapObjectsParams::Create(request.GetEcmaVM(), request.GetParamsObj());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = heapprofiler_->TakeHeapSnapshot(std::move(params));
    SendResponse(request, response);
}

bool HeapProfilerImpl::Frontend::AllowNotify() const
{
    return channel_ != nullptr;
}

void HeapProfilerImpl::Frontend::AddHeapSnapshotChunk(char *data, int size)
{
    if (!AllowNotify()) {
        return;
    }

    tooling::AddHeapSnapshotChunk addHeapSnapshotChunk;
    addHeapSnapshotChunk.GetChunk().resize(size);
    for (int i = 0; i < size; ++i) {
        addHeapSnapshotChunk.GetChunk()[i] = data[i];
    }

    channel_->SendNotification(addHeapSnapshotChunk);
}

void HeapProfilerImpl::Frontend::ReportHeapSnapshotProgress(int32_t done, int32_t total)
{
    if (!AllowNotify()) {
        return;
    }

    tooling::ReportHeapSnapshotProgress reportHeapSnapshotProgress;
    reportHeapSnapshotProgress.SetDone(done).SetTotal(total);
    if (done >= total) {
        reportHeapSnapshotProgress.SetFinished(true);
    }
    channel_->SendNotification(reportHeapSnapshotProgress);
}

void HeapProfilerImpl::Frontend::HeapStatsUpdate(HeapStat* updateData, int count)
{
    if (!AllowNotify()) {
        return;
    }
    std::vector<uint32_t> statsDiff;
    for (int i = 0; i < count; ++i) {
        statsDiff.emplace_back(updateData[i].index_);
        statsDiff.emplace_back(updateData[i].count_);
        statsDiff.emplace_back(updateData[i].size_);
    }
    tooling::HeapStatsUpdate heapStatsUpdate;
    heapStatsUpdate.SetStatsUpdate(std::move(statsDiff));
    channel_->SendNotification(heapStatsUpdate);
}

void HeapProfilerImpl::Frontend::LastSeenObjectId(uint32_t lastSeenObjectId)
{
    if (!AllowNotify()) {
        return;
    }

    tooling::LastSeenObjectId lastSeenObjectIdEvent;
    lastSeenObjectIdEvent.SetLastSeenObjectId(lastSeenObjectId);
    size_t timestamp = 0;
    struct timeval tv = {0, 0};
    gettimeofday(&tv, nullptr);
    const int THOUSAND = 1000;
    timestamp = tv.tv_usec + tv.tv_sec * THOUSAND * THOUSAND;
    lastSeenObjectIdEvent.SetTimestamp(timestamp);
    channel_->SendNotification(lastSeenObjectIdEvent);
}

void HeapProfilerImpl::Frontend::ResetProfiles()
{
    if (!AllowNotify()) {
        return;
    }
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

DispatchResponse HeapProfilerImpl::StartTrackingHeapObjects(std::unique_ptr<StartTrackingHeapObjectsParams> params)
{
    bool result = false;
    bool trackAllocations = params->GetTrackAllocations();
    if (trackAllocations) {
        result = panda::DFXJSNApi::StartHeapTracking(vm_, INTERVAL, true, &stream_);
    } else {
        result = panda::DFXJSNApi::StartHeapTracking(vm_, INTERVAL, true, nullptr);
    }

    if (result) {
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

DispatchResponse HeapProfilerImpl::StopTrackingHeapObjects(std::unique_ptr<StopTrackingHeapObjectsParams> params)
{
    bool result = false;
    if (params->GetReportProgress()) {
        HeapProfilerProgress progress(&frontend_);
        result = panda::DFXJSNApi::StopHeapTracking(vm_, &stream_, &progress);
    } else {
        result = panda::DFXJSNApi::StopHeapTracking(vm_, &stream_, nullptr);
    }
    if (result) {
        return DispatchResponse::Ok();
    } else {
        return DispatchResponse::Fail("StopHeapTracking fail");
    }
}

DispatchResponse HeapProfilerImpl::TakeHeapSnapshot(std::unique_ptr<StopTrackingHeapObjectsParams> params)
{
    if (params->GetReportProgress()) {
        HeapProfilerProgress progress(&frontend_);
        panda::DFXJSNApi::DumpHeapSnapshot(vm_, 0, &stream_, &progress, true);
    } else {
        panda::DFXJSNApi::DumpHeapSnapshot(vm_, 0, &stream_, nullptr, true);
    }
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
