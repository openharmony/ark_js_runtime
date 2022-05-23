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

#include "js_backend.h"

#include <boost/beast/core/detail/base64.hpp>
#include <regex>

#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/front_end.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "ecmascript/napi/jsnapi_helper-inl.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript::tooling {
using namespace boost::beast::detail;
using namespace std::placeholders;

using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

const std::string DATA_APP_PATH = "/data/";

JSBackend::JSBackend(FrontEnd *frontend) : frontend_(frontend)
{
    ecmaVm_ = static_cast<ProtocolHandler *>(frontend)->GetEcmaVM();
    hooks_ = std::make_unique<JSPtHooks>(this);

    debugger_ = DebuggerApi::CreateJSDebugger(ecmaVm_);
    DebuggerApi::InitJSDebugger(debugger_);
    DebuggerApi::RegisterHooks(debugger_, hooks_.get());

    updaterFunc_ = std::bind(&JSBackend::UpdateScopeObject, this, _1, _2, _3);
    ecmaVm_->GetJsDebuggerManager()->SetLocalScopeUpdater(&updaterFunc_);
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

void JSBackend::NotifyPaused(std::optional<JSPtLocation> location, PauseReason reason)
{
    if (!pauseOnException_ && reason == EXCEPTION) {
        return;
    }

    Local<JSValueRef> exception = DebuggerApi::GetAndClearException(ecmaVm_);

    CVector<CString> hitBreakpoints;
    if (location.has_value()) {
        BreakpointDetails detail;
        PtJSExtractor *extractor = nullptr;
        auto scriptFunc = [this, &extractor, &detail](PtScript *script) -> bool {
            detail.url_ = script->GetUrl();
            extractor = GetExtractor(detail.url_);
            return true;
        };
        auto callbackFunc = [&detail](size_t line, size_t column) -> bool {
            detail.line_ = line;
            detail.column_ = column;
            return true;
        };
        if (!MatchScripts(scriptFunc, location->GetPandaFile(), ScriptMatchType::FILE_NAME) || extractor == nullptr ||
            !extractor->MatchWithOffset(callbackFunc, location->GetMethodId(), location->GetBytecodeOffset())) {
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

bool JSBackend::NotifyScriptParsed(int32_t scriptId, const CString &fileName)
{
    auto scriptFunc = []([[maybe_unused]] PtScript *script) -> bool {
        return true;
    };
    if (MatchScripts(scriptFunc, fileName, ScriptMatchType::FILE_NAME)) {
        LOG(WARNING, DEBUGGER) << "NotifyScriptParsed: already loaded: " << fileName;
        return false;
    }
    const panda_file::File *pfs = DebuggerApi::FindPandaFile(ecmaVm_, fileName);
    if (pfs == nullptr) {
        LOG(WARNING, DEBUGGER) << "NotifyScriptParsed: unknown file: " << fileName;
        return false;
    }

    auto classes = pfs->GetClasses();
    ASSERT(classes.Size() > 0);
    size_t index = 0;
    for (; index < classes.Size(); index++) {
        if (!(*pfs).IsExternal(panda_file::File::EntityId(classes[index]))) {
            break;
        }
    }
    panda_file::ClassDataAccessor cda(*pfs, panda_file::File::EntityId(classes[index]));
    auto lang = cda.GetSourceLang();
    if (lang.value_or(panda_file::SourceLang::PANDA_ASSEMBLY) != panda_file::SourceLang::ECMASCRIPT) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: Unsupport file: " << fileName;
        return false;
    }

    CString url;
    CString source;
    PtJSExtractor *extractor = GenerateExtractor(pfs);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: Unsupport file: " << fileName;
        return false;
    }

    const uint32_t MIN_SOURCE_CODE_LENGTH = 5;  // maybe return 'ANDA' when source code is empty
    for (const auto &method : extractor->GetMethodIdList()) {
        source = CString(extractor->GetSourceCode(method));
        // only main function has source code
        if (source.size() >= MIN_SOURCE_CODE_LENGTH) {
            url = CString(extractor->GetSourceFile(method));
            break;
        }
    }
    if (url.empty()) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: invalid file: " << fileName;
        return false;
    }
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
    PtJSExtractor *extractor = nullptr;
    auto scriptFunc = [this, &extractor](PtScript *script) -> bool {
        extractor = GetExtractor(script->GetUrl());
        return true;
    };
    auto callbackFunc = [](size_t line, [[maybe_unused]] size_t column) -> bool {
        return line == static_cast<size_t>(SPECIAL_LINE_MARK);
    };
    if (MatchScripts(scriptFunc, location.GetPandaFile(), ScriptMatchType::FILE_NAME) && extractor != nullptr &&
        extractor->MatchWithOffset(callbackFunc, location.GetMethodId(), location.GetBytecodeOffset())) {
        LOG(INFO, DEBUGGER) << "StepComplete: skip -1";
        return false;
    }

    if (pauseOnNextByteCode_ ||
        (singleStepper_ != nullptr && singleStepper_->StepComplete(location.GetBytecodeOffset()))) {
        LOG(INFO, DEBUGGER) << "StepComplete: pause on current byte_code";
        pauseOnNextByteCode_ = false;
        return true;
    }

    return false;
}

std::optional<CString> JSBackend::GetPossibleBreakpoints(Location *start, [[maybe_unused]] Location *end,
    CVector<std::unique_ptr<BreakLocation>> *locations)
{
    PtJSExtractor *extractor = nullptr;
    auto scriptFunc = [this, &extractor](PtScript *script) -> bool {
        extractor = GetExtractor(script->GetUrl());
        return true;
    };
    if (!MatchScripts(scriptFunc, start->GetScriptId(), ScriptMatchType::SCRIPT_ID) || extractor == nullptr) {
        return "Unknown file name.";
    }

    size_t line = start->GetLine();
    size_t column = start->GetColumn();
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

std::optional<CString> JSBackend::SetBreakpointByUrl(const CString &url, size_t lineNumber,
    size_t columnNumber, CString *out_id, CVector<std::unique_ptr<Location>> *outLocations)
{
    PtJSExtractor *extractor = GetExtractor(url);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: extractor is null";
        return "Unknown file name.";
    }

    CString scriptId;
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

    auto callbackFunc = [this, fileName](File::EntityId id, uint32_t offset) -> bool {
        JSPtLocation location {fileName.c_str(), id, offset};
        return DebuggerApi::SetBreakpoint(debugger_, location);
    };
    if (!extractor->MatchWithLocation(callbackFunc, lineNumber, columnNumber)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: " << lineNumber << ":" << columnNumber;
        return "Breakpoint not found.";
    }

    BreakpointDetails metaData{lineNumber, 0, url};
    *out_id = BreakpointDetails::ToString(metaData);
    *outLocations = CVector<std::unique_ptr<Location>>();
    std::unique_ptr<Location> location = std::make_unique<Location>();
    location->SetScriptId(scriptId).SetLine(lineNumber).SetColumn(0);
    outLocations->emplace_back(std::move(location));
    return {};
}

std::optional<CString> JSBackend::RemoveBreakpoint(const BreakpointDetails &metaData)
{
    PtJSExtractor *extractor = GetExtractor(metaData.url_);
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

    LOG(INFO, DEBUGGER) << "remove breakpoint line number:" << metaData.line_;
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
    PtJSExtractor *extractor = GetExtractor(method->GetPandaFile());
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
    PtJSExtractor *extractor = GetExtractor(method->GetPandaFile());
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
    PtJSExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOut: extractor is null";
        return "Unknown file name.";
    }

    singleStepper_ = extractor->GetStepOutStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<CString> JSBackend::CmptEvaluateValue(const CString &callFrameId, const CString &expression,
    std::unique_ptr<RemoteObject> *result)
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    if (method->IsNative()) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Runtime internal error")));
        return "Native Frame not support.";
    }
    DebugInfoExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Runtime internal error")));
        return "Internal error.";
    }
    CString varName = expression;
    CString varValue;
    CString::size_type indexEqual = expression.find_first_of('=', 0);
    if (indexEqual != CString::npos) {
        varName = Trim(expression.substr(0, indexEqual));
        varValue = Trim(expression.substr(indexEqual + 1, expression.length()));
    }

    if (!varValue.empty() && callFrameId != "0") {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Native Frame not support.")));
        return "Native Frame not support.";
    }

    int32_t regIndex = -1;
    auto varInfos = extractor->GetLocalVariableTable(method->GetFileId());
    for (const auto &varInfo : varInfos) {
        if (varInfo.name == std::string(varName)) {
            regIndex = varInfo.reg_number;
        }
    }
    if (regIndex != -1) {
        if (varValue.empty()) {
            return GetVregValue(regIndex, result);
        }
        return SetVregValue(regIndex, varValue, result);
    }
    int32_t level = 0;
    uint32_t slot = 0;
    if (!DebuggerApi::EvaluateLexicalValue(ecmaVm_, varName, level, slot)) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unknow input params")));
        return "Unsupported expression.";
    }
    if (varValue.empty()) {
        return GetLexicalValue(level, slot, result);
    }
    return SetLexicalValue(level, slot, varValue, result);
}

