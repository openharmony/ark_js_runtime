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

#ifndef PANDA_RUNTIME_ECMASCRIPT_SYMBOL_TABLE_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_SYMBOL_TABLE_INL_H

#include "symbol_table.h"
#include "js_symbol.h"
#include "tagged_hash_table-inl.h"
#include "libpandabase/utils/hash.h"

namespace panda::ecmascript {
int SymbolTable::Hash(const JSTaggedValue &obj)
{
    if (obj.IsHeapObject()) {
        if (obj.IsString()) {
            auto *nameString = static_cast<EcmaString *>(obj.GetTaggedObject());
            return static_cast<int>(nameString->GetHashcode());
        }
        return static_cast<int>(JSSymbol::ComputeHash());
    }
    UNREACHABLE();
}

bool SymbolTable::IsMatch(const JSTaggedValue &name, const JSTaggedValue &other)
{
    if (name.IsHole() || name.IsUndefined()) {
        return false;
    }

    auto *nameString = static_cast<EcmaString *>(name.GetTaggedObject());
    auto *otherString = static_cast<EcmaString *>(other.GetTaggedObject());
    return EcmaString::StringsAreEqual(nameString, otherString);
}

bool SymbolTable::ContainsKey(JSThread *thread, const JSTaggedValue &key)
{
    int entry = FindEntry(key);
    return entry != -1;
}

JSTaggedValue SymbolTable::GetSymbol(const JSTaggedValue &key)
{
    int entry = FindEntry(key);
    ASSERT(entry != -1);
    return GetValue(entry);
}

JSTaggedValue SymbolTable::FindSymbol(JSThread *thread, const JSTaggedValue &value)
{
    JSSymbol *symbol = JSSymbol::Cast(value.GetTaggedObject());
    JSTaggedValue des = symbol->GetDescription();
    if (!des.IsUndefined()) {
        if (ContainsKey(thread, des)) {
            return des;
        }
    }
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_SYMBOL_TABLE_INL_H