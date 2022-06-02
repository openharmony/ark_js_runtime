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
#include "ecmascript/tooling/agent/runtime_impl.h"
#include "ecmascript/tooling/backend/js_pt_hooks.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/backend/js_pt_extractor.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"

namespace panda::ecmascript::tooling {
class DebuggerImpl final {
public:
    DebuggerImpl(const EcmaVM *vm, ProtocolChannel *channel, RuntimeImpl *runtime);
    ~DebuggerImpl();

    // event
    bool NotifyScriptParsed(ScriptId scriptId, const CString &fileName);
    bool NotifySingleStep(const JSPtLocation &location);
    void NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason);
    void NotifyPendingJobEntry();

    DispatchResponse Enable(std::unique_ptr<EnableParams> params, UniqueDebuggerId *id);
    DispatchResponse Disable();
    DispatchResponse EvaluateOnCallFrame(std::unique_ptr<EvaluateOnCallFrameParams> params,
                                         std::unique_ptr<RemoteObject> *result);
    DispatchResponse GetPossibleBreakpoints(std::unique_ptr<GetPossibleBreakpointsParams> params,
                                            CVector<std::unique_ptr<BreakLocation>> *outLocations);
    DispatchResponse GetScriptSource(std::unique_ptr<GetScriptSourceParams> params, CString *source);
    DispatchResponse Pause();
    DispatchResponse RemoveBreakpoint(std::unique_ptr<RemoveBreakpointParams> params);
    DispatchResponse Resume(std::unique_ptr<ResumeParams> params);
    DispatchResponse SetAsyncCallStackDepth();
    DispatchResponse SetBreakpointByUrl(std::unique_ptr<SetBreakpointByUrlParams> params, CString *outId,
                                        CVector<std::unique_ptr<Location>> *outLocations);
    DispatchResponse SetPauseOnExceptions(std::unique_ptr<SetPauseOnExceptionsParams> params);
    DispatchResponse StepInto(std::unique_ptr<StepIntoParams> params);
    DispatchResponse StepOut();
    DispatchResponse StepOver(std::unique_ptr<StepOverParams> params);
    DispatchResponse SetBlackboxPatterns();

    // for testcase
    JSDebugger *GetDebugger() const
    {
        return jsDebugger_;
    }
    bool GenerateCallFrames(CVector<std::unique_ptr<CallFrame>> *callFrames);

    /**
     * @brief: match first script and callback
     *
     * @return: true means matched and callback execute success
     */
    template<class Callback>
    bool MatchScripts(const Callback &cb, const CString &matchStr, ScriptMatchType type) const
    {
        for (const auto &script : scripts_) {
            CString value;
            switch (type) {
                case ScriptMatchType::URL: {
                    value = script.second->GetUrl();
                    break;
                }
                case ScriptMatchType::FILE_NAME: {
                    value = script.second->GetFileName();
                    break;
                }
                case ScriptMatchType::HASH: {
                    value = script.second->GetHash();
                    break;
                }
                default: {
                    return false;
                }
            }
            if (matchStr == value) {
                return cb(script.second.get());
            }
        }
        return false;
    }

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(ProtocolChannel *channel, std::unique_ptr<DebuggerImpl> debugger)
            : DispatcherBase(channel), debugger_(std::move(debugger)) {}
        ~DispatcherImpl() override = default;

        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void Disable(const DispatchRequest &request);
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
        std::unique_ptr<DebuggerImpl> debugger_ {};
    };

private:
    NO_COPY_SEMANTIC(DebuggerImpl);
    NO_MOVE_SEMANTIC(DebuggerImpl);

    CString Trim(const CString &str);
    JSPtExtractor *GetExtractor(const JSPandaFile *jsPandaFile);
    JSPtExtractor *GetExtractor(const CString &url);
    std::optional<CString> CmptEvaluateValue(CallFrameId callFrameId, const CString &expression,
                                             std::unique_ptr<RemoteObject> *result);
    bool GenerateCallFrame(CallFrame *callFrame, const FrameHandler *frameHandler, CallFrameId frameId);
    void SaveCallFrameHandler(const FrameHandler *frameHandler);
    std::unique_ptr<Scope> GetLocalScopeChain(const FrameHandler *frameHandler,
        std::unique_ptr<RemoteObject> *thisObj);
    std::unique_ptr<Scope> GetGlobalScopeChain();
    void GetLocalVariables(const FrameHandler *frameHandler, const JSMethod *method,
        Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj);
    void CleanUpOnPaused();
    void UpdateScopeObject(const FrameHandler *frameHandler, std::string_view varName, const Local<JSValueRef> &newVal);
    Local<JSValueRef> ConvertToLocal(const CString &varValue);
    bool DecodeAndCheckBase64(const CString &src, CString &dest);
    bool IsSkipLine(const JSPtLocation &location);

    class Frontend {
    public:
        explicit Frontend(ProtocolChannel *channel) : channel_(channel) {}

        void BreakpointResolved(const EcmaVM *vm);
        void Paused(const EcmaVM *vm, std::unique_ptr<tooling::Paused> paused);
        void Resumed(const EcmaVM *vm);
        void ScriptFailedToParse(const EcmaVM *vm);
        void ScriptParsed(const EcmaVM *vm, const std::unique_ptr<PtScript> &script);
        void WaitForDebugger(const EcmaVM *vm);

    private:
        bool AllowNotify(const EcmaVM *vm) const;

        ProtocolChannel *channel_ {nullptr};
    };

    const EcmaVM *vm_ {nullptr};
    Frontend frontend_;

    RuntimeImpl *runtime_ {nullptr};
    std::unique_ptr<JSPtHooks> hooks_ {nullptr};
    JSDebugger *jsDebugger_ {nullptr};

    CUnorderedMap<CString, JSPtExtractor *> extractors_ {};
    CUnorderedMap<ScriptId, std::unique_ptr<PtScript>> scripts_ {};
    bool pauseOnException_ {false};
    bool pauseOnNextByteCode_ {false};
    std::unique_ptr<JSPtExtractor::SingleStepper> singleStepper_ {nullptr};

    CUnorderedMap<JSTaggedType *, RemoteObjectId> scopeObjects_ {};
    CVector<std::shared_ptr<FrameHandler>> callFrameHandlers_;
    JsDebuggerManager::ObjectUpdaterFunc updaterFunc_ {nullptr};

    friend class JSPtHooks;
};
}  // namespace panda::ecmascript::tooling
#endif