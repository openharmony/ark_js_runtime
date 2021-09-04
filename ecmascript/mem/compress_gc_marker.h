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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_GC_MARKER_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_GC_MARKER_H

#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {
class JSHClass;
class CompressCollector;

class CompressGCMarker {
public:
    CompressGCMarker() = delete;
    inline ~CompressGCMarker() = default;
    NO_COPY_SEMANTIC(CompressGCMarker);
    NO_MOVE_SEMANTIC(CompressGCMarker);

    inline explicit CompressGCMarker(CompressCollector *compressCollector);

    inline void MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot);

private:
    inline void EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord, ObjectSlot slot);
    inline void CopyObjectWithoutHeader(TaggedObject *object, uintptr_t address, size_t size);

    CompressCollector *collector_;
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_COMPRESS_GC_MARKER_H
