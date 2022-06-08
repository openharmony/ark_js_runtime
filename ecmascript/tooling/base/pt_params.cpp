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

#include "ecmascript/tooling/base/pt_params.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<EnableParams> EnableParams::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "EnableParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<EnableParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "maxScriptsCacheSize")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->maxScriptsCacheSize_ = Local<NumberRef>(result)->Value();
        } else {
            error += "'maxScriptsCacheSize' should be a Number;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "EnableParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<EnableParams> EnableParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<EnableParams>();

    double result = params.GetDouble("maxScriptsCacheSize", -1.0);
    if (result != -1.0) {
        // optional params
        paramsObject->maxScriptsCacheSize_ = result;
    }

    return paramsObject;
}

std::unique_ptr<EvaluateOnCallFrameParams> EvaluateOnCallFrameParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "EvaluateOnCallFrameParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<EvaluateOnCallFrameParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrameId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->callFrameId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'callframeid' should be a String;";
        }
    } else {
        error += "should contain 'callframeid';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "expression")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->expression_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'expression' should be a String;";
        }
    } else {
        error += "should contain 'expression';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectGroup")));
    if (!result.IsEmpty() && result->IsString()) {
        paramsObject->objectGroup_ = DebuggerApi::ToStdString(result);
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "includeCommandLineAPI")));
    if (!result.IsEmpty() && result->IsBoolean()) {
        paramsObject->includeCommandLineApi_ = result->IsTrue();
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "silent")));
    if (!result.IsEmpty() && result->IsBoolean()) {
        paramsObject->silent_ = result->IsTrue();
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "returnByValue")));
    if (!result.IsEmpty() && result->IsBoolean()) {
        paramsObject->returnByValue_ = result->IsTrue();
    }

    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "generatePreview")));
    if (!result.IsEmpty() && result->IsBoolean()) {
        paramsObject->generatePreview_ = result->IsTrue();
    }

    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "throwOnSideEffect")));
    if (!result.IsEmpty() && result->IsBoolean()) {
        paramsObject->throwOnSideEffect_ = result->IsTrue();
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "EvaluateOnCallFrameParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<GetPossibleBreakpointsParams> GetPossibleBreakpointsParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<GetPossibleBreakpointsParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "start")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'start' format error;";
            } else {
                paramsObject->start_ = std::move(obj);
            }
        } else {
            error += "'start' should be an Object;";
        }
    } else {
        error += "should contain 'start';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "end")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'end' format error;";
            } else {
                paramsObject->end_ = std::move(obj);
            }
        } else {
            error += "'end' should be an Object;";
        }
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "restrictToFunction")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->restrictToFunction_ = result->IsTrue();
        } else {
            error += "'restrictToFunction' should be a Boolean;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "GetPossibleBreakpointsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<GetScriptSourceParams> GetScriptSourceParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<GetScriptSourceParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "GetScriptSourceParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<RemoveBreakpointParams> RemoveBreakpointParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<RemoveBreakpointParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->breakpointId_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'breakpointId' should be a String;";
        }
    } else {
        error += "should contain 'breakpointId';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpointParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<ResumeParams> ResumeParams::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<ResumeParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "terminateOnResume")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->terminateOnResume_ = result->IsTrue();
        } else {
            error += "'terminateOnResume' should be a Boolean;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ResumeParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetAsyncCallStackDepthParams> SetAsyncCallStackDepthParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<SetAsyncCallStackDepthParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "maxDepth")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->maxDepth_ = static_cast<uint32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'maxDepth' should be a Number;";
        }
    } else {
        error += "should contain 'maxDepth';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SetAsyncCallStackDepthParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetBlackboxPatternsParams> SetBlackboxPatternsParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<SetBlackboxPatternsParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "patterns")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            Local<ArrayRef> array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; i++) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> value = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                if (value->IsString()) {
                    paramsObject->patterns_.emplace_back(
                        DebuggerApi::ToStdString(value));
                } else {
                    error += "'patterns' items should be a String;";
                }
            }
        } else {
            error += "'patterns' should be an Array;";
        }
    } else {
        error += "should contain 'patterns';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SetBlackboxPatternsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetBreakpointByUrlParams> SetBreakpointByUrlParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<SetBreakpointByUrlParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->line_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->url_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'url' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "urlRegex")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->urlRegex_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'urlRegex' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptHash")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->scriptHash_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'scriptHash' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->column_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "condition")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->condition_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'condition' should be a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SetBreakpointByUrlParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetPauseOnExceptionsParams> SetPauseOnExceptionsParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<SetPauseOnExceptionsParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "state")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            if (!paramsObject->StoreState(DebuggerApi::ToStdString(result))) {
                error += "'state' is invalid;";
            }
        } else {
            error += "'state' should be a String;";
        }
    } else {
        error += "should contain 'state';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SetPauseOnExceptionsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StepIntoParams> StepIntoParams::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StepIntoParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakOnAsyncCall")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->breakOnAsyncCall_ = result->IsTrue();
        } else {
            error += "'terminateOnResume' should be a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "skipList")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            Local<ArrayRef> array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; i++) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> value = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                if (value->IsObject()) {
                    std::unique_ptr<LocationRange> obj = LocationRange::Create(ecmaVm, value);
                    if (obj != nullptr) {
                        paramsObject->skipList_->emplace_back(std::move(obj));
                    } else {
                        error += "'skipList' items LocationRange is invalid;";
                    }
                } else {
                    error += "'skipList' items should be an Object;";
                }
            }
        } else {
            error += "'skipList' should be an Array;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StepIntoParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StepOverParams> StepOverParams::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StepOverParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "skipList")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            Local<ArrayRef> array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; i++) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> value = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                if (value->IsObject()) {
                    std::unique_ptr<LocationRange> obj = LocationRange::Create(ecmaVm, value);
                    if (obj != nullptr) {
                        paramsObject->skipList_->emplace_back(std::move(obj));
                    } else {
                        error += "'skipList' items LocationRange is invalid;";
                    }
                } else {
                    error += "'skipList' items should be an Object;";
                }
            }
        } else {
            error += "'skipList' should be an Array;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StepOverParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<GetPropertiesParams> GetPropertiesParams::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "GetPropertiesParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<GetPropertiesParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    } else {
        error += "should contain 'objectId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ownProperties")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->ownProperties_ = result->IsTrue();
        } else {
            error += "'ownProperties' should be a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "accessorPropertiesOnly")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->accessorPropertiesOnly_ = result->IsTrue();
        } else {
            error += "'accessorPropertiesOnly' should be a Boolean;";
        }
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "generatePreview")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->generatePreview_ = result->IsTrue();
        } else {
            error += "'generatePreview' should be a Boolean;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "GetPropertiesParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<CallFunctionOnParams> CallFunctionOnParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "CallFunctionOnParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<CallFunctionOnParams>();

    // paramsObject->functionDeclaration_
    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionDeclaration")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->functionDeclaration_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'functionDeclaration' should be a String;";
        }
    } else {
        error += "should contain 'functionDeclaration';";
    }
    // paramsObject->objectId_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    }
    // paramsObject->arguments_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "arguments")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            Local<ArrayRef> array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; i++) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> value = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                if (value->IsObject()) {
                    std::unique_ptr<CallArgument> obj = CallArgument::Create(ecmaVm, value);
                    if (obj != nullptr) {
                        paramsObject->arguments_->emplace_back(std::move(obj));
                    } else {
                        error += "'arguments' items CallArgument is invaild;";
                    }
                } else {
                    error += "'arguments' items should be an Object;";
                }
            }
        } else {
            error += "'arguments' should be an Array;";
        }
    }
    // paramsObject->silent_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "silent")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->silent_ = result->IsTrue();
        } else {
            error += "'silent' should be a Boolean;";
        }
    }
    // paramsObject->returnByValue_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "returnByValue")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->returnByValue_ = result->IsTrue();
        } else {
            error += "'returnByValue' should be a Boolean;";
        }
    }
    // paramsObject->generatePreview_
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "generatePreview")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->generatePreview_ = result->IsTrue();
        } else {
            error += "'generatePreview' should be a Boolean;";
        }
    }
    // paramsObject->userGesture_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "userGesture")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->userGesture_ = result->IsTrue();
        } else {
            error += "'userGesture' should be a Boolean;";
        }
    }
    // paramsObject->awaitPromise_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "awaitPromise")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->awaitPromise_ = result->IsTrue();
        } else {
            error += "'awaitPromise' should be a Boolean;";
        }
    }
    // paramsObject->executionContextId_
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->executionContextId_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'executionContextId' should be a Number;";
        }
    }
    // paramsObject->objectGroup_
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectGroup")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectGroup_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'objectGroup' should be a String;";
        }
    }
    // paramsObject->throwOnSideEffect_
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "throwOnSideEffect")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->throwOnSideEffect_ = result->IsTrue();
        } else {
            error += "'throwOnSideEffect' should be a Boolean;";
        }
    }
    // Check whether the error is empty.
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "CallFunctionOnParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StartSamplingParams> StartSamplingParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "StartSamplingParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StartSamplingParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samplingInterval")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->samplingInterval_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'samplingInterval' should be a Number;";
        }
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StartSamplingParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> StartSamplingParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samplingInterval")),
            NumberRef::New(ecmaVm, static_cast<int64_t>(samplingInterval_.value())));

    return params;
}

