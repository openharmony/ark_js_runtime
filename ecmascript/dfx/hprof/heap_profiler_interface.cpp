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

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {
HeapProfilerInterface *HeapProfilerInterface::heapProfile_ = nullptr;
HeapProfilerInterface *HeapProfilerInterface::GetInstance(const EcmaVM *vm)
{
    if (heapProfile_ == nullptr) {
        heapProfile_ = const_cast<NativeAreaAllocator *>(vm->GetNativeAreaAllocator())->New<HeapProfiler>(vm);
    }
    ASSERT(heapProfile_ != nullptr);
    return heapProfile_;
}

void HeapProfilerInterface::Destroy(const EcmaVM *vm)
{
    if (heapProfile_ == nullptr) {
        return;
    }

    const_cast<NativeAreaAllocator *>(vm->GetNativeAreaAllocator())->Delete(heapProfile_);
    heapProfile_ = nullptr;
}
}  // namespace panda::ecmascript
