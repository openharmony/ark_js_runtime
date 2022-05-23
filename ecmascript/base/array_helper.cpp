/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/base/array_helper.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::base {
bool ArrayHelper::IsConcatSpreadable(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 1. If Type(O) is not Object, return false.
    if (!obj->IsECMAObject()) {
        return false;
    }

    // 2. Let spreadable be Get(O, @@isConcatSpreadable).
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> isConcatsprKey = env->GetIsConcatSpreadableSymbol();
    JSHandle<JSTaggedValue> spreadable = JSTaggedValue::GetProperty(thread, obj, isConcatsprKey).GetValue();
    // 3. ReturnIfAbrupt(spreadable).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If spreadable is not undefined, return ToBoolean(spreadable).
    if (!spreadable->IsUndefined()) {
        return spreadable->ToBoolean();
    }

    // 5. Return IsArray(O).
    return obj->IsArray(thread);
}

int32_t ArrayHelper::SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                                 const JSHandle<JSTaggedValue> &valueX, const JSHandle<JSTaggedValue> &valueY)
{
    // 1. If x and y are both undefined, return +0.
    if (valueX->IsHole()) {
        if (valueY->IsHole()) {
            return 0;
        }
        return 1;
    }
    if (valueY->IsHole()) {
        return -1;
    }
    if (valueX->IsUndefined()) {
        if (valueY->IsUndefined()) {
            return 0;
        }
        // 2. If x is undefined, return 1.
        return 1;
    }
    // 3. If y is undefined, return -1.
    if (valueY->IsUndefined()) {
        return -1;
    }
    // 4. If the argument comparefn is not undefined, then
    // a. Let v be ToNumber(Call(comparefn, undefined, «x, y»)).
    // b. ReturnIfAbrupt(v).
    // c. If v is NaN, return +0.
    // d. Return v.
    if (!callbackfnHandle->IsUndefined()) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackfnHandle, undefined, undefined, 2); // 2: «x, y»
        info.SetCallArg(valueX.GetTaggedValue(), valueY.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(&info);
        if (callResult.IsInt()) {
            return callResult.GetInt();
        }
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        JSHandle<JSTaggedValue> testResult(thread, callResult);
        JSTaggedNumber v = JSTaggedValue::ToNumber(thread, testResult);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        double value = v.GetNumber();
        if (std::isnan(value)) {
            return +0;
        }
        return static_cast<int32_t>(value);
    }
    // 5. Let xString be ToString(x).
    // 6. ReturnIfAbrupt(xString).
    // 7. Let yString be ToString(y).
    // 8. ReturnIfAbrupt(yString).
    // 9. If xString < yString, return -1.
    // 10. If xString > yString, return 1.
    // 11. Return +0.
    JSHandle<JSTaggedValue> xValueHandle(JSTaggedValue::ToString(thread, valueX));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSHandle<JSTaggedValue> yValueHandle(JSTaggedValue::ToString(thread, valueY));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    ComparisonResult compareResult = JSTaggedValue::Compare(thread, xValueHandle, yValueHandle);
    return compareResult == ComparisonResult::GREAT ? 1 : 0;
}

double ArrayHelper::GetLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    if (thisHandle->IsJSArray()) {
        return JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    if (thisHandle->IsTypedArray()) {
        return JSHandle<JSTypedArray>::Cast(thisHandle)->GetArrayLength();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, thisHandle, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, lenResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return len.GetNumber();
}

double ArrayHelper::GetArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    if (thisHandle->IsJSArray()) {
        return JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, thisHandle, lengthKey).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, lenResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return len.GetNumber();
}

