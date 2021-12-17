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

#ifndef ECMASCRIPT_JSSYMBOL_H
#define ECMASCRIPT_JSSYMBOL_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_object.h"

namespace panda {
namespace ecmascript {
class JSSymbol : public TaggedObject {
public:
    static constexpr uint64_t IS_PRIVATE = 1U << 0U;
    static constexpr uint64_t IS_WELL_KNOWN_SYMBOL = 1U << 1U;
    static constexpr uint64_t IS_IN_PUBLIC_SYMBOL_TABLE = 1U << 2U;
    static constexpr uint64_t IS_INTERESTING_SYMBOL = 1U << 3U;
    static constexpr uint64_t IS_PRIVATE_NAME = 1U << 4U;
    static constexpr uint64_t IS_PRIVATE_BRAND = 1U << 5U;

    static constexpr int SYMBOL_HAS_INSTANCE_TYPE = 0;
    static constexpr int SYMBOL_TO_PRIMITIVE_TYPE = 1;
    static constexpr int SYMBOL_DEFAULT_TYPE = 2;

    static constexpr const uint32_t LINEAR_X = 1103515245U;
    static constexpr const uint32_t LINEAR_Y = 12345U;
    static constexpr const uint32_t LINEAR_SEED = 987654321U;

public:
    CAST_CHECK(JSSymbol, IsSymbol);

    static inline uint32_t ComputeHash()
    {
        uint32_t hashSeed = LINEAR_SEED + std::time(nullptr);
        uint32_t hash = hashSeed * LINEAR_X + LINEAR_Y;
        return hash;
    }

    bool IsPrivate() const
    {
        JSTaggedValue flags = this->GetFlags();
        ASSERT(flags.IsInteger());
        return (flags.GetRawData() & IS_PRIVATE) != 0U;
    }

    void SetPrivate(const JSThread *thread)
    {
        JSTaggedValue flags = this->GetFlags();
        if (flags.IsUndefined()) {
            SetFlags(thread, JSTaggedValue(IS_PRIVATE));
            return;
        }
        SetFlags(thread, JSTaggedValue(flags.GetRawData() | IS_PRIVATE));
    }

    bool IsWellKnownSymbol() const
    {
        JSTaggedValue flags = this->GetFlags();
        ASSERT(flags.IsInteger());
        return (flags.GetRawData() & IS_WELL_KNOWN_SYMBOL) != 0U;
    }

    void SetWellKnownSymbol(const JSThread *thread)
    {
        JSTaggedValue flags = this->GetFlags();
        if (flags.IsUndefined()) {
            SetFlags(thread, JSTaggedValue(static_cast<int>(IS_WELL_KNOWN_SYMBOL)));
            return;
        }
        flags = JSTaggedValue(static_cast<int>(flags.GetRawData() | IS_WELL_KNOWN_SYMBOL));
        SetFlags(thread, flags);
    }

    bool IsInPublicSymbolTable() const
    {
        JSTaggedValue flags = this->GetFlags();
        ASSERT(flags.IsInteger());
        return (flags.GetRawData() & IS_IN_PUBLIC_SYMBOL_TABLE) != 0U;
    }

    void SetInPublicSymbolTable(const JSThread *thread)
    {
        JSTaggedValue flags = this->GetFlags();
        if (flags.IsUndefined()) {
            SetFlags(thread, JSTaggedValue(static_cast<int>(IS_IN_PUBLIC_SYMBOL_TABLE)));
            return;
        }
        flags = JSTaggedValue(static_cast<int>(flags.GetRawData() | IS_IN_PUBLIC_SYMBOL_TABLE));
        SetFlags(thread, flags);
    }

    bool IsInterestingSymbol() const
    {
        JSTaggedValue flags = this->GetFlags();
        ASSERT(flags.IsInteger());
        return (flags.GetRawData() & IS_INTERESTING_SYMBOL) != 0U;
    }

    void SetInterestingSymbol(const JSThread *thread)
    {
        JSTaggedValue flags = this->GetFlags();
        if (flags.IsUndefined()) {
            SetFlags(thread, JSTaggedValue(static_cast<int>(IS_INTERESTING_SYMBOL)));
            return;
        }
        flags = JSTaggedValue(static_cast<int>(flags.GetRawData() | IS_INTERESTING_SYMBOL));
        SetFlags(thread, flags);
    }

    bool IsPrivateNameSymbol() const
    {
        JSTaggedValue flags = this->GetFlags();
        ASSERT(flags.IsInteger());
        return (flags.GetRawData() & IS_PRIVATE_NAME) != 0U;
    }

    void SetPrivateNameSymbol(const JSThread *thread)
    {
        JSTaggedValue flags = this->GetFlags();
        if (flags.IsUndefined()) {
            SetFlags(thread, JSTaggedValue(static_cast<int>(IS_PRIVATE_NAME)));
            return;
        }
        flags = JSTaggedValue(static_cast<int>(flags.GetRawData() | IS_PRIVATE_NAME));
        SetFlags(thread, flags);
    }

    static bool Equal(const JSSymbol &src, const JSSymbol &dst)
    {
        if (src.GetFlags() != dst.GetFlags()) {
            return false;
        }
        EcmaString *srcString = EcmaString::Cast(src.GetDescription().GetTaggedObject());
        EcmaString *dstString = EcmaString::Cast(dst.GetDescription().GetTaggedObject());
        return EcmaString::StringsAreEqual(srcString, dstString);
    }

public:
    static constexpr size_t HASHFIELD_OFFSET = TaggedObjectSize();
    ACCESSORS(HashField, HASHFIELD_OFFSET, FLAGS_OFFSET)
    ACCESSORS(Flags, FLAGS_OFFSET, DESCRIPTION_OFFSET)
    ACCESSORS(Description, DESCRIPTION_OFFSET, SIZE)

    DECL_DUMP()

    DECL_VISIT_OBJECT(HASHFIELD_OFFSET, SIZE)
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_NAME_H
