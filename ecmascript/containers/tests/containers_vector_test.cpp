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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_vector.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_vector.h"
#include "ecmascript/js_api_vector_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersVectorTest : public testing::Test {
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
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIVector::Cast(vector.GetTaggedValue().
                                            GetTaggedObject())->GetElements().GetTaggedObject());
                    JSTaggedValue result = elements->Get(key->GetInt());
                    EXPECT_EQ(result, value.GetTaggedValue());
                }
            }
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestReplaceAllElementsFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSHandle<JSAPIVector>::Cast(vector)->Set(thread, index->GetNumber(), newValue.GetTaggedValue());
                    return newValue.GetTaggedValue();
                }
            }
            return JSTaggedValue::Undefined();
        }
    };
protected:
    JSTaggedValue InitializeVectorConstructor()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::Vector)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIVector> CreateJSAPIVector(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeVectorConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersVector::VectorConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIVector> vector(thread, result);
        return vector;
    }
};

// new vector
HWTEST_F_L0(ContainersVectorTest, VectorConstructor)
{
    // Initialize twice and return directly the second time
    InitializeVectorConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeVectorConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersVector::VectorConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIVector());
    
    JSHandle<JSAPIVector> setHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(setHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = setHandle->GetSize();
    ASSERT_EQ(size, 0);
}

// add  has
HWTEST_F_L0(ContainersVectorTest, AddAndHas)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // test add insert string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS + i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2);

    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Insert(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2 + i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 3);

    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

// getIndexOf  getLastIndexOf
HWTEST_F_L0(ContainersVectorTest, GetFirstValueAndGetLastValue)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        // double
        result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), 2 * (i + 1));
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2);

    // getIndexOf
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2 * i));
    }
    // getLastIndexOf
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2 * i + 1));
    }
    // getFirstElement
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetFirstElement(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // getLastElement
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastElement(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(7));
    }
}

// removeByIndex  remove  removeByRange  clear  isEmpty
HWTEST_F_L0(ContainersVectorTest, RemoveByIndexAndRemoveAndRemoveRangeAndClear)
{
    // add
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }

    // removeByIndex
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::RemoveByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
        EXPECT_EQ(vector->GetSize(), 7);
    }
    // remove fail
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
        EXPECT_EQ(vector->GetSize(), 7);
    }
    // remove success
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), 6);
    }
    // removeByRange
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(2));
        callInfo->SetCallArg(1, JSTaggedValue(6));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::RemoveByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), 2);
    }
    // isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::IsEmpty(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
    }
    // clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), 0);
    }
    // isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::IsEmpty(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

// replaceAllElements  forEach
HWTEST_F_L0(ContainersVectorTest, ReplaceAllElementsAndForEach)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // forEach
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIVector> vec = CreateJSAPIVector();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, vec.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // replaceAllElements
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReplaceAllElementsFunc));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::ReplaceAllElements(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i * 2));
        }
    }
}

// sort
HWTEST_F_L0(ContainersVectorTest, Sort)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // sort
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
}

// clone  convertToArray  copyToArray
HWTEST_F_L0(ContainersVectorTest, CloneAndConvertToArrayAndCopyToArray)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    // clone
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue vec = ContainersVector::Clone(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIVector> handleVec(thread, vec);
        EXPECT_EQ(handleVec->GetSize(), vector->GetSize());

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vec);
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
    // convertToArray
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue arr = ContainersVector::ConvertToArray(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(arr.IsJSArray());
        JSHandle<JSTaggedValue> handleArr(thread, arr);
        JSHandle<TaggedArray> taggedArr = JSArray::ToTaggedArray(thread, handleArr);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            JSTaggedValue result = taggedArr->Get(i);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
    // copyToArray
    {
        JSHandle<JSArray> array = thread->GetEcmaVM()->GetFactory()->NewJSArray();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, array.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::CopyToArray(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(array->IsJSArray());
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(array.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            int result = JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(array), i).GetValue()->GetInt();
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, i);
        }
    }
}

// subVector  getCapacity  trimToCurrentLength  increaseCapacityTo
HWTEST_F_L0(ContainersVectorTest, SubVectorAndGetCapacityAndTrimToCurrentLengthAndIncreaseCapacityTo)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t capacity = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(capacity, 10);
    }
    // increaseCapacityTo
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(20));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::IncreaseCapacityTo(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t newCapacity1 = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(newCapacity1, 20);
    }
    // trimToCurrentLength
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::TrimToCurrentLength(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t newCapacity2 = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(newCapacity2, 8);
    }
    // subVector
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue vec = ContainersVector::SubVector(callInfo);
        for (int32_t i = 0; i < 2; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vec);
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
}

// toString  getLastIndexFrom  getIndexFrom  setLength
HWTEST_F_L0(ContainersVectorTest, ToStringAndGetLastIndexFromAndGetIndexFrom)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    // toString
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::ToString(callInfo);
        
        EXPECT_TRUE(result.IsString());
    }
    // getLastIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, JSTaggedValue(1)));
    }
    // getIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, JSTaggedValue(-1)));
    }
    // getIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(10));
        callInfo->SetCallArg(1, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, JSTaggedValue(-1)));
    }
}
}
