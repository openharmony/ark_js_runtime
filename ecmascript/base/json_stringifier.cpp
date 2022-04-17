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

#include "ecmascript/base/json_stringifier.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::base {
constexpr unsigned char CODE_SPACE = 0x20;
constexpr int GAP_MAX_LEN = 10;
constexpr int FOUR_HEX = 4;
constexpr char ZERO_FIRST = static_cast<char>(0xc0); // \u0000 => c0 80

bool JsonStringifier::IsFastValueToQuotedString(const char *value)
{
    if (strpbrk(value, "\"\\\b\f\n\r\t") != nullptr) {
        return false;
    }
    while (*value != '\0') {
        if ((*value > 0 && *value < CODE_SPACE) || *value == ZERO_FIRST) {
            return false;
        }
        value++;
    }
    return true;
}

CString JsonStringifier::ValueToQuotedString(CString str)
{
    CString product;
    const char *value = str.c_str();
    // fast mode
    bool isFast = IsFastValueToQuotedString(value);
    if (isFast) {
        product += "\"";
        product += str;
        product += "\"";
        return product;
    }
    // 1. Let product be code unit 0x0022 (QUOTATION MARK).
    product += "\"";
    // 2. For each code unit C in value
    for (const char *c = value; *c != 0; ++c) {
        switch (*c) {
            /*
             * a. If C is 0x0022 (QUOTATION MARK) or 0x005C (REVERSE SOLIDUS), then
             * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
             * ii. Let product be the concatenation of product and C.
             */
            case '\"':
                product += "\\\"";
                break;
            case '\\':
                product += "\\\\";
                break;
            /*
             * b. Else if C is 0x0008 (BACKSPACE), 0x000C (FORM FEED), 0x000A (LINE FEED), 0x000D (CARRIAGE RETURN),
             * or 0x000B (LINE TABULATION), then
             * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
             * ii. Let abbrev be the String value corresponding to the value of C as follows:
             * BACKSPACE "b"
             * FORM FEED (FF) "f"
             * LINE FEED (LF) "n"
             * CARRIAGE RETURN (CR) "r"
             * LINE TABULATION "t"
             * iii. Let product be the concatenation of product and abbrev.
             */
            case '\b':
                product += "\\b";
                break;
            case '\f':
                product += "\\f";
                break;
            case '\n':
                product += "\\n";
                break;
            case '\r':
                product += "\\r";
                break;
            case '\t':
                product += "\\t";
                break;
            case ZERO_FIRST:
                product += "\\u0000";
                ++c;
                break;
            default:
                // c. Else if C has a code unit value less than 0x0020 (SPACE), then
                if (*c > 0 && *c < CODE_SPACE) {
                    /*
                     * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
                     * ii. Let product be the concatenation of product and "u".
                     * iii. Let hex be the string result of converting the numeric code unit value of C to a String of
                     * four hexadecimal digits. Alphabetic hexadecimal digits are presented as lowercase Latin letters.
                     * iv. Let product be the concatenation of product and hex.
                     */
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(FOUR_HEX) << static_cast<int>(*c);
                    product += oss.str();
                } else {
                    // Else,
                    // i. Let product be the concatenation of product and C.
                    product += *c;
                }
        }
    }
    // 3. Let product be the concatenation of product and code unit 0x0022 (QUOTATION MARK).
    product += "\"";
    // Return product.
    return product;
}

