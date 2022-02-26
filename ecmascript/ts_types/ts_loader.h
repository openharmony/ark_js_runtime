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

#ifndef ECMASCRIPT_TS_TYPES_TS_LOADER_H
#define ECMASCRIPT_TS_TYPES_TS_LOADER_H

#include "ecmascript/mem/c_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class GlobalTSTypeRef {
public:
    GlobalTSTypeRef(uint64_t num = 0) : ref_(num) {};
    GlobalTSTypeRef(int moduleId, int localId, int typeKind)
    {
        ref_ = 0;
        SetUserDefineTypeKind(typeKind);
        SetLocalId(localId);
        SetModuleId(moduleId);
    }
    ~GlobalTSTypeRef() = default;

    static constexpr int TS_TYPE_RESERVED_COUNT = 50;
    static constexpr int GLOBAL_TSTYPE_REF_SIZE = 64;
    static constexpr int USER_DEFINE_TYPE_KIND_FIELD_NUM = 6;
    static constexpr int GC_TYPE_FIELD_NUM = 3;
    static constexpr int LOCAL_TSTYPETABLE_INDEX_FIELD_NUM = 32;
    static constexpr int GLOBAL_MODULE_ID_FIELD_NUM = 23;
    FIRST_BIT_FIELD(GlobalTSTypeRef, UserDefineTypeKind, uint8_t, USER_DEFINE_TYPE_KIND_FIELD_NUM);
    NEXT_BIT_FIELD(GlobalTSTypeRef, GCType, uint32_t, GC_TYPE_FIELD_NUM, UserDefineTypeKind);
    NEXT_BIT_FIELD(GlobalTSTypeRef, LocalId, uint32_t, LOCAL_TSTYPETABLE_INDEX_FIELD_NUM, GCType);
    NEXT_BIT_FIELD(GlobalTSTypeRef, ModuleId, uint32_t, GLOBAL_MODULE_ID_FIELD_NUM, LocalId);

    static GlobalTSTypeRef Default()
    {
        return GlobalTSTypeRef(0u);
    }

    uint64_t GetGlobalTSTypeRef() const
    {
        return ref_;
    }

    void SetGlobalTSTypeRef(uint64_t gt)
    {
        ref_ = gt;
    }

    void Clear()
    {
        ref_ = 0;
    }
private:
    uint64_t ref_;
};

enum class TSTypeKind : int {
    TS_ANY = 0,
    TS_NUMBER,
    TS_BOOLEAN,
    TS_VOID,
    TS_STRING,
    TS_SYMBOL,
    TS_NULL,
    TS_UNDEFINED,
    TS_INT,
    TS_TYPE_RESERVED_LENGTH = GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT, // include builtins type and MR type
    TS_CLASS,
    TS_CLASS_INSTANCE,
    TS_FUNCTION,
    TS_UNION,
    TS_ARRAY,
    TS_OBJECT,
    TS_IMPORT,
    TS_INTERFACE
};

class TSModuleTable : public TaggedArray {
public:

    static constexpr int AMI_PATH_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    static constexpr int ELEMENT_OFFSET = 3;
    static constexpr int NUMBER_ELEMENTS_OFFSET = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    static constexpr int DEAULT_TABLE_CAPACITY = 4;
    static constexpr int INITIA_TSTYPE_TABLE_NUMBER = 0;
    static constexpr int NOT_FOUND = -1;

    static TSModuleTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSModuleTable *>(object);
    }

    static JSHandle<TSModuleTable> AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    JSHandle<EcmaString> GetAmiPathByModuleId(JSThread *thread, int entry) const;

    JSHandle<TSTypeTable> GetTSTypeTable(JSThread *thread, int entry) const;

    int GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const;

    inline int GetNumberOfTSTypeTable() const
    {
        return Get(NUMBER_ELEMENTS_OFFSET).GetInt();;
    }

    static uint32_t GetTSTypeTableOffset(int entry)
    {
        return entry * ELEMENT_OFFSET + TYPE_TABLE_OFFSET;
    }

    void InitializeNumberOfTSTypeTable(JSThread *thread)
    {
        Set(thread, NUMBER_ELEMENTS_OFFSET, JSTaggedValue(INITIA_TSTYPE_TABLE_NUMBER));
    }
private:

    static int GetAmiPathOffset(int entry)
    {
        return entry * ELEMENT_OFFSET + AMI_PATH_OFFSET;
    }

    static int GetSortIdOffset(int entry)
    {
        return entry * ELEMENT_OFFSET + SORT_ID_OFFSET;
    }
};

class TSLoader {
public:
    explicit TSLoader(EcmaVM *vm);
    ~TSLoader() = default;

    void DecodeTSTypes(const panda_file::File &pf);

    void AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    GlobalTSTypeRef PUBLIC_API GetGTFromPandFile(const panda_file::File &pf, int localId) const;

    void Link();

    void Dump();

    inline static TSTypeKind PUBLIC_API GetTypeKind(GlobalTSTypeRef gt)
    {
        return static_cast<TSTypeKind>(gt.GetUserDefineTypeKind());
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const;

    int PUBLIC_API GetUnionTypeLength(GlobalTSTypeRef gt) const;

    bool IsPrimtiveBuiltinTypes(int localId) const;

    int GetUTableIndex(GlobalTSTypeRef gt, uint32_t index) const;

    GlobalTSTypeRef PUBLIC_API GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeRef, int size);

    void Iterate(const RootVisitor &v);

    GlobalTSTypeRef PUBLIC_API GetPrmitiveGT(TSTypeKind kind) const;

    GlobalTSTypeRef PUBLIC_API GetImportTypeTargetGT(GlobalTSTypeRef gt) const;

    JSHandle<TSModuleTable> GetTSModuleTable() const
    {
        return JSHandle<TSModuleTable>(reinterpret_cast<uintptr_t>(&globalModuleTable_));
    }

    void SetTSModuleTable(JSHandle<TSModuleTable> table)
    {
        globalModuleTable_ = table.GetTaggedValue();
    }

    int GetNextModuleId() const
    {
        JSHandle<TSModuleTable> table = GetTSModuleTable();
        return table->GetNumberOfTSTypeTable();
    }

    JSHandle<EcmaString> GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel) const;

    JSHandle<EcmaString> GenerateImportVar(JSHandle<EcmaString> import) const;

    JSHandle<EcmaString> GenerateImportRelativePath(JSHandle<EcmaString> importRel) const;

private:

    NO_COPY_SEMANTIC(TSLoader);
    NO_MOVE_SEMANTIC(TSLoader);

    void LinkTSTypeTable(JSHandle<TSTypeTable> table);

    void RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType);

    int GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const;

    JSHandle<JSTaggedValue> InitUnionTypeTable();

    GlobalTSTypeRef CreateGT(int moduleId, int localId, int typeKind) const;

    GlobalTSTypeRef AddUinonTypeToGlobalUnionTable(JSHandle<TSUnionType> unionType);

    GlobalTSTypeRef FindInGlobalUTable(JSHandle<TSUnionType> unionType) const;

    JSHandle<TaggedArray> GetGlobalUTable() const;

    EcmaVM *vm_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    friend class EcmaVM;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_LOADER_H
