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

using JSTaggedType = uint64_t;

static const JSTaggedType NULL_POINTER = 0x05ULL;

static inline JSTaggedType ReinterpretDoubleToTaggedType(double value)
{
    return bit_cast<JSTaggedType>(value);
}
static inline double ReinterpretTaggedTypeToDouble(JSTaggedType value)
{
    return bit_cast<double>(value);
}

//  Every double with all of its exponent bits set and its highest mantissa bit set is a quiet NaN.
//  That leaves 51 bits unaccounted for. We’ll avoid one of those so that we don’t step on Intel’s
//  “QNaN Floating-Point Indefinite” value, leaving us 50 bits. Those remaining bits can be anything.
//  so we use a special quietNaN as TaggedInt tag(highest 16bits as 0xFFFF), and need to encode double
//  to the value will begin with a 16-bit pattern within the range 0x0001..0xFFFE.

//  Nan-boxing pointer is used and the first four bytes are used as tag:
//    Object:             [0x0000] [48 bit direct pointer]
//    WeakRef:            [0x0000] [47 bits direct pointer] | 1 bit 1
//                   /    [0x0001] [48 bit any value]
//    TaggedDouble:       ......
//                   \    [0xFFFE] [48 bit any value]
//    TaggedInt:          [0xFFFF] [0x0000] [32 bit signed integer]
//
//  There are some special markers of Object:
//    False:       [56 bits 0] | 0x06          // 0110
//    True:        [56 bits 0] | 0x07          // 0111
//    Undefined:   [56 bits 0] | 0x02          // 0010
//    Null:        [56 bits 0] | 0x03          // 0011
//    Hole:        [56 bits 0] | 0x05          // 0101

class JSTaggedValue {
public:
    static constexpr size_t TAG_BITS_SIZE = 16;  // 16 means bit numbers of 0xFFFF
    static constexpr size_t TAG_BITS_SHIFT = BitNumbers<JSTaggedType>() - TAG_BITS_SIZE;
    static_assert((TAG_BITS_SHIFT + TAG_BITS_SIZE) == sizeof(JSTaggedType) * CHAR_BIT, "Insufficient bits!");
    static constexpr JSTaggedType TAG_MARK = 0xFFFFULL << TAG_BITS_SHIFT;
    // int tag
    static constexpr JSTaggedType TAG_INT = TAG_MARK;
    // object tag
    static constexpr JSTaggedType TAG_OBJECT = 0x0000ULL << TAG_BITS_SHIFT;
    // weak object tag
    static constexpr JSTaggedType TAG_WEAK = TAG_OBJECT | 0x01ULL;
    // special tag
    static constexpr JSTaggedType TAG_NULL = 0x01ULL;
    static constexpr JSTaggedType TAG_SPECIAL = 0x02ULL;
    static constexpr JSTaggedType TAG_BOOLEAN = 0x04ULL;
    static constexpr JSTaggedType TAG_EXCEPTION = 0x08ULL;
    // tag mark
    static constexpr JSTaggedType TAG_SPECIAL_MARK = TAG_MARK | TAG_SPECIAL;
    static constexpr JSTaggedType TAG_BOOLEAN_MARK = TAG_SPECIAL | TAG_BOOLEAN;
    static constexpr JSTaggedType TAG_HEAPOBJECT_MARK = TAG_MARK | TAG_SPECIAL | TAG_BOOLEAN;
    static constexpr JSTaggedType TAG_WEAK_MARK = TAG_HEAPOBJECT_MARK | TAG_WEAK;
    // special value
    static constexpr JSTaggedType VALUE_HOLE = 0x05ULL;
    static constexpr JSTaggedType VALUE_NULL = TAG_OBJECT | TAG_SPECIAL | TAG_NULL;
    static constexpr JSTaggedType VALUE_FALSE = TAG_BOOLEAN_MARK | static_cast<JSTaggedType>(false);
    static constexpr JSTaggedType VALUE_TRUE = TAG_BOOLEAN_MARK | static_cast<JSTaggedType>(true);
    static constexpr JSTaggedType VALUE_UNDEFINED = TAG_SPECIAL;
    static constexpr JSTaggedType VALUE_EXCEPTION = TAG_SPECIAL | TAG_EXCEPTION;
    static constexpr JSTaggedType VALUE_ZERO = TAG_INT | 0x00ULL;

