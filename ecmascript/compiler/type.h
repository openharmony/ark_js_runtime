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

#ifndef ECMASCRIPT_COMPILER_TYPE_H
#define ECMASCRIPT_COMPILER_TYPE_H

#include <cstdint>

namespace kungfu {
using GateType = uint8_t;
enum TypeCode : GateType {
    // for HIR
    NOTYPE,
    JS_ANY,
    JS_NULL,
    JS_UNDEFINED,
    JS_BOOLEAN,
    JS_NUMBER,
    JS_STRING,
    JS_BIGINT,
    JS_SYMBOL,
    JS_OBJECT,
    JS_FUNCTION,
    JS_ARRAY,
    JS_INT8ARRAY,
    JS_INT16ARRAY,
    JS_INT32ARRAY,
    JS_UINT8ARRAY,
    JS_UINT16ARRAY,
    JS_UINT32ARRAY,
    JS_FLOAT32ARRAY,
    JS_FLOAT64ARRAY,
    // for MIR
    IS_REFERENCE,
    NOT_REFERENCE,
};

class Type {
public:
    explicit Type(GateType payload);
    [[nodiscard]] bool IsBitset() const;
    ~Type();

private:
    GateType payload;
};
}  // namespace kungfu

#endif  // ECMASCRIPT_COMPILER_TYPE_H
