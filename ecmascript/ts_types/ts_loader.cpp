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

#include "ecmascript/ts_types/ts_loader.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "libpandafile/file-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
void TSLoader::DecodeTSTypes(const JSPandaFile *jsPandaFile)
{
    ASSERT_PRINT(jsPandaFile->HasTSTypes(), "the file has no ts type info");
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();

    JSHandle<EcmaString> queryFileName = factory->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    int index = mTable->GetGlobalModuleID(thread, queryFileName);
    if (index == TSModuleTable::NOT_FOUND) {
        CVector<JSHandle<EcmaString>> recordImportModules {};
        TSTypeTable::Initialize(thread, jsPandaFile, recordImportModules);
        Link();
    }
}

TSLoader::TSLoader(EcmaVM *vm) : vm_(vm)
{
    JSHandle<TSModuleTable> mTable = vm_->GetFactory()->NewTSModuleTable(TSModuleTable::DEFAULT_TABLE_CAPACITY);
    TSModuleTable::Initialize(vm_->GetJSThread(), mTable);
    SetTSModuleTable(mTable);
}

void TSLoader::AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<TSModuleTable> updateTable = TSModuleTable::AddTypeTable(thread, table, typeTable, amiPath);
    SetTSModuleTable(updateTable);
}

void TSLoader::Link()
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int length = table->GetNumberOfTSTypeTables();
    for (int i = 0; i < length; i++) {
        JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, i);
        LinkTSTypeTable(typeTable);
    }
}

void TSLoader::LinkTSTypeTable(JSHandle<TSTypeTable> table)
{
    uint32_t length = table->GetLength();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSMutableHandle<TSImportType> importType(factory->NewTSImportType());
    for (uint32_t i = 1; i < length && table->Get(i).IsTSImportType(); i++) {
        importType.Update(table->Get(i));
        RecursivelyResolveTargetType(importType);
        table->Set(thread, i, importType);
    }
}

void TSLoader::RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType)
{
    if (!importType->GetTargetRefGT().IsDefault()) {
        return;
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    JSHandle<EcmaString> importVarAndPath(thread, importType->GetImportPath());
    JSHandle<EcmaString> target = GenerateImportVar(importVarAndPath);
    JSHandle<EcmaString> amiPath = GenerateImportRelativePath(importVarAndPath);
    int index = table->GetGlobalModuleID(thread, amiPath);
    ASSERT(index != TSModuleTable::NOT_FOUND);
    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, index);
    JSHandle<TaggedArray> moduleExportTable = TSTypeTable::GetExportValueTable(thread, typeTable);

    int localId = GetTypeIndexFromExportTable(target, moduleExportTable);
    if (GlobalTSTypeRef(static_cast<uint32_t>(localId)).IsBuiltinType()) {
        importType->SetTargetRefGT(GlobalTSTypeRef(localId));
        return;
    }
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localId);

    JSHandle<TSType> bindType(thread, typeTable->Get(static_cast<uint32_t>(userDefId)));
    if (bindType.GetTaggedValue().IsTSImportType()) {
        JSMutableHandle<TSImportType> redirectImportType(bindType);
        RecursivelyResolveTargetType(redirectImportType);
        typeTable->Set(thread, static_cast<uint32_t>(userDefId), redirectImportType);
        importType->SetTargetRefGT(redirectImportType->GetTargetRefGT());
    } else {
        importType->SetTargetRefGT(bindType->GetGTRef());
    }
}

int TSLoader::GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const
{
    uint32_t capacity = exportTable->GetLength();
    // capacity/2 -> ["A", "B", 51, 52] -> if target is "A", return 51, index = 0 + 4/2 = 2
    uint32_t length = capacity / 2 ;
    for (uint32_t i = 0; i < length; i++) {
        EcmaString *valueString = EcmaString::Cast(exportTable->Get(i).GetTaggedObject());
        if (EcmaString::StringsAreEqual(*target, valueString)) {
            EcmaString *localIdString = EcmaString::Cast(exportTable->Get(i + 1).GetTaggedObject());
            return CStringToULL(ConvertToString(localIdString));
        }
    }
    return -1;
}

GlobalTSTypeRef TSLoader::GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    TSTypeKind typeKind = GetTypeKind(gt);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    GlobalTSTypeRef propTypeRef = TSTypeTable::GetPropertyTypeGT(thread, typeTable, typeKind, localId, propertyName);
    return propTypeRef;
}

GlobalTSTypeRef TSLoader::GetPropType(GlobalTSTypeRef gt, const uint64_t key) const
{
    ObjectFactory *factory = vm_->GetFactory();
    auto propertyName = factory->NewFromStdString(std::to_string(key).c_str());
    return GetPropType(gt, propertyName);
}

