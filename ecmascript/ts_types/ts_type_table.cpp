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
#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/literal_data_extractor.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "ecmascript/ts_types/ts_obj_layout_info-inl.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
void TSTypeTable::Initialize(JSThread *thread, const panda_file::File &pf,
                             CVector<JSHandle<EcmaString>> &recordImportMdoules)
{
    EcmaVM *vm = thread->GetEcmaVM();
    TSLoader *tsLoader = vm->GetTSLoader();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<TSTypeTable> tsTypetable = GenerateTypeTable (thread, pf, recordImportMdoules);
    LOG(ERROR, RUNTIME) << "dump TStypeTable";

    // Set TStypeTable -> GlobleModuleTable
    tsLoader->AddTypeTable(JSHandle<JSTaggedValue>(tsTypetable), factory->NewFromStdString(pf.GetFilename()));

    // management dependency module
    while (recordImportMdoules.size() > 0) {
        CString importModuleName = ConvertToString(recordImportMdoules.back().GetTaggedValue());
        recordImportMdoules.pop_back();
        const panda_file::File *moduleFile = GetPandaFile(factory->NewFromString(importModuleName));
        ASSERT(moduleFile != nullptr);
        TSTypeTable::Initialize(thread, *moduleFile, recordImportMdoules);
    }
}

JSHandle<TSTypeTable> TSTypeTable::GenerateTypeTable(JSThread *thread, const panda_file::File &pf,
                                                     CVector<JSHandle<EcmaString>> &recordImportMdoules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TSLoader *tsLoader = thread->GetEcmaVM()->GetTSLoader();
    JSHandle<TaggedArray> summaryLiteral = LiteralDataExtractor::GetDatasIgnoreType(thread, &pf, 0);
    GlobalTSTypeRef globalTSTypeRef(0);
    int typeKind = 0;

    ASSERT_PRINT(summaryLiteral->Get(0).GetInt() == static_cast<int32_t>(TypeLiteralFlag::COUNTER),
                 "summary type literal flag is not counter");

    uint32_t length = summaryLiteral->Get(1).GetInt();

    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(length + 2); // +2 for reserve length and reserve exportTable
    table->Set(thread, 0, JSTaggedValue(length));  // just for corresponding index between type literal and type table

    JSMutableHandle<TaggedArray> typeLiteral(thread, JSTaggedValue::Undefined());
    for (uint32_t idx = 1; idx <= length; ++idx) {
        typeLiteral.Update(LiteralDataExtractor::GetDatasIgnoreType(thread, &pf, idx).GetTaggedValue());
        JSTaggedValue type = ParseType(thread, table, typeLiteral, factory->NewFromStdString(pf.GetFilename()),
                                       recordImportMdoules);
        if (!type.IsNull()) {
            typeKind = typeLiteral->Get(0).GetInt();
            globalTSTypeRef.SetUserDefineTypeKind(typeKind + GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES);
            globalTSTypeRef.SetModuleId(tsLoader->GetNextModuleId());
            globalTSTypeRef.SetLocalId(idx + GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES);
            TSType::Cast(type.GetTaggedObject())->SetGTRef(globalTSTypeRef);
            globalTSTypeRef.Clear();
        }

        table->Set(thread, idx, type);
    }

    JSHandle<TaggedArray> exportValueTable = GetExportTableFromPandFile(thread, pf);
    if (exportValueTable->GetLength() != 0) { //add exprotValueTable to tSTypeTable if isn't empty
        table->Set(thread, table->GetLength() - 1, exportValueTable);
    }

    return table;
}

JSTaggedValue TSTypeTable::ParseType(JSThread *thread, JSHandle<TSTypeTable> &table, JSHandle<TaggedArray> &literal,
                                     JSHandle<EcmaString> fileName, CVector<JSHandle<EcmaString>> &recordImportMdoules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TypeLiteralFlag flag = static_cast<TypeLiteralFlag>(literal->Get(0).GetInt());
    switch (flag) {
        case TypeLiteralFlag::CLASS: {
            JSHandle<TSClassType> classType = ParseClassType(thread, table, literal);
            return classType.GetTaggedValue();
        }
        case TypeLiteralFlag::CLASS_INSTANCE: {
            uint32_t bindIndex = literal->Get(1).GetInt() - GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES;
            JSHandle<TSClassType> createClassType(thread, table->Get(bindIndex));
            ASSERT(createClassType.GetTaggedValue().IsTSClassType());
            JSHandle<TSClassInstanceType> classInstance = factory->NewTSClassInstanceType();
            classInstance->SetCreateClassType(thread, createClassType);
            return classInstance.GetTaggedValue();
        }
        case TypeLiteralFlag::INTERFACE: {
            JSHandle<TSInterfaceType> interfaceType = ParseInterfaceType(thread, table, literal);
            return interfaceType.GetTaggedValue();
        }
        case TypeLiteralFlag::IMPORT: {
            JSHandle<TSImportType> importType = ParseImportType(thread, literal, fileName, recordImportMdoules);
            return importType.GetTaggedValue();
        }
        case TypeLiteralFlag::UNION: {
            JSHandle<TSUnionType> unionType = ParseUnionType(thread, table, literal);
            return unionType.GetTaggedValue();
        }
        case TypeLiteralFlag::FUNCTION:
            break;
        case TypeLiteralFlag::OBJECT:
            break;
        default:
            UNREACHABLE();
    }
    // not support type yet
    return JSTaggedValue::Null();
}

