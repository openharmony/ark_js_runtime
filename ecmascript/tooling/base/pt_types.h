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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_TYPES_H
#define ECMASCRIPT_TOOLING_BASE_PT_TYPES_H

#include <memory>
#include <optional>

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/tooling/backend/debugger_api.h"
#include "ecmascript/tooling/base/pt_json.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
// ========== Base types begin
class PtBaseTypes {
public:
    PtBaseTypes() = default;
    virtual ~PtBaseTypes() = default;
    virtual Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const = 0;
    virtual std::unique_ptr<PtJson> ToJson() const
    {
        return PtJson::CreateObject();
    }

protected:
    static Local<ObjectRef> NewObject(const EcmaVM *ecmaVm);

private:
    NO_COPY_SEMANTIC(PtBaseTypes);
    NO_MOVE_SEMANTIC(PtBaseTypes);

    friend class ProtocolHandler;
    friend class DebuggerImpl;
};

// ========== Debugger types begin
// Debugger.BreakpointId
using BreakpointId = std::string;
struct BreakpointDetails {
    static BreakpointId ToString(const BreakpointDetails &metaData)
    {
        return "id:" + std::to_string(metaData.line_) + ":" + std::to_string(metaData.column_) + ":" +
            metaData.url_;
    }

    static bool ParseBreakpointId(const BreakpointId &id, BreakpointDetails *metaData)
    {
        auto lineStart = id.find(':');
        if (lineStart == std::string::npos) {
            return false;
        }
        auto columnStart = id.find(':', lineStart + 1);
        if (columnStart == std::string::npos) {
            return false;
        }
        auto urlStart = id.find(':', columnStart + 1);
        if (urlStart == std::string::npos) {
            return false;
        }
        std::string lineStr = id.substr(lineStart + 1, columnStart - lineStart - 1);
        std::string columnStr = id.substr(columnStart + 1, urlStart - columnStart - 1);
        std::string url = id.substr(urlStart + 1);
        metaData->line_ = std::stoi(lineStr);
        metaData->column_ = std::stoi(columnStr);
        metaData->url_ = url;

        return true;
    }

    int32_t line_ {0};
    int32_t column_ {0};
    std::string url_ {};
};

// Debugger.CallFrameId
using CallFrameId = uint32_t;

// ========== Runtime types begin
// Runtime.ScriptId
using ScriptId = uint32_t;

// Runtime.RemoteObjectId

using RemoteObjectId = uint32_t;

// Runtime.ExecutionContextId
using ExecutionContextId = int32_t;

// Runtime.UnserializableValue
using UnserializableValue = std::string;

// Runtime.UniqueDebuggerId
using UniqueDebuggerId = uint32_t;

// Runtime.RemoteObject
class RemoteObject : public PtBaseTypes {
public:
    RemoteObject() = default;
    ~RemoteObject() override = default;

    static std::unique_ptr<RemoteObject> FromTagged(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged);
    static std::unique_ptr<RemoteObject> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    /*
     * @see {#ObjectType}
     */
    const std::string &GetType() const
    {
        return type_;
    }

    RemoteObject &SetType(const std::string &type)
    {
        type_ = type;
        return *this;
    }
    /*
     * @see {#ObjectSubType}
     */
    const std::string &GetSubType() const
    {
        return subtype_.value();
    }

    RemoteObject &SetSubType(const std::string &type)
    {
        subtype_ = type;
        return *this;
    }

    bool HasSubType() const
    {
        return subtype_.has_value();
    }

    const std::string &GetClassName() const
    {
        return className_.value();
    }

    RemoteObject &SetClassName(const std::string &className)
    {
        className_ = className;
        return *this;
    }

    bool HasClassName() const
    {
        return className_.has_value();
    }

    Local<JSValueRef> GetValue() const
    {
        return value_.value_or(Local<JSValueRef>());
    }

    RemoteObject &SetValue(const Local<JSValueRef> &value)
    {
        value_ = value;
        return *this;
    }

    bool HasValue() const
    {
        return value_.has_value();
    }

    const UnserializableValue &GetUnserializableValue() const
    {
        return unserializableValue_.value();
    }

    RemoteObject &SetUnserializableValue(const UnserializableValue &unserializableValue)
    {
        unserializableValue_ = unserializableValue;
        return *this;
    }

    bool HasUnserializableValue() const
    {
        return unserializableValue_.has_value();
    }

    const std::string &GetDescription() const
    {
        return description_.value();
    }

    RemoteObject &SetDescription(const std::string &description)
    {
        description_ = description;
        return *this;
    }

    bool HasDescription() const
    {
        return description_.has_value();
    }

    RemoteObjectId GetObjectId() const
    {
        return objectId_.value_or(0);
    }

    RemoteObject &SetObjectId(RemoteObjectId objectId)
    {
        objectId_ = objectId;
        return *this;
    }

    bool HasObjectId() const
    {
        return objectId_.has_value();
    }

    struct TypeName {
        static const std::string Object;     // NOLINT (readability-identifier-naming)
        static const std::string Function;   // NOLINT (readability-identifier-naming)
        static const std::string Undefined;  // NOLINT (readability-identifier-naming)
        static const std::string String;     // NOLINT (readability-identifier-naming)
        static const std::string Number;     // NOLINT (readability-identifier-naming)
        static const std::string Boolean;    // NOLINT (readability-identifier-naming)
        static const std::string Symbol;     // NOLINT (readability-identifier-naming)
        static const std::string Bigint;     // NOLINT (readability-identifier-naming)
        static const std::string Wasm;       // NOLINT (readability-identifier-naming)
        static bool Valid(const std::string &type)
        {
            return type == Object || type == Function || type == Undefined || type == String || type == Number ||
                   type == Boolean || type == Symbol || type == Bigint || type == Wasm;
        }
    };

