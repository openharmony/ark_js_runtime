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

#include "ecmascript/js_set_iterator.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_set.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.If Type(O) is not Object, throw a TypeError exception.
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    // 3.If O does not have all of the internal slots of a Set Iterator Instance (23.2.5.3), throw a TypeError
    // exception.
    if (!input->IsJSSetIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a set iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSSetIterator> iter(input);
    iter->Update(thread);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    // 4.Let s be O.[[IteratedSet]].
    JSHandle<JSTaggedValue> iteratedSet(thread, iter->GetIteratedSet());

    // 5.Let index be O.[[SetNextIndex]].
    int index = iter->GetNextIndex();
    IterationKind itemKind = iter->GetIterationKind();
    // 7.If s is undefined, return CreateIterResultObject(undefined, true).
    if (iteratedSet->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    JSHandle<LinkedHashSet> set(iteratedSet);
    int totalElements = set->NumberOfElements() + set->NumberOfDeletedElements();

    while (index < totalElements) {
        if (!set->GetKey(index).IsHole()) {
            iter->SetNextIndex(index + 1);
            JSHandle<JSTaggedValue> key(thread, set->GetKey(index));
            // If itemKind is value
            if (itemKind == IterationKind::VALUE || itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, key, false).GetTaggedValue();
            }
            // If itemKind is key+value, then
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2: key and value pair
            array->Set(thread, 0, key);
            array->Set(thread, 1, key);
            JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    // 13.Set O.[[IteratedSet]] to undefined.
    iter->SetIteratedSet(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

void JSSetIterator::Update(const JSThread *thread)
{
    [[maybe_unused]] DisallowGarbageCollection noGc;
    JSTaggedValue iteratedSet = GetIteratedSet();
    if (iteratedSet.IsUndefined()) {
        return;
    }
    LinkedHashSet *set = LinkedHashSet::Cast(iteratedSet.GetTaggedObject());
    if (set->GetNextTable().IsHole()) {
        return;
    }
    int index = GetNextIndex();
    JSTaggedValue nextTable = set->GetNextTable();
    while (!nextTable.IsHole()) {
        index -= set->GetDeletedElementsAt(index);
        set = LinkedHashSet::Cast(nextTable.GetTaggedObject());
        nextTable = set->GetNextTable();
    }
    SetIteratedSet(thread, JSTaggedValue(set));
    SetNextIndex(index);
}

JSHandle<JSTaggedValue> JSSetIterator::CreateSetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                         IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSSet()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSSetIterator(JSHandle<JSSet>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
