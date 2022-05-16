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

#ifndef ECMASCRIPT_TOOLING_AGENT_JS_BACKEND_H
#define ECMASCRIPT_TOOLING_AGENT_JS_BACKEND_H

#include "ecmascript/tooling/agent/js_pt_hooks.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "ecmascript/tooling/pt_js_extractor.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::CString;

class JSBackend {
public:
    explicit JSBackend(FrontEnd *frontend);
    explicit JSBackend(const EcmaVM *vm);
    ~JSBackend();

    // add for hooks
    void ProcessCommand();
    void WaitForDebugger();
    void NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason);
    void NotifyResume();
    void NotifyAllScriptParsed();
    bool NotifyScriptParsed(int32_t scriptId, const CString &fileName);
    bool StepComplete(const JSPtLocation &location);

    std::optional<CString> GetPossibleBreakpoints(Location *start, Location *end,
                                                CVector<std::unique_ptr<BreakLocation>> *locations);
    JSDebugger *GetDebugger() const
    {
        return debugger_;
    }

    std::optional<CString> SetBreakpointByUrl(const CString &url, size_t lineNumber, size_t columnNumber,
                                            CString *outId,
                                            CVector<std::unique_ptr<Location>> *outLocations);
    std::optional<CString> RemoveBreakpoint(const BreakpointDetails &metaData);

    std::optional<CString> Pause();
    std::optional<CString> Resume();
    std::optional<CString> StepInto();
    std::optional<CString> StepOver();
    std::optional<CString> StepOut();
    std::optional<CString> EvaluateValue(const CString &callFrameId, const CString &expression,
                                       std::unique_ptr<RemoteObject> *result);
    std::optional<CString> CmptEvaluateValue(const CString &callFrameId, const CString &expression,
                                       std::unique_ptr<RemoteObject> *result);

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
                case ScriptMatchType::SCRIPT_ID: {
                    value = script.second->GetScriptId();
                    break;
                }
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

    void SetPauseOnException(bool flag);
    void GetProperties(uint32_t objectId, bool isOwn, bool isAccessorOnly,
                       CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    // public for testcases
    bool GenerateCallFrames(CVector<std::unique_ptr<CallFrame>> *callFrames);
    const EcmaVM *GetEcmaVm() const
    {
        return ecmaVm_;
    }

private:
    NO_MOVE_SEMANTIC(JSBackend);
    NO_COPY_SEMANTIC(JSBackend);
    CString Trim(const CString &str);
    PtJSExtractor *GenerateExtractor(const panda_file::File *file);
    PtJSExtractor *GetExtractor(const panda_file::File *file);
    PtJSExtractor *GetExtractor(const CString &url);
    bool GenerateCallFrame(CallFrame *callFrame, const InterpretedFrameHandler *frameHandler, int32_t frameId);
    void SaveCallFrameHandler(const InterpretedFrameHandler *frameHandler);
    void GetLocalVariables(const InterpretedFrameHandler *frameHandler, const JSMethod *method,
        Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj);
    void CacheObjectIfNeeded(const Local<JSValueRef> &valRef, std::unique_ptr<RemoteObject> *remoteObj);
    void CleanUpOnPaused();
    std::unique_ptr<Scope> GetLocalScopeChain(const InterpretedFrameHandler *frameHandler,
        std::unique_ptr<RemoteObject> *thisObj);
    void UpdateScopeObject(const InterpretedFrameHandler *frameHandler, const CString &varName,
        const Local<JSValueRef> &newVal);
    std::unique_ptr<Scope> GetGlobalScopeChain();
    std::optional<CString> ConvertToLocal(Local<JSValueRef> &taggedValue, std::unique_ptr<RemoteObject> *result,
        const CString &varValue);
    std::optional<CString> SetVregValue(int32_t regIndex, const CString &varValue,
        std::unique_ptr<RemoteObject> *result);
    std::optional<CString> SetLexicalValue(int32_t level, uint32_t slot, const CString &varValue,
        std::unique_ptr<RemoteObject> *result);
    std::optional<CString> GetVregValue(int32_t regIndex, std::unique_ptr<RemoteObject> *result);
    std::optional<CString> GetLexicalValue(int32_t level, uint32_t slot, std::unique_ptr<RemoteObject> *result);
    void GetProtoOrProtoType(const Local<JSValueRef> &value, bool isOwn, bool isAccessorOnly,
                             CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    bool DecodeAndCheckBase64(const CString &src, CString &dest);

    constexpr static int32_t SPECIAL_LINE_MARK = -1;

    FrontEnd *frontend_ {nullptr};
    const EcmaVM *ecmaVm_ {nullptr};
    std::unique_ptr<JSPtHooks> hooks_ {nullptr};
    JSDebugger *debugger_ {nullptr};
    CMap<const std::string, std::unique_ptr<PtJSExtractor>> extractors_ {};
    CMap<CString, std::unique_ptr<PtScript>> scripts_ {};
    CMap<uint32_t, Global<JSValueRef>> propertiesPair_ {};
    CUnorderedMap<JSTaggedType *, uint32_t> scopeObjects_ {};
    CVector<std::shared_ptr<InterpretedFrameHandler>> callFrameHandlers_;
    uint32_t curObjectId_ {0};
    bool pauseOnException_ {false};
    bool pauseOnNextByteCode_ {false};
    std::unique_ptr<PtJSExtractor::SingleStepper> singleStepper_ {nullptr};
    JsDebuggerManager::ObjectUpdaterFunc updaterFunc_ {nullptr};

    friend class JSPtHooks;
};
}  // namespace panda::ecmascript::tooling
#endif
