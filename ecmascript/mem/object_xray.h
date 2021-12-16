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

#ifndef ECMASCRIPT_MEM_HEAP_ROOTS_H
#define ECMASCRIPT_MEM_HEAP_ROOTS_H

#include <cstdint>

#include "ecmascript/mem/slots.h"
#include "mem/mem.h"

namespace panda::ecmascript {
class EcmaVM;
class JSHClass;

enum class Root {
    ROOT_FRAME,
    ROOT_HANDLE,
    ROOT_VM,
    ROOT_STRING,
    ROOT_INTERNAL_CALL_PARAMS,
};

enum class GCType : size_t { SEMI_GC, OLD_GC };

using RootVisitor = std::function<void(Root type, ObjectSlot p)>;
using RootRangeVisitor = std::function<void(Root type, ObjectSlot start, ObjectSlot end)>;
using EcmaObjectVisitor = std::function<void(TaggedObject *root, ObjectSlot p)>;
using EcmaObjectRangeVisitor = std::function<void(TaggedObject *root, ObjectSlot start, ObjectSlot end)>;

using WeakRootVisitor = std::function<TaggedObject *(TaggedObject *p)>;

class ObjectXRay {
public:
    explicit ObjectXRay(EcmaVM *ecmaVm) : ecmaVm_(ecmaVm) {}
    ~ObjectXRay() = default;

    inline void VisitVMRoots(const RootVisitor &visitor, const RootRangeVisitor &range_visitor) const;
    template<GCType gc_type>
    inline void VisitObjectBody(TaggedObject *object, JSHClass *klass, const EcmaObjectRangeVisitor &visitor);

private:
    EcmaVM *ecmaVm_ {nullptr};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_ROOTS_H
