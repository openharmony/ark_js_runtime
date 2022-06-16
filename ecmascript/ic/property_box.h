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

#ifndef ECMASCRIPT_IC_PROPERTY_BOX_H
#define ECMASCRIPT_IC_PROPERTY_BOX_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/property_attributes.h"

namespace panda {
namespace ecmascript {
class PropertyBox : public TaggedObject {
public:
    static PropertyBox *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsPropertyBox());
        return static_cast<PropertyBox *>(object);
    }

    void Clear(const JSThread *thread);

    inline bool IsInvalid() const
    {
        return GetValue().IsHole();
    }

    static constexpr size_t VALUE_OFFSET = TaggedObjectSize();
    ACCESSORS(Value, VALUE_OFFSET, SIZE);

    DECL_VISIT_OBJECT(VALUE_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda

#endif