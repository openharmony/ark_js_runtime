/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H
#define ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H

#include "ecmascript/ecma_macros.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
enum class TSTypeKind : int {
    PRIMITIVE = 0,
    CLASS,
    CLASS_INSTANCE,
    FUNCTION,
    UNION,
    ARRAY,
    OBJECT,
    IMPORT,
    INTERFACE_KIND
};

enum class TSPrimitiveType : int {
    ANY = 0,
    NUMBER,
    BOOLEAN,
    VOID_TYPE,
    STRING,
    SYMBOL,
    NULL_TYPE,
    UNDEFINED,
    INT,
    BIG_INT,
    END
};

class GlobalTSTypeRef {
public:
    explicit GlobalTSTypeRef(uint32_t type = 0) : type_(type) {}
    explicit GlobalTSTypeRef(int moduleId, int localId, int typeKind)
    {
        type_ = 0;
        SetKind(typeKind);
        SetLocalId(localId);
        SetModuleId(moduleId);
    }
    ~GlobalTSTypeRef() = default;

    static constexpr int TS_TYPE_RESERVED_COUNT = 50;
    static constexpr int LOCAL_ID_BITS = 16;
    static constexpr int MODULE_ID_BITS = 7;
    static constexpr int KIND_BITS = 6;
    static constexpr int GC_TYPE_BITS = 2;
    static constexpr int FLAG_BITS = 1;
    FIRST_BIT_FIELD(Type, LocalId, uint16_t, LOCAL_ID_BITS);           // 0 ~ 15
    NEXT_BIT_FIELD(Type, ModuleId, uint8_t, MODULE_ID_BITS, LocalId);  // 16 ~ 22
    NEXT_BIT_FIELD(Type, Kind, uint8_t, KIND_BITS, ModuleId);          // 23 ~ 28
    NEXT_BIT_FIELD(Type, GCType, uint8_t, GC_TYPE_BITS, Kind);         // 29 ~ 30
    NEXT_BIT_FIELD(Type, Flag, bool, FLAG_BITS, GCType);               // 31: 0: TS type, 1: MIR type

    static GlobalTSTypeRef Default()
    {
        return GlobalTSTypeRef(0u);
    }

    uint32_t GetType() const
    {
        return type_;
    }

    void SetType(uint32_t type)
    {
        type_ = type;
    }

    void Clear()
    {
        type_ = 0;
    }

    bool IsBuiltinType() const
    {
        return type_ < TS_TYPE_RESERVED_COUNT;
    }

    bool IsDefault() const
    {
        return type_ == 0;
    }

    bool operator <(const GlobalTSTypeRef &other) const
    {
        return type_ < other.type_;
    }

    bool operator ==(const GlobalTSTypeRef &other) const
    {
        return type_ == other.type_;
    }

    void Dump() const
    {
        uint8_t kind = GetKind();
        uint32_t gcType = GetGCType();
        uint32_t moduleId = GetModuleId();
        uint32_t localId = GetLocalId();
        LOG_ECMA(ERROR) << "kind: " << kind << " gcType: " << gcType
                               << " moduleId: " << moduleId << " localId: " << localId;
    }

private:
     uint32_t type_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H