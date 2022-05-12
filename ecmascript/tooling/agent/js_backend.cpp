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

#include "ecmascript/tooling/agent/js_backend.h"

#include <boost/beast/core/detail/base64.hpp>
#include <iomanip>

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/front_end.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript::tooling {
using namespace boost::beast::detail;
using namespace std::placeholders;

using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

#ifdef DEBUGGER_TEST
const CString DATA_APP_PATH = "/";
#else
const CString DATA_APP_PATH = "/data/";
#endif

JSBackend::JSBackend(FrontEnd *frontend) : frontend_(frontend)
{
    ecmaVm_ = static_cast<ProtocolHandler *>(frontend)->GetEcmaVM();
    hooks_ = std::make_unique<JSPtHooks>(this);

    debugger_ = DebuggerApi::CreateJSDebugger(ecmaVm_);
    DebuggerApi::InitJSDebugger(debugger_);
    DebuggerApi::RegisterHooks(debugger_, hooks_.get());

    ecmaVm_->GetJsDebuggerManager()->SetLocalScopeUpdater(
        std::bind(&JSBackend::UpdateScopeObject, this, _1, _2, _3));
}

JSBackend::JSBackend(const EcmaVM *vm) : ecmaVm_(vm)
{
    // For testcases
    debugger_ = DebuggerApi::CreateJSDebugger(ecmaVm_);
}

JSBackend::~JSBackend()
{
    DebuggerApi::DestroyJSDebugger(debugger_);
}

void JSBackend::WaitForDebugger()
{
    frontend_->WaitForDebugger();
}

void JSBackend::NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason)
{
    if (!pauseOnException_ && reason == EXCEPTION) {
        return;
    }
    Local<JSValueRef> exception = DebuggerApi::GetAndClearException(ecmaVm_);

    CVector<CString> hitBreakpoints;
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
    CVector<std::unique_ptr<CallFrame>> callFrames;
    if (!GenerateCallFrames(&callFrames)) {
        LOG(ERROR, DEBUGGER) << "NotifyPaused: GenerateCallFrames failed";
        return;
    }
    std::unique_ptr<Paused> paused = std::make_unique<Paused>();
    paused->SetCallFrames(std::move(callFrames)).SetReason(reason).SetHitBreakpoints(std::move(hitBreakpoints));
    if (reason == EXCEPTION && exception->IsError()) {
        std::unique_ptr<RemoteObject> tmpException = RemoteObject::FromTagged(ecmaVm_, exception);
        paused->SetData(std::move(tmpException));
    }
    frontend_->SendNotification(ecmaVm_, std::move(paused));

    // Waiting for Debugger
    frontend_->WaitForDebugger();
    if (!exception->IsHole()) {
        DebuggerApi::SetException(ecmaVm_, exception);
    }
}

void JSBackend::NotifyResume()
{
    frontend_->RunIfWaitingForDebugger();
    // Notify resumed event
    frontend_->SendNotification(ecmaVm_, std::make_unique<Resumed>());
}

void JSBackend::NotifyAllScriptParsed()
{
    for (auto &script : scripts_) {
        if (frontend_ != nullptr) {
            frontend_->SendNotification(ecmaVm_, ScriptParsed::Create(script.second));
        }
    }
}

bool JSBackend::NotifyScriptParsed(ScriptId scriptId, const CString &fileName)
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
        if (pf->GetJSPandaFileDesc() == fileName) {
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
    const CString &source = extractor->GetSourceCode(mainMethodIndex);
    const CString &url = extractor->GetSourceFile(mainMethodIndex);
    const uint32_t MIN_SOURCE_CODE_LENGTH = 5;  // maybe return 'ANDA' when source code is empty
    if (source.size() < MIN_SOURCE_CODE_LENGTH) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: invalid file: " << fileName;
        return false;
    }
    // store here for performance of get extractor from url
    extractors_[url] = extractor;

    // Notify script parsed event
    std::unique_ptr<PtScript> script = std::make_unique<PtScript>(scriptId, fileName, url, source);

    if (frontend_ != nullptr) {
        frontend_->SendNotification(ecmaVm_, ScriptParsed::Create(script));
    }

    // Store parsed script in map
    scripts_[script->GetScriptId()] = std::move(script);
    return true;
}

