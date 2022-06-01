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

#include "ecmascript/tooling/base/pt_events.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<BreakpointResolved> BreakpointResolved::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "BreakpointResolved::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto breakpointResolved = std::make_unique<BreakpointResolved>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            breakpointResolved->breakpointId_ = DebuggerApi::ToCString(result);
        } else {
            error += "'breakpointId' should a String;";
        }
    } else {
        error += "should contain 'breakpointId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> location = Location::Create(ecmaVm, result);
            if (location == nullptr) {
                error += "'location' format error;";
            } else {
                breakpointResolved->location_ = std::move(location);
            }
        } else {
            error += "'exception' should a Object;";
        }
    } else {
        error += "should contain 'location';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "BreakpointResolved::Create " << error;
        return nullptr;
    }

    return breakpointResolved;
}

Local<ObjectRef> BreakpointResolved::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, breakpointId_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<Paused> Paused::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "Paused::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto paused = std::make_unique<Paused>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrames")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<CallFrame> callFrame = CallFrame::Create(ecmaVm, resultValue);
                if (resultValue.IsEmpty() || callFrame == nullptr) {
                    error += "'callFrames' format invalid;";
                }
                paused->callFrames_.emplace_back(std::move(callFrame));
            }
        } else {
            error += "'callFrames' should a Array;";
        }
    } else {
        error += "should contain 'callFrames';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "reason")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paused->reason_ = DebuggerApi::ToCString(result);
        } else {
            error += "'reason' should a String;";
        }
    } else {
        error += "should contain 'reason';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "data")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'data' format error;";
            } else {
                paused->data_ = std::move(obj);
            }
        } else {
            error += "'data' should a Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hitBreakpoints")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            CVector<BreakpointId> breakPoints;
            auto array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                if (resultValue.IsEmpty()) {
                    error += "'hitBreakpoints' format invalid;";
                }
                breakPoints.emplace_back(DebuggerApi::ToCString(result));
            }
            paused->hitBreakpoints_ = std::move(breakPoints);
        } else {
            error += "'hitBreakpoints' should a Array;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Parsed::Create " << error;
        return nullptr;
    }

    return paused;
}

Local<ObjectRef> Paused::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    size_t len = callFrames_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> callFrame = callFrames_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, callFrame);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrames")), values);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "reason")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, reason_.c_str())));
    if (data_) {
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "data")),
            Local<JSValueRef>(data_.value()->ToObject(ecmaVm)));
    }
    if (hitBreakpoints_) {
        len = hitBreakpoints_->size();
        values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<StringRef> id = StringRef::NewFromUtf8(ecmaVm, hitBreakpoints_.value()[i].c_str());
            values->Set(ecmaVm, i, id);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hitBreakpoints")), values);
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<Resumed> Resumed::Create([[maybe_unused]] const EcmaVM *ecmaVm,
                                         [[maybe_unused]] const Local<JSValueRef> &params)
{
    return std::make_unique<Resumed>();
}

Local<ObjectRef> Resumed::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<ScriptFailedToParse> ScriptFailedToParse::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "ScriptFailedToParse::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scriptEvent = std::make_unique<ScriptFailedToParse>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "scriptId"));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should a String;";
        }
    } else {
        error += "should contain 'url';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->startLine_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'startLine' should a Number;";
        }
    } else {
        error += "should contain 'startLine';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->startColumn_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'startColumn' should a Number;";
        }
    } else {
        error += "should contain 'startColumn';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->endLine_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'endLine' should a Number;";
        }
    } else {
        error += "should contain 'endLine';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->endColumn_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'endColumn' should a Number;";
        }
    } else {
        error += "should contain 'endColumn';";
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->executionContextId_ = static_cast<ExecutionContextId>(Local<NumberRef>(result)->Value());
        } else {
            error += "'executionContextId' should a Number;";
        }
    } else {
        error += "should contain 'executionContextId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->hash_ = DebuggerApi::ToCString(result);
        } else {
            error += "'hash' should a String;";
        }
    } else {
        error += "should contain 'hash';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            scriptEvent->execContextAuxData_ = Local<ObjectRef>(result);
        } else {
            error += "'executionContextAuxData' should a Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->sourceMapUrl_ = DebuggerApi::ToCString(result);
        } else {
            error += "'sourceMapURL' should a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            scriptEvent->hasSourceUrl_ = result->IsTrue();
        } else {
            error += "'hasSourceURL' should a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            scriptEvent->isModule_ = result->IsTrue();
        } else {
            error += "'isModule' should a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->length_ = static_cast<uint32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'length' should a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->codeOffset_ = static_cast<uint32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'codeOffset' should a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->scriptLanguage_ = DebuggerApi::ToCString(result);
        } else {
            error += "'scriptLanguage' should a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->embedderName_ = DebuggerApi::ToCString(result);
        } else {
            error += "'embedderName' should a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptFailedToParse::Create " << error;
        return nullptr;
    }

    return scriptEvent;
}