uint32_t TSLoader::GetUnionTypeLength(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::UNION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(localId));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponents());

    return unionTypeArray->GetLength();
}

GlobalTSTypeRef TSLoader::GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::UNION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(localId));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponents());

    int localUnionId = unionTypeArray->Get(index).GetNumber();
    if (GlobalTSTypeRef(localUnionId).IsBuiltinType()) {
        return GlobalTSTypeRef(localUnionId);
    }
    int unionTableIndex = TSTypeTable::GetUserdefinedTypeId(localUnionId);
    JSHandle<TSType> resultType(thread, typeTable->Get(unionTableIndex));
    if (resultType.GetTaggedValue().IsTSImportType()) {
        return JSHandle<TSImportType>(resultType)->GetTargetRefGT();
    }
    return resultType->GetGTRef();
}

GlobalTSTypeRef TSLoader::GetGTFromPandaFile(const panda_file::File &pf, uint32_t vregId, const JSMethod* method) const
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();

    panda_file::File::EntityId fieldId = method->GetMethodId();
    panda_file::MethodDataAccessor mda(pf, fieldId);

    uint32_t localId = 0;
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(pf, annotation_id);
        auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
        ASSERT(annotationName != nullptr);
        if (::strcmp("L_ESTypeAnnotation;", annotationName) == 0) {
            uint32_t length = ada.GetCount();
            for (uint32_t i = 0; i < length; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elemName = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
                ASSERT(elemName != nullptr);
                uint32_t elemCount = adae.GetArrayValue().GetCount();
                if (::strcmp("_TypeOfInstruction", elemName) == 0) {
                    for (uint32_t j = 0; j < elemCount; j = j + 2) { // + 2 means localId index
                        auto value = adae.GetArrayValue().Get<panda_file::File::EntityId>(j).GetOffset();
                        if (value == vregId) {
                            localId = adae.GetArrayValue().Get<panda_file::File::EntityId>(j + 1).GetOffset();
                            break;
                        }
                    }
                }
            }
        }
    });

    if (GlobalTSTypeRef(localId).IsBuiltinType()) {
        return GlobalTSTypeRef(localId);
    }

    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<EcmaString> moduleName = factory->NewFromStdString(pf.GetFilename());
    int moduleId = table->GetGlobalModuleID(thread, moduleName);
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localId);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSType> bindType(thread, typeTable->Get(userDefId));

    return bindType->GetGTRef();
}

void TSLoader::Dump()
{
    std::cout << "TSTypeTables:";
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    uint32_t GTLength = table->GetLength();
    for (uint32_t i = 0; i < GTLength; i++) {
        JSHandle<JSTaggedValue>(thread, table->Get(i))->Dump(std::cout);
    }
}

JSHandle<EcmaString> TSLoader::GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel) const
{
    ObjectFactory *factory = vm_->GetFactory();
    CString currentAbcFile = ConvertToString(cur.GetTaggedValue());
    CString relativeAbcFile = ConvertToString(rel.GetTaggedValue());
    CString fullPath;

    if (relativeAbcFile.find("./") != 0 && relativeAbcFile.find("../") != 0) { // not start with "./" or "../"
        fullPath = relativeAbcFile + ".abc";
        return factory->NewFromUtf8(fullPath); // not relative
    }
    auto slashPos = currentAbcFile.find_last_of('/');
    if (slashPos == std::string::npos) {
        fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
        fullPath.append(".abc"); // ".js" -> ".abc"
        return factory->NewFromUtf8(fullPath);
    }

    fullPath.append(currentAbcFile.substr(0, slashPos + 1)); // 1: with "/"
    fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
    fullPath.append(".abc"); // ".js" -> ".abc"

    return factory->NewFromUtf8(fullPath);
}

JSHandle<EcmaString> TSLoader::GenerateImportVar(JSHandle<EcmaString> import) const
{
    ObjectFactory *factory = vm_->GetFactory();
    // importNamePath #A#./A
    CString importVarNamePath = ConvertToString(import.GetTaggedValue());
    auto firstPos = importVarNamePath.find_first_of('#');
    auto lastPos = importVarNamePath.find_last_of('#');
    CString target = importVarNamePath.substr(firstPos + 1, lastPos - firstPos - 1);
    return factory->NewFromUtf8(target); // #A#./A -> A
}

JSHandle<EcmaString> TSLoader::GenerateImportRelativePath(JSHandle<EcmaString> importRel) const
{
    ObjectFactory *factory = vm_->GetFactory();
    CString importNamePath = ConvertToString(importRel.GetTaggedValue());
    auto lastPos = importNamePath.find_last_of('#');
    CString path = importNamePath.substr(lastPos + 1, importNamePath.size() - lastPos - 1);
    return factory->NewFromUtf8(path); // #A#./A -> ./A
}