bool JSBackend::StepComplete(const JSPtLocation &location)
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

bool JSBackend::IsSkipLine(const JSPtLocation &location)
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
        return line == SPECIAL_LINE_MARK;
    };
    File::EntityId methodId = location.GetMethodId();
    uint32_t offset = location.GetBytecodeOffset();
    if (extractor->MatchLineWithOffset(callbackFunc, methodId, offset)) {
        LOG(INFO, DEBUGGER) << "StepComplete: skip -1";
        return true;
    }

    return false;
}

std::optional<CString> JSBackend::GetPossibleBreakpoints(Location *start, [[maybe_unused]] Location *end,
    CVector<std::unique_ptr<BreakLocation>> *locations)
{
    auto iter = scripts_.find(start->GetScriptId());
    if (iter == scripts_.end()) {
        return "Unknown file name.";
    }
    JSPtExtractor *extractor = GetExtractor(iter->second->GetUrl());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GetPossibleBreakpoints: extractor is null";
        return "Unknown file name.";
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

    return {};
}

std::optional<CString> JSBackend::SetBreakpointByUrl(const CString &url, int32_t lineNumber,
    int32_t columnNumber, const std::optional<CString> &condition, CString *outId,
    CVector<std::unique_ptr<Location>> *outLocations)
{
    JSPtExtractor *extractor = GetExtractor(url);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: extractor is null";
        return "Unknown file name.";
    }

    ScriptId scriptId;
    CString fileName;
    auto scriptFunc = [&scriptId, &fileName](PtScript *script) -> bool {
        scriptId = script->GetScriptId();
        fileName = script->GetFileName();
        return true;
    };
    if (!MatchScripts(scriptFunc, url, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: Unknown url: " << url;
        return "Unknown file name.";
    }

    auto callbackFunc = [this, fileName, &condition](File::EntityId id, uint32_t offset) -> bool {
        JSPtLocation location {fileName.c_str(), id, offset};
        Local<FunctionRef> condFuncRef = FunctionRef::Undefined(ecmaVm_);
        if (condition.has_value() && !condition.value().empty()) {
            CString dest;
            if (!DecodeAndCheckBase64(condition.value(), dest)) {
                LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: base64 decode failed";
                return false;
            }
            condFuncRef = DebuggerApi::GenerateFuncFromBuffer(ecmaVm_, dest.data(), dest.size(),
                JSPandaFile::ENTRY_MAIN_FUNCTION);
            if (condFuncRef->IsUndefined()) {
                LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: generate function failed";
                return false;
            }
        }
        return DebuggerApi::SetBreakpoint(debugger_, location, condFuncRef);
    };
    if (!extractor->MatchWithLocation(callbackFunc, lineNumber, columnNumber)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: " << lineNumber << ":" << columnNumber;
        return "Breakpoint not found.";
    }

    BreakpointDetails metaData{lineNumber, 0, url};
    *outId = BreakpointDetails::ToString(metaData);
    *outLocations = CVector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(scriptId).SetLine(lineNumber).SetColumn(0);
    outLocations->emplace_back(std::move(location));

    return {};
}

