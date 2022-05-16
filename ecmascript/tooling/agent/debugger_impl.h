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

namespace panda::ecmascript::tooling {
using panda::ecmascript::CString;

class DebuggerImpl final {
public:
    explicit DebuggerImpl(std::unique_ptr<JSBackend> backend) : backend_(std::move(backend)) {}
    ~DebuggerImpl() = default;

    DispatchResponse Enable(std::unique_ptr<EnableParams> params, UniqueDebuggerId *id);
    DispatchResponse EvaluateOnCallFrame(std::unique_ptr<EvaluateOnCallFrameParams> params,
                                         std::unique_ptr<RemoteObject> *result);
    DispatchResponse GetPossibleBreakpoints(std::unique_ptr<GetPossibleBreakpointsParams> params,
                                            CVector<std::unique_ptr<BreakLocation>> *outLocations);
    DispatchResponse GetScriptSource(std::unique_ptr<GetScriptSourceParams> params, CString *source);
    DispatchResponse Pause();
    DispatchResponse RemoveBreakpoint(std::unique_ptr<RemoveBreakpointParams> params);
    DispatchResponse Resume(std::unique_ptr<ResumeParams> params);
    DispatchResponse SetAsyncCallStackDepth();
    DispatchResponse SetBreakpointByUrl(std::unique_ptr<SetBreakpointByUrlParams> params, CString *out_id,
                                        CVector<std::unique_ptr<Location>> *outLocations);
    DispatchResponse SetPauseOnExceptions(std::unique_ptr<SetPauseOnExceptionsParams> params);
    DispatchResponse StepInto(std::unique_ptr<StepIntoParams> params);
    DispatchResponse StepOut();
    DispatchResponse StepOver(std::unique_ptr<StepOverParams> params);
    DispatchResponse SetBlackboxPatterns();

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(FrontEnd *frontend, std::unique_ptr<DebuggerImpl> debugger);
        ~DispatcherImpl() override = default;
        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void EvaluateOnCallFrame(const DispatchRequest &request);
        void GetPossibleBreakpoints(const DispatchRequest &request);
        void GetScriptSource(const DispatchRequest &request);
        void Pause(const DispatchRequest &request);
        void RemoveBreakpoint(const DispatchRequest &request);
        void Resume(const DispatchRequest &request);
        void SetAsyncCallStackDepth(const DispatchRequest &request);
        void SetBreakpointByUrl(const DispatchRequest &request);
        void SetPauseOnExceptions(const DispatchRequest &request);
        void StepInto(const DispatchRequest &request);
        void StepOut(const DispatchRequest &request);
        void StepOver(const DispatchRequest &request);
        void SetBlackboxPatterns(const DispatchRequest &request);

    private:
        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);

        using AgentHandler = void (DebuggerImpl::DispatcherImpl::*)(const DispatchRequest &request);
        CMap<CString, AgentHandler> dispatcherTable_ {};
        std::unique_ptr<DebuggerImpl> debugger_ {};
    };

private:
    NO_COPY_SEMANTIC(DebuggerImpl);
    NO_MOVE_SEMANTIC(DebuggerImpl);

    std::unique_ptr<JSBackend> backend_ {nullptr};
};
}  // namespace panda::ecmascript::tooling
#endif