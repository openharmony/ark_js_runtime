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

#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/linked_hash_table.h"


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
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/*
 * Feature: Module
 * Function: AddImportEntry
 * SubFunction: AddImportEntry
 * FunctionPoints: Add import entry
 * CaseDescription: Add two import item and check module import entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddImportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<ImportEntry> importEntry1 = objectFactory->NewImportEntry();
    SourceTextModule::AddImportEntry(thread, module, importEntry1);
    JSHandle<ImportEntry> importEntry2 = objectFactory->NewImportEntry();
    SourceTextModule::AddImportEntry(thread, module, importEntry2);
    JSHandle<TaggedArray> importEntries(thread, module->GetImportEntries());
    EXPECT_TRUE(importEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: AddLocalExportEntry
 * SubFunction: AddLocalExportEntry
 * FunctionPoints: Add local export entry
 * CaseDescription: Add two local export item and check module local export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddLocalExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<ExportEntry> exportEntry1 = objectFactory->NewExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, exportEntry1);
    JSHandle<ExportEntry> exportEntry2 = objectFactory->NewExportEntry();
    SourceTextModule::AddLocalExportEntry(thread, module, exportEntry2);
    JSHandle<TaggedArray> localExportEntries(thread, module->GetLocalExportEntries());
    EXPECT_TRUE(localExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: AddIndirectExportEntry
 * SubFunction: AddIndirectExportEntry
 * FunctionPoints: Add indirect export entry
 * CaseDescription: Add two indirect export item and check module indirect export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddIndirectExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<ExportEntry> exportEntry1 = objectFactory->NewExportEntry();
    SourceTextModule::AddIndirectExportEntry(thread, module, exportEntry1);
    JSHandle<ExportEntry> exportEntry2 = objectFactory->NewExportEntry();
    SourceTextModule::AddIndirectExportEntry(thread, module, exportEntry2);
    JSHandle<TaggedArray> indirectExportEntries(thread, module->GetIndirectExportEntries());
    EXPECT_TRUE(indirectExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: StarExportEntries
 * SubFunction: StarExportEntries
 * FunctionPoints: Add start export entry
 * CaseDescription: Add two start export item and check module start export entries size
 */
HWTEST_F_L0(EcmaModuleTest, AddStarExportEntry)
{
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<ExportEntry> exportEntry1 = objectFactory->NewExportEntry();
    SourceTextModule::AddStarExportEntry(thread, module, exportEntry1);
    JSHandle<ExportEntry> exportEntry2 = objectFactory->NewExportEntry();
    SourceTextModule::AddStarExportEntry(thread, module, exportEntry2);
    JSHandle<TaggedArray> startExportEntries(thread, module->GetStarExportEntries());
    EXPECT_TRUE(startExportEntries->GetLength() == 2U);
}

/*
 * Feature: Module
 * Function: StoreModuleValue
 * SubFunction: StoreModuleValue/GetModuleValue
 * FunctionPoints: store a module export item in module
 * CaseDescription: Simulated implementation of "export foo as bar", set foo as "hello world",
 *                  use "import bar" in same js file
 */
HWTEST_F_L0(EcmaModuleTest, StoreModuleValue)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    CString localName = "foo";
    CString exportName = "bar";
    CString value = "hello world";

    JSHandle<JSTaggedValue> localNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> exportNameHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSHandle<JSTaggedValue> defaultValue = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<ExportEntry> exportEntry =
        objFactory->NewExportEntry(exportNameHandle, defaultValue, defaultValue, localNameHandle);
    JSHandle<SourceTextModule> module = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, module, exportEntry);

    JSHandle<JSTaggedValue> storeKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(localName));
    JSHandle<JSTaggedValue> valueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(value));
    module->StoreModuleValue(thread, storeKey, valueHandle);

    JSHandle<JSTaggedValue> loadKey = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSTaggedValue loadValue = module->GetModuleValue(thread, loadKey.GetTaggedValue(), false);
    EXPECT_EQ(valueHandle.GetTaggedValue(), loadValue);
}