JSTaggedValue ArrayHelper::FlattenIntoArray(JSThread *thread, const JSHandle<JSObject> &newArrayHandle,
                                            const JSHandle<JSTaggedValue> &thisObjVal, const FlattenArgs &args,
                                            const JSHandle<JSTaggedValue> &mapperFunctionHandle,
                                            const JSHandle<JSTaggedValue> &thisArg)
{
    // 1. Assert: Type(target) is Object.
    // 2. Assert: Type(source) is Object.
    // 3. Assert: If mapperFunction is present, then ! IsCallable(mapperFunction) is true,
    //    thisArg is present, and depth is 1.
    ASSERT(mapperFunctionHandle->IsUndefined() || mapperFunctionHandle->IsCallable() ||
           (!thisArg->IsUndefined() && args.depth == 1));
    // 4. Let targetIndex be start.
    // 5. Let sourceIndex be +0!.
    FlattenArgs tempArgs;
    tempArgs.start = args.start;
    double sourceIndex = 0.0;
    JSMutableHandle<JSTaggedValue> p(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> targetIndexHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> sourceIndexHandle(thread, JSTaggedValue::Undefined());
    JSHandle<EcmaString> sourceIndexStr;
    // 6. Repeat, while (sourceIndex) < sourceLen,
    //     a. Let P be ! ToString(sourceIndex).
    //     b. Let exists be ? HasProperty(source, P).
    //     c. If exists is true, then
    //         i. Let element be ? Get(source, P).
    //     ii. If mapperFunction is present, then
    //             1. Set element to ? Call(mapperFunction, thisArg, « element, sourceIndex, source »).
    //     iii. Let shouldFlatten be false.
    //     iv. If depth > 0, then
    //             1. Set shouldFlatten to ? IsArray(element).
    //         v. If shouldFlatten is true, then
    //             1. If depth is +∞, let newDepth be +∞.
    //             2. Else, let newDepth be depth - 1.
    //             3. Let elementLen be ? LengthOfArrayLike(element).
    //             4. Set targetIndex to ? FlattenIntoArray(target, element, elementLen, targetIndex, newDepth).
    //     vi. Else,
    //             1. If targetIndex ≥ 2^53 - 1, throw a TypeError exception.
    //             2. Perform ? CreateDataPropertyOrThrow(target, ! ToString(!(targetIndex)), element).
    //             3. Set targetIndex to targetIndex + 1.
    //     d. Set sourceIndex to sourceIndex + 1!.
    while (sourceIndex < args.sourceLen) {
        sourceIndexHandle.Update(JSTaggedValue(sourceIndex));
        sourceIndexStr = JSTaggedValue::ToString(thread, sourceIndexHandle);
        p.Update(sourceIndexStr.GetTaggedValue());
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, p);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            element.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, p).GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!mapperFunctionHandle->IsUndefined()) {
                const size_t argsLength = 3; // 3: « element, sourceIndex, source »
                JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
                EcmaRuntimeCallInfo info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, mapperFunctionHandle, thisArg, undefined, argsLength);
                info.SetCallArg(element.GetTaggedValue(), p.GetTaggedValue(), thisObjVal.GetTaggedValue());
                JSTaggedValue obj = JSFunction::Call(&info);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element.Update(obj);
            }
            bool shouldFlatten = false;
            if (args.depth > 0) {
                shouldFlatten = element->IsArray(thread);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            if (shouldFlatten) {
                tempArgs.depth = args.depth > POSITIVE_INFINITY ? POSITIVE_INFINITY : args.depth - 1;
                tempArgs.sourceLen = ArrayHelper::GetLength(thread, element);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSTaggedValue TargetIndexObj = FlattenIntoArray(thread, newArrayHandle, element, tempArgs,
                                                                thread->GlobalConstants()->GetHandledUndefined(),
                                                                thread->GlobalConstants()->GetHandledUndefined());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                targetIndexHandle.Update(TargetIndexObj);
                JSTaggedNumber targetIndexTemp = JSTaggedValue::ToNumber(thread, targetIndexHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                tempArgs.start = base::NumberHelper::TruncateDouble(targetIndexTemp.GetNumber());
            } else {
                if (tempArgs.start > base::MAX_SAFE_INTEGER) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
                }
                sourceIndexHandle.Update(JSTaggedValue(tempArgs.start));
                sourceIndexStr = JSTaggedValue::ToString(thread, sourceIndexHandle);
                targetIndexHandle.Update(sourceIndexStr.GetTaggedValue());
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, targetIndexHandle, element);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                tempArgs.start++;
            }
        }
        sourceIndex++;
    }
    // 7. Return targetIndex.
    return BuiltinsBase::GetTaggedDouble(tempArgs.start);
}
}  // namespace panda::ecmascript::base
