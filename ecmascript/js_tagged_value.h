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

#ifndef ECMASCRIPT_JS_TAGGED_VALUE_H
#define ECMASCRIPT_JS_TAGGED_VALUE_H

#include "ecmascript/mem/c_string.h"
#include "include/coretypes/tagged_value.h"

namespace panda::ecmascript {
class JSObject;
class JSTaggedNumber;
template<typename T>
class JSHandle;
class TaggedArray;
class LinkedHashMap;
class LinkedHashSet;
class PropertyDescriptor;
class OperationResult;
class EcmaString;
class JSThread;

// Don't switch the order!
enum PreferredPrimitiveType : uint8_t { PREFER_NUMBER = 0, PREFER_STRING, NO_PREFERENCE };

// Result of an abstract relational comparison of x and y, implemented according
// to ES6 section 7.2.11 Abstract Relational Comparison.
enum class ComparisonResult {
    LESS,      // x < y
    EQUAL,     // x = y
    GREAT,     // x > y
    UNDEFINED  // at least one of x or y was undefined or NaN
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_TAGGED_VALUE_IF_ABRUPT(thread) RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());

using JSTaggedType = coretypes::TaggedType;

static inline JSTaggedType ReinterpretDoubleToTaggedType(double value)
{
    return bit_cast<JSTaggedType>(value);
}
static inline double ReinterpretTaggedTypeToDouble(JSTaggedType value)
{
    return bit_cast<double>(value);
}

class JSTaggedValue : public coretypes::TaggedValue {
public:
    static JSTaggedValue Cast(ObjectHeader *object)
    {
        return JSTaggedValue(object);
    }

    static constexpr size_t SizeArch32 = TaggedTypeSize();
    static constexpr size_t SizeArch64 = TaggedTypeSize();

    explicit JSTaggedValue(void *) = delete;

    constexpr JSTaggedValue() = default;
    constexpr explicit JSTaggedValue(coretypes::TaggedType v) : coretypes::TaggedValue(v) {}
    constexpr explicit JSTaggedValue(int v) : coretypes::TaggedValue(v) {}
    explicit JSTaggedValue(unsigned int v) : coretypes::TaggedValue(v) {}
    constexpr explicit JSTaggedValue(bool v) : coretypes::TaggedValue(v) {}
    explicit JSTaggedValue(double v) : coretypes::TaggedValue(v) {}
    explicit JSTaggedValue(const ObjectHeader *v) : coretypes::TaggedValue(v) {}
    explicit JSTaggedValue(const TaggedObject *v) : coretypes::TaggedValue(v) {}
    explicit JSTaggedValue(const coretypes::TaggedValue &other) : coretypes::TaggedValue(other.GetRawData()) {}
    explicit JSTaggedValue(int64_t v) : coretypes::TaggedValue(v){}

    ~JSTaggedValue() = default;
    DEFAULT_COPY_SEMANTIC(JSTaggedValue);
    DEFAULT_MOVE_SEMANTIC(JSTaggedValue);

    inline TaggedObject *GetWeakReferentUnChecked() const
    {
        return reinterpret_cast<TaggedObject *>(GetRawData() & (~TAG_WEAK_MASK));
    }

    inline bool IsWeakForHeapObject() const
    {
        return (GetRawData() & TAG_WEAK_MASK) == 1U;
    }

    static inline constexpr JSTaggedValue False()
    {
        return JSTaggedValue(VALUE_FALSE);
    }

    static inline constexpr JSTaggedValue True()
    {
        return JSTaggedValue(VALUE_TRUE);
    }

    static inline constexpr JSTaggedValue Undefined()
    {
        return JSTaggedValue(VALUE_UNDEFINED);
    }

    static inline constexpr JSTaggedValue Null()
    {
        return JSTaggedValue(VALUE_NULL);
    }

    static inline constexpr JSTaggedValue Hole()
    {
        return JSTaggedValue(VALUE_HOLE);
    }

    static inline constexpr JSTaggedValue Exception()
    {
        return JSTaggedValue(VALUE_EXCEPTION);
    }

    inline double GetNumber() const
    {
        return IsInt() ? GetInt() : GetDouble();
    }

    inline TaggedObject *GetTaggedObject() const
    {
        return reinterpret_cast<TaggedObject *>(GetHeapObject());
    }

    inline TaggedObject *GetRawTaggedObject() const
    {
        return reinterpret_cast<TaggedObject *>(GetRawHeapObject());
    }

    inline TaggedObject *GetTaggedWeakRef() const
    {
        return reinterpret_cast<TaggedObject *>(GetWeakReferent());
    }