JSHandle<JSTaggedValue> JsonStringifier::Stringify(const JSHandle<JSTaggedValue> &value,
                                                   const JSHandle<JSTaggedValue> &replacer,
                                                   const JSHandle<JSTaggedValue> &gap)
{
    factory_ = thread_->GetEcmaVM()->GetFactory();
    handleValue_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    handleKey_ = JSMutableHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    // Let isArray be IsArray(replacer).
    bool isArray = replacer->IsArray(thread_);
    // ReturnIfAbrupt(isArray).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    // If isArray is true, then
    if (isArray) {
        uint32_t len;
        if (replacer->IsJSArray()) {
            // FastPath
            JSHandle<JSArray> arr(replacer);
            len = arr->GetArrayLength();
        } else {
            // Let len be ToLength(Get(replacer, "length")).
            JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
            JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread_, replacer, lengthKey).GetValue();
            // ReturnIfAbrupt(len).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread_, lenResult);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            len = lenNumber.ToUint32();
        }
        if (len > 0) {
            JSMutableHandle<JSTaggedValue> propHandle(thread_, JSTaggedValue::Undefined());
            // Repeat while k<len.
            for (uint32_t i = 0; i < len; i++) {
                // a. Let v be Get(replacer, ToString(k)).
                JSTaggedValue prop = FastRuntimeStub::FastGetPropertyByIndex(thread_, replacer.GetTaggedValue(), i);
                // b. ReturnIfAbrupt(v).
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                /*
                 * c. Let item be undefined.
                 * d. If Type(v) is String, let item be v.
                 * e. Else if Type(v) is Number, let item be ToString(v).
                 * f. Else if Type(v) is Object, then
                 * i. If v has a [[StringData]] or [[NumberData]] internal slot, let item be ToString(v).
                 * ii. ReturnIfAbrupt(item).
                 * g. If item is not undefined and item is not currently an element of PropertyList, then
                 * i. Append item to the end of PropertyList.
                 * h. Let k be k+1.
                 */
                propHandle.Update(prop);
                if (prop.IsNumber() || prop.IsString()) {
                    AddDeduplicateProp(propHandle);
                    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                } else if (prop.IsJSPrimitiveRef()) {
                    JSTaggedValue primitive = JSPrimitiveRef::Cast(prop.GetTaggedObject())->GetValue();
                    if (primitive.IsNumber() || primitive.IsString()) {
                        AddDeduplicateProp(propHandle);
                        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
                    }
                }
            }
        }
    }

    // If Type(space) is Object, then
    if (gap->IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(gap->GetTaggedObject())->GetValue();
        // a. If space has a [[NumberData]] internal slot, then
        if (primitive.IsNumber()) {
            // i. Let space be ToNumber(space).
            JSTaggedNumber num = JSTaggedValue::ToNumber(thread_, gap);
            // ii. ReturnIfAbrupt(space).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            CalculateNumberGap(num);
        } else if (primitive.IsString()) {
            // b. Else if space has a [[StringData]] internal slot, then
            // i. Let space be ToString(space).
            auto str = JSTaggedValue::ToString(thread_, gap);
            // ii. ReturnIfAbrupt(space).
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            CalculateStringGap(JSHandle<EcmaString>(thread_, str.GetTaggedValue()));
        }
    } else if (gap->IsNumber()) {
        // If Type(space) is Number
        CalculateNumberGap(gap.GetTaggedValue());
    } else if (gap->IsString()) {
        // Else if Type(space) is String
        CalculateStringGap(JSHandle<EcmaString>::Cast(gap));
    }

    JSHandle<JSTaggedValue> key(factory_->GetEmptyString());
    JSTaggedValue serializeValue = GetSerializeValue(value, key, value, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    handleValue_.Update(serializeValue);
    JSTaggedValue result = SerializeJSONProperty(handleValue_, replacer);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
    if (!result.IsUndefined()) {
        return JSHandle<JSTaggedValue>(
            factory_->NewFromUtf8Literal(reinterpret_cast<const uint8_t *>(result_.c_str()), result_.size()));
    }
    return thread_->GlobalConstants()->GetHandledUndefined();
}

void JsonStringifier::AddDeduplicateProp(const JSHandle<JSTaggedValue> &property)
{
    uint32_t propLen = propList_.size();
    for (uint32_t i = 0; i < propLen; i++) {
        if (JSTaggedValue::SameValue(propList_[i], property)) {
            return;
        }
    }
    JSHandle<EcmaString> primString = JSTaggedValue::ToString(thread_, property);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    JSHandle<JSTaggedValue> addVal(thread_, *primString);
    propList_.emplace_back(addVal);
}

bool JsonStringifier::CalculateNumberGap(JSTaggedValue gap)
{
    double numValue = gap.GetNumber();
    int num = static_cast<int>(numValue);
    if (num > 0) {
        int gapLength = std::min(num, GAP_MAX_LEN);
        for (int i = 0; i < gapLength; i++) {
            gap_ += " ";
        }
        gap_.append("\0");
    }
    return true;
}

bool JsonStringifier::CalculateStringGap(const JSHandle<EcmaString> &primString)
{
    CString gapString = ConvertToString(*primString, StringConvertedUsage::LOGICOPERATION);
    uint32_t gapLen = gapString.length();
    if (gapLen > 0) {
        int gapLength = std::min(gapLen, static_cast<uint32_t>(GAP_MAX_LEN));
        CString str = gapString.substr(0, gapLength);
        gap_ += str;
        gap_.append("\0");
    }
    return true;
}

