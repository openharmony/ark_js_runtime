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

#include "js_api_lightweightmap_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_lightweightmap.h"
#include "js_array.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSAPILightWeightMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPILightWeightMapIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an linkedList iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPILightWeightMapIterator> iter(input);
    JSHandle<JSTaggedValue> oldlightWeightMap(thread, iter->GetIteratedLightWeightMap());
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILightWeightMap> lightWeightMap(oldlightWeightMap);
    IterationKind itemKind = IterationKind(iter->GetIterationKind());
    if (oldlightWeightMap->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    int32_t index = iter->GetNextIndex();
    int32_t length = lightWeightMap->GetSize();
    if (index >= length) {
        iter->SetIteratedLightWeightMap(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);

    JSHandle<JSTaggedValue> key(thread, lightWeightMap->GetKeyAt(thread, lightWeightMap, index));
    if (itemKind == IterationKind::KEY) {
        return JSIterator::CreateIterResultObject(thread, key, false).GetTaggedValue();
    }
    JSHandle<JSTaggedValue> value(thread, lightWeightMap->Get(thread, lightWeightMap, key));
    if (itemKind == IterationKind::VALUE) {
        return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(2); // 2 means the length of array
    array->Set(thread, 0, key);
    array->Set(thread, 1, value);
    JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));

    return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPILightWeightMapIterator::CreateLightWeightMapIterator(JSThread *thread,
                                                                                  const JSHandle<JSTaggedValue> &obj,
                                                                                  IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPILightWeightMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPILightWeightMap",
                                    thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPILightWeightMapIterator(JSHandle<JSAPILightWeightMap>(obj), kind));
    return iter;
}
} // namespace panda::ecmascript
