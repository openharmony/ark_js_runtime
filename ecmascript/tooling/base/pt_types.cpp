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

#include "pt_types.h"

namespace panda::ecmascript::tooling {
using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

const CString ObjectType::Object = "object";        // NOLINT (readability-identifier-naming)
const CString ObjectType::Function = "function";    // NOLINT (readability-identifier-naming)
const CString ObjectType::Undefined = "undefined";  // NOLINT (readability-identifier-naming)
const CString ObjectType::String = "string";        // NOLINT (readability-identifier-naming)
const CString ObjectType::Number = "number";        // NOLINT (readability-identifier-naming)
const CString ObjectType::Boolean = "boolean";      // NOLINT (readability-identifier-naming)
const CString ObjectType::Symbol = "symbol";        // NOLINT (readability-identifier-naming)
const CString ObjectType::Bigint = "bigint";        // NOLINT (readability-identifier-naming)
const CString ObjectType::Wasm = "wasm";            // NOLINT (readability-identifier-naming)

const CString ObjectSubType::Array = "array";              // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Null = "null";                // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Node = "node";                // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Regexp = "regexp";            // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Date = "date";                // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Map = "map";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Set = "set";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Weakmap = "weakmap";          // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Weakset = "weakset";          // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Iterator = "iterator";        // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Generator = "generator";      // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Error = "error";              // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Proxy = "proxy";              // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Promise = "promise";          // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Typedarray = "typedarray";    // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Arraybuffer = "arraybuffer";  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Dataview = "dataview";        // NOLINT (readability-identifier-naming)
const CString ObjectSubType::I32 = "i32";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::I64 = "i64";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::F32 = "f32";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::F64 = "f64";                  // NOLINT (readability-identifier-naming)
const CString ObjectSubType::V128 = "v128";                // NOLINT (readability-identifier-naming)
const CString ObjectSubType::Externref = "externref";      // NOLINT (readability-identifier-naming)

const CString ObjectClassName::Object = "Object";                  // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Function = "Function";              // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Array = "Array";                    // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Regexp = "RegExp";                  // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Date = "Date";                      // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Map = "Map";                        // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Set = "Set";                        // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Weakmap = "Weakmap";                // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Weakset = "Weakset";                // NOLINT (readability-identifier-naming)
const CString ObjectClassName::ArrayIterator = "ArrayIterator";    // NOLINT (readability-identifier-naming)
const CString ObjectClassName::StringIterator = "StringIterator";  // NOLINT (readability-identifier-naming)
const CString ObjectClassName::SetIterator = "SetIterator";        // NOLINT (readability-identifier-naming)
const CString ObjectClassName::MapIterator = "MapIterator";        // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Iterator = "Iterator";              // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Error = "Error";                    // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Proxy = "Object";                   // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Promise = "Promise";                // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Typedarray = "Typedarray";          // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Arraybuffer = "Arraybuffer";        // NOLINT (readability-identifier-naming)
const CString ObjectClassName::Global = "global";                  // NOLINT (readability-identifier-naming)

const CString RemoteObject::ObjectDescription = "Object";    // NOLINT (readability-identifier-naming)
const CString RemoteObject::GlobalDescription = "global";    // NOLINT (readability-identifier-naming)
const CString RemoteObject::ProxyDescription = "Proxy";      // NOLINT (readability-identifier-naming)
const CString RemoteObject::PromiseDescription = "Promise";  // NOLINT (readability-identifier-naming)
const CString RemoteObject::ArrayIteratorDescription =       // NOLINT (readability-identifier-naming)
    "ArrayIterator";
const CString RemoteObject::StringIteratorDescription =  // NOLINT (readability-identifier-naming)
    "StringIterator";
const CString RemoteObject::SetIteratorDescription = "SetIterator";  // NOLINT (readability-identifier-naming)
const CString RemoteObject::MapIteratorDescription = "MapIterator";  // NOLINT (readability-identifier-naming)
const CString RemoteObject::WeakMapDescription = "WeakMap";          // NOLINT (readability-identifier-naming)
const CString RemoteObject::WeakSetDescription = "WeakSet";          // NOLINT (readability-identifier-naming)

static constexpr uint64_t DOUBLE_SIGN_MASK = 0x8000000000000000ULL;

Local<ObjectRef> PtBaseTypes::NewObject(const EcmaVM *ecmaVm)
{
    return ObjectRef::New(ecmaVm);
}

std::unique_ptr<RemoteObject> RemoteObject::FromTagged(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
{
    if (tagged->IsNull() || tagged->IsUndefined() ||
        tagged->IsBoolean() || tagged->IsNumber() ||
        tagged->IsBigInt()) {
        return std::make_unique<PrimitiveRemoteObject>(ecmaVm, tagged);
    }
    if (tagged->IsString()) {
        return std::make_unique<StringRemoteObject>(ecmaVm, tagged);
    }
    if (tagged->IsSymbol()) {
        return std::make_unique<SymbolRemoteObject>(ecmaVm, tagged);
    }
    if (tagged->IsFunction()) {
        return std::make_unique<FunctionRemoteObject>(ecmaVm, tagged);
    }
    if (tagged->IsArray(ecmaVm)) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Array, ObjectSubType::Array);
    }
    if (tagged->IsRegExp()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Regexp, ObjectSubType::Regexp);
    }
    if (tagged->IsDate()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Date, ObjectSubType::Date);
    }
    if (tagged->IsMap()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Map, ObjectSubType::Map);
    }
    if (tagged->IsWeakMap()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Weakmap, ObjectSubType::Weakmap);
    }
    if (tagged->IsSet()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Set, ObjectSubType::Set);
    }
    if (tagged->IsWeakSet()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Weakset, ObjectSubType::Weakset);
    }
    if (tagged->IsError()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Error, ObjectSubType::Error);
    }
    if (tagged->IsProxy()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Proxy, ObjectSubType::Proxy);
    }
    if (tagged->IsPromise()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Promise, ObjectSubType::Promise);
    }
    if (tagged->IsArrayBuffer()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Arraybuffer,
            ObjectSubType::Arraybuffer);
    }
    if (tagged->IsArrayIterator()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::ArrayIterator);
    }
    if (tagged->IsStringIterator()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::StringIterator);
    }
    if (tagged->IsSetIterator()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::SetIterator,
            ObjectSubType::Iterator);
    }
    if (tagged->IsMapIterator()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::MapIterator,
            ObjectSubType::Iterator);
    }
    if (tagged->IsObject()) {
        return std::make_unique<ObjectRemoteObject>(ecmaVm, tagged, ObjectClassName::Object);
    }
    std::unique_ptr<RemoteObject> object = std::make_unique<RemoteObject>();
    object->SetType(ObjectType::Undefined);
    return object;
}

