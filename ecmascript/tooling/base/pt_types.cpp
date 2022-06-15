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

#include "ecmascript/dfx/cpu_profiler/samples_record.h"

namespace panda::ecmascript::tooling {
using ObjectType = RemoteObject::TypeName;
using ObjectSubType = RemoteObject::SubTypeName;
using ObjectClassName = RemoteObject::ClassName;

const std::string ObjectType::Object = "object";        // NOLINT (readability-identifier-naming)
const std::string ObjectType::Function = "function";    // NOLINT (readability-identifier-naming)
const std::string ObjectType::Undefined = "undefined";  // NOLINT (readability-identifier-naming)
const std::string ObjectType::String = "string";        // NOLINT (readability-identifier-naming)
const std::string ObjectType::Number = "number";        // NOLINT (readability-identifier-naming)
const std::string ObjectType::Boolean = "boolean";      // NOLINT (readability-identifier-naming)
const std::string ObjectType::Symbol = "symbol";        // NOLINT (readability-identifier-naming)
const std::string ObjectType::Bigint = "bigint";        // NOLINT (readability-identifier-naming)
const std::string ObjectType::Wasm = "wasm";            // NOLINT (readability-identifier-naming)

const std::string ObjectSubType::Array = "array";              // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Null = "null";                // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Node = "node";                // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Regexp = "regexp";            // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Date = "date";                // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Map = "map";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Set = "set";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Weakmap = "weakmap";          // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Weakset = "weakset";          // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Iterator = "iterator";        // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Generator = "generator";      // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Error = "error";              // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Proxy = "proxy";              // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Promise = "promise";          // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Typedarray = "typedarray";    // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Arraybuffer = "arraybuffer";  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Dataview = "dataview";        // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::I32 = "i32";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::I64 = "i64";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::F32 = "f32";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::F64 = "f64";                  // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::V128 = "v128";                // NOLINT (readability-identifier-naming)
const std::string ObjectSubType::Externref = "externref";      // NOLINT (readability-identifier-naming)

const std::string ObjectClassName::Object = "Object";                  // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Function = "Function";              // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Array = "Array";                    // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Regexp = "RegExp";                  // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Date = "Date";                      // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Map = "Map";                        // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Set = "Set";                        // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Weakmap = "Weakmap";                // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Weakset = "Weakset";                // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::ArrayIterator = "ArrayIterator";    // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::StringIterator = "StringIterator";  // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::SetIterator = "SetIterator";        // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::MapIterator = "MapIterator";        // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Iterator = "Iterator";              // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Error = "Error";                    // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Proxy = "Object";                   // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Promise = "Promise";                // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Typedarray = "Typedarray";          // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Arraybuffer = "Arraybuffer";        // NOLINT (readability-identifier-naming)
const std::string ObjectClassName::Global = "global";                  // NOLINT (readability-identifier-naming)

const std::string RemoteObject::ObjectDescription = "Object";    // NOLINT (readability-identifier-naming)
const std::string RemoteObject::GlobalDescription = "global";    // NOLINT (readability-identifier-naming)
const std::string RemoteObject::ProxyDescription = "Proxy";      // NOLINT (readability-identifier-naming)
const std::string RemoteObject::PromiseDescription = "Promise";  // NOLINT (readability-identifier-naming)
const std::string RemoteObject::ArrayIteratorDescription =       // NOLINT (readability-identifier-naming)
    "ArrayIterator";
const std::string RemoteObject::StringIteratorDescription =  // NOLINT (readability-identifier-naming)
    "StringIterator";
const std::string RemoteObject::SetIteratorDescription = "SetIterator";  // NOLINT (readability-identifier-naming)
const std::string RemoteObject::MapIteratorDescription = "MapIterator";  // NOLINT (readability-identifier-naming)
const std::string RemoteObject::WeakMapDescription = "WeakMap";          // NOLINT (readability-identifier-naming)
const std::string RemoteObject::WeakSetDescription = "WeakSet";          // NOLINT (readability-identifier-naming)

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
            .SetDescription(DebuggerApi::ToStdString(tagged->ToString(ecmaVm)));
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

std::string ObjectRemoteObject::DescriptionForObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
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

std::string ObjectRemoteObject::DescriptionForArray(const EcmaVM *ecmaVm, const Local<ArrayRef> &tagged)
{
    std::string description = "Array(" + std::to_string(tagged->Length(ecmaVm)) + ")";
    return description;
}

std::string ObjectRemoteObject::DescriptionForRegexp(const EcmaVM *ecmaVm, const Local<RegExpRef> &tagged)
{
    std::string regexpSource = DebuggerApi::ToStdString(tagged->GetOriginalSource(ecmaVm));
    std::string description = "/" + regexpSource + "/";
    return description;
}

std::string ObjectRemoteObject::DescriptionForDate(const EcmaVM *ecmaVm, const Local<DateRef> &tagged)
{
    std::string description = DebuggerApi::ToStdString(tagged->ToString(ecmaVm));
    return description;
}

std::string ObjectRemoteObject::DescriptionForMap(const Local<MapRef> &tagged)
{
    std::string description = ("Map(" + std::to_string(tagged->GetSize()) + ")");
    return description;
}

std::string ObjectRemoteObject::DescriptionForSet(const Local<SetRef> &tagged)
{
    std::string description = ("Set(" + std::to_string(tagged->GetSize()) + ")");
    return description;
}

std::string ObjectRemoteObject::DescriptionForError(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
{
    // add message
    Local<JSValueRef> stack = StringRef::NewFromUtf8(ecmaVm, "message");
    Local<JSValueRef> result = Local<ObjectRef>(tagged)->Get(ecmaVm, stack);
    return DebuggerApi::ToStdString(result->ToString(ecmaVm));
}

std::string ObjectRemoteObject::DescriptionForArrayBuffer(const EcmaVM *ecmaVm, const Local<ArrayBufferRef> &tagged)
{
    int32_t len = tagged->ByteLength(ecmaVm);
    std::string description = ("ArrayBuffer(" + std::to_string(len) + ")");
    return description;
}

std::string SymbolRemoteObject::DescriptionForSymbol(const EcmaVM *ecmaVm, const Local<SymbolRef> &tagged) const
{
    std::string description =
        "Symbol(" + DebuggerApi::ToStdString(tagged->GetDescription(ecmaVm)) + ")";
    return description;
}

std::string FunctionRemoteObject::DescriptionForFunction(const EcmaVM *ecmaVm, const Local<FunctionRef> &tagged) const
{
    std::string sourceCode;
    if (tagged->IsNative(ecmaVm)) {
        sourceCode = "[native code]";
    } else {
        sourceCode = "[js code]";
    }
    Local<StringRef> name = tagged->GetName(ecmaVm);
    std::string description = "function " + DebuggerApi::ToStdString(name) + "( { " + sourceCode + " }";
    return description;
}

std::unique_ptr<RemoteObject> RemoteObject::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto remoteObject = std::make_unique<RemoteObject>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToStdString(result);
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
            auto type = DebuggerApi::ToStdString(result);
            if (ObjectSubType::Valid(type)) {
                remoteObject->subType_ = type;
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
            remoteObject->className_ = DebuggerApi::ToStdString(result);
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
            remoteObject->unserializableValue_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'unserializableValue' should be a String;";
        }
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "description")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            remoteObject->description_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<RemoteObject> RemoteObject::Create(const PtJson &params)
{
    std::string error;
    auto remoteObject = std::make_unique<RemoteObject>();
    Result ret;

    std::string type;
    ret = params.GetString("type", &type);
    if (ret == Result::SUCCESS) {
        if (ObjectType::Valid(type)) {
            remoteObject->type_ = std::move(type);
        } else {
            error += "'type' is invalid;";
        }
    } else {
        error += "Unknown 'type';";
    }

    std::string subType;
    ret = params.GetString("subtype", &subType);
    if (ret == Result::SUCCESS) {
        if (ObjectSubType::Valid(subType)) {
            remoteObject->subType_ = std::move(subType);
        } else {
            error += "'subtype' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'subtype';";
    }

    std::string className;
    ret = params.GetString("className", &className);
    if (ret == Result::SUCCESS) {
        remoteObject->className_ = std::move(className);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'className';";
    }

    std::string unserializableValue;
    ret = params.GetString("unserializableValue", &unserializableValue);
    if (ret == Result::SUCCESS) {
        remoteObject->unserializableValue_ = std::move(unserializableValue);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'unserializableValue';";
    }

    std::string description;
    ret = params.GetString("description", &description);
    if (ret == Result::SUCCESS) {
        remoteObject->description_ = std::move(description);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'description';";
    }

    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        remoteObject->objectId_ = std::stoi(objectId);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'objectId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "RemoteObject::Create " << error;
        return nullptr;
    }

    return remoteObject;
}

Local<ObjectRef> RemoteObject::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, type_.c_str())));
    if (subType_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "subtype")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, subType_->c_str())));
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

