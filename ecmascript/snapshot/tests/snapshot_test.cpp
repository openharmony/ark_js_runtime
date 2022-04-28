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
#include "ecmascript/jspandafile/program_object.h"
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
        ecmaVm = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm != nullptr) << "Cannot create EcmaVM";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    EcmaVM *ecmaVm {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(SnapShotTest, SerializeConstStringTable)
{
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
    CString fileName = "snapshot";
    SnapShot snapshotSerialize(ecmaVm);
    // serialize
    CVector<JSTaggedType> constStringTable = tsLoader->GetConstStringTable();
    snapshotSerialize.Serialize(reinterpret_cast<uintptr_t>(constStringTable.data()),
                                constStringTable.size(), fileName);
    tsLoader->ClearConstStringTable();
    SnapShot snapshotDeserialize(ecmaVm);
    // deserialize
    snapshotDeserialize.Deserialize(SnapShotType::TS_LOADER, fileName);
    CVector<JSTaggedType> constStringTable1 = tsLoader->GetConstStringTable();
    ASSERT_EQ(constStringTable1.size(), 4U);
    EcmaString *str11 = reinterpret_cast<EcmaString *>(constStringTable1[0]);
    EcmaString *str22 = reinterpret_cast<EcmaString *>(constStringTable1[1]);
    EcmaString *str33 = reinterpret_cast<EcmaString *>(constStringTable1[2]);
    EcmaString *str44 = reinterpret_cast<EcmaString *>(constStringTable1[3]);
    ASSERT_EQ(std::strcmp(str11->GetCString().get(), "str1"), 0);
    ASSERT_EQ(std::strcmp(str22->GetCString().get(), "str2"), 0);
    ASSERT_EQ(std::strcmp(str33->GetCString().get(), "str3"), 0);
    ASSERT_EQ(std::strcmp(str44->GetCString().get(), "str4"), 0);
    ecmaVm->GetHeap()->GetSnapShotSpace()->ReclaimRegions();
    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapShotTest, SerializeConstPool)
{
    auto factory = ecmaVm->GetFactory();
    auto env = ecmaVm->GetGlobalEnv();

    JSHandle<ConstantPool> constpool = factory->NewConstantPool(6);
    JSHandle<JSFunction> funcFunc(env->GetFunctionFunction());
    JSHandle<JSFunction> dateFunc(env->GetDateFunction());
    JSHandle<JSFunction> numberFunc(env->GetNumberFunction());
    JSHandle<EcmaString> str1 = factory->NewFromASCII("str11");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("str22");
    constpool->Set(thread, 0, funcFunc.GetTaggedValue());
    constpool->Set(thread, 1, dateFunc.GetTaggedValue());
    constpool->Set(thread, 2, str1.GetTaggedValue());
    constpool->Set(thread, 3, numberFunc.GetTaggedValue());
    constpool->Set(thread, 4, str2.GetTaggedValue());
    constpool->Set(thread, 5, str1.GetTaggedValue());

    CString fileName = "snapshot";
    SnapShot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool, nullptr, fileName);
    // deserialize
    SnapShot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapShotType::VM_ROOT, fileName);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetSnapShotSpace()->GetFirstRegion();
    auto constpool1 = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_EQ(constpool->GetClass()->SizeFromJSHClass(*constpool),
              constpool1->GetClass()->SizeFromJSHClass(constpool1));
    EXPECT_TRUE(constpool1->Get(0).IsJSFunction());
    EXPECT_TRUE(constpool1->Get(1).IsJSFunction());
    EXPECT_TRUE(constpool1->Get(3).IsJSFunction());
    EXPECT_EQ(constpool1->Get(0).GetRawData(), constpool1->Get(0).GetRawData());
    EcmaString *str11 = reinterpret_cast<EcmaString *>(constpool1->Get(2).GetTaggedObject());
    EcmaString *str22 = reinterpret_cast<EcmaString *>(constpool1->Get(4).GetTaggedObject());
    EcmaString *str33 = reinterpret_cast<EcmaString *>(constpool1->Get(5).GetTaggedObject());
    EXPECT_EQ(std::strcmp(str11->GetCString().get(), "str11"), 0);
    EXPECT_EQ(std::strcmp(str22->GetCString().get(), "str22"), 0);
    EXPECT_EQ(std::strcmp(str33->GetCString().get(), "str11"), 0);
    std::remove(fileName.c_str());
}
}  // namespace panda::test
