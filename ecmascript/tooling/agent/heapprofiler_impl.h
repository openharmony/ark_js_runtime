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

#include <sys/time.h>
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
    explicit HeapProfilerImpl(const EcmaVM *vm, ProtocolChannel *channel)
        : vm_(vm), frontend_(channel), stream_(&frontend_) {}
    ~HeapProfilerImpl() = default;

    DispatchResponse AddInspectedHeapObject(const AddInspectedHeapObjectParams &params);
    DispatchResponse CollectGarbage();
    DispatchResponse Enable();
    DispatchResponse Disable();
    DispatchResponse GetHeapObjectId(const GetHeapObjectIdParams &params, HeapSnapshotObjectId *objectId);
    DispatchResponse GetObjectByHeapObjectId(const GetObjectByHeapObjectIdParams &params,
                                             std::unique_ptr<RemoteObject> *remoteObjectResult);
    DispatchResponse GetSamplingProfile(std::unique_ptr<SamplingHeapProfile> *profile);
    DispatchResponse StartSampling(const StartSamplingParams &params);
    DispatchResponse StartTrackingHeapObjects(const StartTrackingHeapObjectsParams &params);
    DispatchResponse StopSampling(std::unique_ptr<SamplingHeapProfile> *profile);
    DispatchResponse StopTrackingHeapObjects(const StopTrackingHeapObjectsParams &params);
    // The params type of TakeHeapSnapshot is the same as of StopTrackingHeapObjects.
    DispatchResponse TakeHeapSnapshot(const StopTrackingHeapObjectsParams &params);

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

private:
    NO_COPY_SEMANTIC(HeapProfilerImpl);
    NO_MOVE_SEMANTIC(HeapProfilerImpl);

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}

        void AddHeapSnapshotChunk(char *data, int32_t size);
        void ReportHeapSnapshotProgress(int32_t done, int32_t total);
        void HeapStatsUpdate(HeapStat* updateData, int32_t count);
        void LastSeenObjectId(int32_t lastSeenObjectId);
        void ResetProfiles();

    private:
        bool AllowNotify() const;

        ProtocolChannel *channel_ {nullptr};
    };

    class HeapProfilerStream final : public Stream {
    public:
        explicit HeapProfilerStream(Frontend *frontend)
            : frontend_(frontend) {}
        
        void EndOfStream() override {}
        int GetSize() override
        {
            static const int heapProfilerChunkSise = 102400;
            return heapProfilerChunkSise;
        }
        bool WriteChunk(char *data, int32_t size) override
        {
            if (!Good()) {
                return false;
            }
            frontend_->AddHeapSnapshotChunk(data, size);
            return true;
        }
        bool Good() override
        {
            return frontend_ != nullptr;
        }

        void UpdateHeapStats(HeapStat* updateData, int32_t count) override
        {
            if (!Good()) {
                return;
            }
            frontend_->HeapStatsUpdate(updateData, count);
        }

        void UpdateLastSeenObjectId(int32_t lastSeenObjectId) override
        {
            if (!Good()) {
                return;
            }
            frontend_->LastSeenObjectId(lastSeenObjectId);
        }

    private:
        NO_COPY_SEMANTIC(HeapProfilerStream);
        NO_MOVE_SEMANTIC(HeapProfilerStream);

        Frontend *frontend_ {nullptr};
    };

    class HeapProfilerProgress final : public Progress {
    public:
        explicit HeapProfilerProgress(Frontend *frontend)
            : frontend_(frontend) {}
        
        void ReportProgress(int32_t done, int32_t total) override
        {
            frontend_->ReportHeapSnapshotProgress(done, total);
        }

    private:
        NO_COPY_SEMANTIC(HeapProfilerProgress);
        NO_MOVE_SEMANTIC(HeapProfilerProgress);

        Frontend *frontend_ {nullptr};
    };

    const EcmaVM *vm_ {nullptr};
    Frontend frontend_;
    HeapProfilerStream stream_;
};
}  // namespace panda::ecmascript::tooling
#endif