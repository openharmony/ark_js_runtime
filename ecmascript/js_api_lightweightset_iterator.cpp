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
 
#include "js_api_lightweightset_iterator.h"
#include "builtins/builtins_errors.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"
#include "global_env.h"
#include "js_api_lightweightset.h"
#include "js_array.h"
#include "object_factory.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSAPILightWeightSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPILightWeightSetIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an lightweightset iterator",
                                    JSTaggedValue::Exception());
    }
    JSHandle<JSAPILightWeightSetIterator> iter(input);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> lightWeightSet(thread, iter->GetIteratedLightWeightSet());
    uint32_t index = iter->GetNextIndex();
    IterationKind itemKind = IterationKind(iter->GetIterationKind());
    if (lightWeightSet->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    uint32_t length = 0;
    if (lightWeightSet->IsJSAPILightWeightSet()) {
        length = JSHandle<JSAPILightWeightSet>(lightWeightSet)->GetLength();
    }
    if (index >= length) {
        iter->SetIteratedLightWeightSet(thread, undefinedHandle);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    iter->SetNextIndex(index + 1);
    JSHandle<TaggedArray> valueArray(
        thread, TaggedArray::Cast(JSHandle<JSAPILightWeightSet>(lightWeightSet)->GetValues().GetTaggedObject()));
    JSHandle<JSTaggedValue> value(thread, valueArray->Get(index));
    if (itemKind == IterationKind::VALUE) {
        return JSIterator::CreateIterResultObject(thread, value, false).GetTaggedValue();
    }
    TaggedArray *hashArray =
        TaggedArray::Cast(JSHandle<JSAPILightWeightSet>(lightWeightSet)->GetHashes().GetTaggedObject());
    JSHandle<JSTaggedValue> keyHandle(thread, hashArray->Get(index));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2)); // 2 means the length of array
    array->Set(thread, 0, keyHandle);
    array->Set(thread, 1, value);
    JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
    return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
}
} // namespace panda::ecmascript