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
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class LargeObjectTest : public testing::Test {
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
        thread->GetEcmaVM()->GetFactory()->SetTriggerGc(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    JSThread *thread {nullptr};
    PandaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
};

#if !defined(NDEBUG)
static JSObject *JSObjectTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
    JSHandle<JSObject> newObj =
        ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    return *newObj;
}
#endif

#if !defined(NDEBUG)
static TaggedArray *LargeArrayTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    // 20 * 10000 : test case
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(20 * 10000);
    return *array;
}
#endif

HWTEST_F_L0(LargeObjectTest, LargeArrayKeep)
{
#if !defined(NDEBUG)
    TaggedArray *array = LargeArrayTestCreate(thread);
    EXPECT_TRUE(array != nullptr);
    JSHandle<TaggedArray> arrayHandle(thread, array);
    JSHandle<JSObject> newObj(thread, JSObjectTestCreate(thread));
    arrayHandle->Set(thread, 0, newObj.GetTaggedValue());
    auto ecmaVm = thread->GetEcmaVM();
    EXPECT_EQ(*arrayHandle, reinterpret_cast<TaggedObject *>(array));
    ecmaVm->CollectGarbage(TriggerGCType::SEMI_GC);   // Trigger GC.
    ecmaVm->CollectGarbage(TriggerGCType::LARGE_GC);  // Trigger GC.
    EXPECT_EQ(*newObj, array->Get(0).GetTaggedObject());
    EXPECT_EQ(*arrayHandle, reinterpret_cast<TaggedObject *>(array));
#endif
}

HWTEST_F_L0(LargeObjectTest, MultipleArrays)
{
#if !defined(NDEBUG)
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = ecmaVm->GetHeap();
    Region *firstPage = nullptr;
    Region *secondPage = nullptr;
    Region *thirdPage = nullptr;

    JSHandle<TaggedArray> array1(thread, LargeArrayTestCreate(thread));
    firstPage = Region::ObjectAddressToRange(*array1);
    {
        DISALLOW_GARBAGE_COLLECTION;
        [[maybe_unused]] TaggedArray *array2 = LargeArrayTestCreate(thread);
        secondPage = Region::ObjectAddressToRange(array2);
    }
    JSHandle<TaggedArray> array3(thread, LargeArrayTestCreate(thread));
    thirdPage = Region::ObjectAddressToRange(*array3);

    EXPECT_EQ(firstPage->GetNext(), secondPage);
    EXPECT_EQ(secondPage->GetNext(), thirdPage);

    ecmaVm->CollectGarbage(TriggerGCType::LARGE_GC);  // Trigger GC.

    EXPECT_EQ(firstPage->GetNext(), thirdPage);

    size_t failCount = 0;
    VerifyObjectVisitor objVerifier(heap, &failCount);
    heap->GetLargeObjectSpace()->IterateOverObjects(objVerifier);  // newspace reference the old space
    EXPECT_TRUE(failCount == 0);
#endif
}
}  // namespace panda::test
