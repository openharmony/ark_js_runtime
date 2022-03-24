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
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsWeakMap = ecmascript::builtins::BuiltinsWeakMap;
using JSWeakMap = ecmascript::JSWeakMap;

class BuiltinsWeakMapTest : public testing::Test {
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
};

static JSObject *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
    return *factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
}

JSWeakMap *CreateBuiltinsWeakMap(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsWeakMapFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, newTarget.GetTaggedValue(), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsWeakMap::WeakMapConstructor(ecmaRuntimeCallInfo.get());

    EXPECT_TRUE(result.IsECMAObject());
    return JSWeakMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
}

// new Map("abrupt").toString()
HWTEST_F_L0(BuiltinsWeakMapTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsWeakMapFunction());
    JSHandle<JSWeakMap> map(thread, CreateBuiltinsWeakMap(thread));

    JSHandle<TaggedArray> array(factory->NewTaggedArray(1));
    JSHandle<TaggedArray> internal_array(factory->NewTaggedArray(2));
    JSTaggedValue value(JSObjectTestCreate(thread));
    internal_array->Set(thread, 0, value);
    internal_array->Set(thread, 1, JSTaggedValue(0));
    auto result = JSArray::CreateArrayFromList(thread, internal_array);
    array->Set(thread, 0, result);

    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo->SetNewTarget(newTarget.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());

    JSTaggedValue result1 = BuiltinsWeakMap::WeakMapConstructor(ecmaRuntimeCallInfo.get());
    JSHandle<JSWeakMap> weakMap(thread, JSWeakMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    EXPECT_EQ(weakMap->GetSize(), 1);
}

HWTEST_F_L0(BuiltinsWeakMapTest, SetAndHas)
{
    // create jsWeakMap
    JSHandle<JSWeakMap> weakMap(thread, CreateBuiltinsWeakMap(thread));
    JSHandle<JSTaggedValue> key(thread, JSObjectTestCreate(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(weakMap.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsWeakMap::Has(ecmaRuntimeCallInfo.get());

        EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());
    }

    // test Set()
    JSTaggedValue result2 = BuiltinsWeakMap::Set(ecmaRuntimeCallInfo.get());
    EXPECT_TRUE(result2.IsECMAObject());
    JSWeakMap *jsWeakMap = JSWeakMap::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
    EXPECT_EQ(jsWeakMap->GetSize(), 1);

    // test Has()
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(jsWeakMap));
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
        JSTaggedValue result3 = BuiltinsWeakMap::Has(ecmaRuntimeCallInfo1.get());

        EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
    }
}

HWTEST_F_L0(BuiltinsWeakMapTest, DeleteAndRemove)
{
    // create jsWeakMap
    JSHandle<JSWeakMap> weakMap(thread, CreateBuiltinsWeakMap(thread));

    // add 40 keys
    JSTaggedValue lastKey(JSTaggedValue::Undefined());
    for (int i = 0; i < 40; i++) {
        JSHandle<JSTaggedValue> key(thread, JSObjectTestCreate(thread));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(weakMap.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(i)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
        JSTaggedValue result1 = BuiltinsWeakMap::Set(ecmaRuntimeCallInfo.get());

        EXPECT_TRUE(result1.IsECMAObject());
        JSWeakMap *jsWeakMap = JSWeakMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsWeakMap->GetSize(), static_cast<int>(i) + 1);
        lastKey = key.GetTaggedValue();
    }

    // whether jsWeakMap has delete lastKey

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(weakMap.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, lastKey);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = BuiltinsWeakMap::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    JSTaggedValue result3 = BuiltinsWeakMap::Delete(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    JSTaggedValue result4 = BuiltinsWeakMap::Has(ecmaRuntimeCallInfo1.get());

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
}
}  // namespace panda::test
