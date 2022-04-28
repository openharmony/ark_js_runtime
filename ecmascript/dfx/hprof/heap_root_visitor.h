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

#ifndef ECMASCRIPT_HPROF_HEAP_ROOT_VISITOR_H
#define ECMASCRIPT_HPROF_HEAP_ROOT_VISITOR_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class JSThread;

class HeapRootVisitor {
public:
    HeapRootVisitor() = default;
    ~HeapRootVisitor() = default;
    NO_MOVE_SEMANTIC(HeapRootVisitor);
    NO_COPY_SEMANTIC(HeapRootVisitor);
    void VisitHeapRoots(JSThread *thread, const RootVisitor &visitor, const RootRangeVisitor &range_visitor);

private:
    EcmaVM *GetVMInstance(JSThread *thread) const;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_ROOT_VISITOR_H
