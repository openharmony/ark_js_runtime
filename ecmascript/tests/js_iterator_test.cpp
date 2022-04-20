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
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class JSIteratorTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSIteratorTest, GetIterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> data(factory->NewTaggedArray(2));
    data->Set(thread, 0, JSTaggedValue(1));
    data->Set(thread, 1, JSTaggedValue(1));
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    EXPECT_TRUE(array->IsArray(thread));
    JSHandle<JSArrayIterator> iter(JSIterator::GetIterator(thread, array));
    EXPECT_TRUE(iter->IsJSArrayIterator());
    EXPECT_TRUE(iter->GetIteratedArray().IsArray(thread));
}

HWTEST_F_L0(JSIteratorTest, IteratorNext)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();

    JSHandle<TaggedArray> data(factory->NewTaggedArray(1));
    data->Set(thread, 0, JSTaggedValue(1));
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> result(JSIterator::IteratorNext(thread, iter));
    JSHandle<JSTaggedValue> resultValue(JSObject::GetProperty(thread, result, valueStr).GetValue());
    EXPECT_EQ(resultValue->GetInt(), 1);
}

HWTEST_F_L0(JSIteratorTest, IteratorComplete)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> data(factory->NewTaggedArray(2));
    data->Set(thread, 0, JSTaggedValue(1));
    data->Set(thread, 1, JSTaggedValue(1));
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> result1(JSIterator::IteratorNext(thread, iter));
    EXPECT_EQ(false, JSIterator::IteratorComplete(thread, result1));
    JSHandle<JSTaggedValue> result2(JSIterator::IteratorNext(thread, iter));
    EXPECT_EQ(false, JSIterator::IteratorComplete(thread, result2));
    JSHandle<JSTaggedValue> result3(JSIterator::IteratorNext(thread, iter));
    EXPECT_EQ(true, JSIterator::IteratorComplete(thread, result3));
}

HWTEST_F_L0(JSIteratorTest, IteratorValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TaggedArray> data(factory->NewTaggedArray(3));
    data->Set(thread, 0, JSTaggedValue(1));
    data->Set(thread, 1, JSTaggedValue(1));
    data->Set(thread, 2, JSTaggedValue(1));
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> result(JSIterator::IteratorNext(thread, iter));
    JSHandle<JSTaggedValue> resultValue(JSIterator::IteratorValue(thread, result));
    EXPECT_EQ(resultValue->GetInt(), 1);
}

HWTEST_F_L0(JSIteratorTest, IteratorStep)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> data(factory->NewTaggedArray(2));
    data->Set(thread, 0, JSTaggedValue(1));
    data->Set(thread, 1, JSTaggedValue(2));
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> result1(JSIterator::IteratorStep(thread, iter));
    EXPECT_EQ(JSIterator::IteratorValue(thread, result1)->GetInt(), 1);
    JSHandle<JSTaggedValue> result2(JSIterator::IteratorStep(thread, iter));
    EXPECT_EQ(JSIterator::IteratorValue(thread, result2)->GetInt(), 2);
    JSHandle<JSTaggedValue> result3(JSIterator::IteratorStep(thread, iter));
    EXPECT_EQ(result3.GetTaggedValue(), JSTaggedValue::False());
}
}  // namespace panda::test
