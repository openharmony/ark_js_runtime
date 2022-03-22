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
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_tree_set.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersTreeSetTest : public testing::Test {
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

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSAPITreeSet> set(GetCallArg(argv, 2)); // 2 means the second arg
            EXPECT_EQ(key.GetTaggedValue(), value.GetTaggedValue());
            JSAPITreeSet::Delete(thread, set, key);

            JSHandle<JSAPITreeSet> jsTreeSet(GetThis(argv));
            JSAPITreeSet::Add(thread, jsTreeSet, key);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestCompareFunction(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> valueX = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> valueY = GetCallArg(argv, 1);

            if (valueX->IsString() && valueY->IsString()) {
                auto xString = static_cast<EcmaString *>(valueX->GetTaggedObject());
                auto yString = static_cast<EcmaString *>(valueY->GetTaggedObject());
                int result = xString->Compare(yString);
                if (result < 0) {
                    return JSTaggedValue(1);
                }
                if (result == 0) {
                    return JSTaggedValue(0);
                }
                return JSTaggedValue(-1);
            }

            if (valueX->IsNumber() && valueY->IsString()) {
                return JSTaggedValue(1);
            }
            if (valueX->IsString() && valueY->IsNumber()) {
                return JSTaggedValue(-1);
            }

            ComparisonResult res = ComparisonResult::UNDEFINED;
            if (valueX->IsNumber() && valueY->IsNumber()) {
                res = JSTaggedValue::StrictNumberCompare(valueY->GetNumber(), valueX->GetNumber());
            } else {
                res = JSTaggedValue::Compare(thread, valueY, valueX);
            }
            return res == ComparisonResult::GREAT ?
                JSTaggedValue(1) : (res == ComparisonResult::LESS ? JSTaggedValue(-1) : JSTaggedValue(0));
        }
    };

protected:
    JSTaggedValue InitializeTreeSetConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::TreeSet)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPITreeSet> CreateJSAPITreeSet(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeTreeSetConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
        JSTaggedValue result = ContainersTreeSet::TreeSetConstructor(objCallInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPITreeSet> set(thread, result);
        return set;
    }
};

// new TreeSet()
HWTEST_F_L0(ContainersTreeSetTest, TreeSetConstructor)
{
    // Initialize twice and return directly the second time
    InitializeTreeSetConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeTreeSetConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo.get());
    JSTaggedValue result = ContainersTreeSet::TreeSetConstructor(objCallInfo.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPITreeSet());
    JSHandle<JSAPITreeSet> setHandle(thread, result);
    JSTaggedValue resultProto = JSHandle<JSObject>::Cast(setHandle)->GetPrototype(thread);
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = setHandle->GetSize();
    ASSERT_EQ(size, 0);
}

// treeset.add(value), treeset.has(value)
HWTEST_F_L0(ContainersTreeSetTest, AddAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS);

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

// treeset.remove(key)
HWTEST_F_L0(ContainersTreeSetTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    constexpr uint32_t REMOVE_SIZE = 48;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }

    for (uint32_t i = 0; i < REMOVE_SIZE; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue rvalue = ContainersTreeSet::Remove(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(rvalue.IsTrue());
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS - REMOVE_SIZE);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i < REMOVE_SIZE) {
            EXPECT_TRUE(result.IsFalse());
        } else {
            EXPECT_TRUE(result.IsTrue());
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS - REMOVE_SIZE + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2 - REMOVE_SIZE);

    for (uint32_t i = 0; i < REMOVE_SIZE; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue rvalue = ContainersTreeSet::Remove(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(rvalue.IsTrue());
    }
    EXPECT_EQ(tset->GetSize(), (NODE_NUMBERS - REMOVE_SIZE) * 2);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i < REMOVE_SIZE) {
            EXPECT_TRUE(result.IsFalse());
        } else {
            EXPECT_TRUE(result.IsTrue());
        }
    }
}

// treeset.getFirstValue(), treeset.getLastValue()
HWTEST_F_L0(ContainersTreeSetTest, GetFirstValueAndGetLastValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    // test getFirstValue
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetFirstValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // test getLastValue
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetLastValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS - 1));
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2);

    // test getFirstValue
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetFirstValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // test getLastValue
    {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetLastValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, key.GetTaggedValue());
    }
}

// treeset.clear()
HWTEST_F_L0(ContainersTreeSetTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersTreeSet::Clear(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tset->GetSize(), 0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS);
    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersTreeSet::Clear(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tset->GetSize(), 0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
    }
}