    static JSTaggedValue OrdinaryToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                             PreferredPrimitiveType type = PREFER_NUMBER);

    // ecma6 7.1 Type Conversion
    static JSTaggedValue ToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                     PreferredPrimitiveType type = NO_PREFERENCE);
    bool ToBoolean() const;
    static JSTaggedNumber ToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigInt64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue ToBigUint64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToInteger(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<JSTaggedValue> ToNumeric(JSThread *thread, JSTaggedValue tagged);
    static int32_t ToInt32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint32_t ToUint32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static int16_t ToInt16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint16_t ToUint16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static int8_t ToInt8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint8_t ToUint8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static uint8_t ToUint8Clamp(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<EcmaString> ToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<JSObject> ToObject(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSHandle<JSTaggedValue> ToPropertyKey(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedValue CanonicalNumericIndexString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static JSTaggedNumber ToIndex(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);

    static bool ToArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged, uint32_t *output);
    static bool ToElementIndex(JSTaggedValue key, uint32_t *output);
    static bool StringToElementIndex(JSTaggedValue key, uint32_t *output);
    uint32_t GetArrayLength() const;

    // ecma6 7.2 Testing and Comparison Operations
    bool IsCallable() const;
    bool IsConstructor() const;
    bool IsExtensible(JSThread *thread) const;
    bool IsInteger() const;
    bool WithinInt32() const;
    bool IsZero() const;
    static bool IsPropertyKey(const JSHandle<JSTaggedValue> &key);
    static JSHandle<JSTaggedValue> RequireObjectCoercible(JSThread *thread, const JSHandle<JSTaggedValue> &tagged);
    static bool SameValue(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValue(const JSHandle<JSTaggedValue> &xHandle, const JSHandle<JSTaggedValue> &yHandle);
    static bool SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool Less(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool Equal(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool StrictEqual(const JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y);
    static bool SameValueNumberic(const JSTaggedValue &x, const JSTaggedValue &y);

    // ES6 7.4 Operations on Iterator Objects
    static JSObject *CreateIterResultObject(JSThread *thread, const JSHandle<JSTaggedValue> &value, bool done);

    // ecma6 7.3
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);
    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver,
                            bool mayThrow = false);
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &key);
    static bool DeletePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key);
    static bool DefinePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    static bool SetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                             const JSHandle<JSTaggedValue> &proto);
    static JSTaggedValue GetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool PreventExtensions(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static JSHandle<TaggedArray> GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key);
    static bool HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key);
    static bool HasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &key);
    static bool GlobalHasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key);

    // Type
    bool IsJSMap() const;
    bool IsJSSet() const;
    bool IsJSWeakMap() const;
    bool IsJSWeakSet() const;
    bool IsJSRegExp() const;
    bool IsNumber() const;
    bool IsBigInt() const;
    bool IsString() const;
    bool IsStringOrSymbol() const;
    bool IsTaggedArray() const;
    bool IsNativePointer() const;
    bool IsJSNativePointer() const;
    bool IsBoolean() const;
    bool IsSymbol() const;
    bool IsJSObject() const;
    bool IsJSGlobalObject() const;
    bool IsJSError() const;
    bool IsArray(JSThread *thread) const;
    bool IsJSArray() const;
    bool IsStableJSArray(JSThread *thread) const;
    bool IsStableJSArguments(JSThread *thread) const;
    bool HasStableElements(JSThread *thread) const;
    bool IsTypedArray() const;
    bool IsJSTypedArray() const;
    bool IsJSInt8Array() const;
    bool IsJSUint8Array() const;
    bool IsJSUint8ClampedArray() const;
    bool IsJSInt16Array() const;
    bool IsJSUint16Array() const;
    bool IsJSInt32Array() const;
    bool IsJSUint32Array() const;
    bool IsJSFloat32Array() const;
    bool IsJSFloat64Array() const;
    bool IsJSBigInt64Array() const;
    bool IsJSBigUint64Array() const;
    bool IsArguments() const;
    bool IsDate() const;
    bool IsBoundFunction() const;
    bool IsJSIntlBoundFunction() const;
    bool IsProxyRevocFunction() const;
    bool IsJSAsyncFunction() const;
    bool IsJSAsyncAwaitStatusFunction() const;
    bool IsClassConstructor() const;
    bool IsClassPrototype() const;
    bool IsJSFunction() const;
    bool IsJSFunctionBase() const;
    bool IsECMAObject() const;
    bool IsJSPrimitiveRef() const;
    bool IsJSPrimitive() const;
    bool IsAccessorData() const;
    bool IsInternalAccessor() const;
    bool IsAccessor() const;
    bool IsJSGlobalEnv() const;
    bool IsJSProxy() const;
    bool IsDynClass() const;
    bool IsJSHClass() const;
    bool IsForinIterator() const;
    bool IsStringIterator() const;
    bool IsArrayBuffer() const;
    bool IsSharedArrayBuffer() const;

    bool IsJSSetIterator() const;
    bool IsJSMapIterator() const;
    bool IsJSArrayIterator() const;
    bool IsIterator() const;
    bool IsGeneratorFunction() const;
    bool IsGeneratorObject() const;
    bool IsGeneratorContext() const;
    bool IsAsyncFuncObject() const;
    bool IsJSPromise() const;
    bool IsRecord() const;
    bool IsPromiseReaction() const;
    bool IsProgram() const;
    bool IsJSPromiseReactionFunction() const;
    bool IsJSPromiseExecutorFunction() const;
    bool IsJSPromiseAllResolveElementFunction() const;
    bool IsPromiseCapability() const;
    bool IsPromiseIteratorRecord() const;
    bool IsPromiseRecord() const;
    bool IsResolvingFunctionsRecord() const;
    bool IsCompletionRecord() const;
    bool IsDataView() const;
    bool IsTemplateMap() const;
    bool IsMicroJobQueue() const;
    bool IsPendingJob() const;
    bool IsJSLocale() const;
    bool IsJSDateTimeFormat() const;
    bool IsJSRelativeTimeFormat() const;
    bool IsJSIntl() const;
    bool IsJSNumberFormat() const;
    bool IsJSCollator() const;
    bool IsJSPluralRules() const;
    bool IsJSDisplayNames() const;

    // non ECMA standard jsapis
    bool IsJSAPIArrayList() const;
    bool IsJSAPIArrayListIterator() const;
    bool IsJSAPITreeMap() const;
    bool IsJSAPITreeSet() const;
    bool IsJSAPITreeMapIterator() const;
    bool IsJSAPITreeSetIterator() const;
    bool IsJSAPIQueue() const;
    bool IsJSAPIQueueIterator() const;
    bool IsJSAPIPlainArray() const;
    bool IsJSAPIPlainArrayIterator() const;
    bool IsJSAPIDeque() const;
    bool IsJSAPIDequeIterator() const;
    bool IsJSAPIStack() const;
    bool IsJSAPIStackIterator() const;
    bool IsSpecialContainer() const;

    bool IsPrototypeHandler() const;
    bool IsTransitionHandler() const;
    bool IsPropertyBox() const;
    bool IsProtoChangeMarker() const;
    bool IsProtoChangeDetails() const;
    bool IsMachineCodeObject() const;
    bool IsClassInfoExtractor() const;
    bool IsTSObjectType() const;
    bool IsTSClassType() const;
    bool IsTSUnionType() const;
    bool IsTSInterfaceType() const;
    bool IsTSClassInstanceType() const;
    bool IsTSImportType() const;
    bool IsTSFunctionType() const;
    bool IsTSArrayType() const;

    bool IsModuleRecord() const;
    bool IsSourceTextModule() const;
    bool IsImportEntry() const;
    bool IsExportEntry() const;
    bool IsResolvedBinding() const;
    bool IsModuleNamespace() const;
    static bool IsSameTypeOrHClass(JSTaggedValue x, JSTaggedValue y);

    static ComparisonResult Compare(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                    const JSHandle<JSTaggedValue> &y);
    static ComparisonResult StrictNumberCompare(double x, double y);
    static bool StrictNumberEquals(double x, double y);

    static JSHandle<JSTaggedValue> ToPrototypeOrObj(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    inline uint32_t GetKeyHashCode() const;
    static JSTaggedValue GetSuperBase(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    void DumpTaggedValue(std::ostream &os) const DUMP_API_ATTR;
    void Dump(std::ostream &os) const DUMP_API_ATTR;
    void D() const DUMP_API_ATTR;
    void DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec,
                         bool isVmMode = true) const;
    static void DV(JSTaggedType val) DUMP_API_ATTR;

private:
    inline double ExtractNumber() const;

    void DumpSpecialValue(std::ostream &os) const;
    void DumpHeapObjectType(std::ostream &os) const;

    // non ECMA standard jsapis
    static bool HasContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key);
    static JSHandle<TaggedArray> GetOwnContainerPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static bool GetContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);
    static OperationResult GetJSAPIProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                            const JSHandle<JSTaggedValue> &key);
};
STATIC_ASSERT_EQ_ARCH(sizeof(JSTaggedValue), JSTaggedValue::SizeArch32, JSTaggedValue::SizeArch64);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_TAGGED_VALUE_H