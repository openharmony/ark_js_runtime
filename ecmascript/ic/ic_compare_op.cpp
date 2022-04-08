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

#include "ecmascript/ic/ic_compare_op.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/tagged_object-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
JSTaggedValue CompareOp::EqualWithIC(JSThread* thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, EqualWithIC);
    double leftDouble = 0;
    double rightDouble = 0;
    JSTaggedValue ret = JSTaggedValue::False();
    switch (operationType) {
        case CompareOpType::NUMBER_NUMBER: {
            leftDouble = left.GetNumber();
            rightDouble = right.GetNumber();
            ret = JSTaggedValue(JSTaggedValue::StrictNumberEquals(leftDouble, rightDouble));
            break;
        }
        case CompareOpType::STRING_NUMBER: {
            JSTaggedValue temp = left;
            left = right;
            right = temp;
            [[fallthrough]];
        }
        case CompareOpType::NUMBER_STRING: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            rightDouble = JSTaggedValue::ToNumber(thread, rightHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            leftDouble = leftHandle.GetTaggedValue().GetNumber();
            ret = JSTaggedValue(JSTaggedValue::StrictNumberEquals(leftDouble, rightDouble));
            break;
        }
        case CompareOpType::BOOLEAN_NUMBER: {
            JSTaggedValue temp = left;
            left = right;
            right = temp;
            [[fallthrough]];
        }
        case CompareOpType::NUMBER_BOOLEAN: {
            leftDouble = left.GetNumber();
            if (right.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue(JSTaggedValue::StrictNumberEquals(leftDouble, rightDouble));
            break;
        }
        case CompareOpType::OBJ_NUMBER: {
            JSTaggedValue temp = left;
            left = right;
            right = temp;
            [[fallthrough]];
        }
        case CompareOpType::NUMBER_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            if (rightPrimitive->IsNumber()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
            } else if (rightPrimitive->IsString()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_STRING);
            } else if (rightPrimitive->IsBoolean()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            }
            break;
        }
        case CompareOpType::STRING_STRING: {
            bool result = EcmaString::StringsAreEqual(static_cast<EcmaString *>(left.GetTaggedObject()),
                static_cast<EcmaString *>(right.GetTaggedObject()));
            ret = result ? JSTaggedValue::True() : JSTaggedValue::False();
            break;
        }
        case CompareOpType::BOOLEAN_STRING: {
            JSTaggedValue temp = left;
            left = right;
            right = temp;
            [[fallthrough]];
        }
        case CompareOpType::STRING_BOOLEAN: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            leftDouble = JSTaggedValue::ToNumber(thread, leftHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            if (rightHandle.GetTaggedValue().GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue(JSTaggedValue::StrictNumberEquals(leftDouble, rightDouble));
            break;
        }
        case CompareOpType::OBJ_STRING: {
            JSTaggedValue temp = left;
            left = right;
            right = temp;
            [[fallthrough]];
        }
        case CompareOpType::STRING_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            if (rightPrimitive->IsNumber()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_STRING);
            } else if (rightPrimitive->IsString()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::STRING_STRING);
            } else if (rightPrimitive->IsBoolean()) {
                ret = EqualWithIC(thread, leftHandle.GetTaggedValue(),
                                  rightPrimitive.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            }
            break;
        }
        case CompareOpType::BOOLEAN_BOOLEAN: {
            if (left.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                leftDouble = 1;
            }
            if (right.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue(JSTaggedValue::StrictNumberEquals(leftDouble, rightDouble));
            break;
        }
        case CompareOpType::OBJ_BOOLEAN: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> leftPrimitive(thread, JSTaggedValue::ToPrimitive(thread, leftHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            if (leftPrimitive->IsNumber()) {
                ret = EqualWithIC(thread, leftPrimitive.GetTaggedValue(),
                                  rightHandle.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            } else if (leftPrimitive->IsString()) {
                ret = EqualWithIC(thread, leftPrimitive.GetTaggedValue(),
                                  rightHandle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            } else if (leftPrimitive->IsBoolean()) {
                ret = EqualWithIC(thread, leftPrimitive.GetTaggedValue(),
                                  rightHandle.GetTaggedValue(), CompareOpType::BOOLEAN_BOOLEAN);
            }
            break;
        }
        case CompareOpType::BOOLEAN_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
            if (rightPrimitive->IsNumber()) {
                ret = EqualWithIC(thread, rightPrimitive.GetTaggedValue(),
                                  leftHandle.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            } else if (rightPrimitive->IsString()) {
                ret = EqualWithIC(thread, rightPrimitive.GetTaggedValue(),
                                  leftHandle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            } else if (rightPrimitive->IsBoolean()) {
                ret = EqualWithIC(thread, rightPrimitive.GetTaggedValue(),
                                  leftHandle.GetTaggedValue(), CompareOpType::BOOLEAN_BOOLEAN);
            }
            break;
        }
        case CompareOpType::OBJ_OBJ: {
            // if same type, must call Type::StrictEqual()
            JSType xType = left.GetTaggedObject()->GetClass()->GetObjectType();
            JSType yType = right.GetTaggedObject()->GetClass()->GetObjectType();
            bool resultObj = false;
            if (xType == yType) {
                resultObj = JSTaggedValue::StrictEqual(thread, JSHandle<JSTaggedValue>(thread, left),
                    JSHandle<JSTaggedValue>(thread, right));
            }
            ret = resultObj ? JSTaggedValue::True() : JSTaggedValue::False();
            break;
        }
        case CompareOpType::SYMBOL_SYMBOL: {
            ret = left == right ? JSTaggedValue::True() : JSTaggedValue::False();
            break;
        }
        case CompareOpType::NULL_NULL:
        case CompareOpType::NULL_UNDEFINED:
        case CompareOpType::UNDEFINED_UNDEFINED:
        case CompareOpType::UNDEFINED_NULL: {
            ret = JSTaggedValue::True();
            break;
        }
        default:
            ret = JSTaggedValue::False();
    }
    return ret;
}

JSTaggedValue CompareOp::NotEqualWithIC(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, NotEqualWithIC);
    JSTaggedValue res = EqualWithIC(thread, left, right, operationType);
    return res == JSTaggedValue::True() ? JSTaggedValue::False() : JSTaggedValue::True();
}

ComparisonResult CompareOp::Compare(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, Compare);
    double leftDouble = 0;
    double rightDouble = 0;
    ComparisonResult ret = ComparisonResult::UNDEFINED;
    switch (operationType) {
        case CompareOpType::NUMBER_NUMBER: {
            leftDouble = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
            rightDouble = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }
        case CompareOpType::NUMBER_STRING: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);

            rightDouble = JSTaggedValue::ToNumber(thread, rightHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            leftDouble = leftHandle.GetTaggedValue().GetNumber();
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }
        case CompareOpType::NUMBER_BOOLEAN: {
            leftDouble = left.GetNumber();
            if (right.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }
        case CompareOpType::NUMBER_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (rightPrimitive->IsNumber()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
            } else if (rightPrimitive->IsString()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_STRING);
            } else if (rightPrimitive->IsBoolean()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            }
            break;
        }
        case CompareOpType::STRING_STRING: {
            auto xString = static_cast<EcmaString *>(left.GetTaggedObject());
            auto yString = static_cast<EcmaString *>(right.GetTaggedObject());
            int result = xString->Compare(yString);
            if (result < 0) {
                ret =  ComparisonResult::LESS;
            } else if (result == 0) {
                ret =  ComparisonResult::EQUAL;
            } else {
                ret = ComparisonResult::GREAT;
            }
            break;
        }
        case CompareOpType::STRING_NUMBER: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            leftDouble = JSTaggedValue::ToNumber(thread, leftHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            rightDouble = rightHandle.GetTaggedValue().GetNumber();
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }
        case CompareOpType::STRING_BOOLEAN: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            leftDouble = JSTaggedValue::ToNumber(thread, leftHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (rightHandle.GetTaggedValue().GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }
        case CompareOpType::STRING_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (rightPrimitive->IsNumber()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::NUMBER_STRING);
            } else if (rightPrimitive->IsString()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::STRING_STRING);
            } else if (rightPrimitive->IsBoolean()) {
                ret = Compare(thread, leftHandle.GetTaggedValue(),
                              rightPrimitive.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            }
            break;
        }
        case CompareOpType::BOOLEAN_BOOLEAN: {
            if (left.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                leftDouble = 1;
            }
            if (right.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                rightDouble = 1;
            }
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }

        case CompareOpType::BOOLEAN_NUMBER: {
            if (left.GetRawData() == JSTaggedValue::VALUE_TRUE) {
                leftDouble = 1;
            }
            rightDouble = right.GetNumber();
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }

        case CompareOpType::BOOLEAN_STRING: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            rightDouble = JSTaggedValue::ToNumber(thread, rightHandle).GetNumber();
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (leftHandle.GetTaggedValue().GetRawData() == JSTaggedValue::VALUE_TRUE) {
                leftDouble = 1;
            }
            ret = JSTaggedValue::StrictNumberCompare(leftDouble, rightDouble);
            break;
        }

        case CompareOpType::BOOLEAN_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> rightPrimitive(thread, JSTaggedValue::ToPrimitive(thread, rightHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (rightPrimitive->IsNumber()) {
                ret = Compare(thread, rightPrimitive.GetTaggedValue(),
                              leftHandle.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            } else if (rightPrimitive->IsString()) {
                ret = Compare(thread, rightPrimitive.GetTaggedValue(),
                              leftHandle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            } else if (rightPrimitive->IsBoolean()) {
                ret = Compare(thread, rightPrimitive.GetTaggedValue(),
                              leftHandle.GetTaggedValue(), CompareOpType::BOOLEAN_BOOLEAN);
            }
            break;
        }
        case CompareOpType::OBJ_OBJ: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            ret = JSTaggedValue::Compare(thread, leftHandle, rightHandle);
            break;
        }
        case CompareOpType::OBJ_NUMBER: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> leftPrimitive(thread, JSTaggedValue::ToPrimitive(thread, leftHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (leftPrimitive->IsNumber()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
            } else if (leftPrimitive->IsString()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
            } else if (leftPrimitive->IsBoolean()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::BOOLEAN_NUMBER);
            }
            break;
        }
        case CompareOpType::OBJ_STRING: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> leftPrimitive(thread, JSTaggedValue::ToPrimitive(thread, leftHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (leftPrimitive->IsNumber()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::NUMBER_STRING);
            } else if (leftPrimitive->IsString()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::STRING_STRING);
            } else if (leftPrimitive->IsBoolean()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::BOOLEAN_STRING);
            }
            break;
        }
        case CompareOpType::OBJ_BOOLEAN: {
            JSHandle<JSTaggedValue> leftHandle(thread, left);
            JSHandle<JSTaggedValue> rightHandle(thread, right);
            JSHandle<JSTaggedValue> leftPrimitive(thread, JSTaggedValue::ToPrimitive(thread, leftHandle));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            if (leftPrimitive->IsNumber()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::NUMBER_BOOLEAN);
            } else if (leftPrimitive->IsString()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
            } else if (leftPrimitive->IsBoolean()) {
                ret = Compare(thread, leftPrimitive.GetTaggedValue(),
                              rightHandle.GetTaggedValue(), CompareOpType::BOOLEAN_BOOLEAN);
            }
            break;
        }
        default:
            ret = ComparisonResult::UNDEFINED;
    }
    return ret;
}

JSTaggedValue CompareOp::LessDynWithIC(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, LessDynWithIC);
    bool ret = CompareOp::Compare(thread, left, right, operationType) == ComparisonResult::LESS;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue CompareOp::LessEqDynWithIC(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, LessEqDynWithIC);
    bool ret = CompareOp::Compare(thread, left, right, operationType) <= ComparisonResult::EQUAL;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue CompareOp::GreaterDynWithIC(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, GreaterDynWithIC);
    bool ret = CompareOp::Compare(thread, left, right, operationType) == ComparisonResult::GREAT;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue CompareOp::GreaterEqDynWithIC(JSThread *thread, JSTaggedValue left,
    JSTaggedValue right, CompareOpType operationType)
{
    INTERPRETER_TRACE(thread, GreaterEqDynWithIC);
    ComparisonResult comparison = CompareOp::Compare(thread, left, right, operationType);
    bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}
}   // namespace panda::ecmascript
