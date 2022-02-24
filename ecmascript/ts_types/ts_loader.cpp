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
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
void TSLoader::DecodeTSTypes(const panda_file::File &pf)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    JSHandle<EcmaString> queryFileName = factory->NewFromStdString(pf.GetFilename());
    int index = table->GetGlobalModuleID(thread, queryFileName);
    if (index == TSModuleTable::NOT_FOUND) {
        CVector<JSHandle<EcmaString>> recordImportMdoules {};
        TSTypeTable::Initialize(thread, pf, recordImportMdoules);
        Link();
    }
}

TSLoader::TSLoader(EcmaVM *vm) : vm_(vm)
{
    JSHandle<TSModuleTable> table = vm_->GetFactory()->NewTSModuleTable(TSModuleTable::DEAULT_TABLE_CAPACITY);
    SetTSModuleTable(table);
}

void TSLoader::AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<TSModuleTable> largeTable = TSModuleTable::AddTypeTable(thread, table, typeTable, amiPath);
    SetTSModuleTable(largeTable);
}

void TSLoader::Link()
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int length = table->GetNumberOfTSTypeTable();
    for (int i = 0; i < length; i++) {
        JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, i);
        LinkTSTypeTable(typeTable);
    }
}

void TSLoader::LinkTSTypeTable(JSHandle<TSTypeTable> table)
{
    int length = table->GetLength();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSMutableHandle<TSImportType> importType(factory->NewTSImportType());
    for (int i = 1; i < length && table->Get(i).IsTSImportType(); i++) {
        importType.Update(table->Get(i));
        RecursivelyResolveTargetType(importType);
        table->Set(thread, i, importType);
    }
}

bool TSLoader::IsPrimtiveBuiltinTypes(int localId) const
{
    return localId < GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT ? 1 : 0;
}

void TSLoader::RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType)
{
    if (!importType->GetTargetType().IsUndefined()) {
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
    if (IsPrimtiveBuiltinTypes(localId)) {
        importType->SetTargetType(JSTaggedValue(localId));
        return;
    }
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localId);

    JSHandle<TSType> bindType = JSHandle<TSType>(thread, typeTable->Get(userDefId));
    if (bindType.GetTaggedValue().IsTSImportType()) {
        JSMutableHandle<TSImportType> redirectImportType(bindType);
        RecursivelyResolveTargetType(redirectImportType);
        typeTable->Set(thread, userDefId, redirectImportType);
        importType->SetTargetType(thread, redirectImportType->GetTargetType());
    } else {
        importType->SetTargetType(thread, bindType);
    }
}

int TSLoader::GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const
{
    int capcity = exportTable->GetLength();
    // capcity/2 -> ["A", "B", 51, 52] -> if target is "A", return 51, index = 0 + 4/2 = 2
    int length = capcity / 2 ;
    for (int i = 0; i < length; i++) {
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

    int moduleId = gt.GetModuleId();
    int localTSTypeTableId = gt.GetLocalId();
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localTSTypeTableId);
    TSTypeKind typeKind = GetTypeKind(gt);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    GlobalTSTypeRef propTypeRef = TSTypeTable::GetPropertyType(thread, typeTable, typeKind, userDefId,
                                                               propertyName);
    return propTypeRef;
}

int TSLoader::GetUnionTypeLength(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int moduleId = gt.GetModuleId();
    int localTSTypetableId = gt.GetLocalId();
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localTSTypetableId);
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_UNION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(userDefId));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());

    return unionTypeArray->GetLength();
}

int TSLoader::GetUTableIndex(GlobalTSTypeRef gt, int index) const
{
    JSThread *thread = vm_->GetJSThread();
    int localTSTypetableId = gt.GetLocalId();
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localTSTypetableId);

    JSHandle<TaggedArray> typeTable = GetGlobalUTable();
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(userDefId));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());
    ASSERT(index < unionTypeArray->GetLength());
    int localUnionId = unionTypeArray->Get(index).GetNumber();
    return localUnionId;
}

GlobalTSTypeRef TSLoader::GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const
{
    JSThread *thread = vm_->GetJSThread();
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_UNION);

    JSHandle<TaggedArray> typeTable = GetGlobalUTable();
    int localUnionId = GetUTableIndex(gt, index);
    if (IsPrimtiveBuiltinTypes(localUnionId)) {
        return GlobalTSTypeRef(localUnionId);
    }
    int unionTableIndex = TSTypeTable::GetUserdefinedTypeId(localUnionId);
    JSHandle<TSType> resulteType(thread, typeTable->Get(unionTableIndex));
    if (resulteType.GetTaggedValue().IsTSImportType()) {
            JSHandle<TSImportType> ImportType(thread, typeTable->Get(unionTableIndex));
            return JSHandle<TSType>(thread, ImportType->GetTargetType())->GetGTRef();
        }
    return resulteType->GetGTRef();
}

