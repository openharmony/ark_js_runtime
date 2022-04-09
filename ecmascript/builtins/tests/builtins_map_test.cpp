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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsMap = ecmascript::builtins::BuiltinsMap;
using JSMap = ecmascript::JSMap;

class BuiltinsMapTest : public testing::Test {
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
        static JSTaggedValue TestFunc(EcmaRuntimeCallInfo *argv)
        {
            int num = GetCallArg(argv, 0)->GetInt();
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            int length = jsArray->GetArrayLength() + num;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }
    };
};

JSMap *CreateBuiltinsMap(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsMap::MapConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());
    JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return jsMap;
}
// new Map("abrupt").toString()
HWTEST_F_L0(BuiltinsMapTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result = BuiltinsMap::GetSize(ecmaRuntimeCallInfo.get());

        EXPECT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());
    }
    JSHandle<TaggedArray> array(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> internal_array(factory->NewTaggedArray(2));
        internal_array->Set(thread, 0, JSTaggedValue(i));
        internal_array->Set(thread, 1, JSTaggedValue(i));
        auto arr = JSArray::CreateArrayFromList(thread, internal_array);
        array->Set(thread, i, arr);
    }
    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetNewTarget(newTarget.GetTaggedValue());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
        JSTaggedValue result1 = BuiltinsMap::MapConstructor(ecmaRuntimeCallInfo1.get());

        EXPECT_EQ(JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()))->GetSize(), 5);
    }
}

HWTEST_F_L0(BuiltinsMapTest, SetAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    JSMap *jsMap;
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsMap::Has(ecmaRuntimeCallInfo.get());

        EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

        // test Set()
        JSTaggedValue result2 = BuiltinsMap::Set(ecmaRuntimeCallInfo.get());

        EXPECT_TRUE(result2.IsECMAObject());
        jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(), 1);
    }

    // test Has()
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(jsMap));
    ecmaRuntimeCallInfo1->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
        JSTaggedValue result3 = BuiltinsMap::Has(ecmaRuntimeCallInfo1.get());

        EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
    }
}

HWTEST_F_L0(BuiltinsMapTest, ForEach)
{
    // generate a map has 5 entries{key1:0,key2:1,key3:2,key4:3,key5:4}
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));
    char keyArray[] = "key0";
    for (int i = 0; i < 5; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(i)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsMap::Set(ecmaRuntimeCallInfo.get());
        EXPECT_TRUE(result1.IsECMAObject());
        JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(), static_cast<int>(i) + 1);
    }
    // test foreach;
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = BuiltinsMap::ForEach(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 10U);
}

HWTEST_F_L0(BuiltinsMapTest, DeleteAndRemove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));

    // add 40 keys
    char keyArray[] = "key0";
    for (int i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(thread, factory->NewFromASCII(keyArray).GetTaggedValue());
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(i)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsMap::Set(ecmaRuntimeCallInfo.get());

        EXPECT_TRUE(result1.IsECMAObject());
        JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(), static_cast<int>(i) + 1);
    }
    // whether jsMap has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, deleteKey.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = BuiltinsMap::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    JSTaggedValue result3 = BuiltinsMap::Delete(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    JSTaggedValue result4 = BuiltinsMap::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
    JSTaggedValue result5 = BuiltinsMap::GetSize(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result5.GetRawData(), JSTaggedValue(39).GetRawData());

    // clear
    JSTaggedValue result6 = BuiltinsMap::Clear(ecmaRuntimeCallInfo1.get());
    EXPECT_EQ(result6.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(map->GetSize(), 0);
}

HWTEST_F_L0(BuiltinsMapTest, Species)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> map(thread, CreateBuiltinsMap(thread));

    // test species
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    EXPECT_EQ(value, newTarget.GetTaggedValue());

    // to string tag
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<EcmaString> stringTag(JSObject::GetProperty(thread, map, toStringTagSymbol).GetValue());
    JSHandle<EcmaString> str = factory->NewFromASCII("Map");
    EXPECT_TRUE(!stringTag.GetTaggedValue().IsUndefined());
    EXPECT_TRUE(EcmaString::StringsAreEqual(*str, *stringTag));

    JSHandle<JSFunction> constructor = JSHandle<JSFunction>::Cast(JSTaggedValue::ToObject(thread, valueHandle));
    EXPECT_EQ(JSHandle<JSObject>(map)->GetPrototype(thread), constructor->GetFunctionPrototype());

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("set"));
    JSTaggedValue value1 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value1.IsUndefined());

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("has"));
    JSTaggedValue value2 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value2.IsUndefined());

    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("clear"));
    JSTaggedValue value3 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value3.IsUndefined());

    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("size"));
    JSTaggedValue value4 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value4.IsUndefined());

    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("delete"));
    JSTaggedValue value5 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value5.IsUndefined());

    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("forEach"));
    JSTaggedValue value6 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value6.IsUndefined());

    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("get"));
    JSTaggedValue value7 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value7.IsUndefined());
}

HWTEST_F_L0(BuiltinsMapTest, GetIterator)
{
    JSHandle<JSTaggedValue> map(thread, CreateBuiltinsMap(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());

    // test Values()
    JSTaggedValue result = BuiltinsMap::Values(ecmaRuntimeCallInfo.get());
    JSHandle<JSMapIterator> iter(thread, result);
    EXPECT_TRUE(iter->IsJSMapIterator());
    EXPECT_EQ(IterationKind::VALUE, iter->GetIterationKind());
    EXPECT_EQ(JSMap::Cast(map.GetTaggedValue().GetTaggedObject())->GetLinkedMap(), iter->GetIteratedMap());

    // test Keys()
    JSTaggedValue result1 = BuiltinsMap::Keys(ecmaRuntimeCallInfo.get());
    JSHandle<JSMapIterator> iter1(thread, result1);
    EXPECT_TRUE(iter1->IsJSMapIterator());
    EXPECT_EQ(IterationKind::KEY, iter1->GetIterationKind());

    // test entries()
    JSTaggedValue result2 = BuiltinsMap::Entries(ecmaRuntimeCallInfo.get());
    JSHandle<JSMapIterator> iter2(thread, result2);
    EXPECT_TRUE(iter2->IsJSMapIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
}
}  // namespace panda::test