    static constexpr size_t DOUBLE_ENCODE_OFFSET_BIT = 48;
    static constexpr JSTaggedType DOUBLE_ENCODE_OFFSET = 1ULL << DOUBLE_ENCODE_OFFSET_BIT;

    static JSTaggedValue Cast(TaggedObject *object)
    {
        return JSTaggedValue(object);
    }

    static inline constexpr size_t TaggedTypeSize()
    {
        return sizeof(JSTaggedType);
    }

    static constexpr size_t SizeArch32 = sizeof(JSTaggedType);

    static constexpr size_t SizeArch64 = sizeof(JSTaggedType);

    explicit JSTaggedValue(void *) = delete;

    constexpr JSTaggedValue() : value_(NULL_POINTER) {}

    constexpr explicit JSTaggedValue(JSTaggedType v) : value_(v) {}

    constexpr explicit JSTaggedValue(int v) : value_(static_cast<JSTaggedType>(v) | TAG_INT) {}

    explicit JSTaggedValue(unsigned int v)
    {
        if (static_cast<int32_t>(v) < 0) {
            value_ = JSTaggedValue(static_cast<double>(v)).GetRawData();
            return;
        }
        value_ = JSTaggedValue(static_cast<int32_t>(v)).GetRawData();
    }

    constexpr explicit JSTaggedValue(bool v)
        : value_(static_cast<JSTaggedType>(v) | TAG_BOOLEAN_MARK) {}

    explicit JSTaggedValue(double v)
    {
        ASSERT_PRINT(!IsImpureNaN(v), "pureNaN will break the encoding of tagged double: "
                                          << std::hex << ReinterpretDoubleToTaggedType(v));
        value_ = ReinterpretDoubleToTaggedType(v) + DOUBLE_ENCODE_OFFSET;
    }

    explicit JSTaggedValue(const TaggedObject *v) : value_(static_cast<JSTaggedType>(ToUintPtr(v))) {}

    explicit JSTaggedValue(int64_t v)
    {
        if (UNLIKELY(static_cast<int32_t>(v) != v)) {
            value_ = JSTaggedValue(static_cast<double>(v)).GetRawData();
            return;
        }
        value_ = JSTaggedValue(static_cast<int32_t>(v)).GetRawData();
    }

    ~JSTaggedValue() = default;
    DEFAULT_COPY_SEMANTIC(JSTaggedValue);
    DEFAULT_MOVE_SEMANTIC(JSTaggedValue);

