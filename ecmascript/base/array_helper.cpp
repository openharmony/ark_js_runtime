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

#include "ecmascript/base/array_helper.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
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
                                 const JSHandle<JSTaggedValue> &valueX, const JSHandle<JSTaggedValue> &valueY,
                                 const JSHandle<TaggedArray> &argv)
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
        JSHandle<JSTaggedValue> thisArgHandle(thread, JSTaggedValue::Undefined());
        argv->Set(thread, 0, valueX);
        argv->Set(thread, 1, valueY);
        JSTaggedValue callResult = JSFunction::Call(thread, callbackfnHandle, thisArgHandle, argv);
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
    if (valueX->IsInt() && valueY->IsInt()) {
        auto xNumber = JSTaggedNumber(valueX.GetTaggedValue());
        auto yNumber = JSTaggedNumber(valueY.GetTaggedValue());
        return xNumber.GetInt() > yNumber.GetInt() ? 1 : 0;
    }
    ComparisonResult compareResult;
    if (valueX->IsDouble() && valueY->IsDouble() && !std::isinf(valueX->GetDouble()) &&
        !std::isinf(valueY->GetDouble())) {
        auto xNumber = JSTaggedNumber(valueX.GetTaggedValue());
        auto yNumber = JSTaggedNumber(valueY.GetTaggedValue());
        compareResult = JSTaggedValue::StrictNumberCompare(xNumber.GetDouble(), yNumber.GetDouble());
        return compareResult == ComparisonResult::GREAT ? 1 : 0;
    }
    JSHandle<JSTaggedValue> xValueHandle(JSTaggedValue::ToString(thread, valueX));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    JSHandle<JSTaggedValue> yValueHandle(JSTaggedValue::ToString(thread, valueY));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    compareResult = JSTaggedValue::Compare(thread, xValueHandle, yValueHandle);
    return compareResult == ComparisonResult::GREAT ? 1 : 0;
}

double ArrayHelper::GetLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    if (thisHandle->IsJSArray()) {
        return JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    }
    if (thisHandle->IsTypedArray()) {
        return TypedArrayHelper::GetArrayLength(thread, JSHandle<JSObject>::Cast(thisHandle));
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
}  // namespace panda::ecmascript::base
