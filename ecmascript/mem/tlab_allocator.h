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

#ifndef ECMASCRIPT_MEM_TLAB_ALLOCATOR_H
#define ECMASCRIPT_MEM_TLAB_ALLOCATOR_H

#include "ecmascript/mem/allocator-inl.h"

namespace panda::ecmascript {
class Heap;

class TlabAllocator {
public:
    TlabAllocator() = delete;
    inline explicit TlabAllocator(Heap *heap);
    ~TlabAllocator()
    {
        delete localSpace_;
    }

    NO_COPY_SEMANTIC(TlabAllocator);
    NO_MOVE_SEMANTIC(TlabAllocator);

    inline void Finalize();

    inline uintptr_t Allocate(size_t size, MemSpaceType space);

private:
    inline uintptr_t AllocateInYoungSpace(size_t size);
    inline uintptr_t AllocateInOldSpace(size_t size);
    inline uintptr_t AllocateInCompressSpace(size_t size);

    inline bool ExpandYoung();
    inline bool ExpandCompressFromOld(size_t size);

    Heap *heap_;

    bool enableExpandYoung_;
    BumpPointerAllocator youngAllocator_;

    LocalSpace *localSpace_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_H
