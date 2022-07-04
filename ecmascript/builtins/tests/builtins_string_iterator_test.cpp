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

#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsStringIteratorTest : public testing::Test {
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
};

static JSTaggedValue CreateBuiltinsJSStringIterator(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<EcmaString> string = factory->NewFromUtf8(&keyCStr[0]);
    JSHandle<JSStringIterator> stringIterator = JSStringIterator::CreateStringIterator(thread, string);
    EXPECT_TRUE(*stringIterator != nullptr);

    return stringIterator.GetTaggedValue();
}

// Single char16_t Basic Multilingual plane character
HWTEST_F_L0(BuiltinsStringIteratorTest, Next_001)
{
    auto globalConst = thread->GlobalConstants();
    CString string = "没有";
    JSHandle<JSStringIterator> stringIterator =
        JSHandle<JSStringIterator>(thread, CreateBuiltinsJSStringIterator(thread, string));
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(stringIterator.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsStringIterator::Next(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 1U);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(stringIterator.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    BuiltinsStringIterator::Next(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 2U);

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(stringIterator.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result = BuiltinsStringIterator::Next(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj(thread, result);
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(thread, result), valueStr).GetValue()->IsUndefined());
}

// character with lead surrogate and trail surrogate character
HWTEST_F_L0(BuiltinsStringIteratorTest, Next_002)
{
    auto globalConst = thread->GlobalConstants();
    CString string = "没𠕇";
    JSHandle<JSStringIterator> stringIterator =
        JSHandle<JSStringIterator>(thread, CreateBuiltinsJSStringIterator(thread, string));
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(stringIterator.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsStringIterator::Next(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 1U);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(stringIterator.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    BuiltinsStringIterator::Next(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 3U);

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(stringIterator.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result = BuiltinsStringIterator::Next(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj(thread, result);
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(thread, result), valueStr).GetValue()->IsUndefined());
}
}  // namespace panda::test
