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
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class WeakRefOldGCTest : public testing::Test {
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

static JSObject *JSObjectTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSFunction *jsFunc = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> jsFunc1(thread, jsFunc);
    JSHandle<JSObject> newObj =
        ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc1), jsFunc1);
    return *newObj;
}

static TaggedArray *ArrayTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    // 2 : test case
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(2);
    return *array;
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayNonMovable)
{
    auto vm = thread->GetEcmaVM();
    auto array = vm->GetFactory()->NewTaggedArray(2, JSTaggedValue::Undefined(), true);
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    array->Set(thread, 0, newObj1.GetTaggedValue());

    JSObject *newObj2 = JSObjectTestCreate(thread);
    JSTaggedValue value(newObj2);
    value.CreateWeakRef();
    array->Set(thread, 1, value);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(value, array->Get(1));
    vm->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(JSTaggedValue::Undefined(), array->Get(1));
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayUndefined)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2);
    EXPECT_TRUE(*array != nullptr);
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    array->Set(thread, 0, newObj1.GetTaggedValue());

    JSObject *newObj2 = JSObjectTestCreate(thread);
    JSTaggedValue value(newObj2);
    value.CreateWeakRef();
    array->Set(thread, 1, value);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(value, array->Get(1));
    ecmaVM->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(JSTaggedValue::Undefined(), array->Get(1));
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayKeep)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<TaggedArray> array = ecmaVM->GetFactory()->NewTaggedArray(2);
    EXPECT_TRUE(*array != nullptr);
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    array->Set(thread, 0, newObj1.GetTaggedValue());

    JSHandle<JSObject> newObj2(thread, JSObjectTestCreate(thread));
    JSTaggedValue value(newObj2.GetTaggedValue());
    value.CreateWeakRef();
    array->Set(thread, 1, value);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(value, array->Get(1));
    ecmaVM->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(true, array->Get(1).IsWeak());
    value = newObj2.GetTaggedValue();
    value.CreateWeakRef();
    EXPECT_EQ(value, array->Get(1));
}

HWTEST_F_L0(WeakRefOldGCTest, DynObjectUndefined)
{
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    JSTaggedValue array(ArrayTestCreate(thread));
    array.CreateWeakRef();
    newObj1->SetElements(thread, array);
    EXPECT_EQ(newObj1->GetElements(), array);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1->GetElements(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(WeakRefOldGCTest, DynObjectKeep)
{
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    JSHandle<TaggedArray> array(thread, ArrayTestCreate(thread));
    JSTaggedValue value = array.GetTaggedValue();
    value.CreateWeakRef();
    newObj1->SetElements(thread, value);
    EXPECT_EQ(newObj1->GetElements(), value);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::OLD_GC);
    value = array.GetTaggedValue();
    value.CreateWeakRef();
    EXPECT_EQ(newObj1->GetElements(), value);
}
}  // namespace panda::test
