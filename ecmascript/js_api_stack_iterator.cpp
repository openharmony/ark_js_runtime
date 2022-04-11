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

#include "js_api_stack_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_stack.h"
#include "object_factory.h"
#include "js_hclass.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// StackIteratorPrototype%.next()
JSTaggedValue JSAPIStackIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIStackIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an stack iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIStackIterator> iter(input);
    JSHandle<JSTaggedValue> stack(thread, iter->GetIteratedStack());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    if (stack->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    uint32_t index = iter->GetNextIndex();

    uint32_t length = static_cast<uint32_t>((JSHandle<JSAPIStack>::Cast(stack))->GetSize() + 1);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    if (index + 1 > length) {
        iter->SetIteratedStack(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, stack, key).GetValue();
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
}  // namespace panda::ecmascript