GlobalTSTypeRef TSTypeTable::GetPropertyType(JSThread *thread, JSHandle<TSTypeTable> &table,
                                             TSTypeKind typeKind, uint32_t localtypeId, JSHandle<EcmaString> propName)
{
    CString propertyName = ConvertToString(propName.GetTaggedValue());
    switch (typeKind) {
        case TSTypeKind::TS_CLASS: {
            JSHandle<TSClassType> classType(thread, table->Get(localtypeId));
            JSHandle<TSObjectType> constructorType(thread, classType->GetConstructorType());
            TSObjLayoutInfo *propTypeInfo = TSObjLayoutInfo::Cast(constructorType->GetObjLayoutInfo()
                                                                  .GetTaggedObject());

            for (int index = 0; index < propTypeInfo->NumberOfElements(); ++index) {
                JSTaggedValue tsPropKey = propTypeInfo->GetKey(index);
                if (ConvertToString(tsPropKey) == propertyName) {
                    int localId = propTypeInfo->GetTypeId(index).GetNumber();
                    if (localId < GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES) {
                        return GlobalTSTypeRef(localId);
                    }
                    int localIdNonOffset = localId - GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES;
                    JSHandle<TSType> propertyType(thread, table->Get(localIdNonOffset));
                    if (propertyType.GetTaggedValue().IsTSImportType()) {
                        JSHandle<TSImportType> ImportType(thread, table->Get(localIdNonOffset));
                        return ImportType->GetGTRef();
                    }
                    return propertyType->GetGTRef();
                }
            }
            break;
        }
        case TSTypeKind::TS_CLASS_INSTANCE: {
            JSHandle<TSClassInstanceType> classInstanceType(thread, table->Get(localtypeId));
            uint32_t localId = 0;
            JSHandle<TSClassType> createClassType(thread, classInstanceType->GetCreateClassType());
            JSHandle<TSObjectType> instanceType(thread, createClassType->GetInstanceType());
            JSHandle<TSObjectType> prototype(thread, createClassType->GetPrototypeType());
            TSObjLayoutInfo *instanceTypeInfo = TSObjLayoutInfo::Cast(instanceType->GetObjLayoutInfo()
                                                                      .GetTaggedObject());
            TSObjLayoutInfo *prototypeTypeInfo = TSObjLayoutInfo::Cast(prototype->GetObjLayoutInfo()
                                                                       .GetTaggedObject());

            for (int index = 0; index < instanceTypeInfo->NumberOfElements(); ++index) {
                JSTaggedValue instancePropKey = instanceTypeInfo->GetKey(index);
                if (ConvertToString(instancePropKey) == propertyName) {
                    localId = instanceTypeInfo->GetTypeId(index).GetNumber();
                    if (localId < GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES) {
                        return GlobalTSTypeRef(localId);
                    }
                    uint32_t localIdNonOffset = localId - GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES;
                    JSHandle<TSType> propertyType(thread, table->Get(localIdNonOffset));
                    if (propertyType->GetGTRef().GetUserDefineTypeKind() ==
                        static_cast<uint8_t>(TSTypeKind::TS_IMPORT)) {
                        JSHandle<TSImportType> ImportType(thread, table->Get(localIdNonOffset));
                        return ImportType->GetGTRef();
                    }
                    return propertyType->GetGTRef();
                }
            }

            for (int index = 0; index < prototypeTypeInfo->NumberOfElements(); ++index) {
                JSTaggedValue prototypePropKey = prototypeTypeInfo->GetKey(index);
                if (ConvertToString(prototypePropKey) == propertyName) {
                    localId = prototypeTypeInfo->GetTypeId(index).GetNumber();
                    ASSERT(localId > GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES);
                    return GlobalTSTypeRef(localId);
                }
            }
            break;
        }
        case TSTypeKind::TS_IMPORT: {
            break;
        }
        case TSTypeKind::TS_UNION:
            break;
        case TSTypeKind::TS_FUNCTION:
            break;
        case TSTypeKind::TS_OBJECT:
            break;
        default:
            UNREACHABLE();
    }
    return 0;
}

