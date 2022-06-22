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

#include "js_stable_array.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "interpreter/fast_runtime_stub-inl.h"

namespace panda::ecmascript {
JSTaggedValue JSStableArray::Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();
    uint32_t oldLength = receiver->GetArrayLength();
    uint32_t newLength = argc + oldLength;

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    if (newLength > elements->GetLength()) {
        elements = *JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(receiver), newLength);
    }

    for (uint32_t k = 0; k < argc; k++) {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(k);
        elements->Set(thread, oldLength + k, value.GetTaggedValue());
    }
    receiver->SetArrayLength(thread, newLength);

    return JSTaggedValue(newLength);
}

JSTaggedValue JSStableArray::Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    uint32_t capacity = elements->GetLength();
    uint32_t index = length - 1;
    auto result = elements->Get(index);
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        elements->Set(thread, index, JSTaggedValue::Hole());
    }
    receiver->SetArrayLength(thread, index);
    return result;
}

JSTaggedValue JSStableArray::Splice(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv,
                                    double start, double insertCount, double actualDeleteCount)
{
    JSThread *thread = argv->GetThread();
    uint32_t len = receiver->GetArrayLength();
    uint32_t argc = argv->GetArgsNumber();

    JSHandle<JSObject> thisObjHandle(receiver);
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(actualDeleteCount));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    TaggedArray *srcElements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    JSHandle<TaggedArray> srcElementsHandle(thread, srcElements);
    if (newArray.IsStableJSArray(thread)) {
        TaggedArray *destElements = TaggedArray::Cast(newArrayHandle->GetElements().GetTaggedObject());
        if (actualDeleteCount > destElements->GetLength()) {
            destElements = *JSObject::GrowElementsCapacity(thread, newArrayHandle, actualDeleteCount);
        }

        for (uint32_t idx = 0; idx < actualDeleteCount; idx++) {
            destElements->Set(thread, idx, srcElementsHandle->Get(start + idx));
        }
        JSHandle<JSArray>::Cast(newArrayHandle)->SetArrayLength(thread, actualDeleteCount);
    } else {
        JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
        double k = 0;
        while (k < actualDeleteCount) {
            double from = start + k;
            fromKey.Update(JSTaggedValue(from));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                toKey.Update(JSTaggedValue(k));
                if (newArrayHandle->IsJSProxy()) {
                    toKey.Update(JSTaggedValue::ToString(thread, toKey).GetTaggedValue());
                }
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k++;
        }

        JSHandle<JSTaggedValue> deleteCount(thread, JSTaggedValue(actualDeleteCount));
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, deleteCount,
                                   true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    uint32_t oldCapacity = srcElementsHandle->GetLength();
    uint32_t newCapacity = len - actualDeleteCount + insertCount;
    if (insertCount < actualDeleteCount) {
        for (uint32_t idx = start; idx < len - actualDeleteCount; idx++) {
            auto element = srcElementsHandle->Get(idx + actualDeleteCount);
            element = element.IsHole() ? JSTaggedValue::Undefined() : element;
            srcElementsHandle->Set(thread, idx + insertCount, element);
        }

        if (TaggedArray::ShouldTrim(oldCapacity, newCapacity)) {
            srcElementsHandle->Trim(thread, newCapacity);
        } else {
            for (uint32_t idx = newCapacity; idx < len; idx++) {
                srcElementsHandle->Set(thread, idx, JSTaggedValue::Hole());
            }
        }
    } else {
        if (newCapacity > oldCapacity) {
            srcElementsHandle = JSObject::GrowElementsCapacity(thread, thisObjHandle, newCapacity);
        }
        for (uint32_t idx = len - actualDeleteCount; idx > start; idx--) {
            auto element = srcElementsHandle->Get(idx + actualDeleteCount - 1);
            element = element.IsHole() ? JSTaggedValue::Undefined() : element;
            srcElementsHandle->Set(thread, idx + insertCount - 1, element);
        }
    }

    for (uint32_t i = 2, idx = start; i < argc; i++, idx++) {
        srcElementsHandle->Set(thread, idx, argv->GetCallArg(i));
    }

    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newCapacity));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue JSStableArray::Shift(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    auto result = elements->Get(0);
    for (uint32_t k = 1; k < length; k++) {
        auto kValue = elements->Get(k);
        if (kValue.IsHole()) {
            elements->Set(thread, k - 1, JSTaggedValue::Undefined());
        } else {
            elements->Set(thread, k - 1, kValue);
        }
    }
    uint32_t capacity = elements->GetLength();
    uint32_t index = length - 1;
    if (TaggedArray::ShouldTrim(capacity, index)) {
        elements->Trim(thread, index);
    } else {
        elements->Set(thread, index, JSTaggedValue::Hole());
    }
    receiver->SetArrayLength(thread, index);
    return result;
}

