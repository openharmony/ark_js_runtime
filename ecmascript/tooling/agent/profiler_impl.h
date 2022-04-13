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

#ifndef ECMASCRIPT_TOOLING_AGENT_DEBUGGER_IMPL_H
#define ECMASCRIPT_TOOLING_AGENT_DEBUGGER_IMPL_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/dispatcher.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CString;


class ProfilerImpl final {
public:
    explicit ProfilerImpl(JSBackend *backend) : backend_(backend) {}
    ~ProfilerImpl() = default;

    DispatchResponse Disable();
    DispatchResponse Enable();
    DispatchResponse Start();
    DispatchResponse Stop();

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(FrontEnd *frontend, std::unique_ptr<ProfilerImpl> profiler);
        ~DispatcherImpl() override = default;
        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
        void Start(const DispatchRequest &request);
        void Stop(const DispatchRequest &request);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        using AgentHandler = void (ProfilerImpl::DispatcherImpl::*)(const DispatchRequest &request);
        CUnorderedMap<CString, AgentHandler> dispatcherTable_ {};
        std::unique_ptr<ProfilerImpl> profiler_ {};
    };

private:
    NO_COPY_SEMANTIC(ProfilerImpl);
    NO_MOVE_SEMANTIC(ProfilerImpl);
    
    std::unique_ptr<JSBackend> backend_ {nullptr};
};
}  // namespace panda::tooling::ecmascript
#endif
