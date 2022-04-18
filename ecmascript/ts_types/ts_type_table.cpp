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

#include "ecmascript/ts_types/ts_type_table.h"

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {
void TSTypeTable::Initialize(JSThread *thread, const JSPandaFile *jsPandaFile,
                             CVector<JSHandle<EcmaString>> &recordImportModules)
{
    EcmaVM *vm = thread->GetEcmaVM();
    TSLoader *tsLoader = vm->GetTSLoader();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<TSTypeTable> tsTypetable = GenerateTypeTable(thread, jsPandaFile, recordImportModules);

    // Set TStypeTable -> GlobleModuleTable
    JSHandle<EcmaString> fileName = factory->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    tsLoader->AddTypeTable(JSHandle<JSTaggedValue>(tsTypetable), fileName);

    // management dependency module
    while (recordImportModules.size() > 0) {
        CString filename = ConvertToString(recordImportModules.back().GetTaggedValue());
        recordImportModules.pop_back();
        const JSPandaFile *moduleFile = JSPandaFileManager::GetInstance()->OpenJSPandaFile(filename.c_str());
        ASSERT(moduleFile != nullptr);
        TSTypeTable::Initialize(thread, moduleFile, recordImportModules);
    }
}

JSHandle<TSTypeTable> TSTypeTable::GenerateTypeTable(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                     CVector<JSHandle<EcmaString>> &recordImportModules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TSLoader *tsLoader = thread->GetEcmaVM()->GetTSLoader();
    GlobalTSTypeRef ref = GlobalTSTypeRef::Default();
    int typeKind = 0;
    uint32_t summaryIndex = jsPandaFile->GetTypeSummaryIndex();

    JSHandle<TaggedArray> summaryLiteral = LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, summaryIndex);
    ASSERT_PRINT(summaryLiteral->Get(TYPE_KIND_OFFSET).GetInt() == static_cast<int32_t>(TypeLiteralFlag::COUNTER),
                 "summary type literal flag is not counter");

    uint32_t length = static_cast<uint32_t>(summaryLiteral->Get(TABLE_LENGTH_OFFSET_IN_LITREAL).GetInt());
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(length);
    JSMutableHandle<TaggedArray> typeLiteral(thread, JSTaggedValue::Undefined());

    JSHandle<EcmaString> fileName = factory->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    for (uint32_t idx = 1; idx <= length; ++idx) {
        JSHandle<TaggedArray> literalData = LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile,
                                                                                     idx + summaryIndex);
        typeLiteral.Update(literalData);
        JSHandle<JSTaggedValue> type = ParseType(thread, table, typeLiteral, fileName, recordImportModules);
        if (!type->IsNull()) {
            // Set every object type GT
            typeKind = typeLiteral->Get(TYPE_KIND_OFFSET).GetInt();
            ref.SetUserDefineTypeKind(typeKind + GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
            ref.SetModuleId(tsLoader->GetNextModuleId());
            ref.SetLocalId(idx);
            JSHandle<TSType>(type)->SetGTRef(ref);
        }
        table->Set(thread, idx, type);
    }

    table->SetTypeTableLength(thread, length); // record the object type number
    table->SetExportValueTable(thread, *jsPandaFile->GetPandaFile());
    return table;
}

