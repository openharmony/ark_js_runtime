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

#ifndef PANDA_RUNTIME_ECMASCRIPT_CODE_H
#define PANDA_RUNTIME_ECMASCRIPT_CODE_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda {
namespace ecmascript {
class Code : public TaggedObject {
public:
    Code();
    ~Code();
    Code(const Code &) = delete;
    Code(Code &&) = delete;
    Code &operator=(const Code &) = delete;
    Code &operator=(Code &&) = delete;
    static Code *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsCode());
        return static_cast<Code *>(object);
    }
    static constexpr size_t LENGTH_OFFSET = TaggedObject::ObjectHeaderSize();

    ACCESSORS(length, LENGTH_OFFSET, DATA_OFFSET);
    ACCESSORS(data, DATA_OFFSET, SIZE);
};
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_ECMASCRIPT_CODE_H
