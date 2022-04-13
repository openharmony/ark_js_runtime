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

#include "ecmascript/interpreter/slow_runtime_stub.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/slow_runtime_helper.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/template_string.h"

namespace panda::ecmascript {
JSTaggedValue SlowRuntimeStub::CallSpreadDyn(JSThread *thread, JSTaggedValue func, JSTaggedValue obj,
                                             JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, CallSpreadDyn);
    if ((!obj.IsUndefined() && !obj.IsECMAObject()) || !func.IsJSFunction() || !array.IsJSArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "cannot Callspread", JSTaggedValue::Exception());
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> jsFunc(thread, func);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    JSHandle<JSTaggedValue> taggedObj(thread, obj);

    JSHandle<TaggedArray> coretypesArray(thread, GetCallSpreadArgs(thread, jsArray.GetTaggedValue()));
    const size_t argsLength = coretypesArray->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(jsFunc), taggedObj, undefined, argsLength);
    info.SetCallArg(argsLength, coretypesArray);
    return EcmaInterpreter::Execute(&info);
}

JSTaggedValue SlowRuntimeStub::NegDyn(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, NegDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::UnaryMinus(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    if (number.IsInt()) {
        int32_t intValue = number.GetInt();
        if (intValue == 0) {
            return JSTaggedValue(-0.0);
        }
        return JSTaggedValue(-intValue);
    }
    if (number.IsDouble()) {
        return JSTaggedValue(-number.GetDouble());
    }
    UNREACHABLE();
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionEnter(JSThread *thread)
{
    INTERPRETER_TRACE(thread, AsyncFunctionEnter);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. create promise
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = globalEnv->GetPromiseFunction();

    JSHandle<JSPromise> promiseObject =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    promiseObject->SetPromiseState(PromiseState::PENDING);
    // 2. create asyncfuncobj
    JSHandle<JSAsyncFuncObject> asyncFuncObj = factory->NewJSAsyncFuncObject();
    asyncFuncObj->SetPromise(thread, promiseObject);

    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, asyncFuncObj);

    // change state to EXECUTING
    asyncFuncObj->SetGeneratorState(JSGeneratorState::EXECUTING);
    asyncFuncObj->SetGeneratorContext(thread, context);

    // 3. return asyncfuncobj
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return asyncFuncObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::ToNumber(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, Tonumber);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> number(thread, value);
    // may return exception
    return JSTaggedValue::ToNumeric(thread, number.GetTaggedValue()).GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::NotDyn(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, NotDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BitwiseNOT(thread, bigValue).GetTaggedValue();
    }
    int32_t number = JSTaggedValue::ToInt32(thread, inputVal);
    return JSTaggedValue(~number); // NOLINT(hicpp-signed-bitwise)
}

JSTaggedValue SlowRuntimeStub::IncDyn(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, IncDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintAddOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(++number);
}

JSTaggedValue SlowRuntimeStub::DecDyn(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, DecDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintSubOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(--number);
}

void SlowRuntimeStub::ThrowDyn(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ThrowDyn);
    thread->SetException(value);
}

JSTaggedValue SlowRuntimeStub::GetPropIterator(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, GetPropIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, value);
    JSHandle<JSForInIterator> iteratorHandle = JSObject::EnumerateObjectProperties(thread, objHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iteratorHandle.GetTaggedValue();
}