PrimitiveRemoteObject::PrimitiveRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
{
    if (tagged->IsNull()) {
        this->SetType(ObjectType::Object).SetSubType(ObjectSubType::Null).SetValue(tagged);
    } else if (tagged->IsBoolean()) {
        this->SetType(ObjectType::Boolean).SetValue(tagged);
    } else if (tagged->IsUndefined()) {
        this->SetType(ObjectType::Undefined);
    } else if (tagged->IsNumber()) {
        this->SetType(ObjectType::Number)
            .SetDescription(DebuggerApi::ToCString(tagged->ToString(ecmaVm)));
        double val = Local<NumberRef>(tagged)->Value();
        if (!std::isfinite(val) || (val == 0 && ((bit_cast<uint64_t>(val) & DOUBLE_SIGN_MASK) == DOUBLE_SIGN_MASK))) {
            this->SetUnserializableValue(this->GetDescription());
        } else {
            this->SetValue(tagged);
        }
    } else if (tagged->IsBigInt()) {
        std::string literal = tagged->ToString(ecmaVm)->ToString() + "n"; // n : BigInt literal postfix
        this->SetType(ObjectType::Bigint).SetValue(StringRef::NewFromUtf8(ecmaVm, literal.data()));
    }
}

CString ObjectRemoteObject::DescriptionForObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
{
    if (tagged->IsArray(ecmaVm)) {
        return DescriptionForArray(ecmaVm, Local<ArrayRef>(tagged));
    }
    if (tagged->IsRegExp()) {
        return DescriptionForRegexp(ecmaVm, Local<RegExpRef>(tagged));
    }
    if (tagged->IsDate()) {
        return DescriptionForDate(ecmaVm, Local<DateRef>(tagged));
    }
    if (tagged->IsMap()) {
        return DescriptionForMap(Local<MapRef>(tagged));
    }
    if (tagged->IsWeakMap()) {
        return RemoteObject::WeakMapDescription;
    }
    if (tagged->IsSet()) {
        return DescriptionForSet(Local<SetRef>(tagged));
    }
    if (tagged->IsWeakSet()) {
        return RemoteObject::WeakSetDescription;
    }
    if (tagged->IsError()) {
        return DescriptionForError(ecmaVm, tagged);
    }
    if (tagged->IsProxy()) {
        return RemoteObject::ProxyDescription;
    }
    if (tagged->IsPromise()) {
        return RemoteObject::PromiseDescription;
    }
    if (tagged->IsArrayIterator()) {
        return RemoteObject::ArrayIteratorDescription;
    }
    if (tagged->IsStringIterator()) {
        return RemoteObject::StringIteratorDescription;
    }
    if (tagged->IsSetIterator()) {
        return RemoteObject::SetIteratorDescription;
    }
    if (tagged->IsMapIterator()) {
        return RemoteObject::MapIteratorDescription;
    }
    if (tagged->IsArrayBuffer()) {
        return DescriptionForArrayBuffer(ecmaVm, Local<ArrayBufferRef>(tagged));
    }
    return RemoteObject::ObjectDescription;
}

CString ObjectRemoteObject::DescriptionForArray(const EcmaVM *ecmaVm, const Local<ArrayRef> &tagged)
{
    CString description = "Array(" + ToCString<uint32_t>(tagged->Length(ecmaVm)) + ")";
    return description;
}

CString ObjectRemoteObject::DescriptionForRegexp(const EcmaVM *ecmaVm, const Local<RegExpRef> &tagged)
{
    CString regexpSource = DebuggerApi::ToCString(tagged->GetOriginalSource(ecmaVm));
    CString description = "/" + regexpSource + "/";
    return description;
}

CString ObjectRemoteObject::DescriptionForDate(const EcmaVM *ecmaVm, const Local<DateRef> &tagged)
{
    CString description = DebuggerApi::ToCString(tagged->ToString(ecmaVm));
    return description;
}

CString ObjectRemoteObject::DescriptionForMap(const Local<MapRef> &tagged)
{
    CString description = ("Map(" + ToCString<uint32_t>(tagged->GetSize()) + ")");
    return description;
}

CString ObjectRemoteObject::DescriptionForSet(const Local<SetRef> &tagged)
{
    CString description = ("Set(" + ToCString<uint32_t>(tagged->GetSize()) + ")");
    return description;
}

