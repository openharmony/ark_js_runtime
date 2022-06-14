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
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "ecmascript/ts_types/ts_loader.h"

using namespace panda::ecmascript;

namespace panda::test {
class SnapshotTest : public testing::Test {
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

HWTEST_F_L0(SnapshotTest, SerializeConstStringTable)
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
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    CVector<JSTaggedType> constStringTable = tsLoader->GetConstStringTable();
    snapshotSerialize.Serialize(reinterpret_cast<uintptr_t>(constStringTable.data()),
                                constStringTable.size(), fileName);
    tsLoader->ClearConstStringTable();
    Snapshot snapshotDeserialize(ecmaVm);
    // deserialize
    snapshotDeserialize.Deserialize(SnapshotType::TS_LOADER, fileName);
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
    ecmaVm->GetHeap()->GetSnapshotSpace()->ReclaimRegions();
    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeConstPool)
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
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool, nullptr, fileName);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetNonMovableSpace()->GetFirstRegion();
    auto constpool1 = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_EQ(constpool->GetClass()->SizeFromJSHClass(*constpool),
              constpool1->GetClass()->SizeFromJSHClass(constpool1));
    EXPECT_TRUE(constpool1->Get(0).IsJSFunction());
    EXPECT_TRUE(constpool1->Get(1).IsJSFunction());
    EXPECT_TRUE(constpool1->Get(3).IsJSFunction());
    EcmaString *str11 = reinterpret_cast<EcmaString *>(constpool1->Get(2).GetTaggedObject());
    EcmaString *str22 = reinterpret_cast<EcmaString *>(constpool1->Get(4).GetTaggedObject());
    EcmaString *str33 = reinterpret_cast<EcmaString *>(constpool1->Get(5).GetTaggedObject());
    EXPECT_EQ(std::strcmp(str11->GetCString().get(), "str11"), 0);
    EXPECT_EQ(std::strcmp(str22->GetCString().get(), "str22"), 0);
    EXPECT_EQ(std::strcmp(str33->GetCString().get(), "str11"), 0);
    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeDifferentSpace)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(400);
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        constpool->Set(thread, i, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        constpool->Set(thread, i + 100, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<MachineCode> codeObj = factory->NewMachineCodeObject(0, nullptr);
        constpool->Set(thread, i + 200, codeObj.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<ConstantPool> constpool1 = factory->NewConstantPool(10);
        constpool->Set(thread, i + 300, constpool1.GetTaggedValue());
    }

    CString fileName = "snapshot";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool, nullptr, fileName);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetNonMovableSpace()->GetFirstRegion();
    auto constpool1 = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_EQ(constpool->GetClass()->SizeFromJSHClass(*constpool),
              constpool1->GetClass()->SizeFromJSHClass(constpool1));
    EXPECT_TRUE(constpool1->Get(0).IsTaggedArray());
    EXPECT_TRUE(constpool1->Get(100).IsTaggedArray());
    EXPECT_TRUE(constpool1->Get(200).IsMachineCodeObject());
    EXPECT_TRUE(constpool1->Get(300).IsTaggedArray());
    auto obj1 = constpool1->Get(0).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj1)->InOldSpace());
    auto obj2 = constpool1->Get(100).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj2)->InOldSpace());
    auto obj3 = constpool1->Get(200).GetTaggedObject();
    auto region = Region::ObjectAddressToRange(obj3);
    EXPECT_TRUE(region->InMachineCodeSpace());
    auto region1 = Region::ObjectAddressToRange(constpool1);
    EXPECT_TRUE(region1->InNonMovableSpace());

    std::remove(fileName.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeMultiFile)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<ConstantPool> constpool1 = factory->NewConstantPool(400);
    JSHandle<ConstantPool> constpool2 = factory->NewConstantPool(400);
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        constpool1->Set(thread, i, array.GetTaggedValue());
        constpool2->Set(thread, i, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<TaggedArray> array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
        constpool1->Set(thread, i + 100, array.GetTaggedValue());
        constpool2->Set(thread, i + 100, array.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<MachineCode> codeObj = factory->NewMachineCodeObject(0, nullptr);
        constpool1->Set(thread, i + 200, codeObj.GetTaggedValue());
        constpool2->Set(thread, i + 200, codeObj.GetTaggedValue());
    }
    for (int i = 0; i < 100; i++) {
        JSHandle<ConstantPool> constpool3 = factory->NewConstantPool(10);
        constpool1->Set(thread, i + 300, constpool3.GetTaggedValue());
        constpool2->Set(thread, i + 300, constpool3.GetTaggedValue());
    }

    CString fileName1 = "snapshot1";
    CString fileName2 = "snapshot2";
    Snapshot snapshotSerialize(ecmaVm);
    // serialize
    snapshotSerialize.Serialize(*constpool1, nullptr, fileName1);
    snapshotSerialize.Serialize(*constpool2, nullptr, fileName2);
    // deserialize
    Snapshot snapshotDeserialize(ecmaVm);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName1);
    snapshotDeserialize.Deserialize(SnapshotType::VM_ROOT, fileName2);

    auto beginRegion = const_cast<Heap *>(ecmaVm->GetHeap())->GetNonMovableSpace()->GetFirstRegion();
    auto constpool = reinterpret_cast<ConstantPool *>(beginRegion->GetBegin());
    EXPECT_TRUE(constpool->Get(0).IsTaggedArray());
    EXPECT_TRUE(constpool->Get(100).IsTaggedArray());
    EXPECT_TRUE(constpool->Get(200).IsMachineCodeObject());
    auto obj1 = constpool->Get(0).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj1)->InOldSpace());
    auto obj2 = constpool->Get(100).GetTaggedObject();
    EXPECT_TRUE(Region::ObjectAddressToRange(obj2)->InOldSpace());
    auto obj3 = constpool->Get(200).GetTaggedObject();
    auto region = Region::ObjectAddressToRange(obj3);
    EXPECT_TRUE(region->InMachineCodeSpace());
    auto region1 = Region::ObjectAddressToRange(constpool);
    EXPECT_TRUE(region1->InNonMovableSpace());

    std::remove(fileName1.c_str());
    std::remove(fileName2.c_str());
}

