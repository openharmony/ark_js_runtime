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

#ifndef ECMASCRIPT_STRING_TABLE_H
#define ECMASCRIPT_STRING_TABLE_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/object_xray.h"

namespace panda::ecmascript {
class EcmaString;
class EcmaVM;

class EcmaStringTable {
public:
    explicit EcmaStringTable(const EcmaVM *vm);
    virtual ~EcmaStringTable()
    {
        table_.clear();
    }

    void InternEmptyString(EcmaString *emptyStr);
    EcmaString *GetOrInternString(const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);
    EcmaString *GetOrInternString(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    EcmaString *GetOrInternString(EcmaString *string);

    void SweepWeakReference(const WeakRootVisitor &visitor);

private:
    NO_COPY_SEMANTIC(EcmaStringTable);
    NO_MOVE_SEMANTIC(EcmaStringTable);

    EcmaString *GetString(const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString) const;
    EcmaString *GetString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const;
    EcmaString *GetString(const uint16_t *utf16Data, uint32_t utf16Len) const;
    EcmaString *GetString(EcmaString *string) const;

    void InternString(EcmaString *string);

    void InsertStringIfNotExist(EcmaString *string)
    {
        EcmaString *str = GetString(string);
        if (str == nullptr) {
            InternString(string);
        }
    }

    CUnorderedMultiMap<uint32_t, EcmaString *> table_;
    const EcmaVM *vm_{nullptr};
    friend class SnapShotSerialize;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_STRING_TABLE_H
