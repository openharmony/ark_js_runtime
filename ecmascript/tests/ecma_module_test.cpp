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

#include "ecmascript/ecma_module.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaModuleTest : public testing::Test {
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
    PandaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

EcmaModule *EcmaModuleCreate(JSThread *thread)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaModule> handleEcmaModule = objectFactory->NewEmptyEcmaModule();
    return *handleEcmaModule;
}

/*
 * Feature: EcmaModule
 * Function: AddItem
 * SubFunction: GetItem
 * FunctionPoints: Add Item To EcmaModule
 * CaseDescription: Add an item for a EcmaModule that has called "SetNameDictionary" function in the HWTEST_F_L0, and
 *                  check its value through 'GetItem' function.
 */
HWTEST_F_L0(EcmaModuleTest, AddItem_001)
{
    int numOfElementsDict = 4;
    CString cStrItemName = "key1";
    int intItemValue = 1;
    JSHandle<EcmaModule> handleEcmaModule(thread, EcmaModuleCreate(thread));
    JSHandle<NameDictionary> handleNameDict(thread, NameDictionary::Create(thread, numOfElementsDict));
    JSHandle<JSTaggedValue> handleTagValItemName(
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(cStrItemName));
    JSHandle<JSTaggedValue> handleTagValItemValue(thread, JSTaggedValue(intItemValue));

    handleEcmaModule->SetNameDictionary(thread, handleNameDict); // Call SetNameDictionary in HWTEST_F_L0
    EcmaModule::AddItem(thread, handleEcmaModule, handleTagValItemName, handleTagValItemValue);
    EXPECT_EQ(handleEcmaModule->GetItem(thread, handleTagValItemName)->GetNumber(), intItemValue);
}

/*
 * Feature: EcmaModule
 * Function: AddItem
 * SubFunction: GetItem
 * FunctionPoints: Add Item To EcmaModule
 * CaseDescription: Add an item for a EcmaModule that has not called "SetNameDictionary" function in the HWTEST_F_L0,
 *                  and check its value through 'GetItem' function.
 */
HWTEST_F_L0(EcmaModuleTest, AddItem_002)
{
    CString cStrItemName = "cStrItemName";
    int intItemValue = 1;
    JSHandle<EcmaModule> handleEcmaModule(thread, EcmaModuleCreate(thread));
    JSHandle<JSTaggedValue> handleTagValItemName(
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(cStrItemName));
    JSHandle<JSTaggedValue> handleTagValItemValue(thread, JSTaggedValue(intItemValue));

    // This EcmaModule calls 'SetNameDictionary' function through 'AddItem' function of "ecma_module.cpp".
    EcmaModule::AddItem(thread, handleEcmaModule, handleTagValItemName, handleTagValItemValue);
    EXPECT_EQ(handleEcmaModule->GetItem(thread, handleTagValItemName)->GetNumber(), intItemValue);
}

/*
 * Feature: EcmaModule
 * Function: RemoveItem
 * SubFunction: AddItem/GetItem
 * FunctionPoints: Remove Item From EcmaModule
 * CaseDescription: Add an item for an empty EcmaModule through 'AddItem' function, then remove the item from the
 *                  EcmaModule through 'RemoveItem' function, finally check whether the item obtained from the
 *                  EcmaModule through 'GetItem' function is undefined.
 */
HWTEST_F_L0(EcmaModuleTest, RemoveItem)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();

    CString cStrItemName = "cStrItemName";
    int intItemValue = 1;
    JSHandle<EcmaModule> handleEcmaModule(thread, EcmaModuleCreate(thread));
    JSHandle<JSTaggedValue> handleTagValItemName(objFactory->NewFromCanBeCompressString(cStrItemName));
    JSHandle<JSTaggedValue> handleTagValItemValue(thread, JSTaggedValue(intItemValue));

    EcmaModule::AddItem(thread, handleEcmaModule, handleTagValItemName, handleTagValItemValue);
    EcmaModule::RemoveItem(thread, handleEcmaModule, handleTagValItemName);
    EXPECT_TRUE(handleEcmaModule->GetItem(thread, handleTagValItemName)->IsUndefined());
}

