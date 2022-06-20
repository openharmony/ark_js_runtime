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

#include "ecmascript/tooling/agent/debugger_impl.h"

#include <boost/beast/core/detail/base64.hpp>

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/jsnapi_helper-inl.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/backend/debugger_executor.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/protocol_channel.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
using namespace boost::beast::detail;
using namespace std::placeholders;

using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

#ifdef DEBUGGER_TEST
const std::string DATA_APP_PATH = "/";
#else
const std::string DATA_APP_PATH = "/data/";
#endif

DebuggerImpl::DebuggerImpl(const EcmaVM *vm, ProtocolChannel *channel, RuntimeImpl *runtime)
    : vm_(vm), frontend_(channel), runtime_(runtime)
{
    hooks_ = std::make_unique<JSPtHooks>(this);

    jsDebugger_ = DebuggerApi::CreateJSDebugger(vm_);
    DebuggerApi::RegisterHooks(jsDebugger_, hooks_.get());

    DebuggerExecutor::Initialize(vm_);
    updaterFunc_ = std::bind(&DebuggerImpl::UpdateScopeObject, this, _1, _2, _3);
    vm_->GetJsDebuggerManager()->SetLocalScopeUpdater(&updaterFunc_);
}

DebuggerImpl::~DebuggerImpl()
{
    DebuggerApi::DestroyJSDebugger(jsDebugger_);
}

bool DebuggerImpl::NotifyScriptParsed(ScriptId scriptId, const std::string &fileName)
{
    if (fileName.substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH) {
        LOG(WARNING, DEBUGGER) << "NotifyScriptParsed: unsupport file: " << fileName;
        return false;
    }

    auto scriptFunc = []([[maybe_unused]] PtScript *script) -> bool {
        return true;
    };
    if (MatchScripts(scriptFunc, fileName, ScriptMatchType::FILE_NAME)) {
        LOG(WARNING, DEBUGGER) << "NotifyScriptParsed: already loaded: " << fileName;
        return false;
    }
    const JSPandaFile *jsPandaFile = nullptr;
    JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&jsPandaFile, &fileName](
        const panda::ecmascript::JSPandaFile *pf) {
        if (fileName == pf->GetJSPandaFileDesc().c_str()) {
            jsPandaFile = pf;
            return false;
        }
        return true;
    });
    if (jsPandaFile == nullptr) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: unknown file: " << fileName;
        return false;
    }

    JSPtExtractor *extractor = GetExtractor(jsPandaFile);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: Unsupported file: " << fileName;
        return false;
    }

    auto mainMethodIndex = panda_file::File::EntityId(jsPandaFile->GetMainMethodIndex());
    const std::string &source = extractor->GetSourceCode(mainMethodIndex);
    const std::string &url = extractor->GetSourceFile(mainMethodIndex);
    const uint32_t MIN_SOURCE_CODE_LENGTH = 5;  // maybe return 'ANDA' when source code is empty
    if (source.size() < MIN_SOURCE_CODE_LENGTH) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: invalid file: " << fileName;
        return false;
    }
    // store here for performance of get extractor from url
    extractors_[url] = extractor;

    // Notify script parsed event
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(scriptId, fileName, url, source);

    frontend_.ScriptParsed(vm_, *script);

    // Store parsed script in map
    scripts_[script->GetScriptId()] = std::move(script);
    return true;
}

bool DebuggerImpl::NotifySingleStep(const JSPtLocation &location)
{
    if (UNLIKELY(pauseOnNextByteCode_)) {
        if (IsSkipLine(location)) {
            return false;
        }
        pauseOnNextByteCode_ = false;
        LOG(INFO, DEBUGGER) << "StepComplete: pause on next bytecode";
        return true;
    }

    if (LIKELY(singleStepper_ == nullptr)) {
        return false;
    }

    // step not complete
    if (!singleStepper_->StepComplete(location.GetBytecodeOffset())) {
        return false;
    }

    // skip unknown file or special line -1
    if (IsSkipLine(location)) {
        return false;
    }

    LOG(INFO, DEBUGGER) << "StepComplete: pause on current byte_code";
    return true;
}

bool DebuggerImpl::IsSkipLine(const JSPtLocation &location)
{
    JSPtExtractor *extractor = nullptr;
    auto scriptFunc = [this, &extractor](PtScript *script) -> bool {
        extractor = GetExtractor(script->GetUrl());
        return true;
    };
    if (!MatchScripts(scriptFunc, location.GetPandaFile(), ScriptMatchType::FILE_NAME) || extractor == nullptr) {
        LOG(INFO, DEBUGGER) << "StepComplete: skip unknown file";
        return true;
    }

    auto callbackFunc = [](int32_t line) -> bool {
        return line == JSPtExtractor::SPECIAL_LINE_MARK;
    };
    File::EntityId methodId = location.GetMethodId();
    uint32_t offset = location.GetBytecodeOffset();
    if (extractor->MatchLineWithOffset(callbackFunc, methodId, offset)) {
        LOG(INFO, DEBUGGER) << "StepComplete: skip -1";
        return true;
    }

    return false;
}

