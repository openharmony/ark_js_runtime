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
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "ecmascript/ts_types/ts_loader.h"

using namespace panda::ecmascript;

namespace panda::test {
class SnapShotTest : public testing::Test {
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
        JSRuntimeOptions options;
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetEnableParalledYoungGc(false);
        static EcmaLanguageContext lcEcma;
        [[maybe_unused]] bool success = Runtime::Create(options, {&lcEcma});
        ASSERT_TRUE(success) << "Cannot create Runtime";
        instance = Runtime::GetCurrent()->GetPandaVM();
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = EcmaVM::Cast(instance)->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        delete scope;
        scope = nullptr;
        EcmaVM::Cast(instance)->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(EcmaVM::Cast(instance));
    }

    PandaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(SnapShotTest, SnapShotSerialize)
{
    auto ecmaVm = EcmaVM::Cast(instance);
    auto factory = ecmaVm->GetFactory();
    auto tsLoader = ecmaVm->GetTSLoader();
    JSHandle<EcmaString> str1 = factory->NewFromASCII("str1");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("str2");
    JSHandle<EcmaString> str3 = factory->NewFromASCII("str3");
    JSHandle<EcmaString> str4 = factory->NewFromASCII("str4");
    tsLoader->AddConstString(str1.GetTaggedValue());
    tsLoader->AddConstString(str2.GetTaggedValue());
    tsLoader->AddConstString(str3.GetTaggedValue());
    tsLoader->AddConstString(str4.GetTaggedValue());
    SnapShot snapShot(ecmaVm);
    // serialize
    CVector<JSTaggedType> constStringTable = tsLoader->GetConstStringTable();
    snapShot.MakeSnapShot(reinterpret_cast<uintptr_t>(constStringTable.data()), constStringTable.size(), "snapshot");
    // deserialize
    snapShot.SnapShotDeserialize(SnapShotType::TS_LOADER, "snapshot");
    CVector<JSTaggedType> constStringTable1 = tsLoader->GetConstStringTable();
    ASSERT_EQ(constStringTable1.size(), 8U);
    EcmaString *str11 = reinterpret_cast<EcmaString *>(constStringTable1[4]);
    EcmaString *str22 = reinterpret_cast<EcmaString *>(constStringTable1[5]);
    EcmaString *str33 = reinterpret_cast<EcmaString *>(constStringTable1[6]);
    EcmaString *str44 = reinterpret_cast<EcmaString *>(constStringTable1[7]);
    ASSERT_EQ(std::strcmp(str11->GetCString().get(), "str1"), 0);
    ASSERT_EQ(std::strcmp(str22->GetCString().get(), "str2"), 0);
    ASSERT_EQ(std::strcmp(str33->GetCString().get(), "str3"), 0);
    ASSERT_EQ(std::strcmp(str44->GetCString().get(), "str4"), 0);
}
}  // namespace panda::test
