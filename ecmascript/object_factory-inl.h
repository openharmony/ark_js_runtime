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

#ifndef ECMASCRIPT_OBJECT_FACTORY_INL_H
#define ECMASCRIPT_OBJECT_FACTORY_INL_H

#include "object_factory.h"
#include "ecmascript/mem/ecma_heap_manager-inl.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/lexical_env.h"

namespace panda::ecmascript {
EcmaString *ObjectFactory::AllocNonMovableStringObject(size_t size)
{
    NewObjectHook();
    return reinterpret_cast<EcmaString *>(heapHelper_.AllocateNonMovableOrHugeObject(stringClass_, size));
}

EcmaString *ObjectFactory::AllocStringObject(size_t size)
{
    NewObjectHook();
    return reinterpret_cast<EcmaString *>(heapHelper_.AllocateYoungGenerationOrHugeObject(stringClass_, size));
}

JSHandle<JSNativePointer> ObjectFactory::NewJSNativePointer(void *externalPointer,
                                                            const DeleteEntryPoint &callBack,
                                                            void *data,
                                                            bool nonMovable)
{
    NewObjectHook();
    TaggedObject *header;
    if (nonMovable) {
        header = heapHelper_.AllocateNonMovableOrHugeObject(jsNativePointerClass_);
    } else {
        header = heapHelper_.AllocateYoungGenerationOrHugeObject(jsNativePointerClass_);
    }
    JSHandle<JSNativePointer> obj(thread_, header);
    obj->SetExternalPointer(externalPointer);
    obj->SetDeleter(callBack);
    obj->SetData(data);
    return obj;
}

LexicalEnv *ObjectFactory::InlineNewLexicalEnv(int numSlots)
{
    NewObjectHook();
    size_t size = LexicalEnv::ComputeSize(numSlots);
    auto header = heapHelper_.TryAllocateYoungGeneration(size);
    if (UNLIKELY(header == nullptr)) {
        return nullptr;
    }
    heapHelper_.SetClass(header, envClass_);
    LexicalEnv *array = LexicalEnv::Cast(header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), numSlots + LexicalEnv::RESERVED_ENV_LENGTH);
    return array;
}

template<typename T, typename S>
void ObjectFactory::NewJSIntlIcuData(const JSHandle<T> &obj, const S &icu, const DeleteEntryPoint &callback)
{
    S *icuPoint = vm_->GetRegionFactory()->New<S>(icu);
    ASSERT(icuPoint != nullptr);
    JSTaggedValue data = obj->GetIcuField();
    if (data.IsHeapObject() && data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(icuPoint);
        return;
    }
    JSHandle<JSNativePointer> pointer(thread_, NewJSNativePointer(icuPoint, callback, vm_).GetTaggedValue());
    obj->SetIcuField(thread_, pointer.GetTaggedValue());
    // push uint8_t* to ecma array_data_list
    vm_->PushToArrayDataList(*pointer);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_FACTORY_INL_H
