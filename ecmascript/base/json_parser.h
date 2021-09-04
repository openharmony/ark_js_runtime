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

#ifndef PANDA_RUNTIME_ECMASCRIPT_BASE_JSON_PARSE_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_BASE_JSON_PARSE_INL_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::base {
class JsonParser {
public:
    using Text = const uint8_t *;
    explicit JsonParser() = default;
    explicit JsonParser(JSThread *thread) : thread_(thread) {}
    ~JsonParser() = default;
    NO_COPY_SEMANTIC(JsonParser);
    NO_MOVE_SEMANTIC(JsonParser);
    JSHandle<JSTaggedValue> Parse(Text begin, Text end);
    JSHandle<JSTaggedValue> Parse(EcmaString *str);
    JSHandle<JSTaggedValue> InternalizeJsonProperty(const JSHandle<JSObject> &holder,
                                                    const JSHandle<JSTaggedValue> &name,
                                                    const JSHandle<JSTaggedValue> &receiver);

private:
    enum class Tokens : uint8_t {
        // six structural tokens
        OBJECT = 0,
        ARRAY,
        NUMBER,
        STRING,
        LITERAL_TRUE,
        LITERAL_FALSE,
        LITERAL_NULL,
        TOKEN_ILLEGAL,
    };

    template <bool inObjorArr = false>
    JSTaggedValue ParseJSONText();

    template <bool inObjOrArr = false>
    JSTaggedValue ParseNumber();

    template <bool inObjorArr = false>
    JSTaggedValue ParseString();

    template <bool inObjorArr = false>
    JSTaggedValue ParseArray();

    template <bool inObjorArr = false>
    JSTaggedValue ParseObject();

    void SkipEndWhiteSpace();
    void SkipStartWhiteSpace();
    JsonParser::Tokens ParseToken();
    JSTaggedValue ParseLiteral(CString str, Tokens literalToken);
    bool MatchText(CString str, uint32_t matchLen);
    bool ReadNumberRange();
    bool IsNumberCharacter(uint8_t ch);
    bool IsNumberSignalCharacter(uint8_t ch);
    bool IsExponentNumber();
    bool IsDecimalsLegal(bool &hasExponent);
    bool IsExponentLegal(bool &hasExponent);
    bool ReadStringRange(bool &isFast);
    bool IsFastParseString(CString &value);
    bool ConvertStringUnicode(CVector<uint16_t> &vec);

    bool RecurseAndApply(const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &name,
                         const JSHandle<JSTaggedValue> &receiver);

    Text end_{nullptr};
    Text current_{nullptr};
    Text range_{nullptr};
    JSThread *thread_{nullptr};
    ObjectFactory *factory_{nullptr};
    GlobalEnv *env_{nullptr};
};
}  // namespace panda::ecmascript::base

#endif  // PANDA_RUNTIME_ECMASCRIPT_BASE_JSON_PARSE_INL_H
