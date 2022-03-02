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
#include "generated/base_options.h"
#include "include/runtime_notification.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaVMTest : public testing::Test {
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
    }

    void TearDown() override
    {
    }
};

/*
 * @tc.name: SetCompressedStringsEnabled
 * @tc.desc: Create and destroy 2 EcmaVM from JSNApi,Check the Options state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateAndDestoryEcmaVMFromJSNApi)
{
    RuntimeOption options1, options2;
    options1.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    options2.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EXPECT_TRUE(&options2 != &options1);

    EcmaVM *ecmaVm1 = JSNApi::CreateJSVM(options1);
    ASSERT_TRUE(ecmaVm1 != nullptr) << "Cannot create Runtime 1";

    EcmaVM *ecmaVm2 = JSNApi::CreateJSVM(options2);
    ASSERT_TRUE(ecmaVm2 != nullptr) << "Cannot create Runtime 2";

    EXPECT_TRUE(&ecmaVm1->GetJSOptions() != &ecmaVm2->GetJSOptions());

    JSNApi::DestroyJSVM(ecmaVm1);
    JSNApi::DestroyJSVM(ecmaVm2);
}

/*
 * @tc.name: SetCompressedStringsEnabled
 * @tc.desc: 
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateEcmaVMWithTwoWays)
{
    JSRuntimeOptions options1;
    options1.SetEnableTsAot(true);

    options1.SetRuntimeType("ecmascript");
    // GC
    options1.SetGcType("gen-gc");
    options1.SetRunGcInPlace(true);
    options1.SetArkProperties(ArkProperties::OPTIONAL_LOG);
    // Mem
    options1.SetHeapSizeLimit(panda::DEFAULT_GC_POOL_SIZE);
    options1.SetInternalAllocatorType("malloc");
    // Boot
    options1.SetShouldLoadBootPandaFiles(false);
    options1.SetShouldInitializeIntrinsics(false);
    options1.SetBootClassSpaces({"ecmascript"});
    // Dfx
    base_options::Options baseOptions("");
    baseOptions.SetLogLevel("info");
    arg_list_t logComponents;
    logComponents.emplace_back("all");
    baseOptions.SetLogComponents(logComponents);
    Logger::Initialize(baseOptions);

    options1.SetEnableArkTools(true);
    static EcmaLanguageContext lcEcma;
    bool success = Runtime::Create(options1, {&lcEcma});
    EXPECT_TRUE(success);
    

    auto runtime = Runtime::GetCurrent();
    EcmaVM *ecmaVm1 = EcmaVM::Cast(runtime->GetPandaVM());

    JSRuntimeOptions options2;
    options2.SetEnableTsAot(false);


    options2.SetArkProperties(ArkProperties::GC_STATS_PRINT);
    // GC
    options2.SetGcTriggerType("no-gc-for-start-up");  // A non-production gc strategy. Prohibit stw-gc 10 times.
    EcmaVM *ecmaVm2 = EcmaVM::Cast(EcmaVM::Create(options2));

    EXPECT_TRUE(ecmaVm1 != ecmaVm2);

    JSRuntimeOptions options1Out = ecmaVm1->GetJSOptions();
    JSRuntimeOptions options2Out = ecmaVm2->GetJSOptions();

    EXPECT_TRUE(&options1Out != &options2Out);

    EXPECT_EQ(options1Out.GetArkProperties(), ArkProperties::OPTIONAL_LOG);
    EXPECT_EQ(options2Out.GetArkProperties(), ArkProperties::GC_STATS_PRINT);

    // EXPECT_TRUE(options1Out.IsEnableTsAot());
    // EXPECT_TRUE(!options2Out.IsEnableTsAot());

    PandaVM *mainVm = runtime->GetPandaVM();
    ecmaVm1->GetNotificationManager()->VmDeathEvent();
    if (mainVm != ecmaVm1) {
        EcmaVM::Destroy(ecmaVm1);
    }

    ecmaVm2->GetNotificationManager()->VmDeathEvent();
    if (mainVm != ecmaVm2) {
        EcmaVM::Destroy(ecmaVm2);
    }

    Runtime::Destroy();

}

/*
 * @tc.name: SetCompressedStringsEnabled
 * @tc.desc: Create and destroy an EcmaVM with Scope,Check the Options state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateAndDestroyEcmaVMWithScope)
{
    PandaVM *instance = nullptr;
    ecmascript::EcmaHandleScope *scope = nullptr;
    JSThread *thread = nullptr;

    TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    EXPECT_TRUE(instance != nullptr);
    EXPECT_TRUE(thread != nullptr);
    EXPECT_TRUE(scope != nullptr);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    EXPECT_TRUE(ecmaVm != nullptr);
    EXPECT_TRUE(&ecmaVm->GetJSOptions() != &JSRuntimeOptions::GetRuntimeOptions());
    TestHelper::DestroyEcmaVMWithScope(instance, scope);
}
}  // namespace panda::ecmascript