Local<ObjectRef> ScriptFailedToParse::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")),
        IntegerRef::New(ecmaVm, startLine_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")),
        IntegerRef::New(ecmaVm, startColumn_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")),
        IntegerRef::New(ecmaVm, endLine_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")),
        IntegerRef::New(ecmaVm, endColumn_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")),
        IntegerRef::New(ecmaVm, executionContextId_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, hash_.c_str())));
    if (execContextAuxData_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")),
            Local<JSValueRef>(execContextAuxData_.value()));
    }
    if (sourceMapUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, sourceMapUrl_->c_str())));
    }
    if (hasSourceUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")),
            BooleanRef::New(ecmaVm, hasSourceUrl_.value()));
    }
    if (isModule_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")),
            BooleanRef::New(ecmaVm, isModule_.value()));
    }
    if (length_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")),
            IntegerRef::New(ecmaVm, length_.value()));
    }
    if (codeOffset_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")),
            IntegerRef::New(ecmaVm, codeOffset_.value()));
    }
    if (scriptLanguage_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptLanguage_->c_str())));
    }
    if (embedderName_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, embedderName_->c_str())));
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<ScriptParsed> ScriptParsed::Create(const std::unique_ptr<PtScript> &script)
{
    std::unique_ptr<ScriptParsed> scriptParsed = std::make_unique<ScriptParsed>();
    scriptParsed->SetScriptId(script->GetScriptId())
        .SetUrl(script->GetUrl())
        .SetStartLine(0)
        .SetStartColumn(0)
        .SetEndLine(script->GetEndLine())
        .SetEndColumn(0)
        .SetExecutionContextId(0)
        .SetHash(script->GetHash());
    return scriptParsed;
}

std::unique_ptr<ScriptParsed> ScriptParsed::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "ScriptParsed::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scriptEvent = std::make_unique<ScriptParsed>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should a String;";
        }
    } else {
        error += "should contain 'url';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->startLine_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'startLine' should a Number;";
        }
    } else {
        error += "should contain 'startLine';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->startColumn_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'startColumn' should a Number;";
        }
    } else {
        error += "should contain 'startColumn';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->endLine_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'endLine' should a Number;";
        }
    } else {
        error += "should contain 'endLine';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->endColumn_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'endColumn' should a Number;";
        }
    } else {
        error += "should contain 'endColumn';";
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->executionContextId_ = static_cast<ExecutionContextId>(Local<NumberRef>(result)->Value());
        } else {
            error += "'executionContextId' should a Number;";
        }
    } else {
        error += "should contain 'executionContextId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->hash_ = DebuggerApi::ToCString(result);
        } else {
            error += "'hash' should a String;";
        }
    } else {
        error += "should contain 'hash';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            scriptEvent->execContextAuxData_ = Local<ObjectRef>(result);
        } else {
            error += "'executionContextAuxData' should a Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isLiveEdit")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            scriptEvent->isLiveEdit_ = result->IsTrue();
        } else {
            error += "'isLiveEdit' should a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->sourceMapUrl_ = DebuggerApi::ToCString(result);
        } else {
            error += "'sourceMapURL' should a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            scriptEvent->hasSourceUrl_ = result->IsTrue();
        } else {
            error += "'hasSourceURL' should a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            scriptEvent->isModule_ = result->IsTrue();
        } else {
            error += "'isModule' should a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->length_ = static_cast<uint32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'length' should a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptEvent->codeOffset_ = static_cast<uint32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'codeOffset' should a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->scriptLanguage_ = DebuggerApi::ToCString(result);
        } else {
            error += "'scriptLanguage' should a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptEvent->embedderName_ = DebuggerApi::ToCString(result);
        } else {
            error += "'embedderName' should a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptParsed::Create " << error;
        return nullptr;
    }

    return scriptEvent;
}

