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

#include "containers_treeset.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_api_tree_set.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_tree-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersTreeSet::TreeSetConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // new TreeSet
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Set set’s internal slot with a new empty List.
    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(obj);
    JSTaggedValue internal = TaggedTreeSet::Create(thread);
    set->SetTreeSet(thread, internal);

    // If comparefn was supplied, let compare be comparefn; else let compare be hole.
    JSHandle<JSTaggedValue> compareFn(GetCallArg(argv, 0));
    if (compareFn->IsUndefined() || compareFn->IsNull()) {
        return set.GetTaggedValue();
    }
    if (!compareFn->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "comparefn is not Callable", JSTaggedValue::Exception());
    }

    TaggedTreeSet::Cast(internal.GetTaggedObject())->SetCompare(thread, compareFn.GetTaggedValue());
    return set.GetTaggedValue();
}

JSTaggedValue ContainersTreeSet::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    JSAPITreeSet::Add(thread, set, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersTreeSet::Remove(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Remove);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    return GetTaggedBoolean(JSAPITreeSet::Delete(thread, set, key));
}

JSTaggedValue ContainersTreeSet::Has(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);

    bool flag = JSAPITreeSet::Has(thread, JSHandle<JSAPITreeSet>::Cast(set), key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return GetTaggedBoolean(flag);
}

JSTaggedValue ContainersTreeSet::GetFirstValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, GetFirstValue);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    return TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject())->GetFirstKey();
}

JSTaggedValue ContainersTreeSet::GetLastValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, GetLastValue);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    return TaggedTreeSet::Cast(set->GetTreeSet().GetTaggedObject())->GetLastKey();
}

JSTaggedValue ContainersTreeSet::Clear(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSAPITreeSet::Clear(thread, JSHandle<JSAPITreeSet>::Cast(self));
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersTreeSet::GetLowerValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, GetLowerValue);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);

    JSHandle<TaggedTreeSet> tset(thread, set->GetTreeSet());
    return TaggedTreeSet::GetLowerKey(thread, tset, key);
}

JSTaggedValue ContainersTreeSet::GetHigherValue(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, GetHigherValue);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    if (!key->IsString() && !key->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Incorrect key parameters", JSTaggedValue::Exception());
    }
    JSHandle<TaggedTreeSet> tset(thread, set->GetTreeSet());
    return TaggedTreeSet::GetHigherKey(thread, tset, key);
}

JSTaggedValue ContainersTreeSet::PopFirst(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, PopFirst);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    return JSAPITreeSet::PopFirst(thread, set);
}

JSTaggedValue ContainersTreeSet::PopLast(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, PopLast);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    return JSAPITreeSet::PopLast(thread, set);
}

JSTaggedValue ContainersTreeSet::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }
    JSHandle<JSAPITreeSet> set = JSHandle<JSAPITreeSet>::Cast(self);
    return GetTaggedBoolean(set->GetSize() == 0);
}

JSTaggedValue ContainersTreeSet::Values(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPITreeSetIterator::CreateTreeSetIterator(thread, self, IterationKind::KEY);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersTreeSet::Entries(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPITreeSetIterator::CreateTreeSetIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersTreeSet::ForEach(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // get and check TreeSet object
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    // get and check callback function
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The first arg is not Callable", JSTaggedValue::Exception());
    }

    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSHandle<JSAPITreeSet> tset = JSHandle<JSAPITreeSet>::Cast(self);
    JSMutableHandle<TaggedTreeSet> iteratedSet(thread, tset->GetTreeSet());
    int elements = iteratedSet->NumberOfElements();
    JSMutableHandle<TaggedArray> entries(TaggedTreeSet::GetArrayFromSet(thread, iteratedSet));

    int index = 0;
    int length = entries->GetLength();
    InternalCallParams *arguments = thread->GetInternalCallParams();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    while (index < elements) {
        int entriesIndex = entries->Get(index).GetInt();
        key.Update(iteratedSet->GetKey(entriesIndex));

        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        arguments->MakeArgv(key, key, self);
        JSTaggedValue ret = JSFunction::Call(thread, func, thisArg, 3, arguments->GetArgv());  // 3: three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);

        // check entries should be update, size will be update by set add and remove.
        if (tset->GetSize() != length) {
            iteratedSet.Update(tset->GetTreeSet());
            entries.Update(TaggedTreeSet::GetArrayFromSet(thread, iteratedSet).GetTaggedValue());
            elements = iteratedSet->NumberOfElements();
            length = entries->GetLength();
        }
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersTreeSet::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), TreeSet, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check this set
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSAPITreeSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPITreeSet", JSTaggedValue::Exception());
    }

    int count = JSHandle<JSAPITreeSet>::Cast(self)->GetSize();
    return JSTaggedValue(count);
}
}  // namespace panda::ecmascript::containers
