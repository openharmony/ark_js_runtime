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
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSArrayTest : public testing::Test {
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

HWTEST_F_L0(JSArrayTest, ArrayCreate)
{
    JSHandle<JSTaggedValue> lengthKeyHandle(thread->GlobalConstants()->GetHandledLengthString());
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    JSArray *arr2 = JSArray::ArrayCreate(thread, JSTaggedNumber(10)).GetObject<JSArray>();
    EXPECT_TRUE(arr2 != nullptr);
    JSHandle<JSTaggedValue> obj2(thread, arr2);
    EXPECT_EQ(JSArray::GetProperty(thread, obj2, lengthKeyHandle).GetValue()->GetInt(), 10);
}

HWTEST_F_L0(JSArrayTest, ArraySpeciesCreate)
{
    JSHandle<JSTaggedValue> lengthKeyHandle(thread->GlobalConstants()->GetHandledLengthString());
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSArray *arr2 = JSArray::Cast(JSArray::ArraySpeciesCreate(thread, obj, JSTaggedNumber(10)).GetTaggedObject());
    EXPECT_TRUE(arr2 != nullptr);
    JSHandle<JSTaggedValue> obj2(thread, arr2);
    EXPECT_EQ(JSArray::GetProperty(thread, obj2, lengthKeyHandle).GetValue()->GetInt(), 10);
}

HWTEST_F_L0(JSArrayTest, DefineOwnProperty)
{
    auto ecmaVM = thread->GetEcmaVM();
    auto factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> lengthKeyHandle(thread->GlobalConstants()->GetHandledLengthString());
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)), true, true, true);

    EcmaString *string1 = *factory->NewFromCanBeCompressString("1");
    JSHandle<JSTaggedValue> key1(thread, static_cast<TaggedObject *>(string1));
    JSHandle<JSTaggedValue> index1(thread, JSTaggedValue(1));
    EXPECT_TRUE(JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key1, desc));
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 2);
    JSTaggedValue v = JSArray::GetProperty(thread, obj, key1).GetValue().GetTaggedValue();
    EXPECT_EQ(v.GetInt(), 100);
    v = JSArray::GetProperty(thread, obj, index1).GetValue().GetTaggedValue();
    EXPECT_EQ(v.GetInt(), 100);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 2);

    EcmaString *string100 = *factory->NewFromCanBeCompressString("100");
    JSHandle<JSTaggedValue> key100(thread, static_cast<TaggedObject *>(string100));
    JSHandle<JSTaggedValue> index100(thread, JSTaggedValue(100));

    EXPECT_TRUE(JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key100, desc));
    EXPECT_EQ(JSArray::GetProperty(thread, obj, key100).GetValue()->GetInt(), 100);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, index100).GetValue()->GetInt(), 100);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 101);

    EcmaString *stringx = *factory->NewFromCanBeCompressString("2147483646");
    JSHandle<JSTaggedValue> keyx(thread, static_cast<TaggedObject *>(stringx));
    JSHandle<JSTaggedValue> indexx(thread, JSTaggedValue(2147483646U));  // 2147483646U

    EXPECT_TRUE(JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), keyx, desc));
    EXPECT_EQ(JSArray::GetProperty(thread, obj, keyx).GetValue()->GetInt(), 100);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, indexx).GetValue()->GetInt(), 100);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 2147483647);

    EXPECT_TRUE(JSArray::DeleteProperty(thread, JSHandle<JSObject>(obj), indexx));
    EXPECT_TRUE(JSArray::GetProperty(thread, obj, keyx).GetValue()->IsUndefined());
    EXPECT_TRUE(JSArray::GetProperty(thread, obj, indexx).GetValue()->IsUndefined());
}

HWTEST_F_L0(JSArrayTest, Next)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        values->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSObject> array(JSArray::CreateArrayFromList(thread, values));
    JSHandle<JSTaggedValue> iter(factory->NewJSArrayIterator(array, IterationKind::KEY));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    for (int i = 0; i < 5; i++) {
        ecmaRuntimeCallInfo->SetThis(iter.GetTaggedValue());
        JSTaggedValue ret = JSArrayIterator::Next(ecmaRuntimeCallInfo.get());
        JSHandle<JSTaggedValue> result(thread, ret);
        EXPECT_EQ(JSIterator::IteratorValue(thread, result)->GetInt(), i);
    }
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(JSArrayTest, Iterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        values->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSObject> array(JSArray::CreateArrayFromList(thread, values));
    JSHandle<JSTaggedValue> key_iter(factory->NewJSArrayIterator(array, IterationKind::KEY));
    JSHandle<JSTaggedValue> value_iter(factory->NewJSArrayIterator(array, IterationKind::VALUE));
    JSHandle<JSTaggedValue> iter(factory->NewJSArrayIterator(array, IterationKind::KEY_AND_VALUE));

    for (int i = 0; i < 5; i++) {
        if (i == 2) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
            JSObject::DeleteProperty(thread, JSHandle<JSObject>(array), key);
        }
        JSHandle<JSTaggedValue> key_result(JSIterator::IteratorStep(thread, key_iter));
        JSHandle<JSTaggedValue> value_result(JSIterator::IteratorStep(thread, value_iter));
        JSHandle<JSTaggedValue> iter_result(JSIterator::IteratorStep(thread, iter));
        JSHandle<JSTaggedValue> iter_value(JSIterator::IteratorValue(thread, iter_result));
        JSHandle<JSTaggedValue> index_key(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> element_key(thread, JSTaggedValue(1));
        if (i == 2) {
            EXPECT_EQ(static_cast<int>(i), JSIterator::IteratorValue(thread, key_result)->GetInt());
            EXPECT_EQ(JSTaggedValue::Undefined(), JSIterator::IteratorValue(thread, value_result).GetTaggedValue());
            EXPECT_EQ(static_cast<int>(i), JSObject::GetProperty(thread, iter_value, index_key).GetValue()->GetInt());
            EXPECT_EQ(JSTaggedValue::Undefined(),
                      JSObject::GetProperty(thread, iter_value, element_key).GetValue().GetTaggedValue());
            continue;
        }
        EXPECT_EQ(static_cast<int>(i), JSIterator::IteratorValue(thread, key_result)->GetInt());
        EXPECT_EQ(static_cast<int>(i), JSIterator::IteratorValue(thread, value_result)->GetInt());
        EXPECT_EQ(static_cast<int>(i), JSObject::GetProperty(thread, iter_value, index_key).GetValue()->GetInt());
        EXPECT_EQ(static_cast<int>(i), JSObject::GetProperty(thread, iter_value, element_key).GetValue()->GetInt());
    }
}
}  // namespace panda::test
