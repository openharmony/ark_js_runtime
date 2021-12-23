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

#ifndef ECMASCRIPT_MEM_HEAP_VERIFICATION_H
#define ECMASCRIPT_MEM_HEAP_VERIFICATION_H

#include <cstdint>

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {
// Verify the object body
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class VerifyObjectVisitor {
public:
    VerifyObjectVisitor(const Heap *heap, size_t *failCount)
        : heap_(heap), failCount_(failCount), objXRay_(heap->GetEcmaVM())
    {
    }
    ~VerifyObjectVisitor() = default;

    void operator()(TaggedObject *obj)
    {
        VisitAllObjects(obj);
    }

    size_t GetFailedCount() const
    {
        return *failCount_;
    }

private:
    void VisitAllObjects(TaggedObject *obj);

    const Heap* const heap_ {nullptr};
    size_t* const failCount_ {nullptr};
    ObjectXRay objXRay_;
};

class Verification {
public:
    explicit Verification(const Heap *heap) : heap_(heap), objXRay_(heap->GetEcmaVM()) {}
    ~Verification() = default;

    size_t VerifyAll() const
    {
        size_t result = VerifyRoot();
        result += VerifyHeap();
        return result;
    }

    bool IsHeapAddress(void *addr) const;
    size_t VerifyRoot() const;
    size_t VerifyHeap() const;
private:
    NO_COPY_SEMANTIC(Verification);
    NO_MOVE_SEMANTIC(Verification);

    const Heap *heap_ {nullptr};
    ObjectXRay objXRay_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_VERIFICATION_H