GlobalTSTypeRef TSLoader::AddUnionToInferTable(JSHandle<TSUnionType> unionType)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSTypeTable> iTable = GetInferTypeTable();
    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToInferTable(thread, iTable,
                                                                            JSHandle<TSType>(unionType));
    SetInferTypeTable(newITable);

    int localId = newITable->GetNumberOfTypes() - 1;
    GlobalTSTypeRef gt = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, localId,
                                         static_cast<int>(TSTypeKind::UNION));
    unionType->SetGTRef(gt);
    return gt;
}

GlobalTSTypeRef TSLoader::FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(unionType.GetTaggedValue().IsTSUnionType());

    for (int index = 1; index <= table->GetNumberOfTypes(); ++index) {  // index 0 reseved for num of types
        JSTaggedValue type = table->Get(index);
        if (!type.IsTSUnionType()) {
            continue;
        }

        TSUnionType *uType = TSUnionType::Cast(type.GetTaggedObject());
        if (uType->IsEqual(unionType)) {
            return uType->GetGTRef();
        }
    }

    return GlobalTSTypeRef::Default();  // not found
}

GlobalTSTypeRef TSLoader::GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t length = unionTypeVec.size();
    JSHandle<TSUnionType> unionType = factory->NewTSUnionType(length);
    JSHandle<TaggedArray> components(thread, unionType->GetComponents());
    for (uint32_t unionArgIndex = 0; unionArgIndex < length; unionArgIndex++) {
        components->Set(thread, unionArgIndex, JSTaggedValue(unionTypeVec[unionArgIndex].GetType()));
    }
    unionType->SetComponents(thread, components);

    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    for (int tableIndex = 0; tableIndex < mTable->GetNumberOfTSTypeTables(); ++tableIndex) {
        JSHandle<TSTypeTable> typeTable = mTable->GetTSTypeTable(thread, tableIndex);
        GlobalTSTypeRef foundUnionRef = FindUnionInTypeTable(typeTable, unionType);
        if (!foundUnionRef.IsDefault()) {
            return foundUnionRef;
        }
    }

    return AddUnionToInferTable(unionType);
}

void TSLoader::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
    uint64_t length = constantStringTable_.size();
    for (uint64_t i = 0; i < length; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(constantStringTable_.data()[i]))));
    }

    uint64_t hclassTableLength = staticHClassTable_.size();
    for (uint64_t i = 0; i < hclassTableLength; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(staticHClassTable_.data()[i]))));
    }
}

GlobalTSTypeRef TSLoader::GetImportTypeTargetGT(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();

    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::IMPORT);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSImportType> importType(thread, typeTable->Get(localId));

    return importType->GetTargetRefGT();
}

JSHandle<TSTypeTable> TSLoader::GetInferTypeTable() const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();

    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread, TSModuleTable::INFER_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::INFER_TABLE_NAME);

    uint32_t inferTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::INFER_TABLE_ID);
    JSHandle<TSTypeTable> inferTable(thread, mTable->Get(inferTableOffset));
    return inferTable;
}

void TSLoader::SetInferTypeTable(JSHandle<TSTypeTable> inferTable)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();

    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread, TSModuleTable::INFER_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::INFER_TABLE_NAME);

    uint32_t inferTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::INFER_TABLE_ID);
    mTable->Set(thread, inferTableOffset, inferTable);
}

int TSLoader::GetFuncParametersNum(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSFunctionType> functionType(thread, typeTable->Get(localId));
    return functionType->GetParametersNum();
}

GlobalTSTypeRef TSLoader::GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSFunctionType> functionType(thread, typeTable->Get(localId));
    return functionType->GetParameterTypeGT(typeTable, index);
}

GlobalTSTypeRef TSLoader::GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSFunctionType> functionType(thread, typeTable->Get(localId));
    return functionType->GetReturnValueTypeGT(typeTable);
}

GlobalTSTypeRef TSLoader::GetArrayParameterTypeGT(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();
    ASSERT(GetTypeKind(gt) == TSTypeKind::ARRAY);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSArrayType> arrayType(thread, typeTable->Get(localId));

    return arrayType->GetElementTypeGT(typeTable);
}

size_t TSLoader::AddConstString(JSTaggedValue string)
{
    auto it = std::find(constantStringTable_.begin(), constantStringTable_.end(), string.GetRawData());
    if (it != constantStringTable_.end()) {
        return it - constantStringTable_.begin();
    } else {
        constantStringTable_.emplace_back(string.GetRawData());
        return constantStringTable_.size() - 1;
    }
}