/*
 * Feature: Module
 * Function: GetModuleValue
 * SubFunction: StoreModuleValue/GetModuleValue
 * FunctionPoints: load module value from module
 * CaseDescription: Simulated implementation of "export default let foo = 'hello world'",
 *                  use "import C from 'xxx' to get default value"
 */
HWTEST_F_L0(EcmaModuleTest, GetModuleValue)
{
    ObjectFactory* objFactory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> defaultValue = thread->GlobalConstants()->GetHandledUndefined();
    // export entry
    CString exportLocalName = "*default*";
    CString exportName = "default";
    CString exportValue = "hello world";
    JSHandle<JSTaggedValue> exportLocalNameHandle =
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportLocalName));
    JSHandle<JSTaggedValue> exportNameHandle =
        JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportName));
    JSHandle<ExportEntry> exportEntry =
        objFactory->NewExportEntry(exportNameHandle, defaultValue, defaultValue, exportLocalNameHandle);
    JSHandle<SourceTextModule> moduleExport = objFactory->NewSourceTextModule();
    SourceTextModule::AddLocalExportEntry(thread, moduleExport, exportEntry);
    // store module value
    JSHandle<JSTaggedValue> exportValueHandle = JSHandle<JSTaggedValue>::Cast(objFactory->NewFromUtf8(exportValue));
    moduleExport->StoreModuleValue(thread, exportLocalNameHandle, exportValueHandle);

    JSTaggedValue importDefaultValue =
        moduleExport->GetModuleValue(thread, exportLocalNameHandle.GetTaggedValue(), false);
    EXPECT_EQ(exportValueHandle.GetTaggedValue(), importDefaultValue);
}

/*
 * Feature: Module
 * Function: EcmaModule use CjsModule
 * SubFunction:
 * FunctionPoints: EcmaModule use CjsModule
 * CaseDescription: Simulated implementation of "module.export.obj = 'hello world cjs'","import {a as b} from "test.js,
 *                   import cjs from "cjs"", print(cjs)", while exectue the ldModuleVar byteCode to load cjs var;
 *                  use ParseCjsModule function to generate Cjsmodule which type is MODULETYPES::CJSMODULE
 */