// treeset.getLowerValue(value), treeset.getHigherValue(value)
HWTEST_F_L0(ContainersTreeSetTest, GetLowerValueAndGetHigherValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }

    // test getLowerValue
    for (uint32_t i = 0; i <= NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetLowerValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i == 0) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i - 1));
        }
    }
    // test getHigherValue
    for (uint32_t i = 0; i <= NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetHigherValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i >= NODE_NUMBERS - 1) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i + 1));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2);

    // test getLowerValue
    // using to compare the result of GetLowerValue
    JSMutableHandle<JSTaggedValue> resultKey(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i <= NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string rkey = myKey + std::to_string(i - 1);
        resultKey.Update(factory->NewFromStdString(rkey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetLowerValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i == 0) {
            EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS - 1));
        } else {
            EXPECT_EQ(result, resultKey.GetTaggedValue());
        }
    }
    // test getHigherValue
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string rkey = myKey + std::to_string(i + 1);
        resultKey.Update(factory->NewFromStdString(rkey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::GetHigherValue(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        if (i == NODE_NUMBERS - 1) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_TRUE(JSTaggedValue::SameValue(result, resultKey.GetTaggedValue()));
        }
    }
}

// treeset.popFirst(), treeset.popLast()
HWTEST_F_L0(ContainersTreeSetTest, PopFirstAndPopLast)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }

    // test popFirst
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::PopFirst(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS - 1);
    }
    // test popLast
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::PopLast(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS - 1));
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS - 2); // 2 means two elements
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i - 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2 - 2);

    // test popFirst
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::PopFirst(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(1));
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2 - 3); // 3 means three elements
    }
    // test popLast
    {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::PopLast(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, key.GetTaggedValue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2 - 4); // 4 means four elements
    }
}

// testset.isEmpty()
HWTEST_F_L0(ContainersTreeSetTest, IsEmpty)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    // test isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::IsEmpty(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), 0);
    }

    // add elements
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    // test isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::IsEmpty(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS);
    }
}

// treeset.values(), treeset.entries()
HWTEST_F_L0(ContainersTreeSetTest, KeysAndValuesAndEntries)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }

    // test values
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(tset.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
    JSHandle<JSTaggedValue> iterValues(thread, ContainersTreeSet::Values(callInfo.get()));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterValues->IsJSAPITreeSetIterator());
    {
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iterValues.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
            result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(i, JSIterator::IteratorValue(thread, result)->GetInt());
        }
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2);
    {
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            std::string ikey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iterValues.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
            result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
            TestHelper::TearDownFrame(thread, prev);
            JSHandle<JSTaggedValue> itRes = JSIterator::IteratorValue(thread, result);
            EXPECT_TRUE(JSTaggedValue::SameValue(key, itRes));
        }
    }
    // test entries
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSHandle<JSTaggedValue> iter(thread, ContainersTreeSet::Entries(callInfo.get()));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(iter->IsJSAPITreeSetIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
            result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());
            EXPECT_EQ(i, JSObject::GetProperty(thread, entries, first).GetValue()->GetInt());
            EXPECT_EQ(i, JSObject::GetProperty(thread, entries, second).GetValue()->GetInt());
        }
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            std::string ikey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
            result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());
            EXPECT_TRUE(JSTaggedValue::SameValue(key, JSObject::GetProperty(thread, entries, first).GetValue()));
            EXPECT_TRUE(JSTaggedValue::SameValue(key, JSObject::GetProperty(thread, entries, second).GetValue()));
        }
    }
}

// treeset.ForEach(callbackfn, this)
HWTEST_F_L0(ContainersTreeSetTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }

    // test foreach function with TestForEachFunc;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeSet> dset = CreateJSAPITreeSet();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersTreeSet::ForEach(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
    }

    EXPECT_EQ(dset->GetSize(), NODE_NUMBERS / 2);
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS / 2);
    for (uint32_t i = 0; i < NODE_NUMBERS; i += 2) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }

    // test add string
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS / 2 + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS / 2 + NODE_NUMBERS);
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dset.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        ContainersTreeSet::ForEach(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(dset->GetSize(), NODE_NUMBERS + 2);
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS - 2);
    for (uint32_t i = 0; i < NODE_NUMBERS; i += 2) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Has(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

HWTEST_F_L0(ContainersTreeSetTest, CustomCompareFunctionTest)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestCompareFunction));
    JSHandle<JSAPITreeSet> tset = CreateJSAPITreeSet(func.GetTaggedValue());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS);

    // test add string
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tset.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        JSTaggedValue result = ContainersTreeSet::Add(callInfo.get());
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tset->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tset->GetSize(), NODE_NUMBERS * 2);

    // test sort
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(tset.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
    JSHandle<JSTaggedValue> iterValues(thread, ContainersTreeSet::Values(callInfo.get()));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterValues->IsJSAPITreeSetIterator());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1 - i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSTaggedValue> itRes = JSIterator::IteratorValue(thread, result);
        EXPECT_TRUE(JSTaggedValue::SameValue(key, itRes));
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo.get());
        result.Update(JSAPITreeSetIterator::Next(callInfo.get()));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ((NODE_NUMBERS - 1 - i), JSIterator::IteratorValue(thread, result)->GetInt());
    }
}
}  // namespace panda::test