std::optional<CString> JSBackend::RemoveBreakpoint(const BreakpointDetails &metaData)
{
    JSPtExtractor *extractor = GetExtractor(metaData.url_);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: extractor is null";
        return "Unknown file name.";
    }

    CString fileName;
    auto scriptFunc = [&fileName](PtScript *script) -> bool {
        fileName = script->GetFileName();
        return true;
    };
    if (!MatchScripts(scriptFunc, metaData.url_, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Unknown url: " << metaData.url_;
        return "Unknown file name.";
    }

    auto callbackFunc = [this, fileName](File::EntityId id, uint32_t offset) -> bool {
        JSPtLocation location {fileName.c_str(), id, offset};
        return DebuggerApi::RemoveBreakpoint(debugger_, location);
    };
    if (!extractor->MatchWithLocation(callbackFunc, metaData.line_, metaData.column_)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: "
            << metaData.line_ << ":" << metaData.column_;
        return "Breakpoint not found.";
    }

    LOG(INFO, DEBUGGER) << "remove breakpoint32_t line number:" << metaData.line_;
    return {};
}

std::optional<CString> JSBackend::Pause()
{
    pauseOnNextByteCode_ = true;
    return {};
}

std::optional<CString> JSBackend::Resume()
{
    singleStepper_.reset();

    NotifyResume();
    return {};
}

std::optional<CString> JSBackend::StepInto()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepInto: extractor is null";
        return "Unknown file name.";
    }

    singleStepper_ = extractor->GetStepIntoStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<CString> JSBackend::StepOver()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOver: extractor is null";
        return "Unknown file name.";
    }

    singleStepper_ = extractor->GetStepOverStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<CString> JSBackend::StepOut()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOut: extractor is null";
        return "Unknown file name.";
    }

    singleStepper_ = extractor->GetStepOutStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<CString> JSBackend::CmptEvaluateValue(CallFrameId callFrameId, const CString &expression,
    std::unique_ptr<RemoteObject> *result)
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    if (method->IsNativeWithCallField()) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Native Frame not support.")));
        return "Native Frame not support.";
    }
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Internal error.")));
        return "Internal error.";
    }
    CString varName = expression;
    CString varValue;
    CString::size_type indexEqual = expression.find_first_of('=', 0);
    if (indexEqual != CString::npos) {
        varName = Trim(expression.substr(0, indexEqual));
        varValue = Trim(expression.substr(indexEqual + 1, expression.length()));
    }

    if (!varValue.empty() && callFrameId != 0) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Native Frame not support.")));
        return "Native Frame not support.";
    }

    int32_t regIndex = -1;
    auto varInfos = extractor->GetLocalVariableTable(method->GetMethodId());
    auto iter = varInfos.find(varName.c_str());
    if (iter != varInfos.end()) {
        regIndex = iter->second;
    }
    if (regIndex != -1) {
        if (varValue.empty()) {
            return GetVregValue(regIndex, result);
        }
        return SetVregValue(regIndex, varValue, result);
    }
    int32_t level = 0;
    uint32_t slot = 0;
    if (!DebuggerApi::EvaluateLexicalValue(ecmaVm_, varName.c_str(), level, slot)) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupported expression.")));
        return "Unsupported expression.";
    }
    if (varValue.empty()) {
        return GetLexicalValue(level, slot, result);
    }
    return SetLexicalValue(level, slot, varValue, result);
}

std::optional<CString> JSBackend::EvaluateValue(CallFrameId callFrameId, const CString &expression,
    std::unique_ptr<RemoteObject> *result)
{
    if (callFrameId < 0 || callFrameId >= callFrameHandlers_.size()) {
        return "Invalid callFrameId.";
    }

    CString dest;
    if (!DecodeAndCheckBase64(expression, dest)) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: base64 decode failed";
        return CmptEvaluateValue(callFrameId, expression, result);
    }

    auto funcRef = DebuggerApi::GenerateFuncFromBuffer(ecmaVm_, dest.data(), dest.size(),
        JSPandaFile::ENTRY_MAIN_FUNCTION);
    auto res = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_), funcRef,
        callFrameHandlers_[callFrameId]);
    if (ecmaVm_->GetJSThread()->HasPendingException()) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: has pending exception";
        CString msg;
        DebuggerApi::HandleUncaughtException(ecmaVm_, msg);
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, msg.data())));
        return msg;
    }

    CacheObjectIfNeeded(res, result);
    return {};
}

