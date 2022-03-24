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
#include <regex>
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/front_end.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::tooling::ecmascript {
using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

const std::string DATA_APP_PATH = "/data/";

JSBackend::JSBackend(FrontEnd *frontend) : frontend_(frontend)
{
    ecmaVm_ = static_cast<ProtocolHandler *>(frontend)->GetEcmaVM();
    hooks_ = std::make_unique<JSPtHooks>(this);

    debugger_ = DebuggerApi::CreateJSDebugger(ecmaVm_);
    DebuggerApi::RegisterHooks(debugger_, hooks_.get());
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

void JSBackend::NotifyPaused(std::optional<PtLocation> location, PauseReason reason)
{
    if (!pauseOnException_ && reason == EXCEPTION) {
        return;
    }
    CVector<CString> hitBreakpoints;
    if (location.has_value()) {
        BreakpointDetails detail;
        JSPtExtractor *extractor = nullptr;
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

    // Notify paused event
    CVector<std::unique_ptr<CallFrame>> callFrames;
    if (!GenerateCallFrames(&callFrames)) {
        LOG(ERROR, DEBUGGER) << "NotifyPaused: GenerateCallFrames failed";
        return;
    }
    std::unique_ptr<Paused> paused = std::make_unique<Paused>();
    paused->SetCallFrames(std::move(callFrames)).SetReason(reason).SetHitBreakpoints(std::move(hitBreakpoints));
    frontend_->SendNotification(ecmaVm_, std::move(paused));

    // Waiting for Debugger
    frontend_->WaitForDebugger();
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
    const panda_file::File *pfs = DebuggerApi::FindPandaFile(fileName);
    if (pfs == nullptr) {
        LOG(WARNING, DEBUGGER) << "NotifyScriptParsed: unknown file: " << fileName;
        return false;
    }

    CString url;
    CString source;
    JSPtExtractor *extractor = GenerateExtractor(pfs);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "NotifyScriptParsed: Unsupported file: " << fileName;
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

bool JSBackend::StepComplete(const PtLocation &location)
{
    JSPtExtractor *extractor = nullptr;
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

std::optional<Error> JSBackend::GetPossibleBreakpoints(Location *start, [[maybe_unused]] Location *end,
    CVector<std::unique_ptr<BreakLocation>> *locations)
{
    JSPtExtractor *extractor = nullptr;
    auto scriptFunc = [this, &extractor](PtScript *script) -> bool {
        extractor = GetExtractor(script->GetUrl());
        return true;
    };
    if (!MatchScripts(scriptFunc, start->GetScriptId(), ScriptMatchType::SCRIPT_ID) || extractor == nullptr) {
        return Error(Error::Type::INVALID_BREAKPOINT, "extractor not found");
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

std::optional<Error> JSBackend::SetBreakpointByUrl(const CString &url, size_t lineNumber,
    size_t columnNumber, CString *out_id, CVector<std::unique_ptr<Location>> *outLocations)
{
    JSPtExtractor *extractor = GetExtractor(url);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrl: extractor is null";
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
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
        return Error(Error::Type::INVALID_BREAKPOINT, "Url not found");
    }

    std::optional<Error> ret = std::nullopt;
    auto callbackFunc = [this, fileName, &ret](File::EntityId id, uint32_t offset) -> bool {
        PtLocation location {fileName.c_str(), id, offset};
        ret = DebuggerApi::SetBreakpoint(debugger_, location);
        return true;
    };
    if (!extractor->MatchWithLocation(callbackFunc, lineNumber, columnNumber)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: " << lineNumber << ":" << columnNumber;
        return Error(Error::Type::INVALID_BREAKPOINT, "Breakpoint not found");
    }

    if (!ret.has_value()) {
        BreakpointDetails metaData{lineNumber, 0, url};
        *out_id = BreakpointDetails::ToString(metaData);
        *outLocations = CVector<std::unique_ptr<Location>>();
        std::unique_ptr<Location> location = std::make_unique<Location>();
        location->SetScriptId(scriptId).SetLine(lineNumber).SetColumn(0);
        outLocations->emplace_back(std::move(location));
    }

    return ret;
}

std::optional<Error> JSBackend::RemoveBreakpoint(const BreakpointDetails &metaData)
{
    JSPtExtractor *extractor = GetExtractor(metaData.url_);
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: extractor is null";
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
    }

    CString fileName;
    auto scriptFunc = [&fileName](PtScript *script) -> bool {
        fileName = script->GetFileName();
        return true;
    };
    if (!MatchScripts(scriptFunc, metaData.url_, ScriptMatchType::URL)) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Unknown url: " << metaData.url_;
        return Error(Error::Type::INVALID_BREAKPOINT, "Url not found");
    }

    std::optional<Error> ret = std::nullopt;
    auto callbackFunc = [this, fileName, &ret](File::EntityId id, uint32_t offset) -> bool {
        PtLocation location {fileName.c_str(), id, offset};
        ret = DebuggerApi::RemoveBreakpoint(debugger_, location);
        return true;
    };
    if (!extractor->MatchWithLocation(callbackFunc, metaData.line_, metaData.column_)) {
        LOG(ERROR, DEBUGGER) << "failed to set breakpoint location number: "
            << metaData.line_ << ":" << metaData.column_;
        return Error(Error::Type::INVALID_BREAKPOINT, "Breakpoint not found");
    }

    LOG(INFO, DEBUGGER) << "remove breakpoint line number:" << metaData.line_;
    return ret;
}

std::optional<Error> JSBackend::Pause()
{
    pauseOnNextByteCode_ = true;
    return {};
}

std::optional<Error> JSBackend::Resume()
{
    singleStepper_.reset();

    NotifyResume();
    return {};
}

std::optional<Error> JSBackend::StepInto()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepInto: extractor is null";
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
    }

    singleStepper_ = extractor->GetStepIntoStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<Error> JSBackend::StepOver()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOver: extractor is null";
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
    }

    singleStepper_ = extractor->GetStepOverStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<Error> JSBackend::StepOut()
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    JSPtExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "StepOut: extractor is null";
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
    }

    singleStepper_ = extractor->GetStepOutStepper(ecmaVm_);

    NotifyResume();
    return {};
}

