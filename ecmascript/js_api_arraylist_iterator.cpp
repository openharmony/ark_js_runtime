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

#include "js_api_arraylist_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_arraylist.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
// ArrayListIteratorPrototype%.next ( )
JSTaggedValue JSAPIArrayListIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPIArrayListIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an arrayList iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIArrayListIterator> iter(input);
    JSHandle<JSTaggedValue> arrayList(thread, iter->GetIteratedArrayList());
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();

    if (arrayList->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }

    uint32_t index = iter->GetNextIndex();
    uint32_t length = 0;

    if (arrayList->IsJSAPIArrayList()) {
        length = JSHandle<JSAPIArrayList>(arrayList)->GetLength().GetArrayLength();
    }

    if (index >= length) {
        iter->SetIteratedArrayList(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }

    iter->SetNextIndex(index + 1);
    JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, arrayList, index).GetValue();

    return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
}
} // namespace panda::ecmascript