GlobalTSTypeRef TSLoader::GetGTFromPandFile(const panda_file::File &pf, int localId) const
{
    if (IsPrimtiveBuiltinTypes(localId)) {
        return GlobalTSTypeRef(localId);
    }

    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    CString moduleName = CString(pf.GetFilename());
    int moduleId = table->GetGlobalModuleID(thread, factory->NewFromString(moduleName));
    int userDefId = TSTypeTable::GetUserdefinedTypeId(localId);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSType> bindType = JSHandle<TSType>(thread, typeTable->Get(userDefId));

    return bindType->GetGTRef();
}

void TSLoader::Dump()
{
    std::cout << "TSTypeTables:";
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int GTLength = table->GetLength();
    for (int i = 0; i < GTLength; i++) {
        JSHandle<JSTaggedValue>(thread, table->Get(i))->Dump(thread, std::cout);
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
        return factory->NewFromString(fullPath); // not relative
    }
    auto slashPos = currentAbcFile.find_last_of('/');
    if (slashPos == std::string::npos) {
        fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
        fullPath.append(".abc"); // ".js" -> ".abc"
        return factory->NewFromString(fullPath);
    }

    fullPath.append(currentAbcFile.substr(0, slashPos + 1)); // 1: with "/"
    fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
    fullPath.append(".abc"); // ".js" -> ".abc"

    return factory->NewFromString(fullPath);
}

JSHandle<EcmaString> TSLoader::GenerateImportVar(JSHandle<EcmaString> import) const
{
    ObjectFactory *factory = vm_->GetFactory();
    // importNamePath #A#./A
    CString importVarNamePath = ConvertToString(import.GetTaggedValue());
    auto firstPos = importVarNamePath.find_first_of('#');
    auto lastPos = importVarNamePath.find_last_of('#');
    CString target = importVarNamePath.substr(firstPos + 1, lastPos - firstPos - 1);
    return factory->NewFromString(target); // #A#./A -> A
}

JSHandle<EcmaString> TSLoader::GenerateImportRelativePath(JSHandle<EcmaString> importRel) const
{
    ObjectFactory *factory = vm_->GetFactory();
    CString importNamePath = ConvertToString(importRel.GetTaggedValue());
    auto lastPos = importNamePath.find_last_of('#');
    CString path = importNamePath.substr(lastPos + 1, importNamePath.size() - lastPos - 1);
    return factory->NewFromString(path); // #A#./A -> ./A
}

JSHandle<JSTaggedValue> TSLoader::InitUnionTypeTable()
{
    ObjectFactory *factory = vm_->GetFactory();

    JSHandle<JSTaggedValue> initUnionTypeTable = JSHandle<JSTaggedValue>::Cast(factory->EmptyArray());
    return  initUnionTypeTable;
}

GlobalTSTypeRef TSLoader::AddUinonTypeToGlobalUnionTable(JSHandle<TSUnionType> unionType)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int unionTableId = table->GetNumberOfTSTypeTable();
    JSHandle<EcmaString> finalNameString = table->GetAmiPathByModuleId(thread, (unionTableId));
    [[maybe_unused]] CString finalModuleName = ConvertToString(finalNameString.GetTaggedValue());
    ASSERT(finalModuleName == "UnionTypeTable");

    JSHandle<TaggedArray> unionTypeTable = GetGlobalUTable();
    JSHandle<TaggedArray> utWithNewUnion = TaggedArray::SetCapacity(thread, unionTypeTable,
                                                                    unionTypeTable->GetLength() + 1);
    int unionIndex = utWithNewUnion->GetLength() - 1;
    utWithNewUnion->Set(thread, unionIndex, unionType.GetTaggedValue());

    int unionTypeTableOffset = TSModuleTable::GetTSTypeTableOffset(unionTableId);
    table->Set(thread, unionTypeTableOffset, utWithNewUnion.GetTaggedValue());
    SetTSModuleTable(table);
    GlobalTSTypeRef addUnionRef = CreateGT(unionTableId, unionIndex, static_cast<int>(TSTypeKind::TS_UNION));
    return addUnionRef;
}

GlobalTSTypeRef TSLoader::CreateGT(int moduleId, int localId, int typeKind) const
{
    return GlobalTSTypeRef(moduleId, localId, typeKind);
}

GlobalTSTypeRef TSLoader::FindInGlobalUTable(JSHandle<TSUnionType> unionType) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int unionTableId = table->GetNumberOfTSTypeTable();

    JSHandle<EcmaString> finalNameString = table->GetAmiPathByModuleId(thread, unionTableId);
    [[maybe_unused]] CString finalModuleName = ConvertToString(finalNameString.GetTaggedValue());
    ASSERT(finalModuleName == "UnionTypeTable");

    JSHandle<TaggedArray> unionTypeTable = GetGlobalUTable();
    int tableLength = unionTypeTable->GetLength();
    GlobalTSTypeRef foundUnionRef = GlobalTSTypeRef::Default();
    for (int unionIndex = 0; unionIndex < tableLength; unionIndex++) {
        JSHandle<TSUnionType> unionTemp = JSHandle<TSUnionType>(thread, unionTypeTable->Get(unionIndex));
        ASSERT(unionTemp.GetTaggedValue().IsTSUnionType() && unionType.GetTaggedValue().IsTSUnionType());
        bool foundTag = unionTemp->IsEqual(thread, unionType);
        if (foundTag) {
            foundUnionRef = CreateGT(unionTableId, unionIndex,
                                     static_cast<int>(TSTypeKind::TS_UNION));
            break;
        }
    }
    return foundUnionRef;
}

