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

#include "ecmascript/builtins/builtins_global.h"
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/slow_runtime_helper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
using NumberHelper = base::NumberHelper;
using StringHelper = base::StringHelper;

// 18.2.1
JSTaggedValue BuiltinsGlobal::NotSupportEval(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "not support eval()", JSTaggedValue::Exception());
}

// 18.2.2
JSTaggedValue BuiltinsGlobal::IsFinite(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, IsFinite);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> numberInput = GetCallArg(msg, 0);
    // 1. Let num be ToNumber(number).
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, numberInput);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If num is NaN, +Infinite, or -Infinite, return false.
    // 4. Otherwise, return true.
    if (std::isfinite(number.GetNumber())) {
        return GetTaggedBoolean(true);
    }
    return GetTaggedBoolean(false);
}

// 18.2.3
JSTaggedValue BuiltinsGlobal::IsNaN(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, IsNaN);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> numberInput = GetCallArg(msg, 0);
    // 1. Let num be ToNumber(number).
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, numberInput);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If num is NaN, return true.
    if (std::isnan(number.GetNumber())) {
        return GetTaggedBoolean(true);
    }
    // 4. Otherwise, return false.
    return GetTaggedBoolean(false);
}

bool BuiltinsGlobal::IsUnescapedURI(uint16_t ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
        return true;
    }
    return IsInMarkURISet(ch);
}

bool BuiltinsGlobal::IsInUnescapedURISet(uint16_t ch)
{
    if (ch == '#') {
        return true;
    }
    return IsUnescapedURI(ch) || IsReservedURI(ch);
}

bool BuiltinsGlobal::IsInReservedURISet(uint16_t ch)
{
    if (ch == '#') {
        return true;
    }
    return IsReservedURI(ch);
}

bool BuiltinsGlobal::IsReservedURI(uint16_t ch)
{
    std::u16string str(u";/?:@&=+$,");
    std::u16string::size_type index = str.find(ch);
    return (index != std::u16string::npos);
}

bool BuiltinsGlobal::IsInMarkURISet(uint16_t ch)
{
    std::u16string str(u"-_.!~*'()");
    std::u16string::size_type index = str.find(ch);
    return (index != std::u16string::npos);
}

bool BuiltinsGlobal::IsHexDigits(uint16_t ch)
{
    return ('0' <= ch && ch <= '9') || ('A' <= ch && ch <= 'F') || ('a' <= ch && ch <= 'f');
}

// 18.2.6
JSTaggedValue BuiltinsGlobal::DecodeURI(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, DecodeURI);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let uriString be ToString(encodedURI).
    // 2. ReturnIfAbrupt(uriString).
    [[maybe_unused]] JSHandle<EcmaString> uriString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let reservedURISet be a String containing one instance of each code unit valid in uriReserved plus "#".
    // 4. Return Decode(uriString, reservedURISet).
    return Decode(thread, uriString, IsInReservedURISet);
}

JSTaggedValue BuiltinsGlobal::EncodeURI(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, EncodeURI);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let uriString be ToString(uri).
    // 2. ReturnIfAbrupt(uriString).
    [[maybe_unused]] JSHandle<EcmaString> uriString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let unescapedURISet be a String containing one instance of
    //    each code unit valid in uriReserved and uriUnescaped plus "#".
    // 4. Return Encode(uriString, unescapedURISet).
    return Encode(thread, uriString, IsInUnescapedURISet);
}

JSTaggedValue BuiltinsGlobal::DecodeURIComponent(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, DecodeURIComponent);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let componentString be ToString(encodedURIComponent).
    // 2. ReturnIfAbrupt(componentString).
    [[maybe_unused]] JSHandle<EcmaString> componentString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let reservedURIComponentSet be the empty String.
    // 4. Return Decode(componentString, reservedURIComponentSet).
    return Decode(thread, componentString, []([[maybe_unused]] uint16_t unused) { return false; });
}

JSTaggedValue BuiltinsGlobal::EncodeURIComponent(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, EncodeURIComponent);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let componentString be ToString(uriComponent).
    // 2. ReturnIfAbrupt(componentString).
    [[maybe_unused]] JSHandle<EcmaString> componentString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let unescapedURIComponentSet be a String containing one instance of each code unit valid in uriUnescaped.
    // 4. Return Encode(componentString, unescapedURIComponentSet).
    return Encode(thread, componentString, IsUnescapedURI);
}

// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    // 1. Let strLen be the number of code units in string.
    uint32_t strLen = str->GetLength();
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;

    // 3. Let k be 0.
    // 4. Repeat
    uint32_t k = 0;
    while (true) {
        // a. If k equals strLen, return R.
        if (k == strLen) {
            auto *uint16tData = reinterpret_cast<uint16_t *>(resStr.data());
            uint32_t resSize = resStr.size();
            return factory->NewFromUtf16Literal(uint16tData, resSize).GetTaggedValue();
        }

        // b. Let C be the code unit at index k within string.
        // c. If C is in unescapedSet, then
        //   i. Let S be a String containing only the code unit C.
        //   ii. Let R be a new String value computed by concatenating the previous value of R and S.
        // d. Else C is not in unescapedSet,
        uint16_t cc = str->At(k);
        if (IsInURISet(cc)) {
            std::u16string sStr = StringHelper::Utf16ToU16String(&cc, 1);
            resStr.append(sStr);
        } else {
            // i. If the code unit value of C is not less than 0xDC00 and not greater than 0xDFFF,
            //    throw a URIError exception.
            if (cc >= base::utf_helper::DECODE_TRAIL_LOW && cc <= base::utf_helper::DECODE_TRAIL_HIGH) {
                THROW_URI_ERROR_AND_RETURN(thread, "EncodeURI: The format of the URI to be parsed is incorrect",
                                           JSTaggedValue::Exception());
            }

            // ii. If the code unit value of C is less than 0xD800 or greater than 0xDBFF, then
            //    1. Let V be the code unit value of C.
            // iii. Else,
            //    1. Increase k by 1.
            //    2. If k equals strLen, throw a URIError exception.
            //    3. Let kChar be the code unit value of the code unit at index k within string.
            //    4. If kChar is less than 0xDC00 or greater than 0xDFFF, throw a URIError exception.
            //    5. Let V be UTF16Decode(C, kChar).
            uint32_t vv;
            if (cc < base::utf_helper::DECODE_LEAD_LOW || cc > base::utf_helper::DECODE_LEAD_HIGH) {
                vv = cc;
            } else {
                k++;
                if (k == strLen) {
                    THROW_URI_ERROR_AND_RETURN(thread, "k is invalid", JSTaggedValue::Exception());
                }
                uint16_t kc = str->At(k);
                if (kc < base::utf_helper::DECODE_TRAIL_LOW || kc > base::utf_helper::DECODE_TRAIL_HIGH) {
                    THROW_URI_ERROR_AND_RETURN(thread, "EncodeURI: The format of the URI to be parsed is incorrect",
                                               JSTaggedValue::Exception());
                }
                vv = base::utf_helper::UTF16Decode(cc, kc);
            }

            // iv. Let Octets be the array of octets resulting by applying the UTF-8 transformation to V,
            //     and let L be the array size.
            // v. Let j be 0.
            // vi. Repeat, while j < L
            //    1. Let jOctet be the value at index j within Octets.
            //    2. Let S be a String containing three code units "%XY" where XY are two uppercase hexadecimal
            //       digits encoding the value of jOctet.
            //    3. Let R be a new String value computed by concatenating the previous value of R and S.
            //    4. Increase j by 1.
            std::string oct = StringHelper::Utf32ToString(vv);
            std::string hexStr("0123456789ABCDEF");

            uint32_t length = oct.length();
            std::stringstream tmpStr;
            for (uint32_t j = 0; j < length; j++) {
                uint8_t joct = static_cast<uint8_t>(oct.at(j));
                tmpStr << '%' << hexStr.at((joct >> 4U) & BIT_MASK)  // NOLINT
                       << hexStr.at(joct & BIT_MASK);                // 4: means shift right by 4 digits
            }
            resStr.append(StringHelper::StringToU16string(tmpStr.str()));
        }

        // e. Increase k by 1.
        k++;
    }
}

uint8_t BuiltinsGlobal::GetValueFromTwoHex(uint16_t front, uint16_t behind)
{
    ASSERT(IsHexDigits(front) && IsHexDigits(behind));
    std::u16string hexString(u"0123456789ABCDEF");

    size_t idxf = StringHelper::FindFromU16ToUpper(hexString, &front);
    size_t idxb = StringHelper::FindFromU16ToUpper(hexString, &behind);
    uint8_t res = ((idxf << 4U) | idxb) & BIT_MASK_FF;  // NOLINT 4: means shift left by 4 digits
    return res;
}

// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    // 1. Let strLen be the number of code units in string.
    [[maybe_unused]] int32_t strLen = static_cast<int32_t>(str->GetLength());
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;

    // 3. Let k be 0.
    // 4. Repeat
    int32_t k = 0;
    while (true) {
        // a. If k equals strLen, return R.
        if (k == strLen) {
            auto *uint16tData = reinterpret_cast<uint16_t *>(resStr.data());
            uint32_t resSize = resStr.size();
            return factory->NewFromUtf16Literal(uint16tData, resSize).GetTaggedValue();
        }

        // b. Let C be the code unit at index k within string.
        // c. If C is not "%", then
        //    i. Let S be the String containing only the code unit C.
        // d. Else C is "%",
        //   i. Let start be k.
        //   iv. Let B be the 8-bit value represented by the two hexadecimal digits at index (k + 1) and (k + 2).
        //   v. Increment k by 2.
        //   vi. If the most significant bit in B is 0, then
        //      1. Let C be the code unit with code unit value B.
        //      2. If C is not in reservedSet, then
        //         a. Let S be the String containing only the code unit C.
        //      3. Else C is in reservedSet,
        //         a. Let S be the substring of string from index start to index k inclusive.
        uint16_t cc = str->At(k);
        std::u16string sStr;
        if (cc != '%') {
            if (cc == 0 && strLen == 1) {
                JSHandle<EcmaString> tmpEcmaString = factory->NewFromUtf16Literal(&cc, 1);
                return tmpEcmaString.GetTaggedValue();
            }
            sStr = StringHelper::Utf16ToU16String(&cc, 1);
        } else {
            [[maybe_unused]] uint32_t start = k;

            // ii. If k + 2 is greater than or equal to strLen, throw a URIError exception.
            // iii. If the code units at index (k+1) and (k + 2) within string do not represent hexadecimal digits,
            //      throw a URIError exception.
            if ((k + 2) >= strLen) {  // 2: means plus 2
                THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                           JSTaggedValue::Exception());
            }
            if (!(IsHexDigits(str->At(k + 1)) && IsHexDigits(str->At(k + 2)))) {  // 2: means plus 2
                THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                           JSTaggedValue::Exception());
            }

            uint16_t frontChar = str->At(k + 1);
            uint16_t behindChar = str->At(k + 2);  // 2: means plus 2
            uint8_t bb = GetValueFromTwoHex(frontChar, behindChar);
            k += 2;  // 2: means plus 2
            if ((bb & BIT_MASK_ONE) == 0) {
                if (!IsInURISet(bb)) {
                    sStr = StringHelper::Utf8ToU16String(&bb, 1);
                    if (bb == 0) {
                        return factory->NewFromUtf16Literal(reinterpret_cast<uint16_t *>(sStr.data()), 1)
                            .GetTaggedValue();
                    }
                } else {
                    sStr = StringHelper::StringToU16string(StringHelper::SubString(thread, str, start, k - start + 1));
                }
            } else {
                // vii. Else the most significant bit in B is 1,
                //   1. Let n be the smallest nonnegative integer such that (B << n) & 0x80 is equal to 0.
                //   3. Let Octets be an array of 8-bit integers of size n.
                //   4. Put B into Octets at index 0.
                //   6. Let j be 1.
                //   7. Repeat, while j < n
                //     a. Increment k by 1.
                //     d. Let B be the 8-bit value represented by the two hexadecimal digits at
                //        index (k + 1) and (k + 2).
                //     f. Increment k by 2.
                //     g. Put B into Octets at index j.
                //     h. Increment j by 1.
                //   9. If V < 0x10000, then
                //     a. Let C be the code unit V.
                //     b. If C is not in reservedSet, then
                //        i. Let S be the String containing only the code unit C.
                //     c. Else C is in reservedSet,
                //        i. Let S be the substring of string from index start to index k inclusive.
                //   10. Else V ≥ 0x10000,
                //     a. Let L be (((V – 0x10000) & 0x3FF) + 0xDC00).
                //     b. Let H be ((((V – 0x10000) >> 10) & 0x3FF) + 0xD800).
                //     c. Let S be the String containing the two code units H and L.
                int32_t n = 0;
                while ((((static_cast<uint32_t>(bb) << static_cast<uint32_t>(n)) & BIT_MASK_ONE) != 0)) {
                    n++;
                    if (n > 4) // 4 : 4 means less than 4
                        break;
                }
                // 2. If n equals 1 or n is greater than 4, throw a URIError exception.
                if ((n == 1) || (n > 4)) {  // 4: means greater than 4
                    THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                               JSTaggedValue::Exception());
                }

                std::vector<uint8_t> oct = {bb};

                // 5. If k + (3 × (n – 1)) is greater than or equal to strLen, throw a URIError exception.
                if (k + (3 * (n - 1)) >= strLen) {  // 3: means multiply by 3
                    THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                               JSTaggedValue::Exception());
                }
                int32_t j = 1;
                while (j < n) {
                    k++;
                    uint16_t codeUnit = str->At(k);
                    // b. If the code unit at index k within string is not "%", throw a URIError exception.
                    // c. If the code units at index (k +1) and (k + 2) within string do not represent hexadecimal
                    //    digits, throw a URIError exception.
                    if (!(codeUnit == '%')) {
                        THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                                   JSTaggedValue::Exception());
                    }
                    if (!(IsHexDigits(str->At(k + 1)) && IsHexDigits(str->At(k + 2)))) {  // 2: means plus 2
                        THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                                   JSTaggedValue::Exception());
                    }

                    uint16_t frontChart = str->At(k + 1);
                    uint16_t behindChart = str->At(k + 2);  // 2: means plus 2
                    bb = GetValueFromTwoHex(frontChart, behindChart);
                    // e. If the two most significant bits in B are not 10, throw a URIError exception.
                    if (!((bb & BIT_MASK_TWO) == BIT_MASK_ONE)) {
                        THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                                   JSTaggedValue::Exception());
                    }

                    k += 2;  // 2: means plus 2
                    oct.push_back(bb);
                    j++;
                }

                // 8. Let V be the value obtained by applying the UTF-8 transformation to Octets, that is,
                //     from an array of octets into a 21-bit value. If Octets does not contain a valid UTF-8 encoding of
                //     a Unicode code point throw a URIError exception.
                if (!base::utf_helper::IsValidUTF8(oct)) {
                    THROW_URI_ERROR_AND_RETURN(thread, "DecodeURI: The format of the URI to be parsed is incorrect",
                                               JSTaggedValue::Exception());
                }
                uint32_t vv = StringHelper::Utf8ToU32String(oct);
                if (vv < base::utf_helper::DECODE_SECOND_FACTOR) {
                    if (!IsInURISet(vv)) {
                        sStr = StringHelper::Utf16ToU16String(reinterpret_cast<uint16_t *>(&vv), 1);
                    } else {
                        sStr =
                            StringHelper::StringToU16string(StringHelper::SubString(thread, str, start, k - start + 1));
                    }
                } else {
                    uint16_t lv = (((vv - base::utf_helper::DECODE_SECOND_FACTOR) & BIT16_MASK) +
                        base::utf_helper::DECODE_TRAIL_LOW);
                    uint16_t hv = ((((vv - base::utf_helper::DECODE_SECOND_FACTOR) >> 10U) & BIT16_MASK) +  // NOLINT
                        base::utf_helper::DECODE_LEAD_LOW);  // 10: means shift left by 10 digits
                    sStr = StringHelper::Append(StringHelper::Utf16ToU16String(&hv, 1),
                                                StringHelper::Utf16ToU16String(&lv, 1));
                }
            }
        }
        // e. Let R be a new String value computed by concatenating the previous value of R and S.
        // f. Increase k by 1.
        resStr.append(sStr);
        k++;
    }
}

