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

#include "js_api_linked_list_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "global_env.h"
#include "js_api_linked_list.h"
#include "js_array.h"
#include "object_factory.h"
#include "tagged_list.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSAPILinkedListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPILinkedListIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an linkedList iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILinkedListIterator> iter(input);
    JSHandle<JSTaggedValue> linkedList(thread, iter->GetIteratedLinkedList());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<TaggedDoubleList> list(linkedList);
    if (linkedList->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    int index = static_cast<int>(iter->GetNextIndex());
    int length = list->Length();
    if (index >= length) {
        iter->SetIteratedLinkedList(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value(thread, list->Get(index));
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPILinkedListIterator::CreateLinkedListIterator(JSThread *thread,
                                                                          const JSHandle<JSTaggedValue> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPILinkedList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILinkedList",
                                    thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPILinkedListIterator(JSHandle<JSAPILinkedList>(obj)));
    return iter;
}
} // namespace panda::ecmascript