std::unique_ptr<PtJson> RemoteObject::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("type", type_.c_str());
    if (subType_) {
        result->Add("subtype", subType_->c_str());
    }
    if (className_) {
        result->Add("className", className_->c_str());
    }
    if (unserializableValue_) {
        result->Add("unserializableValue", unserializableValue_->c_str());
    }
    if (description_) {
        result->Add("description", description_->c_str());
    }
    if (objectId_) {
        result->Add("objectId", std::to_string(objectId_.value()).c_str());
    }

    return result;
}

std::unique_ptr<ExceptionDetails> ExceptionDetails::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ExceptionDetails::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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
            exceptionDetails->text_ = DebuggerApi::ToStdString(result);
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
            exceptionDetails->url_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<ExceptionDetails> ExceptionDetails::Create(const PtJson &params)
{
    std::string error;
    auto exceptionDetails = std::make_unique<ExceptionDetails>();
    Result ret;

    int32_t exceptionId;
    ret = params.GetInt("exceptionId", &exceptionId);
    if (ret == Result::SUCCESS) {
        exceptionDetails->exceptionId_ = exceptionId;
    } else {
        error += "Unknown 'exceptionId';";
    }

    std::string text;
    ret = params.GetString("text", &text);
    if (ret == Result::SUCCESS) {
        exceptionDetails->text_ = std::move(text);
    } else {
        error += "Unknown 'text';";
    }

    int32_t line;
    ret = params.GetInt("lineNumber", &line);
    if (ret == Result::SUCCESS) {
        exceptionDetails->line_ = line;
    } else {
        error += "Unknown 'lineNumber';";
    }

    int32_t column;
    ret = params.GetInt("columnNumber", &column);
    if (ret == Result::SUCCESS) {
        exceptionDetails->column_ = column;
    } else {
        error += "Unknown 'columnNumber';";
    }

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        exceptionDetails->scriptId_ = std::stoi(scriptId);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'scriptId';";
    }

    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        exceptionDetails->url_ = std::move(url);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'url';";
    }

    std::unique_ptr<PtJson> exception;
    ret = params.GetObject("exception", &exception);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<RemoteObject> obj = RemoteObject::Create(*exception);
        if (obj != nullptr) {
            exceptionDetails->exception_ = std::move(obj);
        } else {
            error += "'exception' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'exception';";
    }

    int32_t executionContextId;
    ret = params.GetInt("executionContextId", &executionContextId);
    if (ret == Result::SUCCESS) {
        exceptionDetails->executionContextId_ = executionContextId;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'executionContextId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ExceptionDetails::Create " << error;
        return nullptr;
    }

    return exceptionDetails;
}

Local<ObjectRef> ExceptionDetails::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> ExceptionDetails::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("exceptionId", exceptionId_);
    result->Add("text", text_.c_str());
    result->Add("lineNumber", line_);
    result->Add("columnNumber", column_);

    if (scriptId_) {
        result->Add("scriptId", std::to_string(scriptId_.value()).c_str());
    }
    if (url_) {
        result->Add("url", url_->c_str());
    }
    if (exception_ && exception_.value() != nullptr) {
        result->Add("exception", exception_.value()->ToJson());
    }
    if (executionContextId_) {
        result->Add("executionContextId", executionContextId_.value());
    }

    return result;
}

std::unique_ptr<InternalPropertyDescriptor> InternalPropertyDescriptor::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "InternalPropertyDescriptor::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto internalPropertyDescriptor = std::make_unique<InternalPropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            internalPropertyDescriptor->name_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<InternalPropertyDescriptor> InternalPropertyDescriptor::Create(const PtJson &params)
{
    std::string error;
    auto internalPropertyDescriptor = std::make_unique<InternalPropertyDescriptor>();
    Result ret;

    std::string name;
    ret = params.GetString("name", &name);
    if (ret == Result::SUCCESS) {
        internalPropertyDescriptor->name_ = std::move(name);
    } else {
        error += "Unknown 'name';";
    }

    std::unique_ptr<PtJson> value;
    ret = params.GetObject("value", &value);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<RemoteObject> obj = RemoteObject::Create(*value);
        if (obj != nullptr) {
            internalPropertyDescriptor->value_ = std::move(obj);
        } else {
            error += "'value' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'value';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "InternalPropertyDescriptor::Create " << error;
        return nullptr;
    }

    return internalPropertyDescriptor;
}

Local<ObjectRef> InternalPropertyDescriptor::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> InternalPropertyDescriptor::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("name", name_.c_str());
    if (value_ && value_.value() != nullptr) {
        result->Add("value", value_.value()->ToJson());
    }

    return result;
}

std::unique_ptr<PrivatePropertyDescriptor> PrivatePropertyDescriptor::Create(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "PrivatePropertyDescriptor::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto propertyDescriptor = std::make_unique<PrivatePropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            propertyDescriptor->name_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<PrivatePropertyDescriptor> PrivatePropertyDescriptor::Create(const PtJson &params)
{
    std::string error;
    auto privatePropertyDescriptor = std::make_unique<PrivatePropertyDescriptor>();
    Result ret;

    std::string name;
    ret = params.GetString("name", &name);
    if (ret == Result::SUCCESS) {
        privatePropertyDescriptor->name_ = std::move(name);
    } else {
        error += "Unknown 'name';";
    }

    std::unique_ptr<PtJson> value;
    ret = params.GetObject("value", &value);
    std::unique_ptr<RemoteObject> obj;
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*value);
        if (obj != nullptr) {
            privatePropertyDescriptor->value_ = std::move(obj);
        } else {
            error += "'value' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'value';";
    }

    std::unique_ptr<PtJson> get;
    ret = params.GetObject("get", &get);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*get);
        if (obj != nullptr) {
            privatePropertyDescriptor->get_ = std::move(obj);
        } else {
            error += "'get' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'get';";
    }

    std::unique_ptr<PtJson> set;
    ret = params.GetObject("set", &set);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*set);
        if (obj != nullptr) {
            privatePropertyDescriptor->set_ = std::move(obj);
        } else {
            error += "'set' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'set';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PrivatePropertyDescriptor::Create " << error;
        return nullptr;
    }

    return privatePropertyDescriptor;
}

Local<ObjectRef> PrivatePropertyDescriptor::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> PrivatePropertyDescriptor::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("name", name_.c_str());
    if (value_ && value_.value() != nullptr) {
        result->Add("value", value_.value()->ToJson());
    }
    if (get_ && get_.value() != nullptr) {
        result->Add("get", get_.value()->ToJson());
    }
    if (set_ && set_.value() != nullptr) {
        result->Add("set", set_.value()->ToJson());
    }

    return result;
}

