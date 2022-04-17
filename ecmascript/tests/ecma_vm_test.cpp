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
#include "ecmascript/tooling/interface/notification_manager.h"
#include "generated/base_options.h"

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
 * @tc.desc: Create EcmaVM in 2 ways, check the Options state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(EcmaVMTest, CreateEcmaVMInTwoWays)
{
    RuntimeOption options;
    options.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *ecmaVm1 = JSNApi::CreateJSVM(options);

    JSRuntimeOptions options2;
    options2.SetEnableArkTools(true);
    options2.SetEnableStubAot(true);
    options2.SetComStubFile("file1");
    options2.SetBcStubFile("file2");
    options2.SetEnableForceGC(false);
    options2.SetForceFullGC(false);
    options2.SetEnableCpuprofiler(true);
    options2.SetEnableTsAot(true);
    options2.SetArkProperties(ArkProperties::GC_STATS_PRINT);

    // // GC
    // options2.SetGcTriggerType("no-gc-for-start-up");  // A non-production gc strategy. Prohibit stw-gc 10 times.
    EcmaVM *ecmaVm2 = EcmaVM::Cast(EcmaVM::Create(options2));

    EXPECT_TRUE(ecmaVm1 != ecmaVm2);

    JSRuntimeOptions options1Out = ecmaVm1->GetJSOptions();
    JSRuntimeOptions options2Out = ecmaVm2->GetJSOptions();

    EXPECT_TRUE(&options1Out != &options2Out);

    EXPECT_TRUE(options1Out.IsEnableArkTools() != options2Out.IsEnableArkTools());
    EXPECT_TRUE(options1Out.IsEnableStubAot() != options2Out.IsEnableStubAot());
    EXPECT_TRUE(options1Out.GetComStubFile() != options2Out.GetComStubFile());
    EXPECT_TRUE(options1Out.GetBcStubFile() != options2Out.GetBcStubFile());
    EXPECT_TRUE(options1Out.IsEnableForceGC() != options2Out.IsEnableForceGC());
    EXPECT_TRUE(options1Out.IsForceFullGC() != options2Out.IsForceFullGC());
    EXPECT_TRUE(options1Out.IsEnableCpuProfiler() != options2Out.IsEnableCpuProfiler());
    EXPECT_TRUE(options1Out.EnableTSAot() != options2Out.EnableTSAot());
    EXPECT_TRUE(options1Out.GetArkProperties() != options2Out.GetArkProperties());

    auto runtime = Runtime::GetCurrent();
    PandaVM *mainVm = runtime->GetPandaVM();
    ecmaVm2->GetNotificationManager()->VmDeathEvent();
    if (mainVm != ecmaVm2) {
        EcmaVM::Destroy(ecmaVm2);
    }

    JSNApi::DestroyJSVM(ecmaVm1);
}
}  // namespace panda::ecmascript