    struct SubTypeName {
        static const std::string Array;        // NOLINT (readability-identifier-naming)
        static const std::string Null;         // NOLINT (readability-identifier-naming)
        static const std::string Node;         // NOLINT (readability-identifier-naming)
        static const std::string Regexp;       // NOLINT (readability-identifier-naming)
        static const std::string Date;         // NOLINT (readability-identifier-naming)
        static const std::string Map;          // NOLINT (readability-identifier-naming)
        static const std::string Set;          // NOLINT (readability-identifier-naming)
        static const std::string Weakmap;      // NOLINT (readability-identifier-naming)
        static const std::string Weakset;      // NOLINT (readability-identifier-naming)
        static const std::string Iterator;     // NOLINT (readability-identifier-naming)
        static const std::string Generator;    // NOLINT (readability-identifier-naming)
        static const std::string Error;        // NOLINT (readability-identifier-naming)
        static const std::string Proxy;        // NOLINT (readability-identifier-naming)
        static const std::string Promise;      // NOLINT (readability-identifier-naming)
        static const std::string Typedarray;   // NOLINT (readability-identifier-naming)
        static const std::string Arraybuffer;  // NOLINT (readability-identifier-naming)
        static const std::string Dataview;     // NOLINT (readability-identifier-naming)
        static const std::string I32;          // NOLINT (readability-identifier-naming)
        static const std::string I64;          // NOLINT (readability-identifier-naming)
        static const std::string F32;          // NOLINT (readability-identifier-naming)
        static const std::string F64;          // NOLINT (readability-identifier-naming)
        static const std::string V128;         // NOLINT (readability-identifier-naming)
        static const std::string Externref;    // NOLINT (readability-identifier-naming)
        static bool Valid(const std::string &type)
        {
            return type == Array || type == Null || type == Node || type == Regexp || type == Map || type == Set ||
                   type == Weakmap || type == Iterator || type == Generator || type == Error || type == Proxy ||
                   type == Promise || type == Typedarray || type == Arraybuffer || type == Dataview || type == I32 ||
                   type == I64 || type == F32 || type == F64 || type == V128 || type == Externref;
        }
    };
    struct ClassName {
        static const std::string Object;          // NOLINT (readability-identifier-naming)
        static const std::string Function;        // NOLINT (readability-identifier-naming)
        static const std::string Array;           // NOLINT (readability-identifier-naming)
        static const std::string Regexp;          // NOLINT (readability-identifier-naming)
        static const std::string Date;            // NOLINT (readability-identifier-naming)
        static const std::string Map;             // NOLINT (readability-identifier-naming)
        static const std::string Set;             // NOLINT (readability-identifier-naming)
        static const std::string Weakmap;         // NOLINT (readability-identifier-naming)
        static const std::string Weakset;         // NOLINT (readability-identifier-naming)
        static const std::string ArrayIterator;   // NOLINT (readability-identifier-naming)
        static const std::string StringIterator;  // NOLINT (readability-identifier-naming)
        static const std::string SetIterator;     // NOLINT (readability-identifier-naming)
        static const std::string MapIterator;     // NOLINT (readability-identifier-naming)
        static const std::string Iterator;        // NOLINT (readability-identifier-naming)
        static const std::string Error;           // NOLINT (readability-identifier-naming)
        static const std::string Proxy;           // NOLINT (readability-identifier-naming)
        static const std::string Promise;         // NOLINT (readability-identifier-naming)
        static const std::string Typedarray;      // NOLINT (readability-identifier-naming)
        static const std::string Arraybuffer;     // NOLINT (readability-identifier-naming)
        static const std::string Global;          // NOLINT (readability-identifier-naming)
        static bool Valid(const std::string &type)
        {
            return type == Object || type == Array || type == Regexp || type == Date || type == Map || type == Set ||
                   type == Weakmap || type == Weakset || type == ArrayIterator || type == StringIterator ||
                   type == Error || type == SetIterator || type == MapIterator || type == Iterator || type == Proxy ||
                   type == Promise || type == Typedarray || type == Arraybuffer || type == Function;
        }
    };
    static const std::string ObjectDescription;          // NOLINT (readability-identifier-naming)
    static const std::string GlobalDescription;          // NOLINT (readability-identifier-naming)
    static const std::string ProxyDescription;           // NOLINT (readability-identifier-naming)
    static const std::string PromiseDescription;         // NOLINT (readability-identifier-naming)
    static const std::string ArrayIteratorDescription;   // NOLINT (readability-identifier-naming)
    static const std::string StringIteratorDescription;  // NOLINT (readability-identifier-naming)
    static const std::string SetIteratorDescription;     // NOLINT (readability-identifier-naming)
    static const std::string MapIteratorDescription;     // NOLINT (readability-identifier-naming)
    static const std::string WeakMapDescription;         // NOLINT (readability-identifier-naming)
    static const std::string WeakSetDescription;         // NOLINT (readability-identifier-naming)

private:
    NO_COPY_SEMANTIC(RemoteObject);
    NO_MOVE_SEMANTIC(RemoteObject);

    std::string type_ {};
    std::optional<std::string> subtype_ {};
    std::optional<std::string> className_ {};
    std::optional<Local<JSValueRef>> value_ {};
    std::optional<UnserializableValue> unserializableValue_ {};
    std::optional<std::string> description_ {};
    std::optional<RemoteObjectId> objectId_ {};
};

class PrimitiveRemoteObject final : public RemoteObject {
public:
    explicit PrimitiveRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged);
    ~PrimitiveRemoteObject() override = default;
};

class StringRemoteObject final : public RemoteObject {
public:
    explicit StringRemoteObject([[maybe_unused]] const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
    {
        SetType(RemoteObject::TypeName::String).SetValue(tagged);
    }
    virtual ~StringRemoteObject() = default;
};

class SymbolRemoteObject final : public RemoteObject {
public:
    explicit SymbolRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
    {
        SetType(RemoteObject::TypeName::Symbol).SetDescription(DescriptionForSymbol(ecmaVm, Local<SymbolRef>(tagged)));
    }
    ~SymbolRemoteObject() override = default;

private:
    std::string DescriptionForSymbol(const EcmaVM *ecmaVm, const Local<SymbolRef> &tagged) const;
};

class FunctionRemoteObject final : public RemoteObject {
public:
    FunctionRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged)
    {
        SetType(RemoteObject::TypeName::Function)
            .SetClassName(RemoteObject::ClassName::Function)
            .SetDescription(DescriptionForFunction(ecmaVm, tagged));
    }
    ~FunctionRemoteObject() override = default;

private:
    std::string DescriptionForFunction(const EcmaVM *ecmaVm, const Local<FunctionRef> &tagged) const;
};

class ObjectRemoteObject final : public RemoteObject {
public:
    explicit ObjectRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged, const std::string &classname)
    {
        SetType(RemoteObject::TypeName::Object)
            .SetClassName(classname)
            .SetDescription(DescriptionForObject(ecmaVm, tagged));
    }
    explicit ObjectRemoteObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged, const std::string &classname,
        const std::string &subtype)
    {
        SetType(RemoteObject::TypeName::Object)
            .SetSubType(subtype)
            .SetClassName(classname)
            .SetDescription(DescriptionForObject(ecmaVm, tagged));
    }
    ~ObjectRemoteObject() override = default;
    static std::string DescriptionForObject(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged);