CString ObjectRemoteObject::DescriptionForError(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
{
    // add message
    Local<JSValueRef> stack = StringRef::NewFromUtf8(ecmaVm, "message");
    Local<JSValueRef> result = Local<ObjectRef>(tagged)->Get(ecmaVm, stack);
    return DebuggerApi::ToCString(result->ToString(ecmaVm));
}

CString ObjectRemoteObject::DescriptionForArrayBuffer(const EcmaVM *ecmaVm, const Local<ArrayBufferRef> &tagged)
{
    int32_t len = tagged->ByteLength(ecmaVm);
    CString description = ("ArrayBuffer(" + ToCString<uint32_t>(len) + ")");
    return description;
}

CString SymbolRemoteObject::DescriptionForSymbol(const EcmaVM *ecmaVm, const Local<SymbolRef> &tagged) const
{
    CString description =
        "Symbol(" + DebuggerApi::ToCString(tagged->GetDescription(ecmaVm)) + ")";
    return description;
}

CString FunctionRemoteObject::DescriptionForFunction(const EcmaVM *ecmaVm, const Local<FunctionRef> &tagged) const
{
    CString sourceCode;
    if (tagged->IsNative(ecmaVm)) {
        sourceCode = "[native code]";
    } else {
        sourceCode = "[js code]";
    }
    Local<StringRef> name = tagged->GetName(ecmaVm);
    CString description = "function " + DebuggerApi::ToCString(name) + "( { " + sourceCode + " }";
    return description;
}

std::unique_ptr<RemoteObject> RemoteObject::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto remoteObject = std::make_unique<RemoteObject>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToCString(result);
            if (ObjectType::Valid(type)) {
                remoteObject->type_ = type;
            } else {
                error += "'type' is invalid;";
            }
        } else {
            error += "'type' should be a String;";
        }
    } else {
        error += "should contain 'type';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "subtype")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToCString(result);
            if (ObjectSubType::Valid(type)) {
                remoteObject->subtype_ = type;
            } else {
                error += "'subtype' is invalid;";
            }
        } else {
            error += "'subtype' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "className")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            remoteObject->className_ = DebuggerApi::ToCString(result);
        } else {
            error += "'className' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        remoteObject->value_ = result;
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "unserializableValue")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            remoteObject->unserializableValue_ = DebuggerApi::ToCString(result);
        } else {
            error += "'unserializableValue' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "description")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            remoteObject->description_ = DebuggerApi::ToCString(result);
        } else {
            error += "'description' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            remoteObject->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create " << error;
        return nullptr;
    }

    return remoteObject;
}

Local<ObjectRef> RemoteObject::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, type_.c_str())));
    if (subtype_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "subtype")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, subtype_->c_str())));
    }
    if (className_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "className")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, className_->c_str())));
    }
    if (value_) {
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")), value_.value());
    }
    if (unserializableValue_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "unserializableValue")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, unserializableValue_->c_str())));
    }
    if (description_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "description")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, description_->c_str())));
    }
    if (objectId_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(objectId_.value()).c_str())));
    }

    return params;
}

std::unique_ptr<ExceptionDetails> ExceptionDetails::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ExceptionDetails::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto exceptionDetails = std::make_unique<ExceptionDetails>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            exceptionDetails->exceptionId_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'exceptionId' should be a Number;";
        }
    } else {
        error += "should contain 'exceptionId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "text")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            exceptionDetails->text_ = DebuggerApi::ToCString(result);
        } else {
            error += "'text' should be a String;";
        }
    } else {
        error += "should contain 'text';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            exceptionDetails->line_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            exceptionDetails->column_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    } else {
        error += "should contain 'columnNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            exceptionDetails->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            exceptionDetails->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exception")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'exception' format error;";
            } else {
                exceptionDetails->exception_ = std::move(obj);
            }
        } else {
            error += "'exception' should be an Object;";
        }
    }

    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            exceptionDetails->executionContextId_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'executionContextId' should be a Number;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ExceptionDetails::Create " << error;
        return nullptr;
    }

    return exceptionDetails;
}

Local<ObjectRef> ExceptionDetails::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionId")),
        IntegerRef::New(ecmaVm, exceptionId_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "text")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, text_.c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, line_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
        IntegerRef::New(ecmaVm, column_));
    if (scriptId_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_.value()).c_str())));
    }
    if (url_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_->c_str())));
    }
    if (exception_) {
        ASSERT(exception_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exception")),
            Local<JSValueRef>(exception_.value()->ToObject(ecmaVm)));
    }
    if (executionContextId_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")),
            IntegerRef::New(ecmaVm, executionContextId_.value()));
    }

    return params;
}

std::unique_ptr<InternalPropertyDescriptor> InternalPropertyDescriptor::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "InternalPropertyDescriptor::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto internalPropertyDescriptor = std::make_unique<InternalPropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            internalPropertyDescriptor->name_ = DebuggerApi::ToCString(result);
        } else {
            error += "'name' should be a String;";
        }
    } else {
        error += "should contain 'name';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'value' format error;";
            } else {
                internalPropertyDescriptor->value_ = std::move(obj);
            }
        } else {
            error += "'value' should be an Object;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "InternalPropertyDescriptor::Create " << error;
        return nullptr;
    }

    return internalPropertyDescriptor;
}

Local<ObjectRef> InternalPropertyDescriptor::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_.c_str())));
    if (value_) {
        ASSERT(value_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")),
            Local<JSValueRef>(value_.value()->ToObject(ecmaVm)));
    }

    return params;
}

std::unique_ptr<PrivatePropertyDescriptor> PrivatePropertyDescriptor::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "PrivatePropertyDescriptor::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto propertyDescriptor = std::make_unique<PrivatePropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            propertyDescriptor->name_ = DebuggerApi::ToCString(result);
        } else {
            error += "'name' should be a String;";
        }
    } else {
        error += "should contain 'name';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'value' format error;";
            } else {
                propertyDescriptor->value_ = std::move(obj);
            }
        } else {
            error += "'value' should be a Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "get")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'get' format error;";
            } else {
                propertyDescriptor->get_ = std::move(obj);
            }
        } else {
            error += "'get' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "set")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'set' format error;";
            } else {
                propertyDescriptor->set_ = std::move(obj);
            }
        } else {
            error += "'set' should be an Object;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PrivatePropertyDescriptor::Create " << error;
        return nullptr;
    }

    return propertyDescriptor;
}

Local<ObjectRef> PrivatePropertyDescriptor::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_.c_str())));
    if (value_) {
        ASSERT(value_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")),
            Local<JSValueRef>(value_.value()->ToObject(ecmaVm)));
    }
    if (get_) {
        ASSERT(get_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "get")),
            Local<JSValueRef>(get_.value()->ToObject(ecmaVm)));
    }
    if (set_) {
        ASSERT(set_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "set")),
            Local<JSValueRef>(set_.value()->ToObject(ecmaVm)));
    }

    return params;
}

