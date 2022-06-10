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

#include "js_api_list_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "global_env.h"
#include "js_api_list.h"
#include "object_factory.h"
#include "tagged_list.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSAPIListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPIListIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an List iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIListIterator> iter(input);
    JSHandle<JSTaggedValue> list(thread, iter->GetIteratedList());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<TaggedSingleList> singleList(list);
    if (list->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    int index = static_cast<int>(iter->GetNextIndex());
    int length = singleList->Length();
    if (index >= length) {
        iter->SetIteratedList(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value(thread, singleList->Get(index));
    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPIListIterator::CreateListIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPIList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIList", thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIListIterator(JSHandle<JSAPIList>(obj)));
    return iter;
}
} // namespace panda::ecmascript
