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

#ifndef ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
#define ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H

#include "ecmascript/mem/old_space_collector.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/js_hclass-inl.h"

namespace panda::ecmascript {
void OldSpaceCollector::MarkObject(uint64_t threadId, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);

    auto markBitmap = objectRegion->GetMarkBitmap();
    if (!markBitmap->AtomicTestAndSet(object)) {
        workList_->Push(threadId, object);
    }
}

void OldSpaceCollector::RecordWeakReference(uint64_t threadId, JSTaggedType *ref)
{
    workList_->PushWeakReference(threadId, ref);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SEMI_SAPACE_COLLECTOR_INL_H