std::unique_ptr<PropertyDescriptor> PropertyDescriptor::FromProperty(const EcmaVM *ecmaVm,
    const Local<JSValueRef> &name, const PropertyAttribute &property)
{
    std::unique_ptr<PropertyDescriptor> debuggerProperty = std::make_unique<PropertyDescriptor>();

    std::string nameStr;
    if (name->IsSymbol()) {
        Local<SymbolRef> symbol(name);
        nameStr =
            "Symbol(" + DebuggerApi::ToStdString(Local<SymbolRef>(name)->GetDescription(ecmaVm)) + ")";
        debuggerProperty->symbol_ = RemoteObject::FromTagged(ecmaVm, name);
    } else {
        nameStr = DebuggerApi::ToStdString(name->ToString(ecmaVm));
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
    std::string error;
    auto propertyDescriptor = std::make_unique<PropertyDescriptor>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            propertyDescriptor->name_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<PropertyDescriptor> PropertyDescriptor::Create(const PtJson &params)
{
    std::string error;
    auto propertyDescriptor = std::make_unique<PropertyDescriptor>();
    Result ret;

    std::string name;
    ret = params.GetString("name", &name);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->name_ = std::move(name);
    } else {
        error += "Unknown 'name';";
    }

    std::unique_ptr<PtJson> value;
    std::unique_ptr<RemoteObject> obj;
    ret = params.GetObject("value", &value);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*value);
        if (obj != nullptr) {
            propertyDescriptor->value_ = std::move(obj);
        } else {
            error += "'value' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'value';";
    }

    bool writable;
    ret = params.GetBool("writable", &writable);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->writable_ = writable;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'writable';";
    }

    std::unique_ptr<PtJson> get;
    ret = params.GetObject("get", &get);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*get);
        if (obj != nullptr) {
            propertyDescriptor->get_ = std::move(obj);
        } else {
            error += "'get' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'get';";
    }

    std::unique_ptr<PtJson> set;
    ret = params.GetObject("set", &set);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*set);
        if (obj != nullptr) {
            propertyDescriptor->set_ = std::move(obj);
        } else {
            error += "'set' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'set';";
    }

    bool configurable;
    ret = params.GetBool("configurable", &configurable);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->configurable_ = configurable;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'configurable';";
    }

    bool enumerable;
    ret = params.GetBool("enumerable", &enumerable);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->enumerable_ = enumerable;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'enumerable';";
    }

    bool wasThrown;
    ret = params.GetBool("wasThrown", &wasThrown);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->wasThrown_ = wasThrown;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'wasThrown';";
    }

    bool isOwn;
    ret = params.GetBool("isOwn", &isOwn);
    if (ret == Result::SUCCESS) {
        propertyDescriptor->isOwn_ = isOwn;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'isOwn';";
    }

    std::unique_ptr<PtJson> symbol;
    ret = params.GetObject("symbol", &symbol);
    if (ret == Result::SUCCESS) {
        obj = RemoteObject::Create(*symbol);
        if (obj != nullptr) {
            propertyDescriptor->symbol_ = std::move(obj);
        } else {
            error += "'symbol' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'symbol';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PropertyDescriptor::Create " << error;
        return nullptr;
    }

    return propertyDescriptor;
}

Local<ObjectRef> PropertyDescriptor::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> PropertyDescriptor::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("name", name_.c_str());
    if (value_ && value_.value() != nullptr) {
        result->Add("value", value_.value()->ToJson());
    }
    if (writable_) {
        result->Add("writable", writable_.value());
    }
    if (get_ && get_.value() != nullptr) {
        result->Add("get", get_.value()->ToJson());
    }
    if (set_ && set_.value() != nullptr) {
        result->Add("set", set_.value()->ToJson());
    }
    result->Add("configurable", configurable_);
    result->Add("enumerable", enumerable_);
    if (wasThrown_) {
        result->Add("wasThrown", wasThrown_.value());
    }
    if (isOwn_) {
        result->Add("isOwn", isOwn_.value());
    }
    if (symbol_ && symbol_.value() != nullptr) {
        result->Add("symbol", symbol_.value()->ToJson());
    }

    return result;
}

std::unique_ptr<CallArgument> CallArgument::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "CallArgument::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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
            callArgument->unserializableValue_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<CallArgument> CallArgument::Create(const PtJson &params)
{
    std::string error;
    auto callArgument = std::make_unique<CallArgument>();
    Result ret;

    std::string unserializableValue;
    ret = params.GetString("unserializableValue", &unserializableValue);
    if (ret == Result::SUCCESS) {
        callArgument->unserializableValue_ = std::move(unserializableValue);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'unserializableValue';";
    }

    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        callArgument->objectId_ = std::stoi(objectId);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'objectId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "CallArgument::Create " << error;
        return nullptr;
    }

    return callArgument;
}

Local<ObjectRef> CallArgument::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> CallArgument::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (unserializableValue_) {
        result->Add("unserializableValue", unserializableValue_->c_str());
    }
    if (objectId_) {
        result->Add("objectId", std::to_string(objectId_.value()).c_str());
    }

    return result;
}

std::unique_ptr<Location> Location::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Location::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto location = std::make_unique<Location>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            location->scriptId_ = DebuggerApi::StringToInt(result);
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

std::unique_ptr<Location> Location::Create(const PtJson &params)
{
    auto location = std::make_unique<Location>();
    std::string error;
    Result ret;

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        location->scriptId_ = std::stoi(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }
    int32_t line;
    ret = params.GetInt("lineNumber", &line);
    if (ret == Result::SUCCESS) {
        location->line_ = line;
    } else {
        error += "Unknown 'lineNumber';";
    }
    int32_t column;
    ret = params.GetInt("columnNumber", &column);
    if (ret == Result::SUCCESS) {
        location->column_ = column;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'columnNumber';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return location;
}

Local<ObjectRef> Location::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> Location::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("lineNumber", line_);
    if (column_) {
        result->Add("columnNumber", column_.value());
    }

    return result;
}

std::unique_ptr<ScriptPosition> ScriptPosition::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptPosition::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<ScriptPosition> ScriptPosition::Create(const PtJson &params)
{
    std::string error;
    auto scriptPosition = std::make_unique<ScriptPosition>();
    Result ret;

    int32_t line;
    ret = params.GetInt("lineNumber", &line);
    if (ret == Result::SUCCESS) {
        scriptPosition->line_ = line;
    } else {
        error += "Unknown 'lineNumber';";
    }

    int32_t column;
    ret = params.GetInt("columnNumber", &column);
    if (ret == Result::SUCCESS) {
        scriptPosition->column_ = column;
    } else {
        error += "Unknown 'columnNumber';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptPosition::Create " << error;
        return nullptr;
    }

    return scriptPosition;
}

Local<ObjectRef> ScriptPosition::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, line_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "columnNumber")),
        IntegerRef::New(ecmaVm, column_));

    return params;
}

std::unique_ptr<PtJson> ScriptPosition::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("lineNumber", line_);
    result->Add("columnNumber", column_);

    return result;
}

std::unique_ptr<SearchMatch> SearchMatch::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SearchMatch::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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
            locationSearch->lineContent_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<SearchMatch> SearchMatch::Create(const PtJson &params)
{
    std::string error;
    auto locationSearch = std::make_unique<SearchMatch>();
    Result ret;

    int32_t lineNumber;
    ret = params.GetInt("lineNumber", &lineNumber);
    if (ret == Result::SUCCESS) {
        locationSearch->lineNumber_ = lineNumber;
    } else {
        error += "Unknown 'lineNumber';";
    }

    std::string lineContent;
    ret = params.GetString("lineContent", &lineContent);
    if (ret == Result::SUCCESS) {
        locationSearch->lineContent_ = std::move(lineContent);
    } else {
        error += "Unknown 'lineContent';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SearchMatch::Create " << error;
        return nullptr;
    }

    return locationSearch;
}

Local<ObjectRef> SearchMatch::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineNumber")),
        IntegerRef::New(ecmaVm, lineNumber_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lineContent")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, lineContent_.c_str())));
    return params;
}

std::unique_ptr<PtJson> SearchMatch::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("lineNumber", lineNumber_);
    result->Add("lineContent", lineContent_.c_str());

    return result;
}

std::unique_ptr<LocationRange> LocationRange::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "BreakLocation::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto locationRange = std::make_unique<LocationRange>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            locationRange->scriptId_ = DebuggerApi::StringToInt(result);
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

std::unique_ptr<LocationRange> LocationRange::Create(const PtJson &params)
{
    std::string error;
    auto locationRange = std::make_unique<LocationRange>();
    Result ret;

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        locationRange->scriptId_ = std::stoi(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    std::unique_ptr<PtJson> start;
    std::unique_ptr<ScriptPosition> obj;
    ret = params.GetObject("start", &start);
    if (ret == Result::SUCCESS) {
        obj = ScriptPosition::Create(*start);
        if (obj != nullptr) {
            locationRange->start_ = std::move(obj);
        } else {
            error += "'start' format error;";
        }
    } else {
        error += "Unknown 'start';";
    }

    std::unique_ptr<PtJson> end;
    ret = params.GetObject("end", &end);
    if (ret == Result::SUCCESS) {
        obj = ScriptPosition::Create(*end);
        if (obj != nullptr) {
            locationRange->end_ = std::move(obj);
        } else {
            error += "'end' format error;";
        }
    } else {
        error += "Unknown 'end';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "LocationRange::Create " << error;
        return nullptr;
    }

    return locationRange;
}

Local<ObjectRef> LocationRange::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> LocationRange::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    if (start_ != nullptr) {
        result->Add("start", start_->ToJson());
    }
    if (end_ != nullptr) {
        result->Add("end", end_->ToJson());
    }

    return result;
}