JSHandle<JSTaggedValue> TSTypeTable::ParseType(JSThread *thread, JSHandle<TSTypeTable> &table,
                                               JSHandle<TaggedArray> &literal, JSHandle<EcmaString> fileName,
                                               CVector<JSHandle<EcmaString>> &recordImportModules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TypeLiteralFlag flag = static_cast<TypeLiteralFlag>(literal->Get(TYPE_KIND_OFFSET).GetInt());
    switch (flag) {
        case TypeLiteralFlag::CLASS: {
            JSHandle<TSClassType> classType = ParseClassType(thread, table, literal);
            return JSHandle<JSTaggedValue>(classType);
        }
        case TypeLiteralFlag::CLASS_INSTANCE: {
            JSHandle<TSClassInstanceType> classInstance = factory->NewTSClassInstanceType();
            int classIndex = literal->Get(TSClassInstanceType::CREATE_CLASS_OFFSET).GetInt();
            classInstance->SetClassRefGT(GlobalTSTypeRef(classIndex));
            return JSHandle<JSTaggedValue>(classInstance);
        }
        case TypeLiteralFlag::INTERFACE: {
            JSHandle<TSInterfaceType> interfaceType = ParseInterfaceType(thread, literal);
            return JSHandle<JSTaggedValue>(interfaceType);
        }
        case TypeLiteralFlag::IMPORT: {
            JSHandle<TSImportType> importType = ParseImportType(thread, literal, fileName, recordImportModules);
            return JSHandle<JSTaggedValue>(importType);
        }
        case TypeLiteralFlag::UNION: {
            JSHandle<TSUnionType> unionType = ParseUnionType(thread, literal);
            return JSHandle<JSTaggedValue>(unionType);
        }
        case TypeLiteralFlag::FUNCTION: {
            JSHandle<TSFunctionType> functionType = ParseFunctionType(thread, literal);
            return JSHandle<JSTaggedValue>(functionType);
        }
        case TypeLiteralFlag::ARRAY: {
            JSHandle<TSArrayType> arrayType = ParseArrayType(thread, literal);
            return JSHandle<JSTaggedValue>(arrayType);
        }
        case TypeLiteralFlag::OBJECT: {
            JSHandle<TSObjectType> objectType = ParseObjectType(thread, literal);
            return JSHandle<JSTaggedValue>(objectType);
        }
        default:
            UNREACHABLE();
    }
    // not support type yet
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
}

GlobalTSTypeRef TSTypeTable::GetPropertyTypeGT(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                               uint32_t localtypeId, JSHandle<EcmaString> propName)
{
    GlobalTSTypeRef gt = GlobalTSTypeRef::Default();
    switch (typeKind) {
        case TSTypeKind::TS_CLASS: {
            gt = TSClassType::GetPropTypeGT(thread, table, localtypeId, propName);
            break;
        }
        case TSTypeKind::TS_CLASS_INSTANCE: {
            gt = TSClassInstanceType::GetPropTypeGT(thread, table, localtypeId, propName);
            break;
        }
        case TSTypeKind::TS_OBJECT: {
            JSHandle<TSObjectType> ObjectType(thread, table->Get(localtypeId));
            gt = TSObjectType::GetPropTypeGT(table, ObjectType, propName);
            break;
        }
        default:
            UNREACHABLE();
    }
    return gt;
}

