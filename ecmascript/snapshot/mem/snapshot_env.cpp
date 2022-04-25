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

#include "ecmascript/snapshot/mem/snapshot_env.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript {
void SnapShotEnv::Initialize()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    size_t begin = static_cast<size_t>(ConstantIndex::HCLASS_CLASS_INDEX);
    size_t end = static_cast<size_t>(ConstantIndex::JS_API_TREE_SET_ITERATOR_CLASS_INDEX);
    for (size_t index = begin; index <= end; index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        envMap_.emplace(ToUintPtr(objectValue.GetTaggedObject()), index);
    }

    auto globalEnv = vm_->GetGlobalEnv();
    for (size_t i = 0; i < GlobalEnv::FINAL_INDEX; i++) {
        uintptr_t address = globalEnv->ComputeObjectAddress(i);
        JSHandle<JSTaggedValue> result(address);
        if (result->IsHeapObject()) {
            envMap_.emplace(ToUintPtr(result->GetTaggedObject()), i);
        }
    }
}

void SnapShotEnv::Iterate(const RootVisitor &v)
{
    for (const auto &it : envMap_) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(it.first))));
    }
}
}  // namespace panda::ecmascript