private:
    static std::string DescriptionForArray(const EcmaVM *ecmaVm, const Local<ArrayRef> &tagged);
    static std::string DescriptionForRegexp(const EcmaVM *ecmaVm, const Local<RegExpRef> &tagged);
    static std::string DescriptionForDate(const EcmaVM *ecmaVm, const Local<DateRef> &tagged);
    static std::string DescriptionForMap(const Local<MapRef> &tagged);
    static std::string DescriptionForSet(const Local<SetRef> &tagged);
    static std::string DescriptionForError(const EcmaVM *ecmaVm, const Local<JSValueRef> &tagged);
    static std::string DescriptionForArrayBuffer(const EcmaVM *ecmaVm, const Local<ArrayBufferRef> &tagged);
};

// Runtime.ExceptionDetails
class ExceptionDetails final : public PtBaseTypes {
public:
    ExceptionDetails() = default;
    ~ExceptionDetails() override = default;
    static std::unique_ptr<ExceptionDetails> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    int32_t GetExceptionId() const
    {
        return exceptionId_;
    }

    ExceptionDetails &SetExceptionId(int32_t exceptionId)
    {
        exceptionId_ = exceptionId;
        return *this;
    }

    const std::string &GetText() const
    {
        return text_;
    }

    ExceptionDetails &SetText(const std::string &text)
    {
        text_ = text;
        return *this;
    }

    int32_t GetLine() const
    {
        return line_;
    }

    ExceptionDetails &SetLine(int32_t line)
    {
        line_ = line;
        return *this;
    }

    int32_t GetColumn() const
    {
        return column_;
    }

    ExceptionDetails &SetColumn(int32_t column)
    {
        column_ = column;
        return *this;
    }

    ScriptId GetScriptId() const
    {
        return scriptId_.value_or(0);
    }

    ExceptionDetails &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    bool HasScriptId() const
    {
        return scriptId_.has_value();
    }

    const std::string &GetUrl() const
    {
        return url_.value();
    }

    ExceptionDetails &SetUrl(const std::string &url)
    {
        url_ = url;
        return *this;
    }

    bool HasUrl() const
    {
        return url_.has_value();
    }

    RemoteObject *GetException() const
    {
        if (exception_) {
            return exception_->get();
        }
        return nullptr;
    }

    ExceptionDetails &SetException(std::unique_ptr<RemoteObject> exception)
    {
        exception_ = std::move(exception);
        return *this;
    }

    bool HasException() const
    {
        return exception_.has_value();
    }

    ExecutionContextId GetExecutionContextId() const
    {
        return executionContextId_.value_or(-1);
    }

    ExceptionDetails &SetExecutionContextId(ExecutionContextId executionContextId)
    {
        executionContextId_ = executionContextId;
        return *this;
    }

    bool HasExecutionContextId() const
    {
        return executionContextId_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ExceptionDetails);
    NO_MOVE_SEMANTIC(ExceptionDetails);

    int32_t exceptionId_ {0};
    std::string text_ {};
    int32_t line_ {0};
    int32_t column_ {0};
    std::optional<ScriptId> scriptId_ {};
    std::optional<std::string> url_ {};
    std::optional<std::unique_ptr<RemoteObject>> exception_ {};
    std::optional<ExecutionContextId> executionContextId_ {0};
};

// Runtime.InternalPropertyDescriptor
class InternalPropertyDescriptor final : public PtBaseTypes {
public:
    InternalPropertyDescriptor() = default;
    ~InternalPropertyDescriptor() override = default;

    static std::unique_ptr<InternalPropertyDescriptor> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    std::string GetName() const
    {
        return name_;
    }

    InternalPropertyDescriptor &SetName(const std::string &name)
    {
        name_ = name;
        return *this;
    }

    RemoteObject *GetValue() const
    {
        if (value_) {
            return value_->get();
        }
        return nullptr;
    }

    InternalPropertyDescriptor &SetValue(std::unique_ptr<RemoteObject> value)
    {
        value_ = std::move(value);
        return *this;
    }

    bool HasValue() const
    {
        return value_.has_value();
    }

private:
    NO_COPY_SEMANTIC(InternalPropertyDescriptor);
    NO_MOVE_SEMANTIC(InternalPropertyDescriptor);

    std::string name_ {};
    std::optional<std::unique_ptr<RemoteObject>> value_ {};
};

// Runtime.PrivatePropertyDescriptor
class PrivatePropertyDescriptor final : public PtBaseTypes {
public:
    PrivatePropertyDescriptor() = default;
    ~PrivatePropertyDescriptor() override = default;

    static std::unique_ptr<PrivatePropertyDescriptor> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    std::string GetName() const
    {
        return name_;
    }

    PrivatePropertyDescriptor &SetName(const std::string &name)
    {
        name_ = name;
        return *this;
    }

    RemoteObject *GetValue() const
    {
        if (value_) {
            return value_->get();
        }
        return nullptr;
    }

    PrivatePropertyDescriptor &SetValue(std::unique_ptr<RemoteObject> value)
    {
        value_ = std::move(value);
        return *this;
    }

    bool HasValue() const
    {
        return value_.has_value();
    }

    RemoteObject *GetGet() const
    {
        if (get_) {
            return get_->get();
        }
        return nullptr;
    }

    PrivatePropertyDescriptor &SetGet(std::unique_ptr<RemoteObject> get)
    {
        get_ = std::move(get);
        return *this;
    }

    bool HasGet() const
    {
        return get_.has_value();
    }

    RemoteObject *GetSet() const
    {
        if (set_) {
            return set_->get();
        }
        return nullptr;
    }

    PrivatePropertyDescriptor &SetSet(std::unique_ptr<RemoteObject> set)
    {
        set_ = std::move(set);
        return *this;
    }

    bool HasSet() const
    {
        return set_.has_value();
    }

private:
    NO_COPY_SEMANTIC(PrivatePropertyDescriptor);
    NO_MOVE_SEMANTIC(PrivatePropertyDescriptor);

    std::string name_ {};
    std::optional<std::unique_ptr<RemoteObject>> value_ {};
    std::optional<std::unique_ptr<RemoteObject>> get_ {};
    std::optional<std::unique_ptr<RemoteObject>> set_ {};
};

// Runtime.PropertyDescriptor
class PropertyDescriptor final : public PtBaseTypes {
public:
    PropertyDescriptor() = default;
    ~PropertyDescriptor() override = default;