// add string to constantstringtable and get its index
size_t TSLoader::GetStringIdx(JSHandle<JSTaggedValue> constPool, const uint16_t id)
{
    JSHandle<ConstantPool> newConstPool(vm_->GetJSThread(), constPool.GetTaggedValue());
    auto str = newConstPool->GetObjectFromCache(id);
    return AddConstString(str);
}

bool TSLoader::GetTypeInferenceLog() const
{
    return vm_->GetJSOptions().GetLogTypeInfer();
}

void TSLoader::GenerateStaticHClass(JSHandle<TSTypeTable> tsTypeTable)
{
    JSThread *thread = vm_->GetJSThread();

    JSMutableHandle<TSObjectType> instanceType(thread, JSTaggedValue::Undefined());
    for (int index = 1; index <= tsTypeTable->GetNumberOfTypes(); ++index) {
        JSTaggedValue type = tsTypeTable->Get(index);
        if (!type.IsTSClassType()) {
            continue;
        }

        TSClassType *classType = TSClassType::Cast(type.GetTaggedObject());
        instanceType.Update(classType->GetInstanceType());
        GlobalTSTypeRef gt = classType->GetGTRef();
        JSTaggedValue ihc = JSTaggedValue(TSObjectType::GetOrCreateHClass(thread, instanceType));
        AddStaticHClassInCompilePhase(gt, ihc);
    }
}

JSHandle<JSTaggedValue> TSLoader::GetType(const GlobalTSTypeRef &gt) const
{
    JSThread *thread = vm_->GetJSThread();
    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();

    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    JSHandle<TSTypeTable> typeTable = mTable->GetTSTypeTable(thread, moduleId);
    JSHandle<JSTaggedValue> type(thread, typeTable->Get(localId));
    return type;
}

void TSModuleTable::Initialize(JSThread *thread, JSHandle<TSModuleTable> mTable)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    mTable->SetNumberOfTSTypeTables(thread, DEFAULT_NUMBER_OF_TABLES);

    // set builtins type table
    JSHandle<EcmaString> builtinsTableName = factory->NewFromASCII(TSTypeTable::BUILTINS_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(BUILTINS_TABLE_ID), builtinsTableName);
    mTable->Set(thread, GetSortIdOffset(BUILTINS_TABLE_ID), JSTaggedValue(BUILTINS_TABLE_ID));
    JSHandle<TSTypeTable> builtinsTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(BUILTINS_TABLE_ID), builtinsTable);

    // set infer type table
    JSHandle<EcmaString> inferTableName = factory->NewFromASCII(TSTypeTable::INFER_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(INFER_TABLE_ID), inferTableName);
    mTable->Set(thread, GetSortIdOffset(INFER_TABLE_ID), JSTaggedValue(INFER_TABLE_ID));
    JSHandle<TSTypeTable> inferTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(INFER_TABLE_ID), inferTable);
}

JSHandle<EcmaString> TSModuleTable::GetAmiPathByModuleId(JSThread *thread, int entry) const
{
    int amiOffset = GetAmiPathOffset(entry);
    JSHandle<EcmaString> amiPath(thread, Get(amiOffset));
    return amiPath;
}

JSHandle<TSTypeTable> TSModuleTable::GetTSTypeTable(JSThread *thread, int entry) const
{
    uint32_t typeTableOffset = GetTSTypeTableOffset(entry);
    JSHandle<TSTypeTable> typeTable(thread, Get(typeTableOffset));

    return typeTable;
}

int TSModuleTable::GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const
{
    int length = GetNumberOfTSTypeTables();
    for (int i = 0; i < length; i ++) {
        JSHandle<EcmaString> valueString = GetAmiPathByModuleId(thread, i);
        if (EcmaString::StringsAreEqual(*amiPath, *valueString)) {
            return i;
        }
    }
    return NOT_FOUND;
}

JSHandle<TSModuleTable> TSModuleTable::AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                    JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    int numberOfTSTypeTable = table->GetNumberOfTSTypeTables();
    if (GetTSTypeTableOffset(numberOfTSTypeTable) > table->GetLength()) {
        table = JSHandle<TSModuleTable>(TaggedArray::SetCapacity(thread, JSHandle<TaggedArray>(table),
                                                                 table->GetLength() * INCREASE_CAPACITY_RATE));
    }
    // add ts type table
    table->SetNumberOfTSTypeTables(thread, numberOfTSTypeTable + 1);
    table->Set(thread, GetAmiPathOffset(numberOfTSTypeTable), amiPath);
    table->Set(thread, GetSortIdOffset(numberOfTSTypeTable), JSTaggedValue(numberOfTSTypeTable));
    table->Set(thread, GetTSTypeTableOffset(numberOfTSTypeTable), typeTable);
    return table;
}
} // namespace panda::ecmascript
