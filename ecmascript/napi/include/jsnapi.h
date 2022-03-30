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

#ifndef ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H
#define ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "ecmascript/base/config.h"
#include "ecmascript/common.h"
#include "libpandabase/macros.h"

namespace panda {
class JSNApiHelper;
class EscapeLocalScope;
class PromiseRejectInfo;
template<typename T>
class Global;
class JSNApi;
class PrimitiveRef;
class ArrayRef;
class StringRef;
class ObjectRef;
class FunctionRef;
class NumberRef;
class BooleanRef;
class NativePointerRef;
namespace test {
class JSNApiTests;
}  // namespace test

namespace ecmascript {
class EcmaVM;
class JSRuntimeOptions;
}  // namespace ecmascript

using Deleter = void (*)(void *nativePointer, void *data);
using EcmaVM = ecmascript::EcmaVM;
using JSTaggedType = uint64_t;

static constexpr uint32_t DEFAULT_GC_POOL_SIZE = 256 * 1024 * 1024;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ECMA_DISALLOW_COPY(className)      \
    className(const className &) = delete; \
    className &operator=(const className &) = delete

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ECMA_DISALLOW_MOVE(className) \
    className(className &&) = delete; \
    className &operator=(className &&) = delete

template<typename T>
class PUBLIC_API Local {  // NOLINT(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
public:
    inline Local() = default;

    template<typename S>
    inline Local(const Local<S> &current) : address_(reinterpret_cast<uintptr_t>(*current))
    {
        // Check
    }

    Local(const EcmaVM *vm, const Global<T> &current);

    ~Local() = default;

    inline T *operator*() const
    {
        return GetAddress();
    }

    inline T *operator->() const
    {
        return GetAddress();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == nullptr;
    }

    inline bool CheckException() const
    {
        return IsEmpty() || GetAddress()->IsException();
    }

    inline bool IsException() const
    {
        return !IsEmpty() && GetAddress()->IsException();
    }

    inline bool IsNull() const
    {
        return IsEmpty() || GetAddress()->IsHole();
    }

private:
    explicit inline Local(uintptr_t addr) : address_(addr) {}
    inline T *GetAddress() const
    {
        return reinterpret_cast<T *>(address_);
    };
    uintptr_t address_ = 0U;
    friend JSNApiHelper;
    friend EscapeLocalScope;
};

template<typename T>
class PUBLIC_API Global {  // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions
public:
    inline Global() = default;

    inline Global(const Global &that)
    {
        Update(that);
    }

    inline Global &operator=(const Global &that)
    {
        Update(that);
        return *this;
    }

    inline Global(Global &&that)
    {
        Update(that);
    }

    inline Global &operator=(Global &&that)
    {
        Update(that);
        return *this;
    }
    // Non-empty initial value.
    explicit Global(const EcmaVM *vm);

    template<typename S>
    Global(const EcmaVM *vm, const Local<S> &current);
    template<typename S>
    Global(const EcmaVM *vm, const Global<S> &current);
    ~Global() = default;

    Local<T> ToLocal(const EcmaVM *vm) const
    {
        return Local<T>(vm, *this);
    }

    void Empty()
    {
        address_ = 0;
    }

    // This method must be called before Global is released.
    void FreeGlobalHandleAddr();

    inline T *operator*() const
    {
        return GetAddress();
    }

    inline T *operator->() const
    {
        return GetAddress();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == nullptr;
    }

    inline bool CheckException() const
    {
        return IsEmpty() || GetAddress()->IsException();
    }

    void SetWeak();

    void ClearWeak();

    bool IsWeak() const;

private:
    inline T *GetAddress() const
    {
        return reinterpret_cast<T *>(address_);
    };
    inline void Update(const Global &that);
    uintptr_t address_ = 0U;
    const EcmaVM *vm_ {nullptr};
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class PUBLIC_API LocalScope {
public:
    explicit LocalScope(const EcmaVM *vm);
    virtual ~LocalScope();

protected:
    inline LocalScope(const EcmaVM *vm, JSTaggedType value);

private:
    void *prevNext_ = nullptr;
    void *prevEnd_ = nullptr;
    int prevHandleStorageIndex_ {-1};
    void *thread_ = nullptr;
};

class PUBLIC_API EscapeLocalScope final : public LocalScope {
public:
    explicit EscapeLocalScope(const EcmaVM *vm);
    ~EscapeLocalScope() override = default;