std::unique_ptr<BreakLocation> BreakLocation::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "BreakLocation::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto breakLocation = std::make_unique<BreakLocation>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            breakLocation->scriptId_ = DebuggerApi::StringToInt(result);
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
            auto type = DebuggerApi::ToStdString(result);
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

std::unique_ptr<BreakLocation> BreakLocation::Create(const PtJson &params)
{
    std::string error;
    auto breakLocation = std::make_unique<BreakLocation>();
    Result ret;

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        breakLocation->scriptId_ = std::stoi(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    int32_t lineNumber;
    ret = params.GetInt("lineNumber", &lineNumber);
    if (ret == Result::SUCCESS) {
        breakLocation->line_ = lineNumber;
    } else {
        error += "Unknown 'lineNumber';";
    }

    int32_t columnNumber;
    ret = params.GetInt("columnNumber", &columnNumber);
    if (ret == Result::SUCCESS) {
        breakLocation->column_ = columnNumber;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'columnNumber';";
    }

    std::string type;
    ret = params.GetString("type", &type);
    if (ret == Result::SUCCESS) {
        if (BreakType::Valid(type)) {
            breakLocation->type_ = std::move(type);
        } else {
            error += "'type' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "type 'scriptId';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return breakLocation;
}

Local<ObjectRef> BreakLocation::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> BreakLocation::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("lineNumber", line_);
    if (column_) {
        result->Add("columnNumber", column_.value());
    }
    if (type_) {
        result->Add("type", type_->c_str());
    }

    return result;
}

std::unique_ptr<Scope> Scope::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Scope::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto scope = std::make_unique<Scope>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "type")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            auto type = DebuggerApi::ToStdString(result);
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
            scope->name_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<Scope> Scope::Create(const PtJson &params)
{
    std::string error;
    auto scope = std::make_unique<Scope>();
    Result ret;

    std::string type;
    ret = params.GetString("type", &type);
    if (ret == Result::SUCCESS) {
        if (Scope::Type::Valid(type)) {
            scope->type_ = std::move(type);
        } else {
            error += "'type' is invalid;";
        }
    } else {
        error += "Unknown 'type';";
    }

    std::unique_ptr<PtJson> object;
    std::unique_ptr<RemoteObject> remoteObject;
    ret = params.GetObject("object", &object);
    if (ret == Result::SUCCESS) {
        remoteObject = RemoteObject::Create(*object);
        if (remoteObject != nullptr) {
            scope->object_ = std::move(remoteObject);
        } else {
            error += "'object' format error;";
        }
    } else {
        error += "Unknown 'object';";
    }

    std::string name;
    ret = params.GetString("name", &name);
    if (ret == Result::SUCCESS) {
        scope->name_ = std::move(name);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'name';";
    }

    std::unique_ptr<PtJson> startLocation;
    std::unique_ptr<Location> obj;
    ret = params.GetObject("startLocation", &startLocation);
    if (ret == Result::SUCCESS) {
        obj = Location::Create(*startLocation);
        if (obj != nullptr) {
            scope->startLocation_ = std::move(obj);
        } else {
            error += "'startLocation' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'startLocation';";
    }

    std::unique_ptr<PtJson> endLocation;
    ret = params.GetObject("endLocation", &endLocation);
    if (ret == Result::SUCCESS) {
        obj = Location::Create(*endLocation);
        if (obj != nullptr) {
            scope->endLocation_ = std::move(obj);
        } else {
            error += "'endLocation' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'endLocation';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Location::Create " << error;
        return nullptr;
    }

    return scope;
}

Local<ObjectRef> Scope::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> Scope::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("type", type_.c_str());
    if (object_ != nullptr) {
        result->Add("object", object_->ToJson());
    }
    if (name_) {
        result->Add("name", name_->c_str());
    }
    if (startLocation_ && startLocation_.value() != nullptr) {
        result->Add("startLocation", startLocation_.value()->ToJson());
    }
    if (endLocation_ && endLocation_.value() != nullptr) {
        result->Add("endLocation", endLocation_.value()->ToJson());
    }

    return result;
}

std::unique_ptr<CallFrame> CallFrame::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "CallFrame::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto callFrame = std::make_unique<CallFrame>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrameId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callFrame->callFrameId_ = DebuggerApi::StringToInt(result);
        } else {
            error += "'callFrameId' should be a String;";
        }
    } else {
        error += "should contain 'callFrameId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            callFrame->functionName_ = DebuggerApi::ToStdString(result);
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
            callFrame->url_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<CallFrame> CallFrame::Create(const PtJson &params)
{
    std::string error;
    auto callFrame = std::make_unique<CallFrame>();
    Result ret;

    std::string callFrameId;
    ret = params.GetString("callFrameId", &callFrameId);
    if (ret == Result::SUCCESS) {
        callFrame->callFrameId_ = std::stoi(callFrameId);
    } else {
        error += "Unknown 'callFrameId';";
    }

    std::string functionName;
    ret = params.GetString("functionName", &functionName);
    if (ret == Result::SUCCESS) {
        callFrame->functionName_ = std::move(functionName);
    } else {
        error += "Unknown 'functionName';";
    }

    std::unique_ptr<PtJson> functionLocation;
    std::unique_ptr<Location> obj;
    ret = params.GetObject("functionLocation", &functionLocation);
    if (ret == Result::SUCCESS) {
        obj = Location::Create(*functionLocation);
        if (obj != nullptr) {
            callFrame->functionLocation_ = std::move(obj);
        } else {
            error += "'functionLocation' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'functionLocation';";
    }

    std::unique_ptr<PtJson> location;
    ret = params.GetObject("location", &location);
    if (ret == Result::SUCCESS) {
        obj = Location::Create(*location);
        if (obj != nullptr) {
            callFrame->location_ = std::move(obj);
        } else {
            error += "'location' format error;";
        }
    } else {
        error += "Unknown 'location';";
    }

    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        callFrame->url_ = std::move(url);
    } else {
        error += "Unknown 'url';";
    }

    std::unique_ptr<PtJson> scopeChain;
    ret = params.GetArray("scopeChain", &scopeChain);
    if (ret == Result::SUCCESS) {
        int32_t len = scopeChain->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = scopeChain->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<Scope> scope = Scope::Create(*arrayEle);
                if (scope != nullptr) {
                    callFrame->scopeChain_.emplace_back(std::move(scope));
                } else {
                    error += "'scopeChain' format error;";
                }
            } else {
                error += "Unknown 'scopeChain';";
            }
        }
    } else {
        error += "Unknown 'scopeChain';";
    }

    std::unique_ptr<PtJson> thisObj;
    std::unique_ptr<RemoteObject> remoteObject;
    ret = params.GetObject("this", &thisObj);
    if (ret == Result::SUCCESS) {
        remoteObject = RemoteObject::Create(*thisObj);
        if (remoteObject != nullptr) {
            callFrame->this_ = std::move(remoteObject);
        } else {
            error += "'this' format error;";
        }
    } else {
        error += "Unknown 'this';";
    }

    std::unique_ptr<PtJson> returnValue;
    ret = params.GetObject("returnValue", &returnValue);
    if (ret == Result::SUCCESS) {
        remoteObject = RemoteObject::Create(*returnValue);
        if (remoteObject != nullptr) {
            callFrame->returnValue_ = std::move(remoteObject);
        } else {
            error += "'returnValue' format error;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'returnValue';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "CallFrame::Create " << error;
        return nullptr;
    }

    return callFrame;
}