    static std::unique_ptr<PropertyDescriptor> FromProperty(const EcmaVM *ecmaVm, const Local<JSValueRef> &name,
        const PropertyAttribute &property);
    static std::unique_ptr<PropertyDescriptor> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    std::string GetName() const
    {
        return name_;
    }

    PropertyDescriptor &SetName(const std::string &name)
    {
        name_ = name;
        return *this;
    }

    RemoteObject *GetValue() const
    {
        if (value_) {
            return value_->get();
        }
        return nullptr;
    }

    PropertyDescriptor &SetValue(std::unique_ptr<RemoteObject> value)
    {
        value_ = std::move(value);
        return *this;
    }

    bool HasValue() const
    {
        return value_.has_value();
    }

    bool GetWritable() const
    {
        return writable_.value_or(false);
    }

    PropertyDescriptor &SetWritable(bool writable)
    {
        writable_ = writable;
        return *this;
    }

    bool HasWritable() const
    {
        return writable_.has_value();
    }

    RemoteObject *GetGet() const
    {
        if (get_) {
            return get_->get();
        }
        return nullptr;
    }

    PropertyDescriptor &SetGet(std::unique_ptr<RemoteObject> get)
    {
        get_ = std::move(get);
        return *this;
    }

    bool HasGet() const
    {
        return get_.has_value();
    }

    RemoteObject *GetSet() const
    {
        if (set_) {
            return set_->get();
        }
        return nullptr;
    }

    PropertyDescriptor &SetSet(std::unique_ptr<RemoteObject> set)
    {
        set_ = std::move(set);
        return *this;
    }

    bool HasSet() const
    {
        return set_.has_value();
    }

    bool GetConfigurable() const
    {
        return configurable_;
    }

    PropertyDescriptor &SetConfigurable(bool configurable)
    {
        configurable_ = configurable;
        return *this;
    }

    bool GetEnumerable() const
    {
        return enumerable_;
    }

    PropertyDescriptor &SetEnumerable(bool enumerable)
    {
        enumerable_ = enumerable;
        return *this;
    }

    bool GetWasThrown() const
    {
        return wasThrown_.value_or(false);
    }

    PropertyDescriptor &SetWasThrown(bool wasThrown)
    {
        wasThrown_ = wasThrown;
        return *this;
    }

    bool HasWasThrown() const
    {
        return wasThrown_.has_value();
    }

    bool GetIsOwn() const
    {
        return isOwn_.value_or(false);
    }

    PropertyDescriptor &SetIsOwn(bool isOwn)
    {
        isOwn_ = isOwn;
        return *this;
    }

    bool HasIsOwn() const
    {
        return isOwn_.has_value();
    }

    RemoteObject *GetSymbol() const
    {
        if (symbol_) {
            return symbol_->get();
        }
        return nullptr;
    }

    PropertyDescriptor &SetSymbol(std::unique_ptr<RemoteObject> symbol)
    {
        symbol_ = std::move(symbol);
        return *this;
    }

    bool HasSymbol() const
    {
        return symbol_.has_value();
    }

private:
    NO_COPY_SEMANTIC(PropertyDescriptor);
    NO_MOVE_SEMANTIC(PropertyDescriptor);

    std::string name_ {};
    std::optional<std::unique_ptr<RemoteObject>> value_ {};
    std::optional<bool> writable_ {};
    std::optional<std::unique_ptr<RemoteObject>> get_ {};
    std::optional<std::unique_ptr<RemoteObject>> set_ {};
    bool configurable_ {false};
    bool enumerable_ {false};
    std::optional<bool> wasThrown_ {};
    std::optional<bool> isOwn_ {};
    std::optional<std::unique_ptr<RemoteObject>> symbol_ {};
};

// Runtime.CallArgument
class CallArgument final : public PtBaseTypes {
public:
    CallArgument() = default;
    ~CallArgument() override = default;

    static std::unique_ptr<CallArgument> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    Local<JSValueRef> GetValue() const
    {
        return value_.value_or(Local<JSValueRef>());
    }

    CallArgument &SetValue(const Local<JSValueRef> &value)
    {
        value_ = value;
        return *this;
    }

    bool HasValue() const
    {
        return value_.has_value();
    }

    const UnserializableValue &GetUnserializableValue() const
    {
        return unserializableValue_.value();
    }

    CallArgument &SetUnserializableValue(const UnserializableValue &unserializableValue)
    {
        unserializableValue_ = unserializableValue;
        return *this;
    }

    bool HasUnserializableValue() const
    {
        return unserializableValue_.has_value();
    }

    RemoteObjectId GetObjectId() const
    {
        return objectId_.value_or(0);
    }

    CallArgument &SetObjectId(RemoteObjectId objectId)
    {
        objectId_ = objectId;
        return *this;
    }

    bool HasObjectId() const
    {
        return objectId_.has_value();
    }

private:
    NO_COPY_SEMANTIC(CallArgument);
    NO_MOVE_SEMANTIC(CallArgument);

    std::optional<Local<JSValueRef>> value_ {};
    std::optional<UnserializableValue> unserializableValue_ {};
    std::optional<RemoteObjectId> objectId_ {};
};

// ========== Debugger types begin
// Debugger.ScriptLanguage
struct ScriptLanguage {
    static bool Valid(const std::string &language)
    {
        return language == JavaScript() || language == WebAssembly();
    }
    static std::string JavaScript()
    {
        return "JavaScript";
    }
    static std::string WebAssembly()
    {
        return "WebAssembly";
    }
};

// Debugger.Location
class Location : public PtBaseTypes {
public:
    Location() = default;
    ~Location() override = default;

    static std::unique_ptr<Location> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    static std::unique_ptr<Location> Create(const PtJson &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    Location &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    int32_t GetLine() const
    {
        return line_;
    }

    Location &SetLine(int32_t line)
    {
        line_ = line;
        return *this;
    }

    int32_t GetColumn() const
    {
        return column_.value_or(-1);
    }

    Location &SetColumn(int32_t column)
    {
        column_ = column;
        return *this;
    }

    bool HasColumn() const
    {
        return column_.has_value();
    }

private:
    NO_COPY_SEMANTIC(Location);
    NO_MOVE_SEMANTIC(Location);

    ScriptId scriptId_ {};
    int32_t line_ {0};
    std::optional<int32_t> column_ {};
};

// Debugger.ScriptPosition
class ScriptPosition : public PtBaseTypes {
public:
    ScriptPosition() = default;
    ~ScriptPosition() override = default;