    ECMA_DISALLOW_COPY(EscapeLocalScope);
    ECMA_DISALLOW_MOVE(EscapeLocalScope);

    template<typename T>
    inline Local<T> Escape(Local<T> current)
    {
        ASSERT(!alreadyEscape_);
        alreadyEscape_ = true;
        *(reinterpret_cast<T *>(escapeHandle_)) = **current;
        return Local<T>(escapeHandle_);
    }

private:
    bool alreadyEscape_ = false;
    uintptr_t escapeHandle_ = 0U;
};

class PUBLIC_API JSExecutionScope {
public:
    explicit JSExecutionScope(const EcmaVM *vm);
    ~JSExecutionScope();
    ECMA_DISALLOW_COPY(JSExecutionScope);
    ECMA_DISALLOW_MOVE(JSExecutionScope);

private:
    void *last_current_thread_ = nullptr;
    bool is_revert_ = false;
};

class PUBLIC_API JSValueRef {
public:
    static Local<PrimitiveRef> Undefined(const EcmaVM *vm);
    static Local<PrimitiveRef> Null(const EcmaVM *vm);
    static Local<PrimitiveRef> True(const EcmaVM *vm);
    static Local<PrimitiveRef> False(const EcmaVM *vm);
    static Local<JSValueRef> Exception(const EcmaVM *vm);

    bool BooleaValue();
    int64_t IntegerValue(const EcmaVM *vm);
    uint32_t Uint32Value(const EcmaVM *vm);
    int32_t Int32Value(const EcmaVM *vm);

    Local<NumberRef> ToNumber(const EcmaVM *vm);
    Local<BooleanRef> ToBoolean(const EcmaVM *vm);
    Local<StringRef> ToString(const EcmaVM *vm);
    Local<ObjectRef> ToObject(const EcmaVM *vm);
    Local<NativePointerRef> ToNativePointer(const EcmaVM *vm);

    bool IsUndefined();
    bool IsNull();
    bool IsHole();
    bool IsTrue();
    bool IsFalse();
    bool IsNumber();
    bool IsBigInt();
    bool IsInt();
    bool WithinInt32();
    bool IsBoolean();
    bool IsString();
    bool IsSymbol();
    bool IsObject();
    bool IsArray(const EcmaVM *vm);
    bool IsConstructor();
    bool IsFunction();
    bool IsProxy();
    bool IsException();
    bool IsPromise();
    bool IsDataView();
    bool IsTypedArray();
    bool IsNativePointer();
    bool IsDate();
    bool IsError();
    bool IsMap();
    bool IsSet();
    bool IsWeakMap();
    bool IsWeakSet();
    bool IsRegExp();
    bool IsArrayIterator();
    bool IsStringIterator();
    bool IsSetIterator();
    bool IsMapIterator();
    bool IsArrayBuffer();
    bool IsUint8Array();
    bool IsInt8Array();
    bool IsUint8ClampedArray();
    bool IsInt16Array();
    bool IsUint16Array();
    bool IsInt32Array();
    bool IsUint32Array();
    bool IsFloat32Array();
    bool IsFloat64Array();
    bool IsBigInt64Array();
    bool IsBigUint64Array();
    bool IsJSPrimitiveRef();
    bool IsJSPrimitiveNumber();
    bool IsJSPrimitiveInt();
    bool IsJSPrimitiveBoolean();
    bool IsJSPrimitiveString();

    bool IsGeneratorObject();
    bool IsJSPrimitiveSymbol();

    bool IsArgumentsObject();
    bool IsGeneratorFunction();
    bool IsAsyncFunction();