std::unique_ptr<PropertyDescriptor> PropertyDescriptor::FromProperty(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &name, const PropertyAttribute &property)
{
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();

    CString nameStr;
    if (name->IsSymbol()) {
        Local<SymbolRef> symbol(name);
        nameStr =
            "Symbol(" + DebuggerApi::ToCString(Local<SymbolRef>(name)->GetDescription(ecmaVm)) + ")";
        debuggerProperty->symbol_ = RemoteObject::FromTagged(ecmaVm, name);
    } else {
        nameStr = DebuggerApi::ToCString(name->ToString(ecmaVm));
    }

    debuggerProperty->name_ = nameStr;
    if (property.HasValue()) {
        debuggerProperty->value_ = RemoteObject::FromTagged(ecmaVm, property.GetValue(ecmaVm));
    }
    if (property.HasWritable()) {
        debuggerProperty->writable_ = property.IsWritable();
    }
    if (property.HasGetter()) {
        debuggerProperty->get_ = RemoteObject::FromTagged(ecmaVm, property.GetGetter(ecmaVm));
    }
    if (property.HasSetter()) {
        debuggerProperty->set_ = RemoteObject::FromTagged(ecmaVm, property.GetSetter(ecmaVm));
    }
    debuggerProperty->configurable_ = property.IsConfigurable();
    debuggerProperty->enumerable_ = property.IsEnumerable();
    debuggerProperty->isOwn_ = true;

    return debuggerProperty;
}

std::unique_ptr<PropertyDescriptor> PropertyDescriptor::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "PropertyDescriptor::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto propertyDescriptor = std::make_unique<PropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            propertyDescriptor->name_ = DebuggerApi::ToCString(result);
        } else {
            error += "'name' should be a String;";
        }
    } else {
        error += "should contain 'name';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'value' format error;";
            } else {
                propertyDescriptor->value_ = std::move(obj);
            }
        } else {
            error += "'value' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "writable")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            propertyDescriptor->writable_ = result->IsTrue();
        } else {
            error += "'writable' should be a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "get")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'get' format error;";
            } else {
                propertyDescriptor->get_ = std::move(obj);
            }
        } else {
            error += "'get' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "set")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'set' format error;";
            } else {
                propertyDescriptor->set_ = std::move(obj);
            }
        } else {
            error += "'set' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "configurable")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            propertyDescriptor->configurable_ = result->IsTrue();
        } else {
            error += "'configurable' should be a Boolean;";
        }
    } else {
        error += "should contain 'configurable';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "enumerable")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            propertyDescriptor->enumerable_ = result->IsTrue();
        } else {
            error += "'enumerable' should be a Boolean;";
        }
    } else {
        error += "should contain 'enumerable';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "wasThrown")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            propertyDescriptor->wasThrown_ = result->IsTrue();
        } else {
            error += "'wasThrown' should be a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isOwn")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            propertyDescriptor->isOwn_ = result->IsTrue();
        } else {
            error += "'isOwn' should be a Boolean;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "symbol")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'symbol' format error;";
            } else {
                propertyDescriptor->symbol_ = std::move(obj);
            }
        } else {
            error += "'symbol' should be an Object;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PropertyDescriptor::Create " << error;
        return nullptr;
    }

    return propertyDescriptor;
}

Local<ObjectRef> PropertyDescriptor::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_.c_str())));
    if (value_) {
        ASSERT(value_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")),
            Local<JSValueRef>(value_.value()->ToObject(ecmaVm)));
    }
    if (writable_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "writable")),
            BooleanRef::New(ecmaVm, writable_.value()));
    }
    if (get_) {
        ASSERT(get_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "get")),
            Local<JSValueRef>(get_.value()->ToObject(ecmaVm)));
    }
    if (set_) {
        ASSERT(set_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "set")),
            Local<JSValueRef>(set_.value()->ToObject(ecmaVm)));
    }
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "configurable")),
        BooleanRef::New(ecmaVm, configurable_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "enumerable")),
        BooleanRef::New(ecmaVm, enumerable_));
    if (wasThrown_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "wasThrown")),
            BooleanRef::New(ecmaVm, wasThrown_.value()));
    }
    if (isOwn_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isOwn")),
            BooleanRef::New(ecmaVm, isOwn_.value()));
    }
    if (symbol_) {
        ASSERT(symbol_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "symbol")),
            Local<JSValueRef>(symbol_.value()->ToObject(ecmaVm)));
    }

    return params;
}

std::unique_ptr<CallArgument> CallArgument::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "CallArgument::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto callArgument = std::make_unique<CallArgument>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        callArgument->value_ = result;
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "unserializableValue")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callArgument->unserializableValue_ = DebuggerApi::ToCString(result);
        } else {
            error += "'unserializableValue' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callArgument->objectId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'objectId' should be a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "CallArgument::Create " << error;
        return nullptr;
    }

    return callArgument;
}

Local<ObjectRef> CallArgument::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    if (value_) {
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "value")), value_.value());
    }
    if (unserializableValue_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "unserializableValue")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, unserializableValue_->c_str())));
    }
    if (objectId_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "objectId")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(objectId_.value()).c_str())));
    }

    return params;
}

std::unique_ptr<Location> Location::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Location::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto location = std::make_unique<Location>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            location->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            location->line_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            location->column_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return location;
}

Local<ObjectRef> Location::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, line_));
    if (column_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
            IntegerRef::New(ecmaVm, column_.value()));
    }

    return params;
}

std::unique_ptr<ScriptPosition> ScriptPosition::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptPosition::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scriptPosition = std::make_unique<ScriptPosition>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptPosition->line_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            scriptPosition->column_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    } else {
        error += "should contain 'columnNumber';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptPosition::Create " << error;
        return nullptr;
    }

    return scriptPosition;
}

Local<ObjectRef> ScriptPosition::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, line_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
        IntegerRef::New(ecmaVm, column_));

    return params;
}

std::unique_ptr<SearchMatch> SearchMatch::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SearchMatch::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto locationSearch = std::make_unique<SearchMatch>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            locationSearch->lineNumber_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineContent")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            locationSearch->lineContent_ = DebuggerApi::ToCString(result);
        } else {
            error += "'lineContent' should be a String;";
        }
    } else {
        error += "should contain 'lineContent';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SearchMatch::Create " << error;
        return nullptr;
    }

    return locationSearch;
}

Local<ObjectRef> SearchMatch::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, lineNumber_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineContent")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, lineContent_.c_str())));
    return params;
}

