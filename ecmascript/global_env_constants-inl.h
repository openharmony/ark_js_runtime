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

#ifndef RUNTIME_ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H
#define RUNTIME_ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H

#include "ecmascript/global_env_constants.h"
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript {
inline const JSTaggedValue *GlobalEnvConstants::BeginSlot() const
{
    return &constants_[static_cast<int>(ConstantIndex::CONSTATNT_BEGIN)];
}

inline const JSTaggedValue *GlobalEnvConstants::EndSlot() const
{
    return &constants_[static_cast<int>(ConstantIndex::CONSTATNT_END)];
}

inline void GlobalEnvConstants::SetConstant(ConstantIndex index, JSTaggedValue value)
{
    DASSERT_PRINT(index >= ConstantIndex::CONSTATNT_BEGIN && index < ConstantIndex::CONSTATNT_END,
                  "Root Index out of bound");
    constants_[static_cast<int>(index)] = value;
}

template<typename T>
inline void GlobalEnvConstants::SetConstant(ConstantIndex index, JSHandle<T> value)
{
    DASSERT_PRINT(index >= ConstantIndex::CONSTATNT_BEGIN && index < ConstantIndex::CONSTATNT_END,
                  "Root Index out of bound");
    constants_[static_cast<int>(index)] = value.GetTaggedValue();
}

inline uintptr_t GlobalEnvConstants::GetGlobalConstantAddr(ConstantIndex index) const
{
    return ToUintPtr(this) + sizeof(JSTaggedValue) * static_cast<int>(index);
}

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_GET_IMPL(Type, Name, Index, Desc)                               \
    inline const Type GlobalEnvConstants::Get##Name() const                  \
    {                                                                        \
        return constants_[static_cast<int>(ConstantIndex::Index)];           \
    }                                                                        \
    inline const JSHandle<Type> GlobalEnvConstants::GetHandled##Name() const \
    {                                                                        \
        return JSHandle<Type>(reinterpret_cast<uintptr_t>(                   \
            &constants_[static_cast<int>(ConstantIndex::Index)]));           \
    }

    GLOBAL_ENV_CONSTANT_CLASS(DECL_GET_IMPL)  // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET_IMPL)  // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_CONSTANT(DECL_GET_IMPL)  // NOLINT(readability-const-return-type)
    GLOBAL_ENV_CONSTANT_ACCESSOR(DECL_GET_IMPL)  // NOLINT(readability-const-return-type)
#undef DECL_GET_IMPL
// clang-format on
}  // namespace panda::ecmascript
#endif  // RUNTIME_ECMASCRIPT_GLOBAL_ENV_CONSTANTS_INL_H
