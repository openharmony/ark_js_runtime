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

#ifndef ECMASCRIPT_TAGGED_VALUE_INL_H
#define ECMASCRIPT_TAGGED_VALUE_INL_H

#include "ecmascript/js_tagged_value.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/tagged_object-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
// ecma6 7.1 Type Conversion
static constexpr double SAFE_NUMBER = 9007199254740991LL;
static constexpr uint32_t MAX_INDEX_LEN = 10;

inline bool JSTaggedValue::ToBoolean() const
{
    if (IsInt()) {
        return GetInt() != 0;
    }
    if (IsDouble()) {
        double d = GetDouble();
        return !std::isnan(d) && d != 0;
    }

    switch (GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED:
            [[fallthrough]];
        case JSTaggedValue::VALUE_HOLE:
            [[fallthrough]];
        case JSTaggedValue::VALUE_NULL:
            [[fallthrough]];
        case JSTaggedValue::VALUE_FALSE: {
            return false;
        }
        case JSTaggedValue::VALUE_TRUE: {
            return true;
        }
        default: {
            break;
        }
    }

    if (IsHeapObject()) {
        TaggedObject *obj = GetTaggedObject();
        if (IsString()) {
            auto str = static_cast<EcmaString *>(obj);
            return str->GetLength() != 0;
        }

        return true;
    }

    UNREACHABLE();
}

inline JSTaggedNumber JSTaggedValue::ToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsInt() || tagged->IsDouble()) {
        return JSTaggedNumber(tagged.GetTaggedValue());
    }

    switch (tagged->GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED:
        case JSTaggedValue::VALUE_HOLE: {
            return JSTaggedNumber(base::NAN_VALUE);
        }
        case JSTaggedValue::VALUE_TRUE: {
            return JSTaggedNumber(1);
        }
        case JSTaggedValue::VALUE_FALSE:
        case JSTaggedValue::VALUE_NULL: {
            return JSTaggedNumber(0);
        }
        default: {
            break;
        }
    }

    if (tagged->IsString()) {
        Span<const uint8_t> str;
        auto strObj = static_cast<EcmaString *>(tagged->GetTaggedObject());
        size_t strLen = strObj->GetLength();
        if (strLen == 0) {
            return JSTaggedNumber(0);
        }
        [[maybe_unused]] CVector<uint8_t> buf;  // Span will use buf.data(), shouldn't define inside 'if'
        if (UNLIKELY(strObj->IsUtf16())) {
            size_t len = base::utf_helper::Utf16ToUtf8Size(strObj->GetDataUtf16(), strLen) - 1;
            buf.reserve(len);
            len = base::utf_helper::ConvertRegionUtf16ToUtf8(strObj->GetDataUtf16(), buf.data(), strLen, len, 0);
            str = Span<const uint8_t>(buf.data(), len);
        } else {
            str = Span<const uint8_t>(strObj->GetDataUtf8(), strLen);
        }
        double d = base::NumberHelper::StringToDouble(str.begin(), str.end(), 0,
                                                      base::ALLOW_BINARY + base::ALLOW_OCTAL + base::ALLOW_HEX);
        return JSTaggedNumber(d);
    }
    if (tagged->IsECMAObject()) {
        JSHandle<JSTaggedValue> primValue(thread, ToPrimitive(thread, tagged, PREFER_NUMBER));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
        return ToNumber(thread, primValue);
    }
    if (tagged->IsSymbol()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Symbol value to a number", JSTaggedNumber::Exception());
    }

    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Unknown value to a number", JSTaggedNumber::Exception());
}

inline JSTaggedNumber JSTaggedValue::ToInteger(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());

    return JSTaggedNumber(base::NumberHelper::TruncateDouble(number.GetNumber()));
}

inline int32_t JSTaggedValue::ToInt32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT32_BITS);
}

inline uint32_t JSTaggedValue::ToUint32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt32(thread, tagged);
}

inline int16_t JSTaggedValue::ToInt16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT16_BITS);
}

inline uint16_t JSTaggedValue::ToUint16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt16(thread, tagged);
}

inline int8_t JSTaggedValue::ToInt8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT8_BITS);
}

inline uint8_t JSTaggedValue::ToUint8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt8(thread, tagged);
}