    inline void CreateWeakRef()
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "The least significant two bits of JSTaggedValue are not zero.");
        value_ = value_ | TAG_WEAK;
    }

    inline void RemoveWeakTag()
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == TAG_WEAK), "The tagged value is not a weak ref.");
        value_ = value_ & (~TAG_WEAK);
    }

    inline JSTaggedValue CreateAndGetWeakRef()
    {
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "The least significant two bits of JSTaggedValue are not zero.");
        return JSTaggedValue(value_ | TAG_WEAK);
    }

    inline bool IsWeak() const
    {
        return ((value_ & TAG_WEAK_MARK) == TAG_WEAK);
    }

    inline bool IsDouble() const
    {
        return !IsInt() && !IsObject();
    }

    inline bool IsInt() const
    {
        return (value_ & TAG_MARK) == TAG_INT;
    }

    inline bool IsSpecial() const
    {
        return ((value_ & TAG_SPECIAL_MARK) == TAG_SPECIAL) || IsHole();
    }

    inline bool IsObject() const
    {
        return ((value_ & TAG_MARK) == TAG_OBJECT);
    }

    inline TaggedObject *GetWeakReferentUnChecked() const
    {
        return reinterpret_cast<TaggedObject *>(value_ & (~TAG_WEAK));
    }

    inline bool IsHeapObject() const
    {
        return ((value_ & TAG_HEAPOBJECT_MARK) == 0U);
    }

    inline double GetDouble() const
    {
        ASSERT_PRINT(IsDouble(), "can not convert JSTaggedValue to Double : " << std::hex << value_);
        return ReinterpretTaggedTypeToDouble(value_ - DOUBLE_ENCODE_OFFSET);
    }

    inline int GetInt() const
    {
        ASSERT_PRINT(IsInt(), "can not convert JSTaggedValue to Int :" << std::hex << value_);
        return static_cast<int>(value_ & (~TAG_MARK));
    }

    inline JSTaggedType GetRawData() const
    {
        return value_;
    }

    //  This function returns the heap object pointer which may have the weak tag.
    inline TaggedObject *GetRawHeapObject() const
    {
        ASSERT_PRINT(IsHeapObject(), "can not convert JSTaggedValue to HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_);
    }

    inline TaggedObject *GetWeakReferent() const
    {
        ASSERT_PRINT(IsWeak(), "can not convert JSTaggedValue to WeakRef HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_ & (~TAG_WEAK));
    }

    static inline JSTaggedType Cast(void *ptr)
    {
        ASSERT_PRINT(sizeof(void *) == TaggedTypeSize(), "32bit platform is not support yet");
        return static_cast<JSTaggedType>(ToUintPtr(ptr));
    }

    inline bool IsFalse() const
    {
        return value_ == VALUE_FALSE;
    }

    inline bool IsTrue() const
    {
        return value_ == VALUE_TRUE;
    }

    inline bool IsUndefined() const
    {
        return value_ == VALUE_UNDEFINED;
    }

    inline bool IsNull() const
    {
        return value_ == VALUE_NULL;
    }

    inline bool IsUndefinedOrNull() const
    {
        return ((value_ & TAG_HEAPOBJECT_MARK) == TAG_SPECIAL);
    }

    inline bool IsHole() const
    {
        return value_ == VALUE_HOLE || value_ == 0U;
    }

    inline bool IsException() const
    {
        return value_ == VALUE_EXCEPTION;
    }

    static inline bool IsImpureNaN(double value)
    {
        // Tests if the double value would break tagged double encoding.
        return bit_cast<JSTaggedType>(value) >= (TAG_INT - DOUBLE_ENCODE_OFFSET);
    }

    inline bool operator==(const JSTaggedValue &other) const
    {
        return value_ == other.value_;
    }

    inline bool operator!=(const JSTaggedValue &other) const
    {
        return value_ != other.value_;
    }

    inline bool IsWeakForHeapObject() const
    {
        return (value_ & TAG_WEAK) == 1U;
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
        ASSERT_PRINT(IsHeapObject() && ((value_ & TAG_WEAK) == 0U),
                     "can not convert JSTaggedValue to HeapObject :" << std::hex << value_);
        return reinterpret_cast<TaggedObject *>(value_);
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
    static JSHandle<JSTaggedValue> ToNumeric(JSThread *thread, JSHandle<JSTaggedValue> tagged);
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
    static JSTaggedNumber StringToDouble(JSTaggedValue tagged);

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
    bool IsJSWeakRef() const;
    bool IsJSFinalizationRegistry() const;
    bool IsCellRecord() const;
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
    bool IsJSHClass() const;
    bool IsForinIterator() const;
    bool IsStringIterator() const;
    bool IsArrayBuffer() const;
    bool IsSharedArrayBuffer() const;

    bool IsJSSetIterator() const;
    bool IsJSRegExpIterator() const;
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
    bool IsJSPromiseAnyRejectElementFunction() const;
    bool IsJSPromiseAllSettledElementFunction() const;
    bool IsJSPromiseFinallyFunction() const;
    bool IsJSPromiseValueThunkOrThrowerFunction() const;
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
    bool IsJSListFormat() const;

    // non ECMA standard jsapis
    bool IsJSAPIArrayList() const;
    bool IsJSAPIArrayListIterator() const;
    bool IsJSAPILightWeightMap() const;
    bool IsJSAPILightWeightMapIterator() const;
    bool IsJSAPILightWeightSet() const;
    bool IsJSAPILightWeightSetIterator() const;
    bool IsJSAPITreeMap() const;
    bool IsJSAPITreeSet() const;
    bool IsJSAPITreeMapIterator() const;
    bool IsJSAPITreeSetIterator() const;
    bool IsJSAPIVector() const;
    bool IsJSAPIVectorIterator() const;
    bool IsJSAPIQueue() const;
    bool IsJSAPIQueueIterator() const;
    bool IsJSAPIPlainArray() const;
    bool IsJSAPIPlainArrayIterator() const;
    bool IsJSAPIDeque() const;
    bool IsJSAPIDequeIterator() const;
    bool IsJSAPIStack() const;
    bool IsJSAPIStackIterator() const;
    bool IsJSAPIList() const;
    bool IsJSAPILinkedList() const;
    bool IsJSAPIListIterator() const;
    bool IsJSAPILinkedListIterator() const;
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

    bool IsCjsExports() const;
    bool IsCjsModule() const;
    bool IsCjsRequire() const;
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
    JSTaggedType value_;

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
