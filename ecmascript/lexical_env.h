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

#ifndef PANDA_RUNTIME_ECMASCRIPT_LEXICALENV_H
#define PANDA_RUNTIME_ECMASCRIPT_LEXICALENV_H

#include "ecmascript/js_object.h"

namespace panda::ecmascript {
class LexicalEnv : public TaggedArray {
public:
    static constexpr array_size_t PARENT_ENV_INDEX = 0;
    static constexpr array_size_t RESERVED_ENV_LENGTH = 1;

    static LexicalEnv *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<LexicalEnv *>(object);
    }

    static size_t ComputeSize(uint32_t numSlots)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), numSlots + RESERVED_ENV_LENGTH);
    }

    void SetParentEnv(JSThread *thread, JSTaggedValue value)
    {
        Set(thread, PARENT_ENV_INDEX, value);
    }

    JSTaggedValue GetParentEnv() const
    {
        return Get(PARENT_ENV_INDEX);
    }

    JSTaggedValue GetProperties(uint32_t index) const
    {
        return Get(index + RESERVED_ENV_LENGTH);
    }

    void SetProperties(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index + RESERVED_ENV_LENGTH, value);
    }

    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_LEXICALENV_H
