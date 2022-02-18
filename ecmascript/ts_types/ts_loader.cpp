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
    int32_t index = table->GetGlobalModuleID(thread, queryFileName);
    if (index == -1) {
        CVector<JSHandle<EcmaString>> recordImportMdoules {};
        TSTypeTable::Initialize(thread, pf, recordImportMdoules);
        Link();
    }
}

TSLoader::TSLoader(EcmaVM *vm) : vm_(vm)
{
    JSHandle<TSModuleTable> table = vm_->GetFactory()->NewTSModuleTable(DEAULT_TABLE_CAPACITY);
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
        JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, i);
        LinkTSTypeTable(typeTable);
    }
}

void TSLoader::LinkTSTypeTable(JSHandle<TSTypeTable> tstypeTable)
{
    int i = 1;
    int length = tstypeTable->GetLength();
    JSThread *thread = vm_->GetJSThread();
    while (i < length && tstypeTable->Get(i).IsTSImportType()) {
        JSMutableHandle<TSImportType> importType = JSMutableHandle<TSImportType>(thread, tstypeTable->Get(i));
        FindTargetTypeId(importType);
        tstypeTable->Set(thread, i, importType.GetTaggedValue());
        i++;
    }
}

uint64_t TSLoader::GetModuleTableIndex(uint64_t LocalId) const
{
    return LocalId - GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES;
}

bool TSLoader::IsPrimtiveBuiltinTypes(uint64_t LocalId) const
{
    return LocalId < GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES ? 1 : 0;
}

void TSLoader::FindTargetTypeId(JSMutableHandle<TSImportType>& importType)
{
    if (!importType->GetTargetType().IsUndefined()) {
        return;
    }
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    JSHandle<EcmaString> importVarAndPath = JSHandle<EcmaString>(thread, importType->GetImportPath());
    JSHandle<EcmaString> target = GenerateImportVar(importVarAndPath);
    JSHandle<EcmaString> amiPath = GenerateImportRelativePath(importVarAndPath);
    int32_t index = table->GetGlobalModuleID(thread, amiPath);
    ASSERT(index != -1);
    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, index);
    JSHandle<TaggedArray> moduleExportTable = TSTypeTable::GetExportValueTable(thread, typeTable);

    uint64_t LocalId = GetTypeIndexFromExportTable(target, moduleExportTable);
    if (IsPrimtiveBuiltinTypes(LocalId)) {
        importType->SetTargetType(JSTaggedValue(LocalId));
        return;
    }
    uint64_t localTableIndex = GetModuleTableIndex(LocalId);

    JSHandle<TSType> bindType = JSHandle<TSType>(thread, typeTable->Get(localTableIndex));
    if (bindType.GetTaggedValue().IsTSImportType()) {
        JSMutableHandle<TSImportType> redirectImportType(bindType);
        FindTargetTypeId(redirectImportType);
        typeTable->Set(thread, localTableIndex, redirectImportType);
        importType->SetTargetType(thread, redirectImportType->GetTargetType());
    } else {
        importType->SetTargetType(thread, bindType);
    }
}

int TSLoader::GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable)
{

    unsigned int capcity = exportTable->GetLength();
    // capcity/2 -> ["A", "B", 51, 52] -> if target is "A", return 51, index = 0 + 4/2 = 2
    unsigned int length = capcity / 2 ;
    for (unsigned int i = 0; i < length; i++) {
        EcmaString *valueString = EcmaString::Cast(exportTable->Get(i).GetTaggedObject());
        if (EcmaString::StringsAreEqual(*target, valueString)) {
            EcmaString *localIdString = EcmaString::Cast(exportTable->Get(i + 1).GetTaggedObject());
            return CStringToULL(ConvertToString(localIdString));
        }
    }
    return -1;
}

GlobalTSTypeRef TSLoader::GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localTSTypeTableId = gt.GetLocalId();
    uint32_t localTableIndex = GetModuleTableIndex(localTSTypeTableId);
    TSTypeKind typeKind = GetTypeKind(gt);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, moduleId);
    GlobalTSTypeRef propTypeRef = TSTypeTable::GetPropertyType(thread, typeTable, typeKind, localTableIndex,
                                                               propertyName);
    return propTypeRef;
}

int TSLoader::GetUnionTypeLength(GlobalTSTypeRef gt)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localTSTypetableId = gt.GetLocalId();
    uint32_t localTableIndex = GetModuleTableIndex(localTSTypetableId);
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_UNION);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, moduleId);
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(localTableIndex));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());

    return unionTypeArray->GetLength();
}

