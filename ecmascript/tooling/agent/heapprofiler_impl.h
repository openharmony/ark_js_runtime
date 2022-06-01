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
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/interface/stream.h"
#include "ecmascript/tooling/interface/progress.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "ecmascript/tooling/protocol_channel.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "libpandabase/utils/logger.h"

static const double INTERVAL = 0.05;

namespace panda::ecmascript::tooling {
class HeapProfilerImpl final {
public:
    explicit HeapProfilerImpl(const EcmaVM *vm, ProtocolChannel *channel) : vm_(vm), frontend_(channel) {}
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
        DispatcherImpl(ProtocolChannel *channel, std::unique_ptr<HeapProfilerImpl> heapprofiler)
            : DispatcherBase(channel), heapprofiler_(std::move(heapprofiler)) {}
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
        std::unique_ptr<HeapProfilerImpl> heapprofiler_ {};
    };

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}

        void AddHeapSnapshotChunk(char *data, int size);
        void ReportHeapSnapshotProgress(int32_t done, int32_t total);
        void HeapStatsUpdate();
        void LastSeenObjectId();
        void ResetProfiles();

    private:
        bool AllowNotify() const;

        ProtocolChannel *channel_ {nullptr};
    };

private:
    NO_COPY_SEMANTIC(HeapProfilerImpl);
    NO_MOVE_SEMANTIC(HeapProfilerImpl);

    const EcmaVM *vm_ {nullptr};
    Frontend frontend_;
};
}  // namespace panda::ecmascript::tooling
#endif