panda_file::File::EntityId TSTypeTable::GetFileId(const panda_file::File &pf)
{
    Span<const uint32_t> classIndexes = pf.GetClasses();
    panda_file::File::EntityId fileId {0};
    CString mainMethodName = CString(ENTRY_FUNC_NAME);
    panda_file::File::StringData sd = {static_cast<uint32_t>(mainMethodName.size()),
                                       reinterpret_cast<const uint8_t *>(mainMethodName.c_str())};

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

JSHandle<TaggedArray> TSTypeTable::GetExportTableFromPandFile(JSThread *thread, const panda_file::File &pf)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    panda_file::File::EntityId fileId = GetFileId(pf);
    panda_file::MethodDataAccessor mda(pf, fileId);

    CVector<CString> exportTable;
    const char *symbols;
    const char *symbolTypes;
    auto *fileName = pf.GetFilename().c_str();
    if (::strcmp(DECLARED_FILE_NAME, fileName) == 0) {
        symbols = DECLARED_SYMBOLS;
        symbolTypes = DECLARED_SYMBOL_TYPES;
    } else {
        symbols = EXPORTED_SYMBOLS;
        symbolTypes = EXPORTED_SYMBOL_TYPES;
    }

    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
    panda_file::AnnotationDataAccessor ada(pf, annotation_id);
    auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
    ASSERT(annotationName != nullptr);
    if (::strcmp("L_ESTypeAnnotation;", annotationName) == 0) {
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
            uint32_t elemCount = adae.GetArrayValue().GetCount();
            ASSERT(elemName != nullptr);
            if (::strcmp(symbols, elemName) == 0) { // symbols -> ["A", "B", "C"]
                for (uint32_t j = 0; j < elemCount; ++j) {
                    auto valueEntityId = adae.GetArrayValue().Get<panda_file::File::EntityId>(j);
                    auto *valueStringData = reinterpret_cast<const char *>(pf.GetStringData(valueEntityId).data);
                    CString target = ConvertToString(std::string(valueStringData));
                    exportTable.push_back(target);
                }
            }
            if (::strcmp(symbolTypes, elemName) == 0) { // symbolTypes -> [51, 52, 53]
                for (uint32_t k = 0; k < elemCount; ++k) {
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
        JSHandle<EcmaString> typeIdString = factory->NewFromUtf8(exportTable[i]);
        exportArray->Set(thread, i, typeIdString);
    }
    return exportArray;
}

JSHandle<TSImportType> TSTypeTable::ParseImportType(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                    JSHandle<EcmaString> fileName,
                                                    CVector<JSHandle<EcmaString>> &recordImportModules)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<EcmaString> importVarNamePath(thread, literal->Get(TSImportType::IMPORT_PATH_OFFSET_IN_LITERAL)); // #A#./A
    JSHandle<EcmaString> targetAndPathEcmaStr = GenerateVarNameAndPath(thread, importVarNamePath,
                                                                       fileName, recordImportModules);
    JSHandle<TSImportType> importType = factory->NewTSImportType();
    importType->SetImportPath(thread, targetAndPathEcmaStr);
    return importType;
}

JSHandle<TSClassType> TSTypeTable::ParseClassType(JSThread *thread, JSHandle<TSTypeTable> &typeTable,
                                                  const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSClassType> classType = factory->NewTSClassType();
    int32_t index = 0;
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::CLASS);

    index = index + 2;  // 2: ignore accessFlag and readonly
    int32_t extendsTypeId = literal->Get(index++).GetInt();

    // ignore implement
    int32_t numImplement = literal->Get(index++).GetInt();
    index += numImplement;

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> typeId(thread, JSTaggedValue::Undefined());

    // resolve instance type
    uint32_t numBaseFields = 0;
    uint32_t numFields = static_cast<uint32_t>(literal->Get(index++).GetInt());

    JSHandle<TSObjectType> instanceType;
    if (extendsTypeId) { // base class is 0
        uint32_t realExtendsTypeId = static_cast<uint32_t>(extendsTypeId - GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
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
    int32_t numNonStatic = literal->Get(index++).GetInt();
    JSHandle<TSObjectType> prototypeType = factory->NewTSObjectType(numNonStatic);

    JSHandle<TSObjLayoutInfo> nonStaticTypeInfo(thread, prototypeType->GetObjLayoutInfo());
    ASSERT(nonStaticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numNonStatic));
    for (int32_t nonStaticIndex = 0; nonStaticIndex < numNonStatic; ++nonStaticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        nonStaticTypeInfo->SetKey(thread, nonStaticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    classType->SetPrototypeType(thread, prototypeType);

    // resolve constructor type
    // stitic include fields and methods, which the former takes up 4 spaces and the latter takes up 2 spaces.
    int32_t numStaticFields = literal->Get(index++).GetInt();
    int32_t numStaticMethods = literal->Get(index + numStaticFields * TSClassType::FIELD_LENGTH).GetInt();
    int32_t numStatic = numStaticFields + numStaticMethods;
    // new function type when support it
    JSHandle<TSObjectType> constructorType = factory->NewTSObjectType(numStatic);

    JSHandle<TSObjLayoutInfo> staticTypeInfo(thread, constructorType->GetObjLayoutInfo());
    ASSERT(staticTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numStatic));
    for (int32_t staticIndex = 0; staticIndex < numStaticFields; ++staticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        index += 2;  // 2: ignore accessFlag and readonly
        staticTypeInfo->SetKey(thread, staticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    index++;  // jmp over numStaticMethods

    // static methods
    for (int32_t staticIndex = numStaticFields; staticIndex < numStatic; ++staticIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        staticTypeInfo->SetKey(thread, staticIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    classType->SetConstructorType(thread, constructorType);
    return classType;
}

JSHandle<TSInterfaceType> TSTypeTable::ParseInterfaceType(JSThread *thread, const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t index = 0;
    JSHandle<TSInterfaceType> interfaceType = factory->NewTSInterfaceType();
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::INTERFACE);

    index++;
    // resolve extends of interface
    int32_t numExtends = literal->Get(index++).GetInt();
    JSHandle<TaggedArray> extendsId = factory->NewTaggedArray(numExtends);
    JSMutableHandle<JSTaggedValue> extendsType(thread, JSTaggedValue::Undefined());
    for (int32_t extendsIndex = 0; extendsIndex < numExtends; extendsIndex++) {
        extendsType.Update(literal->Get(index++));
        extendsId->Set(thread, extendsIndex, extendsType);
    }
    interfaceType->SetExtends(thread, extendsId);

    // resolve fields of interface
    int32_t numFields = literal->Get(index++).GetInt();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> typeId(thread, JSTaggedValue::Undefined());

    JSHandle<TSObjectType> fieldsType = factory->NewTSObjectType(numFields);

    JSHandle<TSObjLayoutInfo> fieldsTypeInfo(thread, fieldsType->GetObjLayoutInfo());
    ASSERT(fieldsTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(numFields));
    for (int32_t fieldsIndex = 0; fieldsIndex < numFields; ++fieldsIndex) {
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
    JSHandle<TSObjLayoutInfo> baseInstanceTypeInfo(thread, baseInstanceType->GetObjLayoutInfo());

    *numBaseFields = baseInstanceTypeInfo->GetLength();

    JSHandle<TSObjectType> derivedInstanceType = factory->NewTSObjectType(*numBaseFields + numDerivedFields);
    JSHandle<TSObjLayoutInfo> derivedInstanceTypeInfo(thread, derivedInstanceType->GetObjLayoutInfo());

    JSMutableHandle<JSTaggedValue> baseInstanceKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> baseInstanceTypeId(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < baseInstanceTypeInfo->GetLength(); ++index) {
        baseInstanceKey.Update(baseInstanceTypeInfo->GetKey(index));
        baseInstanceTypeId.Update(baseInstanceTypeInfo->GetTypeId(index));
        derivedInstanceTypeInfo->SetKey(thread, index, baseInstanceKey.GetTaggedValue(),
                                        baseInstanceTypeId.GetTaggedValue());
    }
    return derivedInstanceType;
}

JSHandle<TSUnionType> TSTypeTable::ParseUnionType(JSThread *thread, const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t index = 0;
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::UNION);
    index++;
    int32_t unionTypeLength = literal->Get(index++).GetInt();

    JSHandle<TSUnionType> unionType = factory->NewTSUnionType(unionTypeLength);
    JSHandle<TaggedArray> unionTypeArray(thread, unionType->GetComponentTypes());
    JSMutableHandle<JSTaggedValue> localTypeIdHandle(thread, JSTaggedValue::Undefined());
    for (int32_t unionTypeId = 0; unionTypeId < unionTypeLength; ++unionTypeId) {
        localTypeIdHandle.Update(literal->Get(index++));
        unionTypeArray->Set(thread, unionTypeId, localTypeIdHandle);
    }
    unionType->SetComponentTypes(thread, unionTypeArray);
    return unionType;
}

JSHandle<TaggedArray> TSTypeTable::GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable)
{
    int index = static_cast<int>(typeTable->GetLength()) - 1;
    JSHandle<TaggedArray> exportValueTable(thread, typeTable->Get(index));
    return exportValueTable;
}

void TSTypeTable::SetExportValueTable(JSThread *thread, const panda_file::File &pf)
{
    JSHandle<TaggedArray> exportValueTable = GetExportTableFromPandFile(thread, pf);
    if (exportValueTable->GetLength() != 0) { // add exprotValueTable to tSTypeTable if isn't empty
        Set(thread, GetLength() - 1, exportValueTable);
    }
}

void TSTypeTable::CheckModule(JSThread *thread, const TSLoader* tsLoader,  const JSHandle<EcmaString> target,
                              CVector<JSHandle<EcmaString>> &recordImportModules)
{
    int32_t entry = tsLoader->GetTSModuleTable()->GetGlobalModuleID(thread, target);
    if (entry == -1) {
        bool flag = false;
        for (const auto it : recordImportModules) {
            if (EcmaString::StringsAreEqual(*it, *target)) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            recordImportModules.push_back(target);
        }
    }
}

JSHandle<EcmaString> TSTypeTable::GenerateVarNameAndPath(JSThread *thread, JSHandle<EcmaString> importPath,
                                                         JSHandle<EcmaString> fileName,
                                                         CVector<JSHandle<EcmaString>> &recordImportModules)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    TSLoader *tsLoader = ecmaVm->GetTSLoader();

    JSHandle<EcmaString> targetVarName = tsLoader->GenerateImportVar(importPath); // #A#./A -> A
    JSHandle<EcmaString> relativePath = tsLoader->GenerateImportRelativePath(importPath); // #A#./A -> ./A
    JSHandle<EcmaString> fullPathEcmaStr = tsLoader->GenerateAmiPath(fileName, relativePath); // ./A -> XXX/XXX/A
    CheckModule(thread, tsLoader, fullPathEcmaStr, recordImportModules);

    CString fullPath = ConvertToString(fullPathEcmaStr.GetTaggedValue());
    CString target = ConvertToString(targetVarName.GetTaggedValue());
    CString targetNameAndPath = "#" + target + "#" + fullPath; // #A#XXX/XXX/A

    JSHandle<EcmaString> targetNameAndPathEcmaStr = factory->NewFromUtf8(targetNameAndPath);
    return targetNameAndPathEcmaStr;
}

JSHandle<TSFunctionType> TSTypeTable::ParseFunctionType(JSThread *thread, const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t index = 0;
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::FUNCTION);
    index++;

    index += TSFunctionType::FIELD_LENGTH;
    JSHandle<JSTaggedValue> functionName(thread, literal->Get(index++));

    uint32_t length = 0;
    length = static_cast<uint32_t>(literal->Get(index++).GetInt());
    JSHandle<TSFunctionType> functionType = factory->NewTSFunctionType(length);
    JSHandle<TaggedArray> parameterTypes(thread, functionType->GetParameterTypes());
    JSMutableHandle<JSTaggedValue> parameterTypeRef(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; ++i) {
        parameterTypeRef.Update(literal->Get(index++));
        parameterTypes->Set(thread, i + TSFunctionType::PARAMETER_START_ENTRY, parameterTypeRef);
    }
    JSHandle<JSTaggedValue> returnValueTypeRef = JSHandle<JSTaggedValue>(thread, literal->Get(index++));
    parameterTypes->Set(thread, TSFunctionType::FUNCTION_NAME_OFFSET, functionName);
    parameterTypes->Set(thread, TSFunctionType::RETURN_VALUE_TYPE_OFFSET, returnValueTypeRef);
    functionType->SetParameterTypes(thread, parameterTypes);
    return functionType;
}

JSHandle<TSArrayType> TSTypeTable::ParseArrayType(JSThread *thread, const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t index = 0;
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::ARRAY);
    index++;
    JSHandle<JSTaggedValue> parameterTypeRef(thread, literal->Get(index++));
    ASSERT(parameterTypeRef->IsInt());
    JSHandle<TSArrayType> arrayType = factory->NewTSArrayType();
    arrayType->SetElementTypeRef(parameterTypeRef->GetInt());
    return arrayType;
}

JSHandle<TSObjectType> TSTypeTable::ParseObjectType(JSThread *thread, const JSHandle<TaggedArray> &literal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    uint32_t index = 0;
    ASSERT(static_cast<TypeLiteralFlag>(literal->Get(index).GetInt()) == TypeLiteralFlag::OBJECT);
    index++;
    int32_t length = literal->Get(index++).GetInt();

    JSHandle<TSObjectType> objectType = factory->NewTSObjectType(length);
    JSHandle<TSObjLayoutInfo> propertyTypeInfo(thread, objectType->GetObjLayoutInfo());
    ASSERT(propertyTypeInfo->GetPropertiesCapacity() == static_cast<uint32_t>(length));

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> typeId(thread, JSTaggedValue::Undefined());
    for (int32_t fieldIndex = 0; fieldIndex < length; ++fieldIndex) {
        key.Update(literal->Get(index++));
        typeId.Update(literal->Get(index++));
        propertyTypeInfo->SetKey(thread, fieldIndex, key.GetTaggedValue(), typeId.GetTaggedValue());
    }
    objectType->SetObjLayoutInfo(thread, propertyTypeInfo);
    return objectType;
}
} // namespace panda::ecmascript
