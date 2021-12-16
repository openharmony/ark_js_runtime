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

#include "ecmascript/mem/mem_manager-inl.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
MemManager::MemManager(Heap *heap)
    : heap_(heap),
      newSpaceAllocator_(heap->GetNewSpace()),
      freeListAllocator_ { FreeListAllocator(heap->GetOldSpace()), FreeListAllocator(heap_->GetNonMovableSpace()),
      FreeListAllocator(heap->GetMachineCodeSpace()) }
{
    ASSERT(heap != nullptr);
    heap->SetHeapManager(this);
}
}  // namespace panda::ecmascript