Local<ObjectRef> CallFrame::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> CallFrame::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("callFrameId", std::to_string(callFrameId_).c_str());
    result->Add("functionName", functionName_.c_str());

    if (functionLocation_ && functionLocation_.value() != nullptr) {
        result->Add("functionLocation", functionLocation_.value()->ToJson());
    }
    if (location_ != nullptr) {
        result->Add("location", location_->ToJson());
    }
    result->Add("url", url_.c_str());

    size_t len = scopeChain_.size();
    std::unique_ptr<PtJson> values = PtJson::CreateArray();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> scope = scopeChain_[i]->ToJson();
        values->Push(scope);
    }
    result->Add("scopeChain", values);

    if (this_ != nullptr) {
        result->Add("this", this_->ToJson());
    }
    if (returnValue_ && returnValue_.value() != nullptr) {
        result->Add("returnValue", returnValue_.value()->ToJson());
    }

    return result;
}

std::unique_ptr<SamplingHeapProfileSample> SamplingHeapProfileSample::Create(const EcmaVM *ecmaVm,
                                                                             const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileSample::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<SamplingHeapProfileSample> SamplingHeapProfileSample::Create(const PtJson &params)
{
    std::string error;
    auto samplingHeapProfileSample = std::make_unique<SamplingHeapProfileSample>();
    Result ret;

    int32_t size;
    ret = params.GetInt("size", &size);
    if (ret == Result::SUCCESS) {
        samplingHeapProfileSample->size_ = size;
    } else {
        error += "Unknown 'size';";
    }
    int32_t nodeId;
    ret = params.GetInt("nodeId", &nodeId);
    if (ret == Result::SUCCESS) {
        samplingHeapProfileSample->nodeId_ = nodeId;
    } else {
        error += "Unknown 'nodeId';";
    }
    int32_t ordinal;
    ret = params.GetInt("ordinal", &ordinal);
    if (ret == Result::SUCCESS) {
        samplingHeapProfileSample->ordinal_ = ordinal;
    } else {
        error += "Unknown 'ordinal';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileSample::Create " << error;
        return nullptr;
    }

    return samplingHeapProfileSample;
}

Local<ObjectRef> SamplingHeapProfileSample::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> SamplingHeapProfileSample::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("size", static_cast<int32_t>(size_));
    result->Add("nodeId", nodeId_);
    result->Add("ordinal", static_cast<int32_t>(ordinal_));

    return result;
}

std::unique_ptr<RuntimeCallFrame> RuntimeCallFrame::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "RuntimeCallFrame::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto runtimeCallFrame = std::make_unique<RuntimeCallFrame>();

    Local<JSValueRef> result = Local<ObjectRef>(params)->Get(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->functionName_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'functionName' should be a String;";
        }
    } else {
        error += "should contain 'functionName';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->scriptId_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }

    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            runtimeCallFrame->url_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<RuntimeCallFrame> RuntimeCallFrame::Create(const PtJson &params)
{
    std::string error;
    auto runtimeCallFrame = std::make_unique<RuntimeCallFrame>();
    Result ret;
    
    std::string functionName;
    ret = params.GetString("functionName", &functionName);
    if (ret == Result::SUCCESS) {
        runtimeCallFrame->functionName_ = std::move(functionName);
    } else {
        error += "Unknown 'functionName';";
    }

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        runtimeCallFrame->scriptId_ = std::move(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        runtimeCallFrame->url_ = std::move(url);
    } else {
        error += "Unknown 'url';";
    }
    
    int32_t lineNumber;
    ret = params.GetInt("lineNumber", &lineNumber);
    if (ret == Result::SUCCESS) {
        runtimeCallFrame->lineNumber_ = lineNumber;
    } else {
        error += "Unknown 'lineNumber';";
    }

    int32_t columnNumber;
    ret = params.GetInt("columnNumber", &columnNumber);
    if (ret == Result::SUCCESS) {
        runtimeCallFrame->columnNumber_ = columnNumber;
    } else {
        error += "Unknown 'columnNumber';";
    }
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "RuntimeCallFrame::Create " << error;
        return nullptr;
    }

    return runtimeCallFrame;
}

std::unique_ptr<RuntimeCallFrame> RuntimeCallFrame::FromFrameInfo(const FrameInfo &cpuFrameInfo)
{
    auto runtimeCallFrame = std::make_unique<RuntimeCallFrame>();
    runtimeCallFrame->SetFunctionName(cpuFrameInfo.functionName);
    runtimeCallFrame->SetScriptId(std::to_string(cpuFrameInfo.scriptId));
    runtimeCallFrame->SetColumnNumber(cpuFrameInfo.columnNumber);
    runtimeCallFrame->SetLineNumber(cpuFrameInfo.lineNumber);
    runtimeCallFrame->SetUrl(cpuFrameInfo.url);
    return runtimeCallFrame;
}

Local<ObjectRef> RuntimeCallFrame::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> RuntimeCallFrame::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("functionName", functionName_.c_str());
    result->Add("scriptId", scriptId_.c_str());
    result->Add("url", url_.c_str());
    result->Add("lineNumber", lineNumber_);
    result->Add("columnNumber", columnNumber_);

    return result;
}

std::unique_ptr<SamplingHeapProfileNode> SamplingHeapProfileNode::Create(const EcmaVM *ecmaVm,
                                                                         const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileNode::Create params is nullptr";
        return nullptr;
    }

    std::string error;
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

std::unique_ptr<SamplingHeapProfileNode> SamplingHeapProfileNode::Create(const PtJson &params)
{
    std::string error;
    auto samplingHeapProfileNode = std::make_unique<SamplingHeapProfileNode>();
    Result ret;

    std::unique_ptr<PtJson> callFrame;
    ret = params.GetObject("callFrame", &callFrame);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<RuntimeCallFrame> runtimeCallFrame = RuntimeCallFrame::Create(*callFrame);
        if (runtimeCallFrame == nullptr) {
            error += "'callFrame' format invalid;";
        } else {
            samplingHeapProfileNode->callFrame_ = std::move(runtimeCallFrame);
        }
    } else {
        error += "Unknown 'callFrame';";
    }
    
    int32_t selfSize;
    ret = params.GetInt("selfSize", &selfSize);
    if (ret == Result::SUCCESS) {
        samplingHeapProfileNode->selfSize_ = static_cast<size_t>(selfSize);
    } else {
        error += "Unknown 'selfSize';";
    }

    int32_t id;
    ret = params.GetInt("id", &id);
    if (ret == Result::SUCCESS) {
        samplingHeapProfileNode->id_ = id;
    } else {
        error += "Unknown 'id';";
    }

    std::unique_ptr<PtJson> children;
    ret = params.GetArray("children", &children);
    if (ret == Result::SUCCESS) {
        int32_t len = children->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = children->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<SamplingHeapProfileNode> node = SamplingHeapProfileNode::Create(*arrayEle);
                if (node == nullptr) {
                    error += "'children' format invalid;";
                } else {
                    samplingHeapProfileNode->children_.emplace_back(std::move(node));
                }
            } else {
                error += "Unknown 'children';";
            }
        }
    } else {
        error += "Unknown 'children';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfileNode::Create " << error;
        return nullptr;
    }

    return samplingHeapProfileNode;
}

Local<ObjectRef> SamplingHeapProfileNode::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> SamplingHeapProfileNode::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();
    if (callFrame_ != nullptr) {
        result->Add("callFrame", callFrame_->ToJson());
    }
    
    result->Add("selfSize", static_cast<int32_t>(selfSize_));
    result->Add("id", id_);
    
    std::unique_ptr<PtJson> childrens = PtJson::CreateArray();
    size_t len = children_.size();
    for (size_t i = 0; i < len; i++) {
        childrens->Push(children_[i]->ToJson());
    }
    result->Add("children", childrens);
    return result;
}

