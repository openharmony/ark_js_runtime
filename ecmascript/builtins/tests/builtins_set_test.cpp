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

#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsSet = ecmascript::builtins::BuiltinsSet;
using JSSet = ecmascript::JSSet;

class BuiltinsSetTest : public testing::Test {
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
        static JSTaggedValue TestFunc(EcmaRuntimeCallInfo *argv)
        {
            JSTaggedValue key = GetCallArg(argv, 0).GetTaggedValue();
            if (key.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            int length = jsArray->GetArrayLength() + 1;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }
    };
};

JSSet *CreateBuiltinsSet(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsSetFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsSet::SetConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());
    return JSSet::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
}
// new Set("abrupt").toString()
HWTEST_F_L0(BuiltinsSetTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsSetFunction());
    JSHandle<JSSet> set(thread, CreateBuiltinsSet(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = BuiltinsSet::GetSize(ecmaRuntimeCallInfo.get());

        EXPECT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());
    }

    JSHandle<TaggedArray> array(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        array->Set(thread, i, JSTaggedValue(i));
    }

    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetNewTarget(newTarget.GetTaggedValue());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
        JSTaggedValue result1 = BuiltinsSet::SetConstructor(ecmaRuntimeCallInfo1.get());

        EXPECT_EQ(JSSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()))->GetSize(), 5);
    }
}

HWTEST_F_L0(BuiltinsSetTest, AddAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSet> set(thread, CreateBuiltinsSet(thread));
    JSHandle<JSTaggedValue> key(thread, factory->NewFromASCII("key").GetTaggedValue());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result1 = BuiltinsSet::Has(ecmaRuntimeCallInfo.get());

    EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

    // test Add()
    JSTaggedValue result2 = BuiltinsSet::Add(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result2.IsECMAObject());
    JSSet *jsSet = JSSet::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
    EXPECT_EQ(jsSet->GetSize(), 1);

    // test Has()
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(jsSet));
    ecmaRuntimeCallInfo1->SetCallArg(0, key.GetTaggedValue());
    {
        [[maybe_unused]] auto prevLocal = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
        JSTaggedValue result3 = BuiltinsSet::Has(ecmaRuntimeCallInfo1.get());

        EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
    }

    // test -0.0
    JSHandle<JSTaggedValue> negativeZero(thread, JSTaggedValue(-0.0));
    JSHandle<JSTaggedValue> positiveZero(thread, JSTaggedValue(+0.0));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue(jsSet));
    ecmaRuntimeCallInfo2->SetCallArg(0, negativeZero.GetTaggedValue());
    {
        [[maybe_unused]] auto prevLocal = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
        BuiltinsSet::Add(ecmaRuntimeCallInfo.get());
    }

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue(jsSet));
    ecmaRuntimeCallInfo3->SetCallArg(0, positiveZero.GetTaggedValue());
    {
        [[maybe_unused]] auto prevLocal = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
        JSTaggedValue result4 = BuiltinsSet::Has(ecmaRuntimeCallInfo3.get());

        EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
    }
}

HWTEST_F_L0(BuiltinsSetTest, ForEach)
{
    // generate a set has 5 entry{key1,key2,key3,key4,key5}
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSet> set(thread, CreateBuiltinsSet(thread));
    char keyArray[] = "key0";
    for (int i = 0; i < 5; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsSet::Add(ecmaRuntimeCallInfo.get());

        EXPECT_TRUE(result1.IsECMAObject());
        JSSet *jsSet = JSSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsSet->GetSize(), static_cast<int>(i) + 1);
    }
    // test foreach;
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func =
        factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(), reinterpret_cast<void *>(TestClass::TestFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = BuiltinsSet::ForEach(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 5U);
}

HWTEST_F_L0(BuiltinsSetTest, DeleteAndRemove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSet> set(thread, CreateBuiltinsSet(thread));

    // add 40 keys
    char keyArray[] = "key0";
    for (int i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsSet::Add(ecmaRuntimeCallInfo.get());

        EXPECT_TRUE(result1.IsECMAObject());
        JSSet *jsSet = JSSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsSet->GetSize(), static_cast<int>(i) + 1);
    }
    // whether jsSet has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, deleteKey.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = BuiltinsSet::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    JSTaggedValue result3 = BuiltinsSet::Delete(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    JSTaggedValue result4 = BuiltinsSet::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());

    JSTaggedValue result5 = BuiltinsSet::GetSize(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result5.GetRawData(), JSTaggedValue(39).GetRawData());

    // clear
    JSTaggedValue result6 = BuiltinsSet::Clear(ecmaRuntimeCallInfo1.get());
    EXPECT_EQ(result6.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(set->GetSize(), 0);
}

HWTEST_F_L0(BuiltinsSetTest, Species)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> set(thread, CreateBuiltinsSet(thread));
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol->IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetBuiltinsSetFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    EXPECT_EQ(value, newTarget.GetTaggedValue());

    // to string tag
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<EcmaString> stringTag(JSObject::GetProperty(thread, set, toStringTagSymbol).GetValue());
    JSHandle<EcmaString> str = factory->NewFromASCII("Set");
    EXPECT_TRUE(!stringTag.GetTaggedValue().IsUndefined());
    EXPECT_TRUE(EcmaString::StringsAreEqual(*str, *stringTag));

    JSHandle<JSFunction> constructor = JSHandle<JSFunction>::Cast(JSTaggedValue::ToObject(thread, valueHandle));
    EXPECT_EQ(JSTaggedValue::GetPrototype(thread, set), constructor->GetFunctionPrototype());

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("add"));
    JSTaggedValue value1 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value1.IsUndefined());

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("has"));
    JSTaggedValue value2 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value2.IsUndefined());

    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("clear"));
    JSTaggedValue value3 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value3.IsUndefined());

    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("size"));
    JSTaggedValue value4 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value4.IsUndefined());

    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("delete"));
    JSTaggedValue value5 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value5.IsUndefined());

    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("forEach"));
    JSTaggedValue value6 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value6.IsUndefined());
}

HWTEST_F_L0(BuiltinsSetTest, GetIterator)
{
    JSHandle<JSTaggedValue> set(thread, CreateBuiltinsSet(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());

    // test Values()
    JSTaggedValue result = BuiltinsSet::Values(ecmaRuntimeCallInfo.get());
    JSHandle<JSSetIterator> iter(thread, result);
    EXPECT_TRUE(iter->IsJSSetIterator());
    EXPECT_EQ(IterationKind::VALUE, IterationKind(iter->GetIterationKind()));
    EXPECT_EQ(JSSet::Cast(set.GetTaggedValue().GetTaggedObject())->GetLinkedSet(), iter->GetIteratedSet());

    // test entries()
    JSTaggedValue result2 = BuiltinsSet::Entries(ecmaRuntimeCallInfo.get());
    JSHandle<JSSetIterator> iter2(thread, result2);
    EXPECT_TRUE(iter2->IsJSSetIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
}
}  // namespace panda::test
