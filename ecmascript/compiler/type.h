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

#ifndef ECMASCRIPT_COMPILER_TYPE_H
#define ECMASCRIPT_COMPILER_TYPE_H

#include <cstdint>
#include "ecmascript/ts_types/global_ts_type_ref.h"

namespace panda::ecmascript::kungfu {
class GateType {
public:
    static constexpr uint32_t GC_MASK = ~(1 << 30); // 30 : the 30-th bit is unset implies GC-related type
    static constexpr uint32_t NO_GC_MASK = ~(1 << 29); // 29 : the 29-th bit is unset implies NO-GC-related type

    // 31 : the 31-st bit is set implies MIR type
    static constexpr uint32_t MIR_BASE_BITS = (1 << 31) | (1 << 30) | (1 << 29);
    static constexpr uint32_t EMPTY_TYPE_OFFSET = 1; // 1 : means offset of empty type

    static constexpr uint32_t NJS_VALUE = MIR_BASE_BITS;
    static constexpr uint32_t TAGGED_VALUE = MIR_BASE_BITS & GC_MASK & NO_GC_MASK; // (100)
    static constexpr uint32_t TAGGED_POINTER = MIR_BASE_BITS & GC_MASK; // (101)
    static constexpr uint32_t TAGGED_NPOINTER = MIR_BASE_BITS & NO_GC_MASK; // (110)

    static constexpr uint32_t EMPTY = NJS_VALUE + EMPTY_TYPE_OFFSET;

    constexpr explicit GateType(uint32_t type = 0) : type_(type) {}

    explicit GateType(GlobalTSTypeRef gt) : type_(gt.GetType()) {}

    constexpr explicit GateType(TSPrimitiveType primitiveType) : type_(static_cast<uint32_t>(primitiveType)) {}

    ~GateType() = default;

    uint32_t GetType() const
    {
        return type_;
    }

    static inline constexpr GateType NJSValue()
    {
        return GateType(NJS_VALUE);
    }

    static inline constexpr GateType TaggedValue()
    {
        return GateType(TAGGED_VALUE);
    }

    static inline constexpr GateType TaggedPointer()
    {
        return GateType(TAGGED_POINTER);
    }

    static inline constexpr GateType TaggedNPointer()
    {
        return GateType(TAGGED_NPOINTER);
    }

    static inline constexpr GateType Empty()
    {
        return GateType(EMPTY);
    }

    static inline constexpr GateType AnyType()
    {
        return GateType(TSPrimitiveType::ANY);
    }

    static inline constexpr GateType NumberType()
    {
        return GateType(TSPrimitiveType::NUMBER);
    }

    static inline constexpr GateType BooleanType()
    {
        return GateType(TSPrimitiveType::BOOLEAN);
    }

    static inline constexpr GateType VoidType()
    {
        return GateType(TSPrimitiveType::VOID_TYPE);
    }

    static inline constexpr GateType StringType()
    {
        return GateType(TSPrimitiveType::STRING);
    }

    static inline constexpr GateType SymbolType()
    {
        return GateType(TSPrimitiveType::SYMBOL);
    }

    static inline constexpr GateType NullType()
    {
        return GateType(TSPrimitiveType::NULL_TYPE);
    }

    static inline constexpr GateType UndefinedType()
    {
        return GateType(TSPrimitiveType::UNDEFINED);
    }

    static inline constexpr GateType IntType()
    {
        return GateType(TSPrimitiveType::INT);
    }

    static inline constexpr GateType BigIntType()
    {
        return GateType(TSPrimitiveType::BIG_INT);
    }

    bool inline IsTSType() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        // 0: TS type
        return gt.GetFlag() == 0;
    }

    bool inline IsAnyType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::ANY);
    }

    bool inline IsNumberType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::NUMBER);
    }

    bool inline IsBooleanType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::BOOLEAN);
    }

    bool inline IsVoidType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::VOID_TYPE);
    }

    bool inline IsStringType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::STRING);
    }

    bool inline IsSymbolType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::SYMBOL);
    }

    bool inline IsNullType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::NULL_TYPE);
    }

    bool inline IsUndefinedType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::UNDEFINED);
    }

    bool inline IsIntType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::INT);
    }

    bool inline IsBigIntType() const
    {
        return type_ == static_cast<uint32_t>(TSPrimitiveType::BIG_INT);
    }

    bool inline IsPrimitiveTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::PRIMITIVE;
    }

    bool inline IsClassTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::CLASS;
    }

    bool inline IsClassInstanceTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::CLASS_INSTANCE;
    }

    bool inline IsFunctionTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::FUNCTION;
    }

    bool inline IsUnionTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::UNION;
    }

    bool inline IsArrayTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::ARRAY;
    }

    bool inline IsObjectTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::OBJECT;
    }

    bool inline IsImportTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::IMPORT;
    }

    bool inline IsInterfaceTypeKind() const
    {
        GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
        ASSERT(gt.GetFlag() == 0);
        return static_cast<TSTypeKind>(gt.GetKind()) == TSTypeKind::INTERFACE_KIND;
    }

    bool operator ==(const GateType &other) const
    {
        return type_ == other.type_;
    }

    bool operator !=(const GateType &other) const
    {
        return type_ != other.type_;
    }

    bool operator <(const GateType &other) const
    {
        return type_ < other.type_;
    }

    bool operator <=(const GateType &other) const
    {
        return type_ <= other.type_;
    }

    bool operator >(const GateType &other) const
    {
        return type_ > other.type_;
    }

    bool operator >=(const GateType &other) const
    {
        return type_ >= other.type_;
    }

    std::string GetTypeStr() const;

private:
    uint32_t type_ {0};
};

class Type {
public:
    explicit Type(GateType payload);
    [[nodiscard]] bool IsBitset() const;
    ~Type();

private:
    GateType payload;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_TYPE_H