    static std::unique_ptr<ScriptPosition> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    int32_t GetLine() const
    {
        return line_;
    }

    ScriptPosition &SetLine(int32_t line)
    {
        line_ = line;
        return *this;
    }

    int32_t GetColumn() const
    {
        return column_;
    }

    ScriptPosition &SetColumn(int32_t column)
    {
        column_ = column;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(ScriptPosition);
    NO_MOVE_SEMANTIC(ScriptPosition);

    int32_t line_ {0};
    int32_t column_ {0};
};

// Debugger.SearchMatch
class SearchMatch : public PtBaseTypes {
public:
    SearchMatch() = default;
    ~SearchMatch() override = default;
    static std::unique_ptr<SearchMatch> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

private:
    NO_COPY_SEMANTIC(SearchMatch);
    NO_MOVE_SEMANTIC(SearchMatch);

    int32_t lineNumber_ {0};
    std::string lineContent_ {};
};

// Debugger.LocationRange
class LocationRange : public PtBaseTypes {
public:
    LocationRange() = default;
    ~LocationRange() override = default;

    static std::unique_ptr<LocationRange> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    LocationRange &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    ScriptPosition *GetStart() const
    {
        return start_.get();
    }

    LocationRange &SetStart(std::unique_ptr<ScriptPosition> start)
    {
        start_ = std::move(start);
        return *this;
    }

    ScriptPosition *GetEnd() const
    {
        return end_.get();
    }

    LocationRange &SetEnd(std::unique_ptr<ScriptPosition> end)
    {
        end_ = std::move(end);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(LocationRange);
    NO_MOVE_SEMANTIC(LocationRange);

    ScriptId scriptId_ {};
    std::unique_ptr<ScriptPosition> start_ {nullptr};
    std::unique_ptr<ScriptPosition> end_ {nullptr};
};

// Debugger.BreakLocation
class BreakLocation final : public PtBaseTypes {
public:
    BreakLocation() = default;
    ~BreakLocation() override = default;

    static std::unique_ptr<BreakLocation> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    BreakLocation &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    int32_t GetLine() const
    {
        return line_;
    }

    BreakLocation &SetLine(int32_t line)
    {
        line_ = line;
        return *this;
    }

    int32_t GetColumn() const
    {
        return column_.value_or(-1);
    }

    BreakLocation &SetColumn(int32_t column)
    {
        column_ = column;
        return *this;
    }

    bool HasColumn() const
    {
        return column_.has_value();
    }

    /*
     * @see {#BreakType}
     */
    const std::string &GetType() const
    {
        return type_.value();
    }

    BreakLocation &SetType(const std::string &type)
    {
        type_ = type;
        return *this;
    }

    bool HasType() const
    {
        return type_.has_value();
    }

    struct Type {
        static bool Valid(const std::string &type)
        {
            return type == DebuggerStatement() || type == Call() || type == Return();
        }
        static std::string DebuggerStatement()
        {
            return "debuggerStatement";
        }
        static std::string Call()
        {
            return "call";
        }
        static std::string Return()
        {
            return "return";
        }
    };

private:
    NO_COPY_SEMANTIC(BreakLocation);
    NO_MOVE_SEMANTIC(BreakLocation);

    ScriptId scriptId_ {};
    int32_t line_ {0};
    std::optional<int32_t> column_ {};
    std::optional<std::string> type_ {};
};
using BreakType = BreakLocation::Type;

enum class ScopeType : uint8_t {
    GLOBAL,
    LOCAL,
    WITH,
    CLOSURE,
    CATCH,
    BLOCK,
    SCRIPT,
    EVAL,
    MODULE,
    WASM_EXPRESSION_STACK
};

// Debugger.Scope
class Scope final : public PtBaseTypes {
public:
    Scope() = default;
    ~Scope() override = default;

    static std::unique_ptr<Scope> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    /*
     * @see {#Scope::Type}
     */
    const std::string &GetType() const
    {
        return type_;
    }

    Scope &SetType(const std::string &type)
    {
        type_ = type;
        return *this;
    }

    RemoteObject *GetObject() const
    {
        return object_.get();
    }

    Scope &SetObject(std::unique_ptr<RemoteObject> params)
    {
        object_ = std::move(params);
        return *this;
    }

    const std::string &GetName() const
    {
        return name_.value();
    }

    Scope &SetName(const std::string &name)
    {
        name_ = name;
        return *this;
    }

    bool HasName() const
    {
        return name_.has_value();
    }

    Location *GetStartLocation() const
    {
        if (startLocation_) {
            return startLocation_->get();
        }
        return nullptr;
    }

    Scope &SetStartLocation(std::unique_ptr<Location> location)
    {
        startLocation_ = std::move(location);
        return *this;
    }

    bool HasStartLocation() const
    {
        return startLocation_.has_value();
    }

    Location *GetEndLocation() const
    {
        if (endLocation_) {
            return endLocation_->get();
        }
        return nullptr;
    }

    Scope &SetEndLocation(std::unique_ptr<Location> location)
    {
        endLocation_ = std::move(location);
        return *this;
    }

    bool HasEndLocation() const
    {
        return endLocation_.has_value();
    }

    struct Type {
        static bool Valid(const std::string &type)
        {
            return type == Global() || type == Local() || type == With() || type == Closure() || type == Catch() ||
                   type == Block() || type == Script() || type == Eval() || type == Module() ||
                   type == WasmExpressionStack();
        }
        static std::string Global()
        {
            return "global";
        }
        static std::string Local()
        {
            return "local";
        }
        static std::string With()
        {
            return "with";
        }
        static std::string Closure()
        {
            return "closure";
        }
        static std::string Catch()
        {
            return "catch";
        }
        static std::string Block()
        {
            return "block";
        }
        static std::string Script()
        {
            return "script";
        }
        static std::string Eval()
        {
            return "eval";
        }
        static std::string Module()
        {
            return "module";
        }
        static std::string WasmExpressionStack()
        {
            return "wasm-expression-stack";
        }
    };

private:
    NO_COPY_SEMANTIC(Scope);
    NO_MOVE_SEMANTIC(Scope);

    std::string type_ {};
    std::unique_ptr<RemoteObject> object_ {nullptr};
    std::optional<std::string> name_ {};
    std::optional<std::unique_ptr<Location>> startLocation_ {};
    std::optional<std::unique_ptr<Location>> endLocation_ {};
};

// Debugger.CallFrame
class CallFrame final : public PtBaseTypes {
public:
    CallFrame() = default;
    ~CallFrame() override = default;