panda_file::File::EntityId TSTypeTable::GetFileId(const panda_file::File &pf)
{
    Span<const uint32_t> classIndexes = pf.GetClasses();
    panda_file::File::EntityId fileId {0};
    panda_file::File::StringData sd = {static_cast<uint32_t>(ENTRY_FUNC_NAME.size()),
                                       reinterpret_cast<const uint8_t *>(CString(ENTRY_FUNC_NAME).c_str())};

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        cda.EnumerateMethods([&fileId, &pf, &sd](panda_file::MethodDataAccessor &mda) {
            if (pf.GetStringData(mda.GetNameId()) == sd) {
                fileId = mda.GetMethodId();
            }
        });
    }
    return fileId;
}

const panda_file::File* TSTypeTable::GetPandaFile(JSHandle<EcmaString> modulePath)
{
    CString modulePathString = ConvertToString(modulePath.GetTaggedValue());
    auto pandfile = panda_file::OpenPandaFileOrZip(modulePathString, panda_file::File::READ_WRITE);
    ASSERT(pandfile != nullptr);
    return pandfile.release();
}

JSHandle<TaggedArray> TSTypeTable::GetExportTableFromPandFile(JSThread *thread, const panda_file::File &pf)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    panda_file::File::EntityId fileId = GetFileId(pf);
    panda_file::MethodDataAccessor mda(pf, fileId);

    CVector<CString> exportTable;
    const char *exportSymbols;
    const char *exportSymbolTypes;
    auto *fileName = pf.GetFilename().c_str();
    if (::strcmp("ohos.lib.d.ts", fileName) == 0) {
        exportSymbols = "declaredSymbols";
        exportSymbolTypes = "declaredSymbols";
    } else {
        exportSymbols = "exportedSymbols";
        exportSymbolTypes = "exportedSymbolTypes";
    }

    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
    panda_file::AnnotationDataAccessor ada(pf, annotation_id);
    auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
    ASSERT(annotationName != nullptr);
    if (::strcmp("L_ESTypeAnnotation;", annotationName) == 0) {
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elem_name = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
            uint32_t elem_count = adae.GetArrayValue().GetCount();
            ASSERT(elem_name != nullptr);
            if (::strcmp(exportSymbols, elem_name) == 0) { // exportedSymbols -> ["A", "B", "c"]
                for (uint32_t j = 0; j < elem_count; ++j) {
                    auto valueEntityId = adae.GetArrayValue().Get<panda_file::File::EntityId>(j);
                    auto *valueStringData = reinterpret_cast<const char *>(pf.GetStringData(valueEntityId).data);
                    CString target = ConvertToString(std::string(valueStringData));
                    exportTable.push_back(target);
                }
            }
            if (::strcmp(exportSymbolTypes, elem_name) == 0) { // exportedSymbolTypes -> [51, 52, 53]
                for (uint32_t k = 0; k < elem_count; ++k) {
                    auto value = adae.GetArrayValue().Get<panda_file::File::EntityId>(k).GetOffset();
                    CString typeId = ToCString(value);
                    exportTable.push_back(typeId);
                }
            }
        }
    }
    });

    array_size_t length = exportTable.size();
    JSHandle<TaggedArray> exportArray = factory->NewTaggedArray(length);
    for (array_size_t i = 0; i < length; i ++) {
        JSHandle<EcmaString> typeIdString = factory->NewFromString(exportTable[i]);
        exportArray->Set(thread, i, JSHandle<JSTaggedValue>::Cast(typeIdString));
    }
    return exportArray;
}