/*
 * Feature: EcmaModule
 * Function: SetNameDictionary
 * SubFunction: NameDictionary::Put/GetNameDictionary
 * FunctionPoints: Set NameDictionary For EcmaModule
 * CaseDescription: Create a source key, a source value, a source NameDictionary and a target EcmaModule, change the
 *                  NameDictionary through 'NameDictionary::Put' function, set the changed source NameDictionary as
 *                  this target EcmaModule's NameDictionary through 'SetNameDictionary' function, check whether the
 *                  result returned through 'GetNameDictionary' function from the target EcmaModule are within
 *                   expectations.
 */
HWTEST_F_L0(EcmaModuleTest, SetNameDictionary)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();

    int numOfElementsDict = 4;
    JSHandle<NameDictionary> handleNameDict(thread, NameDictionary::Create(thread, numOfElementsDict));
    JSHandle<JSTaggedValue> handleObjFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction();
    CString keyArray1 = "hello1";
    JSHandle<EcmaString> stringKey1 = objFactory->NewFromCanBeCompressString(keyArray1);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(objFactory
        ->NewJSObjectByConstructor(JSHandle<JSFunction>(handleObjFunc), handleObjFunc));
    JSHandle<NameDictionary> handleNameDictionaryFrom(thread,
        NameDictionary::Put(thread, handleNameDict, key1, value1, PropertyAttributes::Default()));
    JSHandle<EcmaModule> handleEcmaModule(thread, EcmaModuleCreate(thread));

    handleEcmaModule->SetNameDictionary(thread, handleNameDictionaryFrom);
    JSHandle<NameDictionary> handleNameDictionaryTo(thread,
        NameDictionary::Cast(handleEcmaModule->GetNameDictionary().GetTaggedObject()));
    EXPECT_EQ(handleNameDictionaryTo->EntriesCount(), 1);
    int entry1 = handleNameDictionaryTo->FindEntry(key1.GetTaggedValue());
    EXPECT_TRUE(key1.GetTaggedValue() == handleNameDictionaryTo->GetKey(entry1));
    EXPECT_TRUE(value1.GetTaggedValue() == handleNameDictionaryTo->GetValue(entry1));
}