    bool IsStrictEquals(const EcmaVM *vm, Local<JSValueRef> value);
    Local<StringRef> Typeof(const EcmaVM *vm);
    bool InstanceOf(const EcmaVM *vm, Local<JSValueRef> value);

private:
    JSTaggedType value_;
    friend JSNApi;
    template<typename T>
    friend class Global;
    template<typename T>
    friend class Local;
};

class PUBLIC_API PrimitiveRef : public JSValueRef {
};

class PUBLIC_API IntegerRef : public PrimitiveRef {
public:
    static Local<IntegerRef> New(const EcmaVM *vm, int input);
    static Local<IntegerRef> NewFromUnsigned(const EcmaVM *vm, unsigned int input);
    int Value();
};

class PUBLIC_API NumberRef : public PrimitiveRef {
public:
    static Local<NumberRef> New(const EcmaVM *vm, double input);
    double Value();
};

class PUBLIC_API BigIntRef : public PrimitiveRef {
public:
    static Local<BigIntRef> New(const EcmaVM *vm, uint64_t input);
    static Local<BigIntRef> New(const EcmaVM *vm, int64_t input);
    static Local<JSValueRef> CreateBigWords(const EcmaVM *vm, bool sign, uint32_t size, const uint64_t* words);
    void BigIntToInt64(const EcmaVM *vm, int64_t *cValue, bool *lossless);
    void BigIntToUint64(const EcmaVM *vm, uint64_t *cValue, bool *lossless);
    void GetWordsArray(bool* signBit, size_t wordCount, uint64_t* words);
    uint32_t GetWordsArraySize();
};

class PUBLIC_API BooleanRef : public PrimitiveRef {
public:
    static Local<BooleanRef> New(const EcmaVM *vm, bool input);
    bool Value();
};

class PUBLIC_API StringRef : public PrimitiveRef {
public:
    static inline StringRef *Cast(JSValueRef *value)
    {
        // check
        return static_cast<StringRef *>(value);
    }
    static Local<StringRef> NewFromUtf8(const EcmaVM *vm, const char *utf8, int length = -1);
    std::string ToString();
    int32_t Length();
    int32_t Utf8Length();
    int WriteUtf8(char *buffer, int length);
};

class PUBLIC_API SymbolRef : public PrimitiveRef {
public:
    static Local<SymbolRef> New(const EcmaVM *vm, Local<StringRef> description);
    Local<StringRef> GetDescription(const EcmaVM *vm);
};

using NativePointerCallback = void (*)(void* value, void* hint);
class PUBLIC_API NativePointerRef : public JSValueRef {
public:
    static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer);
    static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack,
                                       void *data);
    void *Value();
};

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class PUBLIC_API PropertyAttribute {
public:
    static PropertyAttribute Default()
    {
        return PropertyAttribute();
    }
    PropertyAttribute() = default;
    PropertyAttribute(Local<JSValueRef> value, bool w, bool e, bool c)
        : value_(value),
          writable_(w),
          enumerable_(e),
          configurable_(c),
          hasWritable_(true),
          hasEnumerable_(true),
          hasConfigurable_(true)
    {}
    ~PropertyAttribute() = default;

    bool IsWritable() const
    {
        return writable_;
    }
    void SetWritable(bool flag)
    {
        writable_ = flag;
        hasWritable_ = true;
    }
    bool IsEnumerable() const
    {
        return enumerable_;
    }
    void SetEnumerable(bool flag)
    {
        enumerable_ = flag;
        hasEnumerable_ = true;
    }
    bool IsConfigurable() const
    {
        return configurable_;
    }
    void SetConfigurable(bool flag)
    {
        configurable_ = flag;
        hasConfigurable_ = true;
    }
    bool HasWritable() const
    {
        return hasWritable_;
    }
    bool HasConfigurable() const
    {
        return hasConfigurable_;
    }
    bool HasEnumerable() const
    {
        return hasEnumerable_;
    }
    Local<JSValueRef> GetValue(const EcmaVM *vm) const
    {
        if (value_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return value_;
    }
    void SetValue(Local<JSValueRef> value)
    {
        value_ = value;
    }
    inline bool HasValue() const
    {
        return !value_.IsEmpty();
    }
    Local<JSValueRef> GetGetter(const EcmaVM *vm) const
    {
        if (getter_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return getter_;
    }
    void SetGetter(Local<JSValueRef> value)
    {
        getter_ = value;
    }
    bool HasGetter() const
    {
        return !getter_.IsEmpty();
    }
    Local<JSValueRef> GetSetter(const EcmaVM *vm) const
    {
        if (setter_.IsEmpty()) {
            return JSValueRef::Undefined(vm);
        }
        return setter_;
    }
    void SetSetter(Local<JSValueRef> value)
    {
        setter_ = value;
    }
    bool HasSetter() const
    {
        return !setter_.IsEmpty();
    }

private:
    Local<JSValueRef> value_;
    Local<JSValueRef> getter_;
    Local<JSValueRef> setter_;
    bool writable_ = false;
    bool enumerable_ = false;
    bool configurable_ = false;
    bool hasWritable_ = false;
    bool hasEnumerable_ = false;
    bool hasConfigurable_ = false;
};

class PUBLIC_API ObjectRef : public JSValueRef {
public:
    static inline ObjectRef *Cast(JSValueRef *value)
    {
        // check
        return static_cast<ObjectRef *>(value);
    }
    static Local<ObjectRef> New(const EcmaVM *vm);
    bool Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);
    bool Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value);
    bool SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter,
                             Local<FunctionRef> setter, PropertyAttribute attribute = PropertyAttribute::Default());
    Local<JSValueRef> Get(const EcmaVM *vm, Local<JSValueRef> key);
    Local<JSValueRef> Get(const EcmaVM *vm, int32_t key);

    bool GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property);
    Local<ArrayRef> GetOwnPropertyNames(const EcmaVM *vm);
    Local<ArrayRef> GetOwnEnumerablePropertyNames(const EcmaVM *vm);
    Local<JSValueRef> GetPrototype(const EcmaVM *vm);

    bool DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute);

    bool Has(const EcmaVM *vm, Local<JSValueRef> key);
    bool Has(const EcmaVM *vm, uint32_t key);

    bool Delete(const EcmaVM *vm, Local<JSValueRef> key);
    bool Delete(const EcmaVM *vm, uint32_t key);

    void SetNativePointerFieldCount(int32_t count);
    int32_t GetNativePointerFieldCount();
    void *GetNativePointerField(int32_t index);
    void SetNativePointerField(int32_t index,
                               void *nativePointer = nullptr,
                               NativePointerCallback callBack = nullptr,
                               void *data = nullptr);
};