std::unique_ptr<StartTrackingHeapObjectsParams> StartTrackingHeapObjectsParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "StartTrackingHeapObjectsParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StartTrackingHeapObjectsParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "trackAllocations")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->trackAllocations_ = result->IsTrue();
        } else {
            error += "'trackAllocations' should be a boolean;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StartTrackingHeapObjectsParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> StartTrackingHeapObjectsParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "trackAllocations")),
        BooleanRef::New(ecmaVm, trackAllocations_.value()));

    return params;
}

std::unique_ptr<StopTrackingHeapObjectsParams> StopTrackingHeapObjectsParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "StopTrackingHeapObjectsParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StopTrackingHeapObjectsParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "reportProgress")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->reportProgress_ = result->IsTrue();
        } else {
            error += "'reportProgress' should be a boolean;";
        }
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "treatGlobalObjectsAsRoots")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->treatGlobalObjectsAsRoots_ = result->IsTrue();
        } else {
            error += "'treatGlobalObjectsAsRoots' should be a boolean;";
        }
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "captureNumericValue")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->captureNumericValue_ = result->IsTrue();
        } else {
            error += "'captureNumericValue' should be a boolean;";
        }
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StopTrackingHeapObjectsParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> StopTrackingHeapObjectsParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "reportProgress")),
        BooleanRef::New(ecmaVm, reportProgress_.value()));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "treatGlobalObjectsAsRoots")),
        BooleanRef::New(ecmaVm, treatGlobalObjectsAsRoots_.value()));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "captureNumericValue")),
        BooleanRef::New(ecmaVm, captureNumericValue_.value()));

    return params;
}

