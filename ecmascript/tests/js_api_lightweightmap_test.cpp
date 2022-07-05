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

#include "ecmascript/ecma_string.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_lightweightmap.h"
#include "ecmascript/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPILightWeightMapTest : public testing::Test {
public:
    const static int DEFAULT_SIZE = 8;
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

protected:
    JSAPILightWeightMap *CreateLightWeightMap()
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread,
                                                              JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightMap)));

        auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSHandle<JSTaggedValue> constructor =
            JSHandle<JSTaggedValue>(thread, containers::ContainersPrivate::Load(objCallInfo));
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPILightWeightMap> lightWeightMap = JSHandle<JSAPILightWeightMap>::
            Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_SIZE));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_SIZE));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(DEFAULT_SIZE));
        lightWeightMap->SetHashes(thread, hashArray);
        lightWeightMap->SetKeys(thread, keyArray);
        lightWeightMap->SetValues(thread, valueArray);
        lightWeightMap->SetLength(0);
        return *lightWeightMap;
    }
};

HWTEST_F_L0(JSAPILightWeightMapTest, LightWeightMapCreate)
{
    JSAPILightWeightMap *lightWeightMap = CreateLightWeightMap();
    EXPECT_TRUE(lightWeightMap != nullptr);
}

HWTEST_F_L0(JSAPILightWeightMapTest, SetHasKeyGetHasValue)
{
    JSAPILightWeightMap *lightWeightMap = CreateLightWeightMap();

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSHandle<JSAPILightWeightMap> lwm(thread, lightWeightMap);
    JSAPILightWeightMap::Set(thread, lwm, key, value);
    EXPECT_TRUE(JSTaggedValue::Equal(thread,
                                     JSHandle<JSTaggedValue>(thread,
                                                             JSAPILightWeightMap::Get(thread, lwm, key)),
                                                             value));

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);

     // test species
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> key3 = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(5));
    JSAPILightWeightMap::Set(thread, lwm, key3, value3);

    EXPECT_TRUE(JSTaggedValue::Equal(thread,
                                     JSHandle<JSTaggedValue>(thread,
                                                             JSAPILightWeightMap::Get(thread, lwm, key)),
                                                             value));
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key1), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key3), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, value), JSTaggedValue::True());
}

HWTEST_F_L0(JSAPILightWeightMapTest, GetIndexOfKeyAndGetIndexOfValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILightWeightMap::Set(thread, lwm, key, value);
        EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfKey(thread, lwm, key) != -1);
        EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfValue(thread, lwm, value) != -1);
        uint32_t length = lwm->GetLength();
        EXPECT_EQ(length, i + 1);
    }
}

HWTEST_F_L0(JSAPILightWeightMapTest, IsEmptyGetKeyAtGetValue)
{
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSAPILightWeightMap::Set(thread, lwm, key, value);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);

    JSHandle<JSTaggedValue> result =
        JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 1));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value1));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 2));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value2));

    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 1));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key1));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 2));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key2));

    JSAPILightWeightMap::Clear(thread, lwm);
    EXPECT_EQ(lwm->IsEmpty(), JSTaggedValue::True());
}

HWTEST_F_L0(JSAPILightWeightMapTest, RemoveRemoveAt)
{
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());
    JSHandle<TaggedArray> valueArray(thread,
                                     JSTaggedValue(TaggedArray::Cast(lwm->GetValues().GetTaggedObject())));

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSAPILightWeightMap::Set(thread, lwm, key, value);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);
    
    JSHandle<JSTaggedValue> result =
        JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::Remove(thread, lwm, key2));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value2));
    EXPECT_EQ(JSAPILightWeightMap::RemoveAt(thread, lwm, 0), JSTaggedValue::True());
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value1));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key1));
}

HWTEST_F_L0(JSAPILightWeightMapTest, IncreaseCapacityTo)
{
    constexpr uint32_t NODE_NUMBERS = 10;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILightWeightMap::Set(thread, lwm, key, value);
        EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfKey(thread, lwm, key) != -1);
        EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfValue(thread, lwm, value) != -1);
        uint32_t length = lwm->GetLength();
        EXPECT_EQ(length, i + 1);
    }
    EXPECT_EQ(JSAPILightWeightMap::IncreaseCapacityTo(thread, lwm, 15), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::IncreaseCapacityTo(thread, lwm, 9), JSTaggedValue::False());
}

HWTEST_F_L0(JSAPILightWeightMapTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i + 1));
        JSAPILightWeightMap::Set(thread, lwm, key, value);
    }

    // test key or value
    JSHandle<JSTaggedValue> keyIter(factory->NewJSAPILightWeightMapIterator(lwm, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPILightWeightMapIterator(lwm, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> keyIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        JSTaggedValue k = JSIterator::IteratorValue(thread, keyIterResult).GetTaggedValue();
        keyHandle.Update(k);
        JSTaggedValue v = JSIterator::IteratorValue(thread, valueIterResult).GetTaggedValue();
        EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, keyHandle), JSTaggedValue::True());
        EXPECT_EQ(JSAPILightWeightMap::Get(thread, lwm, keyHandle), v);
    }

    // test key and value
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue k = JSTaggedValue(i);
        JSTaggedValue v = JSTaggedValue(i + 1);
        keyHandle.Update(k);
        EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, keyHandle), JSTaggedValue::True());
        EXPECT_EQ(JSAPILightWeightMap::Get(thread, lwm, keyHandle), v);
    }
}
}  // namespace panda::test
