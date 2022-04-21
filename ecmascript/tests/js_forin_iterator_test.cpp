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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_handle.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSForinIteratorTest : public testing::Test {
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

HWTEST_F_L0(JSForinIteratorTest, Create)
{
    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> grandfather = JSObject::ObjectCreate(thread, nullHandle);
    EXPECT_TRUE(JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(grandfather)).IsNull());

    JSHandle<JSObject> father = JSObject::ObjectCreate(thread, grandfather);

    JSHandle<JSObject> son = JSObject::ObjectCreate(thread, father);

    JSHandle<JSTaggedValue> key1(thread->GetEcmaVM()->GetFactory()->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> key2(thread->GetEcmaVM()->GetFactory()->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> key3(thread->GetEcmaVM()->GetFactory()->NewFromASCII("key3"));
    JSHandle<JSTaggedValue> key1Value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2Value(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> key3Value(thread, JSTaggedValue(3));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(grandfather), key3, key3Value);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(father), key2, key2Value);

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(son), key1, key1Value);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(son), key2, key1Value);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(son), key3, key1Value);

    JSHandle<JSForInIterator> it = thread->GetEcmaVM()->GetFactory()->NewJSForinIterator(JSHandle<JSTaggedValue>(son));
    std::pair<JSTaggedValue, bool> n1 = JSForInIterator::NextInternal(thread, it);
    EXPECT_EQ(n1.first, key1.GetTaggedValue());
    EXPECT_FALSE(n1.second);

    std::pair<JSTaggedValue, bool> n2 = JSForInIterator::NextInternal(thread, it);
    EXPECT_EQ(n2.first, key2.GetTaggedValue());
    EXPECT_FALSE(n2.second);

    std::pair<JSTaggedValue, bool> n3 = JSForInIterator::NextInternal(thread, it);
    EXPECT_EQ(n3.first, key3.GetTaggedValue());
    EXPECT_FALSE(n3.second);

    std::pair<JSTaggedValue, bool> n4 = JSForInIterator::NextInternal(thread, it);
    EXPECT_EQ(n4.first, JSTaggedValue::Undefined());
    EXPECT_TRUE(n4.second);
}
}  // namespace panda::test