    static std::unique_ptr<CallFrame> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    CallFrameId GetCallFrameId() const
    {
        return callFrameId_;
    }

    CallFrame &SetCallFrameId(CallFrameId callFrameId)
    {
        callFrameId_ = callFrameId;
        return *this;
    }

    const std::string &GetFunctionName() const
    {
        return functionName_;
    }

    CallFrame &SetFunctionName(const std::string &functionName)
    {
        functionName_ = functionName;
        return *this;
    }

    Location *GetFunctionLocation() const
    {
        if (functionLocation_) {
            return functionLocation_->get();
        }
        return nullptr;
    }

    CallFrame &SetFunctionLocation(std::unique_ptr<Location> location)
    {
        functionLocation_ = std::move(location);
        return *this;
    }

    bool HasFunctionLocation() const
    {
        return functionLocation_.has_value();
    }

    Location *GetLocation() const
    {
        return location_.get();
    }

    CallFrame &SetLocation(std::unique_ptr<Location> location)
    {
        location_ = std::move(location);
        return *this;
    }

    const std::string &GetUrl() const
    {
        return url_;
    }

    CallFrame &SetUrl(const std::string &url)
    {
        url_ = url;
        return *this;
    }

    const std::vector<std::unique_ptr<Scope>> *GetScopeChain() const
    {
        return &scopeChain_;
    }

    CallFrame &SetScopeChain(std::vector<std::unique_ptr<Scope>> scopeChain)
    {
        scopeChain_ = std::move(scopeChain);
        return *this;
    }
    RemoteObject *GetThis() const
    {
        return this_.get();
    }

    CallFrame &SetThis(std::unique_ptr<RemoteObject> thisObj)
    {
        this_ = std::move(thisObj);
        return *this;
    }

    RemoteObject *GetReturnValue() const
    {
        if (returnValue_) {
            return returnValue_->get();
        }
        return nullptr;
    }

    CallFrame &SetReturnValue(std::unique_ptr<RemoteObject> returnValue)
    {
        returnValue_ = std::move(returnValue);
        return *this;
    }

    bool HasReturnValue() const
    {
        return returnValue_.has_value();
    }

private:
    NO_COPY_SEMANTIC(CallFrame);
    NO_MOVE_SEMANTIC(CallFrame);

    CallFrameId callFrameId_ {};
    std::string functionName_ {};
    std::optional<std::unique_ptr<Location>> functionLocation_ {};
    std::unique_ptr<Location> location_ {nullptr};
    std::string url_ {};
    std::vector<std::unique_ptr<Scope>> scopeChain_ {};
    std::unique_ptr<RemoteObject> this_ {nullptr};
    std::optional<std::unique_ptr<RemoteObject>> returnValue_ {};
};

// ========== Heapprofiler types begin

using HeapSnapshotObjectId = uint32_t;

class SamplingHeapProfileSample  final :  public PtBaseTypes {
public:
    SamplingHeapProfileSample() = default;
    ~SamplingHeapProfileSample() override = default;
    static std::unique_ptr<SamplingHeapProfileSample> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    SamplingHeapProfileSample &SetSize(size_t size)
    {
        size_ = size;
        return *this;
    }
    
    int32_t GetSize() const
    {
        return size_;
    }

    SamplingHeapProfileSample &SetNodeId(int32_t nodeId)
    {
        nodeId_ = nodeId;
        return *this;
    }

    int32_t GetNodeId() const
    {
        return nodeId_;
    }

    SamplingHeapProfileSample &SetOrdinal(size_t ordinal)
    {
        ordinal_ = ordinal;
        return *this;
    }

    int32_t GetOrdinal() const
    {
        return ordinal_;
    }

private:
    NO_COPY_SEMANTIC(SamplingHeapProfileSample);
    NO_MOVE_SEMANTIC(SamplingHeapProfileSample);

    size_t size_ {};
    int32_t nodeId_ {};
    size_t ordinal_ {};
};

class RuntimeCallFrame  final :  public PtBaseTypes {
public:
    RuntimeCallFrame() = default;
    ~RuntimeCallFrame() override = default;
    static std::unique_ptr<RuntimeCallFrame> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    static std::unique_ptr<RuntimeCallFrame> FromFrameInfo(const FrameInfo &cpuFrameInfo);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    RuntimeCallFrame &SetFunctionName(const std::string &functionName)
    {
        functionName_ = functionName;
        return *this;
    }

    const std::string &GetFunctionName() const
    {
        return functionName_;
    }

    RuntimeCallFrame &SetScriptId(const std::string &scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    const std::string &GetScriptId() const
    {
        return scriptId_;
    }

    RuntimeCallFrame &SetUrl(const std::string &url)
    {
        url_ = url;
        return *this;
    }

    const std::string &GetUrl() const
    {
        return url_;
    }

    RuntimeCallFrame &SetLineNumber(int32_t lineNumber)
    {
        lineNumber_ = lineNumber;
        return *this;
    }

    int32_t GetLineNumber() const
    {
        return lineNumber_;
    }

    RuntimeCallFrame &SetColumnNumber(int32_t columnNumber)
    {
        columnNumber_ = columnNumber;
        return *this;
    }

    int32_t GetColumnNumber() const
    {
        return columnNumber_;
    }

private:
    NO_COPY_SEMANTIC(RuntimeCallFrame);
    NO_MOVE_SEMANTIC(RuntimeCallFrame);

    std::string functionName_ {};
    std::string scriptId_ {};
    std::string url_ {};
    int32_t lineNumber_ {};
    int32_t columnNumber_ {};
};

class SamplingHeapProfileNode  final :  public PtBaseTypes {
public:
    SamplingHeapProfileNode() = default;
    ~SamplingHeapProfileNode() override = default;
    static std::unique_ptr<SamplingHeapProfileNode> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    SamplingHeapProfileNode &SetCallFrame(std::unique_ptr<RuntimeCallFrame> callFrame)
    {
        callFrame_ = std::move(callFrame);
        return *this;
    }

    RuntimeCallFrame *GetCallFrame() const
    {
        return callFrame_.get();
    }

    SamplingHeapProfileNode &SetSelfSize(size_t selfSize)
    {
        selfSize_ = selfSize;
        return *this;
    }

    int32_t GetSelfSize() const
    {
        return selfSize_;
    }

    SamplingHeapProfileNode &SetId(int32_t id)
    {
        id_ = id;
        return *this;
    }

    int32_t GetId() const
    {
        return id_;
    }
    