using FunctionCallback = Local<JSValueRef> (*)(EcmaVM *, Local<JSValueRef>,
                                               const Local<JSValueRef>[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
                                               int32_t, void *);
using FunctionCallbackWithNewTarget =
    Local<JSValueRef> (*)(EcmaVM *, Local<JSValueRef>, Local<JSValueRef>, const Local<JSValueRef>[], int32_t, void *);
class PUBLIC_API FunctionRef : public ObjectRef {
public:
    static Local<FunctionRef> New(EcmaVM *vm, FunctionCallback nativeFunc, void *data);
    static Local<FunctionRef> New(EcmaVM *vm, FunctionCallback nativeFunc, Deleter deleter, void *data);
    static Local<FunctionRef> NewWithProperty(EcmaVM *vm, FunctionCallback nativeFunc, void *data);
    static Local<FunctionRef> NewClassFunction(EcmaVM *vm, FunctionCallbackWithNewTarget nativeFunc, Deleter deleter,
        void *data);
    Local<JSValueRef> Call(const EcmaVM *vm, Local<JSValueRef> thisObj, const Local<JSValueRef> argv[],
        int32_t length);
    Local<JSValueRef> Constructor(const EcmaVM *vm, const Local<JSValueRef> argv[], int32_t length);
    Local<JSValueRef> GetFunctionPrototype(const EcmaVM *vm);
    // Inherit Prototype from parent function
    // set this.Prototype.__proto__ to parent.Prototype, set this.__proto__ to parent function
    bool Inherit(const EcmaVM *vm, Local<FunctionRef> parent);
    void SetName(const EcmaVM *vm, Local<StringRef> name);
    Local<StringRef> GetName(const EcmaVM *vm);
    bool IsNative(const EcmaVM *vm);
};

class PUBLIC_API ArrayRef : public ObjectRef {
public:
    static Local<ArrayRef> New(const EcmaVM *vm, int32_t length = 0);
    int32_t Length(const EcmaVM *vm);
    static bool SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value);
    static Local<JSValueRef> GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index);
};