std::optional<Error> JSBackend::EvaluateValue(const CString &callFrameId, const CString &expression,
    std::unique_ptr<RemoteObject> *result)
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    if (method->IsNative()) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: Native Frame not support";
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Runtime internal error")));
        return Error(Error::Type::METHOD_NOT_FOUND, "Native Frame not support");
    }
    JSPtExtractor *extractor = GetExtractor(method->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "EvaluateValue: extractor is null";
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Runtime internal error")));
        return Error(Error::Type::METHOD_NOT_FOUND, "Extractor not found");
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
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Only allow set value in current frame")));
        return Error(Error::Type::METHOD_NOT_FOUND, "Unsupported parent frame set value");
    }

    int32_t regIndex = -1;
    auto varInfos = extractor->GetLocalVariableTable(method->GetFileId());
    auto iter = varInfos.find(varName.c_str());
    if (iter != varInfos.end()) {
        regIndex = iter->second;
    }
    if (regIndex != -1) {
        if (varValue.empty()) {
            return GetVregValue(regIndex, result);
        }
        return SetVregValue(regIndex, result, varValue);
    }
    int32_t level = 0;
    uint32_t slot = 0;
    if (!DebuggerApi::EvaluateLexicalValue(ecmaVm_, varName.c_str(), level, slot)) {
        *result = RemoteObject::FromTagged(ecmaVm_,
            Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unknown input params")));
        return Error(Error::Type::METHOD_NOT_FOUND, "Unsupported expression");
    }
    if (varValue.empty()) {
        return GetLexicalValue(level, result, slot);
    }
    return SetLexicalValue(level, result, varValue, slot);
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

JSPtExtractor *JSBackend::GenerateExtractor(const panda_file::File *file)
{
    if (file->GetFilename().substr(0, DATA_APP_PATH.length()) != DATA_APP_PATH) {
        return nullptr;
    }
    auto extractor = std::make_unique<JSPtExtractor>(file);
    JSPtExtractor *res = extractor.get();
    extractors_[file->GetFilename()] = std::move(extractor);
    return res;
}

JSPtExtractor *JSBackend::GetExtractor(const panda_file::File *file)
{
    const std::string fileName = file->GetFilename();
    if (extractors_.find(fileName) == extractors_.end()) {
        return nullptr;
    }

    return extractors_[fileName].get();
}

JSPtExtractor *JSBackend::GetExtractor(const CString &url)
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
            callFrames->emplace_back(std::move(callFrame));
            callFrameId++;
        }
        return StackState::CONTINUE;
    };
    return DebuggerApi::StackWalker(ecmaVm_, walkerFunc);
}

bool JSBackend::GenerateCallFrame(CallFrame *callFrame,
    const InterpretedFrameHandler *frameHandler, int32_t callFrameId)
{
    JSMethod *method = DebuggerApi::GetMethod(frameHandler);
    auto *pf = method->GetPandaFile();
    JSPtExtractor *extractor = GetExtractor(pf);
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

    std::unique_ptr<RemoteObject> local = std::make_unique<RemoteObject>();
    Local<ObjectRef> localObject(local->NewObject(ecmaVm_));
    local->SetType(ObjectType::Object)
        .SetObjectId(curObjectId_)
        .SetClassName(ObjectClassName::Object)
        .SetDescription(RemoteObject::ObjectDescription);
    propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, localObject);

    JSPtExtractor *extractor = GetExtractor(DebuggerApi::GetMethod(frameHandler)->GetPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "GetScopeChain: extractor is null";
        return localScope;
    }
    panda_file::File::EntityId methodId = DebuggerApi::GetMethod(frameHandler)->GetFileId();
    Local<JSValueRef> name = JSValueRef::Undefined(ecmaVm_);
    Local<JSValueRef> value = JSValueRef::Undefined(ecmaVm_);
    for (const auto &var : extractor->GetLocalVariableTable(methodId)) {
        value = DebuggerApi::GetVRegValue(ecmaVm_, frameHandler, var.second);
        if (var.first == "this") {
            *thisObj = RemoteObject::FromTagged(ecmaVm_, value);
            if (value->IsObject() && !value->IsProxy()) {
                (*thisObj)->SetObjectId(curObjectId_);
                propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, value);
            }
        } else {
            name = StringRef::NewFromUtf8(ecmaVm_, var.first.c_str());
            PropertyAttribute descriptor(value, true, true, true);
            localObject->DefineProperty(ecmaVm_, name, descriptor);
        }
    }

    if ((*thisObj)->GetType() == ObjectType::Undefined) {
        value = DebuggerApi::GetLexicalValueInfo(ecmaVm_, "this");
        *thisObj = RemoteObject::FromTagged(ecmaVm_, value);
        if (value->IsObject() && !value->IsProxy()) {
            (*thisObj)->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, value);
        }
    }

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