JSHandle<TSImportType> TSTypeTable::ParseImportType(JSThread *thread, JSHandle<TaggedArray> &literal,
                                                    JSHandle<EcmaString> fileName,
                                                    CVector<JSHandle<EcmaString>> &recordImportMdoules)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    TSLoader *tsLoader = ecmaVm->GetTSLoader();

    JSHandle<TSImportType> importType = factory->NewTSImportType();
    JSHandle<EcmaString> importVarNamePath = JSHandle<EcmaString>(thread, literal->Get(1)); // #A#./A
    JSHandle<EcmaString> targetVar = tsLoader->GenerateImportVar(importVarNamePath); // #A#./A -> A
    JSHandle<EcmaString> relativePath = tsLoader->GenerateImportRelativePath(importVarNamePath); // #A#./A -> ./A
    JSHandle<EcmaString> fullPathHandle = tsLoader->GenerateAmiPath(fileName, relativePath); // ./A -> XXX/XXX/A
    CString fullPath = ConvertToString(fullPathHandle.GetTaggedValue());
    CString target = ConvertToString(targetVar.GetTaggedValue());

    CString indexAndPath = "#" + target + "#" + fullPath; // #A#XXX/XXX/A

    int32_t entry = tsLoader->GetTSModuleTable()->GetGlobalModuleID(thread, fullPathHandle);
    if (entry == -1) {
        bool flag = false;
        for (auto it : recordImportMdoules) {
            if (ConvertToString(it.GetTaggedValue()) == fullPath) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            recordImportMdoules.push_back(fullPathHandle);
        }
    }

    JSHandle<EcmaString> indexAndPathString = factory->NewFromString(indexAndPath);
    importType->SetImportPath(thread, indexAndPathString.GetTaggedValue());
    return importType;
}

