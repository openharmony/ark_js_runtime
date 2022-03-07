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
#include "js_api_queue_iterator.h"
#include "builtins/builtins_errors.h"
#include "global_env.h"
#include "js_api_queue.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// QueueIteratorPrototype%.next()
JSTaggedValue JSAPIQueueIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIQueueIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an queue iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIQueueIterator> iter(input);
    JSHandle<JSTaggedValue> queue(thread, iter->GetIteratedQueue());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    if (queue->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    
    uint32_t index = iter->GetNextIndex();
    uint32_t length = JSAPIQueue::GetArrayLength(thread, JSHandle<JSAPIQueue>(queue));
    if (index >= length) {
        iter->SetIteratedQueue(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, queue, key).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