std::optional<Error> JSBackend::ConvertToLocal(Local<JSValueRef> &taggedValue, std::unique_ptr<RemoteObject> *result,
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
                Exception::EvalError(ecmaVm_, StringRef::NewFromUtf8(ecmaVm_, "Unsupported value")));
            return Error(Error::Type::METHOD_NOT_FOUND, "Unsupported value");
        }
        taggedValue = NumberRef::New(ecmaVm_, d);
    }
    return {};
}

std::optional<Error> JSBackend::SetVregValue(int32_t regIndex, std::unique_ptr<RemoteObject> *result,
    const CString &varValue)
{
    Local<JSValueRef> taggedValue;
    std::optional<Error> ret = ConvertToLocal(taggedValue, result, varValue);
    if (ret.has_value()) {
        return ret;
    }
    DebuggerApi::SetVRegValue(ecmaVm_, regIndex, taggedValue);
    *result = RemoteObject::FromTagged(ecmaVm_, taggedValue);
    return {};
}

std::optional<Error> JSBackend::SetLexicalValue(int32_t level, std::unique_ptr<RemoteObject> *result,
    const CString &varValue, uint32_t slot)
{
    Local<JSValueRef> taggedValue;
    std::optional<Error> ret = ConvertToLocal(taggedValue, result, varValue);
    if (ret.has_value()) {
        return ret;
    }
    DebuggerApi::SetProperties(ecmaVm_, level, slot, taggedValue);
    *result = RemoteObject::FromTagged(ecmaVm_, taggedValue);
    return {};
}

std::optional<Error> JSBackend::GetVregValue(int32_t regIndex, std::unique_ptr<RemoteObject> *result)
{
    Local<JSValueRef> vValue = DebuggerApi::GetVRegValue(ecmaVm_, regIndex);
    *result = RemoteObject::FromTagged(ecmaVm_, vValue);
    if (vValue->IsObject() && !vValue->IsProxy()) {
        (*result)->SetObjectId(curObjectId_);
        propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, vValue);
    }
    return {};
}

std::optional<Error> JSBackend::GetLexicalValue(int32_t level, std::unique_ptr<RemoteObject> *result, uint32_t slot)
{
    Local<JSValueRef> vValue = DebuggerApi::GetProperties(ecmaVm_, level, slot);
    *result = RemoteObject::FromTagged(ecmaVm_, vValue);
    if (vValue->IsObject() && !vValue->IsProxy()) {
        (*result)->SetObjectId(curObjectId_);
        propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, vValue);
    }
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
        std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(ecmaVm_, prototype);
        if (prototype->IsObject() && !prototype->IsProxy()) {
            protoObj->SetObjectId(curObjectId_);
            propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, prototype);
        }
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
    std::unique_ptr<RemoteObject> protoObj = RemoteObject::FromTagged(ecmaVm_, proto);
    if (proto->IsObject() && !proto->IsProxy()) {
        protoObj->SetObjectId(curObjectId_);
        propertiesPair_[curObjectId_++] = Global<JSValueRef>(ecmaVm_, proto);
    }
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

void JSBackend::CallFunctionOn([[maybe_unused]] const CString &functionDeclaration, [[maybe_unused]] uint32_t objectId,
    [[maybe_unused]] const CVector<std::unique_ptr<CallArgument>> *arguments, [[maybe_unused]] bool isSilent,
    [[maybe_unused]] bool returnByValue, [[maybe_unused]] bool generatePreview, [[maybe_unused]] bool userGesture,
    [[maybe_unused]] bool awaitPromise, [[maybe_unused]] ExecutionContextId executionContextId,
    [[maybe_unused]] const CString &objectGroup, [[maybe_unused]] bool throwOnSideEffect,
    std::unique_ptr<RemoteObject> *outRemoteObject)
{
    // Return undefined value temporarily.
    std::unique_ptr<RemoteObject> remoteObjUndefVal =
        std::make_unique<PrimitiveRemoteObject>(ecmaVm_, JSValueRef::Undefined(ecmaVm_));
    (*outRemoteObject) = std::move(remoteObjUndefVal);
}
}  // namespace panda::tooling::ecmascript