CString JSBackend::Trim(const CString &str)
{
    CString ret = str;
    // If ret has only ' ', remove all charactors.
    ret.erase(ret.find_last_not_of(' ') + 1);
    // If ret has only ' ', remove all charactors.
    ret.erase(0, ret.find_first_not_of(' '));
    return ret;
}

JSPtExtractor *JSBackend::GetExtractor(const JSPandaFile *jsPandaFile)
{
    return JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
}

JSPtExtractor *JSBackend::GetExtractor(const CString &url)
{
    auto iter = extractors_.find(url);
    if (iter == extractors_.end()) {
        return nullptr;
    }

    return iter->second;
}

bool JSBackend::GenerateCallFrames(CVector<std::unique_ptr<CallFrame>> *callFrames)
{
    CallFrameId callFrameId = 0;
    auto walkerFunc = [this, &callFrameId, &callFrames](const FrameHandler *frameHandler) -> StackState {
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
    return DebuggerApi::StackWalker(ecmaVm_, walkerFunc);
}

void JSBackend::SaveCallFrameHandler(const FrameHandler *frameHandler)
{
    auto handlerPtr = DebuggerApi::NewFrameHandler(ecmaVm_);
    *handlerPtr = *frameHandler;
    callFrameHandlers_.emplace_back(handlerPtr);
}

bool JSBackend::GenerateCallFrame(CallFrame *callFrame,
    const FrameHandler *frameHandler, CallFrameId callFrameId)
{
    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    JSPtExtractor *extractor = GetExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: extractor is null";
        return false;
    }

    // location
    std::unique_ptr<Location> location = std::make_unique<Location>();
    CString url = extractor->GetSourceFile(method->GetMethodId());
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

    CVector<std::unique_ptr<Scope>> scopeChain;
    scopeChain.emplace_back(GetLocalScopeChain(frameHandler, &thisObj));
    scopeChain.emplace_back(GetGlobalScopeChain());

    // functionName
    CString functionName = DebuggerApi::ParseFunctionName(method);

    callFrame->SetCallFrameId(callFrameId)
        .SetFunctionName(functionName)
        .SetLocation(std::move(location))
        .SetUrl(url)
        .SetScopeChain(std::move(scopeChain))
        .SetThis(std::move(thisObj));
    return true;
}

std::unique_ptr<Scope> JSBackend::GetLocalScopeChain(const FrameHandler *frameHandler,
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
    Local<ObjectRef> localObj(local->NewObject(ecmaVm_));
    local->SetType(ObjectType::Object)
        .SetObjectId(curObjectId_)
        .SetClassName(ObjectClassName::Object)
        .SetDescription(RemoteObject::ObjectDescription);
    auto *sp = DebuggerApi::GetSp(frameHandler);
    scopeObjects_[sp] = curObjectId_;
    propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, localObj);

    Local<JSValueRef> thisVal = JSValueRef::Undefined(ecmaVm_);
    GetLocalVariables(frameHandler, method, thisVal, localObj);
    CacheObjectIfNeeded(thisVal, thisObj);

    auto methodId = method->GetMethodId();
    auto lines = extractor->GetLineNumberTable(methodId);
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