class PUBLIC_API PromiseRef : public ObjectRef {
public:
    Local<PromiseRef> Catch(const EcmaVM *vm, Local<FunctionRef> handler);
    Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> handler);
    Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected);
};

class PUBLIC_API PromiseCapabilityRef : public ObjectRef {
public:
    static Local<PromiseCapabilityRef> New(const EcmaVM *vm);
    bool Resolve(const EcmaVM *vm, Local<JSValueRef> value);
    bool Reject(const EcmaVM *vm, Local<JSValueRef> reason);
    Local<PromiseRef> GetPromise(const EcmaVM *vm);
};

class PUBLIC_API ArrayBufferRef : public ObjectRef {
public:
    static Local<ArrayBufferRef> New(const EcmaVM *vm, int32_t length);
    static Local<ArrayBufferRef> New(const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter,
                                     void *data);

    int32_t ByteLength(const EcmaVM *vm);
    void *GetBuffer();
};

class PUBLIC_API DataViewRef : public ObjectRef {
public:
    static Local<DataViewRef> New(const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, uint32_t byteOffset,
                                  uint32_t byteLength);
    uint32_t ByteLength();
    uint32_t ByteOffset();
    Local<ArrayBufferRef> GetArrayBuffer(const EcmaVM *vm);
};

class PUBLIC_API TypedArrayRef : public ObjectRef {
public:
    int32_t ByteLength(const EcmaVM *vm);
    int32_t ByteOffset(const EcmaVM *vm);
    int32_t ArrayLength(const EcmaVM *vm);
    Local<ArrayBufferRef> GetArrayBuffer(const EcmaVM *vm);
};

class PUBLIC_API Int8ArrayRef : public TypedArrayRef {
public:
    static Local<Int8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint8ArrayRef : public TypedArrayRef {
public:
    static Local<Uint8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint8ClampedArrayRef : public TypedArrayRef {
public:
    static Local<Uint8ClampedArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                           int32_t length);
};

class PUBLIC_API Int16ArrayRef : public TypedArrayRef {
public:
    static Local<Int16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint16ArrayRef : public TypedArrayRef {
public:
    static Local<Uint16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                     int32_t length);
};

class PUBLIC_API Int32ArrayRef : public TypedArrayRef {
public:
    static Local<Int32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);
};

class PUBLIC_API Uint32ArrayRef : public TypedArrayRef {
public:
    static Local<Uint32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                     int32_t length);
};