void SlowRuntimeStub::ThrowConstAssignment(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ThrowConstAssignment);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> name(thread, value.GetTaggedObject());
    JSHandle<EcmaString> info = factory->NewFromASCII("Assignment to const variable ");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, name);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::Add2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Add2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    if (leftValue->IsString() && rightValue->IsString()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(JSHandle<EcmaString>(leftValue), JSHandle<EcmaString>(rightValue));
        return newString.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> primitiveA0(thread, JSTaggedValue::ToPrimitive(thread, leftValue));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> primitiveA1(thread, JSTaggedValue::ToPrimitive(thread, rightValue));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // contain string
    if (primitiveA0->IsString() || primitiveA1->IsString()) {
        JSHandle<EcmaString> stringA0 = JSTaggedValue::ToString(thread, primitiveA0);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<EcmaString> stringA1 = JSTaggedValue::ToString(thread, primitiveA1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> newString = factory->ConcatFromString(stringA0, stringA1);
        return newString.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, primitiveA0.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, primitiveA1.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Add(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double a0Double = valLeft->GetNumber();
    double a1Double = valRight->GetNumber();
    return JSTaggedValue(a0Double + a1Double);
}

JSTaggedValue SlowRuntimeStub::Sub2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Sub2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Subtract(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 - number1;
}

JSTaggedValue SlowRuntimeStub::Mul2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mul2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9. ReturnIfAbrupt(rnum).
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Multiply(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    // 12.6.3.1 Applying the * Operator
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 * number1;
}

JSTaggedValue SlowRuntimeStub::Div2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Div2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Divide(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    if (dRight == 0) {
        if (dLeft == 0 || std::isnan(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        bool positive = (((bit_cast<uint64_t>(dRight)) & base::DOUBLE_SIGN_MASK) ==
                         ((bit_cast<uint64_t>(dLeft)) & base::DOUBLE_SIGN_MASK));
        return JSTaggedValue(positive ? base::POSITIVE_INFINITY : -base::POSITIVE_INFINITY);
    }
    return JSTaggedValue(dLeft / dRight);
}

JSTaggedValue SlowRuntimeStub::Mod2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mod2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12.6.3.3 Applying the % Operator
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::Remainder(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    // 12.6.3.3 Applying the % Operator
    if ((dRight == 0.0) || std::isnan(dRight) || std::isnan(dLeft) || std::isinf(dLeft)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if ((dLeft == 0.0) || std::isinf(dRight)) {
        return JSTaggedValue(dLeft);
    }
    return JSTaggedValue(std::fmod(dLeft, dRight));
}

JSTaggedValue SlowRuntimeStub::EqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, EqDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    bool ret = JSTaggedValue::Equal(thread, leftValue, rightValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue SlowRuntimeStub::NotEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, NotEqDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    bool ret = JSTaggedValue::Equal(thread, leftValue, rightValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::False() : JSTaggedValue::True());
}

JSTaggedValue SlowRuntimeStub::LessDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, LessDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    bool ret = JSTaggedValue::Compare(thread, leftValue, rightValue) == ComparisonResult::LESS;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue SlowRuntimeStub::LessEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, LessEqDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    bool ret = JSTaggedValue::Compare(thread, leftValue, rightValue) <= ComparisonResult::EQUAL;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue SlowRuntimeStub::GreaterDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, GreaterDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    bool ret = JSTaggedValue::Compare(thread, leftValue, rightValue) == ComparisonResult::GREAT;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue SlowRuntimeStub::GreaterEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, GreaterEqDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    ComparisonResult comparison = JSTaggedValue::Compare(thread, leftValue, rightValue);
    bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue SlowRuntimeStub::Shl2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Shl2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> rightValue = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (leftValue->IsBigInt() || rightValue->IsBigInt()) {
        if (leftValue->IsBigInt() && rightValue->IsBigInt()) {
            JSHandle<BigInt> leftBigint(leftValue);
            JSHandle<BigInt> rightBigint(rightValue);
            return BigInt::LeftShift(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, leftValue.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithUint32(thread, rightValue.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift =
        static_cast<uint32_t>(opNumber1) & 0x1f;  // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<int32_t>;
    auto ret =
        static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift);  // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::Shr2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Shr2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            return BigInt::UnsignedRightShift(thread);
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithUint32(thread, valLeft.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithUint32(thread, valRight.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift = static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<uint32_t>;
    auto ret =
        static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::Ashr2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Ashr2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::SignedRightShift(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valLeft.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithUint32(thread, valRight.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    uint32_t shift = static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::And2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, And2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseAND(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valLeft.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valRight.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue SlowRuntimeStub::Or2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Or2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseOR(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valLeft.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valRight.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue SlowRuntimeStub::Xor2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Xor2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::BitwiseXOR(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return ThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedValue taggedNumber0 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valLeft.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue taggedNumber1 = SlowRuntimeStub::ToJSTaggedValueWithInt32(thread, valRight.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t opNumber0 = taggedNumber0.GetInt();
    int32_t opNumber1 = taggedNumber1.GetInt();
    // NOLINT(hicpp-signed-bitwise)
    auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
    return JSTaggedValue(static_cast<int32_t>(ret));
}

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithInt32(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ToJSTaggedValueWithInt32);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    int32_t res = JSTaggedValue::ToInt32(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithUint32(JSThread *thread, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, ToJSTaggedValueWithUint32);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    int32_t res = JSTaggedValue::ToUint32(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue SlowRuntimeStub::DelObjProp(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, Delobjprop);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> jsObj(JSTaggedValue::ToObject(thread, objHandle));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, propHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::DeletePropertyOrThrow(thread, jsObj, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::NewObjDynRange(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                              uint16_t firstArgIdx, uint16_t length)
{
    INTERPRETER_TRACE(thread, NewobjDynrange);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);

    JSHandle<JSTaggedValue> preArgs(thread, JSTaggedValue::Undefined());
    auto tagged = SlowRuntimeHelper::Construct(thread, funcHandle, newTargetHandle, preArgs, length, firstArgIdx);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return tagged;
}

JSTaggedValue SlowRuntimeStub::CreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys, JSTaggedValue objVal,
                                                            uint16_t firstArgRegIdx)
{
    INTERPRETER_TRACE(thread, CreateObjectWithExcludedKeys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    ASSERT(objVal.IsJSObject());
    JSHandle<JSObject> obj(thread, objVal);
    uint32_t numExcludedKeys = 0;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys + 1);
    InterpretedFrameHandler frameHandler(thread);
    JSTaggedValue excludedKey = frameHandler.GetVRegValue(firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys + 1;
        excludedKeys->Set(thread, 0, excludedKey);
        for (uint32_t i = 1; i < numExcludedKeys; i++) {
            excludedKey = frameHandler.GetVRegValue(firstArgRegIdx + i);
            excludedKeys->Set(thread, i, excludedKey);
        }
    }

    uint32_t numAllKeys = obj->GetNumberOfKeys();
    JSHandle<TaggedArray> allKeys = factory->NewTaggedArray(numAllKeys);
    JSObject::GetAllKeys(thread, obj, 0, allKeys);

    JSHandle<JSObject> restObj = factory->NewEmptyJSObject();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < numAllKeys; i++) {
        key.Update(allKeys->Get(i));
        bool isExcludedKey = false;
        for (uint32_t j = 0; j < numExcludedKeys; j++) {
            if (JSTaggedValue::Equal(thread, key, JSHandle<JSTaggedValue>(thread, excludedKeys->Get(j)))) {
                isExcludedKey = true;
                break;
            }
        }
        if (!isExcludedKey) {
            PropertyDescriptor desc(thread);
            bool success = JSObject::GetOwnProperty(thread, obj, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (success && desc.IsEnumerable()) {
                JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, obj, key).GetValue();
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSObject::SetProperty(thread, restObj, key, value, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return restObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::ExpDyn(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent)
{
    INTERPRETER_TRACE(thread, ExpDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valBase = JSTaggedValue::ToNumeric(thread, base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valExponent = JSTaggedValue::ToNumeric(thread, exponent);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valBase->IsBigInt() || valExponent->IsBigInt()) {
        if (valBase->IsBigInt() && valExponent->IsBigInt()) {
            JSHandle<BigInt> bigBaseVale(valBase);
            JSHandle<BigInt> bigExponentValue(valExponent);
            return  BigInt::Exponentiate(thread, bigBaseVale, bigExponentValue).GetTaggedValue();
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot mix BigInt and other types, use explicit conversions",
                                    JSTaggedValue::Exception());
    }
    double doubleBase = valBase->GetNumber();
    double doubleExponent = valExponent->GetNumber();
    if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if (((doubleBase == 0) && ((bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
        std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
        base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF == (doubleExponent / 2)) {  // 2: half
        if (doubleExponent > 0) {
            return JSTaggedValue(-0.0);
        }
        if (doubleExponent < 0) {
            return JSTaggedValue(-base::POSITIVE_INFINITY);
        }
    }
    return JSTaggedValue(std::pow(doubleBase, doubleExponent));
}

JSTaggedValue SlowRuntimeStub::IsInDyn(JSThread *thread, JSTaggedValue prop, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, IsInDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    if (!objHandle->IsECMAObject()) {
        return ThrowTypeError(thread, "Cannot use 'in' operator in Non-Object");
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, propHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::HasProperty(thread, objHandle, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::InstanceofDyn(JSThread *thread, JSTaggedValue obj, JSTaggedValue target)
{
    INTERPRETER_TRACE(thread, InstanceofDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> targetHandle(thread, target);
    bool ret = JSObject::InstanceOf(thread, objHandle, targetHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue SlowRuntimeStub::NewLexicalEnvDyn(JSThread *thread, uint16_t numVars)
{
    INTERPRETER_TRACE(thread, NewlexenvDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    newEnv->SetScopeInfo(thread, JSTaggedValue::Hole());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::NewLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId)
{
    INTERPRETER_TRACE(thread, NewlexenvwithNameDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    JSTaggedValue scopeInfo = ScopeInfoExtractor::GenerateScopeInfo(thread, scopeId);
    newEnv->SetScopeInfo(thread, scopeInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateIterResultObj(JSThread *thread, JSTaggedValue value, JSTaggedValue flag)
{
    INTERPRETER_TRACE(thread, CreateIterResultObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valueHandle(thread, value);
    ASSERT(flag.IsBoolean());
    bool done = flag.IsTrue();
    JSHandle<JSObject> iter = JSIterator::CreateIterResultObject(thread, valueHandle, done);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateGeneratorObj(JSThread *thread, JSTaggedValue genFunc)
{
    INTERPRETER_TRACE(thread, CreateGeneratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> generatorFunction(thread, genFunc);
    JSHandle<JSGeneratorObject> obj = factory->NewJSGeneratorObject(generatorFunction);
    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, obj.GetTaggedValue());

    // change state to SUSPENDED_START
    obj->SetGeneratorState(JSGeneratorState::SUSPENDED_START);
    obj->SetGeneratorContext(thread, context);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return obj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SuspendGenerator(JSThread *thread, JSTaggedValue genObj, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SuspendGenerator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSGeneratorObject> generatorObjectHandle(thread, genObj);
    JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    // save stack, should copy cur_frame, function execute over will free cur_frame
    SlowRuntimeHelper::SaveFrameToContext(thread, genContextHandle);

    // change state to SuspendedYield
    if (generatorObjectHandle->IsExecuting()) {
        generatorObjectHandle->SetGeneratorState(JSGeneratorState::SUSPENDED_YIELD);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return valueHandle.GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return generatorObjectHandle.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionAwaitUncaught(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                          JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, AsyncFunctionAwaitUncaught);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSAsyncFunction::AsyncFunctionAwait(thread, asyncFuncObjHandle, valueHandle);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::AsyncFunctionResolveOrReject(JSThread *thread, JSTaggedValue asyncFuncObj,
                                                            JSTaggedValue value, bool is_resolve)
{
    INTERPRETER_TRACE(thread, AsyncFunctionResolveOrReject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    // ActivePromise
    JSHandle<ResolvingFunctionsRecord> reactions = JSPromise::CreateResolvingFunctions(thread, promise);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> activeFunc;
    if (is_resolve) {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetResolveFunction());
    } else {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetRejectFunction());
    }
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, activeFunc, thisArg, undefined, 1);
    info.SetCallArg(valueHandle.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(&info);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::NewObjSpreadDyn(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                               JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, NewobjspreadDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    if (!jsArray->IsJSArray()) {
        return ThrowTypeError(thread, "Cannot Newobjspread");
    }

    uint32_t length = JSHandle<JSArray>::Cast(jsArray)->GetArrayLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, funcHandle, undefined, newTargetHandle, length);
    for (size_t i = 0; i < length; i++) {
        auto prop = JSTaggedValue::GetProperty(thread, jsArray, i).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info.SetCallArg(i, prop.GetTaggedValue());
    }
    return SlowRuntimeHelper::NewObject(&info);
}

void SlowRuntimeStub::ThrowUndefinedIfHole(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, ThrowUndefinedIfHole);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> name(thread, obj);
    JSHandle<EcmaString> info = factory->NewFromASCII(" is not initialized");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, name);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::ThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index, JSTaggedValue thisValue)
{
    INTERPRETER_TRACE(thread, ThrowIfSuperNotCorrectCall);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    if (index == 0 && (thisValue.IsUndefined() || thisValue.IsHole())) {
        return ThrowReferenceError(thread, JSTaggedValue::Undefined(), "sub-class must call super before use 'this'");
    }
    if (index == 1 && !thisValue.IsUndefined() && !thisValue.IsHole()) {
        return ThrowReferenceError(thread, JSTaggedValue::Undefined(), "super() forbidden re-bind 'this'");
    }
    return JSTaggedValue::True();
}

void SlowRuntimeStub::ThrowIfNotObject(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowIfNotObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    THROW_TYPE_ERROR(thread, "Inner return result is not object");
}

void SlowRuntimeStub::ThrowThrowNotExists(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowThrowNotExists);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    THROW_TYPE_ERROR(thread, "Throw method is not defined");
}

void SlowRuntimeStub::ThrowPatternNonCoercible(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowPatternNonCoercible);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> msg(thread->GlobalConstants()->GetHandledObjNotCoercibleString());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::StOwnByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByNameDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    ASSERT(propHandle->IsStringOrSymbol());

    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, propHandle, desc);
    if (!ret) {
        return ThrowTypeError(thread, "SetOwnByName failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::StOwnByNameWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                      JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByNameDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    ASSERT(propHandle->IsStringOrSymbol());

    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, propHandle);

    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, propHandle, desc);
    if (!ret) {
        return ThrowTypeError(thread, "SetOwnByNameWithNameSet failed");
    }
    JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(valueHandle), propKey,
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::StOwnByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByIdDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> idxHandle(thread, JSTaggedValue(idx));
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, idxHandle, desc);
    if (!ret) {
        return ThrowTypeError(thread, "SetOwnByIndex failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::StOwnByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    INTERPRETER_TRACE(thread, StOwnByValueDyn);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    if (objHandle->IsClassConstructor() &&
        JSTaggedValue::SameValue(keyHandle, globalConst->GetHandledPrototypeString())) {
        return ThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    JSMutableHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, keyHandle));
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, propKey, desc);
    if (!ret) {
        return ThrowTypeError(thread, "StOwnByValue failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::StOwnByValueWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                       JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    INTERPRETER_TRACE(thread, StOwnByValueDyn);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    if (objHandle->IsClassConstructor() &&
        JSTaggedValue::SameValue(keyHandle, globalConst->GetHandledPrototypeString())) {
        return ThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(objHandle->IsClassPrototype() || objHandle->IsClassConstructor());

    PropertyDescriptor desc(thread, valueHandle, true, enumerable, true);
    JSMutableHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, keyHandle));
    bool ret = JSTaggedValue::DefineOwnProperty(thread, objHandle, propKey, desc);
    if (!ret) {
        return ThrowTypeError(thread, "StOwnByValueWithNameSet failed");
    }
    if (valueHandle->IsJSFunction()) {
        if (propKey->IsNumber()) {
            propKey.Update(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()).GetTaggedValue());
        }
        JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(valueHandle), propKey,
                                        JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    }
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::CreateEmptyArray(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv)
{
    INTERPRETER_TRACE(thread, CreateEmptyArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> builtinObj(globalEnv->GetArrayFunction());
    JSHandle<JSObject> arr = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return arr.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateEmptyObject(JSThread *thread, ObjectFactory *factory,
                                                 JSHandle<GlobalEnv> globalEnv)
{
    INTERPRETER_TRACE(thread, CreateEmptyObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> builtinObj(globalEnv->GetObjectFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return obj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory, JSObject *literal)
{
    INTERPRETER_TRACE(thread, CreateObjectWithBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory, JSObject *literal,
                                                        JSTaggedValue env, ConstantPool *constpool)
{
    INTERPRETER_TRACE(thread, CreateObjectHavingMethod);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(
        obj, JSHandle<JSTaggedValue>(thread, env), JSHandle<JSTaggedValue>(thread, JSTaggedValue(constpool)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SetObjectWithProto(JSThread *thread, JSTaggedValue proto, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, SetObjectWithProto);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    if (!proto.IsECMAObject() && !proto.IsNull()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSTaggedValue> protoHandle(thread, proto);
    JSHandle<JSObject> objHandle(thread, obj);
    JSObject::SetPrototype(thread, objHandle, protoHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::IterNext(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, IterNext);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    JSHandle<JSObject> resultObj = JSIterator::IteratorNext(thread, iterHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return resultObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CloseIterator(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, CloseIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    JSHandle<JSTaggedValue> record;
    if (thread->HasPendingException()) {
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(
            CompletionRecordType::THROW, JSHandle<JSTaggedValue>(thread, thread->GetException())));
    } else {
        JSHandle<JSTaggedValue> undefinedVal = globalConst->GetHandledUndefined();
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(CompletionRecordType::NORMAL, undefinedVal));
    }
    JSHandle<JSTaggedValue> result = JSIterator::IteratorClose(thread, iterHandle, record);
    if (result->IsCompletionRecord()) {
        return CompletionRecord::Cast(result->GetTaggedObject())->GetValue();
    }
    return result.GetTaggedValue();
}

void SlowRuntimeStub::StModuleVar([[maybe_unused]] JSThread *thread, [[maybe_unused]] JSTaggedValue key,
                                  [[maybe_unused]] JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StModuleVar);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    thread->GetEcmaVM()->GetModuleManager()->StoreModuleValue(key, value);
}

JSTaggedValue SlowRuntimeStub::LdModuleVar([[maybe_unused]] JSThread *thread,
                                           [[maybe_unused]] JSTaggedValue key,
                                           [[maybe_unused]] bool inner)
{
    INTERPRETER_TRACE(thread, LdModuleVar);
    [[maybe_unused]] EcmaHandleScope scope(thread);
    if (inner) {
        JSTaggedValue moduleValue = thread->GetEcmaVM()->GetModuleManager()->GetModuleValueInner(key);
        return moduleValue;
    }

    return thread->GetEcmaVM()->GetModuleManager()->GetModuleValueOutter(key);
}

JSTaggedValue SlowRuntimeStub::CreateRegExpWithLiteral(JSThread *thread, JSTaggedValue pattern, uint8_t flags)
{
    INTERPRETER_TRACE(thread, CreateRegExpWithLiteral);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> patternHandle(thread, pattern);
    JSHandle<JSTaggedValue> flagsHandle(thread, JSTaggedValue(flags));

    return builtins::BuiltinsRegExp::RegExpCreate(thread, patternHandle, flagsHandle);
}

JSTaggedValue SlowRuntimeStub::CreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory, JSArray *literal)
{
    INTERPRETER_TRACE(thread, CreateArrayWithBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSArray> array(thread, literal);
    JSHandle<JSArray> arrLiteral = factory->CloneArrayLiteral(array);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return arrLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetTemplateObject(JSThread *thread, JSTaggedValue literal)
{
    INTERPRETER_TRACE(thread, GetTemplateObject);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> templateLiteral(thread, literal);
    JSHandle<JSTaggedValue> templateObj = TemplateString::GetTemplateObject(thread, templateLiteral);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return templateObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetNextPropName(JSThread *thread, JSTaggedValue iter)
{
    INTERPRETER_TRACE(thread, GetNextPropName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterator(thread, iter);
    ASSERT(iterator->IsForinIterator());
    std::pair<JSTaggedValue, bool> res =
        JSForInIterator::NextInternal(thread, JSHandle<JSForInIterator>::Cast(iterator));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.first;
}

JSTaggedValue SlowRuntimeStub::CopyDataProperties(JSThread *thread, JSTaggedValue dst, JSTaggedValue src)
{
    INTERPRETER_TRACE(thread, CopyDataProperties);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    if (!srcHandle->IsNull() && !srcHandle->IsUndefined()) {
        JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, srcHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        uint32_t keysLen = keys->GetLength();
        for (uint32_t i = 0; i < keysLen; i++) {
            PropertyDescriptor desc(thread);
            key.Update(keys->Get(i));
            bool success = JSTaggedValue::GetOwnProperty(thread, srcHandle, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            if (success && desc.IsEnumerable()) {
                JSTaggedValue::DefineOwnProperty(thread, dstHandle, key, desc);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return dstHandle.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetIteratorNext(JSThread *thread, JSTaggedValue obj, JSTaggedValue method)
{
    INTERPRETER_TRACE(thread, GetIteratorNext);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iter(thread, obj);
    JSHandle<JSTaggedValue> next(thread, method);

    ASSERT(next->IsCallable());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, next, iter, undefined, 0);
    JSTaggedValue ret = JSFunction::Call(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret.IsECMAObject()) {
        return ThrowTypeError(thread, "the Iterator is not an ecmaobject.");
    }
    return ret;
}

JSTaggedValue SlowRuntimeStub::GetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                              uint32_t startIdx)
{
    INTERPRETER_TRACE(thread, GetUnmapedArgs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs);
    for (uint32_t i = 0; i < actualNumArgs; ++i) {
        argumentsList->Set(thread, i,
                           JSTaggedValue(sp[startIdx + i]));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    // 1. Let len be the number of elements in argumentsList
    int32_t len = argumentsList->GetLength();
    // 2. Let obj be ObjectCreate(%ObjectPrototype%, «[[ParameterMap]]»).
    // 3. Set obj’s [[ParameterMap]] internal slot to undefined.
    JSHandle<JSArguments> obj = factory->NewJSArguments();
    // 4. Perform DefinePropertyOrThrow(obj, "length", PropertyDescriptor{[[Value]]: len, [[Writable]]: true,
    // [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(len));
    // 5. Let index be 0.
    // 6. Repeat while index < len,
    //    a. Let val be argumentsList[index].
    //    b. Perform CreateDataProperty(obj, ToString(index), val).
    //    c. Let index be index + 1
    obj->SetElements(thread, argumentsList.GetTaggedValue());
    // 7. Perform DefinePropertyOrThrow(obj, @@iterator, PropertyDescriptor
    // {[[Value]]:%ArrayProto_values%,
    // [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 globalEnv->GetArrayProtoValuesFunction().GetTaggedValue());
    // 8. Perform DefinePropertyOrThrow(obj, "caller", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> throwFunction = globalEnv->GetThrowTypeError();
    JSHandle<AccessorData> accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLER_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    // 9. Perform DefinePropertyOrThrow(obj, "callee", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLEE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return obj
    return obj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs, uint32_t startIdx)
{
    INTERPRETER_TRACE(thread, Copyrestargs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> restArray = JSArray::ArrayCreate(thread, JSTaggedNumber(restNumArgs));

    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < restNumArgs; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        element.Update(JSTaggedValue(sp[startIdx + i]));
        JSObject::SetProperty(thread, restArray, i, element, true);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return restArray.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetIterator(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, GetIterator);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> valuesFunc =
        JSTaggedValue::GetProperty(thread, objHandle, env->GetIteratorSymbol()).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!valuesFunc->IsCallable()) {
        return valuesFunc.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, valuesFunc, objHandle, undefined, 0);
    return EcmaInterpreter::Execute(&info);
}

JSTaggedValue SlowRuntimeStub::DefineGetterSetterByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                         JSTaggedValue getter, JSTaggedValue setter, bool flag)
{
    INTERPRETER_TRACE(thread, DefineGetterSetterByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSObject> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);

    JSHandle<JSTaggedValue> getterHandle(thread, getter);
    JSHandle<JSTaggedValue> setterHandle(thread, setter);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, propHandle);

    auto globalConst = thread->GlobalConstants();
    if (objHandle.GetTaggedValue().IsClassConstructor() &&
        JSTaggedValue::SameValue(propKey, globalConst->GetHandledPrototypeString())) {
        return ThrowTypeError(
            thread,
            "In a class, computed property names for static getter that are named 'prototype' throw a TypeError");
    }

    if (flag) {
        if (!getterHandle->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(getterHandle), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetGetString()));
        }

        if (!setterHandle->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(setterHandle), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetSetString()));
        }
    }

    // set accessor
    bool enumerable =
        !(objHandle.GetTaggedValue().IsClassPrototype() || objHandle.GetTaggedValue().IsClassConstructor());
    PropertyDescriptor desc(thread, true, enumerable, true);
    if (!getterHandle->IsUndefined()) {
        JSHandle<JSFunction>::Cast(getterHandle)->SetFunctionKind(FunctionKind::GETTER_FUNCTION);
        desc.SetGetter(getterHandle);
    }
    if (!setterHandle->IsUndefined()) {
        JSHandle<JSFunction>::Cast(setterHandle)->SetFunctionKind(FunctionKind::SETTER_FUNCTION);
        desc.SetSetter(setterHandle);
    }
    JSObject::DefineOwnProperty(thread, objHandle, propKey, desc);

    return objHandle.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::LdObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, bool callGetter,
                                            JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByIndexDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedValue res;
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), objHandle);
    } else {
        res = JSTaggedValue::GetProperty(thread, objHandle, idx).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue SlowRuntimeStub::StObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByIndexDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, obj), idx,
                               JSHandle<JSTaggedValue>(thread, value), true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::LdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                           JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByNameDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), objHandle);
    } else {
        JSHandle<JSTaggedValue> propHandle(thread, prop);
        res = JSTaggedValue::GetProperty(thread, objHandle, propHandle).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue SlowRuntimeStub::StObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByNameDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSTaggedValue::SetProperty(thread, objHandle, propHandle, valueHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::LdObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                            JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByValueDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), objHandle);
    } else {
        JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, JSHandle<JSTaggedValue>(thread, prop));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        res = JSTaggedValue::GetProperty(thread, objHandle, propKey).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue SlowRuntimeStub::StObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                            JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByValueDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, propHandle));

    // strict mode is true
    JSTaggedValue::SetProperty(thread, objHandle, propKey, valueHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::TryLdGlobalByName(JSThread *thread, JSTaggedValue global, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, Trygetobjprop);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj(thread, global.GetTaggedObject()->GetClass()->GetPrototype());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    OperationResult res = JSTaggedValue::GetProperty(thread, obj, propHandle);
    if (!res.GetPropertyMetaData().IsFound()) {
        return ThrowReferenceError(thread, prop, " is not defined");
    }
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::TryStGlobalByName(JSThread *thread, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, TryStGlobalByName);
    // If fast path is fail, not need slow path, just throw error.
    return ThrowReferenceError(thread, prop, " is not defined");
}

JSTaggedValue SlowRuntimeStub::LdGlobalVar(JSThread *thread, JSTaggedValue global, JSTaggedValue prop)
{
    INTERPRETER_TRACE(thread, LdGlobalVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, global.GetTaggedObject()->GetClass()->GetPrototype());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    OperationResult res = JSTaggedValue::GetProperty(thread, objHandle, propHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::StGlobalVar(JSThread *thread, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StGlobalVar);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> global(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    JSObject::GlobalSetProperty(thread, propHandle, valueHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::TryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, TryUpdateGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(prop);
    ASSERT(entry != -1);

    if (dict->GetAttributes(entry).IsConstProps()) {
        return ThrowSyntaxError(thread, "const variable can not be modified");
    }

    PropertyBox *box = dict->GetBox(entry);
    box->SetValue(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

// return box
JSTaggedValue SlowRuntimeStub::LdGlobalRecord(JSThread *thread, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, LdGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return JSTaggedValue(dict->GetBox(entry));
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue SlowRuntimeStub::StGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value, bool isConst)
{
    INTERPRETER_TRACE(thread, StGlobalRecord);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());

    // cross files global record name binding judgment
    int entry = dict->FindEntry(prop);
    if (entry != -1) {
        return ThrowSyntaxError(thread, "Duplicate identifier");
    }

    PropertyAttributes attributes;
    if (isConst) {
        attributes.SetIsConstProps(true);
    }
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSHandle<GlobalDictionary> dictHandle(thread, dict);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PropertyBox> box = factory->NewPropertyBox(valueHandle);
    PropertyBoxType boxType = valueHandle->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
    attributes.SetBoxType(boxType);

    dict = *GlobalDictionary::PutIfAbsent(thread, dictHandle, propHandle, JSHandle<JSTaggedValue>(box), attributes);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    env->SetGlobalRecord(thread, JSTaggedValue(dict));
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::ThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc)
{
    INTERPRETER_TRACE(thread, ThrowReferenceError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> propName = JSTaggedValue::ToString(thread, JSHandle<JSTaggedValue>(thread, prop));
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> info = factory->NewFromUtf8(desc);
    JSHandle<EcmaString> msg = factory->ConcatFromString(propName, info);
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread,
                                     factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue(),
                                     JSTaggedValue::Exception());
}

JSTaggedValue SlowRuntimeStub::ThrowTypeError(JSThread *thread, const char *message)
{
    INTERPRETER_TRACE(thread, ThrowTypeError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue SlowRuntimeStub::ThrowSyntaxError(JSThread *thread, const char *message)
{
    INTERPRETER_TRACE(thread, ThrowSyntaxError);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_SYNTAX_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue SlowRuntimeStub::StArraySpread(JSThread *thread, JSTaggedValue dst, JSTaggedValue index,
                                             JSTaggedValue src)
{
    INTERPRETER_TRACE(thread, StArraySpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(dstHandle->IsJSArray() && !srcHandle->IsNull() && !srcHandle->IsUndefined());
    if (srcHandle->IsString()) {
        JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, srcHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        uint32_t dstLen = index.GetInt();
        uint32_t strLen = srcString->GetLength();
        for (uint32_t i = 0; i < strLen; i++) {
            uint16_t res = srcString->At<false>(i);
            JSHandle<JSTaggedValue> strValue(factory->NewFromUtf16Literal(&res, 1));
            JSTaggedValue::SetProperty(thread, dstHandle, dstLen + i, strValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue(dstLen + strLen);
    }

    JSHandle<JSTaggedValue> iter;
    auto globalConst = thread->GlobalConstants();
    if (srcHandle->IsJSArrayIterator() || srcHandle->IsJSMapIterator() || srcHandle->IsJSSetIterator() ||
        srcHandle->IsIterator()) {
        iter = srcHandle;
    } else if (srcHandle->IsJSArray() || srcHandle->IsJSMap() || srcHandle->IsTypedArray() || srcHandle->IsJSSet()) {
        JSHandle<JSTaggedValue> valuesStr = globalConst->GetHandledValuesString();
        JSHandle<JSTaggedValue> valuesMethod = JSObject::GetMethod(thread, srcHandle, valuesStr);
        iter = JSIterator::GetIterator(thread, srcHandle, valuesMethod);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        iter = JSIterator::GetIterator(thread, srcHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSMutableHandle<JSTaggedValue> indexHandle(thread, index);
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> iterResult;
    do {
        iterResult = JSIterator::IteratorStep(thread, iter);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (iterResult->IsFalse()) {
            break;
        }
        bool success = JSTaggedValue::GetOwnProperty(thread, iterResult, valueStr, desc);
        if (success && desc.IsEnumerable()) {
            JSTaggedValue::DefineOwnProperty(thread, dstHandle, indexHandle, desc);
            int tmp = indexHandle->GetInt();
            indexHandle.Update(JSTaggedValue(tmp + 1));
        }
    } while (true);

    return indexHandle.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::DefineGeneratorFunc(JSThread *thread, JSFunction *func)
{
    INTERPRETER_TRACE(thread, DefineGeneratorFunc);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> jsFunc = factory->NewJSGeneratorFunction(method);
    ASSERT_NO_ABRUPT_COMPLETION(thread);

    // 26.3.4.3 prototype
    // Whenever a GeneratorFunction instance is created another ordinary object is also created and
    // is the initial value of the generator function's "prototype" property.
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> initialGeneratorFuncPrototype =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    jsFunc->SetProtoOrDynClass(thread, initialGeneratorFuncPrototype);

    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::DefineAsyncFunc(JSThread *thread, JSFunction *func)
{
    INTERPRETER_TRACE(thread, DefineAsyncFunc);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::ASYNC_FUNCTION);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::DefineNCFuncDyn(JSThread *thread, JSFunction *func)
{
    INTERPRETER_TRACE(thread, DefineNCFuncDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::ARROW_FUNCTION);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::DefinefuncDyn(JSThread *thread, JSFunction *func)
{
    INTERPRETER_TRACE(thread, DefinefuncDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::BASE_CONSTRUCTOR);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SuperCall(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         uint16_t firstVRegIdx, uint16_t length)
{
    INTERPRETER_TRACE(thread, SuperCall);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    InterpretedFrameHandler frameHandler(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);

    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, funcHandle));
    ASSERT(superFunc->IsJSFunction());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTargetHandle, length);
    for (size_t i = 0; i < length; i++) {
        info.SetCallArg(i, frameHandler.GetVRegValue(firstVRegIdx + i));
    }
    JSTaggedValue result = JSFunction::Construct(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue SlowRuntimeStub::SuperCallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                               JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, SuperCallSpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSTaggedValue> jsArray(thread, array);

    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, funcHandle));
    ASSERT(superFunc->IsJSFunction());

    JSHandle<TaggedArray> argv(thread, GetCallSpreadArgs(thread, jsArray.GetTaggedValue()));
    const size_t argsLength = argv->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTargetHandle, argsLength);
    info.SetCallArg(argsLength, argv);
    JSTaggedValue result = JSFunction::Construct(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue SlowRuntimeStub::DefineMethod(JSThread *thread, JSFunction *func, JSTaggedValue homeObject)
{
    INTERPRETER_TRACE(thread, DefineMethod);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT(homeObject.IsECMAObject());
    JSHandle<JSTaggedValue> homeObjectHandle(thread, homeObject);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::NORMAL_FUNCTION);
    jsFunc->SetHomeObject(thread, homeObjectHandle);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::LdSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                              JSTaggedValue thisFunc)
{
    INTERPRETER_TRACE(thread, LdSuperByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj.IsUndefined()) {
        return ThrowReferenceError(thread, obj, "this is uninitialized.");
    }
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, key);

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, propHandle));
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue res = JSTaggedValue::GetProperty(thread, superBase, propKey, objHandle).GetValue().GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue SlowRuntimeStub::StSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                              JSTaggedValue value, JSTaggedValue thisFunc)
{
    INTERPRETER_TRACE(thread, StSuperByValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj.IsUndefined()) {
        return ThrowReferenceError(thread, obj, "this is uninitialized.");
    }
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> propHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, propHandle));
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // check may_throw is false?
    JSTaggedValue::SetProperty(thread, superBase, propKey, valueHandle, objHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::GetCallSpreadArgs(JSThread *thread, JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, GetCallSpreadArgs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> jsArray(thread, array);
    uint32_t argvMayMaxLength = JSHandle<JSArray>::Cast(jsArray)->GetArrayLength();
    JSHandle<TaggedArray> argv = factory->NewTaggedArray(argvMayMaxLength);
    JSHandle<JSTaggedValue> itor = JSIterator::GetIterator(thread, jsArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> next(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> nextArg(thread, JSTaggedValue::Undefined());
    size_t argvIndex = 0;
    while (true) {
        next.Update(JSIterator::IteratorStep(thread, itor).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValue(next.GetTaggedValue(), JSTaggedValue::False())) {
            break;
        }
        nextArg.Update(JSIterator::IteratorValue(thread, next).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argv->Set(thread, argvIndex++, nextArg);
    }

    argv = factory->CopyArray(argv, argvMayMaxLength, argvIndex);
    return argv.GetTaggedValue();
}

void SlowRuntimeStub::ThrowDeleteSuperProperty(JSThread *thread)
{
    INTERPRETER_TRACE(thread, ThrowDeleteSuperProperty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII("Can not delete super property");
    JSHandle<JSObject> errorObj = factory->NewJSError(base::ErrorType::REFERENCE_ERROR, info);
    THROW_NEW_ERROR_AND_RETURN(thread, errorObj.GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::NotifyInlineCache(JSThread *thread, JSFunction *func, JSMethod *method)
{
    INTERPRETER_TRACE(thread, NotifyInlineCache);
    uint32_t icSlotSize = method->GetSlotSize();
    if (icSlotSize > 0 && icSlotSize < ProfileTypeInfo::INVALID_SLOT_INDEX) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        [[maybe_unused]] EcmaHandleScope handleScope(thread);

        JSHandle<JSFunction> funcHandle(thread, func);
        JSHandle<ProfileTypeInfo> profileTypeInfo = factory->NewProfileTypeInfo(icSlotSize);
        funcHandle->SetProfileTypeInfo(thread, profileTypeInfo.GetTaggedValue());
        return profileTypeInfo.GetTaggedValue();
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue SlowRuntimeStub::ResolveClass(JSThread *thread, JSTaggedValue ctor, TaggedArray *literal,
                                            JSTaggedValue base, JSTaggedValue lexenv, ConstantPool *constpool)
{
    ASSERT(ctor.IsClassConstructor());
    JSHandle<JSFunction> cls(thread, ctor);
    JSHandle<TaggedArray> literalBuffer(thread, literal);
    JSHandle<JSTaggedValue> lexicalEnv(thread, lexenv);
    JSHandle<ConstantPool> constpoolHandle(thread, constpool);

    InterpretedFrameHandler frameHandler(thread);
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    JSHandle<JSTaggedValue> ecmaModule(thread, JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule());

    SetClassInheritanceRelationship(thread, ctor, base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t literalBufferLength = literalBuffer->GetLength();

    // only traverse the value of key-value pair
    for (uint32_t index = 1; index < literalBufferLength - 1; index += 2) {  // 2: key-value pair
        JSTaggedValue value = literalBuffer->Get(index);
        if (LIKELY(value.IsJSFunction())) {
            JSFunction::Cast(value.GetTaggedObject())->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
            JSFunction::Cast(value.GetTaggedObject())->SetConstantPool(thread, constpoolHandle.GetTaggedValue());
            JSFunction::Cast(value.GetTaggedObject())->SetModule(thread, ecmaModule);
        }
    }

    cls->SetResolved(true);
    return cls.GetTaggedValue();
}

// clone class may need re-set inheritance relationship due to extends may be a variable.
JSTaggedValue SlowRuntimeStub::CloneClassFromTemplate(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base,
                                                      JSTaggedValue lexenv, ConstantPool *constpool)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(ctor.IsClassConstructor());
    JSHandle<JSTaggedValue> lexenvHandle(thread, lexenv);
    JSHandle<JSTaggedValue> constpoolHandle(thread, JSTaggedValue(constpool));
    JSHandle<JSTaggedValue> baseHandle(thread, base);

    JSHandle<JSFunction> cls(thread, ctor);

    JSHandle<JSObject> clsPrototype(thread, cls->GetFunctionPrototype());

    bool canShareHClass = false;
    JSHandle<JSFunction> cloneClass = factory->CloneClassCtor(cls, lexenvHandle, canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> cloneClassPrototype = factory->CloneObjectLiteral(JSHandle<JSObject>(clsPrototype), lexenvHandle,
                                                                         constpoolHandle, canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // After clone both, reset "constructor" and "prototype" properties.
    cloneClass->SetFunctionPrototype(thread, cloneClassPrototype.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>(cloneClass), true, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(cloneClassPrototype),
                                         globalConst->GetHandledConstructorString(), ctorDesc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    cloneClass->SetHomeObject(thread, cloneClassPrototype);

    if (!canShareHClass) {
        SetClassInheritanceRelationship(thread, cloneClass.GetTaggedValue(), baseHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    return cloneClass.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SetClassInheritanceRelationship(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> cls(thread, ctor);
    ASSERT(cls->IsJSFunction());
    JSMutableHandle<JSTaggedValue> parent(thread, base);

    /*
     *         class A / class A extends null                             class A extends B
     *                                       a                                                 a
     *                                       |                                                 |
     *                                       |  __proto__                                      |  __proto__
     *                                       |                                                 |
     *       A            ---->         A.prototype                  A             ---->    A.prototype
     *       |                               |                       |                         |
     *       |  __proto__                    |  __proto__            |  __proto__              |  __proto__
     *       |                               |                       |                         |
     *   Function.prototype       Object.prototype / null            B             ---->    B.prototype
     */

    JSHandle<JSTaggedValue> parentPrototype;
    // hole means parent is not present
    if (parent->IsHole()) {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
        parentPrototype = env->GetObjectFunctionPrototype();
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (parent->IsNull()) {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (!parent->IsConstructor()) {
        return ThrowTypeError(thread, "parent class is not constructor");
    } else {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSTaggedValue::GetProperty(thread, parent,
            globalConst->GetHandledPrototypeString()).GetValue();
        if (!parentPrototype->IsECMAObject() && !parentPrototype->IsNull()) {
            return ThrowTypeError(thread, "parent class have no valid prototype");
        }
    }

    cls->GetTaggedObject()->GetClass()->SetPrototype(thread, parent);

    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(cls)->GetFunctionPrototype());
    clsPrototype->GetClass()->SetPrototype(thread, parentPrototype);

    return JSTaggedValue::Undefined();
}

JSTaggedValue SlowRuntimeStub::SetClassConstructorLength(JSThread *thread, JSTaggedValue ctor, JSTaggedValue length)
{
    ASSERT(ctor.IsClassConstructor());

    JSFunction* cls = JSFunction::Cast(ctor.GetTaggedObject());
    if (LIKELY(!cls->GetClass()->IsDictionaryMode())) {
        cls->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    } else {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        cls->UpdatePropertyInDictionary(thread, globalConst->GetLengthString(), length);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue SlowRuntimeStub::GetModuleNamespace(JSThread *thread, JSTaggedValue localName)
{
    return thread->GetEcmaVM()->GetModuleManager()->GetModuleNamespace(localName);
}

JSTaggedValue SlowRuntimeStub::LdBigInt(JSThread *thread, JSTaggedValue numberBigInt)
{
    INTERPRETER_TRACE(thread, LdBigInt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> bigint(thread, numberBigInt);
    return JSTaggedValue::ToBigInt(thread, bigint);
}
}  // namespace panda::ecmascript