JSTaggedValue JsonStringifier::GetSerializeValue(const JSHandle<JSTaggedValue> &object,
                                                 const JSHandle<JSTaggedValue> &key,
                                                 const JSHandle<JSTaggedValue> &value,
                                                 const JSHandle<JSTaggedValue> &replacer)
{
    JSTaggedValue tagValue = value.GetTaggedValue();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    // If Type(value) is Object, then
    if (value->IsECMAObject()) {
        // a. Let toJSON be Get(value, "toJSON").
        JSHandle<JSTaggedValue> toJson = thread_->GlobalConstants()->GetHandledToJsonString();
        JSHandle<JSTaggedValue> toJsonFun(
            thread_, FastRuntimeStub::FastGetPropertyByValue(thread_, tagValue, toJson.GetTaggedValue()));
        // b. ReturnIfAbrupt(toJSON).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);

        // c. If IsCallable(toJSON) is true
        if (UNLIKELY(toJsonFun->IsCallable())) {
            // Let value be Call(toJSON, value, «key»).
            EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread_, toJsonFun, value, undefined, 1);
            info.SetCallArg(key.GetTaggedValue());
            tagValue = JSFunction::Call(&info);
            // ii. ReturnIfAbrupt(value).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        }
    }

    if (UNLIKELY(replacer->IsCallable())) {
        handleValue_.Update(tagValue);
        // a. Let value be Call(ReplacerFunction, holder, «key, value»).
        const size_t argsLength = 2; // 2: «key, value»
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread_, replacer, object, undefined, argsLength);
        info.SetCallArg(key.GetTaggedValue(), handleValue_.GetTaggedValue());
        tagValue = JSFunction::Call(&info);
        // b. ReturnIfAbrupt(value).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
    }
    return tagValue;
}

JSTaggedValue JsonStringifier::SerializeJSONProperty(const JSHandle<JSTaggedValue> &value,
                                                     const JSHandle<JSTaggedValue> &replacer)
{
    JSTaggedValue tagValue = value.GetTaggedValue();
    if (!tagValue.IsHeapObject()) {
        TaggedType type = tagValue.GetRawData();
        switch (type) {
            // If value is false, return "false".
            case JSTaggedValue::VALUE_FALSE:
                result_ += "false";
                return tagValue;
            // If value is true, return "true".
            case JSTaggedValue::VALUE_TRUE:
                result_ += "true";
                return tagValue;
            // If value is null, return "null".
            case JSTaggedValue::VALUE_NULL:
                result_ += "null";
                return tagValue;
            default:
                // If Type(value) is Number, then
                if (tagValue.IsNumber()) {
                    // a. If value is finite, return ToString(value).
                    if (std::isfinite(tagValue.GetNumber())) {
                        result_ += ConvertToString(*base::NumberHelper::NumberToString(thread_, tagValue));
                    } else {
                        // b. Else, return "null".
                        result_ += "null";
                    }
                    return tagValue;
                }
        }
    } else {
        JSType jsType = tagValue.GetTaggedObject()->GetClass()->GetObjectType();
        JSHandle<JSTaggedValue> valHandle(thread_, tagValue);
        switch (jsType) {
            case JSType::JS_ARRAY: {
                SerializeJSArray(valHandle, replacer);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                return tagValue;
            }
            // If Type(value) is String, return QuoteJSONString(value).
            case JSType::STRING: {
                CString str = ConvertToString(*JSHandle<EcmaString>(valHandle), StringConvertedUsage::LOGICOPERATION);
                str = ValueToQuotedString(str);
                result_ += str;
                return tagValue;
            }
            case JSType::JS_PRIMITIVE_REF: {
                SerializePrimitiveRef(valHandle);
                return tagValue;
            }
            case JSType::SYMBOL:
                return JSTaggedValue::Undefined();
            default: {
                if (!tagValue.IsCallable()) {
                    JSHClass *jsHclass = tagValue.GetTaggedObject()->GetClass();
                    if (UNLIKELY(jsHclass->IsJSProxy() &&
                        JSProxy::Cast(tagValue.GetTaggedObject())->IsArray(thread_))) {
                        SerializeJSProxy(valHandle, replacer);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    } else {
                        SerializeJSONObject(valHandle, replacer);
                        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
                    }
                    return tagValue;
                }
            }
        }
    }
    return JSTaggedValue::Undefined();
}