JSTaggedValue JSStableArray::Join(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    uint32_t length = receiver->GetArrayLength();
    JSHandle<JSTaggedValue> sepHandle = base::BuiltinsBase::GetCallArg(argv, 0);
    int sep = ',';
    uint32_t sepLength = 1;
    JSHandle<EcmaString> sepStringHandle;
    if (!sepHandle->IsUndefined()) {
        if (sepHandle->IsString()) {
            sepStringHandle = JSHandle<EcmaString>::Cast(sepHandle);
        } else {
            sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (sepStringHandle->IsUtf8() && sepStringHandle->GetLength() == 1) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            sep = sepStringHandle->GetDataUtf8()[0];
        } else if (sepStringHandle->GetLength() == 0) {
            sep = JSStableArray::SeparatorFlag::MINUS_TWO;
            sepLength = 0;
        } else {
            sep = JSStableArray::SeparatorFlag::MINUS_ONE;
            sepLength = sepStringHandle->GetLength();
        }
    }
    if (length == 0) {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        return globalConst->GetEmptyString();
    }
    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    size_t allocateLength = 0;
    bool isOneByte = (sep != JSStableArray::SeparatorFlag::MINUS_ONE) || sepStringHandle->IsUtf8();
    CVector<JSHandle<EcmaString>> vec;
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    uint32_t elementslength = elements->GetLength();
    uint32_t len = elementslength > length ? length : elementslength;
    for (uint32_t k = 0; k < len; k++) {
        JSTaggedValue element = elements->Get(k);
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element = strElement.GetTaggedValue();
                elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            JSHandle<EcmaString> nextStrHandle(thread, nextStr);
            vec.push_back(nextStrHandle);
            isOneByte = nextStr->IsUtf8() ? isOneByte : false;
            allocateLength += nextStr->GetLength();
        } else {
            vec.push_back(JSHandle<EcmaString>(globalConst->GetHandledEmptyString()));
        }
    }
    if (len > 0) {
        allocateLength += sepLength * (len - 1);
    }
    auto newString = EcmaString::AllocStringObject(allocateLength, isOneByte, thread->GetEcmaVM());
    int current = 0;
    DISALLOW_GARBAGE_COLLECTION;
    for (uint32_t k = 0; k < len; k++) {
        if (k > 0) {
            if (sep >= 0) {
                newString->WriteData(static_cast<char>(sep), current);
            } else if (sep != JSStableArray::SeparatorFlag::MINUS_TWO) {
                newString->WriteData(*sepStringHandle, current,
                    allocateLength - static_cast<uint32_t>(current), sepLength);
            }
            current += static_cast<int>(sepLength);
        }
        JSHandle<EcmaString> nextStr = vec[k];
        int nextLength = static_cast<int>(nextStr->GetLength());
        newString->WriteData(*nextStr, current, allocateLength - static_cast<uint32_t>(current), nextLength);
        current += nextLength;
    }
    ASSERT_PRINT(isOneByte == EcmaString::CanBeCompressed(newString), "isOneByte does not match the real value!");
    return JSTaggedValue(newString);
}
}  // namespace panda::ecmascript