inline uint8_t JSTaggedValue::ToUint8Clamp(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    double d = number.GetNumber();
    if (std::isnan(d) || d <= 0) {
        return 0;
    }
    if (d >= UINT8_MAX) {
        return UINT8_MAX;
    }

    return lrint(d);
}

inline JSTaggedNumber JSTaggedValue::ToLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber len = ToInteger(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
    if (len.GetNumber() < 0.0) {
        return JSTaggedNumber(static_cast<double>(0));
    }
    if (len.GetNumber() > SAFE_NUMBER) {
        return JSTaggedNumber(static_cast<double>(SAFE_NUMBER));
    }
    return len;
}

// ecma6 7.2 Testing and Comparison Operations
inline JSHandle<JSTaggedValue> JSTaggedValue::RequireObjectCoercible(JSThread *thread,
                                                                     const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsUndefined() || tagged->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "RequireObjectCoercible",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    return tagged;
}

inline bool JSTaggedValue::IsCallable() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsCallable();
}

inline bool JSTaggedValue::IsConstructor() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsConstructor();
}

inline bool JSTaggedValue::IsExtensible(JSThread *thread) const
{
    if (UNLIKELY(IsJSProxy())) {
        return JSProxy::IsExtensible(thread, JSHandle<JSProxy>(thread, *this));
    }

    return IsHeapObject() && GetTaggedObject()->GetClass()->IsExtensible();
}

inline bool JSTaggedValue::IsClassConstructor() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsClassConstructor();
}

inline bool JSTaggedValue::IsClassPrototype() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsClassPrototype();
}

inline bool JSTaggedValue::IsPropertyKey(const JSHandle<JSTaggedValue> &key)
{
    return key->IsStringOrSymbol() || key->IsNumber();
}

inline bool JSTaggedValue::SameValue(const JSTaggedValue &x, const JSTaggedValue &y)
{
    // same object or special type must be same value
    if (x == y) {
        return true;
    }

    if (x.IsNumber() && y.IsNumber()) {
        return SameValueNumberic(x, y);
    }

    if (x.IsString() && y.IsString()) {
        return EcmaString::StringsAreEqual(static_cast<EcmaString *>(x.GetTaggedObject()),
                                           static_cast<EcmaString *>(y.GetTaggedObject()));
    }
    return false;
}

inline bool JSTaggedValue::SameValue(const JSHandle<JSTaggedValue> &xHandle, const JSHandle<JSTaggedValue> &yHandle)
{
    return SameValue(xHandle.GetTaggedValue(), yHandle.GetTaggedValue());
}

inline bool JSTaggedValue::SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y)
{
    if (x == y) {
        return true;
    }

    if (x.IsNumber() && y.IsNumber()) {
        double xValue = x.ExtractNumber();
        double yValue = y.ExtractNumber();
        // Compare xValue with yValue to deal with -0.0
        return (xValue == yValue) || (std::isnan(xValue) && std::isnan(yValue));
    }

    if (x.IsString() && y.IsString()) {
        return EcmaString::StringsAreEqual(static_cast<EcmaString *>(x.GetTaggedObject()),
                                           static_cast<EcmaString *>(y.GetTaggedObject()));
    }
    return false;
}

inline bool JSTaggedValue::SameValueNumberic(const JSTaggedValue &x, const JSTaggedValue &y)
{
    double xValue = x.ExtractNumber();
    double yValue = y.ExtractNumber();
    // SameNumberValue(NaN, NaN) is true.
    if (xValue != yValue) {
        return std::isnan(xValue) && std::isnan(yValue);
    }
    // SameNumberValue(0.0, -0.0) is false.
    return (std::signbit(xValue) == std::signbit(yValue));
}

inline bool JSTaggedValue::Less(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y)
{
    ComparisonResult result = Compare(thread, x, y);
    return result == ComparisonResult::LESS;
}

inline bool JSTaggedValue::StrictNumberEquals(double x, double y)
{
    // Must check explicitly for NaN's on Windows, but -0 works fine.
    if (std::isnan(x) || std::isnan(y)) {
        return false;
    }
    return x == y;
}

inline bool JSTaggedValue::StrictEqual([[maybe_unused]] const JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                       const JSHandle<JSTaggedValue> &y)
{
    if (x->IsNumber() && y->IsNumber()) {
        return StrictNumberEquals(x->ExtractNumber(), y->ExtractNumber());
    }

    if (x.GetTaggedValue() == y.GetTaggedValue()) {
        return true;
    }

    if (x->IsString() && y->IsString()) {
        return EcmaString::StringsAreEqual(x.GetObject<EcmaString>(), y.GetObject<EcmaString>());
    }
    return false;
}

