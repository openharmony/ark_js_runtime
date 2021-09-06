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

#include "ecmascript/class_linker/program_object-inl.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/slow_runtime_helper.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/template_string.h"
#include "ecmascript/vmstat/runtime_stat.h"

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

    JSHandle<JSTaggedValue> newTarget(thread, JSTaggedValue::Undefined());
    JSTaggedValue res = InvokeJsFunction(thread, jsFunc, taggedObj, newTarget, coretypesArray);

    return res;
}

JSTaggedValue SlowRuntimeStub::NegDyn(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> input(thread, value);
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, input);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

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
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. create promise
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = globalEnv->GetPromiseFunction();

    JSHandle<JSPromise> promiseObject =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    promiseObject->SetPromiseState(thread, JSTaggedValue(static_cast<int32_t>(PromiseStatus::PENDING)));
    // 2. create asyncfuncobj
    JSHandle<JSAsyncFuncObject> asyncFuncObj = factory->NewJSAsyncFuncObject();
    asyncFuncObj->SetPromise(thread, promiseObject);

    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, asyncFuncObj);

    // change state to EXECUTING
    asyncFuncObj->SetGeneratorState(thread, JSTaggedValue(static_cast<int>(JSGeneratorState::EXECUTING)));
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
    return JSTaggedValue::ToNumber(thread, number);
}

