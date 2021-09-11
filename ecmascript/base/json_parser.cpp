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

#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::base {
constexpr unsigned int UNICODE_DIGIT_LENGTH = 4;
constexpr unsigned int NUMBER_TEN = 10;
constexpr unsigned int NUMBER_SIXTEEN = 16;

constexpr unsigned char CODE_SPACE = 0x20;
constexpr unsigned char ASCII_END = 0X7F;
JSHandle<JSTaggedValue> JsonParser::Parse(Text begin, Text end)
{
    end_ = end - 1;
    current_ = begin;

    auto vm = thread_->GetEcmaVM();
    factory_ = vm->GetFactory();
    env_ = *vm->GetGlobalEnv();

    SkipEndWhiteSpace();
    range_ = end_;
    JSTaggedValue result = ParseJSONText<false>();
    return JSHandle<JSTaggedValue>(thread_, result);
}

JSHandle<JSTaggedValue> JsonParser::Parse(EcmaString *str)
{
    ASSERT(str != nullptr);
    if (UNLIKELY(str->IsUtf16())) {
        uint32_t len = str->GetLength();
        std::u16string u16str = StringHelper::Utf16ToU16String(str->GetDataUtf16(), len);
        std::string u8str = StringHelper::U16stringToString(u16str);
        Text begin = reinterpret_cast<Text>(u8str.c_str());
        return Parse(begin, begin + u8str.length());
    }

    isAsciiString_ = true;
    CVector<uint8_t> buf(str->GetUtf8Length());
    str->CopyDataUtf8(buf.data(), str->GetUtf8Length());
    Text begin = buf.data();
    return Parse(begin, begin + str->GetLength());
}

template<bool inObjorArr>
JSTaggedValue JsonParser::ParseJSONText()
{
    SkipStartWhiteSpace();
    Tokens token = ParseToken();
    switch (token) {
        case Tokens::OBJECT:
            return ParseObject<inObjorArr>();
        case Tokens::ARRAY:
            return ParseArray<inObjorArr>();
        case Tokens::LITERAL_TRUE:
            return ParseLiteral("true", Tokens::LITERAL_TRUE);
        case Tokens::LITERAL_FALSE:
            return ParseLiteral("false", Tokens::LITERAL_FALSE);
        case Tokens::LITERAL_NULL:
            return ParseLiteral("null", Tokens::LITERAL_NULL);
        case Tokens::NUMBER:
            return ParseNumber<inObjorArr>();
        case Tokens::STRING:
            return ParseString<inObjorArr>();
        default:
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
    }
}

JsonParser::Tokens JsonParser::ParseToken()
{
    switch (*current_) {
        case '{':
            return Tokens::OBJECT;
        case '[':
            return Tokens::ARRAY;
        case '"':
            return Tokens::STRING;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            return Tokens::NUMBER;
        case 't':
            return Tokens::LITERAL_TRUE;
        case 'f':
            return Tokens::LITERAL_FALSE;
        case 'n':
            return Tokens::LITERAL_NULL;
        default:
            return Tokens::TOKEN_ILLEGAL;
    }
}

void JsonParser::SkipEndWhiteSpace()
{
    while (current_ != end_) {
        if (*end_ == ' ' || *end_ == '\r' || *end_ == '\n' || *end_ == '\t') {
            end_--;
        } else {
            break;
        }
    }
}

void JsonParser::SkipStartWhiteSpace()
{
    while (current_ != end_) {
        if (*current_ == ' ' || *current_ == '\r' || *current_ == '\n' || *current_ == '\t') {
            current_++;
        } else {
            break;
        }
    }
}

void JsonParser::GetNextNonSpaceChar()
{
    current_++;
    SkipStartWhiteSpace();
}

JSTaggedValue JsonParser::ParseLiteral(CString str, Tokens literalToken)
{
    uint32_t strLen = str.size() - 1;
    uint32_t remainingLength = range_ - current_;
    if (UNLIKELY(remainingLength < strLen)) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
    }

    bool isMatch = MatchText(str, strLen);
    if (LIKELY(isMatch)) {
        switch (literalToken) {
            case Tokens::LITERAL_TRUE:
                return JSTaggedValue::True();
            case Tokens::LITERAL_FALSE:
                return JSTaggedValue::False();
            case Tokens::LITERAL_NULL:
                return JSTaggedValue::Null();
            default:
                UNREACHABLE();
        }
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
}