inline ComparisonResult JSTaggedValue::StrictNumberCompare(double x, double y)
{
    if (std::isnan(x) || std::isnan(y)) {
        return ComparisonResult::UNDEFINED;
    }
    if (x < y) {
        return ComparisonResult::LESS;
    }
    if (x > y) {
        return ComparisonResult::GREAT;
    }
    return ComparisonResult::EQUAL;
}

inline bool JSTaggedValue::IsNumber() const
{
    return IsInt() || IsDouble();
}

inline bool JSTaggedValue::IsString() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsString();
}

inline bool JSTaggedValue::IsStringOrSymbol() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStringOrSymbol();
}

inline bool JSTaggedValue::IsTaggedArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsTaggedArray();
}

inline bool JSTaggedValue::IsNativePointer() const
{
    return IsJSNativePointer();
}

inline bool JSTaggedValue::IsJSNativeObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSNativeObject();
}

inline bool JSTaggedValue::IsJSNativePointer() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSNativePointer();
}

inline bool JSTaggedValue::IsSymbol() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSSymbol();
}

inline bool JSTaggedValue::IsJSProxy() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSProxy();
}

inline bool JSTaggedValue::IsBoolean() const
{
    return IsTrue() || IsFalse();
}

inline bool JSTaggedValue::IsJSObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSObject();
}

inline bool JSTaggedValue::IsECMAObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsECMAObject();
}

inline bool JSTaggedValue::IsJSPromise() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSPromise();
}

inline bool JSTaggedValue::IsRecord() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsRecord();
}

inline bool JSTaggedValue::IsPromiseReaction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPromiseReaction();
}

inline bool JSTaggedValue::IsJSPromiseReactionFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSPromiseReactionFunction();
}

inline bool JSTaggedValue::IsProgram() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsProgram();
}

inline bool JSTaggedValue::IsLexicalFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsLexicalFunction();
}

inline bool JSTaggedValue::IsJSPromiseExecutorFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSPromiseExecutorFunction();
}

inline bool JSTaggedValue::IsJSPromiseAllResolveElementFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSPromiseAllResolveElementFunction();
}

inline bool JSTaggedValue::IsCompletionRecord() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsCompletionRecord();
}

inline bool JSTaggedValue::IsResolvingFunctionsRecord() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsResolvingFunctionsRecord();
}

inline bool JSTaggedValue::IsPromiseRecord() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPromiseRecord();
}

inline bool JSTaggedValue::IsJSLocale() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSLocale();
}

inline bool JSTaggedValue::IsJSIntl() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSIntl();
}

inline bool JSTaggedValue::IsJSDateTimeFormat() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSDateTimeFormat();
}

inline bool JSTaggedValue::IsJSRelativeTimeFormat() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSRelativeTimeFormat();
}

inline bool JSTaggedValue::IsJSNumberFormat() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSNumberFormat();
}

inline bool JSTaggedValue::IsJSCollator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSCollator();
}

inline bool JSTaggedValue::IsJSPluralRules() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSPluralRules();
}

inline bool JSTaggedValue::IsPromiseIteratorRecord() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPromiseIteratorRecord();
}

inline bool JSTaggedValue::IsPromiseCapability() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPromiseCapability();
}

inline bool JSTaggedValue::IsJSError() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSError();
}

inline bool JSTaggedValue::IsJSFunctionExtraInfo() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSFunctionExtraInfo();
}

inline bool JSTaggedValue::IsMicroJobQueue() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsMicroJobQueue();
}

inline bool JSTaggedValue::IsPendingJob() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPendingJob();
}

inline bool JSTaggedValue::IsArguments() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsArguments();
}

inline bool JSTaggedValue::IsDate() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsDate();
}

inline bool JSTaggedValue::IsArray(JSThread *thread) const
{
    if (!IsHeapObject()) {
        return false;
    }
    JSHClass *jsHclass = GetTaggedObject()->GetClass();
    if (jsHclass->IsJSArray()) {
        return true;
    }

    if (jsHclass->IsJSProxy()) {
        return JSProxy::Cast(GetTaggedObject())->IsArray(thread);
    }
    return false;
}