void DebuggerImpl::NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason)
{
    if (!pauseOnException_ && reason == EXCEPTION) {
        return;
    }
    Local<JSValueRef> exception = DebuggerApi::GetAndClearException(vm_);

    std::vector<std::string> hitBreakpoints;
    if (location.has_value()) {
        BreakpointDetails detail;
        JSPtExtractor *extractor = nullptr;
        auto scriptFunc = [this, &extractor, &detail](PtScript *script) -> bool {
            detail.url_ = script->GetUrl();
            extractor = GetExtractor(detail.url_);
            return true;
        };
        auto callbackLineFunc = [&detail](int32_t line) -> bool {
            detail.line_ = line;
            return true;
        };
        auto callbackColumnFunc = [&detail](int32_t column) -> bool {
            detail.column_ = column;
            return true;
        };
        File::EntityId methodId = location->GetMethodId();
        uint32_t offset = location->GetBytecodeOffset();
        if (!MatchScripts(scriptFunc, location->GetPandaFile(), ScriptMatchType::FILE_NAME) ||
            extractor == nullptr || !extractor->MatchLineWithOffset(callbackLineFunc, methodId, offset) ||
            !extractor->MatchColumnWithOffset(callbackColumnFunc, methodId, offset)) {
            LOG(ERROR, DEBUGGER) << "NotifyPaused: unknown " << location->GetPandaFile();
            return;
        }
        hitBreakpoints.emplace_back(BreakpointDetails::ToString(detail));
    }

    // Do something cleaning on paused
    CleanUpOnPaused();

    // Notify paused event
    std::vector<std::unique_ptr<CallFrame>> callFrames;
    if (!GenerateCallFrames(&callFrames)) {
        LOG(ERROR, DEBUGGER) << "NotifyPaused: GenerateCallFrames failed";
        return;
    }
    tooling::Paused paused;
    paused.SetCallFrames(std::move(callFrames)).SetReason(reason).SetHitBreakpoints(std::move(hitBreakpoints));
    if (reason == EXCEPTION && exception->IsError()) {
        std::unique_ptr<RemoteObject> tmpException = RemoteObject::FromTagged(vm_, exception);
        paused.SetData(std::move(tmpException));
    }
    frontend_.Paused(vm_, paused);

    // Waiting for Debugger
    frontend_.WaitForDebugger(vm_);
    DebuggerApi::SetException(vm_, exception);
}

void DebuggerImpl::NotifyPendingJobEntry()
{
    if (singleStepper_ != nullptr) {
        singleStepper_.reset();
        pauseOnNextByteCode_ = true;
    }
}

void DebuggerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    static std::unordered_map<std::string, AgentHandler> dispatcherTable {
        { "enable", &DebuggerImpl::DispatcherImpl::Enable },
        { "disable", &DebuggerImpl::DispatcherImpl::Disable },
        { "evaluateOnCallFrame", &DebuggerImpl::DispatcherImpl::EvaluateOnCallFrame },
        { "getPossibleBreakpoints", &DebuggerImpl::DispatcherImpl::GetPossibleBreakpoints },
        { "getScriptSource", &DebuggerImpl::DispatcherImpl::GetScriptSource },
        { "pause", &DebuggerImpl::DispatcherImpl::Pause },
        { "removeBreakpoint", &DebuggerImpl::DispatcherImpl::RemoveBreakpoint },
        { "resume", &DebuggerImpl::DispatcherImpl::Resume },
        { "setAsyncCallStackDepth", &DebuggerImpl::DispatcherImpl::SetAsyncCallStackDepth },
        { "setBreakpointByUrl", &DebuggerImpl::DispatcherImpl::SetBreakpointByUrl },
        { "setPauseOnExceptions", &DebuggerImpl::DispatcherImpl::SetPauseOnExceptions },
        { "stepInto", &DebuggerImpl::DispatcherImpl::StepInto },
        { "stepOut", &DebuggerImpl::DispatcherImpl::StepOut },
        { "stepOver", &DebuggerImpl::DispatcherImpl::StepOver }
    };

    const std::string &method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to DebuggerImpl";
    auto entry = dispatcherTable.find(method);
    if (entry != dispatcherTable.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method));
    }
}

void DebuggerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    std::unique_ptr<EnableParams> params = EnableParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    UniqueDebuggerId id;
    DispatchResponse response = debugger_->Enable(*params, &id);

    EnableReturns result(id);
    SendResponse(request, response, result);
}

void DebuggerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->Disable();
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::EvaluateOnCallFrame(const DispatchRequest &request)
{
    std::unique_ptr<EvaluateOnCallFrameParams> params = EvaluateOnCallFrameParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    std::unique_ptr<RemoteObject> result1;
    DispatchResponse response = debugger_->EvaluateOnCallFrame(*params, &result1);

    EvaluateOnCallFrameReturns result(std::move(result1));
    SendResponse(request, response, result);
}

void DebuggerImpl::DispatcherImpl::GetPossibleBreakpoints(const DispatchRequest &request)
{
    std::unique_ptr<GetPossibleBreakpointsParams> params = GetPossibleBreakpointsParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    std::vector<std::unique_ptr<BreakLocation>> locations;
    DispatchResponse response = debugger_->GetPossibleBreakpoints(*params, &locations);
    GetPossibleBreakpointsReturns result(std::move(locations));
    SendResponse(request, response, result);
}

void DebuggerImpl::DispatcherImpl::GetScriptSource(const DispatchRequest &request)
{
    std::unique_ptr<GetScriptSourceParams> params = GetScriptSourceParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    std::string source;
    DispatchResponse response = debugger_->GetScriptSource(*params, &source);
    GetScriptSourceReturns result(source);
    SendResponse(request, response, result);
}