std::unique_ptr<AddInspectedHeapObjectParams> AddInspectedHeapObjectParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "AddInspectedHeapObjectParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<AddInspectedHeapObjectParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "heapObjectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->heapObjectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'heapObjectId' should be a String;";
        }
    } else {
        error += "should contain 'heapObjectId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "AddInspectedHeapObjectParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> AddInspectedHeapObjectParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "heapObjectId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(heapObjectId_).c_str())));

    return params;
}

std::unique_ptr<GetHeapObjectIdParams> GetHeapObjectIdParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "GetHeapObjectIdParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<GetHeapObjectIdParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    } else {
        error += "should contain 'objectId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "GetHeapObjectIdParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> GetHeapObjectIdParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(objectId_).c_str())));
    return params;
}

std::unique_ptr<GetObjectByHeapObjectIdParams> GetObjectByHeapObjectIdParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "GetObjectByHeapObjectIdParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<GetObjectByHeapObjectIdParams>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    } else {
        error += "should contain 'objectId';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectGroup")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            paramsObject->objectGroup_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'objectGroup' should be a String;";
        }
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "GetObjectByHeapObjectIdParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> GetObjectByHeapObjectIdParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(objectId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectGroup")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, objectGroup_->c_str())));
    return params;
}

std::unique_ptr<StartPreciseCoverageParam> StartPreciseCoverageParam::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "StartPreciseCoverageParam::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<StartPreciseCoverageParam>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callCount")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->callCount_ = result->IsTrue();
        } else {
            error += "'callCount' should be a boolean;";
        }
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "detailed")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->detailed_ = result->IsTrue();
        } else {
            error += "'detailed' should be a boolean;";
        }
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "allowTriggeredUpdates")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            paramsObject->allowTriggeredUpdates_ = result->IsTrue();
        } else {
            error += "'allowTriggeredUpdates' should be a boolean;";
        }
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "StartPreciseCoverageParam::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> StartPreciseCoverageParam::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callCount")),
        BooleanRef::New(ecmaVm, callCount_.value()));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "detailed")),
        BooleanRef::New(ecmaVm, detailed_.value()));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "allowTriggeredUpdates")),
        BooleanRef::New(ecmaVm, allowTriggeredUpdates_.value()));

    return params;
}

std::unique_ptr<SetSamplingIntervalParams> SetSamplingIntervalParams::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    ASSERT(ecmaVm);
    if (params.IsEmpty()) {
        LOG(ERROR, DEBUGGER) << "SetSamplingIntervalParams::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto paramsObject = std::make_unique<SetSamplingIntervalParams>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "interval")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            paramsObject->interval_ = static_cast<int>(Local<NumberRef>(result)->Value());
        } else {
            error += "'interval' should be a Number;";
        }
    } else {
        error += "should contain 'interval';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SetSamplingIntervalParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

Local<ObjectRef> SetSamplingIntervalParams::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "interval")),
        IntegerRef::New(ecmaVm, interval_));
    return params;
}
}  // namespace panda::ecmascript::tooling