std::unique_ptr<LocationRange> LocationRange::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "BreakLocation::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto locationRange = std::make_unique<LocationRange>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            locationRange->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "start")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<ScriptPosition> obj = ScriptPosition::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'start' format error;";
            } else {
                locationRange->start_ = std::move(obj);
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
            std::unique_ptr<ScriptPosition> obj = ScriptPosition::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'end' format error;";
            } else {
                locationRange->end_ = std::move(obj);
            }
        } else {
            error += "'end' should be an Object;";
        }
    } else {
        error += "should contain 'end';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "LocationRange::Create " << error;
        return nullptr;
    }

    return locationRange;
}

Local<ObjectRef> LocationRange::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    ASSERT(start_ != nullptr);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "object")),
        Local<JSValueRef>(start_->ToObject(ecmaVm)));
    ASSERT(end_ != nullptr);
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "object")),
        Local<JSValueRef>(end_->ToObject(ecmaVm)));

    return params;
}

std::unique_ptr<BreakLocation> BreakLocation::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "BreakLocation::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto breakLocation = std::make_unique<BreakLocation>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            breakLocation->scriptId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            breakLocation->line_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            breakLocation->column_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToCString(result);
            if (BreakType::Valid(type)) {
                breakLocation->type_ = type;
            } else {
                error += "'type' is invalid;";
            }
        } else {
            error += "'type' should be a String;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return breakLocation;
}

Local<ObjectRef> BreakLocation::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, line_));
    if (column_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
            IntegerRef::New(ecmaVm, column_.value()));
    }
    if (type_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, type_->c_str())));
    }

    return params;
}

std::unique_ptr<Scope> Scope::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Scope::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scope = std::make_unique<Scope>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToCString(result);
            if (Scope::Type::Valid(type)) {
                scope->type_ = type;
            } else {
                error += "'type' is invalid;";
            }
        } else {
            error += "'type' should be a String;";
        }
    } else {
        error += "should contain 'type';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "object")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'object' format error;";
            } else {
                scope->object_ = std::move(obj);
            }
        } else {
            error += "'object' should be an Object;";
        }
    } else {
        error += "should contain 'object';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scope->name_ = DebuggerApi::ToCString(result);
        } else {
            error += "'name' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLocation")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'startLocation' format error;";
            } else {
                scope->startLocation_ = std::move(obj);
            }
        } else {
            error += "'startLocation' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLocation")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'endLocation' format error;";
            } else {
                scope->endLocation_ = std::move(obj);
            }
        } else {
            error += "'endLocation' should be an Object;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return scope;
}

Local<ObjectRef> Scope::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, type_.c_str())));
    ASSERT(object_ != nullptr);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "object")),
        Local<JSValueRef>(object_->ToObject(ecmaVm)));
    if (name_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_->c_str())));
    }
    if (startLocation_) {
        ASSERT(startLocation_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLocation")),
            Local<JSValueRef>(startLocation_.value()->ToObject(ecmaVm)));
    }
    if (endLocation_) {
        ASSERT(endLocation_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLocation")),
            Local<JSValueRef>(endLocation_.value()->ToObject(ecmaVm)));
    }

    return params;
}

std::unique_ptr<CallFrame> CallFrame::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "CallFrame::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto callFrame = std::make_unique<CallFrame>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrameId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callFrame->callFrameId_ = static_cast<uint32_t>(DebuggerApi::StringToInt(result));
        } else {
            error += "'callFrameId' should be a String;";
        }
    } else {
        error += "should contain 'callFrameId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callFrame->functionName_ = DebuggerApi::ToCString(result);
        } else {
            error += "'functionName' should be a String;";
        }
    } else {
        error += "should contain 'functionName';";
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionLocation")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'functionLocation' format error;";
            } else {
                callFrame->functionLocation_ = std::move(obj);
            }
        } else {
            error += "'functionLocation' should be an Object;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<Location> obj = Location::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'location' format error;";
            } else {
                callFrame->location_ = std::move(obj);
            }
        } else {
            error += "'location' should be an Object;";
        }
    } else {
        error += "should contain 'location';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callFrame->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should be a String;";
        }
    } else {
        error += "should contain 'url';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scopeChain")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<Scope> scope = Scope::Create(ecmaVm, resultValue);
                if (resultValue.IsEmpty() || scope == nullptr) {
                    error += "'scopeChain' format invalid;";
                }
                callFrame->scopeChain_.emplace_back(std::move(scope));
            }
        } else {
            error += "'scopeChain' should be an Array;";
        }
    } else {
        error += "should contain 'scopeChain';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "this")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'this' format error;";
            } else {
                callFrame->this_ = std::move(obj);
            }
        } else {
            error += "'this' should be an Object;";
        }
    } else {
        error += "should contain 'this';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "returnValue")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RemoteObject> obj = RemoteObject::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'returnValue' format error;";
            } else {
                callFrame->returnValue_ = std::move(obj);
            }
        } else {
            error += "'returnValue' should be an Object;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "CallFrame::Create " << error;
        return nullptr;
    }

    return callFrame;
}

Local<ObjectRef> CallFrame::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrameId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(callFrameId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, functionName_.c_str())));
    if (functionLocation_) {
        ASSERT(functionLocation_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionLocation")),
            Local<JSValueRef>(functionLocation_.value()->ToObject(ecmaVm)));
    }
    ASSERT(location_ != nullptr);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    size_t len = scopeChain_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> scope = scopeChain_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, scope);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scopeChain")), values);
    ASSERT(this_ != nullptr);
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "this")),
        Local<JSValueRef>(this_->ToObject(ecmaVm)));
    if (returnValue_) {
        ASSERT(returnValue_.value() != nullptr);
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "returnValue")),
            Local<JSValueRef>(returnValue_.value()->ToObject(ecmaVm)));
    }

    return params;
}

