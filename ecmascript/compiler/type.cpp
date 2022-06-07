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

#include "ecmascript/compiler/type.h"

namespace panda::ecmascript::kungfu {
Type::Type(GateType payload) : payload(payload) {}

bool Type::IsBitset() const
{
    return (this->payload.GetType() & 1U) == 1;
}

Type::~Type() {}

std::string GateType::GetTypeStr() const
{
    GlobalTSTypeRef gt = GlobalTSTypeRef(GetType());
    ASSERT(gt.GetFlag() == 0);
    if (IsPrimitiveTypeKind()) {
        auto primitive = static_cast<TSPrimitiveType>(gt.GetLocalId());
        switch (primitive) {
            case TSPrimitiveType::ANY:
                return "any";
            case TSPrimitiveType::NUMBER:
                return "number";
            case TSPrimitiveType::BOOLEAN:
                return "boolean";
            case TSPrimitiveType::VOID_TYPE:
                return "void";
            case TSPrimitiveType::STRING:
                return "string";
            case TSPrimitiveType::SYMBOL:
                return "symbol";
            case TSPrimitiveType::NULL_TYPE:
                return "null";
            case TSPrimitiveType::UNDEFINED:
                return "undefined";
            case TSPrimitiveType::INT:
                return "int";
            case TSPrimitiveType::BIG_INT:
                return "big_int";
            default:
                break;
        }
    }
    auto typeKind = static_cast<TSTypeKind>(gt.GetKind());
    switch (typeKind) {
        case TSTypeKind::CLASS:
            return "class";
        case TSTypeKind::CLASS_INSTANCE:
            return "class_instance";
        case TSTypeKind::FUNCTION:
            return "function";
        case TSTypeKind::UNION:
            return "union";
        case TSTypeKind::ARRAY:
            return "array";
        case TSTypeKind::OBJECT:
            return "object";
        case TSTypeKind::IMPORT:
            return "import";
        case TSTypeKind::INTERFACE_KIND:
            return "interface";
        default:
            break;
    }
    return "gatetype:" + std::to_string(gt.GetType());
}
};  // namespace panda::ecmascript::kungfu