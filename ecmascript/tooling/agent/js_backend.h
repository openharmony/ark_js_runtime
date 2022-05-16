/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "ecmascript/tooling/interface/js_debugger.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "ecmascript/tooling/js_pt_extractor.h"
#include "libpandabase/macros.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"

namespace panda::ecmascript::tooling {
class JSBackend {
public:
    enum NumberSize : uint8_t { BYTES_OF_16BITS = 2, BYTES_OF_32BITS = 4, BYTES_OF_64BITS = 8 };

    explicit JSBackend(FrontEnd *frontend);
    explicit JSBackend(const EcmaVM *vm);
    ~JSBackend();

    // add for hooks
    void WaitForDebugger();
    void NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason);
    void NotifyResume();
    void NotifyAllScriptParsed();
    bool NotifyScriptParsed(ScriptId scriptId, const CString &fileName);
    bool StepComplete(const JSPtLocation &location);

    std::optional<CString> GetPossibleBreakpoints(Location *start, Location *end,
                                                CVector<std::unique_ptr<BreakLocation>> *locations);
    JSDebugger *GetDebugger() const
    {
        return debugger_;
    }

    std::optional<CString> SetBreakpointByUrl(const CString &url, int32_t lineNumber, int32_t columnNumber,
                                            const std::optional<CString> &condition, CString *outId,
                                            CVector<std::unique_ptr<Location>> *outLocations);
    std::optional<CString> RemoveBreakpoint(const BreakpointDetails &metaData);

    std::optional<CString> Pause();
    std::optional<CString> Resume();
    std::optional<CString> StepInto();
    std::optional<CString> StepOver();
    std::optional<CString> StepOut();
    std::optional<CString> EvaluateValue(CallFrameId callFrameId, const CString &expression,
                                         std::unique_ptr<RemoteObject> *result);
    std::optional<CString> CmptEvaluateValue(CallFrameId callFrameId, const CString &expression,
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

    bool GetScriptSource(ScriptId scriptId, CString *source);

    void SetPauseOnException(bool flag);
    void GetProperties(RemoteObjectId objectId, bool isOwn, bool isAccessorOnly,
                       CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    void CallFunctionOn(const CString &functionDeclaration, std::unique_ptr<RemoteObject> *outRemoteObject);
    void GetHeapUsage(double *usedSize, double *totalSize);
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
    JSPtExtractor *GenerateExtractor(const JSPandaFile *jsPandaFile);
    JSPtExtractor *GetExtractor(const JSPandaFile *jsPandaFile);
    JSPtExtractor *GetExtractor(const CString &url);
    bool GenerateCallFrame(CallFrame *callFrame, const FrameHandler *frameHandler, CallFrameId frameId);
    void SaveCallFrameHandler(const FrameHandler *frameHandler);
    std::unique_ptr<Scope> GetLocalScopeChain(const FrameHandler *frameHandler,
        std::unique_ptr<RemoteObject> *thisObj);
    void GetLocalVariables(const FrameHandler *frameHandler, const JSMethod *method,
        Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj);
    void CacheObjectIfNeeded(const Local<JSValueRef> &valRef, std::unique_ptr<RemoteObject> *remoteObj);
    void CleanUpOnPaused();
    std::unique_ptr<Scope> GetGlobalScopeChain();
    void UpdateScopeObject(const FrameHandler *frameHandler, const CString &varName, const Local<JSValueRef> &newVal);
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
    void AddTypedArrayRefs(Local<ArrayBufferRef> arrayBufferRef,
        CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    template <typename TypedArrayRef>
    void AddTypedArrayRef(Local<ArrayBufferRef> arrayBufferRef, int32_t length,
        const char* name, CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    void GetAdditionalProperties(const Local<JSValueRef> &value,
        CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc);
    bool IsSkipLine(const JSPtLocation &location);

    constexpr static int32_t SPECIAL_LINE_MARK = -1;

    FrontEnd *frontend_ {nullptr};
    const EcmaVM *ecmaVm_ {nullptr};
    std::unique_ptr<JSPtHooks> hooks_ {nullptr};
    JSDebugger *debugger_ {nullptr};
    CUnorderedMap<CString, JSPtExtractor *> extractors_ {};
    CUnorderedMap<ScriptId, std::unique_ptr<PtScript>> scripts_ {};
    CUnorderedMap<RemoteObjectId, Global<JSValueRef>> propertiesPair_ {};
    CUnorderedMap<JSTaggedType *, RemoteObjectId> scopeObjects_ {};
    CVector<std::shared_ptr<FrameHandler>> callFrameHandlers_;
    RemoteObjectId curObjectId_ {0};
    bool pauseOnException_ {false};
    bool pauseOnNextByteCode_ {false};
    std::unique_ptr<JSPtExtractor::SingleStepper> singleStepper_ {nullptr};
    JsDebuggerManager::ObjectUpdaterFunc updaterFunc_ {nullptr};

    friend class JSPtHooks;
};
}  // namespace panda::ecmascript::tooling
#endif