JSHandle<TSClassType> TSTypeTable::ParseClassType(JSThread *thread, JSHandle<TSTypeTable> &typeTable,
                                                  JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSClassType> classType = factory->NewTSClassType();
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(0).GetInt()) == TypeLiteralFlag::CLASS);

    uint32_t index = 1;
    index++;  // ignore class modifier

    uint32_t extendsTypeId = literal->Get(index++).GetInt();

    // ignore implement
    uint32_t numImplement = literal->Get(index++).GetInt();
    index += numImplement;

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> typeId(thread, JSTaggedValue::Undefined());

    // resolve instance type
    uint32_t numBaseFields = 0;
    uint32_t numFields = literal->Get(index++).GetInt();

    JSHandle<TSObjectType> instanceType;
    if (extendsTypeId) { // base class is 0
        uint32_t realExtendsTypeId = extendsTypeId - GlobalTSTypeRef::NUM_RESERVED_BUILTINS_TYPES;
        JSHandle<TSClassType> extensionType(thread, typeTable->Get(realExtendsTypeId));
        ASSERT(extensionType.GetTaggedValue().IsTSClassType());
        classType->SetExtensionType(thread, extensionType);
        instanceType = LinkSuper(thread, extensionType, &numBaseFields, numFields);
    } else {
        instanceType = factory->NewTSObjectType(numFields);
    }

    JSHandle<TSObjLayoutInfo> instanceTypeInfo(thread, instanceType->GetObjLayoutInfo());
    ASSERT(instanceTypeInfo->GetPropertiesCapacity() == numBaseFields + numFields);
    for (uint32_t fieldIndex = 0; fieldIndex < numFields; ++fieldIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        index += 2;  // 2: ignore accessFlag and readonly
        instanceTypeInfo->SetKey(thread, numBaseFields + fieldIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    classType->SetInstanceType(thread, instanceType);

    // resolve prototype type
    uint32_t numNonStatic = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> prototypeType = factory->NewTSObjectType(numNonStatic);

    JSHandle<TSObjLayoutInfo> nonStaticTypeInfo(thread, prototypeType->GetObjLayoutInfo());
    ASSERT(nonStaticTypeInfo->GetPropertiesCapacity() == numNonStatic);
    for (uint32_t nonStaticIndex = 0; nonStaticIndex < numNonStatic; ++nonStaticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        nonStaticTypeInfo->SetKey(thread, nonStaticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    classType->SetPrototypeType(thread, prototypeType);

    // resolve constructor type
    // stitic include fields and methods, which the former takes up 4 spaces and the latter takes up 2 spaces.
    uint32_t numStaticFields = literal->Get(index++).GetInt();
    uint32_t numStaticMethods = literal->Get(index + numStaticFields * TSClassType::FIELD_LENGTH).GetInt();
    uint32_t numStatic = numStaticFields + numStaticMethods;
    // new function type when support it
    JSHandle<TSObjectType> constructorType = factory->NewTSObjectType(numStatic);

    JSHandle<TSObjLayoutInfo> staticTypeInfo(thread, constructorType->GetObjLayoutInfo());
    ASSERT(staticTypeInfo->GetPropertiesCapacity() == numStatic);
    for (uint32_t staticIndex = 0; staticIndex < numStaticFields; ++staticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        index += 2;  // 2: ignore accessFlag and readonly
        staticTypeInfo->SetKey(thread, staticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    index++;  // jmp over numStaticMethods

    // static methods
    for (uint32_t staticIndex = numStaticFields; staticIndex < numStatic; ++staticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        staticTypeInfo->SetKey(thread, staticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    classType->SetConstructorType(thread, constructorType);
    return classType;
}

JSHandle<TSInterfaceType> TSTypeTable::ParseInterfaceType(JSThread *thread, JSHandle<TSTypeTable> &typeTable,
                                                          JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TSInterfaceType> interfaceType = factory->NewTSInterfaceType();
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(0).GetInt()) == TypeLiteralFlag::INTERFACE);

    uint32_t index = 1;
    // resolve extends of interface
    uint32_t numExtends = literal->Get(index++).GetInt();
    JSHandle<TaggedArray> extendsId(factory->NewTaggedArray(numExtends));
    JSMutableHandle<JSTaggedValue> extendsType(thread, JSTaggedValue::Undefined());
    for (uint32_t extendsIndex = 0; extendsIndex < numExtends; extendsIndex++) {
        extendsType.Update(literal->Get(index++));
        extendsId->Set(thread, extendsIndex, extendsType);
    }
    interfaceType->SetExtends(extendsId.GetTaggedValue());

    // resolve fields of interface
    uint32_t numFields = literal->Get(index++).GetInt();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> typeId(thread, JSTaggedValue::Undefined());

    JSHandle<TSObjectType> fieldsType = factory->NewTSObjectType(numFields);

    JSHandle<TSObjLayoutInfo> fieldsTypeInfo(thread, fieldsType->GetObjLayoutInfo());
    ASSERT(fieldsTypeInfo->GetPropertiesCapacity() == numFields);
    for (uint32_t fieldsIndex = 0; fieldsIndex < numFields; ++fieldsIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        index += 2;  // 2: ignore accessFlag and readonly
        fieldsTypeInfo->SetKey(thread, fieldsIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    interfaceType->SetFields(thread, fieldsType);

    return interfaceType;
}

JSHandle<TSObjectType> TSTypeTable::LinkSuper(JSThread *thread, JSHandle<TSClassType> &baseClassType,
                                              uint32_t *numBaseFields, uint32_t numDerivedFields)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<TSObjectType> baseInstanceType(thread, baseClassType->GetInstanceType());
    TSObjLayoutInfo *baseInstanceTypeInfo = TSObjLayoutInfo::Cast(baseInstanceType->GetObjLayoutInfo()
                                                                  .GetTaggedObject());
    *numBaseFields = baseInstanceTypeInfo->GetLength();

    JSHandle<TSObjectType> derivedInstanceType = factory->NewTSObjectType(*numBaseFields + numDerivedFields);
    JSHandle<TSObjLayoutInfo> derivedInstanceTypeInfo(thread, derivedInstanceType->GetObjLayoutInfo());

    for (uint32_t index = 0; index < baseInstanceTypeInfo->GetLength(); ++index) {
        JSTaggedValue baseInstanceKey = baseInstanceTypeInfo->GetKey(index);
        JSTaggedValue baseInstanceTypeId = baseInstanceTypeInfo->GetTypeId(index);
        derivedInstanceTypeInfo->SetKey(thread, index, baseInstanceKey, baseInstanceTypeId);
    }
    return derivedInstanceType;
}

JSHandle<TSUnionType> TSTypeTable::ParseUnionType(JSThread *thread, JSHandle<TSTypeTable> &typeTable,
                                                  JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(0).GetInt()) == TypeLiteralFlag::UNION);

    uint32_t index = 1;
    uint32_t unionTypeLength = literal->Get(index++).GetInt();

    JSHandle<TSUnionType> unionType = factory->NewTSUnionType(unionTypeLength);
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());
    JSMutableHandle<JSTaggedValue> localTypeIdHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t unionTypeId = 0; unionTypeId < unionTypeLength; ++unionTypeId) {
        localTypeIdHandle.Update(literal->Get(index++));
        unionTypeArray->Set(thread, unionTypeId, localTypeIdHandle);
    }
    unionType->SetComponentTypes(thread, unionTypeArray);
    return unionType;
}

JSHandle<TaggedArray> TSTypeTable::GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable)
{
    int index = typeTable->GetLength() - 1;
    JSHandle<TaggedArray> exportValueTable = JSHandle<TaggedArray>(thread, typeTable->Get(index));
    return exportValueTable;
}
}