std::unique_ptr<SamplingHeapProfileSample> SamplingHeapProfileSample::Create(const EcmaVM *ecmaVm,
                                                                             const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileSample::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto samplingHeapProfileSample = std::make_unique<SamplingHeapProfileSample>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "size")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            samplingHeapProfileSample->size_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'size' should be a Number;";
        }
    } else {
        error += "should contain 'size';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ordinal")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            samplingHeapProfileSample->ordinal_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'ordinal' should be a Number;";
        }
    } else {
        error += "should contain 'ordinal';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodeId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            samplingHeapProfileSample->nodeId_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'nodeId' should be a Number;";
        }
    } else {
        error += "should contain 'nodeId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileSample::Create " << error;
        return nullptr;
    }

    return samplingHeapProfileSample;
}

Local<ObjectRef> SamplingHeapProfileSample::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "size")),
        IntegerRef::New(ecmaVm, size_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ordinal")),
        IntegerRef::New(ecmaVm, ordinal_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodeId")),
        IntegerRef::New(ecmaVm, nodeId_));

    return params;
}

std::unique_ptr<RuntimeCallFrame> RuntimeCallFrame::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "RuntimeCallFrame::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto runtimeCallFrame = std::make_unique<RuntimeCallFrame>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->functionName_ = DebuggerApi::ToCString(result);
        } else {
            error += "'functionName' should be a String;";
        }
    } else {
        error += "should contain 'functionName';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->scriptId_ = DebuggerApi::ToCString(result);
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should be a String;";
        }
    } else {
        error += "should contain 'url';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            runtimeCallFrame->lineNumber_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'lineNumber' should be a Number;";
        }
    } else {
        error += "should contain 'lineNumber';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            runtimeCallFrame->columnNumber_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'columnNumber' should be a Number;";
        }
    } else {
        error += "should contain 'columnNumber';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "RuntimeCallFrame::Create " << error;
        return nullptr;
    }

    return runtimeCallFrame;
}

Local<ObjectRef> RuntimeCallFrame::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, functionName_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptId_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, lineNumber_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
        IntegerRef::New(ecmaVm, columnNumber_));

    return params;
}

std::unique_ptr<SamplingHeapProfileNode> SamplingHeapProfileNode::Create(const EcmaVM *ecmaVm,
                                                                         const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileNode::Create params is nullptr";
        return nullptr;
    }

    CString error;
    auto samplingHeapProfileNode = std::make_unique<SamplingHeapProfileNode>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrame")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RuntimeCallFrame> obj = RuntimeCallFrame::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'callFrame' format error;";
            } else {
                samplingHeapProfileNode->callFrame_ = std::move(obj);
            }
        } else {
            error += "'callFrame' should be an Object;";
        }
    } else {
        error += "should contain 'callFrame';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "selfSize")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            samplingHeapProfileNode->selfSize_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'selfSize' should be a Number;";
        }
    } else {
        error += "should contain 'selfSize';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            samplingHeapProfileNode->id_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'id' should be a Number;";
        }
    } else {
        error += "should contain 'id';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "children")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<SamplingHeapProfileNode> node = SamplingHeapProfileNode::Create(ecmaVm, resultValue);
                if (resultValue.IsEmpty() || node == nullptr) {
                    error += "'children' format invalid;";
                }
                samplingHeapProfileNode->children_.emplace_back(std::move(node));
            }
        } else {
            error += "'children' should be an Array;";
        }
    } else {
        error += "should contain 'children';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileNode::Create " << error;
        return nullptr;
    }

    return samplingHeapProfileNode;
}

Local<ObjectRef> SamplingHeapProfileNode::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    if (callFrame_ != nullptr) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrame")),
            Local<JSValueRef>(callFrame_->ToObject(ecmaVm)));
    }

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "selfSize")),
        IntegerRef::New(ecmaVm, selfSize_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        IntegerRef::New(ecmaVm, id_));

    size_t len = children_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        if (children_[i] != nullptr) {
            Local<ObjectRef> node = children_[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, node);
        }
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "children")), values);

    return params;
}

std::unique_ptr<SamplingHeapProfile> SamplingHeapProfile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfile::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto samplingHeapProfile = std::make_unique<SamplingHeapProfile>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "head")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<SamplingHeapProfileNode> obj = SamplingHeapProfileNode::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'head' format error;";
            } else {
                samplingHeapProfile->head_ = std::move(obj);
            }
        } else {
            error += "'head' should be an Object;";
        }
    } else {
        error += "should contain 'head';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t len = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < len; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<SamplingHeapProfileSample> node = SamplingHeapProfileSample::Create(ecmaVm,
                                                                                                    resultValue);
                if (resultValue.IsEmpty() || node == nullptr) {
                    error += "'samples' format invalid;";
                }
                samplingHeapProfile->samples_.emplace_back(std::move(node));
            }
        } else {
            error += "'samples' should be an Array;";
        }
    } else {
        error += "should contain 'samples';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfile::Create " << error;
        return nullptr;
    }

    return samplingHeapProfile;
}

Local<ObjectRef> SamplingHeapProfile::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    if (head_ != nullptr) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "head")),
            Local<JSValueRef>(head_->ToObject(ecmaVm)));
    }

    size_t len = samples_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        if (samples_[i] != nullptr) {
            Local<ObjectRef> node = samples_[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, node);
        }
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")), values);

    return params;
}

std::unique_ptr<PositionTickInfo> PositionTickInfo::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "PositionTickInfo::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto positionTicks = std::make_unique<PositionTickInfo>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "line")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            positionTicks->line_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'line' should be a Number;";
        }
    } else {
        error += "should contain 'line';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ticks")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            positionTicks->ticks_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'ticks' should be a Number;";
        }
    } else {
        error += "should contain 'ticks';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PositionTickInfo::Create " << error;
        return nullptr;
    }
    return positionTicks;
}

Local<ObjectRef> PositionTickInfo::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "line")),
        IntegerRef::New(ecmaVm, line_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ticks")),
        IntegerRef::New(ecmaVm, ticks_));

    return params;
}