bool JsonParser::MatchText(CString str, uint32_t matchLen)
{
    const char *text = str.c_str();
    uint32_t pos = 1;
    while (pos <= matchLen) {
        if (current_[pos] != text[pos]) {
            return false;
        }
        pos++;
    }
    current_ += matchLen;
    return true;
}

template<bool inObjOrArr>
JSTaggedValue JsonParser::ParseNumber()
{
    if (inObjOrArr) {
        bool isFast = true;
        bool isNumber = ReadNumberRange(isFast);
        if (!isNumber) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
        if (isFast) {
            double result = NumberHelper::StringToDouble(current_, end_ + 1, 0, 0);
            current_ = end_;
            return JSTaggedValue(result);
        }
    }

    Text current = current_;
    bool hasExponent = false;
    if (*current_ == '-') {
        if (UNLIKELY(current_++ == end_)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
    }
    if (*current_ == '0') {
        if (!CheckZeroBeginNumber(hasExponent)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
    } else if (*current_ >= '1' && *current_ <= '9') {
        if (!CheckNonZeroBeginNumber(hasExponent)) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
        }
    } else {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Number in JSON", JSTaggedValue::Exception());
    }

    double result = NumberHelper::StringToDouble(current, end_ + 1, 0, 0);
    current_ = end_;
    return JSTaggedValue(result);
}

bool JsonParser::CheckZeroBeginNumber(bool &hasExponent)
{
    if (current_++ != end_) {
        if (*current_ == '.') {
            if (!IsDecimalsLegal(hasExponent)) {
                return false;
            }
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (!IsExponentLegal(hasExponent)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool JsonParser::CheckNonZeroBeginNumber(bool &hasExponent)
{
    while (current_ != end_) {
        current_++;
        if (IsNumberCharacter(*current_)) {
            continue;
        } else if (*current_ == '.') {
            if (!IsDecimalsLegal(hasExponent)) {
                return false;
            }
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (!IsExponentLegal(hasExponent)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool JsonParser::ReadNumberRange(bool &isFast)
{
    Text current = current_;
    if (*current == '0') {
        isFast = false;
        current++;
    } else if (*current == '-') {
        current++;
        if (*current == '0') {
            isFast = false;
            current++;
        }
    }

    while (current != range_) {
        if (IsNumberCharacter(*current)) {
            current++;
            continue;
        } else if (IsNumberSignalCharacter(*current)) {
            isFast = false;
            current++;
            continue;
        } else {
            Text end = current;
            while (current != range_) {
                if (*current == ' ' || *current == '\r' || *current == '\n' || *current == '\t') {
                    current++;
                } else if (*current == ',' || *current == ']' || *current == '}') {
                    end_ = end - 1;
                    return true;
                } else {
                    return false;
                }
            }
            return false;
        }
    }
    end_ = range_ - 1;
    return true;
}

bool JsonParser::IsNumberCharacter(uint8_t ch)
{
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    return false;
}

bool JsonParser::IsNumberSignalCharacter(uint8_t ch)
{
    return ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-';
}

bool JsonParser::IsExponentNumber()
{
    if (IsNumberCharacter(*current_)) {
        return true;
    } else if (*current_ == '-' || *current_ == '+') {
        if (current_ == end_) {
            return false;
        }
        current_++;
        if (IsNumberCharacter(*current_)) {
            return true;
        }
    }
    return false;
}

bool JsonParser::IsDecimalsLegal(bool &hasExponent)
{
    if (current_ == end_ && !IsNumberCharacter(*++current_)) {
        return false;
    }

    while (current_ != end_) {
        current_++;
        if (IsNumberCharacter(*current_)) {
            continue;
        } else if (*current_ == 'e' || *current_ == 'E') {
            if (hasExponent || current_ == end_) {
                return false;
            }
            current_++;
            if (!IsExponentNumber()) {
                return false;
            }
            hasExponent = true;
        } else {
            return false;
        }
    }
    return true;
}

bool JsonParser::IsExponentLegal(bool &hasExponent)
{
    if (hasExponent || current_ == end_) {
        return false;
    }
    current_++;
    if (!IsExponentNumber()) {
        return false;
    }
    while (current_ != end_) {
        if (!IsNumberCharacter(*current_)) {
            return false;
        }
        current_++;
    }
    return true;
}

bool JsonParser::ReadStringRange(bool &isFast, bool &isAscii)
{
    uint8_t c = 0;
    Text current = current_;

    while (current != range_) {
        c = *current;
        if (c == '"') {
            end_ = current;
            return true;
        } else if (UNLIKELY(c == '\\')) {
            if (*(current + 1) == '"') {
                current++;
            }
            isFast = false;
        }
        if (!IsLegalAsciiCharacter(c, isAscii)) {
            return false;
        }
        current++;
    }
    return false;
}

bool JsonParser::ReadAsciiStringRange(bool &isFast)
{
    uint8_t c = 0;
    Text current = current_;

    while (current != range_) {
        c = *current;
        if (c == '"') {
            end_ = current;
            return true;
        } else if (UNLIKELY(c == '\\')) {
            if (*(current + 1) == '"') {
                current++;
            }
            isFast = false;
        } else if (UNLIKELY(c < CODE_SPACE)) {
            return false;
        }
        current++;
    }
    return false;
}

inline bool JsonParser::IsLegalAsciiCharacter(uint8_t c, bool &isAscii)
{
    if (c <= ASCII_END) {
        if (c >= CODE_SPACE) {
            return true;
        }
        return false;
    }
    isAscii = false;
    return true;
}

template<bool inObjorArr>
JSTaggedValue JsonParser::ParseString()
{
    bool isFast = true;
    bool isAscii = true;
    bool isLegal = true;
    if (inObjorArr) {
        current_++;
        if (isAsciiString_) {
            isLegal = ReadAsciiStringRange(isFast);
        } else {
            isLegal = ReadStringRange(isFast, isAscii);
        }
        if (!isLegal) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON", JSTaggedValue::Exception());
        }
        if (isFast) {
            CString value(current_, end_);
            current_ = end_;
            if (isAscii) {
                return factory_->NewFromUtf8LiteralUnCheck(reinterpret_cast<Text>(value.c_str()), value.length(), true)
                    .GetTaggedValue();
            }
            return factory_->NewFromUtf8LiteralUnCheck(reinterpret_cast<Text>(value.c_str()), value.length(), false)
                .GetTaggedValue();
        }
    } else {
        if (*end_ != '"' || current_ == end_) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON", JSTaggedValue::Exception());
        }
        current_++;
        if (isAsciiString_) {
            isLegal = IsFastParseString(isFast, isAscii);
        } else {
            isLegal = IsFastParseAsciiString(isFast);
        }
        if (!isLegal) {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected end Text in JSON", JSTaggedValue::Exception());
        }
        if (LIKELY(isFast)) {
            CString value(current_, end_);
            if (isAscii) {
                return factory_->NewFromUtf8LiteralUnCheck(reinterpret_cast<Text>(value.c_str()), value.length(), true)
                    .GetTaggedValue();
            }
            return factory_->NewFromUtf8LiteralUnCheck(reinterpret_cast<Text>(value.c_str()), value.length(), false)
                .GetTaggedValue();
        }
    }
    end_--;
    CString res;
    while (current_ <= end_) {
        if (*current_ == '\\') {
            if (current_ == end_) {
                THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
            }
            current_++;
            switch (*current_) {
                case '\"':
                    res += "\"";
                    break;
                case '\\':
                    res += "\\";
                    break;
                case '/':
                    res += "/";
                    break;
                case 'b':
                    res += "\b";
                    break;
                case 'f':
                    res += "\f";
                    break;
                case 'n':
                    res += "\n";
                    break;
                case 'r':
                    res += "\r";
                    break;
                case 't':
                    res += "\t";
                    break;
                case 'u': {
                    CVector<uint16_t> vec;
                    if (UNLIKELY(!ConvertStringUnicode(vec))) {
                        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
                    }
                    std::u16string u16Str;
                    u16Str.assign(vec.begin(), vec.end());
                    res += base::StringHelper::U16stringToString(u16Str);
                    break;
                }
                default:
                    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Text in JSON", JSTaggedValue::Exception());
            }
        } else {
            res += *current_;
        }
        current_++;
    }
    return factory_->NewFromUtf8Literal(reinterpret_cast<Text>(res.c_str()), res.length()).GetTaggedValue();
}

bool JsonParser::IsFastParseString(bool &isFast, bool &isAscii)
{
    Text current = current_;
    while (current != end_) {
        if (!IsLegalAsciiCharacter(*current, isAscii)) {
            return false;
        }
        if (*current == '\\') {
            isFast = false;
        }
        current++;
    }
    return true;
}

bool JsonParser::IsFastParseAsciiString(bool &isFast)
{
    Text current = current_;
    while (current != end_) {
        if (*current < CODE_SPACE) {
            return false;
        } else if (*current == '\\') {
            isFast = false;
        }
        current++;
    }
    return true;
}

bool JsonParser::ConvertStringUnicode(CVector<uint16_t> &vec)
{
    uint32_t remainingLength = end_ - current_;
    if (remainingLength < UNICODE_DIGIT_LENGTH) {
        return false;
    }
    uint16_t res = 0;
    uint32_t exponent = UNICODE_DIGIT_LENGTH;
    for (uint32_t pos = 0; pos < UNICODE_DIGIT_LENGTH; pos++) {
        current_++;
        exponent--;
        if (*current_ >= '0' && *current_ <= '9') {
            res += (*current_ - '0') * pow(NUMBER_SIXTEEN, exponent);
        } else if (*current_ >= 'a' && *current_ <= 'f') {
            res += (*current_ - 'a' + NUMBER_TEN) * pow(NUMBER_SIXTEEN, exponent);
        } else if (*current_ >= 'A' && *current_ <= 'F') {
            res += (*current_ - 'A' + NUMBER_TEN) * pow(NUMBER_SIXTEEN, exponent);
        } else {
            return false;
        }
    }
    if (res < CODE_SPACE) {
        return false;
    }

    vec.emplace_back(res);

    if (*(current_ + 1) == '\\' && *(current_ + 2) == 'u') {  // 2: next two chars
        current_ += 2;                                        // 2: point moves backwards by two digits
        return ConvertStringUnicode(vec);
    }
    return true;
}

template<bool inObjorArr>
JSTaggedValue JsonParser::ParseArray()
{
    if (UNLIKELY(*range_ != ']' && !inObjorArr)) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Array in JSON", JSTaggedValue::Exception());
    }

    current_++;
    JSHandle<JSArray> arr = factory_->NewJSArray();
    if (*current_ == ']') {
        return arr.GetTaggedValue();
    }

    JSTaggedValue value;
    uint32_t index = 0;
    while (current_ <= range_) {
        value = ParseJSONText<true>();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread_);
        FastRuntimeStub::SetPropertyByIndex<true>(thread_, arr.GetTaggedValue(), index++, value);
        GetNextNonSpaceChar();
        if (*current_ == ',') {
            current_++;
        } else if (*current_ == ']') {
            if (inObjorArr || current_ == range_) {
                return arr.GetTaggedValue();
            } else {
                THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Array in JSON", JSTaggedValue::Exception());
            }
        }
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Array in JSON", JSTaggedValue::Exception());
}

template<bool inObjorArr>
JSTaggedValue JsonParser::ParseObject()
{
    if (UNLIKELY(*range_ != '}' && !inObjorArr)) {
        THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> proto = env_->GetObjectFunction();
    JSHandle<JSObject> result = factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(proto), proto);
    current_++;
    if (*current_ == '}') {
        return result.GetTaggedValue();
    }

    JSMutableHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::Undefined());
    JSTaggedValue value;
    while (current_ <= range_) {
        SkipStartWhiteSpace();
        if (*current_ == '"') {
            keyHandle.Update(ParseString<true>());
        } else {
            if (*current_ == '}' && (inObjorArr || current_ == range_)) {
                return result.GetTaggedValue();
            }
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON", JSTaggedValue::Exception());
        }
        GetNextNonSpaceChar();
        if (*current_ == ':') {
            current_++;
        } else {
            THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON", JSTaggedValue::Exception());
        }
        value = ParseJSONText<true>();
        FastRuntimeStub::SetPropertyByValue<true>(thread_, result.GetTaggedValue(), keyHandle.GetTaggedValue(), value);
        GetNextNonSpaceChar();
        if (*current_ == ',') {
            current_++;
        } else if (*current_ == '}') {
            if (inObjorArr || current_ == range_) {
                return result.GetTaggedValue();
            } else {
                THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON", JSTaggedValue::Exception());
            }
        }
    }
    THROW_SYNTAX_ERROR_AND_RETURN(thread_, "Unexpected Object in JSON", JSTaggedValue::Exception());
}

JSHandle<JSTaggedValue> JsonParser::InternalizeJsonProperty(const JSHandle<JSObject> &holder,
                                                            const JSHandle<JSTaggedValue> &name,
                                                            const JSHandle<JSTaggedValue> &receiver)
{
    JSHandle<JSTaggedValue> objHandle(holder);
    JSHandle<JSTaggedValue> val = JSTaggedValue::GetProperty(thread_, objHandle, name).GetValue();
    JSHandle<JSTaggedValue> lengthKey = thread_->GlobalConstants()->GetHandledLengthString();
    if (val->IsECMAObject()) {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread_, val);
        bool isArray = val->IsArray(thread_);
        if (isArray) {
            JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread_, val, lengthKey).GetValue();
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread_, lenResult);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            array_size_t length = lenNumber.ToUint32();
            JSMutableHandle<JSTaggedValue> keyUnknow(thread_, JSTaggedValue::Undefined());
            JSMutableHandle<JSTaggedValue> keyName(thread_, JSTaggedValue::Undefined());
            for (array_size_t i = 0; i < length; i++) {
                // Let prop be ! ToString((I)).
                keyUnknow.Update(JSTaggedValue(i));
                keyName.Update(JSTaggedValue::ToString(thread_, keyUnknow).GetTaggedValue());
                RecurseAndApply(obj, keyName, receiver);
            }
        } else {
            // Let keys be ? EnumerableOwnPropertyNames(val, key).
            JSHandle<TaggedArray> ownerNames(JSObject::EnumerableOwnNames(thread_, obj));
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread_);
            array_size_t namesLength = ownerNames->GetLength();
            JSMutableHandle<JSTaggedValue> keyName(thread_, JSTaggedValue::Undefined());
            for (array_size_t i = 0; i < namesLength; i++) {
                keyName.Update(JSTaggedValue::GetProperty(thread_, JSHandle<JSTaggedValue>(ownerNames), i)
                    .GetValue().GetTaggedValue());
                RecurseAndApply(obj, keyName, receiver);
            }
        }
    }

    // Return ? Call(receiver, holder, « name, val »).
    InternalCallParams *arguments = thread_->GetInternalCallParams();
    arguments->MakeArgv(name, val);
    JSTaggedValue result = JSFunction::Call(thread_, receiver, objHandle, 2, arguments->GetArgv());  // 2: two args
    return JSHandle<JSTaggedValue>(thread_, result);
}

bool JsonParser::RecurseAndApply(const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &name,
                                 const JSHandle<JSTaggedValue> &receiver)
{
    JSHandle<JSTaggedValue> value = InternalizeJsonProperty(holder, name, receiver);
    bool changeResult = false;

    // If newElement is undefined, then Perform ? val.[[Delete]](P).
    if (value->IsUndefined()) {
        changeResult = JSObject::DeleteProperty(thread_, holder, name);
    } else {
        // Perform ? CreateDataProperty(val, P, newElement)
        changeResult = JSObject::CreateDataProperty(thread_, holder, name, value);
    }
    return changeResult;
}
}  // namespace panda::ecmascript::base