std::unique_ptr<SamplingHeapProfile> SamplingHeapProfile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfile::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<SamplingHeapProfile> SamplingHeapProfile::Create(const PtJson &params)
{
    std::string error;
    auto samplingHeapProfile = std::make_unique<SamplingHeapProfile>();
    Result ret;
    
    std::unique_ptr<PtJson> head;
    ret = params.GetObject("head", &head);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<SamplingHeapProfileNode> pHead = SamplingHeapProfileNode::Create(*head);
        if (pHead == nullptr) {
            error += "'sample' format invalid;";
        } else {
            samplingHeapProfile->head_ = std::move(pHead);
        }
    } else {
        error += "Unknown 'head';";
    }

    std::unique_ptr<PtJson> samples;
    ret = params.GetArray("samples", &samples);
    if (ret == Result::SUCCESS) {
        int32_t len = samples->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = samples->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<SamplingHeapProfileSample> pSample = SamplingHeapProfileSample::Create(*arrayEle);
                if (pSample == nullptr) {
                    error += "'sample' format invalid;";
                } else {
                    samplingHeapProfile->samples_.emplace_back(std::move(pSample));
                }
            } else {
                error += "Unknown 'samples';";
            }
        }
    } else {
        error += "Unknown 'samples';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "SamplingHeapProfile::Create " << error;
        return nullptr;
    }

    return samplingHeapProfile;
}

Local<ObjectRef> SamplingHeapProfile::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> SamplingHeapProfile::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("head", head_->ToJson());
    
    std::unique_ptr<PtJson> samples = PtJson::CreateArray();
    size_t len = samples_.size();
    for (size_t i = 0; i < len; i++) {
        if (samples_[i] != nullptr) {
            samples->Push(samples_[i]->ToJson());
        }
    }
    result->Add("samples", samples);
    return result;
}

std::unique_ptr<PositionTickInfo> PositionTickInfo::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "PositionTickInfo::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<PositionTickInfo> PositionTickInfo::Create(const PtJson &params)
{
    std::string error;
    auto positionTickInfo = std::make_unique<PositionTickInfo>();
    Result ret;
    
    int32_t line;
    ret = params.GetInt("line", &line);
    if (ret == Result::SUCCESS) {
        positionTickInfo->line_ = line;
    } else {
        error += "Unknown 'line';";
    }

    int32_t ticks;
    ret = params.GetInt("ticks", &ticks);
    if (ret == Result::SUCCESS) {
        positionTickInfo->ticks_ = ticks;
    } else {
        error += "Unknown 'ticks';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "PositionTickInfo::Create " << error;
        return nullptr;
    }

    return positionTickInfo;
}

Local<ObjectRef> PositionTickInfo::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> PositionTickInfo::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("line", static_cast<int32_t>(line_));
    result->Add("ticks", static_cast<int32_t>(ticks_));

    return result;
}

std::unique_ptr<ProfileNode> ProfileNode::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ProfileNode::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto profileNode = std::make_unique<ProfileNode>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profileNode->id_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
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
            profileNode->hitCount_ = static_cast<int32_t>(Local<NumberRef>(result)->Value());
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
                int32_t pChildren;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                pChildren = resultValue->Int32Value(ecmaVm);
                profileNode->children_.value()[i] = pChildren;
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
            profileNode->deoptReason_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<ProfileNode> ProfileNode::Create(const PtJson &params)
{
    std::string error;
    auto profileNode = std::make_unique<ProfileNode>();
    Result ret;
    
    int32_t id;
    ret = params.GetInt("id", &id);
    if (ret == Result::SUCCESS) {
        profileNode->id_ = id;
    } else {
        error += "Unknown 'id';";
    }

    std::unique_ptr<PtJson> callFrame;
    ret = params.GetObject("callFrame", &callFrame);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<RuntimeCallFrame> runtimeCallFrame = RuntimeCallFrame::Create(*callFrame);
        if (runtimeCallFrame == nullptr) {
            error += "'callFrame' format invalid;";
        } else {
            profileNode->callFrame_ = std::move(runtimeCallFrame);
        }
    } else {
        error += "Unknown 'callFrame';";
    }

    int32_t hitCount;
    ret = params.GetInt("hitCount", &hitCount);
    if (ret == Result::SUCCESS) {
        profileNode->hitCount_ = hitCount;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'hitCount';";
    }

    std::unique_ptr<PtJson> children;
    ret = params.GetArray("children", &children);
    if (ret == Result::SUCCESS) {
        int32_t childrenLen = children->GetSize();
        for (int32_t i = 0; i < childrenLen; ++i) {
            int32_t pChildren = children->Get(i)->GetInt();
            profileNode->children_.value().emplace_back(pChildren);
        }
    }  else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'children';";
    }

    std::unique_ptr<PtJson> positionTicks;
    ret = params.GetArray("positionTicks", &positionTicks);
    if (ret == Result::SUCCESS) {
        int32_t positionTicksLen = positionTicks->GetSize();
        for (int32_t i = 0; i < positionTicksLen; ++i) {
            std::unique_ptr<PtJson> arrayEle = positionTicks->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<PositionTickInfo> tmpPositionTicks = PositionTickInfo::Create(*arrayEle);
                if (tmpPositionTicks == nullptr) {
                    error += "'positionTicks' format invalid;";
                } else {
                    profileNode->positionTicks_.value().emplace_back(std::move(tmpPositionTicks));
                }
            } else {
                error += "Unknown 'positionTicks';";
            }
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'positionTicks';";
    }

    std::string deoptReason;
    ret = params.GetString("deoptReason", &deoptReason);
    if (ret == Result::SUCCESS) {
        profileNode->deoptReason_ = std::move(deoptReason);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'deoptReason';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ProfileNode::Create " << error;
        return nullptr;
    }

    return profileNode;
}

std::unique_ptr<ProfileNode> ProfileNode::FromCpuProfileNode(const CpuProfileNode &cpuProfileNode)
{
    auto profileNode = std::make_unique<ProfileNode>();
    profileNode->SetId(cpuProfileNode.id);
    profileNode->SetHitCount(cpuProfileNode.hitCount);

    size_t childrenLen = cpuProfileNode.children.size();
    std::vector<int32_t> tmpChildren;
    tmpChildren.reserve(childrenLen);
    for (uint32_t i = 0; i < childrenLen; ++i) {
        tmpChildren.push_back(cpuProfileNode.children[i]);
    }
    profileNode->SetChildren(tmpChildren);
    profileNode->SetCallFrame(RuntimeCallFrame::FromFrameInfo(cpuProfileNode.codeEntry));
    return profileNode;
}

Local<ObjectRef> ProfileNode::ToObject(const EcmaVM *ecmaVm) const
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
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, children_.value()[i]);
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

std::unique_ptr<PtJson> ProfileNode::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("id", id_);
    result->Add("callFrame", callFrame_->ToJson());
    if (hitCount_) {
        result->Add("hitCount", hitCount_.value());
    }
    if (children_) {
        std::unique_ptr<PtJson> childrens = PtJson::CreateArray();
        size_t len = children_->size();
        for (size_t i = 0; i < len; i++) {
            childrens->Push(children_.value()[i]);
        }
        result->Add("children", childrens);
    }
    if (positionTicks_) {
        std::unique_ptr<PtJson> positionTicks = PtJson::CreateArray();
        size_t len = positionTicks_->size();
        for (size_t i = 0; i < len; i++) {
            if (positionTicks_.value()[i] != nullptr) {
                positionTicks->Push(positionTicks_.value()[i]->ToJson());
            }
        }
        result->Add("positionTicks", positionTicks);
    }

    if (deoptReason_) {
        result->Add("deoptReason", deoptReason_.value().c_str());
    }
    
    return result;
}

