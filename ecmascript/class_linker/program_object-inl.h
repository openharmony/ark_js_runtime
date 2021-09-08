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

#ifndef ECMASCRIPT_CLASS_LINKER_PROGRAM_INL_H
#define ECMASCRIPT_CLASS_LINKER_PROGRAM_INL_H

#include "program_object.h"
#include "ecmascript/mem/region_factory.h"

namespace panda {
namespace ecmascript {
const uint8_t *LexicalFunction::GetInstructions() const
{
    JSTaggedValue inst = GetBytecode();
    void *buffer = JSNativePointer::Cast(inst.GetTaggedObject())->GetExternalPointer();
    return static_cast<uint8_t *>(buffer);
}

JSTaggedValue ConstantPool::GetObjectFromCache(uint32_t index) const
{
    return Get(index);
}

void Program::FreeMethodData(RegionFactory *factory)
{
    factory->FreeBuffer(GetMethodsData());
}
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_CLASS_LINKER_PROGRAM_INL_H