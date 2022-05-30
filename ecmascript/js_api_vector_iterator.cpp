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

#include "js_api_vector_iterator.h"

#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_vector.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// VectorIteratorPrototype%.next ( )
JSTaggedValue JSAPIVectorIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIVectorIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an vector iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIVectorIterator> iter(input);
    // Let a be O.[[IteratedVectorLike]].
    JSHandle<JSTaggedValue> vector(thread, iter->GetIteratedVector());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    // If a is undefined, return CreateIterResultObject(undefined, true).
    if (vector->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    // Let index be O.[[VectorLikeNextIndex]].
    uint32_t index = iter->GetNextIndex();
    // If a has a [[TypedVectorName]] internal slot, then
    // Let len be the value of O’s [[VectorLength]] internal slot.
    ASSERT(vector->IsJSAPIVector());
    const uint32_t length = static_cast<uint32_t>(JSHandle<JSAPIVector>::Cast(vector)->GetSize());
    // If index >= len, then
    if (index >= length) {
        // Set O.[[IteratedVectorLike]] to undefined.
        // Return CreateIterResultObject(undefined, true).
        iter->SetIteratedVector(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    // Set O.[[VectorLikeNextIndex]] to index + 1.
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, vector, index).GetValue();
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