inline bool JSTaggedValue::IsJSArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSArray();
}

inline bool JSTaggedValue::IsStableJSArray(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableJSArray() &&
           !thread->IsStableArrayElementsGuardiansInvalid();
}

inline bool JSTaggedValue::IsStableJSArguments(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableJSArguments() &&
           !thread->IsStableArrayElementsGuardiansInvalid();
}

inline bool JSTaggedValue::HasStableElements(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableElements() &&
           !thread->IsStableArrayElementsGuardiansInvalid();
}

inline bool JSTaggedValue::IsTypedArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsTypedArray();
}

inline bool JSTaggedValue::IsJSTypedArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSTypedArray();
}

inline bool JSTaggedValue::IsJSInt8Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSInt8Array();
}

inline bool JSTaggedValue::IsJSUint8Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSUint8Array();
}

inline bool JSTaggedValue::IsJSUint8ClampedArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSUint8ClampedArray();
}

inline bool JSTaggedValue::IsJSInt16Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSInt16Array();
}

inline bool JSTaggedValue::IsJSUint16Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSUint16Array();
}

inline bool JSTaggedValue::IsJSInt32Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSInt32Array();
}

inline bool JSTaggedValue::IsJSUint32Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSUint32Array();
}

inline bool JSTaggedValue::IsJSFloat32Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSFloat32Array();
}

inline bool JSTaggedValue::IsJSFloat64Array() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSFloat64Array();
}

inline bool JSTaggedValue::IsJSMap() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSMap();
}

inline bool JSTaggedValue::IsJSWeakMap() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSWeakMap();
}

inline bool JSTaggedValue::IsJSWeakSet() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSWeakSet();
}

inline bool JSTaggedValue::IsJSSet() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSSet();
}

inline bool JSTaggedValue::IsJSRegExp() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSRegExp();
}

inline bool JSTaggedValue::IsJSFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSFunction();
}

inline bool JSTaggedValue::IsJSFunctionBase() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSFunctionBase();
}

inline bool JSTaggedValue::IsBoundFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJsBoundFunction();
}

inline bool JSTaggedValue::IsJSIntlBoundFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSIntlBoundFunction();
}

inline bool JSTaggedValue::IsProxyRevocFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSProxyRevocFunction();
}

inline bool JSTaggedValue::IsJSAsyncFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSAsyncFunction();
}

inline bool JSTaggedValue::IsJSAsyncAwaitStatusFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSAsyncAwaitStatusFunction();
}

inline bool JSTaggedValue::IsJSPrimitiveRef() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJsPrimitiveRef();
}

inline bool JSTaggedValue::IsJSPrimitive() const
{
    return IsNumber() || IsStringOrSymbol() || IsBoolean();
}

inline bool JSTaggedValue::IsAccessorData() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsAccessorData();
}

inline bool JSTaggedValue::IsInternalAccessor() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsInternalAccessor();
}

inline bool JSTaggedValue::IsAccessor() const
{
    if (IsHeapObject()) {
        auto *jshcalss = GetTaggedObject()->GetClass();
        return jshcalss->IsAccessorData() || jshcalss->IsInternalAccessor();
    }

    return false;
}

inline bool JSTaggedValue::IsPrototypeHandler() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPrototypeHandler();
}

inline bool JSTaggedValue::IsTransitionHandler() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsTransitionHandler();
}

inline bool JSTaggedValue::IsPropertyBox() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsPropertyBox();
}

inline bool JSTaggedValue::IsProtoChangeDetails() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsProtoChangeDetails();
}
inline bool JSTaggedValue::IsProtoChangeMarker() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsProtoChangeMarker();
}

inline bool JSTaggedValue::IsJSGlobalEnv() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJsGlobalEnv();
}

inline bool JSTaggedValue::IsForinIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsForinIterator();
}

inline bool JSTaggedValue::IsJSSetIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSSetIterator();
}

inline bool JSTaggedValue::IsJSMapIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSMapIterator();
}

inline bool JSTaggedValue::IsJSArrayIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSArrayIterator();
}

inline bool JSTaggedValue::IsIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsIterator();
}

inline bool JSTaggedValue::IsGeneratorFunction() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsGeneratorFunction();
}
inline bool JSTaggedValue::IsGeneratorObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsGeneratorObject();
}

inline bool JSTaggedValue::IsAsyncFuncObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsAsyncFuncObject();
}

