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

#ifndef ECMASCRIPT_BASE_ALIGNED_STRUCT_H
#define ECMASCRIPT_BASE_ALIGNED_STRUCT_H

#include "libpandabase/utils/bit_utils.h"

namespace panda::ecmascript::base {
template<typename...>
struct TypeList {};

template<typename T>
struct TypeList<T> {
    using Head = T;
    using Next = TypeList<>;
    static constexpr size_t Size = 1;
};

template<typename T, typename... Res>
struct TypeList<T, Res...> {
    using Head = T;
    using Next = TypeList<Res...>;
    static constexpr size_t Size = 1 + TypeList<Res...>::Size;
};

template<size_t ElementAlign, typename... Ts>
struct AlignedStruct {
    static constexpr size_t EAS = ElementAlign;

    using ElemTypeList = TypeList<Ts...>;
    static constexpr size_t NumOfTypes = ElemTypeList::Size;

    template<size_t, typename>
    struct OffsetAt;

    template<>
    struct OffsetAt<0, TypeList<>> {
        static constexpr size_t OFFSET64 = 0;
        static constexpr size_t OFFSET32 = 0;
    };

    template<typename Head, typename... Res>
    struct OffsetAt<0, TypeList<Head, Res...>> {
        static constexpr size_t OFFSET64 = 0;
        static constexpr size_t OFFSET32 = 0;
    };

    template<size_t index, typename Head, typename... Res>
    struct OffsetAt<index, TypeList<Head, Res...>> {
        static_assert(std::is_class<Head>::value);
        static constexpr size_t OFFSET64 =
            RoundUp(Head::SizeArch64, EAS) + OffsetAt<index - 1, TypeList<Res...>>::OFFSET64;

        static constexpr size_t OFFSET32 =
            RoundUp(Head::SizeArch32, EAS) + OffsetAt<index - 1, TypeList<Res...>>::OFFSET32;
    };

    template<size_t index>
    static size_t GetOffset(bool isArch32)
    {
        return isArch32 ? OffsetAt<index, ElemTypeList>::OFFSET32
                        : OffsetAt<index, ElemTypeList>::OFFSET64;
    };

    static constexpr size_t SizeArch32 = OffsetAt<NumOfTypes, ElemTypeList>::OFFSET32;
    static constexpr size_t SizeArch64 = OffsetAt<NumOfTypes, ElemTypeList>::OFFSET64;
};

struct AlignedPointer {
    static constexpr size_t SizeArch32 = sizeof(uint32_t);
    static constexpr size_t SizeArch64 = sizeof(uint64_t);
};
}
#endif // ECMASCRIPT_BASE_ALIGNED_STRUCT_H