uint32_t TSLoader::GetUTableIndex(GlobalTSTypeRef gt, uint32_t index)
{
    JSThread *thread = vm_->GetJSThread();
    uint32_t localTSTypetableId = gt.GetLocalId();
    uint32_t localTableIndex = GetModuleTableIndex(localTSTypetableId);

    JSHandle<TaggedArray> typeTable = GetGlobalUTable();
    JSHandle<TSUnionType> unionType(thread, typeTable->Get(localTableIndex));
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());
    ASSERT(index < unionTypeArray->GetLength());
    uint32_t localUnionId = unionTypeArray->Get(index).GetNumber();
    return localUnionId;
}

GlobalTSTypeRef TSLoader::GetUnionTypeByIndex(GlobalTSTypeRef gt, uint32_t index)
{
    JSThread *thread = vm_->GetJSThread();
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_UNION);

    JSHandle<TaggedArray> typeTable = GetGlobalUTable();
    uint32_t localUnionId = GetUTableIndex(gt, index);
    if (IsPrimtiveBuiltinTypes(localUnionId)) {
        return GlobalTSTypeRef(localUnionId);
    }
    uint32_t unionTableIndex = GetModuleTableIndex(localUnionId);
    JSHandle<TSType> resulteType(thread, typeTable->Get(unionTableIndex));
    if (resulteType.GetTaggedValue().IsTSImportType()) {
            JSHandle<TSImportType> ImportType(thread, typeTable->Get(unionTableIndex));
            return JSHandle<TSType>(thread, ImportType->GetTargetType())->GetGTRef();
        }
    return resulteType->GetGTRef();
}

GlobalTSTypeRef TSLoader::GetGTFromPandFile(const panda_file::File &pf, int localId)
{
    if (IsPrimtiveBuiltinTypes(localId)) {
        return GlobalTSTypeRef(localId);
    }

    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    CString moduleName = CString(pf.GetFilename());
    uint32_t moduleId = table->GetGlobalModuleID(thread, factory->NewFromString(moduleName));
    uint32_t localTableIndex = GetModuleTableIndex(localId);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, moduleId);
    JSHandle<TSType> bindType = JSHandle<TSType>(thread, typeTable->Get(localTableIndex));

    return bindType->GetGTRef();
}

void TSLoader::Dump()
{
    std::cout << "TSTypeTables:";
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    unsigned int GTLength = table->GetLength();
    for (unsigned int i = 0; i < GTLength; i++) {
        JSHandle<JSTaggedValue>(thread, table->Get(i))->Dump(thread, std::cout);
    }
}

JSHandle<EcmaString> TSLoader::GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel)
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

JSHandle<EcmaString> TSLoader::GenerateImportVar(JSHandle<EcmaString> import)
{
    ObjectFactory *factory = vm_->GetFactory();
    // importNamePath #A#./A
    CString importVarNamePath = ConvertToString(import.GetTaggedValue());
    auto firstPos = importVarNamePath.find_first_of('#');
    auto lastPos = importVarNamePath.find_last_of('#');
    CString target = importVarNamePath.substr(firstPos + 1, lastPos - firstPos - 1);
    return factory->NewFromString(target); // #A#./A -> A
}

JSHandle<EcmaString> TSLoader::GenerateImportRelativePath(JSHandle<EcmaString> importRel)
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
    uint64_t unionIndex = utWithNewUnion->GetLength() - 1;
    utWithNewUnion->Set(thread, unionIndex, unionType.GetTaggedValue());

    int unionTypeTableOffset = TSModuleTable::GetTSTypeTableOffset(unionTableId);
    table->Set(thread, unionTypeTableOffset, utWithNewUnion.GetTaggedValue());
    SetTSModuleTable(table);
    GlobalTSTypeRef addUnionRef = CreateGT(unionTableId, unionIndex, static_cast<uint8_t>(TSTypeKind::TS_UNION));
    return addUnionRef;
}

GlobalTSTypeRef TSLoader::CreateGT(int moduleId, int localId, int typeKind)
{
    return GlobalTSTypeRef(moduleId, localId, typeKind);
}