void JSBackend::GetLocalVariables(const FrameHandler *frameHandler, const JSMethod *method,
    Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj)
{
    auto methodId = method->GetMethodId();
    auto *extractor = GetExtractor(method->GetJSPandaFile());
    Local<JSValueRef> value = JSValueRef::Undefined(ecmaVm_);
    for (const auto &[varName, regIndex] : extractor->GetLocalVariableTable(methodId)) {
        value = DebuggerApi::GetVRegValue(ecmaVm_, frameHandler, regIndex);
        if (varName == "this") {
            thisVal = value;
        } else {
            Local<JSValueRef> name = StringRef::NewFromUtf8(ecmaVm_, varName.c_str());
            PropertyAttribute descriptor(value, true, true, true);
            localObj->DefineProperty(ecmaVm_, name, descriptor);
        }
    }
    if (thisVal->IsUndefined()) {
        thisVal = DebuggerApi::GetLexicalValueInfo(ecmaVm_, "this");
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
        JSThread *thread = ecmaVm_->GetJSThread();
        for (const auto &[varName, slot] : scopeDebugInfo->scopeInfo) {
            Local<JSValueRef> name = StringRef::NewFromUtf8(ecmaVm_, varName.c_str());
            value = JSNApiHelper::ToLocal<JSValueRef>(
                JSHandle<JSTaggedValue>(thread, lexEnv->GetProperties(slot)));
            PropertyAttribute descriptor(value, true, true, true);
            localObj->DefineProperty(ecmaVm_, name, descriptor);
        }
    }
}

void JSBackend::UpdateScopeObject(const FrameHandler *frameHandler,
    const CString &varName, const Local<JSValueRef> &newVal)
{
    auto *sp = DebuggerApi::GetSp(frameHandler);
    auto iter = scopeObjects_.find(sp);
    if (iter == scopeObjects_.end()) {
        LOG(ERROR, DEBUGGER) << "UpdateScopeObject: object not found";
        return;
    }

    auto objectId = iter->second;
    Local<ObjectRef> localObj = propertiesPair_[objectId].ToLocal(ecmaVm_);
    Local<JSValueRef> name = StringRef::NewFromUtf8(ecmaVm_, varName.c_str());
    if (localObj->Has(ecmaVm_, name)) {
        LOG(DEBUG, DEBUGGER) << "UpdateScopeObject: set new value";
        PropertyAttribute descriptor(newVal, true, true, true);
        localObj->DefineProperty(ecmaVm_, name, descriptor);
    } else {
        LOG(ERROR, DEBUGGER) << "UpdateScopeObject: not found " << varName;
    }
}

std::unique_ptr<Scope> JSBackend::GetGlobalScopeChain()
{
    auto globalScope = std::make_unique<Scope>();

    std::unique_ptr<RemoteObject> global = std::make_unique<RemoteObject>();
    global->SetType(ObjectType::Object)
        .SetObjectId(curObjectId_)
        .SetClassName(ObjectClassName::Global)
        .SetDescription(RemoteObject::GlobalDescription);
    globalScope->SetType(Scope::Type::Global()).SetObject(std::move(global));
    propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, JSNApi::GetGlobalObject(ecmaVm_));
    return globalScope;
}

bool JSBackend::GetScriptSource(ScriptId scriptId, CString *source)
{
    auto iter = scripts_.find(scriptId);
    if (iter == scripts_.end()) {
        *source = "";
        return false;
    }

    *source = iter->second->GetScriptSource();
    return true;
}

void JSBackend::SetPauseOnException(bool flag)
{
    pauseOnException_ = flag;
}

