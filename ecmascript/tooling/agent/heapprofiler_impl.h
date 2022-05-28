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

#ifndef ECMASCRIPT_TOOLING_AGENT_HEAPPROFILER_IMPL_H
#define ECMASCRIPT_TOOLING_AGENT_HEAPPROFILER_IMPL_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/interface/stream.h"
#include "ecmascript/tooling/interface/progress.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "ecmascript/tooling/front_end.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "libpandabase/utils/logger.h"

static const double INTERVAL = 0.05;
static const int  MAX_HEAPPROFILER_CHUNK_SIZE = 102400;

namespace panda::ecmascript::tooling {
class HeapProfilerImpl final {
public:
    explicit HeapProfilerImpl(FrontEnd *frontend) : frontend_(frontend) {}
    ~HeapProfilerImpl() = default;

    DispatchResponse AddInspectedHeapObject(std::unique_ptr<AddInspectedHeapObjectParams> params);
    DispatchResponse CollectGarbage();
    DispatchResponse Enable();
    DispatchResponse Disable();
    DispatchResponse GetHeapObjectId(std::unique_ptr<GetHeapObjectIdParams> params, HeapSnapshotObjectId *objectId);
    DispatchResponse GetObjectByHeapObjectId(std::unique_ptr<GetObjectByHeapObjectIdParams> params,
                                             std::unique_ptr<RemoteObject> *remoteObjectResult);
    DispatchResponse GetSamplingProfile(std::unique_ptr<SamplingHeapProfile> *profile);
    DispatchResponse StartSampling(std::unique_ptr<StartSamplingParams> params);
    DispatchResponse StartTrackingHeapObjects(std::unique_ptr<StartTrackingHeapObjectsParams> params);
    DispatchResponse StopSampling(std::unique_ptr<SamplingHeapProfile> *profile);
    DispatchResponse StopTrackingHeapObjects(std::unique_ptr<StopTrackingHeapObjectsParams> params);
    // The params type of TakeHeapSnapshot is the same as of StopTrackingHeapObjects.
    DispatchResponse TakeHeapSnapshot(std::unique_ptr<StopTrackingHeapObjectsParams> params);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(FrontEnd *frontend, std::unique_ptr<HeapProfilerImpl> heapprofiler);
        ~DispatcherImpl() override = default;
        void Dispatch(const DispatchRequest &request) override;
        void AddInspectedHeapObject(const DispatchRequest &request);
        void CollectGarbage(const DispatchRequest &request);
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
        void GetHeapObjectId(const DispatchRequest &request);
        void GetObjectByHeapObjectId(const DispatchRequest &request);
        void GetSamplingProfile(const DispatchRequest &request);
        void StartSampling(const DispatchRequest &request);
        void StartTrackingHeapObjects(const DispatchRequest &request);
        void StopSampling(const DispatchRequest &request);
        void StopTrackingHeapObjects(const DispatchRequest &request);
        void TakeHeapSnapshot(const DispatchRequest &request);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        using AgentHandler = void (HeapProfilerImpl::DispatcherImpl::*)(const DispatchRequest &request);
        CUnorderedMap<CString, AgentHandler> dispatcherTable_ {};
        std::unique_ptr<HeapProfilerImpl> heapprofiler_ {};
    };

private:
    NO_COPY_SEMANTIC(HeapProfilerImpl);
    NO_MOVE_SEMANTIC(HeapProfilerImpl);

    FrontEnd* frontend_ {nullptr};
};

class HeapProfilerStream final : public Stream {
public:
    explicit HeapProfilerStream(FrontEnd* frontend)
        : frontend_(frontend) {}
    
    void EndOfStream() override {}
    int GetSize() override
    {
        return MAX_HEAPPROFILER_CHUNK_SIZE;
    }
    bool WriteChunk(char* data, int size) override
    {
        auto ecmaVm = static_cast<ProtocolHandler *>(frontend_)->GetEcmaVM();
        frontend_->SendProfilerNotify(ecmaVm, AddHeapSnapshotChunk::Create(data, size));
        return true;
    }
    bool Good() override
    {
        return frontend_ != nullptr;
    }

private:
    NO_COPY_SEMANTIC(HeapProfilerStream);
    NO_MOVE_SEMANTIC(HeapProfilerStream);

    FrontEnd* frontend_ {nullptr};
};

class HeapProfilerProgress final : public Progress {
public:
    explicit HeapProfilerProgress(FrontEnd* frontend)
        : frontend_(frontend) {}
    
    void ReportProgress(int32_t done, int32_t total) override
    {
        auto ecmaVm = static_cast<ProtocolHandler *>(frontend_)->GetEcmaVM();
        auto reportHeapSnapshotProgress = std::make_unique<ReportHeapSnapshotProgress>();
        reportHeapSnapshotProgress->SetDone(done);
        reportHeapSnapshotProgress->SetTotal(total);
        if (done == total) {
            reportHeapSnapshotProgress->SetFinished(true);
        }
        frontend_->SendProfilerNotify(ecmaVm, std::move(reportHeapSnapshotProgress));
    }

private:
    NO_COPY_SEMANTIC(HeapProfilerProgress);
    NO_MOVE_SEMANTIC(HeapProfilerProgress);

    FrontEnd* frontend_ {nullptr};
};
}  // namespace panda::ecmascript::tooling
#endif