std::optional<CString> JSBackend::EvaluateValue(const CString &callFrameId, const CString &expression,
    std::unique_ptr<RemoteObject> *result)
{
    size_t frameId = static_cast<size_t>(DebuggerApi::CStringToULL(callFrameId));
    if (frameId < 0 || frameId >= callFrameHandlers_.size()) {
        return "Invalid callFrameId.";
    }

    CString dest;
    if (!DecodeAndCheckBase64(expression, dest)) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: base64 decode failed";
        return CmptEvaluateValue(callFrameId, expression, result);
    }

    auto funcRef = DebuggerApi::GenerateFuncFromBuffer(ecmaVm_, dest.data(), dest.size());
    auto res = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_), funcRef,
        callFrameHandlers_[frameId]);

    CString msg;
    if (DebuggerApi::HandleUncaughtException(ecmaVm_, msg)) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: has pending exception";
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

PtJSExtractor *JSBackend::GenerateExtractor(const panda_file::File *file)
{
    if (file->GetFilename().substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH) {
        return nullptr;
    }
    auto extractor = std::make_unique<PtJSExtractor>(file);
    PtJSExtractor *res = extractor.get();
    extractors_[file->GetFilename()] = std::move(extractor);
    return res;
}

PtJSExtractor *JSBackend::GetExtractor(const panda_file::File *file)
{
    const std::string fileName = file->GetFilename();
    if (extractors_.find(fileName) == extractors_.end()) {
        return nullptr;
    }

    return extractors_[fileName].get();
}