std::optional<CString> JSBackend::ConvertToLocal(Local<JSValueRef> &taggedValue, std::unique_ptr<RemoteObject> *result,
    const CString &varValue)
{
    if (varValue == "false") {
        taggedValue = JSValueRef::False(ecmaVm_);
    } else if (varValue == "true") {
        taggedValue = JSValueRef::True(ecmaVm_);
    } else if (varValue == "undefined") {
        taggedValue = JSValueRef::Undefined(ecmaVm_);
    } else if (varValue[0] == '\"' && varValue[varValue.length() - 1] == '\"') {
        // 2 : 2 means length
        taggedValue = StringRef::NewFromUtf8(ecmaVm_, varValue.substr(1, varValue.length() - 2).c_str());
    } else {
        auto begin = reinterpret_cast<const uint8_t *>((varValue.c_str()));
        auto end = begin + varValue.length();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        double d = DebuggerApi::StringToDouble(begin, end, 0);
        if (std::isnan(d)) {
            *result = RemoteObject::FromTagged(ecmaVm_,
                Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupported expression.")));
            return "Unsupported expression.";
        }
        taggedValue = NumberRef::New(ecmaVm_, d);
    }
    return {};
}

std::optional<CString> JSBackend::SetVregValue(int32_t regIndex, const CString &varValue,
    std::unique_ptr<RemoteObject> *result)
{
    Local<JSValueRef> taggedValue;
    std::optional<CString> ret = ConvertToLocal(taggedValue, result, varValue);
    if (ret.has_value()) {
        return ret;
    }
    DebuggerApi::SetVRegValue(ecmaVm_, regIndex, taggedValue);
    *result = RemoteObject::FromTagged(ecmaVm_, taggedValue);
    return {};
}

std::optional<CString> JSBackend::SetLexicalValue(int32_t level, uint32_t slot, const CString &varValue,
    std::unique_ptr<RemoteObject> *result)
{
    Local<JSValueRef> taggedValue;
    std::optional<CString> ret = ConvertToLocal(taggedValue, result, varValue);
    if (ret.has_value()) {
        return ret;
    }
    DebuggerApi::SetProperties(ecmaVm_, level, slot, taggedValue);
    *result = RemoteObject::FromTagged(ecmaVm_, taggedValue);
    return {};
}

std::optional<CString> JSBackend::GetVregValue(int32_t regIndex, std::unique_ptr<RemoteObject> *result)
{
    CacheObjectIfNeeded(DebuggerApi::GetVRegValue(ecmaVm_, regIndex), result);
    return {};
}

std::optional<CString> JSBackend::GetLexicalValue(int32_t level, uint32_t slot, std::unique_ptr<RemoteObject> *result)
{
    CacheObjectIfNeeded(DebuggerApi::GetProperties(ecmaVm_, level, slot), result);
    return {};
}

void JSBackend::GetProtoOrProtoType(const Local<JSValueRef> &value, bool isOwn, bool isAccessorOnly,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    if (!isAccessorOnly && isOwn && !value->IsProxy()) {
        return;
    }
    // Get Function ProtoOrDynClass
    if (value->IsConstructor()) {
        Local<JSValueRef> prototype = Local<FunctionRef>(value)->GetFunctionPrototype(ecmaVm_);
        std::unique_ptr<RemoteObject> protoObj = std::make_unique<RemoteObject>();
        CacheObjectIfNeeded(prototype, &protoObj);
        std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
        debuggerProperty->SetName("prototype")
            .SetWritable(false)
            .SetConfigurable(false)
            .SetEnumerable(false)
            .SetIsOwn(true)
            .SetValue(std::move(protoObj));
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    // Get __proto__
    Local<JSValueRef> proto = Local<ObjectRef>(value)->GetPrototype(ecmaVm_);
    std::unique_ptr<RemoteObject> protoObj = std::make_unique<RemoteObject>();
    CacheObjectIfNeeded(proto, &protoObj);
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName("__proto__")
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(protoObj));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void JSBackend::GetProperties(RemoteObjectId objectId, bool isOwn, bool isAccessorOnly,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    auto iter = propertiesPair_.find(objectId);
    if (iter == propertiesPair_.end()) {
        LOG(ERROR, DEBUGGER) << "JSBackend::GetProperties Unknown object id: " << objectId;
        return;
    }
    Local<JSValueRef> value = Local<JSValueRef>(ecmaVm_, iter->second);
    if (value.IsEmpty() || !value->IsObject()) {
        LOG(ERROR, DEBUGGER) << "JSBackend::GetProperties should a js object";
        return;
    }
    if (value->IsArrayBuffer()) {
        Local<ArrayBufferRef> arrayBufferRef(value);
        AddTypedArrayRefs(arrayBufferRef, outPropertyDesc);
    }
    Local<ArrayRef> keys = Local<ObjectRef>(value)->GetOwnPropertyNames(ecmaVm_);
    int32_t length = keys->Length(ecmaVm_);
    Local<JSValueRef> name = JSValueRef::Undefined(ecmaVm_);
    for (int32_t i = 0; i < length; ++i) {
        name = keys->Get(ecmaVm_, i);
        PropertyAttribute jsProperty = PropertyAttribute::Default();
        if (!Local<ObjectRef>(value)->GetOwnProperty(ecmaVm_, name, jsProperty)) {
            continue;
        }
        std::unique_ptr<PropertyDescriptor> debuggerProperty =
            PropertyDescriptor::FromProperty(ecmaVm_, name, jsProperty);
        if (isAccessorOnly && !jsProperty.HasGetter() && !jsProperty.HasSetter()) {
            continue;
        }
        if (jsProperty.HasGetter()) {
            debuggerProperty->GetGet()->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, jsProperty.GetGetter(ecmaVm_));
        }
        if (jsProperty.HasSetter()) {
            debuggerProperty->GetSet()->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, jsProperty.GetSetter(ecmaVm_));
        }
        if (jsProperty.HasValue()) {
            Local<JSValueRef> vValue = jsProperty.GetValue(ecmaVm_);
            if (vValue->IsObject() && !vValue->IsProxy()) {
                debuggerProperty->GetValue()->SetObjectId(curObjectId_);
                propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, vValue);
            }
        }
        if (name->IsSymbol()) {
            debuggerProperty->GetSymbol()->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, name);
        }
        outPropertyDesc->emplace_back(std::move(debuggerProperty));
    }
    GetProtoOrProtoType(value, isOwn, isAccessorOnly, outPropertyDesc);
    GetAdditionalProperties(value, outPropertyDesc);
}

