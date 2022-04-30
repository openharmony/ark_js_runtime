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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSVerificationTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSVerificationTest, ContainObject)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto verifier = Verification(heap);

    auto funcVerify = [](TaggedObject *object, [[maybe_unused]] Verification &v, const Heap *heap) {
        EXPECT_TRUE(heap->ContainObject(object));
        EXPECT_TRUE(heap->IsAlive(object));
    };

    // new space object
    JSHandle<EcmaString> string = objectFactory->NewFromASCII("123");
    funcVerify(*string, verifier, heap);

    // old space object
    auto oldArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    funcVerify(*oldArray, verifier, heap);

    // no movable object
    auto nonMovableArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    funcVerify(*nonMovableArray, verifier, heap);
}

HWTEST_F_L0(JSVerificationTest, VerifyHeapObjects)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    EXPECT_EQ(heap->VerifyHeapObjects(), 0U);  // failcount is 0

    JSTaggedValue oldArray;
    auto verifier = Verification(heap);
    {
        EcmaHandleScope handleScope(thread);
        auto newArray = objectFactory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);

        oldArray =
            (objectFactory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE)).GetTaggedValue();
        newArray->Set(thread, 0, oldArray);
    }
    heap->CollectGarbage(panda::ecmascript::TriggerGCType::OLD_GC);
    EXPECT_EQ(verifier.VerifyRoot(), 0U);
    size_t failCount = 0;
    VerifyObjectVisitor objVerifier(heap, &failCount);
    const_cast<SemiSpace *>(heap->GetNewSpace())->IterateOverObjects(objVerifier);  // newspace reference the old space
}
}  // namespace panda::test