void JsonStringifier::SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent)
{
    CString stepBegin;
    CString stepEnd;
    if (hasContent) {
        result_ += ",";
    }
    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
        stepEnd += " ";
    }
    CString str;
    if (key->IsString()) {
        str = ConvertToString(EcmaString::Cast(key->GetTaggedObject()), StringConvertedUsage::LOGICOPERATION);
    } else if (key->IsInt()) {
        str = NumberHelper::IntToString(static_cast<int32_t>(key->GetInt()));
    } else {
        str = ConvertToString(*JSTaggedValue::ToString(thread_, key), StringConvertedUsage::LOGICOPERATION);
    }
    result_ += stepBegin;
    str = ValueToQuotedString(str);
    result_ += str;
    result_ += ":";
    result_ += stepEnd;
}

bool JsonStringifier::PushValue(const JSHandle<JSTaggedValue> &value)
{
    uint32_t thisLen = stack_.size();

    for (uint32_t i = 0; i < thisLen; i++) {
        bool equal = JSTaggedValue::SameValue(stack_[i].GetTaggedValue(), value.GetTaggedValue());
        if (equal) {
            return true;
        }
    }

    stack_.emplace_back(value);
    return false;
}

void JsonStringifier::PopValue()
{
    stack_.pop_back();
}

