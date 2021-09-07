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

namespace panda::ecmascript {
enum class SpaceAlloc : bool {
    YOUNG_SPACE,
    OLD_SPACE,
};

class Heap;

class TlabAllocator {
public:
    TlabAllocator() = delete;
    inline ~TlabAllocator();
    NO_COPY_SEMANTIC(TlabAllocator);
    NO_MOVE_SEMANTIC(TlabAllocator);

    inline explicit TlabAllocator(Heap *heap, TriggerGCType gcType);

    inline uintptr_t Allocate(size_t size, SpaceAlloc spaceAlloc);

private:
    inline uintptr_t TlabAllocatorYoungSpace(size_t size);
    inline uintptr_t TlabAllocatorOldSpace(size_t size);

    inline bool ExpandYoung();
    inline bool ExpandOld();

    Heap *heap_;
    TriggerGCType gcType_;
    uintptr_t youngBegin_;
    uintptr_t youngTop_;
    uintptr_t youngEnd_;
    bool youngEnable_;

    uintptr_t oldBegin_;
    uintptr_t oldTop_;
    uintptr_t oldEnd_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_TLAB_ALLOCATOR_H
