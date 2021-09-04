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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/hprof/heap_root_visitor.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
void HeapRootVisitor::VisitHeapRoots(JSThread *thread, const RootVisitor &visitor,
                                     const RootRangeVisitor &range_visitor)
{
    auto ecma_vm = GetVMInstance(thread);
    ecma_vm->Iterate(visitor);
    thread->Iterate(visitor, range_visitor);
}

EcmaVM *HeapRootVisitor::GetVMInstance(JSThread *thread) const
{
    return thread->GetEcmaVM();
}
}  // namespace panda::ecmascript