GlobalTSTypeRef TSLoader::GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeRef, int size)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int finalTableId = table->GetNumberOfTSTypeTable();

    JSHandle<EcmaString> finalNameString = table->GetAmiPathByModuleId(thread, finalTableId);
    [[maybe_unused]] CString finalModuleName = ConvertToString(finalNameString.GetTaggedValue());
    if (finalModuleName != "UnionTypeTable") {
        // Set UnionTypeTable -> GlobleModuleTable
        JSHandle<JSTaggedValue> initUnionTypeTable = InitUnionTypeTable();
        JSHandle<EcmaString> unionTableName = factory->NewFromString(CString("UnionTypeTable"));
        AddTypeTable(initUnionTypeTable, unionTableName);
    }

    JSHandle<TSUnionType> unionType = factory->NewTSUnionType(size);
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());

    for (int unionArgIndex = 0; unionArgIndex < size; unionArgIndex++) {
        unionTypeArray->Set(thread, unionArgIndex, JSTaggedValue(unionTypeRef[unionArgIndex].GetGlobalTSTypeRef()));
    }
    unionType->SetComponentTypes(thread, unionTypeArray);

    GlobalTSTypeRef foundUnionRef = FindInGlobalUTable(unionType);
    int unionModuleId = table->GetNumberOfTSTypeTable();

    int foundUnionModuleId = foundUnionRef.GetModuleId();
    if (foundUnionModuleId != unionModuleId) {
        foundUnionRef = AddUinonTypeToGlobalUnionTable(unionType);
    }
    return foundUnionRef;
}

void TSLoader::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
}

GlobalTSTypeRef TSLoader::GetPrmitiveGT(TSTypeKind kind) const
{
    return GlobalTSTypeRef(static_cast<uint64_t>(kind));
}

GlobalTSTypeRef TSLoader::GetImportTypeTargetGT(GlobalTSTypeRef gt) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int moduleId = gt.GetModuleId();
    int localTSTypetableId = gt.GetLocalId();
    int localTableIndex = TSTypeTable::GetUserdefinedTypeId(localTSTypetableId);
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_IMPORT);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread, moduleId);
    JSHandle<TSImportType> importType(thread, typeTable->Get(localTableIndex));

    if (importType->GetTargetType().IsInt()) {
        return GlobalTSTypeRef(importType->GetTargetType().GetInt());
    }
    return JSHandle<TSType>(thread, importType->GetTargetType())->GetGTRef();
}

JSHandle<TaggedArray> TSLoader::GetGlobalUTable() const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int moduleId = table->GetNumberOfTSTypeTable();

    int globalUTableOffset = TSModuleTable::GetTSTypeTableOffset(moduleId);
    JSHandle<TaggedArray> globalUTable(thread, table->Get(globalUTableOffset));

    return globalUTable;
}

JSHandle<EcmaString> TSModuleTable::GetAmiPathByModuleId(JSThread *thread, int entry) const
{
    int amiOffset = GetAmiPathOffset(entry);
    JSHandle<EcmaString> amiPath = JSHandle<EcmaString>(thread, Get(amiOffset));
    return amiPath;
}

JSHandle<TSTypeTable> TSModuleTable::GetTSTypeTable(JSThread *thread, int entry) const
{
    int typeTableOffset = GetTSTypeTableOffset(entry);
    JSHandle<TSTypeTable> typeTable = JSHandle<TSTypeTable>(thread, Get(typeTableOffset));

    return typeTable;
}

int TSModuleTable::GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const
{
    int length = GetNumberOfTSTypeTable();
    for (int i = 0; i < length; i ++) {
        JSHandle<EcmaString> valueString = GetAmiPathByModuleId(thread, i);
        if (EcmaString::StringsAreEqual(*amiPath, *valueString)) {
            return i;
        }
    }
    return -1;
}

JSHandle<TSModuleTable> TSModuleTable::AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                    JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    int numberOfTSTypeTable = table->GetNumberOfTSTypeTable();
    if (GetTSTypeTableOffset(numberOfTSTypeTable) > table->GetLength()) {
        table = JSHandle<TSModuleTable>(TaggedArray::SetCapacity(thread, JSHandle<TaggedArray>(table),
                                        table->GetLength() * INCREASE_CAPACITY_RATE));
    }

    // increase 1 tstypeTable
    table->Set(thread, NUMBER_ELEMENTS_OFFSET, JSTaggedValue(numberOfTSTypeTable + 1));
    table->Set(thread, GetAmiPathOffset(numberOfTSTypeTable), amiPath);
    table->Set(thread, GetSortIdOffset(numberOfTSTypeTable), JSTaggedValue(GetSortIdOffset(numberOfTSTypeTable)));
    table->Set(thread, GetTSTypeTableOffset(numberOfTSTypeTable), typeTable);
    return table;
}
}