HWTEST_F_L0(SnapshotTest, SerializeBuiltins)
{
    // generate builtins.snapshot file
    JSRuntimeOptions options1;
    options1.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_SERIALIZE);
    EcmaVM *ecmaVm1 = EcmaVM::Create(options1);
    EcmaVM::Destroy(ecmaVm1);

    // create EcmaVM use builtins deserialzie
    JSRuntimeOptions options2;
    options2.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE);
    EcmaVM *ecmaVm2 = EcmaVM::Create(options2);
    EXPECT_TRUE(ecmaVm2->GetGlobalEnv()->GetClass()->GetObjectType() == JSType::GLOBAL_ENV);
    auto globalConst = const_cast<GlobalEnvConstants *>(ecmaVm2->GetJSThread()->GlobalConstants());
    size_t hclassEndIndex = static_cast<size_t>(ConstantIndex::UNDEFINED_INDEX);
    size_t hclassIndex = 0;
    globalConst->VisitRangeSlot([&hclassIndex, &hclassEndIndex]([[maybe_unused]]Root type,
                                                                ObjectSlot start, ObjectSlot end) {
        while (start < end) {
            JSTaggedValue object(start.GetTaggedType());
            start++;
            if (hclassIndex < hclassEndIndex) {
                EXPECT_TRUE(object.IsJSHClass());
            }
            hclassIndex++;
        }
    });
    EcmaVM::Destroy(ecmaVm2);

    CString fileName = "builtins.snapshot";
    std::remove(fileName.c_str());
}
}  // namespace panda::test