inline bool JSTaggedValue::IsDynClass() const
{
    return IsJSHClass();
}

inline bool JSTaggedValue::IsJSHClass() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsHClass();
}

inline bool JSTaggedValue::IsObjectWrapper() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsObjectWrapper();
}

inline bool JSTaggedValue::IsStringIterator() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStringIterator();
}

inline bool JSTaggedValue::IsArrayBuffer() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsArrayBuffer();
}

inline bool JSTaggedValue::IsDataView() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsDataView();
}

inline bool JSTaggedValue::IsTemplateMap() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsTemplateMap();
}

inline bool JSTaggedValue::IsJSGlobalObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSGlobalObject();
}

inline bool JSTaggedValue::IsMachineCodeObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsMachineCodeObject();
}

inline double JSTaggedValue::ExtractNumber() const
{
    ASSERT(IsNumber());
    return GetNumber();
}

// 9.4.2.4 ArraySetLength 3 to 7
inline bool JSTaggedValue::ToArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged, uint32_t *output)
{
    // 3. Let newLen be ToUint32(Desc.[[Value]]).
    uint32_t newLen = ToUint32(thread, tagged);
    // 4. ReturnIfAbrupt(newLen).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 5. Let numberLen be ToNumber(Desc.[[Value]]).
    JSTaggedNumber numberLen = ToNumber(thread, tagged);
    // 6. ReturnIfAbrupt(newLen).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 7. If newLen != numberLen, throw a RangeError exception.
    if (JSTaggedNumber(newLen) != numberLen) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Not a valid array length", false);
    }

    *output = newLen;
    return true;
}

inline array_size_t JSTaggedValue::GetArrayLength() const
{
    ASSERT(IsNumber());
    if (IsInt()) {
        return static_cast<array_size_t>(GetInt());
    }
    if (IsDouble()) {
        ASSERT(GetDouble() <= TaggedArray::MAX_ARRAY_INDEX);
        return static_cast<array_size_t>(GetDouble());
    }
    UNREACHABLE();
}

inline bool JSTaggedValue::ToElementIndex(JSTaggedValue key, uint32_t *output)
{
    if (key.IsInt()) {
        int index = key.GetInt();
        if (index >= 0) {
            *output = index;
            return true;
        }
    } else if (key.IsDouble()) {
        double d = key.GetDouble();
        uint32_t index = base::NumberHelper::DoubleToInt(d, base::INT32_BITS);
        if (d - static_cast<double>(index) == 0.0) {
            *output = index;
            return true;
        }
    } else if (key.IsString()) {
        return StringToElementIndex(key, output);
    }
    return false;
}

inline bool JSTaggedValue::StringToElementIndex(JSTaggedValue key, uint32_t *output)
{
    ASSERT(key.IsString());

    auto strObj = static_cast<EcmaString *>(key.GetTaggedObject());
    uint32_t len = strObj->GetLength();
    if (len == 0 || len > MAX_INDEX_LEN) {  // NOLINTNEXTLINEreadability-magic-numbers)
        return false;
    }
    uint32_t c;
    if (strObj->IsUtf16()) {
        c = strObj->GetDataUtf16()[0];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    } else {
        c = strObj->GetDataUtf8()[0];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    uint64_t n = 0;
    if (c >= '0' && c <= '9') {
        if (c == '0') {
            if (len != 1) {
                return false;
            }
            *output = 0;
            return true;
        }

        n = c - '0';
        for (uint32_t i = 1; i < len; i++) {
            if (strObj->IsUtf16()) {
                c = strObj->GetDataUtf16()[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            } else {
                c = strObj->GetDataUtf8()[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            }
            if (c < '0' || c > '9') {
                return false;
            }
            // NOLINTNEXTLINE(readability-magic-numbers)
            n = n * 10 + (c - '0');  // 10: decimal factor
        }
        if (n < JSObject::MAX_ELEMENT_INDEX) {
            *output = n;
            return true;
        }
    }
    return false;
}

inline uint32_t JSTaggedValue::GetKeyHashCode() const
{
    ASSERT(IsStringOrSymbol());
    if (IsString()) {
        return EcmaString::Cast(GetTaggedObject())->GetHashcode();
    }

    return static_cast<uint32_t>(JSSymbol::Cast(GetTaggedObject())->GetHashField().GetInt());
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_VALUE__INL_H