JSTaggedValue SlowRuntimeStub::NotDyn(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    int32_t number = JSTaggedValue::ToInt32(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(~number);  // NOLINT(hicpp-signed-bitwise)
}

JSTaggedValue SlowRuntimeStub::IncDyn(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (++number);
}

JSTaggedValue SlowRuntimeStub::DecDyn(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> valueHandle(thread, value);
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (--number);
}

void SlowRuntimeStub::ThrowDyn(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj(thread, value);
    JSHandle<ObjectWrapper> wrapperObject = factory->NewObjectWrapper(obj);
    thread->SetException(wrapperObject.GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::GetPropIterator(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, value);
    JSHandle<JSForInIterator> iteratorHandle = JSObject::EnumerateObjectProperties(thread, objHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iteratorHandle.GetTaggedValue();
}

void SlowRuntimeStub::ThrowConstAssignment(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> name(thread, value.GetTaggedObject());
    JSHandle<EcmaString> info = factory->NewFromString("Assignment to const variable ");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, name);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::Add2Dyn(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Add2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);
    if (leftValue->IsString() && rightValue->IsString()) {
        JSHandle<EcmaString> stringA0 = JSTaggedValue::ToString(thread, leftValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<EcmaString> stringA1 = JSTaggedValue::ToString(thread, rightValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaString *newString = EcmaString::Concat(stringA0, stringA1, ecma_vm);
        return JSTaggedValue(newString);
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
        EcmaString *newString = EcmaString::Concat(stringA0, stringA1, ecma_vm);
        return JSTaggedValue(newString);
    }
    JSTaggedNumber taggedValueA0 = JSTaggedValue::ToNumber(thread, primitiveA0);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedNumber taggedValueA1 = JSTaggedValue::ToNumber(thread, primitiveA1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double a0Double = taggedValueA0.GetNumber();
    double a1Double = taggedValueA1.GetNumber();
    return JSTaggedValue(a0Double + a1Double);
}

JSTaggedValue SlowRuntimeStub::Sub2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Sub2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftHandle(thread, left);
    JSHandle<JSTaggedValue> rightHandle(thread, right);

    JSTaggedNumber number0 = JSTaggedValue::ToNumber(thread, leftHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedNumber number1 = JSTaggedValue::ToNumber(thread, rightHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return number0 - number1;
}

JSTaggedValue SlowRuntimeStub::Mul2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mul2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> leftValue(thread, left);
    JSHandle<JSTaggedValue> rightValue(thread, right);

    // 6. Let lnum be ToNumber(leftValue).
    JSTaggedNumber primitiveA = JSTaggedValue::ToNumber(thread, leftValue);
    // 7. ReturnIfAbrupt(lnum).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. Let rnum be ToNumber(rightValue).
    JSTaggedNumber primitiveB = JSTaggedValue::ToNumber(thread, rightValue);
    // 9. ReturnIfAbrupt(rnum).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12.6.3.1 Applying the * Operator
    return primitiveA * primitiveB;
}

JSTaggedValue SlowRuntimeStub::Div2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Div2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedNumber leftNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, left));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double dLeft = leftNumber.GetNumber();
    JSTaggedNumber rightNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, right));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double dRight = rightNumber.GetNumber();
    if (dRight == 0) {
        if (dLeft == 0 || std::isnan(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        bool positive = ((bit_cast<uint64_t>(dRight) & base::DOUBLE_SIGN_MASK) ==
                         (bit_cast<uint64_t>(dLeft) & base::DOUBLE_SIGN_MASK));
        return JSTaggedValue(positive ? base::POSITIVE_INFINITY : -base::POSITIVE_INFINITY);
    }
    return JSTaggedValue(dLeft / dRight);
}

JSTaggedValue SlowRuntimeStub::Mod2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right)
{
    INTERPRETER_TRACE(thread, Mod2Dyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedNumber leftNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, left));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double dLeft = leftNumber.GetNumber();
    JSTaggedNumber rightNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, right));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double dRight = rightNumber.GetNumber();
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

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithInt32(JSThread *thread, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    int32_t res = JSTaggedValue::ToInt32(thread, valueHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(res);
}

JSTaggedValue SlowRuntimeStub::ToJSTaggedValueWithUint32(JSThread *thread, JSTaggedValue value)
{
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
    array_size_t numExcludedKeys = 0;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys + 1);
    EcmaFrameHandler frameHandler(thread);
    JSTaggedValue excludedKey = frameHandler.GetVRegValue(firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys + 1;
        excludedKeys->Set(thread, 0, excludedKey);
        for (array_size_t i = 1; i < numExcludedKeys; i++) {
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

    JSTaggedNumber baseNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, base));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double doubleBase = baseNumber.GetNumber();
    JSTaggedNumber exponentNumber = JSTaggedValue::ToNumber(thread, JSHandle<JSTaggedValue>(thread, exponent));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double doubleExponent = exponentNumber.GetNumber();
    if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
        return JSTaggedValue(base::NAN_VALUE);
    }

    if ((doubleBase == 0 && (bit_cast<uint64_t>(doubleBase) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
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
    obj->SetGeneratorState(thread, JSTaggedValue(static_cast<int>(JSGeneratorState::SUSPENDED_START)));
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
        generatorObjectHandle->SetGeneratorState(thread,
                                                 JSTaggedValue(static_cast<int>(JSGeneratorState::SUSPENDED_YIELD)));
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

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(thread, asyncFuncObj);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    // ActivePromise
    JSHandle<ResolvingFunctionsRecord> reactions = JSPromise::CreateResolvingFunctions(thread, promise);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    JSHandle<TaggedArray> args = factory->NewTaggedArray(1);
    args->Set(thread, 0, value);
    JSHandle<JSTaggedValue> activeFunc;
    if (is_resolve) {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetResolveFunction());
    } else {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetRejectFunction());
    }
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(thread, activeFunc, thisArg, args);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::NewObjSpreadDyn(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                               JSTaggedValue array)
{
    INTERPRETER_TRACE(thread, NewobjspreadDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSTaggedValue> jsArray(thread, array);
    if (!jsArray->IsJSArray()) {
        return ThrowTypeError(thread, "Cannot Newobjspread");
    }

    uint32_t length = JSHandle<JSArray>::Cast(jsArray)->GetArrayLength();
    JSHandle<TaggedArray> argsArray = factory->NewTaggedArray(length);
    for (array_size_t i = 0; i < length; ++i) {
        auto prop = JSTaggedValue::GetProperty(thread, jsArray, i).GetValue();
        argsArray->Set(thread, i, prop);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    auto tagged = SlowRuntimeHelper::NewObject(thread, funcHandle, newTargetHandle, argsArray);
    return tagged;
}

void SlowRuntimeStub::ThrowUndefinedIfHole(JSThread *thread, JSTaggedValue obj)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> name(thread, obj);
    JSHandle<EcmaString> info = factory->NewFromString(" is not initialized");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, name);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::ThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index, JSTaggedValue thisValue)
{
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    THROW_TYPE_ERROR(thread, "Inner return result is not object");
}

void SlowRuntimeStub::ThrowThrowNotExists(JSThread *thread)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    THROW_TYPE_ERROR(thread, "Throw method is not defined");
}