Local<ObjectRef> ScriptParsed::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")),
        IntegerRef::New(ecmaVm, startLine_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")),
        IntegerRef::New(ecmaVm, startColumn_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")),
        IntegerRef::New(ecmaVm, endLine_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")),
        IntegerRef::New(ecmaVm, endColumn_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")),
        IntegerRef::New(ecmaVm, executionContextId_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, hash_.c_str())));
    if (execContextAuxData_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")),
            Local<JSValueRef>(execContextAuxData_.value()));
    }
    if (isLiveEdit_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isLiveEdit")),
            BooleanRef::New(ecmaVm, isLiveEdit_.value()));
    }
    if (sourceMapUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, sourceMapUrl_->c_str())));
    }
    if (hasSourceUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")),
            BooleanRef::New(ecmaVm, hasSourceUrl_.value()));
    }
    if (isModule_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")),
            BooleanRef::New(ecmaVm, isModule_.value()));
    }
    if (length_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")),
            IntegerRef::New(ecmaVm, length_.value()));
    }
    if (codeOffset_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")),
            IntegerRef::New(ecmaVm, codeOffset_.value()));
    }
    if (scriptLanguage_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptLanguage_->c_str())));
    }
    if (embedderName_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, embedderName_->c_str())));
    }
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<AddHeapSnapshotChunk> AddHeapSnapshotChunk::Create(char* data, int size)
{
    auto addHeapSnapshotChunk = std::make_unique<AddHeapSnapshotChunk>();

    addHeapSnapshotChunk->chunk_.resize(size);
    for (int i = 0; i < size; ++i) {
        addHeapSnapshotChunk->chunk_[i] = data[i];
    }

    return addHeapSnapshotChunk;
}

std::unique_ptr<AddHeapSnapshotChunk> AddHeapSnapshotChunk::Create(const EcmaVM *ecmaVm,
                                                                   const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "AddHeapSnapshotChunk::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto addHeapSnapshotChunk = std::make_unique<AddHeapSnapshotChunk>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "chunk")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            addHeapSnapshotChunk->chunk_ = DebuggerApi::ToCString(result);
        } else {
            error += "'chunk' should a String;";
        }
    } else {
        error += "should contain 'chunk';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "AddHeapSnapshotChunk::Create " << error;
        return nullptr;
    }

    return addHeapSnapshotChunk;
}

Local<ObjectRef> AddHeapSnapshotChunk::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "chunk")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, chunk_.c_str())));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<ConsoleProfileFinished> ConsoleProfileFinished::Create(const EcmaVM *ecmaVm,
                                                                       const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "ConsoleProfileFinished::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto consoleProfileFinished = std::make_unique<ConsoleProfileFinished>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            consoleProfileFinished->id_ = DebuggerApi::ToCString(result);
        } else {
            error += "'id' should a String;";
        }
    } else {
        error += "should contain 'id';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> location = Location::Create(ecmaVm, result);
            if (location == nullptr) {
                error += "'location' format error;";
            } else {
                consoleProfileFinished->location_ = std::move(location);
            }
        } else {
            error += "'location' should a Object;";
        }
    } else {
        error += "should contain 'location';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Profile> profile = Profile::Create(ecmaVm, result);
            if (profile == nullptr) {
                error += "'profile' format error;";
            } else {
                consoleProfileFinished->profile_ = std::move(profile);
            }
        } else {
            error += "'profile' should a Object;";
        }
    } else {
        error += "should contain 'profile';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            consoleProfileFinished->title_ = DebuggerApi::ToCString(result);
        } else {
            error += "'title' should a String;";
        }
    } else {
        error += "should contain 'title';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ConsoleProfileFinished::Create " << error;
        return nullptr;
    }

    return consoleProfileFinished;
}

Local<ObjectRef> ConsoleProfileFinished::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, id_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")),
        Local<JSValueRef>(profile_->ToObject(ecmaVm)));
    if (title_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, title_->c_str())));
    }
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<ConsoleProfileStarted> ConsoleProfileStarted::Create(const EcmaVM *ecmaVm,
                                                                     const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "ConsoleProfileStarted::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto consoleProfileStarted = std::make_unique<ConsoleProfileStarted>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            consoleProfileStarted->id_ = DebuggerApi::ToCString(result);
        } else {
            error += "'id' should a String;";
        }
    } else {
        error += "should contain 'id';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> location = Location::Create(ecmaVm, result);
            if (location == nullptr) {
                error += "'location' format error;";
            } else {
                consoleProfileStarted->location_ = std::move(location);
            }
        } else {
            error += "'location' should a Object;";
        }
    } else {
        error += "should contain 'location';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            consoleProfileStarted->title_ = DebuggerApi::ToCString(result);
        } else {
            error += "'title' should a String;";
        }
    } else {
        error += "should contain 'title';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ConsoleProfileStarted::Create " << error;
        return nullptr;
    }

    return consoleProfileStarted;
}