    SamplingHeapProfileNode &SetChildren(std::vector<std::unique_ptr<SamplingHeapProfileNode>> children)
    {
        children_ = std::move(children);
        return *this;
    }

    const std::vector<std::unique_ptr<SamplingHeapProfileNode>> *GetChildren() const
    {
        return &children_;
    }

private:
    NO_COPY_SEMANTIC(SamplingHeapProfileNode);
    NO_MOVE_SEMANTIC(SamplingHeapProfileNode);

    std::unique_ptr<RuntimeCallFrame> callFrame_ {nullptr};
    size_t selfSize_ {};
    int32_t id_ {};
    std::vector<std::unique_ptr<SamplingHeapProfileNode>> children_ {};
};

class SamplingHeapProfile final : public PtBaseTypes {
public:
    SamplingHeapProfile() = default;
    ~SamplingHeapProfile() override = default;
    static std::unique_ptr<SamplingHeapProfile> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    SamplingHeapProfile &SetHead(std::unique_ptr<SamplingHeapProfileNode> head)
    {
        head_ = std::move(head);
        return *this;
    }

    SamplingHeapProfileNode *GetHead() const
    {
        return head_.get();
    }
    
    SamplingHeapProfile &SetSamples(std::vector<std::unique_ptr<SamplingHeapProfileSample>> samples)
    {
        samples_ = std::move(samples);
        return *this;
    }

    const std::vector<std::unique_ptr<SamplingHeapProfileSample>> *GetSamples() const
    {
        return &samples_;
    }

private:
    NO_COPY_SEMANTIC(SamplingHeapProfile);
    NO_MOVE_SEMANTIC(SamplingHeapProfile);

    std::unique_ptr<SamplingHeapProfileNode> head_ {nullptr};
    std::vector<std::unique_ptr<SamplingHeapProfileSample>> samples_ {};
};

// ========== Profiler types begin
// Profiler.PositionTickInfo
class PositionTickInfo final : public PtBaseTypes {
public:
    PositionTickInfo() = default;
    ~PositionTickInfo() override = default;

    static std::unique_ptr<PositionTickInfo> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;
    int32_t GetLine() const
    {
        return line_;
    }

    PositionTickInfo &SetLine(size_t line)
    {
        line_ = line;
        return *this;
    }

    int32_t GetTicks() const
    {
        return ticks_;
    }

    PositionTickInfo &SetTicks(size_t ticks)
    {
        ticks_ = ticks;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(PositionTickInfo);
    NO_MOVE_SEMANTIC(PositionTickInfo);
    size_t line_ {0};
    size_t ticks_ {0};
};

// Profiler.ProfileNode
class ProfileNode final : public PtBaseTypes {
public:
    ProfileNode() = default;
    ~ProfileNode() override = default;

    static std::unique_ptr<ProfileNode> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    static std::unique_ptr<ProfileNode> FromCpuProfileNode(const CpuProfileNode &cpuProfileNode);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;
    
    int32_t GetId() const
    {
        return id_;
    }

    ProfileNode &SetId(int32_t id)
    {
        id_ = id;
        return *this;
    }

    RuntimeCallFrame *GetCallFrame() const
    {
        return callFrame_.get();
    }

    ProfileNode &SetCallFrame(std::unique_ptr<RuntimeCallFrame> callFrame)
    {
        callFrame_ = std::move(callFrame);
        return *this;
    }

    int32_t GetHitCount() const
    {
        return hitCount_.value();
    }

    ProfileNode &SetHitCount(int32_t hitCount)
    {
        hitCount_ = hitCount;
        return *this;
    }

    bool HasHitCount() const
    {
        return hitCount_.has_value();
    }

    const std::vector<int32_t> *GetChildren() const
    {
        if (children_) {
            return &children_.value();
        }
        return nullptr;
    }

    ProfileNode &SetChildren(std::vector<int32_t> children)
    {
        children_ = std::move(children);
        return *this;
    }

    bool HasChildren() const
    {
        return children_.has_value();
    }

    const std::vector<std::unique_ptr<PositionTickInfo>> *GetPositionTicks() const
    {
        if (positionTicks_) {
            return &positionTicks_.value();
        }
        return nullptr;
    }

    ProfileNode &SetPositionTicks(std::vector<std::unique_ptr<PositionTickInfo>> positionTicks)
    {
        positionTicks_ = std::move(positionTicks);
        return *this;
    }

    bool HasPositionTicks() const
    {
        return positionTicks_.has_value();
    }

    const std::string &GetDeoptReason() const
    {
        return deoptReason_.value();
    }

    ProfileNode &SetDeoptReason(const std::string &deoptReason)
    {
        deoptReason_ = deoptReason;
        return *this;
    }

    bool HasDeoptReason() const
    {
        return deoptReason_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ProfileNode);
    NO_MOVE_SEMANTIC(ProfileNode);
    int32_t id_ {0};
    std::unique_ptr<RuntimeCallFrame> callFrame_ {nullptr};
    std::optional<int32_t> hitCount_ {0};
    std::optional<std::vector<int32_t>> children_ {};
    std::optional<std::vector<std::unique_ptr<PositionTickInfo>>> positionTicks_ {};
    std::optional<std::string> deoptReason_ {};
};

// Profiler.Profile
class Profile final : public PtBaseTypes {
public:
    Profile() = default;
    ~Profile() override = default;

    static std::unique_ptr<Profile> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    static std::unique_ptr<Profile> FromProfileInfo(const ProfileInfo &profileInfo);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    int64_t GetStartTime() const
    {
        return startTime_;
    }

    Profile &SetStartTime(int64_t startTime)
    {
        startTime_ = startTime;
        return *this;
    }

    int64_t GetEndTime() const
    {
        return endTime_;
    }

    Profile &SetEndTime(int64_t endTime)
    {
        endTime_ = endTime;
        return *this;
    }

    const std::vector<std::unique_ptr<ProfileNode>> *GetNodes() const
    {
        return &nodes_;
    }

    Profile &SetNodes(std::vector<std::unique_ptr<ProfileNode>> nodes)
    {
        nodes_ = std::move(nodes);
        return *this;
    }

    const std::vector<int32_t> *GetSamples() const
    {
        if (samples_) {
            return &samples_.value();
        }
        return nullptr;
    }

    Profile &SetSamples(std::vector<int32_t> samples)
    {
        samples_ = std::move(samples);
        return *this;
    }

    bool HasSamples() const
    {
        return samples_.has_value();
    }