HWTEST_F_L0(EcmaModuleTest, ParseCjsModule)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> defaultValue = thread->GlobalConstants()->GetHandledUndefined();
    // fileName
    CString ecmaFileName = "currentModule.js";
    CString cjsFileName = "cjs.js";
    CString requesFileName = "test.js";
    // current module
    JSHandle<SourceTextModule> EcmaModule = factory->NewSourceTextModule();
    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromUtf8(ecmaFileName);

    JSHandle<JSTaggedValue> importName1 = JSHandle<JSTaggedValue>(factory->NewFromUtf8("a"));
    JSHandle<JSTaggedValue> localName1 = JSHandle<JSTaggedValue>(factory->NewFromUtf8("b"));
    JSHandle<JSTaggedValue> moduleRequest1 = JSHandle<JSTaggedValue>(factory->NewFromUtf8(requesFileName));
    JSHandle<ImportEntry> importEntry1 = factory->NewImportEntry(moduleRequest1, importName1, localName1);
    JSHandle<JSTaggedValue> importName2 = JSHandle<JSTaggedValue>(factory->NewFromUtf8("default"));
    JSHandle<JSTaggedValue> localName2 = JSHandle<JSTaggedValue>(factory->NewFromUtf8("cjs"));
    JSHandle<JSTaggedValue> moduleRequest2 = JSHandle<JSTaggedValue>(factory->NewFromUtf8(cjsFileName));
    JSHandle<ImportEntry> importEntry2 = factory->NewImportEntry(moduleRequest2, importName2, localName2);

    JSHandle<TaggedArray> requestModuleArray = factory->NewTaggedArray(2);
    requestModuleArray->Set(thread, 0, moduleRequest1);
    requestModuleArray->Set(thread, 1, moduleRequest2);

    SourceTextModule::AddImportEntry(thread, EcmaModule, importEntry1);
    SourceTextModule::AddImportEntry(thread, EcmaModule, importEntry2);
    EcmaModule->SetRequestedModules(thread, requestModuleArray);
    EcmaModule->SetEcmaModuleFilename(thread, ecmaModuleFilename);
    EcmaModule->SetStatus(ModuleStatus::UNINSTANTIATED);
    EcmaModule->SetTypes(ModuleTypes::ECMAMODULE);

    // export module
    JSHandle<SourceTextModule> exportModule = factory->NewSourceTextModule();
    JSHandle<JSTaggedValue> exportModuleValue =JSHandle<JSTaggedValue>::Cast(exportModule);
    JSHandle<EcmaString> exportModuleFilename = factory->NewFromUtf8(requesFileName);
    JSHandle<JSTaggedValue> exportName = JSHandle<JSTaggedValue>(factory->NewFromUtf8("a"));
    JSHandle<JSTaggedValue> localName3 = JSHandle<JSTaggedValue>(factory->NewFromUtf8("a"));
    JSHandle<ExportEntry> exportEntry = factory->NewExportEntry(exportName, defaultValue, defaultValue, localName3);

    JSHandle<TaggedArray> requestModuleArray1 = factory->NewTaggedArray(0);

    SourceTextModule::AddLocalExportEntry(thread, exportModule, exportEntry);
    exportModule->SetRequestedModules(thread, requestModuleArray1);
    exportModule->SetEcmaModuleFilename(thread, exportModuleFilename);
    exportModule->SetStatus(ModuleStatus::UNINSTANTIATED);
    exportModule->SetTypes(ModuleTypes::ECMAMODULE);

    // cjs module
    JSHandle<JSTaggedValue> cjsModule = ModuleDataExtractor::ParseCjsModule(thread, cjsFileName);

    const CString requesFile = "test.abc";
    const CString cjsFile = "cjs.abc";
    thread->GetEcmaVM()->GetModuleManager()->AddResolveImportedModule(requesFile, exportModuleValue);
    thread->GetEcmaVM()->GetModuleManager()->AddResolveImportedModule(cjsFile, cjsModule);

    SourceTextModule::Instantiate(thread, EcmaModule);
    JSTaggedValue moduleEnvironment = EcmaModule->GetEnvironment();

    JSTaggedValue resolvedBinding1 =
        LinkedHashMap::Cast(moduleEnvironment.GetTaggedObject())->Get(localName1.GetTaggedValue());
    JSTaggedValue resolvedBinding2 =
        LinkedHashMap::Cast(moduleEnvironment.GetTaggedObject())->Get(localName2.GetTaggedValue());
    ResolvedBinding *binding1 = ResolvedBinding::Cast(resolvedBinding1.GetTaggedObject());
    ResolvedBinding *binding2 = ResolvedBinding::Cast(resolvedBinding2.GetTaggedObject());
    JSTaggedValue resolvedModule2 = binding2->GetModule();
    JSTaggedValue resolvedModule1 = binding1->GetModule();

    SourceTextModule *module1 = SourceTextModule::Cast(resolvedModule1.GetTaggedObject());
    SourceTextModule *module2 = SourceTextModule::Cast(resolvedModule2.GetTaggedObject());

    CString bindingName1 = ConvertToString(binding1->GetBindingName());
    CString bindingName2 = ConvertToString(binding2->GetBindingName());

    EXPECT_EQ(module1->GetTypes(), ModuleTypes::ECMAMODULE);
    EXPECT_EQ(module2->GetTypes(), ModuleTypes::CJSMODULE);
    EXPECT_EQ(bindingName1, "a");
    EXPECT_EQ(bindingName2, "default");
}
}  // namespace panda::test
