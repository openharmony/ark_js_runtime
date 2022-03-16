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

#ifndef ECMASCRIPT_JS_NUMBER_H
#define ECMASCRIPT_JS_NUMBER_H

#include "ecmascript/base/number_helper.h"

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda {
namespace ecmascript {
class JSTaggedNumber final : public JSTaggedValue {
public:
    constexpr JSTaggedNumber() = default;
    explicit JSTaggedNumber(double v) : JSTaggedValue(v) {}
    constexpr explicit JSTaggedNumber(int v) : JSTaggedValue(v) {}
    explicit JSTaggedNumber(unsigned int v) : JSTaggedValue(v) {}
    explicit JSTaggedNumber(JSTaggedValue v) : JSTaggedValue(v.GetRawData())
    {
        ASSERT_PRINT(v.IsNumber(), "can not convert non Number JSTaggedValue to JSTaggedNumber");
    }

    ~JSTaggedNumber() = default;
    DEFAULT_COPY_SEMANTIC(JSTaggedNumber);
    DEFAULT_MOVE_SEMANTIC(JSTaggedNumber);

    static inline constexpr JSTaggedNumber Exception()
    {
        return JSTaggedNumber(VALUE_EXCEPTION);
    }

    inline bool IsException() const
    {
        return JSTaggedValue::IsException();
    }

    inline int32_t ToInt32() const
    {
        if (IsInt()) {
            return GetInt();
        }
        return base::NumberHelper::DoubleToInt(GetDouble(), base::INT32_BITS);
    }

    inline uint32_t ToUint32() const
    {
        return ToInt32();
    }

    inline int16_t ToInt16() const
    {
        return base::NumberHelper::DoubleToInt(GetNumber(), base::INT16_BITS);
    }

    inline uint16_t ToUint16() const
    {
        return ToInt16();
    }

    inline int8_t ToInt8() const
    {
        return base::NumberHelper::DoubleToInt(GetNumber(), base::INT8_BITS);
    }

    inline uint8_t ToUint8() const
    {
        return ToInt8();
    }

    inline JSHandle<EcmaString> ToString(const JSThread *thread) const
    {
        return base::NumberHelper::NumberToString(thread, *this);
    }

    JSTaggedNumber operator-(JSTaggedNumber number) const
    {
        if (IsInt() && number.IsInt()) {
            int64_t a0 = GetInt();
            int64_t a1 = number.GetInt();
            int64_t res = a0 - a1;
            if (res > INT32_MAX || res < INT32_MIN) {
                return JSTaggedNumber(static_cast<double>(res));
            }
            return JSTaggedNumber(static_cast<int>(res));
        }
        return JSTaggedNumber(GetNumber() - number.GetNumber());
    }

    JSTaggedNumber operator*(JSTaggedNumber number) const
    {
        if (IsInt() && number.IsInt()) {
            int64_t intA = GetInt();
            int64_t intB = number.GetInt();
            int64_t res = intA * intB;
            if (res > INT32_MAX || res < INT32_MIN) {
                return JSTaggedNumber(static_cast<double>(res));
            }
            return JSTaggedNumber(static_cast<int>(res));
        }
        return JSTaggedNumber(GetNumber() * number.GetNumber());
    }

    JSTaggedNumber operator++() const
    {
        if (IsInt()) {
            int32_t value = GetInt();
            if (value == INT32_MAX) {
                return JSTaggedNumber(static_cast<double>(value) + 1.0);
            }
            return JSTaggedNumber(value + 1);
        }
        return JSTaggedNumber(GetDouble() + 1.0);
    }

    JSTaggedNumber operator--() const
    {
        if (IsInt()) {
            int32_t value = GetInt();
            if (value == INT32_MIN) {
                return JSTaggedNumber(static_cast<double>(value) - 1.0);
            }
            return JSTaggedNumber(value - 1);
        }
        return JSTaggedNumber(GetDouble() - 1.0);
    }

    inline bool operator!=(const JSTaggedNumber &number) const
    {
        return GetNumber() != number.GetNumber();
    }

    /* static */
    inline static bool SameValue(JSTaggedNumber x, JSTaggedNumber y)
    {
        double xValue = x.GetNumber();
        double yValue = y.GetNumber();
        // SameNumberValue(NaN, NaN) is true.
        if (xValue != yValue) {
            return std::isnan(xValue) && std::isnan(yValue);
        }
        // SameNumberValue(0.0, -0.0) is false.
        return (std::signbit(xValue) == std::signbit(yValue));
    }

    inline static JSTaggedNumber FromIntOrDouble(JSThread *thread, JSTaggedValue tagged)
    {
        if (tagged.IsInt() || tagged.IsDouble()) {
            return JSTaggedNumber(tagged);
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert value to a number", JSTaggedNumber::Exception());
    }

private:
    constexpr explicit JSTaggedNumber(JSTaggedType v) : JSTaggedValue(v) {}
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JS_NUMBER_H