class PUBLIC_API Float32ArrayRef : public TypedArrayRef {
public:
    static Local<Float32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API Float64ArrayRef : public TypedArrayRef {
public:
    static Local<Float64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API BigInt64ArrayRef : public TypedArrayRef {
public:
    static Local<BigInt64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API BigUint64ArrayRef : public TypedArrayRef {
public:
    static Local<BigUint64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,
                                      int32_t length);
};

class PUBLIC_API RegExpRef : public ObjectRef {
public:
    Local<StringRef> GetOriginalSource(const EcmaVM *vm);
};

class PUBLIC_API DateRef : public ObjectRef {
public:
    static Local<DateRef> New(const EcmaVM *vm, double time);
    Local<StringRef> ToString(const EcmaVM *vm);
    double GetTime();
};

class PUBLIC_API MapRef : public ObjectRef {
public:
    int32_t GetSize();
};

class PUBLIC_API SetRef : public ObjectRef {
public:
    int32_t GetSize();
};

class PUBLIC_API JSON {
public:
    static Local<JSValueRef> Parse(const EcmaVM *vm, Local<StringRef> string);
    static Local<JSValueRef> Stringify(const EcmaVM *vm, Local<JSValueRef> json);
};

class PUBLIC_API Exception {
public:
    static Local<JSValueRef> Error(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> RangeError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> ReferenceError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> SyntaxError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> TypeError(const EcmaVM *vm, Local<StringRef> message);
    static Local<JSValueRef> EvalError(const EcmaVM *vm, Local<StringRef> message);
};

using LOG_PRINT = int (*)(int id, int level, const char *tag, const char *fmt, const char *message);

class PUBLIC_API RuntimeOption {
public:
    enum class PUBLIC_API GC_TYPE : uint8_t { EPSILON, GEN_GC, STW };
    enum class PUBLIC_API LOG_LEVEL : uint8_t {
        DEBUG = 3,
        INFO = 4,
        WARN = 5,
        ERROR = 6,
        FATAL = 7,
    };

    void SetGcType(GC_TYPE type)
    {
        gcType_ = type;
    }

    void SetGcPoolSize(uint32_t size)
    {
        gcPoolSize_ = size;
    }

    void SetLogLevel(LOG_LEVEL logLevel)
    {
        logLevel_ = logLevel;
    }

    void SetLogBufPrint(LOG_PRINT out)
    {
        logBufPrint_ = out;
    }

    void SetDebuggerLibraryPath(const std::string &path)
    {
        debuggerLibraryPath_ = path;
    }

    void SetEnableArkTools(bool value) {
        enableArkTools_ = value;
    }

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    void SetEnableCpuprofiler(bool value) {
        enableCpuprofiler_ = value;
    }
#endif

    void SetArkProperties(int prop) {
        arkProperties_ = prop;
    }

    void SetAsmInterOption(const std::string &value)
    {
        asmInterOption_ = value;
    }

private:
    std::string GetGcType() const
    {
        std::string gcType;
        switch (gcType_) {
            case GC_TYPE::GEN_GC:
                gcType = "gen-gc";
                break;
            case GC_TYPE::STW:
                gcType = "stw";
                break;
            case GC_TYPE::EPSILON:
                gcType = "epsilon";
                break;
            default:
                UNREACHABLE();
        }
        return gcType;
    }

    std::string GetLogLevel() const
    {
        std::string logLevel;
        switch (logLevel_) {
            case LOG_LEVEL::INFO:
            case LOG_LEVEL::WARN:
                logLevel = "info";
                break;
            case LOG_LEVEL::ERROR:
                logLevel = "error";
                break;
            case LOG_LEVEL::FATAL:
                logLevel = "fatal";
                break;
            case LOG_LEVEL::DEBUG:
            default:
                logLevel = "debug";
                break;
        }

        return logLevel;
    }

    uint32_t GetGcPoolSize() const
    {
        return gcPoolSize_;
    }

    LOG_PRINT GetLogBufPrint() const
    {
        return logBufPrint_;
    }

    std::string GetDebuggerLibraryPath() const
    {
        return debuggerLibraryPath_;
    }

    bool GetEnableArkTools() const
    {
        return enableArkTools_;
    }

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    bool GetEnableCpuprofiler() const
    {
        return enableCpuprofiler_;
    }
#endif

    int GetArkProperties() const
    {
        return arkProperties_;
    }

    std::string GetAsmInterOption() const
    {
        return asmInterOption_;
    }

    GC_TYPE gcType_ = GC_TYPE::EPSILON;
    LOG_LEVEL logLevel_ = LOG_LEVEL::DEBUG;
    uint32_t gcPoolSize_ = DEFAULT_GC_POOL_SIZE;
    LOG_PRINT logBufPrint_ {nullptr};
    std::string debuggerLibraryPath_ {};
    bool enableArkTools_ {false};
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    bool enableCpuprofiler_ {false};
#endif
    int arkProperties_ {-1};
    std::string asmInterOption_ {""};
    friend JSNApi;
};

class PUBLIC_API PromiseRejectInfo {
public:
    enum class PUBLIC_API PROMISE_REJECTION_EVENT : uint32_t { REJECT = 0, HANDLE };
    PromiseRejectInfo(Local<JSValueRef> promise, Local<JSValueRef> reason,
                      PromiseRejectInfo::PROMISE_REJECTION_EVENT operation, void* data);
    ~PromiseRejectInfo() {}
    Local<JSValueRef> GetPromise() const;
    Local<JSValueRef> GetReason() const;
    PromiseRejectInfo::PROMISE_REJECTION_EVENT GetOperation() const;
    void* GetData() const;

private:
    Local<JSValueRef> promise_ {};
    Local<JSValueRef> reason_ {};
    PROMISE_REJECTION_EVENT operation_ = PROMISE_REJECTION_EVENT::REJECT;
    void* data_ {nullptr};
};

class PUBLIC_API JSNApi {
public:
    // JSVM
    enum class PUBLIC_API TRIGGER_GC_TYPE : uint8_t { SEMI_GC, OLD_GC, FULL_GC };
    static EcmaVM *CreateJSVM(const RuntimeOption &option);
    static void DestroyJSVM(EcmaVM *ecmaVm);

    // JS code
    static bool Execute(EcmaVM *vm, const std::string &fileName, const std::string &entry);
    static bool Execute(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &entry,
                        const std::string &filename = "");
    static bool ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file);
    static Local<ObjectRef> GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key);

    // ObjectRef Operation
    static Local<ObjectRef> GetGlobalObject(const EcmaVM *vm);
    static void ExecutePendingJob(const EcmaVM *vm);

    // Memory
    static void TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);
    // Exception
    static void ThrowException(const EcmaVM *vm, Local<JSValueRef> error);
    static Local<ObjectRef> GetAndClearUncaughtException(const EcmaVM *vm);
    static Local<ObjectRef> GetUncaughtException(const EcmaVM *vm);
    static void EnableUserUncaughtErrorHandler(EcmaVM *vm);

