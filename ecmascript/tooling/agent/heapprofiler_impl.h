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
#include "ecmascript/tooling/dispatcher.h"

namespace panda::tooling::ecmascript {
class HeapProfilerImpl final {
public:
    explicit HeapProfilerImpl(std::unique_ptr<JSBackend> backend) : backend_(std::move(backend)) {}
    ~HeapProfilerImpl() = default;

    DispatchResponse Enable();
    DispatchResponse Disable();
    DispatchResponse StartSampling(std::unique_ptr<StartSamplingParams> params);
    DispatchResponse StartTrackingHeapObjects(std::unique_ptr<StartTrackingHeapObjectsParams> params);
    DispatchResponse StopSampling(std::unique_ptr<SamplingHeapProfile> *profile);
    DispatchResponse StopTrackingHeapObjects (std::unique_ptr<StopTrackingHeapObjectsParams> params);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(FrontEnd *frontend, std::unique_ptr<HeapProfilerImpl> heapprofiler);
        ~DispatcherImpl() override = default;
        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
        void StartSampling(const DispatchRequest &request);
        void StartTrackingHeapObjects(const DispatchRequest &request);
        void StopSampling(const DispatchRequest &request);
        void StopTrackingHeapObjects(const DispatchRequest &request);

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

    std::unique_ptr<JSBackend> backend_ {nullptr};
};
}  // namespace panda::tooling::ecmascript
#endif