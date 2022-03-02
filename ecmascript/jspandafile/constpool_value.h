/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPANDAFILE_CONSTPOOL_VALUE_H
#define ECMASCRIPT_JSPANDAFILE_CONSTPOOL_VALUE_H

#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
enum class ConstPoolType : uint8_t {
    STRING,
    BASE_FUNCTION,
    NC_FUNCTION,
    GENERATOR_FUNCTION,
    ASYNC_FUNCTION,
    CLASS_FUNCTION,
    METHOD,
    ARRAY_LITERAL,
    OBJECT_LITERAL,
    CLASS_LITERAL,
};

class ConstPoolValue {
public:
    ConstPoolValue(ConstPoolType type, uint32_t index)
        : value_(ConstPoolIndexField::Encode(index) | ConstPoolTypeField::Encode(type))
    {
    }

    explicit ConstPoolValue(uint64_t v) : value_(v) {}
    ~ConstPoolValue() = default;
    NO_COPY_SEMANTIC(ConstPoolValue);
    NO_MOVE_SEMANTIC(ConstPoolValue);

    inline uint64_t GetValue() const
    {
        return value_;
    }

    inline uint32_t GetConstpoolIndex() const
    {
        return ConstPoolIndexField::Get(value_);
    }

    inline ConstPoolType GetConstpoolType() const
    {
        return ConstPoolTypeField::Get(value_);
    }

private:
    // NOLINTNEXTLINE(readability-magic-numbers)
    using ConstPoolIndexField = BitField<uint32_t, 0, 32>;  // 32: 32 bit
    // NOLINTNEXTLINE(readability-magic-numbers)
    using ConstPoolTypeField = BitField<ConstPoolType, 32, 4>;  // 32: offset, 4: 4bit

    uint64_t value_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_CONSTPOOL_VALUE_H