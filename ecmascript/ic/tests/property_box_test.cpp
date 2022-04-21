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

#include "ecmascript/ic/property_box.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class PropertyBoxTest : public testing::Test {
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

/**
 * @tc.name: Clear
 * @tc.desc: Creating PropertyBox object through "NewPropertyBox" function,the Property value is exist,then calling
 *           "Clear" function make the value be hole.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(PropertyBoxTest, Clear)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<PropertyBox> handlePropertyBox = factory->NewPropertyBox(handleValue);
    EXPECT_FALSE(handlePropertyBox->GetValue().IsHole());

    handlePropertyBox->Clear(thread);
    EXPECT_TRUE(handlePropertyBox->GetValue().IsHole());
    EXPECT_TRUE(handlePropertyBox->IsInvalid());
}

/**
 * @tc.name: SetValue
 * @tc.desc: Creating PropertyBox object through "NewPropertyBox" function,this object call "SetValue" function
 *           check wether the result returned through "GetValue" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(PropertyBoxTest, SetValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handlePropertyBoxVal(factory->NewPropertyBox(handleValue));

    JSHandle<PropertyBox> handlePropertyBox = JSHandle<PropertyBox>::Cast(handlePropertyBoxVal);
    handlePropertyBox->SetValue(thread, JSTaggedValue(2));
    EXPECT_EQ(handlePropertyBox->GetValue().GetInt(), 2);
    EXPECT_FALSE(handlePropertyBox->IsInvalid());
}
} // namespace panda::test