GlobalTSTypeRef TSLoader::FindInGlobalUTable(JSHandle<TSUnionType> unionType)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int unionTableId = table->GetNumberOfTSTypeTable();

    JSHandle<EcmaString> finalNameString = table->GetAmiPathByModuleId(thread, unionTableId);
    [[maybe_unused]] CString finalModuleName = ConvertToString(finalNameString.GetTaggedValue());
    ASSERT(finalModuleName == "UnionTypeTable");

    JSHandle<TaggedArray> unionTypeTable = GetGlobalUTable();
    uint32_t tableLength = unionTypeTable->GetLength();
    GlobalTSTypeRef foundUnionRef = GlobalTSTypeRef::Default();
    for (uint32_t unionIndex = 0; unionIndex < tableLength; unionIndex++) {
        JSHandle<TSUnionType> unionTemp = JSHandle<TSUnionType>(thread, unionTypeTable->Get(unionIndex));
        ASSERT(unionTemp.GetTaggedValue().IsTSUnionType() && unionType.GetTaggedValue().IsTSUnionType());
        bool foundTag = unionTemp->IsEqual(thread, unionType);
        if (foundTag) {
            foundUnionRef = CreateGT(unionTableId, unionIndex,
                                     static_cast<uint8_t>(TSTypeKind::TS_UNION));
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

    for (int32_t unionArgIndex = 0; unionArgIndex < size; unionArgIndex++) {
        unionTypeArray->Set(thread, unionArgIndex, JSTaggedValue(unionTypeRef[unionArgIndex].GetGlobalTSTypeRef()));
    }
    unionType->SetComponentTypes(thread, unionTypeArray);

    GlobalTSTypeRef foundUnionRef = FindInGlobalUTable(unionType);
    int unionModuleId = table->GetNumberOfTSTypeTable();

    uint64_t foundUnionModuleId = foundUnionRef.GetModuleId();
    if (foundUnionModuleId != unionModuleId) {
        foundUnionRef = AddUinonTypeToGlobalUnionTable(unionType);
    }
    return foundUnionRef;
}

void TSLoader::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
}

GlobalTSTypeRef TSLoader::GetPrmitiveGT(TSTypeKind kind)
{
    return GlobalTSTypeRef(static_cast<uint64_t>(kind));
}

GlobalTSTypeRef TSLoader::GetImportTypeTargetGT(GlobalTSTypeRef gt)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    uint32_t moduleId = gt.GetModuleId();
    uint32_t localTSTypetableId = gt.GetLocalId();
    uint32_t LocalTableIndex = GetModuleTableIndex(localTSTypetableId);
    [[maybe_unused]] TSTypeKind typeKind = GetTypeKind(gt);
    ASSERT(typeKind == TSTypeKind::TS_IMPORT);

    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTableByindex(thread, moduleId);
    JSHandle<TSImportType> importType(thread, typeTable->Get(LocalTableIndex));

    return JSHandle<TSType>(thread, importType->GetTargetType())->GetGTRef();
}

JSHandle<TaggedArray> TSLoader::GetGlobalUTable()
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int moduleId = table->GetNumberOfTSTypeTable();

    int globalUTableOffset = TSModuleTable::GetTSTypeTableOffset(moduleId);
    JSHandle<TaggedArray> globalUTable(thread, table->Get(globalUTableOffset));

    return globalUTable;
}

JSHandle<EcmaString> TSModuleTable::GetAmiPathByModuleId(JSThread *thread, uint32_t entry)
{
    int amiOffset = GetAmiPathOffset(entry);
    JSHandle<EcmaString> amiPath = JSHandle<EcmaString>(thread, Get(amiOffset));
    return amiPath;
}

JSHandle<TSTypeTable> TSModuleTable::GetTSTypeTableByindex(JSThread *thread, uint32_t entry)
{
    int typeTableOffset = GetTSTypeTableOffset(entry);
    JSHandle<TSTypeTable> typeTable = JSHandle<TSTypeTable>(thread, Get(typeTableOffset));

    return typeTable;
}

int TSModuleTable::GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath)
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
                                        table->GetLength()*INCREASE_CAPACITY_RATE));
    }

    // increase 1 tstypeTable
    table->Set(thread, NUMBER_ELEMENTS_OFFSET, JSTaggedValue(numberOfTSTypeTable + 1));
    table->Set(thread, GetAmiPathOffset(numberOfTSTypeTable), amiPath);
    table->Set(thread, GetSortIdOffset(numberOfTSTypeTable), JSTaggedValue(GetSortIdOffset(numberOfTSTypeTable)));
    table->Set(thread, GetTSTypeTableOffset(numberOfTSTypeTable), typeTable);
    return table;
}
}