std::unique_ptr<Profile> Profile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Profile::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto profile = std::make_unique<Profile>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodes")));
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
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startTime")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profile->startTime_ = static_cast<int64_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'startTime' should be a Number;";
        }
    } else {
        error += "should contain 'startTime';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endTime")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsNumber()) {
            profile->endTime_ = static_cast<int64_t>(Local<NumberRef>(result)->Value());
        } else {
            error += "'endTime' should be a Number;";
        }
    } else {
        error += "should contain 'endTime';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsArray(ecmaVm)) {
            auto array = Local<ArrayRef>(result);
            int32_t samplesLen = array->Length(ecmaVm);
            Local<JSValueRef> key = JSValueRef::Undefined(ecmaVm);
            for (int32_t i = 0; i < samplesLen; ++i) {
                key = IntegerRef::New(ecmaVm, i);
                int32_t pSamples;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                pSamples = resultValue->Int32Value(ecmaVm);
                profile->samples_.value()[i] = pSamples;
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
                int32_t pTime;
                Local<JSValueRef> resultValue = Local<ObjectRef>(array)->Get(ecmaVm, key->ToString(ecmaVm));
                pTime = resultValue->Int32Value(ecmaVm);
                profile->timeDeltas_.value()[i] = pTime;
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

std::unique_ptr<Profile> Profile::Create(const PtJson &params)
{
    std::string error;
    auto profile = std::make_unique<Profile>();
    Result ret;
    
    std::unique_ptr<PtJson> nodes;
    ret = params.GetArray("nodes", &nodes);
    if (ret == Result::SUCCESS) {
        int32_t nodesLen = nodes->GetSize();
        for (int32_t i = 0; i < nodesLen; ++i) {
            std::unique_ptr<PtJson> arrayEle = nodes->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<ProfileNode> profileNode = ProfileNode::Create(*arrayEle);
                if (profileNode == nullptr) {
                    error += "'nodes' format invalid;";
                } else {
                    profile->nodes_.emplace_back(std::move(profileNode));
                }
            } else {
                error += "Unknown 'nodes';";
            }
        }
    } else {
        error += "Unknown 'nodes';";
    }
    
    int64_t startTime;
    ret = params.GetInt64("startTime", &startTime);
    if (ret == Result::SUCCESS) {
        profile->startTime_ = startTime;
    } else {
        error += "Unknown 'startTime';";
    }

    int64_t endTime;
    ret = params.GetInt64("endTime", &endTime);
    if (ret == Result::SUCCESS) {
        profile->endTime_ = endTime;
    } else {
        error += "Unknown 'endTime';";
    }

    std::unique_ptr<PtJson> samples;
    ret = params.GetArray("samples", &samples);
    if (ret == Result::SUCCESS) {
        int32_t samplesLen = samples->GetSize();
        for (int32_t i = 0; i < samplesLen; ++i) {
            int32_t pSamples = samples->Get(i)->GetInt();
            profile->samples_.value().emplace_back(pSamples);
        }
    }  else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'samples';";
    }

    std::unique_ptr<PtJson> timeDeltas;
    ret = params.GetArray("timeDeltas", &timeDeltas);
    if (ret == Result::SUCCESS) {
        int32_t timeDeltasLen = timeDeltas->GetSize();
        for (int32_t i = 0; i < timeDeltasLen; ++i) {
            int32_t pTimeDeltas = timeDeltas->Get(i)->GetInt();
            profile->timeDeltas_.value().emplace_back(pTimeDeltas);
        }
    }  else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'timeDeltas';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Profile::Create " << error;
        return nullptr;
    }

    return profile;
}

std::unique_ptr<Profile> Profile::FromProfileInfo(const ProfileInfo &profileInfo)
{
    auto profile = std::make_unique<Profile>();
    profile->SetStartTime(static_cast<int64_t>(profileInfo.startTime));
    profile->SetEndTime(static_cast<int64_t>(profileInfo.stopTime));
    size_t samplesLen = profileInfo.samples.size();
    std::vector<int32_t> tmpSamples;
    tmpSamples.reserve(samplesLen);
    for (uint32_t i = 0; i < samplesLen; ++i) {
        tmpSamples.push_back(profileInfo.samples[i]);
    }
    profile->SetSamples(tmpSamples);

    size_t timeDeltasLen = profileInfo.timeDeltas.size();
    std::vector<int32_t> tmpTimeDeltas;
    tmpTimeDeltas.reserve(timeDeltasLen);
    for (uint32_t i = 0; i < timeDeltasLen; ++i) {
        tmpTimeDeltas.push_back(profileInfo.timeDeltas[i]);
    }
    profile->SetTimeDeltas(tmpTimeDeltas);

    std::vector<std::unique_ptr<ProfileNode>> profileNode;
    size_t nodesLen = profileInfo.nodes.size();
    for (uint32_t i = 0; i < nodesLen; ++i) {
        const auto &cpuProfileNode = profileInfo.nodes[i];
        profileNode.push_back(ProfileNode::FromCpuProfileNode(cpuProfileNode));
    }
    profile->SetNodes(std::move(profileNode));
    return profile;
}

Local<ObjectRef> Profile::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    size_t nodeLen = nodes_.size();
    Local<ArrayRef> nodeValues = ArrayRef::New(ecmaVm, nodeLen);
    for (size_t i = 0; i < nodeLen; i++) {
        Local<ObjectRef> profileNode = nodes_[i]->ToObject(ecmaVm);
        nodeValues->Set(ecmaVm, i, profileNode);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "nodes")), nodeValues);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startTime")),
        NumberRef::New(ecmaVm, startTime_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endTime")),
        NumberRef::New(ecmaVm, endTime_));
    
    if (samples_) {
        size_t samplesLen = samples_->size();
        Local<ArrayRef> sampleValues = ArrayRef::New(ecmaVm, samplesLen);
        for (size_t i = 0; i < samplesLen; i++) {
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, samples_.value()[i]);
            sampleValues->Set(ecmaVm, i, elem);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "samples")), sampleValues);
    }
    
    if (timeDeltas_) {
        size_t tdLen = timeDeltas_->size();
        Local<ArrayRef> timeValues = ArrayRef::New(ecmaVm, tdLen);
        for (size_t i = 0; i < tdLen; i++) {
            Local<IntegerRef> elem = IntegerRef::New(ecmaVm, timeDeltas_.value()[i]);
            timeValues->Set(ecmaVm, i, elem);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timeDeltas")), timeValues);
    }
    
    return params;
}

std::unique_ptr<PtJson> Profile::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("startTime", startTime_);
    result->Add("endTime", endTime_);

    std::unique_ptr<PtJson> nodes = PtJson::CreateArray();
    size_t nodesLen = nodes_.size();
    for (size_t i = 0; i < nodesLen; i++) {
        if (nodes_[i] != nullptr) {
            nodes->Push(nodes_[i]->ToJson());
        }
    }
    result->Add("nodes", nodes);

    if (samples_) {
        std::unique_ptr<PtJson> samples = PtJson::CreateArray();
        size_t samplesLen = samples_->size();
        for (size_t i = 0; i < samplesLen; i++) {
            samples->Push(samples_.value()[i]);
        }
        result->Add("samples", samples);
    }

    if (timeDeltas_) {
        std::unique_ptr<PtJson> timeDeltas = PtJson::CreateArray();
        size_t timeDeltasLen = timeDeltas_->size();
        for (size_t i = 0; i < timeDeltasLen; i++) {
            timeDeltas->Push(timeDeltas_.value()[i]);
        }
        result->Add("timeDeltas", timeDeltas);
    }
    
    return result;
}

std::unique_ptr<Coverage> Coverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "Coverage::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<Coverage> Coverage::Create(const PtJson &params)
{
    std::string error;
    auto coverage = std::make_unique<Coverage>();
    Result ret;
    
    int32_t startOffset;
    ret = params.GetInt("startOffset", &startOffset);
    if (ret == Result::SUCCESS) {
        coverage->startOffset_ = startOffset;
    } else {
        error += "Unknown 'startOffset';";
    }

    int32_t endOffset;
    ret = params.GetInt("endOffset", &endOffset);
    if (ret == Result::SUCCESS) {
        coverage->endOffset_ = endOffset;
    } else {
        error += "Unknown 'endOffset';";
    }

    int32_t count;
    ret = params.GetInt("count", &count);
    if (ret == Result::SUCCESS) {
        coverage->count_ = count;
    } else {
        error += "Unknown 'count';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "Coverage::Create " << error;
        return nullptr;
    }

    return coverage;
}

Local<ObjectRef> Coverage::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> Coverage::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("startOffset", startOffset_);
    result->Add("endOffset", endOffset_);
    result->Add("count", count_);
    
    return result;
}