void DebuggerImpl::DispatcherImpl::Pause(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->Pause();
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::RemoveBreakpoint(const DispatchRequest &request)
{
    std::unique_ptr<RemoveBreakpointParams> params = RemoveBreakpointParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = debugger_->RemoveBreakpoint(*params);
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::Resume(const DispatchRequest &request)
{
    std::unique_ptr<ResumeParams> params = ResumeParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = debugger_->Resume(*params);
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::SetAsyncCallStackDepth(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->SetAsyncCallStackDepth();
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::SetBreakpointByUrl(const DispatchRequest &request)
{
    std::unique_ptr<SetBreakpointByUrlParams> params = SetBreakpointByUrlParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    std::string out_id;
    std::vector<std::unique_ptr<Location>> outLocations;
    DispatchResponse response = debugger_->SetBreakpointByUrl(*params, &out_id, &outLocations);
    SetBreakpointByUrlReturns result(out_id, std::move(outLocations));
    SendResponse(request, response, result);
}

void DebuggerImpl::DispatcherImpl::SetPauseOnExceptions(const DispatchRequest &request)
{
    std::unique_ptr<SetPauseOnExceptionsParams> params = SetPauseOnExceptionsParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }

    DispatchResponse response = debugger_->SetPauseOnExceptions(*params);
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::StepInto(const DispatchRequest &request)
{
    std::unique_ptr<StepIntoParams> params = StepIntoParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = debugger_->StepInto(*params);
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::StepOut(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->StepOut();
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::StepOver(const DispatchRequest &request)
{
    std::unique_ptr<StepOverParams> params = StepOverParams::Create(request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("wrong params"));
        return;
    }
    DispatchResponse response = debugger_->StepOver(*params);
    SendResponse(request, response);
}

void DebuggerImpl::DispatcherImpl::SetBlackboxPatterns(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->SetBlackboxPatterns();
    SendResponse(request, response);
}

bool DebuggerImpl::Frontend::AllowNotify(const EcmaVM *vm) const
{
    return vm->GetJsDebuggerManager()->IsDebugMode() && channel_ != nullptr;
}

void DebuggerImpl::Frontend::BreakpointResolved(const EcmaVM *vm)
{
    if (!AllowNotify(vm)) {
        return;
    }

    tooling::BreakpointResolved breakpointResolved;
    channel_->SendNotification(breakpointResolved);
}

void DebuggerImpl::Frontend::Paused(const EcmaVM *vm, const tooling::Paused &paused)
{
    if (!AllowNotify(vm)) {
        return;
    }

    channel_->SendNotification(paused);
}

void DebuggerImpl::Frontend::Resumed(const EcmaVM *vm)
{
    if (!AllowNotify(vm)) {
        return;
    }

    channel_->RunIfWaitingForDebugger();
    tooling::Resumed resumed;
    channel_->SendNotification(resumed);
}

void DebuggerImpl::Frontend::ScriptFailedToParse(const EcmaVM *vm)
{
    if (!AllowNotify(vm)) {
        return;
    }

    tooling::ScriptFailedToParse scriptFailedToParse;
    channel_->SendNotification(scriptFailedToParse);
}

void DebuggerImpl::Frontend::ScriptParsed(const EcmaVM *vm, const PtScript &script)
{
    if (!AllowNotify(vm)) {
        return;
    }

    tooling::ScriptParsed scriptParsed;
    scriptParsed.SetScriptId(script.GetScriptId())
        .SetUrl(script.GetUrl())
        .SetStartLine(0)
        .SetStartColumn(0)
        .SetEndLine(script.GetEndLine())
        .SetEndColumn(0)
        .SetExecutionContextId(0)
        .SetHash(script.GetHash());

    channel_->SendNotification(scriptParsed);
}

void DebuggerImpl::Frontend::WaitForDebugger(const EcmaVM *vm)
{
    if (!AllowNotify(vm)) {
        return;
    }

    channel_->WaitForDebugger();
}

DispatchResponse DebuggerImpl::Enable([[maybe_unused]] const EnableParams &params, UniqueDebuggerId *id)
{
    ASSERT(id != nullptr);
    *id = 0;
    vm_->GetJsDebuggerManager()->SetDebugMode(true);
    for (auto &script : scripts_) {
        frontend_.ScriptParsed(vm_, *script.second);
    }
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::Disable()
{
    vm_->GetJsDebuggerManager()->SetDebugMode(false);
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::EvaluateOnCallFrame(const EvaluateOnCallFrameParams &params,
                                                   std::unique_ptr<RemoteObject> *result)
{
    CallFrameId callFrameId = params.GetCallFrameId();
    const std::string &expression = params.GetExpression();
    if (callFrameId < 0 || callFrameId >= static_cast<CallFrameId>(callFrameHandlers_.size())) {
        return DispatchResponse::Fail("Invalid callFrameId.");
    }

    std::string dest;
    if (!DecodeAndCheckBase64(expression, dest)) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: base64 decode failed";
        auto ret = CmptEvaluateValue(callFrameId, expression, result);
        if (ret.has_value()) {
            LOG(ERROR, DEBUGGER) << "Evaluate fail, expression: " << expression;
        }
        return DispatchResponse::Create(ret);
    }

    auto funcRef = DebuggerApi::GenerateFuncFromBuffer(vm_, dest.data(), dest.size());
    auto res = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(vm_), funcRef,
        callFrameHandlers_[callFrameId]);
    if (vm_->GetJSThread()->HasPendingException()) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: has pending exception";
        std::string msg;
        DebuggerApi::HandleUncaughtException(vm_, msg);
        *result = RemoteObject::FromTagged(vm_,
            Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, msg.data())));
        return DispatchResponse::Fail(msg);
    }

    *result = RemoteObject::FromTagged(vm_, res);
    runtime_->CacheObjectIfNeeded(res, (*result).get());
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::GetPossibleBreakpoints(const GetPossibleBreakpointsParams &params,
                                                      std::vector<std::unique_ptr<BreakLocation>> *locations)
{
    Location *start = params.GetStart();
    auto iter = scripts_.find(start->GetScriptId());
    if (iter == scripts_.end()) {
        return DispatchResponse::Fail("Unknown file name.");
    }
    JSPtExtractor *extractor = GetExtractor(iter->second->GetUrl());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GetPossibleBreakpoints: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }

    int32_t line = start->GetLine();
    int32_t column = start->GetColumn();
    auto callbackFunc = []([[maybe_unused]] File::EntityId id, [[maybe_unused]] uint32_t offset) -> bool {
        return true;
    };
    if (extractor->MatchWithLocation(callbackFunc, line, column)) {
        std::unique_ptr<BreakLocation> location = std::make_unique<BreakLocation>();
        location->SetScriptId(start->GetScriptId()).SetLine(line).SetColumn(column);
        locations->emplace_back(std::move(location));
    }
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::GetScriptSource(const GetScriptSourceParams &params, std::string *source)
{
    ScriptId scriptId = params.GetScriptId();
    auto iter = scripts_.find(scriptId);
    if (iter == scripts_.end()) {
        *source = "";
        return DispatchResponse::Fail("unknown script id: " + std::to_string(scriptId));
    }
    *source = iter->second->GetScriptSource();

    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::Pause()
{
    pauseOnNextByteCode_ = true;
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::RemoveBreakpoint(const RemoveBreakpointParams &params)
{
    std::string id = params.GetBreakpointId();
    LOG(INFO, DEBUGGER) << "RemoveBreakpoint: " << id;
    BreakpointDetails metaData{};
    if (!BreakpointDetails::ParseBreakpointId(id, &metaData)) {
        return DispatchResponse::Fail("Parse breakpoint id failed");
    }
    JSPtExtractor *extractor = GetExtractor(metaData.url_);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }

    std::string fileName;
    auto scriptFunc = [&fileName](PtScript *script) -> bool {
        fileName = script->GetFileName();
        return true;
    };
    if (!MatchScripts(scriptFunc, metaData.url_, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Unknown url: " << metaData.url_;
        return DispatchResponse::Fail("Unknown file name.");
    }

    auto callbackFunc = [this, fileName](File::EntityId id, uint32_t offset) -> bool {
        JSPtLocation location {fileName.c_str(), id, offset};
        return DebuggerApi::RemoveBreakpoint(jsDebugger_, location);
    };
    if (!extractor->MatchWithLocation(callbackFunc, metaData.line_, metaData.column_)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: "
            << metaData.line_ << ":" << metaData.column_;
        return DispatchResponse::Fail("Breakpoint not found.");
    }

    LOG(INFO, DEBUGGER) << "remove breakpoint32_t line number:" << metaData.line_;
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::Resume([[maybe_unused]] const ResumeParams &params)
{
    frontend_.Resumed(vm_);
    singleStepper_.reset();
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::SetAsyncCallStackDepth()
{
    LOG(ERROR, DEBUGGER) << "SetAsyncCallStackDepth not support now.";
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::SetBreakpointByUrl(const SetBreakpointByUrlParams &params,
                                                  std::string *outId,
                                                  std::vector<std::unique_ptr<Location>> *outLocations)
{
    const std::string &url = params.GetUrl();
    int32_t lineNumber = params.GetLine();
    int32_t columnNumber = params.GetColumn();
    auto condition = params.HasCondition() ? params.GetCondition() : std::optional<std::string> {};

    JSPtExtractor *extractor = GetExtractor(url);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }

    ScriptId scriptId;
    std::string fileName;
    auto scriptFunc = [&scriptId, &fileName](PtScript *script) -> bool {
        scriptId = script->GetScriptId();
        fileName = script->GetFileName();
        return true;
    };
    if (!MatchScripts(scriptFunc, url, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: Unknown url: " << url;
        return DispatchResponse::Fail("Unknown file name.");
    }

    auto callbackFunc = [this, fileName, &condition](File::EntityId id, uint32_t offset) -> bool {
        JSPtLocation location {fileName.c_str(), id, offset};
        Local<FunctionRef> condFuncRef = FunctionRef::Undefined(vm_);
        if (condition.has_value() && !condition.value().empty()) {
            std::string dest;
            if (!DecodeAndCheckBase64(condition.value(), dest)) {
                LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: base64 decode failed";
                return false;
            }
            condFuncRef = DebuggerApi::GenerateFuncFromBuffer(vm_, dest.data(), dest.size());
            if (condFuncRef->IsUndefined()) {
                LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: generate function failed";
                return false;
            }
        }
        return DebuggerApi::SetBreakpoint(jsDebugger_, location, condFuncRef);
    };
    if (!extractor->MatchWithLocation(callbackFunc, lineNumber, columnNumber)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: " << lineNumber << ":" << columnNumber;
        return DispatchResponse::Fail("Breakpoint not found.");
    }

    BreakpointDetails metaData{lineNumber, 0, url};
    *outId = BreakpointDetails::ToString(metaData);
    *outLocations = std::vector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(scriptId).SetLine(lineNumber).SetColumn(0);
    outLocations->emplace_back(std::move(location));

    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::SetPauseOnExceptions(const SetPauseOnExceptionsParams &params)
{
    PauseOnExceptionsState state = params.GetState();
    pauseOnException_ = (state != PauseOnExceptionsState::UNCAUGHT);

    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::StepInto([[maybe_unused]] const StepIntoParams &params)
{
    JSMethod *method = DebuggerApi::GetMethod(vm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOver: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }
    singleStepper_ = extractor->GetStepIntoStepper(vm_);

    frontend_.Resumed(vm_);
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::StepOut()
{
    JSMethod *method = DebuggerApi::GetMethod(vm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOut: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }
    singleStepper_ = extractor->GetStepOutStepper(vm_);

    frontend_.Resumed(vm_);
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::StepOver([[maybe_unused]] const StepOverParams &params)
{
    JSMethod *method = DebuggerApi::GetMethod(vm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOver: extractor is null";
        return DispatchResponse::Fail("Unknown file name.");
    }
    singleStepper_ = extractor->GetStepOverStepper(vm_);

    frontend_.Resumed(vm_);
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::SetBlackboxPatterns()
{
    LOG(ERROR, DEBUGGER) << "SetBlackboxPatterns not support now.";
    return DispatchResponse::Ok();
}

void DebuggerImpl::CleanUpOnPaused()
{
    runtime_->curObjectId_ = 0;
    runtime_->properties_.clear();

    callFrameHandlers_.clear();
    scopeObjects_.clear();
}

std::string DebuggerImpl::Trim(const std::string &str)
{
    std::string ret = str;
    // If ret has only ' ', remove all charactors.
    ret.erase(ret.find_last_not_of(' ') + 1);
    // If ret has only ' ', remove all charactors.
    ret.erase(0, ret.find_first_not_of(' '));
    return ret;
}

JSPtExtractor *DebuggerImpl::GetExtractor(const JSPandaFile *jsPandaFile)
{
    return JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
}

JSPtExtractor *DebuggerImpl::GetExtractor(const std::string &url)
{
    auto iter = extractors_.find(url);
    if (iter == extractors_.end()) {
        return nullptr;
    }

    return iter->second;
}

bool DebuggerImpl::GenerateCallFrames(std::vector<std::unique_ptr<CallFrame>> *callFrames)
{
    CallFrameId callFrameId = 0;
    auto walkerFunc = [this, &callFrameId, &callFrames](const InterpretedFrameHandler *frameHandler) -> StackState {
        JSMethod *method = DebuggerApi::GetMethod(frameHandler);
        if (method->IsNativeWithCallField()) {
            LOG(INFO, DEBUGGER) << "GenerateCallFrames: Skip CFrame and Native method";
            return StackState::CONTINUE;
        }
        std::unique_ptr<CallFrame> callFrame = std::make_unique<CallFrame>();
        if (!GenerateCallFrame(callFrame.get(), frameHandler, callFrameId)) {
            if (callFrameId == 0) {
                return StackState::FAILED;
            }
        } else {
            SaveCallFrameHandler(frameHandler);
            callFrames->emplace_back(std::move(callFrame));
            callFrameId++;
        }
        return StackState::CONTINUE;
    };
    return DebuggerApi::StackWalker(vm_, walkerFunc);
}

void DebuggerImpl::SaveCallFrameHandler(const InterpretedFrameHandler *frameHandler)
{
    auto handlerPtr = DebuggerApi::NewFrameHandler(vm_);
    *handlerPtr = *frameHandler;
    callFrameHandlers_.emplace_back(handlerPtr);
}

bool DebuggerImpl::GenerateCallFrame(CallFrame *callFrame,
    const InterpretedFrameHandler *frameHandler, CallFrameId callFrameId)
{
    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: extractor is null";
        return false;
    }

    // location
    std::unique_ptr<Location> location = std::make_unique<Location>();
    std::string url = extractor->GetSourceFile(method->GetMethodId());
    auto scriptFunc = [&location](PtScript *script) -> bool {
        location->SetScriptId(script->GetScriptId());
        return true;
    };
    if (!MatchScripts(scriptFunc, url, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: Unknown url: " << url;
        return false;
    }
    auto callbackLineFunc = [&location](int32_t line) -> bool {
        location->SetLine(line);
        return true;
    };
    auto callbackColumnFunc = [&location](int32_t column) -> bool {
        location->SetColumn(column);
        return true;
    };
    File::EntityId methodId = method->GetMethodId();
    if (!extractor->MatchLineWithOffset(callbackLineFunc, methodId, DebuggerApi::GetBytecodeOffset(frameHandler)) ||
        !extractor->MatchColumnWithOffset(callbackColumnFunc, methodId, DebuggerApi::GetBytecodeOffset(frameHandler))) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: unknown offset: " << DebuggerApi::GetBytecodeOffset(frameHandler);
        return false;
    }

    // scopeChain & this
    std::unique_ptr<RemoteObject> thisObj = std::make_unique<RemoteObject>();
    thisObj->SetType(ObjectType::Undefined);

    std::vector<std::unique_ptr<Scope>> scopeChain;
    scopeChain.emplace_back(GetLocalScopeChain(frameHandler, &thisObj));
    scopeChain.emplace_back(GetGlobalScopeChain());

    // functionName
    std::string functionName = DebuggerApi::ParseFunctionName(method);

    callFrame->SetCallFrameId(callFrameId)
        .SetFunctionName(functionName)
        .SetLocation(std::move(location))
        .SetUrl(url)
        .SetScopeChain(std::move(scopeChain))
        .SetThis(std::move(thisObj));
    return true;
}

std::unique_ptr<Scope> DebuggerImpl::GetLocalScopeChain(const InterpretedFrameHandler *frameHandler,
    std::unique_ptr<RemoteObject> *thisObj)
{
    auto localScope = std::make_unique<Scope>();

    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GetScopeChain: extractor is null";
        return localScope;
    }

    std::unique_ptr<RemoteObject> local = std::make_unique<RemoteObject>();
    Local<ObjectRef> localObj = ObjectRef::New(vm_);
    local->SetType(ObjectType::Object)
        .SetObjectId(runtime_->curObjectId_)
        .SetClassName(ObjectClassName::Object)
        .SetDescription(RemoteObject::ObjectDescription);
    auto *sp = DebuggerApi::GetSp(frameHandler);
    scopeObjects_[sp] = runtime_->curObjectId_;
    runtime_->properties_[runtime_->curObjectId_++] = Global<JSValueRef>(vm_, localObj);

    Local<JSValueRef> thisVal = JSValueRef::Undefined(vm_);
    GetLocalVariables(frameHandler, method, thisVal, localObj);
    *thisObj = RemoteObject::FromTagged(vm_, thisVal);
    runtime_->CacheObjectIfNeeded(thisVal, (*thisObj).get());

    auto methodId = method->GetMethodId();
    const LineNumberTable &lines = extractor->GetLineNumberTable(methodId);
    std::unique_ptr<Location> startLoc = std::make_unique<Location>();
    std::unique_ptr<Location> endLoc = std::make_unique<Location>();
    auto scriptFunc = [&startLoc, &endLoc, lines](PtScript *script) -> bool {
        startLoc->SetScriptId(script->GetScriptId())
            .SetLine(lines.front().line)
            .SetColumn(0);
        endLoc->SetScriptId(script->GetScriptId())
            .SetLine(lines.back().line + 1)
            .SetColumn(0);
        return true;
    };
    if (MatchScripts(scriptFunc, extractor->GetSourceFile(methodId), ScriptMatchType::URL)) {
        localScope->SetType(Scope::Type::Local())
            .SetObject(std::move(local))
            .SetStartLocation(std::move(startLoc))
            .SetEndLocation(std::move(endLoc));
    }

    return localScope;
}

void DebuggerImpl::GetLocalVariables(const InterpretedFrameHandler *frameHandler, const JSMethod *method,
    Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj)
{
    auto methodId = method->GetMethodId();
    auto *extractor = GetExtractor(method->GetJSPandaFile());
    Local<JSValueRef> value = JSValueRef::Undefined(vm_);
    // in case of arrow function, which doesn't have this in local variable table
    bool hasThis = false;
    for (const auto &[varName, regIndex] : extractor->GetLocalVariableTable(methodId)) {
        value = DebuggerApi::GetVRegValue(vm_, frameHandler, regIndex);
        if (varName == "4newTarget") {
            continue;
        }

        if (varName == "this") {
            thisVal = value;
            hasThis = true;
            continue;
        }
        Local<JSValueRef> name = JSValueRef::Undefined(vm_);
        if (varName == "4funcObj") {
            if (value->IsFunction()) {
                auto funcName = Local<FunctionRef>(value)->GetName(vm_)->ToString();
                name = StringRef::NewFromUtf8(vm_, funcName.c_str());
            } else {
                continue;
            }
        } else {
            name = StringRef::NewFromUtf8(vm_, varName.c_str());
        }
        PropertyAttribute descriptor(value, true, true, true);
        localObj->DefineProperty(vm_, name, descriptor);
    }

    // closure variables are stored in env
    JSTaggedValue env = DebuggerApi::GetEnv(frameHandler);
    if (env.IsTaggedArray() && DebuggerApi::GetBytecodeOffset(frameHandler) != 0) {
        LexicalEnv *lexEnv = LexicalEnv::Cast(env.GetTaggedObject());
        if (lexEnv->GetScopeInfo().IsHole()) {
            return;
        }
        auto ptr = JSNativePointer::Cast(lexEnv->GetScopeInfo().GetTaggedObject())->GetExternalPointer();
        auto *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(ptr);
        JSThread *thread = vm_->GetJSThread();
        for (const auto &info : scopeDebugInfo->scopeInfo) {
            // skip possible duplicate variables both in local variable table and env
            if (info.name == "4newTarget") {
                continue;
            }
            value = JSNApiHelper::ToLocal<JSValueRef>(
                JSHandle<JSTaggedValue>(thread, lexEnv->GetProperties(info.slot)));
            if (info.name == "this") {
                if (!hasThis) {
                    thisVal = value;
                }
                continue;
            }
            Local<JSValueRef> name = StringRef::NewFromUtf8(vm_, info.name.c_str());
            PropertyAttribute descriptor(value, true, true, true);
            localObj->DefineProperty(vm_, name, descriptor);
        }
    }
}

std::unique_ptr<Scope> DebuggerImpl::GetGlobalScopeChain()
{
    auto globalScope = std::make_unique<Scope>();

    std::unique_ptr<RemoteObject> global = std::make_unique<RemoteObject>();
    global->SetType(ObjectType::Object)
        .SetObjectId(runtime_->curObjectId_)
        .SetClassName(ObjectClassName::Global)
        .SetDescription(RemoteObject::GlobalDescription);
    globalScope->SetType(Scope::Type::Global()).SetObject(std::move(global));
    runtime_->properties_[runtime_->curObjectId_++] = Global<JSValueRef>(vm_, JSNApi::GetGlobalObject(vm_));
    return globalScope;
}

void DebuggerImpl::UpdateScopeObject(const InterpretedFrameHandler *frameHandler,
    std::string_view varName, Local<JSValueRef> newVal)
{
    auto *sp = DebuggerApi::GetSp(frameHandler);
    auto iter = scopeObjects_.find(sp);
    if (iter == scopeObjects_.end()) {
        LOG(ERROR, DEBUGGER) << "UpdateScopeObject: object not found";
        return;
    }

    auto objectId = iter->second;
    Local<ObjectRef> localObj = runtime_->properties_[objectId].ToLocal(vm_);
    Local<JSValueRef> name = StringRef::NewFromUtf8(vm_, varName.data());
    if (localObj->Has(vm_, name)) {
        LOG(DEBUG, DEBUGGER) << "UpdateScopeObject: set new value";
        PropertyAttribute descriptor(newVal, true, true, true);
        localObj->DefineProperty(vm_, name, descriptor);
    } else {
        LOG(ERROR, DEBUGGER) << "UpdateScopeObject: not found " << varName;
    }
}

std::optional<std::string> DebuggerImpl::CmptEvaluateValue(CallFrameId callFrameId, const std::string &expression,
    std::unique_ptr<RemoteObject> *result)
{
    JSMethod *method = DebuggerApi::GetMethod(vm_);
    if (method->IsNativeWithCallField()) {
        *result = RemoteObject::FromTagged(vm_,
            Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, "Native Frame not support.")));
        return "Native Frame not support.";
    }
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        *result = RemoteObject::FromTagged(vm_,
            Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, "Internal error.")));
        return "Internal error.";
    }
    std::string varName = expression;
    std::string varValue;
    std::string::size_type indexEqual = expression.find_first_of('=', 0);
    if (indexEqual != std::string::npos) {
        varName = Trim(expression.substr(0, indexEqual));
        varValue = Trim(expression.substr(indexEqual + 1, expression.length()));
    }

    Local<StringRef> name = StringRef::NewFromUtf8(vm_, varName.c_str());
    InterpretedFrameHandler *frameHandler = callFrameHandlers_[callFrameId].get();
    if (varValue.empty()) {
        Local<JSValueRef> ret = DebuggerExecutor::GetValue(vm_, frameHandler, name);
        if (!ret.IsEmpty() && !ret->IsException()) {
            *result = RemoteObject::FromTagged(vm_, ret);
            runtime_->CacheObjectIfNeeded(ret, (*result).get());
            return {};
        }
    } else {
        Local<JSValueRef> value = ConvertToLocal(varValue);
        if (value.IsEmpty()) {
            return "Unsupported expression.";
        }
        JsDebuggerManager *mgr = vm_->GetJsDebuggerManager();
        mgr->SetEvalFrameHandler(callFrameHandlers_[callFrameId]);
        bool ret = DebuggerExecutor::SetValue(vm_, frameHandler, name, value);
        mgr->SetEvalFrameHandler(nullptr);
        if (ret) {
            *result = RemoteObject::FromTagged(vm_, value);
            return {};
        }
    }

    *result = RemoteObject::FromTagged(vm_,
        Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, "Unsupported expression.")));
    return "Unsupported expression.";
}

Local<JSValueRef> DebuggerImpl::ConvertToLocal(const std::string &varValue)
{
    Local<JSValueRef> taggedValue;
    if (varValue == "false") {
        taggedValue = JSValueRef::False(vm_);
    } else if (varValue == "true") {
        taggedValue = JSValueRef::True(vm_);
    } else if (varValue == "undefined") {
        taggedValue = JSValueRef::Undefined(vm_);
    } else if (varValue[0] == '\"' && varValue[varValue.length() - 1] == '\"') {
        // 2 : 2 means length
        taggedValue = StringRef::NewFromUtf8(vm_, varValue.substr(1, varValue.length() - 2).c_str());
    } else {
        auto begin = reinterpret_cast<const uint8_t *>((varValue.c_str()));
        auto end = begin + varValue.length();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        double d = DebuggerApi::StringToDouble(begin, end, 0);
        if (!std::isnan(d)) {
            taggedValue = NumberRef::New(vm_, d);
        }
    }
    return taggedValue;
}

bool DebuggerImpl::DecodeAndCheckBase64(const std::string &src, std::string &dest)
{
    dest.resize(base64::decoded_size(src.size()));
    auto [numOctets, _] = base64::decode(dest.data(), src.data(), src.size());
    dest.resize(numOctets);
    if (numOctets > File::MAGIC_SIZE &&
        memcmp(dest.data(), File::MAGIC.data(), File::MAGIC_SIZE) == 0) {
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript::tooling