template <typename TypedArrayRef>
void JSBackend::AddTypedArrayRef(Local<ArrayBufferRef> arrayBufferRef, int32_t length, const char* name,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    Local<JSValueRef> jsValueRefTypedArray(TypedArrayRef::New(ecmaVm_, arrayBufferRef, 0, length));
    std::unique_ptr<RemoteObject> remoteObjectTypedArray = RemoteObject::FromTagged(ecmaVm_, jsValueRefTypedArray);
    remoteObjectTypedArray->SetObjectId(curObjectId_);
    propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, jsValueRefTypedArray);
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();
    debuggerProperty->SetName(name)
        .SetWritable(true)
        .SetConfigurable(true)
        .SetEnumerable(false)
        .SetIsOwn(true)
        .SetValue(std::move(remoteObjectTypedArray));
    outPropertyDesc->emplace_back(std::move(debuggerProperty));
}

void JSBackend::AddTypedArrayRefs(Local<ArrayBufferRef> arrayBufferRef,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    int32_t arrayBufferByteLength = arrayBufferRef->ByteLength(ecmaVm_);

    int32_t typedArrayLength = arrayBufferByteLength;
    if (typedArrayLength > JSTypedArray::MAX_TYPED_ARRAY_INDEX) {
        return;
    }
    AddTypedArrayRef<Int8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8Array]]", outPropertyDesc);
    AddTypedArrayRef<Uint8ClampedArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint8ClampedArray]]", outPropertyDesc);

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_16BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_16BITS;
        AddTypedArrayRef<Int16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int16Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint16ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint16Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_32BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_32BITS;
        AddTypedArrayRef<Int32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Int32Array]]", outPropertyDesc);
        AddTypedArrayRef<Uint32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Uint32Array]]", outPropertyDesc);
        AddTypedArrayRef<Float32ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float32Array]]", outPropertyDesc);
    }

    if ((arrayBufferByteLength % NumberSize::BYTES_OF_64BITS) == 0) {
        typedArrayLength = arrayBufferByteLength / NumberSize::BYTES_OF_64BITS;
        AddTypedArrayRef<Float64ArrayRef>(arrayBufferRef, typedArrayLength, "[[Float64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigInt64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigInt64Array]]", outPropertyDesc);
        AddTypedArrayRef<BigUint64ArrayRef>(arrayBufferRef, typedArrayLength, "[[BigUint64Array]]", outPropertyDesc);
    }
}