std::unique_ptr<FunctionCoverage> FunctionCoverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "FunctionCoverage::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto functionCoverage = std::make_unique<FunctionCoverage>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "functionName")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            functionCoverage->functionName_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<FunctionCoverage> FunctionCoverage::Create(const PtJson &params)
{
    std::string error;
    auto functionCoverage = std::make_unique<FunctionCoverage>();
    Result ret;
    
    std::string functionName;
    ret = params.GetString("functionName", &functionName);
    if (ret == Result::SUCCESS) {
        functionCoverage->functionName_ = std::move(functionName);
    } else {
        error += "Unknown 'functionName';";
    }

    std::unique_ptr<PtJson> ranges;
    ret = params.GetArray("ranges", &ranges);
    if (ret == Result::SUCCESS) {
        int32_t len = ranges->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = ranges->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<Coverage> pRanges = Coverage::Create(*arrayEle);
                if (pRanges == nullptr) {
                    error += "'ranges' format invalid;";
                } else {
                    functionCoverage->ranges_.emplace_back(std::move(pRanges));
                }
            } else {
                error += "Unknown 'ranges';";
            }
        }
    } else {
        error += "Unknown 'ranges';";
    }

    bool isBlockCoverage;
    ret = params.GetBool("isBlockCoverage", &isBlockCoverage);
    if (ret == Result::SUCCESS) {
        functionCoverage->isBlockCoverage_ = isBlockCoverage;
    } else {
        error += "Unknown 'isBlockCoverage';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "FunctionCoverage::Create " << error;
        return nullptr;
    }

    return functionCoverage;
}

Local<ObjectRef> FunctionCoverage::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> FunctionCoverage::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("functionName", functionName_.c_str());

    std::unique_ptr<PtJson> ranges = PtJson::CreateArray();
    size_t len = ranges_.size();
    for (size_t i = 0; i < len; i++) {
        if (ranges_[i] != nullptr) {
            ranges->Push(ranges_[i]->ToJson());
        }
    }
    result->Add("ranges", ranges);

    result->Add("isBlockCoverage", isBlockCoverage_);
    
    return result;
}

std::unique_ptr<ScriptCoverage> ScriptCoverage::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptCoverage::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto scriptCoverage = std::make_unique<ScriptCoverage>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptCoverage->scriptId_ = DebuggerApi::ToStdString(result);
        } else {
            error += "'scriptId' should be a String;";
        }
    } else {
        error += "should contain 'scriptId';";
    }
    result = Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptCoverage->url_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<ScriptCoverage> ScriptCoverage::Create(const PtJson &params)
{
    std::string error;
    auto scriptCoverage = std::make_unique<ScriptCoverage>();
    Result ret;
    
    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        scriptCoverage->scriptId_ = std::move(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        scriptCoverage->url_ = std::move(url);
    } else {
        error += "Unknown 'url';";
    }

    std::unique_ptr<PtJson> functions;
    ret = params.GetArray("functions", &functions);
    if (ret == Result::SUCCESS) {
        int32_t len = functions->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = functions->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<FunctionCoverage> pFunctions = FunctionCoverage::Create(*arrayEle);
                if (pFunctions == nullptr) {
                    error += "'functions' format invalid;";
                } else {
                    scriptCoverage->functions_.emplace_back(std::move(pFunctions));
                }
            } else {
                error += "Unknown 'functions';";
            }
        }
    } else {
        error += "Unknown 'functions';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptCoverage::Create " << error;
        return nullptr;
    }

    return scriptCoverage;
}

Local<ObjectRef> ScriptCoverage::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> ScriptCoverage::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", scriptId_.c_str());
    result->Add("url", url_.c_str());

    std::unique_ptr<PtJson> functions = PtJson::CreateArray();
    size_t len = functions_.size();
    for (size_t i = 0; i < len; i++) {
        if (functions_[i] != nullptr) {
            functions->Push(functions_[i]->ToJson());
        }
    }
    result->Add("functions", functions);
    
    return result;
}

std::unique_ptr<TypeObject> TypeObject::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "TypeObject::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto typeObject = std::make_unique<TypeObject>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            typeObject->name_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<TypeObject> TypeObject::Create(const PtJson &params)
{
    std::string error;
    auto typeObject = std::make_unique<TypeObject>();
    Result ret;
    
    std::string name;
    ret = params.GetString("name", &name);
    if (ret == Result::SUCCESS) {
        typeObject->name_ = std::move(name);
    } else {
        error += "Unknown 'name';";
    }

    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "TypeObject::Create " << error;
        return nullptr;
    }

    return typeObject;
}

Local<ObjectRef> TypeObject::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "name")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, name_.c_str())));
    return params;
}

std::unique_ptr<PtJson> TypeObject::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("name", name_.c_str());
    
    return result;
}

std::unique_ptr<TypeProfileEntry> TypeProfileEntry::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "TypeProfileEntry::Create params is nullptr";
        return nullptr;
    }
    std::string error;
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

std::unique_ptr<TypeProfileEntry> TypeProfileEntry::Create(const PtJson &params)
{
    std::string error;
    auto typeProfileEntry = std::make_unique<TypeProfileEntry>();
    Result ret;
    
    int32_t offset;
    ret = params.GetInt("offset", &offset);
    if (ret == Result::SUCCESS) {
        typeProfileEntry->offset_ = offset;
    } else {
        error += "Unknown 'offset';";
    }

    std::unique_ptr<PtJson> types;
    ret = params.GetArray("types", &types);
    if (ret == Result::SUCCESS) {
        int32_t len = types->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = types->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<TypeObject> pTypes = TypeObject::Create(*arrayEle);
                if (pTypes == nullptr) {
                    error += "'types' format invalid;";
                } else {
                    typeProfileEntry->types_.emplace_back(std::move(pTypes));
                }
            } else {
                error += "Unknown 'types';";
            }
        }
    } else {
        error += "Unknown 'types';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "TypeProfileEntry::Create " << error;
        return nullptr;
    }

    return typeProfileEntry;
}

Local<ObjectRef> TypeProfileEntry::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> TypeProfileEntry::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("offset", static_cast<int32_t>(offset_));

    std::unique_ptr<PtJson> types = PtJson::CreateArray();
    size_t len = types_.size();
    for (size_t i = 0; i < len; i++) {
        types->Push(types_[i]->ToJson());
    }
    result->Add("types", types);

    return result;
}

std::unique_ptr<ScriptTypeProfile> ScriptTypeProfile::Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params)
{
    if (params.IsEmpty() || !params->IsObject()) {
        LOG(ERROR, DEBUGGER) << "ScriptTypeProfile::Create params is nullptr";
        return nullptr;
    }
    std::string error;
    auto scriptTypeProfile = std::make_unique<ScriptTypeProfile>();

    Local<JSValueRef> result =
        Local<ObjectRef>(params)->Get(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")));
    if (!result.IsEmpty() && !result->IsUndefined()) {
        if (result->IsString()) {
            scriptTypeProfile->scriptId_ = DebuggerApi::ToStdString(result);
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
            scriptTypeProfile->url_ = DebuggerApi::ToStdString(result);
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

std::unique_ptr<ScriptTypeProfile> ScriptTypeProfile::Create(const PtJson &params)
{
    std::string error;
    auto scriptTypeProfile = std::make_unique<ScriptTypeProfile>();
    Result ret;
    
    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        scriptTypeProfile->scriptId_ = std::move(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        scriptTypeProfile->url_ = std::move(url);
    } else {
        error += "Unknown 'url';";
    }

    std::unique_ptr<PtJson> entries;
    ret = params.GetArray("entries", &entries);
    if (ret == Result::SUCCESS) {
        int32_t len = entries->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> arrayEle = entries->Get(i);
            if (arrayEle != nullptr) {
                std::unique_ptr<TypeProfileEntry> pEntries = TypeProfileEntry::Create(*arrayEle);
                if (pEntries == nullptr) {
                    error += "'entries' format invalid;";
                } else {
                    scriptTypeProfile->entries_.emplace_back(std::move(pEntries));
                }
            } else {
                error += "Unknown 'entries';";
            }
        }
    } else {
        error += "Unknown 'entries';";
    }
    
    if (!error.empty()) {
        LOG(ERROR, DEBUGGER) << "ScriptTypeProfile::Create " << error;
        return nullptr;
    }

    return scriptTypeProfile;
}

Local<ObjectRef> ScriptTypeProfile::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> ScriptTypeProfile::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", scriptId_.c_str());
    result->Add("url", url_.c_str());

    std::unique_ptr<PtJson> entries = PtJson::CreateArray();
    size_t len = entries_.size();
    for (size_t i = 0; i < len; i++) {
        entries->Push(entries_[i]->ToJson());
    }
    result->Add("entries", entries);
    
    return result;
}
}  // namespace panda::ecmascript::tooling