void SlowRuntimeStub::ThrowPatternNonCoercible(JSThread *thread)
{
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

JSTaggedValue SlowRuntimeStub::StOwnByIndex(JSThread *thread, JSTaggedValue obj, JSTaggedValue idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StOwnByIdDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, obj);
    JSHandle<JSTaggedValue> idxHandle(thread, idx);
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> builtinObj(globalEnv->GetArrayFunction());
    JSHandle<JSObject> arr = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return arr.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateEmptyObject(JSThread *thread, ObjectFactory *factory,
                                                 JSHandle<GlobalEnv> globalEnv)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSFunction> builtinObj(globalEnv->GetObjectFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return obj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory, JSObject *literal)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory, JSObject *literal,
                                                        JSTaggedValue env, ConstantPool *constpool)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> obj(thread, literal);
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(
        obj, JSHandle<JSTaggedValue>(thread, env), JSHandle<JSTaggedValue>(thread, JSTaggedValue(constpool)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SetObjectWithProto(JSThread *thread, JSTaggedValue proto, JSTaggedValue obj)
{
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    JSHandle<JSObject> resultObj = JSIterator::IteratorNext(thread, iterHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return resultObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CloseIterator(JSThread *thread, JSTaggedValue iter)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> iterHandle(thread, iter);
    JSHandle<JSTaggedValue> record;
    if (thread->GetException().IsObjectWrapper()) {
        JSTaggedValue exception = ObjectWrapper::Cast(thread->GetException().GetTaggedObject())->GetValue();
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(CompletionRecord::THROW,
            JSHandle<JSTaggedValue>(thread, exception)));
    } else {
        JSHandle<JSTaggedValue> undefinedVal = globalConst->GetHandledUndefined();
        record = JSHandle<JSTaggedValue>(
            factory->NewCompletionRecord(CompletionRecord::NORMAL, undefinedVal));
    }
    JSHandle<JSTaggedValue> result = JSIterator::IteratorClose(thread, iterHandle, record);
    if (result->IsCompletionRecord()) {
        return CompletionRecord::Cast(result->GetTaggedObject())->GetValue();
    }
    return result.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::ImportModule([[maybe_unused]] JSThread *thread,
                                            [[maybe_unused]] JSTaggedValue moduleName)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> name(thread, moduleName);
    JSHandle<JSTaggedValue> module = thread->GetEcmaVM()->GetModuleByName(name);
    return module.GetTaggedValue();  // return moduleRef
}

void SlowRuntimeStub::StModuleVar([[maybe_unused]] JSThread *thread, [[maybe_unused]] JSTaggedValue exportName,
                                  [[maybe_unused]] JSTaggedValue exportObj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> name(thread, exportName);
    JSHandle<JSTaggedValue> value(thread, exportObj);
    thread->GetEcmaVM()->GetModuleManager()->AddModuleItem(thread, name, value);
}

void SlowRuntimeStub::CopyModule(JSThread *thread, JSTaggedValue srcModule)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> srcModuleObj(thread, srcModule);
    thread->GetEcmaVM()->GetModuleManager()->CopyModule(thread, srcModuleObj);
}

JSTaggedValue SlowRuntimeStub::LdModvarByName([[maybe_unused]] JSThread *thread,
                                              [[maybe_unused]] JSTaggedValue moduleObj,
                                              [[maybe_unused]] JSTaggedValue itemName)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<JSTaggedValue> module(thread, moduleObj);
    JSHandle<JSTaggedValue> item(thread, itemName);
    JSHandle<JSTaggedValue> moduleVar = thread->GetEcmaVM()->GetModuleManager()->GetModuleItem(thread, module, item);
    return moduleVar.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::CreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory, JSArray *literal)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSArray> array(thread, literal);
    JSHandle<JSArray> arrLiteral = factory->CloneArrayLiteral(array);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return arrLiteral.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetTemplateObject(JSThread *thread, JSTaggedValue literal)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> templateLiteral(thread, literal);
    JSHandle<JSTaggedValue> templateObj = TemplateString::GetTemplateObject(thread, templateLiteral);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return templateObj.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::GetNextPropName(JSThread *thread, JSTaggedValue iter)
{
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    if (!srcHandle->IsNull() && !srcHandle->IsUndefined()) {
        JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, srcHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        array_size_t keysLen = keys->GetLength();
        for (array_size_t i = 0; i < keysLen; i++) {
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> iter(thread, obj);
    JSHandle<JSTaggedValue> next(thread, method);

    JSHandle<TaggedArray> argv(factory->EmptyArray());
    ASSERT(next->IsCallable());
    JSTaggedValue ret = JSFunction::Call(thread, next, iter, argv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret.IsECMAObject()) {
        return ThrowTypeError(thread, "the Iterator is not an ecmaobject.");
    }
    return ret;
}

JSTaggedValue SlowRuntimeStub::GetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                              uint32_t startIdx)
{
    INTERPRETER_TRACE(thread, GetUnmappedArgs);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs);
    for (array_size_t i = 0; i < actualNumArgs; ++i) {
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
    JSHandle<JSTaggedValue> newTarget(thread, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> args = vm->GetFactory()->EmptyArray();
    JSTaggedValue res = InvokeJsFunction(thread, JSHandle<JSFunction>(valuesFunc), objHandle, newTarget, args);

    return res;
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
        JSHandle<JSFunction>::Cast(getterHandle)->SetFunctionKind(thread, FunctionKind::GETTER_FUNCTION);
        desc.SetGetter(getterHandle);
    }
    if (!setterHandle->IsUndefined()) {
        JSHandle<JSFunction>::Cast(setterHandle)->SetFunctionKind(thread, FunctionKind::SETTER_FUNCTION);
        desc.SetSetter(setterHandle);
    }
    JSObject::DefineOwnProperty(thread, objHandle, propKey, desc);

    return objHandle.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::LdObjByIndex(JSThread *thread, JSTaggedValue obj, JSTaggedValue idx, bool callGetter,
                                            JSTaggedValue receiver)
{
    INTERPRETER_TRACE(thread, LdObjByIndexDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedValue res;
    JSHandle<JSTaggedValue> objHandle(thread, obj);
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), objHandle);
    } else {
        res = JSTaggedValue::GetProperty(thread, objHandle, idx.GetArrayLength()).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue SlowRuntimeStub::StObjByIndex(JSThread *thread, JSTaggedValue obj, JSTaggedValue idx, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, StObjByIndexDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, obj), idx.GetArrayLength(),
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
    // If fast path is fail, not need slow path, just throw error.
    return ThrowReferenceError(thread, prop, " is not defined");
}

JSTaggedValue SlowRuntimeStub::LdGlobalVar(JSThread *thread, JSTaggedValue global, JSTaggedValue prop)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, global.GetTaggedObject()->GetClass()->GetPrototype());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    OperationResult res = JSTaggedValue::GetProperty(thread, objHandle, propHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::StGlobalVar(JSThread *thread, JSTaggedValue prop, JSTaggedValue value)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> global(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSTaggedValue> propHandle(thread, prop);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    JSObject::GlobalSetProperty(thread, propHandle, valueHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue SlowRuntimeStub::ThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> propName = JSTaggedValue::ToString(thread, JSHandle<JSTaggedValue>(thread, prop));
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> info = factory->NewFromString(desc);
    JSHandle<EcmaString> msg = factory->ConcatFromString(propName, info);
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread,
                                     factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue(),
                                     JSTaggedValue::Exception());
}

JSTaggedValue SlowRuntimeStub::ThrowTypeError(JSThread *thread, const char *message)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue SlowRuntimeStub::StArraySpread(JSThread *thread, JSTaggedValue dst, JSTaggedValue index,
                                             JSTaggedValue src)
{
    INTERPRETER_TRACE(thread, StArraySpread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> dstHandle(thread, dst);
    JSHandle<JSTaggedValue> srcHandle(thread, src);
    ASSERT(dstHandle->IsJSArray() && !srcHandle->IsNull() && !srcHandle->IsUndefined());
    if (srcHandle->IsString()) {
        JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, srcHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        array_size_t dstLen = index.GetInt();
        array_size_t strLen = srcString->GetLength();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (array_size_t i = 0; i < strLen; i++) {
            uint16_t res = srcString->At<false>(i);
            JSHandle<JSTaggedValue> strValue(factory->NewFromUtf16Literal(&res, 1));
            JSTaggedValue::SetProperty(thread, dstHandle, dstLen + i, strValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue(dstLen + strLen);
    }
    JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, srcHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSMutableHandle<JSTaggedValue> indexHandle(thread, index);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    array_size_t length = keys->GetLength();
    for (array_size_t i = 0; i < length; i++) {
        PropertyDescriptor desc(thread);
        key.Update(keys->Get(i));
        bool success = JSTaggedValue::GetOwnProperty(thread, srcHandle, key, desc);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        if (success && desc.IsEnumerable()) {
            JSTaggedValue::DefineOwnProperty(thread, dstHandle, indexHandle, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int tmp = indexHandle->GetInt();
            indexHandle.Update(JSTaggedValue(tmp + 1));
        }
    }

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
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::NORMAL_FUNCTION);
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

JSTaggedValue SlowRuntimeStub::NewClassFunc(JSThread *thread, JSFunction *func)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutName());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::CLASS_CONSTRUCTOR);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::DefineClass(JSThread *thread, JSFunction *func, TaggedArray *literal,
                                           JSTaggedValue proto, JSTaggedValue lexenv, ConstantPool *constpool)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> cls(thread, func);
    ASSERT(cls->IsJSFunction());
    JSHandle<TaggedArray> literalBuffer(thread, literal);
    JSMutableHandle<JSTaggedValue> parent(thread, proto);
    JSHandle<JSTaggedValue> lexicalEnv(thread, lexenv);
    JSHandle<ConstantPool> constantpool(thread, constpool);

    cls->GetTaggedObject()->GetClass()->SetClassConstructor(true);
    JSFunction::Cast(cls->GetTaggedObject())->SetClassConstructor(thread, true);

    /*
     *  set class __proto__
     *
     *         class A / class A extends null                             class A extends B
     *                                       a                                                 a
     *                                       |                                                 |
     *                                       |  __proto__                                      |  __proto__
     *                                       |                                                 |
     *       A                  ---->   A.prototype                  A            ---->    A.prototype
     *       |                               |                       |                         |
     *       |  __proto__                    |  __proto__            |  __proto__              |  __proto__
     *       |                               |                       |                         |
     *   Function.prototype       Object.prototype / null            B             ---->    B.prototype
     */
    JSMutableHandle<JSTaggedValue> parentPrototype(thread, JSTaggedValue::Undefined());
    // hole means parent is not present
    if (parent->IsHole()) {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(thread, FunctionKind::CLASS_CONSTRUCTOR);
        parentPrototype.Update(env->GetObjectFunctionPrototype().GetTaggedValue());
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (parent->IsNull()) {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(thread, FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype.Update(JSTaggedValue::Null());
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (!parent->IsConstructor()) {
        return ThrowTypeError(thread, "parent class is not constructor");
    } else {
        JSHandle<JSFunction>::Cast(cls)->SetFunctionKind(thread, FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype.Update(JSTaggedValue::GetProperty(thread, parent,
            globalConst->GetHandledPrototypeString()).GetValue().GetTaggedValue());
        if (!parentPrototype->IsECMAObject() && !parentPrototype->IsNull()) {
            return ThrowTypeError(thread, "parent class have no valid prototype");
        }
    }

    // null cannot cast JSObject
    JSHandle<JSObject> clsPrototype = JSObject::ObjectCreate(thread, JSHandle<JSObject>(parentPrototype));

    bool success = true;
    success = success && JSFunction::MakeClassConstructor(thread, cls, clsPrototype);
    clsPrototype.GetTaggedValue().GetTaggedObject()->GetClass()->SetClassPrototype(true);

    success = success && JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(cls), parent);

    uint32_t bufferLength = literalBuffer->GetLength();
    // static location is hidden in the last index of Literal buffer
    uint32_t staticLoc = literalBuffer->Get(thread, bufferLength - 1).GetInt();

    // set property on class.prototype and class
    JSMutableHandle<JSTaggedValue> propKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> propValue(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < staticLoc * 2; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
        // set non-static property on cls.prototype
        propKey.Update(literalBuffer->Get(thread, i));
        propValue.Update(literalBuffer->Get(thread, i + 1));
        if (propValue->IsJSFunction()) {
            propValue.Update(
                factory->CloneJSFuction(JSHandle<JSFunction>::Cast(propValue), FunctionKind::NORMAL_FUNCTION)
                    .GetTaggedValue());
            JSHandle<JSFunction> propFunc = JSHandle<JSFunction>::Cast(propValue);
            propFunc->SetHomeObject(thread, clsPrototype);
            propFunc->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
            propFunc->SetConstantPool(thread, constantpool.GetTaggedValue());
        }
        PropertyDescriptor desc(thread, propValue, true, false, true);  // non-enumerable
        success = success && JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>::Cast(clsPrototype),
                                                                  propKey, desc);
    }
    for (uint32_t i = staticLoc * 2; i < bufferLength - 1; i += 2) {  // 2: ditto
        // set static property on cls
        propKey.Update(literalBuffer->Get(thread, i));
        propValue.Update(literalBuffer->Get(thread, i + 1));
        if (propValue->IsJSFunction()) {
            propValue.Update(
                factory->CloneJSFuction(JSHandle<JSFunction>::Cast(propValue), FunctionKind::NORMAL_FUNCTION)
                    .GetTaggedValue());
            JSHandle<JSFunction> propFunc = JSHandle<JSFunction>::Cast(propValue);
            propFunc->SetHomeObject(thread, cls);
            propFunc->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
            propFunc->SetConstantPool(thread, constantpool.GetTaggedValue());
        }
        PropertyDescriptor desc(thread, propValue, true, false, true);  // non-enumerable
        success = success && JSTaggedValue::DefinePropertyOrThrow(thread, cls, propKey, desc);
    }

    // ECMA2015 14.5.15: if class don't have a method which is name(), set the class a string name.
    if (!JSTaggedValue::HasOwnProperty(thread, cls, globalConst->GetHandledNameString())) {
        JSMethod *clsTarget = JSHandle<JSFunction>::Cast(cls)->GetCallTarget();
        ASSERT(clsTarget != nullptr);
        CString clsName(
            utf::Mutf8AsCString(clsTarget->GetStringDataAnnotation(Method::AnnotationField::FUNCTION_NAME).data));
        if (!clsName.empty()) {
            success =
                success && JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(cls),
                                                       JSHandle<JSTaggedValue>(factory->NewFromString(clsName.c_str())),
                                                       globalConst->GetHandledUndefined());
        }
    }

    if (!success) {
        return JSTaggedValue::Exception();
    }

    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return cls.GetTaggedValue();
}

JSTaggedValue SlowRuntimeStub::SuperCall(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         uint16_t firstVRegIdx, uint16_t length)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    EcmaFrameHandler frameHandler(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);

    JSHandle<JSTaggedValue> superFunc(thread, JSHandle<JSObject>::Cast(funcHandle)->GetPrototype(thread));
    ASSERT(superFunc->IsJSFunction());

    JSHandle<TaggedArray> argv = factory->NewTaggedArray(length);
    for (size_t i = 0; i < length; ++i) {
        argv->Set(thread, i, frameHandler.GetVRegValue(firstVRegIdx + i));
    }
    JSTaggedValue result = JSFunction::Construct(thread, superFunc, argv, newTargetHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue SlowRuntimeStub::SuperCallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                               JSTaggedValue array)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaFrameHandler frameHandler(thread);

    JSHandle<JSTaggedValue> funcHandle(thread, func);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSTaggedValue> jsArray(thread, array);

    JSHandle<JSTaggedValue> superFunc(thread, JSHandle<JSObject>::Cast(funcHandle)->GetPrototype(thread));
    ASSERT(superFunc->IsJSFunction());

    JSHandle<TaggedArray> argv(thread, GetCallSpreadArgs(thread, jsArray.GetTaggedValue()));
    JSTaggedValue result = JSFunction::Construct(thread, superFunc, argv, newTargetHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue SlowRuntimeStub::DefineMethod(JSThread *thread, JSFunction *func, JSTaggedValue homeObject)
{
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
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromString("Can not delete super property");
    JSHandle<JSObject> errorObj = factory->NewJSError(base::ErrorType::REFERENCE_ERROR, info);
    THROW_NEW_ERROR_AND_RETURN(thread, errorObj.GetTaggedValue());
}

JSTaggedValue SlowRuntimeStub::NotifyInlineCache(JSThread *thread, JSFunction *func, JSMethod *method)
{
    // use vtable index as ic Size
    uint32_t icSlotSize = method->GetNumericalAnnotation(JSMethod::AnnotationField::IC_SIZE);
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
}  // namespace panda::ecmascript