Local<ObjectRef> ConsoleProfileStarted::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, id_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));
    if (title_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, title_->c_str())));
    }
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<PreciseCoverageDeltaUpdate> PreciseCoverageDeltaUpdate::Create(const EcmaVM *ecmaVm,
                                                                               const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "PreciseCoverageDeltaUpdate::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto preciseCoverageDeltaUpdate = std::make_unique<PreciseCoverageDeltaUpdate>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            preciseCoverageDeltaUpdate->timestamp_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'timestamp' should a Number;";
        }
    } else {
        error += "should contain 'timestamp';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "occasion")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            preciseCoverageDeltaUpdate->occasion_ = DebuggerApi::ToCString(result);
        } else {
            error += "'occasion' should a String;";
        }
    } else {
        error += "should contain 'occasion';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t resultLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < resultLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<ScriptCoverage> tmpResult = ScriptCoverage::Create(ecmaVm, resultValue);
                preciseCoverageDeltaUpdate->result_.emplace_back(std::move(tmpResult));
            }
        } else {
            error += "'result' should be an Array;";
        }
    } else {
        error += "should contain 'result';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PreciseCoverageDeltaUpdate::Create " << error;
        return nullptr;
    }

    return preciseCoverageDeltaUpdate;
}

Local<ObjectRef> PreciseCoverageDeltaUpdate::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        IntegerRef::New(ecmaVm, timestamp_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "occasion")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, occasion_.c_str())));
    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> result = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, result);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<HeapStatsUpdate> HeapStatsUpdate::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "HeapStatsUpdate::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto heapStatsUpdate = std::make_unique<HeapStatsUpdate>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "statsUpdate")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t resultLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < resultLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                int32_t statsUpdate;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                statsUpdate = resultValue->Int32Value(ecmaVm);
                heapStatsUpdate ->statsUpdate_[i] = statsUpdate;
            }
        } else {
            error += "'statsUpdate' should be an Array;";
        }
    } else {
        error += "should contain 'statsUpdate';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "HeapStatsUpdate::Create " << error;
        return nullptr;
    }

    return heapStatsUpdate;
}

Local<ObjectRef> HeapStatsUpdate::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    size_t len = statsUpdate_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<IntegerRef> elem = IntegerRef::New(ecmaVm, statsUpdate_[i]);
        values->Set(ecmaVm, i, elem);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "statsUpdate")), values);
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<LastSeenObjectId> LastSeenObjectId::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "LastSeenObjectId::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto lastSeenObjectId = std::make_unique<LastSeenObjectId>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            lastSeenObjectId->timestamp_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'timestamp' should a Number;";
        }
    } else {
        error += "should contain 'timestamp';";
    }

    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lastSeenObjectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            lastSeenObjectId->lastSeenObjectId_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'lastSeenObjectId' should a Number;";
        }
    } else {
        error += "should contain 'lastSeenObjectId';";
    }
   
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "LastSeenObjectId::Create " << error;
        return nullptr;
    }

    return lastSeenObjectId;
}

Local<ObjectRef> LastSeenObjectId::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        IntegerRef::New(ecmaVm, timestamp_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lastSeenObjectId")),
        IntegerRef::New(ecmaVm, lastSeenObjectId_));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

std::unique_ptr<ReportHeapSnapshotProgress> ReportHeapSnapshotProgress::Create(const EcmaVM *ecmaVm,
                                                                               const Local<JSValueRef> &params)
{
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "ReportHeapSnapshotProgress::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto reportHeapSnapshotProgress = std::make_unique<ReportHeapSnapshotProgress>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "done")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            reportHeapSnapshotProgress->done_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'done' should a Number;";
        }
    } else {
        error += "should contain 'done';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "total")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            reportHeapSnapshotProgress->total_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'total' should a Number;";
        }
    } else {
        error += "should contain 'total';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "finished")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            reportHeapSnapshotProgress->finished_ = result->IsTrue();
        } else {
            error += "'finished' should a Boolean;";
        }
    }
   
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ReportHeapSnapshotProgress::Create " << error;
        return nullptr;
    }

    return reportHeapSnapshotProgress;
}

Local<ObjectRef> ReportHeapSnapshotProgress::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "done")),
        IntegerRef::New(ecmaVm, done_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "total")),
        IntegerRef::New(ecmaVm, total_));

    if (finished_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "finished")),
            BooleanRef::New(ecmaVm, finished_.value()));
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}
}  // namespace panda::ecmascript::tooling
