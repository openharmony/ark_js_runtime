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

#include <thread>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_binary_op-inl.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class ICBinaryOPTest : public testing::Test {
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
        ecmaVm = EcmaVM::Cast(instance);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    EcmaVM *ecmaVm = nullptr;
};

HWTEST_F_L0(ICBinaryOPTest, AddWithTSType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("AddTest");
    JSHandle<EcmaString> Str2 = factory->NewFromCanBeCompressString("IC");
    JSHandle<JSObject> arg4 = factory->NewEmptyJSObject();
    JSTaggedValue arg1Value(static_cast<uint32_t>(2));
    JSTaggedValue arg2Value(static_cast<uint32_t>(3));
    JSTaggedValue arg3Value(static_cast<double>(9.5561));
    JSHandle<JSTaggedValue> arg1(thread, arg1Value);
    JSHandle<JSTaggedValue> arg2(thread, arg2Value);
    JSHandle<JSTaggedValue> arg3(thread, arg3Value);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Add2Dyn(thread, arg1.GetTaggedValue(),
                                                            arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle1(thread, resInSlowPath1);
    JSTaggedValue resInICPath1 = ICBinaryOP::AddWithTSType(thread,  arg1.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle1.GetTaggedValue(), resInICPath1);

    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Add2Dyn(thread, arg1.GetTaggedValue(),
                                                            arg3.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle2(thread, resInSlowPath2);
    JSTaggedValue resInICPath2 = ICBinaryOP::AddWithTSType(thread,  arg1.GetTaggedValue(), arg3.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle2.GetTaggedValue(), resInICPath2);

    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Add2Dyn(thread, Str1.GetTaggedValue(),
                                                            Str2.GetTaggedValue());
    JSHandle<EcmaString> slowHandle3(thread, reinterpret_cast<EcmaString *>(resInSlowPath3.GetRawData()));
    JSTaggedValue resInICPath3 = ICBinaryOP::AddWithTSType(thread,  Str1.GetTaggedValue(), Str2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::STRING)));
    ASSERT_TRUE(resInICPath3.IsString());
    EXPECT_EQ(slowHandle3->Compare(reinterpret_cast<EcmaString *>(resInICPath3.GetRawData())), 0);

    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Add2Dyn(thread, JSTaggedValue::Undefined(),
                                                            arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle4(thread, resInSlowPath4);
    JSTaggedValue resInICPath4 = ICBinaryOP::AddWithTSType(thread,  JSTaggedValue::Undefined(),
                                                           arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER_GEN)));
    EXPECT_EQ(slowHandle4.GetTaggedValue(), resInICPath4);

    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Add2Dyn(thread, arg3.GetTaggedValue(),
                                                            Str1.GetTaggedValue());
    JSHandle<EcmaString> slowHandle5(thread, reinterpret_cast<EcmaString *>(resInSlowPath5.GetRawData()));
    JSTaggedValue resInICPath5 = ICBinaryOP::AddWithTSType(thread,  arg3.GetTaggedValue(),
                                                           Str1.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::STRING_GEN)));
    ASSERT_TRUE(resInICPath5.IsString());
    EXPECT_EQ(slowHandle5->Compare(reinterpret_cast<EcmaString *>(resInICPath5.GetRawData())), 0);

    JSTaggedValue resInSlowPath6 = SlowRuntimeStub::Add2Dyn(thread, Str1.GetTaggedValue(),
                                                            JSTaggedValue::Null());
    JSHandle<EcmaString> slowHandle6(thread, reinterpret_cast<EcmaString *>(resInSlowPath6.GetRawData()));
    JSTaggedValue resInICPath6 = ICBinaryOP::AddWithTSType(thread,  Str1.GetTaggedValue(), JSTaggedValue::Null(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::STRING_GEN)));
    ASSERT_TRUE(resInICPath6.IsString());
    EXPECT_EQ(slowHandle6->Compare(reinterpret_cast<EcmaString *>(resInICPath6.GetRawData())), 0);

    JSTaggedValue resInSlowPath7 = SlowRuntimeStub::Add2Dyn(thread, arg1.GetTaggedValue(),
                                                            JSTaggedValue::True());
    JSHandle<JSTaggedValue> slowHandle7(thread, resInSlowPath7);
    JSTaggedValue resInICPath7 = ICBinaryOP::AddWithTSType(thread, arg1.GetTaggedValue(), JSTaggedValue::True(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER_GEN)));
    EXPECT_EQ(slowHandle7.GetTaggedValue(), resInICPath7);

    JSTaggedValue resInSlowPath8 = SlowRuntimeStub::Add2Dyn(thread, arg4.GetTaggedValue(),
                                                            JSTaggedValue::Null());
    JSHandle<EcmaString> slowHandle8(thread, reinterpret_cast<EcmaString *>(resInSlowPath8.GetRawData()));
    JSTaggedValue resInICPath8 = ICBinaryOP::AddWithTSType(thread,  arg4.GetTaggedValue(), JSTaggedValue::Null(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    ASSERT_TRUE(resInICPath8.IsString());
    EXPECT_EQ(slowHandle8->Compare(reinterpret_cast<EcmaString *>(resInICPath8.GetRawData())), 0);
};

HWTEST_F_L0(ICBinaryOPTest, SubWithTSType)
{
    JSTaggedValue arg1Value(static_cast<uint32_t>(-2));
    JSTaggedValue arg2Value(static_cast<uint32_t>(INT32_MAX-1));
    JSTaggedValue arg3Value(static_cast<double>(9.5561));
    JSHandle<JSTaggedValue> arg1(thread, arg1Value);
    JSHandle<JSTaggedValue> arg2(thread, arg2Value);
    JSHandle<JSTaggedValue> arg3(thread, arg3Value);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Sub2Dyn(thread, arg1.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle1(thread, resInSlowPath1);
    JSTaggedValue resInICPath1 = ICBinaryOP::SubWithTSType(thread,  arg1.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle1.GetTaggedValue(), resInICPath1);

    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Sub2Dyn(thread, arg2.GetTaggedValue(), arg3.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle2(thread, resInSlowPath2);
    JSTaggedValue resInICPath2 = ICBinaryOP::SubWithTSType(thread,  arg2.GetTaggedValue(), arg3.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle2.GetTaggedValue(), resInICPath2);

    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Sub2Dyn(thread, arg1.GetTaggedValue(), JSTaggedValue::True());
    JSHandle<JSTaggedValue> slowHandle3(thread, resInSlowPath3);
    JSTaggedValue resInICPath3 = ICBinaryOP::SubWithTSType(thread,  arg1.GetTaggedValue(), JSTaggedValue::True(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle3.GetTaggedValue(), resInICPath3);
};

HWTEST_F_L0(ICBinaryOPTest, MulWithTSType)
{
    JSTaggedValue arg1Value(static_cast<double>(28.5));
    JSTaggedValue arg2Value(static_cast<uint16_t>(354));
    JSTaggedValue arg3Value(static_cast<double>(9.5561));
    JSHandle<JSTaggedValue> arg1(thread, arg1Value);
    JSHandle<JSTaggedValue> arg2(thread, arg2Value);
    JSHandle<JSTaggedValue> arg3(thread, arg3Value);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Mul2Dyn(thread, arg1.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle1(thread, resInSlowPath1);
    JSTaggedValue resInICPath1 = ICBinaryOP::MulWithTSType(thread,  arg1.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle1.GetTaggedValue(), resInICPath1);

    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Mul2Dyn(thread, arg2.GetTaggedValue(), arg3.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle2(thread, resInSlowPath2);
    JSTaggedValue resInICPath2 = ICBinaryOP::MulWithTSType(thread,  arg2.GetTaggedValue(), arg3.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle2.GetTaggedValue(), resInICPath2);

    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Mul2Dyn(thread, arg1.GetTaggedValue(), JSTaggedValue::True());
    JSHandle<JSTaggedValue> slowHandle3(thread, resInSlowPath3);
    JSTaggedValue resInICPath3 = ICBinaryOP::MulWithTSType(thread,  arg1.GetTaggedValue(), JSTaggedValue::True(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle3.GetTaggedValue(), resInICPath3);

};

HWTEST_F_L0(ICBinaryOPTest, DivWithTSType)
{
    JSTaggedValue arg1Value(static_cast<uint32_t>(2));
    JSTaggedValue arg2Value(static_cast<uint32_t>(39884));
    JSTaggedValue arg3Value(static_cast<uint32_t>(0));
    JSTaggedValue arg4Value(static_cast<double>(934.5561));
    JSHandle<JSTaggedValue> arg1(thread, arg1Value);
    JSHandle<JSTaggedValue> arg2(thread, arg2Value);
    JSHandle<JSTaggedValue> arg3(thread, arg3Value);
    JSHandle<JSTaggedValue> arg4(thread, arg4Value);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Div2Dyn(thread, arg3.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle1(thread, resInSlowPath1);
    JSTaggedValue resInICPath1 = ICBinaryOP::DivWithTSType(thread,  arg3.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle1.GetTaggedValue(), resInICPath1);

    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Div2Dyn(thread, arg2.GetTaggedValue(), arg3.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle2(thread, resInSlowPath2);
    JSTaggedValue resInICPath2 = ICBinaryOP::DivWithTSType(thread,  arg2.GetTaggedValue(), arg3.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle2.GetTaggedValue(), resInICPath2);

    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Div2Dyn(thread, arg1.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle3(thread, resInSlowPath3);
    JSTaggedValue resInICPath3 = ICBinaryOP::DivWithTSType(thread,  arg1.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle3.GetTaggedValue(), resInICPath3);

    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Div2Dyn(thread, arg2.GetTaggedValue(), JSTaggedValue::True());
    JSHandle<JSTaggedValue> slowHandle4(thread, resInSlowPath4);
    JSTaggedValue resInICPath4 = ICBinaryOP::DivWithTSType(thread,  arg2.GetTaggedValue(), JSTaggedValue::True(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle4.GetTaggedValue(), resInICPath4);

    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Div2Dyn(thread, arg4.GetTaggedValue(), JSTaggedValue::False());
    JSHandle<JSTaggedValue> slowHandle5(thread, resInSlowPath5);
    JSTaggedValue resInICPath5 = ICBinaryOP::DivWithTSType(thread,  arg4.GetTaggedValue(),
                                                           JSTaggedValue::False(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle5.GetTaggedValue(), resInICPath5);
};

HWTEST_F_L0(ICBinaryOPTest, ModWithTSType)
{
    JSTaggedValue arg1Value(static_cast<uint32_t>(2));
    JSTaggedValue arg2Value(static_cast<uint32_t>(39884));
    JSTaggedValue arg3Value(static_cast<uint32_t>(0));
    JSTaggedValue arg4Value(static_cast<double>(934.5561));
    JSHandle<JSTaggedValue> arg1(thread, arg1Value);
    JSHandle<JSTaggedValue> arg2(thread, arg2Value);
    JSHandle<JSTaggedValue> arg3(thread, arg3Value);
    JSHandle<JSTaggedValue> arg4(thread, arg4Value);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::Mod2Dyn(thread, arg3.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle1(thread, resInSlowPath1);
    JSTaggedValue resInICPath1 = ICBinaryOP::ModWithTSType(thread,  arg3.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle1.GetTaggedValue(), resInICPath1);

    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::Mod2Dyn(thread, arg2.GetTaggedValue(), arg3.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle2(thread, resInSlowPath2);
    JSTaggedValue resInICPath2 = ICBinaryOP::ModWithTSType(thread,  arg2.GetTaggedValue(), arg3.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle2.GetTaggedValue(), resInICPath2);

    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::Mod2Dyn(thread, arg1.GetTaggedValue(), arg2.GetTaggedValue());
    JSHandle<JSTaggedValue> slowHandle3(thread, resInSlowPath3);
    JSTaggedValue resInICPath3 = ICBinaryOP::ModWithTSType(thread,  arg1.GetTaggedValue(), arg2.GetTaggedValue(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(slowHandle3.GetTaggedValue(), resInICPath3);


    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::Mod2Dyn(thread, arg2.GetTaggedValue(), JSTaggedValue::True());
    JSHandle<JSTaggedValue> slowHandle4(thread, resInSlowPath4);
    JSTaggedValue resInICPath4 = ICBinaryOP::ModWithTSType(thread,  arg2.GetTaggedValue(), JSTaggedValue::True(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle4.GetTaggedValue(), resInICPath4);


    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::Mod2Dyn(thread, arg4.GetTaggedValue(), JSTaggedValue::False());
    JSHandle<JSTaggedValue> slowHandle5(thread, resInSlowPath5);
    JSTaggedValue resInICPath5 = ICBinaryOP::ModWithTSType(thread,  arg4.GetTaggedValue(),
                                                           JSTaggedValue::False(),
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(slowHandle5.GetTaggedValue(), resInICPath5);
};

HWTEST_F_L0(ICBinaryOPTest, ShlWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg3(static_cast<uint32_t>(5));

    JSTaggedValue resInICPath1 = ICBinaryOP::ShlWithTSType(thread,  arg1, arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(9152), resInICPath1);

    JSTaggedValue resInICPath2 = ICBinaryOP::ShlWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(7200), resInICPath2);
};

HWTEST_F_L0(ICBinaryOPTest, ShrWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg3(static_cast<uint32_t>(5));

    JSTaggedValue resInICPath1 = ICBinaryOP::ShrWithTSType(thread,  arg1, arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(8), resInICPath1);

    JSTaggedValue resInICPath2 = ICBinaryOP::ShrWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(7), resInICPath2);
};

HWTEST_F_L0(ICBinaryOPTest, AshrWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg2(static_cast<uint32_t>(-286));
    JSTaggedValue arg3(static_cast<uint32_t>(5));

    JSTaggedValue resInICPath1 = ICBinaryOP::AshrWithTSType(thread,  arg1, arg3,
                                                            JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(8), resInICPath1);

    JSTaggedValue resInICPath3 = ICBinaryOP::AshrWithTSType(thread,  arg2, arg3,
                                                            JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(134217719), resInICPath3);

    JSTaggedValue resInICPath2 = ICBinaryOP::AshrWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                            JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(7), resInICPath2);

};
HWTEST_F_L0(ICBinaryOPTest, AndWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg3(static_cast<uint32_t>(541));

    JSTaggedValue resInICPath1 = ICBinaryOP::AndWithTSType(thread,  arg1, arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(28), resInICPath1);

    JSTaggedValue resInICPath2 = ICBinaryOP::AndWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(1), resInICPath2);
};
HWTEST_F_L0(ICBinaryOPTest, OrWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg3(static_cast<uint32_t>(523));

    JSTaggedValue resInICPath1 = ICBinaryOP::OrWithTSType(thread,  arg1, arg3,
                                                          JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(799), resInICPath1);

    JSTaggedValue resInICPath2 = ICBinaryOP::OrWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                          JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(747), resInICPath2);
};
HWTEST_F_L0(ICBinaryOPTest, XorWithTSType)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> Str1 = factory->NewFromCanBeCompressString("1225");
    JSTaggedValue arg1(static_cast<uint32_t>(286));
    JSTaggedValue arg3(static_cast<uint32_t>(523));

    JSTaggedValue resInICPath1 = ICBinaryOP::XorWithTSType(thread,  arg1, arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::NUMBER)));
    EXPECT_EQ(JSTaggedValue(789), resInICPath1);

    JSTaggedValue resInICPath2 = ICBinaryOP::XorWithTSType(thread,  Str1.GetTaggedValue(), arg3,
                                                           JSTaggedValue(static_cast<int>(BinaryType::GENERIC)));
    EXPECT_EQ(JSTaggedValue(1730), resInICPath2);
};
}  // namespace panda::test