void JSBackend::GetAdditionalProperties(const Local<JSValueRef> &value,
    CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc)
{
    // The length of the TypedArray have to be limited(less than or equal to lengthTypedArrayLimit) until we construct
    // the PropertyPreview class. Let lengthTypedArrayLimit be 10000 temporarily.
    static const int32_t lengthTypedArrayLimit = 10000;

    // The width of the string-expression for JSTypedArray::MAX_TYPED_ARRAY_INDEX which is euqal to
    // JSObject::MAX_ELEMENT_INDEX which is equal to std::numeric_limits<uint32_t>::max(). (42,9496,7295)
    static const int32_t widthStrExprMaxElementIndex = 10;

    if (value->IsTypedArray()) {
        Local<TypedArrayRef> localTypedArrayRef(value);
        int32_t lengthTypedArray = localTypedArrayRef->ArrayLength(ecmaVm_);
        if (lengthTypedArray < 0 || lengthTypedArray > lengthTypedArrayLimit) {
            LOG(ERROR, DEBUGGER) << "The length of the TypedArray is non-compliant or unsupported.";
            return;
        }
        for (int32_t i = 0; i < lengthTypedArray; i++) {
            Local<JSValueRef> localValRefElement = localTypedArrayRef->Get(ecmaVm_, i);
            std::unique_ptr<RemoteObject> remoteObjElement = RemoteObject::FromTagged(ecmaVm_, localValRefElement);
            remoteObjElement->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, localValRefElement);
            std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();

            std::ostringstream osNameElement;
            osNameElement << std::right << std::setw(widthStrExprMaxElementIndex) << i;
            CString cStrNameElement = CString(osNameElement.str());
            debuggerProperty->SetName(cStrNameElement)
                .SetWritable(true)
                .SetConfigurable(true)
                .SetEnumerable(false)
                .SetIsOwn(true)
                .SetValue(std::move(remoteObjElement));
            outPropertyDesc->emplace_back(std::move(debuggerProperty));
        }
    }
}

void JSBackend::CallFunctionOn([[maybe_unused]] const CString &functionDeclaration,
                               std::unique_ptr<RemoteObject> *outRemoteObject)
{
    // Return EvalError temporarily.
    auto error = Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupport eval now"));

    *outRemoteObject = RemoteObject::FromTagged(ecmaVm_, error);
}

void JSBackend::GetHeapUsage(double *usedSize, double *totalSize)
{
    auto ecmaVm = const_cast<EcmaVM *>(static_cast<ProtocolHandler *>(frontend_)->GetEcmaVM());
    *totalSize = static_cast<double>(DFXJSNApi::GetHeapTotalSize(ecmaVm));
    *usedSize = static_cast<double>(DFXJSNApi::GetHeapUsedSize(ecmaVm));
}

void JSBackend::CacheObjectIfNeeded(const Local<JSValueRef> &valRef, std::unique_ptr<RemoteObject> *remoteObj)
{
    *remoteObj = RemoteObject::FromTagged(ecmaVm_, valRef);
    if (valRef->IsObject() && !valRef->IsProxy()) {
        (*remoteObj)->SetObjectId(curObjectId_);
        propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, valRef);
    }
}

void JSBackend::CleanUpOnPaused()
{
    curObjectId_ = 0;
    propertiesPair_.clear();

    callFrameHandlers_.clear();
    scopeObjects_.clear();
}

bool JSBackend::DecodeAndCheckBase64(const CString &src, CString &dest)
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
}  //  namespace panda::ecmascript::tooling