    static bool StartDebugger(const char *library_path, EcmaVM *vm, bool isDebugMode);
    static bool StopDebugger(const char *library_path);
    // Serialize & Deserialize.
    static void* SerializeValue(const EcmaVM *vm, Local<JSValueRef> data, Local<JSValueRef> transfer);
    static Local<JSValueRef> DeserializeValue(const EcmaVM *vm, void* recoder);
    static void DeleteSerializationData(void *data);
    static void SetOptions(const ecmascript::JSRuntimeOptions &options);
    static void SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void* data);
    static void SetHostEnqueueJob(const EcmaVM* vm, Local<JSValueRef> cb);

private:
    static int vmCount;
    static bool CreateRuntime(const RuntimeOption &option);
    static bool DestroyRuntime();

    static uintptr_t GetHandleAddr(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t GetGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t SetWeak(const EcmaVM *vm, uintptr_t localAddress);
    static uintptr_t ClearWeak(const EcmaVM *vm, uintptr_t localAddress);
    static bool IsWeak(const EcmaVM *vm, uintptr_t localAddress);
    static void DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr);
    template<typename T>
    friend class Global;
    template<typename T>
    friend class Local;
    friend class test::JSNApiTests;
};


template<typename T>
template<typename S>
Global<T>::Global(const EcmaVM *vm, const Local<S> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
template<typename S>
Global<T>::Global(const EcmaVM *vm, const Global<S> &current) : vm_(vm)
{
    if (!current.IsEmpty()) {
        address_ = JSNApi::GetGlobalHandleAddr(vm_, reinterpret_cast<uintptr_t>(*current));
    }
}

template<typename T>
void Global<T>::Update(const Global &that)
{
    if (address_ != 0) {
        JSNApi::DisposeGlobalHandleAddr(vm_, address_);
    }
    address_ = that.address_;
    vm_ = that.vm_;
}

template<typename T>
void Global<T>::FreeGlobalHandleAddr()
{
    if (address_ == 0) {
        return;
    }
    JSNApi::DisposeGlobalHandleAddr(vm_, address_);
    address_ = 0;
}

template<typename T>
void Global<T>::SetWeak()
{
    address_ = JSNApi::SetWeak(vm_, address_);
}

template<typename T>
void Global<T>::ClearWeak()
{
    address_ = JSNApi::ClearWeak(vm_, address_);
}

template<typename T>
bool Global<T>::IsWeak() const
{
    return JSNApi::IsWeak(vm_, address_);
}

// ---------------------------------- Local --------------------------------------------
template<typename T>
Local<T>::Local(const EcmaVM *vm, const Global<T> &current)
{
    address_ = JSNApi::GetHandleAddr(vm, reinterpret_cast<uintptr_t>(*current));
}
}  // namespace panda
#endif  // ECMASCRIPT_NAPI_INCLUDE_JSNAPI_H