PtJSExtractor *JSBackend::GetExtractor(const CString &url)
{
    for (const auto &iter : extractors_) {
        auto methods = iter.second->GetMethodIdList();
        for (const auto &method : methods) {
            auto sourceFile = iter.second->GetSourceFile(method);
            if (sourceFile == url) {
                return iter.second.get();
            }
        }
    }
    return nullptr;
}

bool JSBackend::GenerateCallFrames(CVector<std::unique_ptr<CallFrame>> *callFrames)
{
    int32_t callFrameId = 0;
    auto walkerFunc = [this, &callFrameId, &callFrames](const InterpretedFrameHandler *frameHandler) -> StackState {
        JSMethod *method = DebuggerApi::GetMethod(frameHandler);
        if (method->IsNative()) {
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

void JSBackend::SaveCallFrameHandler(const InterpretedFrameHandler *frameHandler)
{
    auto handlerPtr = DebuggerApi::NewFrameHandler(ecmaVm_);
    *handlerPtr = *frameHandler;
    callFrameHandlers_.emplace_back(handlerPtr);
}

bool JSBackend::GenerateCallFrame(CallFrame *callFrame,
    const InterpretedFrameHandler *frameHandler, int32_t callFrameId)
{
    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    auto *pf = method->GetPandaFile();
    PtJSExtractor *extractor = GetExtractor(pf);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: extractor is null";
        return false;
    }

    // location
    std::unique_ptr<Location> location = std::make_unique<Location>();
    CString url = extractor->GetSourceFile(method->GetFileId());
    auto scriptFunc = [&location](PtScript *script) -> bool {
        location->SetScriptId(script->GetScriptId());
        return true;
    };
    if (!MatchScripts(scriptFunc, url, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "GenerateCallFrame: Unknown url: " << url;
        return false;
    }
    auto callbackFunc = [&location](size_t line, size_t column) -> bool {
        location->SetLine(line);
        location->SetColumn(column);
        return true;
    };
    if (!extractor->MatchWithOffset(callbackFunc, method->GetFileId(), DebuggerApi::GetBytecodeOffset(frameHandler))) {
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

    callFrame->SetCallFrameId(DebuggerApi::ToCString(callFrameId))
        .SetFunctionName(functionName)
        .SetLocation(std::move(location))
        .SetUrl(url)
        .SetScopeChain(std::move(scopeChain))
        .SetThis(std::move(thisObj));
    return true;
}

std::unique_ptr<Scope> JSBackend::GetLocalScopeChain(const InterpretedFrameHandler *frameHandler,
    std::unique_ptr<RemoteObject> *thisObj)
{
    auto localScope = std::make_unique<Scope>();

    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    PtJSExtractor *extractor = GetExtractor(method->GetPandaFile());
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

    panda_file::File::EntityId methodId = method->GetFileId();
    const panda_file::LineNumberTable &lines = extractor->GetLineNumberTable(methodId);
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

void JSBackend::GetLocalVariables(const InterpretedFrameHandler *frameHandler, const JSMethod *method,
    Local<JSValueRef> &thisVal, Local<ObjectRef> &localObj)
{
    auto methodId = method->GetFileId();
    auto *extractor = GetExtractor(method->GetPandaFile());
    Local<JSValueRef> value = JSValueRef::Undefined(ecmaVm_);
    Local<JSValueRef> name = JSValueRef::Undefined(ecmaVm_);
    bool hasThis = false;
    for (const auto &var: extractor->GetLocalVariableTable(methodId)) {
        value = DebuggerApi::GetVRegValue(ecmaVm_, frameHandler, var.reg_number);
        if (var.name == "4newTarget") {
            continue;
        } else if (var.name == "this") {
            thisVal = value;
            hasThis = true;
        } else {
            if (var.name == "4funcObj" && value->IsFunction()) {
                name = Local<FunctionRef>(value)->GetName(ecmaVm_);
            } else {
                name = StringRef::NewFromUtf8(ecmaVm_, var.name.c_str());
            }
            PropertyAttribute descriptor(value, true, true, true);
            localObj->DefineProperty(ecmaVm_, name, descriptor);
        }
    }
    if (thisVal->IsUndefined()) {
        thisVal = DebuggerApi::GetLexicalValueInfo(ecmaVm_, "this");
    }

    if (hasThis) {
        // closure variables are stored in env
        DebuggerApi::SetClosureVariables(ecmaVm_, frameHandler, localObj);
    }
}

void JSBackend::UpdateScopeObject(const InterpretedFrameHandler *frameHandler,
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
                Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupport expression")));
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

void JSBackend::GetProperties(uint32_t objectId, bool isOwn, bool isAccessorOnly,
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
    Local<ArrayRef> keys = Local<ObjectRef>(value)->GetOwnPropertyNames(ecmaVm_);
    uint32_t length = keys->Length(ecmaVm_);
    Local<JSValueRef> name = JSValueRef::Undefined(ecmaVm_);
    for (uint32_t i = 0; i < length; ++i) {
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
}

void JSBackend::CallFunctionOn([[maybe_unused]] const CString &functionDeclaration,
                               std::unique_ptr<RemoteObject> *outRemoteObject)
{
    // Return EvalError temporarily.
    auto error = Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupport eval now"));

    *outRemoteObject = RemoteObject::FromTagged(ecmaVm_, error);
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
}  // namespace panda::ecmascript::tooling
