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
    enum class TypeLiteralFlag : uint8_t {
        COUNTER = 0,     // just used for the first type literal
        CLASS,
        CLASS_INSTANCE,
        FUNCTION,
        UNION,
        ARRAY,
        OBJECT, // object literal
        IMPORT,
        INTERFACE
    };

    static constexpr std::string_view ENTRY_FUNC_NAME = "func_main_0";
    static constexpr size_t RESERVE_TABLE_LENGTH = 2; // for reserve length and exportTable
    static constexpr size_t TABLE_LENGTH_OFFSET_IN_LITREAL = 1;
    static constexpr size_t TYPE_KIND_OFFSET = 0;
    static constexpr size_t LENGTH_OFFSET = 0;
    static constexpr const char* DECLARED_FILE_NAME = "ohos.lib.d.ts";
    static constexpr const char* DECLARED_SYMBOLS = "declaredSymbols";
    static constexpr const char* DECLARED_SYMBOL_TYPES = "declaredSymbolTypes";
    static constexpr const char* EXPORTED_SYMBOLS = "exportedSymbols";
    static constexpr const char* EXPORTED_SYMBOL_TYPES = "exportedSymbolTypes";

    inline static TSTypeTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSTypeTable *>(object);
    }

    static void Initialize(JSThread *thread, const panda_file::File &pf,
                           CVector<JSHandle<EcmaString>> &recordImportModules);

    static GlobalTSTypeRef GetPropertyTypeGT(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                           uint32_t index, JSHandle<EcmaString> propName);

    static JSHandle<JSTaggedValue> ParseType(JSThread *thread, JSHandle<TSTypeTable> &table,
                                             JSHandle<TaggedArray> &literal,
                                             JSHandle<EcmaString> fileName,
                                             CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TaggedArray> GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable);

    void SetTypeTableLength(JSThread *thread, uint32_t length)
    {
        TaggedArray::Set(thread, LENGTH_OFFSET, JSTaggedValue(length));
    }

    static int GetUserdefinedTypeId(int localId)
    {
        return localId - GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT;
    }

    void SetExportValueTable(JSThread *thread, const panda_file::File &pf);

private:

    static JSHandle<TSTypeTable> GenerateTypeTable(JSThread *thread, const panda_file::File &pf,
                                                    CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TaggedArray> GetExportTableFromPandFile(JSThread *thread, const panda_file::File &pf);

    static panda_file::File::EntityId GetFileId(const panda_file::File &pf);

    static const panda_file::File* GetPandaFile(JSHandle<EcmaString> modulePath);

    static JSHandle<TSClassType> ParseClassType(JSThread *thread, JSHandle<TSTypeTable> &typeTable,
                                                const JSHandle<TaggedArray> &literal);

    static JSHandle<TSInterfaceType> ParseInterfaceType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSImportType> ParseImportType(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                  JSHandle<EcmaString> fileName,
                                                  CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TSUnionType> ParseUnionType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSObjectType> LinkSuper(JSThread *thread, JSHandle<TSClassType> &baseClassType,
                                            uint32_t *numBaseFields, uint32_t numDerivedFields);

    static void CheckModule(JSThread *thread, const TSLoader* tsLoader, const JSHandle<EcmaString> target,
                             CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<EcmaString> GenerateVarNameAndPath(JSThread *thread, JSHandle<EcmaString> importPath,
                                                       JSHandle<EcmaString> fileName,
                                                       CVector<JSHandle<EcmaString>> &recordImportModules);

    static JSHandle<TSFunctionType> ParseFunctionType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSArrayType> ParseArrayType(JSThread *thread, const JSHandle<TaggedArray> &literal);

    static JSHandle<TSObjectType> ParseObjectType(JSThread *thread, const JSHandle<TaggedArray> &literal);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H