bool JsonStringifier::SerializeJSONObject(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
{
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value", true);
    }

    CString stepback = indent_;
    indent_ += gap_;

    result_ += "{";
    bool hasContent = false;

    JSHandle<JSObject> obj(value);
    if (!replacer->IsArray(thread_)) {
        if (UNLIKELY(value->IsJSProxy())) {  // serialize proxy object
            JSHandle<JSProxy> proxy(value);
            JSHandle<TaggedArray> propertyArray = JSObject::EnumerableOwnNames(thread_, obj);
            uint32_t arrLength = propertyArray->GetLength();
            for (uint32_t i = 0; i < arrLength; i++) {
                handleKey_.Update(propertyArray->Get(i));
                JSHandle<JSTaggedValue> proxyValue = JSProxy::GetProperty(thread_, proxy, handleKey_).GetValue();
                JSTaggedValue serializeValue = GetSerializeValue(value, handleKey_, proxyValue, replacer);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
                    (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
                    continue;
                }
                handleValue_.Update(serializeValue);
                SerializeObjectKey(handleKey_, hasContent);
                JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
                if (!res.IsUndefined()) {
                    hasContent = true;
                }
            }
        } else {
            uint32_t numOfKeys = obj->GetNumberOfKeys();
            uint32_t numOfElements = obj->GetNumberOfElements();
            if (numOfElements > 0) {
                hasContent = JsonStringifier::SerializeElements(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            if (numOfKeys > 0) {
                hasContent = JsonStringifier::SerializeKeys(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        uint32_t propLen = propList_.size();
        for (uint32_t i = 0; i < propLen; i++) {
            JSTaggedValue tagVal =
                FastRuntimeStub::FastGetPropertyByValue(thread_, obj.GetTaggedValue(), propList_[i].GetTaggedValue());
            handleValue_.Update(tagVal);
            JSTaggedValue serializeValue = GetSerializeValue(value, propList_[i], handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
                (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
                continue;
            }
            handleValue_.Update(serializeValue);
            SerializeObjectKey(propList_[i], hasContent);
            JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (!res.IsUndefined()) {
                hasContent = true;
            }
        }
    }
    if (hasContent && gap_.length() != 0) {
        result_ += "\n";
        result_ += stepback;
    }
    result_ += "}";
    PopValue();
    indent_ = stepback;
    return true;
}

bool JsonStringifier::SerializeJSProxy(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &replacer)
{
    bool isContain = PushValue(object);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value", true);
    }

    CString stepback = indent_;
    CString stepBegin;
    indent_ += gap_;

    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
    }
    result_ += "[";
    JSHandle<JSProxy> proxy(object);
    JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenghHandle = JSProxy::GetProperty(thread_, proxy, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread_, lenghHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    uint32_t length = lenNumber.ToUint32();
    for (uint32_t i = 0; i < length; i++) {
        handleKey_.Update(JSTaggedValue(i));
        JSHandle<JSTaggedValue> valHandle = JSProxy::GetProperty(thread_, proxy, handleKey_).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (i > 0) {
            result_ += ",";
        }
        result_ += stepBegin;
        JSTaggedValue serializeValue = GetSerializeValue(object, handleKey_, valHandle, replacer);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        handleValue_.Update(serializeValue);
        JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        if (res.IsUndefined()) {
            result_ += "null";
        }
    }

    if (length > 0 && !gap_.empty()) {
        result_ += "\n";
        result_ += stepback;
    }
    result_ += "]";
    PopValue();
    indent_ = stepback;
    return true;
}

bool JsonStringifier::SerializeJSArray(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer)
{
    // If state.[[Stack]] contains value, throw a TypeError exception because the structure is cyclical.
    bool isContain = PushValue(value);
    if (isContain) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "stack contains value", true);
    }

    CString stepback = indent_;
    CString stepBegin;
    indent_ += gap_;

    if (!gap_.empty()) {
        stepBegin += "\n";
        stepBegin += indent_;
    }
    result_ += "[";
    JSHandle<JSArray> jsArr(value);
    uint32_t len = jsArr->GetArrayLength();
    if (len > 0) {
        for (uint32_t i = 0; i < len; i++) {
            JSTaggedValue tagVal = FastRuntimeStub::FastGetPropertyByIndex(thread_, value.GetTaggedValue(), i);
            if (UNLIKELY(tagVal.IsAccessor())) {
                tagVal = JSObject::CallGetter(thread_, AccessorData::Cast(tagVal.GetTaggedObject()), value);
            }
            handleKey_.Update(JSTaggedValue(i));
            handleValue_.Update(tagVal);

            if (i > 0) {
                result_ += ",";
            }
            result_ += stepBegin;
            JSTaggedValue serializeValue = GetSerializeValue(value, handleKey_, handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            handleValue_.Update(serializeValue);
            JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            if (res.IsUndefined()) {
                result_ += "null";
            }
        }

        if (!gap_.empty()) {
            result_ += "\n";
            result_ += stepback;
        }
    }

    result_ += "]";
    PopValue();
    indent_ = stepback;
    return true;
}

void JsonStringifier::SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef)
{
    JSTaggedValue primitive = JSPrimitiveRef::Cast(primitiveRef.GetTaggedValue().GetTaggedObject())->GetValue();
    if (primitive.IsString()) {
        auto priStr = JSTaggedValue::ToString(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        CString str = ConvertToString(*priStr, StringConvertedUsage::LOGICOPERATION);
        str = ValueToQuotedString(str);
        result_ += str;
    } else if (primitive.IsNumber()) {
        auto priNum = JSTaggedValue::ToNumber(thread_, primitiveRef);
        RETURN_IF_ABRUPT_COMPLETION(thread_);
        if (std::isfinite(priNum.GetNumber())) {
            result_ += ConvertToString(*base::NumberHelper::NumberToString(thread_, priNum));
        } else {
            result_ += "null";
        }
    } else if (primitive.IsBoolean()) {
        result_ += primitive.IsTrue() ? "true" : "false";
    }
}

bool JsonStringifier::SerializeElements(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                        bool hasContent)
{
    JSHandle<TaggedArray> elementsArr(thread_, obj->GetElements());
    if (!elementsArr->IsDictionaryMode()) {
        uint32_t elementsLen = elementsArr->GetLength();
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!elementsArr->Get(i).IsHole()) {
                handleKey_.Update(JSTaggedValue(i));
                handleValue_.Update(elementsArr->Get(i));
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
    } else {
        JSHandle<NumberDictionary> numberDic(elementsArr);
        CVector<JSHandle<JSTaggedValue>> sortArr;
        int size = numberDic->Size();
        for (int hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = numberDic->GetKey(hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                PropertyAttributes attr = numberDic->GetAttributes(hashIndex);
                if (attr.IsEnumerable()) {
                    JSTaggedValue numberKey = JSTaggedValue(static_cast<uint32_t>(key.GetInt()));
                    sortArr.emplace_back(JSHandle<JSTaggedValue>(thread_, numberKey));
                }
            }
        }
        std::sort(sortArr.begin(), sortArr.end(), CompareNumber);
        for (const auto &entry : sortArr) {
            JSTaggedValue entryKey = entry.GetTaggedValue();
            handleKey_.Update(entryKey);
            int index = numberDic->FindEntry(entryKey);
            JSTaggedValue value = numberDic->GetValue(index);
            handleValue_.Update(value);
            hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
        }
    }
    return hasContent;
}

bool JsonStringifier::SerializeKeys(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                    bool hasContent)
{
    JSHandle<TaggedArray> propertiesArr(thread_, obj->GetProperties());
    if (!propertiesArr->IsDictionaryMode()) {
        JSHandle<JSHClass> jsHclass(thread_, obj->GetJSHClass());
        JSTaggedValue enumCache = jsHclass->GetEnumCache();
        if (!enumCache.IsNull()) {
            int propsNumber = static_cast<int>(jsHclass->NumberOfProps());
            JSHandle<TaggedArray> cache(thread_, enumCache);
            uint32_t length = cache->GetLength();
            for (uint32_t i = 0; i < length; i++) {
                JSTaggedValue key = cache->Get(i);
                if (!key.IsString()) {
                    continue;
                }
                handleKey_.Update(key);
                JSTaggedValue value;
                LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
                int index = layoutInfo->FindElementWithCache(thread_, *jsHclass, key, propsNumber);
                PropertyAttributes attr(layoutInfo->GetAttr(index));
                ASSERT(static_cast<int>(attr.GetOffset()) == index);
                value = attr.IsInlinedProps()
                        ? obj->GetPropertyInlinedProps(static_cast<uint32_t>(index))
                        : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
                if (UNLIKELY(value.IsAccessor())) {
                    value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                                 JSHandle<JSTaggedValue>(obj));
                }
                handleValue_.Update(value);
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
            return hasContent;
        }
        int end = static_cast<int>(jsHclass->NumberOfProps());
        if (end <= 0) {
            return hasContent;
        }
        int propsNumber = static_cast<int>(jsHclass->NumberOfProps());
        for (int i = 0; i < end; i++) {
            LayoutInfo *layoutInfo = LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject());
            JSTaggedValue key = layoutInfo->GetKey(i);
            if (key.IsString() && layoutInfo->GetAttr(i).IsEnumerable()) {
                handleKey_.Update(key);
                JSTaggedValue value;
                int index = layoutInfo->FindElementWithCache(thread_, *jsHclass, key, propsNumber);
                PropertyAttributes attr(layoutInfo->GetAttr(index));
                ASSERT(static_cast<int>(attr.GetOffset()) == index);
                value = attr.IsInlinedProps()
                        ? obj->GetPropertyInlinedProps(static_cast<uint32_t>(index))
                        : propertiesArr->Get(static_cast<uint32_t>(index) - jsHclass->GetInlinedProperties());
                if (UNLIKELY(value.IsAccessor())) {
                    value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                                 JSHandle<JSTaggedValue>(obj));
                }
                handleValue_.Update(value);
                hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
            }
        }
        return hasContent;
    }
    JSHandle<NameDictionary> nameDic(propertiesArr);
    int size = nameDic->Size();
    CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = nameDic->GetKey(hashIndex);
        if (!key.IsString()) {
            continue;
        }
        PropertyAttributes attr = nameDic->GetAttributes(hashIndex);
        if (!attr.IsEnumerable()) {
            continue;
        }
        std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> pair(JSHandle<JSTaggedValue>(thread_, key), attr);
        sortArr.emplace_back(pair);
    }
    std::sort(sortArr.begin(), sortArr.end(), CompareKey);
    for (const auto &entry : sortArr) {
        JSTaggedValue entryKey = entry.first.GetTaggedValue();
        handleKey_.Update(entryKey);
        int index = nameDic->FindEntry(entryKey);
        JSTaggedValue value = nameDic->GetValue(index);
        if (UNLIKELY(value.IsAccessor())) {
            value = JSObject::CallGetter(thread_, AccessorData::Cast(value.GetTaggedObject()),
                                         JSHandle<JSTaggedValue>(obj));
        }
        handleValue_.Update(value);
        hasContent = JsonStringifier::AppendJsonString(obj, replacer, hasContent);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    }
    return hasContent;
}

bool JsonStringifier::AppendJsonString(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer,
                                       bool hasContent)
{
    JSTaggedValue serializeValue = GetSerializeValue(JSHandle<JSTaggedValue>(obj), handleKey_, handleValue_, replacer);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (UNLIKELY(serializeValue.IsUndefined() || serializeValue.IsSymbol() ||
        (serializeValue.IsECMAObject() && serializeValue.IsCallable()))) {
        return hasContent;
    }
    handleValue_.Update(serializeValue);
    SerializeObjectKey(handleKey_, hasContent);
    JSTaggedValue res = SerializeJSONProperty(handleValue_, replacer);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, false);
    if (!res.IsUndefined()) {
        return true;
    }
    return hasContent;
}
}  // namespace panda::ecmascript::base
