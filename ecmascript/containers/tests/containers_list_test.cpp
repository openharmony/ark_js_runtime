/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_list.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersListTest : public testing::Test {
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

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> list = GetCallArg(argv, 2); // 2 means the secode arg
            if (!list->IsUndefined()) {
                if (index->IsNumber() && value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSAPIList::Set(thread, JSHandle<JSAPIList>::Cast(list), index->GetInt(), newValue);
                }
            }
            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializeListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::List)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIList> CreateJSAPIList(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeListConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSTaggedValue result = ContainersList::ListConstructor(objCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIList> list(thread, result);
        return list;
    }
};

HWTEST_F_L0(ContainersListTest, ListConstructor)
{
    InitializeListConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeListConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
    JSTaggedValue result = ContainersList::ListConstructor(objCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIList());
    JSHandle<JSAPIList> list(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(list));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = list->Length();
    ASSERT_EQ(size, 0);
}

HWTEST_F_L0(ContainersListTest, InsertAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Insert(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(20));
        callInfo->SetCallArg(1, JSTaggedValue(20));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Insert(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Get(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersListTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Insert(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue rvalue = ContainersList::Remove(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(NODE_NUMBERS - 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(6));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue rvalue = ContainersList::RemoveByIndex(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue(6));
        EXPECT_EQ(list->Length(), static_cast<int>(NODE_NUMBERS - 2));
    }
}

HWTEST_F_L0(ContainersListTest, Equal)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    JSHandle<JSAPIList> list1 = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list1.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list1->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, list1.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue res = ContainersList::Equal(callInfo.get());
        EXPECT_EQ(res, JSTaggedValue::True());
        TestHelper::TearDownFrame(thread, prev);
    }
}

HWTEST_F_L0(ContainersListTest, GetSubList)
{
    constexpr uint32_t NODE_NUMBERS = 10;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(list.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(2));
        callInfo1->SetCallArg(1, JSTaggedValue(5));
        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1.get());
        JSTaggedValue newList = ContainersList::GetSubList(callInfo1.get());
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(list->Length(), 10);
        for (uint32_t i = 0; i < 3; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(newList);
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
            JSTaggedValue result = ContainersList::Get(callInfo.get());
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i + 2));
        }
    }
}

HWTEST_F_L0(ContainersListTest, ConvertToArray)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIList> list = CreateJSAPIList();
    JSHandle<TaggedArray> oldArray(factory->NewTaggedArray(NODE_NUMBERS, JSTaggedValue::Hole()));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        oldArray->Set(thread, i, JSTaggedValue(i));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(list.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
    JSTaggedValue newArray = ContainersList::ConvertToArray(callInfo.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue newArrayValue =
        JSTaggedValue::ToObject(thread, JSHandle<JSTaggedValue>(thread, newArray))->GetElements();
    JSHandle<TaggedArray> newArrayHandle(thread, newArrayValue);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        EXPECT_EQ(newArrayHandle->Get(i), oldArray->Get(i));
    }
}

HWTEST_F_L0(ContainersListTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersList::Clear(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(list->Length(), 0);
    }
}

HWTEST_F_L0(ContainersListTest, Values)
{
    constexpr int NODE_NUMBERS = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }

    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(list.GetTaggedValue());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1.get());
    JSHandle<JSTaggedValue> iterValues(thread, ContainersList::GetIteratorObj(callInfo1.get()));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(iterValues->IsJSAPIListIterator());

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        result.Update(JSAPIListIterator::Next(callInfo.get()));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(i, JSIterator::IteratorValue(thread, result)->GetInt());
    }
}

HWTEST_F_L0(ContainersListTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIList> list = CreateJSAPIList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(list->Length(), static_cast<int>(i + 1));
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIList> dlist = CreateJSAPIList();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersList::ForEach(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(list.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersList::Get(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }
}
}  // namespace panda::test
