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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H

#include "ecmascript/tagged_array.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
class TSTypeTable : public TaggedArray {
public:
    static constexpr std::string_view ENTRY_FUNC_NAME = "func_main_0";
    static constexpr size_t NUM_OF_TYPES_INDEX_IN_SUMMARY_LITREAL = 1;
    static constexpr size_t TYPE_KIND_INDEX_IN_LITERAL = 0;
    static constexpr size_t RESERVE_TABLE_LENGTH = 2; // for reserve length and exportTable
    static constexpr size_t NUMBER_OF_TYPES_INDEX = 0;
    static constexpr size_t INCREASE_CAPACITY_RATE = 2;
    static constexpr const char* BUILTINS_TABLE_NAME = "ohos.lib.d.ts";
    static constexpr const char* INFER_TABLE_NAME = "inferTypes";
    static constexpr const char* DECLARED_SYMBOLS = "declaredSymbols";
    static constexpr const char* DECLARED_SYMBOL_TYPES = "declaredSymbolTypes";
    static constexpr const char* EXPORTED_SYMBOLS = "exportedSymbols";
    static constexpr const char* EXPORTED_SYMBOL_TYPES = "exportedSymbolTypes";

    inline static TSTypeTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSTypeTable *>(object);
    }

    static void Initialize(JSThread *thread, const JSPandaFile *jsPandaFile,
                           CVector<JSHandle<EcmaString>> &recordImportModules);

    static GlobalTSTypeRef GetPropertyTypeGT(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                           uint32_t index, JSHandle<EcmaString> propName);

    static JSHandle<JSTaggedValue> ParseType(JSThread *thread, JSHandle<TSTypeTable> &table,
                                             JSHandle<TaggedArray> &literal, const JSPandaFile *jsPandaFile,
                                             CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TSImportType> ParseImportType(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                  JSHandle<EcmaString> fileName,
                                                  CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TaggedArray> GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable);

    inline int GetNumberOfTypes() const
    {
        return TaggedArray::Get(NUMBER_OF_TYPES_INDEX).GetInt();
    }

    inline void SetNumberOfTypes(JSThread *thread, uint32_t num)
    {
        TaggedArray::Set(thread, NUMBER_OF_TYPES_INDEX, JSTaggedValue(num));
    }

    static int GetUserdefinedTypeId(int localId)
    {
        return localId - GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT;
    }

    void SetExportValueTable(JSThread *thread, const panda_file::File &pf);

    static JSHandle<TSTypeTable> PushBackTypeToInferTable(JSThread *thread, JSHandle<TSTypeTable> &table,
                                                          const JSHandle<TSType> &type);

private:
    static JSHandle<TSTypeTable> GenerateTypeTable(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                   CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TaggedArray> GetExportTableFromPandFile(JSThread *thread, const panda_file::File &pf);

    static panda_file::File::EntityId GetFileId(const panda_file::File &pf);

    static JSHandle<TSClassType> ParseClassType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSInterfaceType> ParseInterfaceType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSUnionType> ParseUnionType(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                const JSHandle<TaggedArray> &literal);

    static void CheckModule(JSThread *thread, const TSLoader* tsLoader, const JSHandle<EcmaString> target,
                            CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<EcmaString> GenerateVarNameAndPath(JSThread *thread, JSHandle<EcmaString> importPath,
                                                       JSHandle<EcmaString> fileName,
                                                       CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TSFunctionType> ParseFunctionType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSArrayType> ParseArrayType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSObjectType> ParseObjectType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static int GetTypeKindFromFileByLocalId(JSThread *thread, const JSPandaFile *jsPandaFile, int localId);

    static void LinkClassType(JSThread *thread, JSHandle<TSTypeTable> table);

    static void MergeClassFiled(JSThread *thread, JSHandle<TSClassType> classType,
                                JSHandle<TSClassType> extendClassType);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H
