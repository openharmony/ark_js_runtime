
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

#include "js_api_deque_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_deque.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// DequeIteratorPrototype%.next ( )
JSTaggedValue JSAPIDequeIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIDequeIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an deque iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIDequeIterator> iter(input);
    JSHandle<JSTaggedValue> deque(thread, iter->GetIteratedDeque());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    if (deque->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    uint32_t index = iter->GetNextIndex();

    JSHandle<TaggedArray> elements(thread, JSHandle<JSAPIDeque>(deque)->GetElements());
    array_size_t capacity = elements->GetLength();
    uint32_t last = JSHandle<JSAPIDeque>(deque)->GetLast();
    if (index == last) {
        iter->SetIteratedDeque(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    ASSERT(capacity != 0);
    iter->SetNextIndex((index + 1) % capacity);
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, deque, index).GetValue();

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
