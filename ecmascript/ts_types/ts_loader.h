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
#include "ecmascript/ts_types/global_ts_type_ref.h"

namespace panda::ecmascript {
class TSModuleTable : public TaggedArray {
public:

    static constexpr int AMI_PATH_OFFSET = 1;
    static constexpr int SORT_ID_OFFSET = 2;
    static constexpr int TYPE_TABLE_OFFSET = 3;
    static constexpr int ELEMENTS_LENGTH = 3;
    static constexpr int NUMBER_OF_TABLES_INDEX = 0;
    static constexpr int INCREASE_CAPACITY_RATE = 2;
    static constexpr int DEFAULT_NUMBER_OF_TABLES = 2;  // builtins table and global union table
    // first +1 means reserve a table from pandafile, second +1 menas the NUMBER_OF_TABLES_INDEX
    static constexpr int DEFAULT_TABLE_CAPACITY =  (DEFAULT_NUMBER_OF_TABLES + 1) * ELEMENTS_LENGTH + 1;
    static constexpr int BUILTINS_TABLE_ID = 0;
    static constexpr int INFER_TABLE_ID = 1;
    static constexpr int NOT_FOUND = -1;

    static TSModuleTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSModuleTable *>(object);
    }

    static void Initialize(JSThread *thread, JSHandle<TSModuleTable> mTable);

    static JSHandle<TSModuleTable> AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    JSHandle<EcmaString> GetAmiPathByModuleId(JSThread *thread, int entry) const;

    JSHandle<TSTypeTable> GetTSTypeTable(JSThread *thread, int entry) const;

    int GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const;

    inline int GetNumberOfTSTypeTables() const
    {
        return Get(NUMBER_OF_TABLES_INDEX).GetInt();
    }

    inline void SetNumberOfTSTypeTables(JSThread *thread, int num)
    {
        Set(thread, NUMBER_OF_TABLES_INDEX, JSTaggedValue(num));
    }

    static uint32_t GetTSTypeTableOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + TYPE_TABLE_OFFSET;
    }

private:

    static int GetAmiPathOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + AMI_PATH_OFFSET;
    }

    static int GetSortIdOffset(int entry)
    {
        return entry * ELEMENTS_LENGTH + SORT_ID_OFFSET;
    }
};

class TSLoader {
public:
    explicit TSLoader(EcmaVM *vm);
    ~TSLoader() = default;

    void PUBLIC_API DecodeTSTypes(const JSPandaFile *jsPandaFile);

    void AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath);

    void Link();

    void Dump();

    void Iterate(const RootVisitor &v);

    JSHandle<TSModuleTable> GetTSModuleTable() const
    {
        return JSHandle<TSModuleTable>(reinterpret_cast<uintptr_t>(&globalModuleTable_));
    }

    CVector<JSTaggedType> GetConstStringTable() const
    {
        return constantStringTable_;
    }

    void ClearConstStringTable()
    {
        constantStringTable_.clear();
    }

    void SetTSModuleTable(JSHandle<TSModuleTable> table)
    {
        globalModuleTable_ = table.GetTaggedValue();
    }

    int GetNextModuleId() const
    {
        JSHandle<TSModuleTable> table = GetTSModuleTable();
        return table->GetNumberOfTSTypeTables();
    }

    inline static TSTypeKind PUBLIC_API GetTypeKind(kungfu::GateType gateType)
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetTypeKind(gt);
    }

    inline static TSTypeKind GetTypeKind(GlobalTSTypeRef gt)
    {
        return static_cast<TSTypeKind>(gt.GetKind());
    }

    JSHandle<EcmaString> GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel) const;

    JSHandle<EcmaString> GenerateImportVar(JSHandle<EcmaString> import) const;

    JSHandle<EcmaString> GenerateImportRelativePath(JSHandle<EcmaString> importRel) const;

    GlobalTSTypeRef PUBLIC_API GetGTFromPandaFile(const panda_file::File &pf, uint32_t vregId,
                                                 const JSMethod* method) const;

    static GlobalTSTypeRef PUBLIC_API GetPrimitiveGT(TSPrimitiveType type)
    {
        return GlobalTSTypeRef(static_cast<uint32_t>(type));
    }

    static GlobalTSTypeRef PUBLIC_API GetBuiltinsGT(int type)
    {
        return GlobalTSTypeRef(type);  // not implement yet.
    }

    GlobalTSTypeRef PUBLIC_API GetImportTypeTargetGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, JSHandle<EcmaString> propertyName) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetPropType(gt, propertyName);
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const;

    // use for object
    inline GlobalTSTypeRef PUBLIC_API GetPropType(kungfu::GateType gateType, const uint64_t key) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetPropType(gt, key);
    }

    GlobalTSTypeRef PUBLIC_API GetPropType(GlobalTSTypeRef gt, const uint64_t key) const;

    uint32_t PUBLIC_API GetUnionTypeLength(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const;

    GlobalTSTypeRef PUBLIC_API GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec);

    int PUBLIC_API GetFuncParametersNum(GlobalTSTypeRef gt) const;

    GlobalTSTypeRef PUBLIC_API GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const;

    inline GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetFuncReturnValueTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const;

    inline GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(kungfu::GateType gateType) const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
        return GetArrayParameterTypeGT(gt);
    }

    GlobalTSTypeRef PUBLIC_API GetArrayParameterTypeGT(GlobalTSTypeRef gt) const;

    size_t PUBLIC_API AddConstString(JSTaggedValue string);

    bool PUBLIC_API GetTypeInferenceLog() const;

    // add string to constantstringtable and get its index
    size_t PUBLIC_API GetStringIdx(JSHandle<JSTaggedValue> constPool, const uint16_t id);

    /*
     * Before using this method for type infer, you need to wait until all the
     * string objects of the panda_file are collected, otherwise an error will
     * be generated, and it will be optimized later.
     */
    JSHandle<EcmaString> PUBLIC_API GetStringById(size_t index) const
    {
        ASSERT(index < constantStringTable_.size());
        return JSHandle<EcmaString>(reinterpret_cast<uintptr_t>(&(constantStringTable_.at(index))));
    }

private:

    NO_COPY_SEMANTIC(TSLoader);
    NO_MOVE_SEMANTIC(TSLoader);

    void LinkTSTypeTable(JSHandle<TSTypeTable> table);

    void RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType);

    int GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const;

    GlobalTSTypeRef PUBLIC_API AddUnionToInferTable(JSHandle<TSUnionType> unionType);

    GlobalTSTypeRef FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const;

    JSHandle<TSTypeTable> GetInferTypeTable() const;

    void SetInferTypeTable(JSHandle<TSTypeTable> inferTable);

    EcmaVM *vm_ {nullptr};
    JSTaggedValue globalModuleTable_ {JSTaggedValue::Hole()};
    CVector<JSTaggedType> constantStringTable_ {};
    friend class EcmaVM;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_LOADER_H