    const std::vector<int32_t> *GetTimeDeltas() const
    {
        if (timeDeltas_) {
            return &timeDeltas_.value();
        }
        return nullptr;
    }

    Profile &SetTimeDeltas(std::vector<int32_t> timeDeltas)
    {
        timeDeltas_ = std::move(timeDeltas);
        return *this;
    }

    bool HasTimeDeltas() const
    {
        return timeDeltas_.has_value();
    }

private:
    NO_COPY_SEMANTIC(Profile);
    NO_MOVE_SEMANTIC(Profile);

    int64_t startTime_ {0};
    int64_t endTime_ {0};
    std::vector<std::unique_ptr<ProfileNode>> nodes_ {};
    std::optional<std::vector<int32_t>> samples_ {};
    std::optional<std::vector<int32_t>> timeDeltas_ {};
};

// Profiler.Coverage
class Coverage final : public PtBaseTypes {
public:
    Coverage() = default;
    ~Coverage() override = default;

    static std::unique_ptr<Coverage > Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    int32_t GetStartOffset() const
    {
        return startOffset_;
    }

    Coverage &SetStartOffset(size_t startOffset)
    {
        startOffset_ = startOffset;
        return *this;
    }

    int32_t GetEndOffset() const
    {
        return endOffset_;
    }

    Coverage &SetEndOffset(size_t endOffset)
    {
        endOffset_ = endOffset;
        return *this;
    }

    int32_t GetCount() const
    {
        return count_;
    }

    Coverage &SetCount(size_t count)
    {
        count_ = count;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(Coverage);
    NO_MOVE_SEMANTIC(Coverage);

    size_t startOffset_ {0};
    size_t endOffset_ {0};
    size_t count_ {0};
};

// Profiler.FunctionCoverage
class FunctionCoverage final : public PtBaseTypes {
public:
    FunctionCoverage() = default;
    ~FunctionCoverage() override = default;

    static std::unique_ptr<FunctionCoverage > Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    const std::string &GetFunctionName() const
    {
        return functionName_;
    }

    FunctionCoverage &SetFunctionName(const std::string &functionName)
    {
        functionName_ = functionName;
        return *this;
    }

    const std::vector<std::unique_ptr<Coverage>> *GetRanges() const
    {
        return &ranges_;
    }

    FunctionCoverage &SetFunctions(std::vector<std::unique_ptr<Coverage>> ranges)
    {
        ranges_ = std::move(ranges);
        return *this;
    }

    bool GetIsBlockCoverage() const
    {
        return isBlockCoverage_;
    }

    FunctionCoverage &SetisBlockCoverage(bool isBlockCoverage)
    {
        isBlockCoverage_ = isBlockCoverage;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(FunctionCoverage);
    NO_MOVE_SEMANTIC(FunctionCoverage);

    std::string functionName_ {};
    std::vector<std::unique_ptr<Coverage>> ranges_ {};
    bool isBlockCoverage_ {};
};

// Profiler.ScriptCoverage
// Profiler.GetBestEffortCoverage and Profiler.TakePreciseCoverage share this return value type
class ScriptCoverage final : public PtBaseTypes {
public:
    ScriptCoverage() = default;
    ~ScriptCoverage() override = default;

    static std::unique_ptr<ScriptCoverage> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;

    const std::string &GetScriptId() const
    {
        return scriptId_;
    }

    ScriptCoverage &SetScriptId(const std::string &scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    const std::string &GetUrl() const
    {
        return url_;
    }

    ScriptCoverage &SetUrl(const std::string &url)
    {
        url_ = url;
        return *this;
    }

    const std::vector<std::unique_ptr<FunctionCoverage>> *GetFunctions() const
    {
        return &functions_;
    }

    ScriptCoverage &SetFunctions(std::vector<std::unique_ptr<FunctionCoverage>> functions)
    {
        functions_ = std::move(functions);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(ScriptCoverage);
    NO_MOVE_SEMANTIC(ScriptCoverage);

    std::string scriptId_ {};
    std::string url_ {};
    std::vector<std::unique_ptr<FunctionCoverage>> functions_ {};
};

// Profiler.TypeObject
class TypeObject final : public PtBaseTypes {
public:
    TypeObject() = default;
    ~TypeObject() override = default;

    static std::unique_ptr<TypeObject> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;
    const std::string &GetName() const
    {
        return name_;
    }

    TypeObject &SetName(const std::string &name)
    {
        name_ = name;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(TypeObject);
    NO_MOVE_SEMANTIC(TypeObject);

    std::string name_ {};
};

// Profiler.TypeProfileEntry
class TypeProfileEntry final : public PtBaseTypes {
public:
    TypeProfileEntry() = default;
    ~TypeProfileEntry() override = default;

    static std::unique_ptr<TypeProfileEntry> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;
    int32_t GetOffset() const
    {
        return offset_;
    }

    TypeProfileEntry &SetOffset(size_t offset)
    {
        offset_ = offset;
        return *this;
    }

    const std::vector<std::unique_ptr<TypeObject>> *GetTypes() const
    {
        return &types_;
    }

    TypeProfileEntry &SetTypes(std::vector<std::unique_ptr<TypeObject>> types)
    {
        types_ = std::move(types);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(TypeProfileEntry);
    NO_MOVE_SEMANTIC(TypeProfileEntry);

    size_t offset_ {0};
    std::vector<std::unique_ptr<TypeObject>> types_ {};
};

// Profiler.ScriptTypeProfile
class ScriptTypeProfile final : public PtBaseTypes {
public:
    ScriptTypeProfile() = default;
    ~ScriptTypeProfile() override = default;

    static std::unique_ptr<ScriptTypeProfile> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) const override;
    const std::string &GetScriptId() const
    {
        return scriptId_;
    }

    ScriptTypeProfile &SetScriptId(const std::string &scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    const std::string &GetUrl() const
    {
        return url_;
    }

    ScriptTypeProfile &SetUrl(const std::string &url)
    {
        url_ = url;
        return *this;
    }

    const std::vector<std::unique_ptr<TypeProfileEntry>> *GetEntries() const
    {
        return &entries_;
    }

    ScriptTypeProfile &SetEntries(std::vector<std::unique_ptr<TypeProfileEntry>> entries)
    {
        entries_ = std::move(entries);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(ScriptTypeProfile);
    NO_MOVE_SEMANTIC(ScriptTypeProfile);

    std::string scriptId_ {};
    std::string url_ {};
    std::vector<std::unique_ptr<TypeProfileEntry>> entries_ {};
};
}  // namespace panda::ecmascript::tooling
#endif