void BuiltinsGlobal::PrintString([[maybe_unused]] JSThread *thread, EcmaString *string)
{
    if (string == nullptr) {
        return;
    }

    CString buffer = ConvertToString(string);
    std::cout << buffer;
}

JSTaggedValue BuiltinsGlobal::PrintEntrypoint(EcmaRuntimeCallInfo *msg)
{
    if (msg == nullptr) {
        return JSTaggedValue::Undefined();
    }
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    BUILTINS_API_TRACE(thread, Global, PrintEntryPoint);

    uint32_t numArgs = msg->GetArgsNumber();
    for (uint32_t i = 0; i < numArgs; i++) {
        JSHandle<EcmaString> stringContent = JSTaggedValue::ToString(thread, GetCallArg(msg, i));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        PrintString(thread, *stringContent);

        if (i != numArgs - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGlobal::CallJsBoundFunction(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, CallJsBoundFunction);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // msg contains jsfunc, this, arg1,...

    JSHandle<JSBoundFunction> boundFunc(GetConstructor(msg));
    JSHandle<JSTaggedValue> thisObj(thread, boundFunc->GetBoundThis());
    msg->SetThis(thisObj.GetTaggedValue());
    return SlowRuntimeHelper::CallBoundFunction(msg);
}

JSTaggedValue BuiltinsGlobal::CallJsProxy(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, CallJsProxy);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // msg contains js_proxy, this, arg1,...
    JSHandle<JSProxy> proxy(GetConstructor(msg));
    if (!proxy->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Proxy target is not callable", JSTaggedValue::Undefined());
    }

    // Calling proxy directly should transfer 'undefined' as this
    return JSProxy::CallInternal(msg);
}

#if ECMASCRIPT_ENABLE_RUNTIME_STAT
JSTaggedValue BuiltinsGlobal::StartRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(true);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGlobal::StopRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(false);
    return JSTaggedValue::Undefined();
}
#endif
}  // namespace panda::ecmascript::builtins
