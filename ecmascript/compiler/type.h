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

const uint64_t GC_MASK = ~(1 << 30); // 30 : the 30-th bit is unset implies GC-related type
const uint64_t NO_GC_MASK = ~(1 << 29); // 29 : the 29-th bit is unset implies NO-GC-related type
const uint64_t MIR_BASE_BITS = (1 << 31) | (1 << 30) | (1 << 29); // 31 : the 31-st bit is set implies MIR type
const uint64_t EMPTY_TYPE_OFFSET = 1; // 1 : means offset of empty type

namespace panda::ecmascript::kungfu {
enum GateType : uint64_t {
    // for MIR
    C_VALUE = MIR_BASE_BITS, // (111)
    TAGGED_VALUE = MIR_BASE_BITS & GC_MASK & NO_GC_MASK, // (100)
    TAGGED_POINTER = MIR_BASE_BITS & GC_MASK, // (101)
    TAGGED_NO_POINTER = MIR_BASE_BITS & NO_GC_MASK, // (110)

    // for no value
    EMPTY = C_VALUE + EMPTY_TYPE_OFFSET,

    // for TS
    JS_ANY = 0,
};

class Type {
public:
    explicit Type(GateType payload);
    [[nodiscard]] bool IsBitset() const;
    ~Type();

private:
    GateType payload;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_TYPE_H
