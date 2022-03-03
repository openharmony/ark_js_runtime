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
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
HeapProfilerInterface *HeapProfilerInterface::heapProfile_ = nullptr;
HeapProfilerInterface *HeapProfilerInterface::GetInstance(JSThread *thread)
{
    if (HeapProfilerInterface::heapProfile_ == nullptr) {
        heapProfile_ = HeapProfilerInterface::CreateHeapProfiler(thread);
    }
    return HeapProfilerInterface::heapProfile_;
}

void HeapProfilerInterface::DumpHeapSnapShot(JSThread *thread, DumpFormat dumpFormat,
                                             const std::string &filePath, bool isVmMode)
{
    LOG(ERROR, RUNTIME) << "HeapProfilerInterface::DumpHeapSnapshot";
    const Heap *heap = thread->GetEcmaVM()->GetHeap();
    auto *hprof = const_cast<NativeAreaAllocator *>(heap->GetNativeAreaAllocator())->New<HeapProfiler>(heap);
    if (UNLIKELY(hprof == nullptr)) {
        LOG_ECMA(FATAL) << "alloc hprof failed";
        UNREACHABLE();
    }
    hprof->DumpHeapSnapShot(thread, dumpFormat, filePath, isVmMode);
    const_cast<NativeAreaAllocator *>(heap->GetNativeAreaAllocator())->Delete(hprof);
}

HeapProfilerInterface *HeapProfilerInterface::CreateHeapProfiler(JSThread *thread)
{
    const Heap *heap = thread->GetEcmaVM()->GetHeap();
    auto *hprof = const_cast<NativeAreaAllocator *>(heap->GetNativeAreaAllocator())->New<HeapProfiler>(heap);
    ASSERT(hprof != nullptr);
    return hprof;
}

void HeapProfilerInterface::Destroy(JSThread *thread, HeapProfilerInterface *heapProfiler)
{
    const Heap *heap = thread->GetEcmaVM()->GetHeap();
    const_cast<NativeAreaAllocator *>(heap->GetNativeAreaAllocator())->Delete(heapProfiler);
}
}  // namespace panda::ecmascript