/*
 * Feature: ModuleManager
 * Function: AddModule
 * SubFunction: AddItem/GetModule/GetItem
 * FunctionPoints: Add EcmaModule To ModuleManager
 * CaseDescription: Create 2 source EcmaModules that both add an item, create a ModuleManager that add the 2 source
 *                  EcmaModule, check whether the items of EcmaModules obtained from the ModuleManager through
 *                  'GetModule' function and 'GetItem' function are within expectations.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_AddModule)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    int numOfElementsDict1 = 4;
    int numOfElementsDict2 = 4;
    CString cStrItemName1 = "cStrItemName1";
    CString cStrItemName2 = "cStrItemName2";
    int intItemValue1 = 1;
    int intItemValue2 = 2;
    JSHandle<NameDictionary> handleNameDict1(thread, NameDictionary::Create(thread, numOfElementsDict1));
    JSHandle<NameDictionary> handleNameDict2(thread, NameDictionary::Create(thread, numOfElementsDict2));
    JSHandle<JSTaggedValue> handleItemName1(
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(cStrItemName1));
    JSHandle<JSTaggedValue> handleItemName2(
        thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(cStrItemName2));
    JSHandle<JSTaggedValue> handleItemValue1(thread, JSTaggedValue(intItemValue1));
    JSHandle<JSTaggedValue> handleItemValue2(thread, JSTaggedValue(intItemValue2));
    JSHandle<EcmaModule> handleEcmaModuleAddFrom1(thread, EcmaModuleCreate(thread));
    JSHandle<EcmaModule> handleEcmaModuleAddFrom2(thread, EcmaModuleCreate(thread));
    handleEcmaModuleAddFrom1->SetNameDictionary(thread, handleNameDict1);
    handleEcmaModuleAddFrom2->SetNameDictionary(thread, handleNameDict2);

    EcmaModule::AddItem(thread, handleEcmaModuleAddFrom1, handleItemName1, handleItemValue1);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleAddFrom1(thread, handleEcmaModuleAddFrom1.GetTaggedValue());
    std::string stdStrNameEcmaModuleAdd1 = "NameEcmaModule1";
    JSHandle<JSTaggedValue> handleTagValNameEcmaModuleAdd1(objFactory->NewFromStdString(stdStrNameEcmaModuleAdd1));
    EcmaModule::AddItem(thread, handleEcmaModuleAddFrom2, handleItemName2, handleItemValue2);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleAddFrom2(thread, handleEcmaModuleAddFrom2.GetTaggedValue());
    std::string stdStrNameEcmaModuleAdd2 = "NameEcmaModule2";
    JSHandle<JSTaggedValue> handleTagValNameEcmaModuleAdd2(objFactory->NewFromStdString(stdStrNameEcmaModuleAdd2));

    moduleManager->AddModule(handleTagValNameEcmaModuleAdd1, handleTagValEcmaModuleAddFrom1);
    moduleManager->AddModule(handleTagValNameEcmaModuleAdd2, handleTagValEcmaModuleAddFrom2);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleGet1 = moduleManager->GetModule(thread,
        handleTagValNameEcmaModuleAdd1);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleGet2 = moduleManager->GetModule(thread,
        handleTagValNameEcmaModuleAdd2);
    EXPECT_EQ(JSHandle<EcmaModule>::Cast(handleTagValEcmaModuleGet1)->GetItem(thread, handleItemName1)->GetNumber(),
        intItemValue1);
    EXPECT_EQ(JSHandle<EcmaModule>::Cast(handleTagValEcmaModuleGet2)->GetItem(thread, handleItemName2)->GetNumber(),
        intItemValue2);
}

/*
 * Feature: ModuleManager
 * Function: RemoveModule
 * SubFunction: AddItem/GetModule/AddModule/GetItem
 * FunctionPoints: Remove EcmaModule From ModuleManager
 * CaseDescription: Create two source EcmaModules that add different items, create a ModuleManager that add the two
 *                  source EcmaModules, check whether the properties of the EcmaModules obtained from the ModuleManager
 *                  through 'GetModule' function are within expectations while removing EcmaModules from the
 *                  ModuleManager through 'RemoveModule' function one by one.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_RemoveModule)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    std::string stdStrNameEcmaModuleAdd1 = "NameEcmaModule1";
    std::string stdStrNameEcmaModuleAdd2 = "NameEcmaModule2";
    int intItemValue1 = 1;
    int intItemValue2 = 2;
    int numOfElementsDict1 = 4;
    int numOfElementsDict2 = 4;
    JSHandle<JSTaggedValue> handleTagValNameEcmaModuleAdd1(objFactory->NewFromStdString(stdStrNameEcmaModuleAdd1));
    JSHandle<JSTaggedValue> handleTagValNameEcmaModuleAdd2(objFactory->NewFromStdString(stdStrNameEcmaModuleAdd2));
    JSHandle<JSTaggedValue> handleTagValItemName1(objFactory->NewFromCanBeCompressString("name1"));
    JSHandle<JSTaggedValue> handleTagValItemName2(objFactory->NewFromCanBeCompressString("name2"));
    JSHandle<JSTaggedValue> handleTagValItemValue1(thread, JSTaggedValue(intItemValue1));
    JSHandle<JSTaggedValue> handleTagValItemValue2(thread, JSTaggedValue(intItemValue2));
    JSHandle<EcmaModule> handleEcmaModuleAddFrom1(thread, EcmaModuleCreate(thread));
    JSHandle<EcmaModule> handleEcmaModuleAddFrom2(thread, EcmaModuleCreate(thread));
    JSHandle<NameDictionary> handleNameDict1(thread, NameDictionary::Create(thread, numOfElementsDict1));
    JSHandle<NameDictionary> handleNameDict2(thread, NameDictionary::Create(thread, numOfElementsDict2));
    handleEcmaModuleAddFrom1->SetNameDictionary(thread, handleNameDict1);
    handleEcmaModuleAddFrom2->SetNameDictionary(thread, handleNameDict2);
    EcmaModule::AddItem(thread, handleEcmaModuleAddFrom1, handleTagValItemName1, handleTagValItemValue1);
    EcmaModule::AddItem(thread, handleEcmaModuleAddFrom2, handleTagValItemName2, handleTagValItemValue2);
    JSHandle<JSTaggedValue> handleTaggedValueEcmaModuleAddFrom1(thread, handleEcmaModuleAddFrom1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTaggedValueEcmaModuleAddFrom2(thread, handleEcmaModuleAddFrom2.GetTaggedValue());

    moduleManager->AddModule(handleTagValNameEcmaModuleAdd1, handleTaggedValueEcmaModuleAddFrom1);
    moduleManager->AddModule(handleTagValNameEcmaModuleAdd2, handleTaggedValueEcmaModuleAddFrom2);
    EXPECT_EQ(JSHandle<EcmaModule>::Cast(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd1))
        ->GetItem(thread, handleTagValItemName1)->GetNumber(), intItemValue1);
    EXPECT_EQ(JSHandle<EcmaModule>::Cast(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd2))
        ->GetItem(thread, handleTagValItemName2)->GetNumber(), intItemValue2);

    moduleManager->RemoveModule(handleTagValNameEcmaModuleAdd1);
    EXPECT_TRUE(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd1)->IsUndefined());
    EXPECT_EQ(JSHandle<EcmaModule>::Cast(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd2))
        ->GetItem(thread, handleTagValItemName2)->GetNumber(), intItemValue2);

    moduleManager->RemoveModule(handleTagValNameEcmaModuleAdd2);
    EXPECT_TRUE(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd1)->IsUndefined());
    EXPECT_TRUE(moduleManager->GetModule(thread, handleTagValNameEcmaModuleAdd2)->IsUndefined());
}

/*
 * Feature: ModuleManager
 * Function: SetCurrentExportModuleName
 * SubFunction: GetCurrentExportModuleName
 * FunctionPoints: Get Current ExportModuleName Of ModuleManager
 * CaseDescription: Create a ModuleManager, check whether the ExportModuleName obtained from the ModuleManager through
 *                  'GetCurrentExportModuleName' function is within expectations while changing the Current
 *                  ExportModuleName of the ModuleManager through 'SetCurrentExportModuleName' function.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_SetCurrentExportModuleName)
{
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    std::string_view strViewNameEcmaModule1 = "NameEcmaModule1";
    std::string_view strViewNameEcmaModule2 = "NameEcmaModule2";
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule1);
    EXPECT_STREQ(moduleManager->GetCurrentExportModuleName().c_str(), CString(strViewNameEcmaModule1).c_str());
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule2);
    EXPECT_STREQ(moduleManager->GetCurrentExportModuleName().c_str(), CString(strViewNameEcmaModule2).c_str());
}

/*
 * Feature: ModuleManager
 * Function: GetPrevExportModuleName
 * SubFunction: SetCurrentExportModuleName
 * FunctionPoints: Get Previous ExportModuleName Of ModuleManager
 * CaseDescription: Create a ModuleManager, check whether the previous ExportModuleName obtained from the ModuleManager
 *                  through 'GetPrevExportModuleName' function is within expectations while changing the Current
 *                  ExportModuleName of the ModuleManager through 'SetCurrentExportModuleName' function.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_GetPrevExportModuleName)
{
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    std::string_view strViewNameEcmaModule1 = "NameEcmaModule1";
    std::string_view strViewNameEcmaModule2 = "NameEcmaModule2";
    std::string_view strViewNameEcmaModule3 = "NameEcmaModule3";
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule1);
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule2);
    EXPECT_STREQ(moduleManager->GetPrevExportModuleName().c_str(), CString(strViewNameEcmaModule1).c_str());
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule3);
    EXPECT_STREQ(moduleManager->GetPrevExportModuleName().c_str(), CString(strViewNameEcmaModule2).c_str());
}

/*
 * Feature: ModuleManager
 * Function: RestoreCurrentExportModuleName
 * SubFunction: SetCurrentExportModuleName/GetCurrentExportModuleName
 * FunctionPoints: Restore Current ExportModuleName Of ModuleManager
 * CaseDescription: Create a ModuleManager, check whether the current ExportModuleName obtained from the ModuleManager
 *                  through 'GetCurrentExportModuleName' function is within expectations while changing the Current
 *                  ExportModuleName of the ModuleManager through 'SetCurrentExportModuleName' function and
 *                  'RestoreCurrentExportModuleName' function.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_RestoreCurrentExportModuleName)
{
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    std::string_view strViewNameEcmaModule1 = "NameEcmaModule1";
    std::string_view strViewNameEcmaModule2 = "NameEcmaModule2";
    std::string_view strViewNameEcmaModule3 = "NameEcmaModule3";
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule1);
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule2);
    moduleManager->SetCurrentExportModuleName(strViewNameEcmaModule3);
    EXPECT_STREQ(moduleManager->GetCurrentExportModuleName().c_str(), CString(strViewNameEcmaModule3).c_str());
    moduleManager->RestoreCurrentExportModuleName();
    EXPECT_STREQ(moduleManager->GetCurrentExportModuleName().c_str(), CString(strViewNameEcmaModule2).c_str());
    moduleManager->RestoreCurrentExportModuleName();
    EXPECT_STREQ(moduleManager->GetCurrentExportModuleName().c_str(), CString(strViewNameEcmaModule1).c_str());
}

/*
 * Feature: ModuleManager
 * Function: AddModuleItem
 * SubFunction: SetCurrentExportModuleName/GetModule/GetModuleItem
 * FunctionPoints: Add ModuleItem For Current EcmaModule Of ModuleManager
 * CaseDescription: Create a ModuleManager, set the current EcmaModule for the ModuleManager through
 *                  'SetCurrentExportModuleName' function, add source ModuleItems for the current EcmaModule Of the
 *                  ModuleManager, check whether the ModuleItems obtained through 'GetModuleItem' function from the
 *                  ModuleManager are within expectations.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_AddModuleItem)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    int intItemValue11 = 11;
    int intItemValue12 = 12;
    int intItemValue21 = 21;
    int intItemValue22 = 22;
    JSHandle<JSTaggedValue> handleTagValItemName11(objFactory->NewFromCanBeCompressString("cStrItemName11"));
    JSHandle<JSTaggedValue> handleTagValItemName12(objFactory->NewFromCanBeCompressString("cStrItemName12"));
    JSHandle<JSTaggedValue> handleTagValItemName21(objFactory->NewFromCanBeCompressString("cStrItemName21"));
    JSHandle<JSTaggedValue> handleTagValItemName22(objFactory->NewFromCanBeCompressString("cStrItemName22"));
    JSHandle<JSTaggedValue> handleTagValItemValue11(thread, JSTaggedValue(intItemValue11));
    JSHandle<JSTaggedValue> handleTagValItemValue12(thread, JSTaggedValue(intItemValue12));
    JSHandle<JSTaggedValue> handleTagValItemValue21(thread, JSTaggedValue(intItemValue21));
    JSHandle<JSTaggedValue> handleTagValItemValue22(thread, JSTaggedValue(intItemValue22));
    JSHandle<EcmaString> handleEcmaStrNameEcmaModule1 = objFactory->NewFromString("cStrNameEcmaModule1");
    JSHandle<EcmaString> handleEcmaStrNameEcmaModule2 = objFactory->NewFromString("cStrNameEcmaModule2");
    std::string stdStrModuleFileName1 = JSLocale::ConvertToStdString(handleEcmaStrNameEcmaModule1);
    std::string stdStrModuleFileName2 = JSLocale::ConvertToStdString(handleEcmaStrNameEcmaModule2);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleName1(handleEcmaStrNameEcmaModule1);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleName2(handleEcmaStrNameEcmaModule2);

    // Test when the module is created through 'NewEmptyEcmaModule' function called at HWTEST_F_L0.
    JSHandle<EcmaModule> handleEcmaModule1(thread, EcmaModuleCreate(thread));
    JSHandle<JSTaggedValue> handleTagValEcmaModule1(thread, handleEcmaModule1.GetTaggedValue());
    moduleManager->AddModule(handleTagValEcmaModuleName1, handleTagValEcmaModule1);
    moduleManager->SetCurrentExportModuleName(stdStrModuleFileName1);
    moduleManager->AddModuleItem(thread, handleTagValItemName11, handleTagValItemValue11);
    moduleManager->AddModuleItem(thread, handleTagValItemName12, handleTagValItemValue12);

    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule1, handleTagValItemName11)->GetNumber(),
        intItemValue11);
    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule1, handleTagValItemName12)->GetNumber(),
        intItemValue12);

    // Test when the module is created through 'NewEmptyEcmaModule' function called at "ecma_module.cpp".
    moduleManager->SetCurrentExportModuleName(stdStrModuleFileName2);
    moduleManager->AddModuleItem(thread, handleTagValItemName21, handleTagValItemValue21);
    moduleManager->AddModuleItem(thread, handleTagValItemName22, handleTagValItemValue22);

    JSHandle<JSTaggedValue> handleTagValEcmaModule2 = moduleManager->GetModule(thread, handleTagValEcmaModuleName2);
    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule1, handleTagValItemName11)->GetNumber(),
        intItemValue11);
    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule1, handleTagValItemName12)->GetNumber(),
        intItemValue12);
    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule2, handleTagValItemName21)->GetNumber(),
        intItemValue21);
    EXPECT_EQ(moduleManager->GetModuleItem(thread, handleTagValEcmaModule2, handleTagValItemName22)->GetNumber(),
        intItemValue22);
}

/*
 * Feature: ModuleManager
 * Function: CopyModule
 * SubFunction: AddItem/SetCurrentExportModuleName/GetModule/GetModuleItem
 * FunctionPoints: Copy EcmaModule To ModuleManager
 * CaseDescription: Create two source EcmaModules and one target ModuleManager, prepare the two source EcmaModules
 *                  through 'AddItem' function, check whether the the ModuleItems obtained through 'GetModuleItem' from
 *                  the target ModuleManager are within expectations while changing the target ModuleManager through
 *                  'SetCurrentExportModuleName' function and 'CopyModule' function.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleManager_CopyModule)
{
    ObjectFactory *objFactory = thread->GetEcmaVM()->GetFactory();
    ModuleManager *moduleManager = thread->GetEcmaVM()->GetModuleManager();

    int intItemValue11 = 11;
    int intItemValue12 = 12;
    int intItemValue21 = 21;
    int intItemValue22 = 22;
    std::string_view fileNameEcmaModuleCopyTo1 = "fileNameEcmaModuleCopyTo1";
    std::string_view fileNameEcmaModuleCopyTo2 = "fileNameEcmaModuleCopyTo2";
    JSHandle<JSTaggedValue> handleTagValItemName11(objFactory->NewFromCanBeCompressString("ItemName11"));
    JSHandle<JSTaggedValue> handleTagValItemName12(objFactory->NewFromCanBeCompressString("ItemName12"));
    JSHandle<JSTaggedValue> handleTagValItemName21(objFactory->NewFromCanBeCompressString("ItemName21"));
    JSHandle<JSTaggedValue> handleTagValItemName22(objFactory->NewFromCanBeCompressString("ItemName22"));
    JSHandle<JSTaggedValue> handleTagValItemValue11(thread, JSTaggedValue(intItemValue11));
    JSHandle<JSTaggedValue> handleTagValItemValue12(thread, JSTaggedValue(intItemValue12));
    JSHandle<JSTaggedValue> handleTagValItemValue21(thread, JSTaggedValue(intItemValue21));
    JSHandle<JSTaggedValue> handleTagValItemValue22(thread, JSTaggedValue(intItemValue22));
    JSHandle<EcmaModule> handleEcmaModuleCopyFrom1(thread, EcmaModuleCreate(thread));
    JSHandle<EcmaModule> handleEcmaModuleCopyFrom2(thread, EcmaModuleCreate(thread));
    JSHandle<JSTaggedValue> handleTagValEcmaModuleCopyFrom1(thread, handleEcmaModuleCopyFrom1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValEcmaModuleCopyFrom2(thread, handleEcmaModuleCopyFrom2.GetTaggedValue());
    EcmaModule::AddItem(thread, handleEcmaModuleCopyFrom1, handleTagValItemName11, handleTagValItemValue11);
    EcmaModule::AddItem(thread, handleEcmaModuleCopyFrom1, handleTagValItemName12, handleTagValItemValue12);
    EcmaModule::AddItem(thread, handleEcmaModuleCopyFrom2, handleTagValItemName21, handleTagValItemValue21);
    EcmaModule::AddItem(thread, handleEcmaModuleCopyFrom2, handleTagValItemName22, handleTagValItemValue22);

    moduleManager->SetCurrentExportModuleName(fileNameEcmaModuleCopyTo1);
    moduleManager->CopyModule(thread, handleTagValEcmaModuleCopyFrom1);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleCopyTo1 = moduleManager->GetModule(thread,
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromString(CString(fileNameEcmaModuleCopyTo1))));
    EXPECT_EQ(intItemValue11,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo1, handleTagValItemName11)->GetNumber());
    EXPECT_EQ(intItemValue12,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo1, handleTagValItemName12)->GetNumber());

    moduleManager->SetCurrentExportModuleName(fileNameEcmaModuleCopyTo2);
    moduleManager->CopyModule(thread, handleTagValEcmaModuleCopyFrom2);
    JSHandle<JSTaggedValue> handleTagValEcmaModuleCopyTo2 = moduleManager->GetModule(thread,
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromString(CString(fileNameEcmaModuleCopyTo2))));
    EXPECT_EQ(intItemValue11,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo1, handleTagValItemName11)->GetNumber());
    EXPECT_EQ(intItemValue12,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo1, handleTagValItemName12)->GetNumber());
    EXPECT_EQ(intItemValue21,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo2, handleTagValItemName21)->GetNumber());
    EXPECT_EQ(intItemValue22,
        moduleManager->GetModuleItem(thread, handleTagValEcmaModuleCopyTo2, handleTagValItemName22)->GetNumber());
}

// ModuleStack
/*
 * Feature: ModuleStack
 * Function: PushModule
 * SubFunction: GetTop
 * FunctionPoints: Push EcmaModule By Name
 * CaseDescription: Create a target ModuleStack, push a source EcmaModule name at the back of the name vector of the
 *                  target ModuleStack, check whether the returned EcmaModule name obtained through 'GetTop' function
 *                  from the target ModuleStack is the same with the source EcmaModule name.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleStack_PushModule)
{
    ModuleStack moduleStack;

    CString cStrPushNo1 = "cStrPushNo1";
    CString cStrPushNo2 = "cStrPushNo2";
    CString cStrPushNo3 = "cStrPushNo3";
    moduleStack.PushModule(cStrPushNo1);
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo1.c_str());
    moduleStack.PushModule(cStrPushNo2);
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo2.c_str());
    moduleStack.PushModule(cStrPushNo3);
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo3.c_str());
}

/*
 * Feature: ModuleStack
 * Function: PopModule
 * SubFunction: PushModule/GetTop
 * FunctionPoints: Pop EcmaModule At The Back
 * CaseDescription: Create a target ModuleStack, push some source EcmaModule names at the back of the name vector of
 *                  the target ModuleStack, check whether the returned EcmaModule name obtained through 'GetTop'
 *                  function from the target ModuleStack now and the returned EcmaModule name obtained through 'GetTop'
 *                  function from the target ModuleStack after the target ModuleStack calls 'PopModule' function are
 *                  within expectations.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleStack_PopModule)
{
    ModuleStack moduleStack;

    CString cStrPushNo1 = "cStrPushNo1";
    CString cStrPushNo2 = "cStrPushNo2";
    CString cStrPushNo3 = "cStrPushNo3";
    moduleStack.PushModule(cStrPushNo1);
    moduleStack.PushModule(cStrPushNo2);
    moduleStack.PushModule(cStrPushNo3);
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo3.c_str());
    moduleStack.PopModule();
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo2.c_str());
    moduleStack.PopModule();
    EXPECT_STREQ(moduleStack.GetTop().c_str(), cStrPushNo1.c_str());
    moduleStack.PopModule();
}

/*
 * Feature: ModuleStack
 * Function: GetPrevModule
 * SubFunction: PushModule/GetTop/PopModule
 * FunctionPoints: Get Previous Module
 * CaseDescription: Create a target ModuleStack, push some source EcmaModule names at the back of the name vector of
 *                  the target ModuleStack, check whether the returned EcmaModule name obtained through 'GetPrevModule'
 *                  function from the target ModuleStack now and the returned EcmaModule name obtained through
 *                 'GetPrevModule' function from the target ModuleStack after the target ModuleStack calls 'PushModule'
 *                  function or 'PopModule' function are within expectations.
 */
HWTEST_F_L0(EcmaModuleTest, ModuleStack_GetPrevModule)
{
    ModuleStack moduleStack;

    CString cStrPushNo1 = "cStrPushNo1";
    CString cStrPushNo2 = "cStrPushNo2";
    CString cStrPushNo3 = "cStrPushNo3";
    moduleStack.PushModule(cStrPushNo1);
    moduleStack.PushModule(cStrPushNo2);
    EXPECT_STREQ(moduleStack.GetPrevModule().c_str(), cStrPushNo1.c_str());
    moduleStack.PushModule(cStrPushNo3);
    EXPECT_STREQ(moduleStack.GetPrevModule().c_str(), cStrPushNo2.c_str());
    moduleStack.PopModule();
    EXPECT_STREQ(moduleStack.GetPrevModule().c_str(), cStrPushNo1.c_str());
}
}  // namespace panda::ecmascript
