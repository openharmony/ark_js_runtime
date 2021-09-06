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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_MARKER_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_MARKER_H

#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/mark_word.h"

namespace panda::ecmascript {
class JSHClass;
class SemiSpaceCollector;

class SemiSpaceMarker {
public:
    SemiSpaceMarker() = delete;
    inline ~SemiSpaceMarker() = default;
    NO_COPY_SEMANTIC(SemiSpaceMarker);
    NO_MOVE_SEMANTIC(SemiSpaceMarker);

    explicit SemiSpaceMarker(SemiSpaceCollector *semiSpaceCollector);

    SlotStatus MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot);

private:
    SlotStatus MarkYoungGenerationObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot);
    SlotStatus EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord, ObjectSlot slot);
    bool ShouldBePromoted(TaggedObject *object);
    void CopyObjectWithoutHeader(TaggedObject *object, uintptr_t address, size_t size);

    SemiSpaceCollector *collector_;
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_MARKER_H