std::unique_ptr<ProfileNode> ProfileNode::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ProfileNode::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto profileNode = std::make_unique<ProfileNode>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profileNode->id_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'id' should be a Number;";
        }
    } else {
        error += "should contain 'id';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrame")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsObject()) {
            std::unique_ptr<RuntimeCallFrame> obj = RuntimeCallFrame::Create(ecmaVm, result);
            if (obj == nullptr) {
                error += "'callFrame' format error;";
            } else {
                profileNode->callFrame_ = std::move(obj);
            }
        } else {
            error += "'callFrame' should be an Object;";
        }
    } else {
        error += "should contain 'callFrame';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hitCount")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profileNode->hitCount_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'hitCount' should be a Number;";
        }
    } else {
        error += "should contain 'hitCount';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "children")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t childrenLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < childrenLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                std::unique_ptr<int32_t> pChildren;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                *pChildren = resultValue->Int32Value(ecmaVm);
                profileNode->children_->emplace_back(std::move(pChildren));
            }
        } else {
            error += "'children' should be an Array;";
        }
    } else {
        error += "should contain 'children';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "positionTicks")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t positionTickLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < positionTickLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<PositionTickInfo> positionTick = PositionTickInfo::Create(ecmaVm, resultValue);
                profileNode->positionTicks_->emplace_back(std::move(positionTick));
            }
        } else {
            error += "'positionTicks' should be an Array;";
        }
    } else {
        error += "should contain 'positionTicks';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "deoptReason")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            profileNode->deoptReason_ = DebuggerApi::ToCString(result);
        } else {
            error += "'deoptReason' should be a String;";
        }
    } else {
        error += "should contain 'deoptReason_';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ProfileNode::Create " << error;
        return nullptr;
    }
    return profileNode;
}

Local<ObjectRef> ProfileNode::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        IntegerRef::New(ecmaVm, id_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrame")),
        Local<JSValueRef>(callFrame_->ToObject(ecmaVm)));
    
    if (hitCount_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hitCount")),
            IntegerRef::New(ecmaVm, hitCount_.value()));
    }
    
    if (children_) {
        size_t childrenLen = children_->size();
        Local<ArrayRef> childrenValues = ArrayRef::New(ecmaVm, childrenLen);
        for (size_t i = 0; i < childrenLen; i++) {
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, *(children_.value()[i]));
            childrenValues->Set(ecmaVm, i, elem);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "children")), childrenValues);
    }
    
    if (positionTicks_) {
        size_t positionTickLen = positionTicks_->size();
        Local<ArrayRef> positionValues = ArrayRef::New(ecmaVm, positionTickLen);
        for (size_t i = 0; i < positionTickLen; i++) {
            Local<ObjectRef> positionTick = positionTicks_.value()[i]->ToObject(ecmaVm);
            positionValues->Set(ecmaVm, i, positionTick);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "positionTicks")), positionValues);
    }
    
    if (deoptReason_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "deoptReason")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, deoptReason_->c_str())));
    }
    
    return params;
}

std::unique_ptr<Profile> Profile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Profile::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto profile = std::make_unique<Profile>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startTime")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profile->startTime_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'startTime' should be a Number;";
        }
    } else {
        error += "should contain 'startTime';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endTime")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profile->endTime_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'endTime' should be a Number;";
        }
    } else {
        error += "should contain 'endTime';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodes")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t nodesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < nodesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<ProfileNode> node = ProfileNode::Create(ecmaVm, resultValue);
                profile->nodes_.emplace_back(std::move(node));
            }
        } else {
            error += "'nodes' should be an Array;";
        }
    } else {
        error += "should contain 'nodes';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t samplesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < samplesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                std::unique_ptr<int32_t> pSamples;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                *pSamples = resultValue->Int32Value(ecmaVm);
                profile->samples_->emplace_back(std::move(pSamples));
            }
        } else {
            error += "'samples' should be an Array;";
        }
    } else {
        error += "should contain 'samples';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timeDeltas")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t timeDeltasLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < timeDeltasLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                std::unique_ptr<int32_t> pTime;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                *pTime = resultValue->Int32Value(ecmaVm);
                profile->timeDeltas_->emplace_back(std::move(pTime));
            }
        } else {
            error += "'timeDeltas' should be an Array;";
        }
    } else {
        error += "should contain 'timeDeltas';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Profile::Create " << error;
        return nullptr;
    }
    return profile;
}

Local<ObjectRef> Profile::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startTime")),
        IntegerRef::New(ecmaVm, startTime_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endTime")),
        IntegerRef::New(ecmaVm, endTime_));
    size_t nodeLen = nodes_.size();
    Local<ArrayRef> nodeValues = ArrayRef::New(ecmaVm, nodeLen);
    for (size_t i = 0; i < nodeLen; i++) {
        Local<ObjectRef> profileNode = nodes_[i]->ToObject(ecmaVm);
        nodeValues->Set(ecmaVm, i, profileNode);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodes")), nodeValues);
    
    if (samples_) {
        size_t samplesLen = samples_->size();
        Local<ArrayRef> sampleValues = ArrayRef::New(ecmaVm, samplesLen);
        for (size_t i = 0; i < samplesLen; i++) {
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, *(samples_.value()[i]));
            sampleValues->Set(ecmaVm, i, elem);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")), sampleValues);
    }
    
    if (timeDeltas_) {
        size_t tdLen = timeDeltas_->size();
        Local<ArrayRef> timeValues = ArrayRef::New(ecmaVm, tdLen);
        for (size_t i = 0; i < tdLen; i++) {
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, *(timeDeltas_.value()[i]));
            timeValues->Set(ecmaVm, i, elem);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timeDeltas")), timeValues);
    }
    
    return params;
}

std::unique_ptr<Coverage> Coverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Coverage::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto coverage = std::make_unique<Coverage>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startOffset")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            coverage->startOffset_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'startOffset' should be a Number;";
        }
    } else {
        error += "should contain 'startOffset';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endOffset")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            coverage->endOffset_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'endOffset' should be a Number;";
        }
    } else {
        error += "should contain 'endOffset';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "count")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            coverage->count_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'count' should be a Number;";
        }
    } else {
        error += "should contain 'count';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Coverage::Create " << error;
        return nullptr;
    }
    return coverage;
}

Local<ObjectRef> Coverage::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startOffset")), IntegerRef::New(ecmaVm, startOffset_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endOffset")), IntegerRef::New(ecmaVm, endOffset_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "count")), IntegerRef::New(ecmaVm, count_));
    return params;
}

