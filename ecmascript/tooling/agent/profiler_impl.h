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

#ifndef ECMASCRIPT_TOOLING_AGENT_PROFILER_IMPL_H
#define ECMASCRIPT_TOOLING_AGENT_PROFILER_IMPL_H

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
using CpuProfileNode = ecmascript::ProfileNode;
class ProfilerImpl final {
public:
    ProfilerImpl(const EcmaVM *vm, ProtocolChannel *channel) : vm_(vm), frontend_(channel) {}
    ~ProfilerImpl() = default;

    DispatchResponse Disable();
    DispatchResponse Enable();
    DispatchResponse Start();
    DispatchResponse Stop(std::unique_ptr<Profile> &profile);
    DispatchResponse GetBestEffortCoverage();
    DispatchResponse StopPreciseCoverage();
    DispatchResponse TakePreciseCoverage();
    DispatchResponse StartPreciseCoverage (std::unique_ptr<StartPreciseCoverageParam> params);
    DispatchResponse StartTypeProfile();
    DispatchResponse StopTypeProfile();
    DispatchResponse TakeTypeProfile();
    std::unique_ptr<Profile> FromCpuProfiler(const std::unique_ptr<ProfileInfo> &profileInfo);
    std::unique_ptr<ProfileNode> FromCpuProfileNode(const std::unique_ptr<CpuProfileNode> &cpuProfileNode);
    std::unique_ptr<RuntimeCallFrame> FromCpuFrameInfo(const std::unique_ptr<FrameInfo> &cpuFrameInfo);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(ProtocolChannel *channel, std::unique_ptr<ProfilerImpl> profiler)
            : DispatcherBase(channel), profiler_(std::move(profiler)) {}
        ~DispatcherImpl() override = default;

        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
        void Start(const DispatchRequest &request);
        void Stop(const DispatchRequest &request);
        void GetBestEffortCoverage(const DispatchRequest &request);
        void StopPreciseCoverage(const DispatchRequest &request);
        void TakePreciseCoverage(const DispatchRequest &request);
        void StartPreciseCoverage(const DispatchRequest &request);
        void StartTypeProfile(const DispatchRequest &request);
        void StopTypeProfile(const DispatchRequest &request);
        void TakeTypeProfile(const DispatchRequest &request);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        using AgentHandler = void (ProfilerImpl::DispatcherImpl::*)(const DispatchRequest &request);
        std::unique_ptr<ProfilerImpl> profiler_ {};
    };

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}

        void ConsoleProfileFinished();
        void ConsoleProfileStarted();
        void PreciseCoverageDeltaUpdate();

    private:
        bool AllowNotify() const;

        ProtocolChannel *channel_ {nullptr};
    };

private:
    NO_COPY_SEMANTIC(ProfilerImpl);
    NO_MOVE_SEMANTIC(ProfilerImpl);

    const EcmaVM *vm_ {nullptr};
    [[maybe_unused]] Frontend frontend_;
};
}  // namespace panda::ecmascript::tooling
#endif
