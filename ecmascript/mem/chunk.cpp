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

#include "ecmascript/mem/chunk.h"

#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
Chunk::Chunk(RegionFactory *factory) : factory_(factory) {}

Area *Chunk::NewArea(size_t size)
{
    auto area = factory_->AllocateArea(size);
    if (area == nullptr) {
        LOG_ECMA_MEM(FATAL) << "OOM Chunk::NewArea area is nullptr";
        UNREACHABLE();
    }
    areaList_.AddNode(area);
    currentArea_ = area;
    return area;
}

uintptr_t Chunk::Expand(size_t size)
{
    ASSERT(end_ - ptr_ < size);

    Area *head = currentArea_;
    size_t newSize;
    if (head != nullptr) {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        newSize = size + (head->GetSize() << 1);
    } else {
        newSize = sizeof(Area) + MEM_ALIGN + size;
    }

    if (newSize < MIN_CHUNK_AREA_SIZE) {
        newSize = MIN_CHUNK_AREA_SIZE;
    } else if (newSize > MAX_CHUNK_AREA_SIZE) {
        size_t minNewSize = sizeof(Area) + MEM_ALIGN + size;
        newSize = std::max(minNewSize, MAX_CHUNK_AREA_SIZE);
    }

    if (newSize > static_cast<size_t>(std::numeric_limits<int>::max())) {
        LOG_ECMA_MEM(FATAL) << "OOM chunk";
        UNREACHABLE();
    }

    Area *area = NewArea(newSize);
    if (area == nullptr) {
        LOG_ECMA_MEM(FATAL) << "OOM chunk";
        UNREACHABLE();
    }
    uintptr_t result = AlignUp(area->GetBegin(), MEM_ALIGN);
    ptr_ = result + size;
    end_ = area->GetEnd();
    return result;
}

void Chunk::ReleaseMemory()
{
    while (!areaList_.IsEmpty()) {
        Area *node = areaList_.PopBack();
        factory_->FreeArea(node);
    }
    ptr_ = 0;
    end_ = 0;
}
}  // namespace panda::ecmascript