std::unique_ptr<FunctionCoverage> FunctionCoverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "FunctionCoverage::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto functionCoverage = std::make_unique<FunctionCoverage>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            functionCoverage->functionName_ = DebuggerApi::ToCString(result);
        } else {
            error += "'functionName' should be a String;";
        }
    } else {
        error += "should contain 'functionName';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ranges")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t rangesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < rangesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<Coverage> range = Coverage::Create(ecmaVm, resultValue);
                functionCoverage->ranges_.emplace_back(std::move(range));
            }
        } else {
            error += "'ranges' should be an Array;";
        }
    } else {
        error += "should contain 'ranges';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm,
                                                                                            "isBlockCoverage")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsBoolean()) {
            functionCoverage->isBlockCoverage_ = result->IsTrue();
        } else {
            error += "'isBlockCoverage' should be a Boolean;";
        }
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "FunctionCoverage::Create " << error;
        return nullptr;
    }
    return functionCoverage;
}

Local<ObjectRef> FunctionCoverage::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, functionName_.c_str())));
    
    size_t rangesLen = ranges_.size();
    Local<ArrayRef> rangesValues = ArrayRef::New(ecmaVm, rangesLen);
    for (size_t i = 0; i < rangesLen; i++) {
        Local<ObjectRef> coverage = ranges_[i]->ToObject(ecmaVm);
        rangesValues->Set(ecmaVm, i, coverage);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "ranges")), rangesValues);
    if (isBlockCoverage_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isBlockCoverage")),
            BooleanRef::New(ecmaVm, isBlockCoverage_));
    }
    return params;
}

std::unique_ptr<ScriptCoverage> ScriptCoverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptCoverage::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scriptCoverage = std::make_unique<ScriptCoverage>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptCoverage->scriptId_ = DebuggerApi::ToCString(result);
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptCoverage->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should be a String;";
        }
    } else {
        error += "should contain 'url';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functions")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t functionsLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < functionsLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<FunctionCoverage> function = FunctionCoverage::Create(ecmaVm, resultValue);
                scriptCoverage->functions_.emplace_back(std::move(function));
            }
        } else {
            error += "'functions' should be an Array;";
        }
    } else {
        error += "should contain 'functions';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptCoverage::Create " << error;
        return nullptr;
    }
    return scriptCoverage;
}

Local<ObjectRef> ScriptCoverage::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptId_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    size_t functionsLen = functions_.size();
    Local<ArrayRef> rangesValues = ArrayRef::New(ecmaVm, functionsLen);
    for (size_t i = 0; i < functionsLen; i++) {
        Local<ObjectRef> functionCoverage = functions_[i]->ToObject(ecmaVm);
        rangesValues->Set(ecmaVm, i, functionCoverage);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functions")), rangesValues);
    return params;
}

std::unique_ptr<TypeObject> TypeObject::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "TypeObject::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto typeObject = std::make_unique<TypeObject>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            typeObject->name_ = DebuggerApi::ToCString(result);
        } else {
            error += "'name' should be a String;";
        }
    } else {
        error += "should contain 'name';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "TypeObject::Create " << error;
        return nullptr;
    }
    return typeObject;
}

Local<ObjectRef> TypeObject::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_.c_str())));
    return params;
}

std::unique_ptr<TypeProfileEntry> TypeProfileEntry::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "TypeProfileEntry::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto typeProfileEntry = std::make_unique<TypeProfileEntry>();
    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "offset")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            typeProfileEntry->offset_ = static_cast<size_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'offset' should be a Number;";
        }
    } else {
        error += "should contain 'offset';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "types")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t typesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < typesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<TypeObject> type = TypeObject::Create(ecmaVm, resultValue);
                typeProfileEntry->types_.emplace_back(std::move(type));
            }
        } else {
            error += "'types' should be an Array;";
        }
    } else {
        error += "should contain 'types';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "TypeProfileEntry::Create " << error;
        return nullptr;
    }
    return typeProfileEntry;
}

Local<ObjectRef> TypeProfileEntry::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "offset")),
        IntegerRef::New(ecmaVm, offset_));
    size_t typeLen = types_.size();
    Local<ArrayRef> typeValues = ArrayRef::New(ecmaVm, typeLen);
    for (size_t i = 0; i < typeLen; i++) {
        Local<ObjectRef> typeObject = types_[i]->ToObject(ecmaVm);
        typeValues->Set(ecmaVm, i, typeObject);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "types")), typeValues);
    return params;
}

std::unique_ptr<ScriptTypeProfile> ScriptTypeProfile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptTypeProfile::Create params is nullptr";
        return nullptr;
    }
    CString error;
    auto scriptTypeProfile = std::make_unique<ScriptTypeProfile>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptTypeProfile->scriptId_ = DebuggerApi::ToCString(result);
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptTypeProfile->url_ = DebuggerApi::ToCString(result);
        } else {
            error += "'url' should be a String;";
        }
    } else {
        error += "should contain 'url';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "entries")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t entriesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < entriesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                Local<JSValueRef> entriesValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                std::unique_ptr<TypeProfileEntry> entries = TypeProfileEntry::Create(ecmaVm, entriesValue);
                scriptTypeProfile->entries_.emplace_back(std::move(entries));
            }
        } else {
            error += "'entries' should be an Array;";
        }
    } else {
        error += "should contain 'entries';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptTypeProfile::Create " << error;
        return scriptTypeProfile;
    }
    return scriptTypeProfile;
}

Local<ObjectRef> ScriptTypeProfile::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptId_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    size_t entriesLen = entries_.size();
    Local<ArrayRef> entriesValues = ArrayRef::New(ecmaVm, entriesLen);
    for (size_t i = 0; i < entriesLen; i++) {
        Local<ObjectRef> typeProfileEntryObject = entries_[i]->ToObject(ecmaVm);
        entriesValues->Set(ecmaVm, i, typeProfileEntryObject);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "entries")), entriesValues);
    return params;
}
}  // namespace panda::ecmascript::tooling
