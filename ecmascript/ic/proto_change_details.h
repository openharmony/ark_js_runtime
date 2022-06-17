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

#ifndef ECMASCRIPT_IC_PROTOTYPE_CHANGE_DETAILS_H
#define ECMASCRIPT_IC_PROTOTYPE_CHANGE_DETAILS_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/weak_vector.h"

namespace panda {
namespace ecmascript {
class ProtoChangeMarker : public TaggedObject {
public:
    static ProtoChangeMarker *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsProtoChangeMarker());
        return static_cast<ProtoChangeMarker *>(object);
    }

    static constexpr size_t BIT_FIELD_OFFSET = TaggedObjectSize();
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t HAS_CHANGED_BITS = 1;
    FIRST_BIT_FIELD(BitField, HasChanged, bool, HAS_CHANGED_BITS);

    DECL_DUMP()
};

class ProtoChangeDetails : public TaggedObject {
public:
    static constexpr int UNREGISTERED = -1;
    static ProtoChangeDetails *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsProtoChangeDetails());
        return static_cast<ProtoChangeDetails *>(object);
    }

    static constexpr size_t CHANGE_LISTENER_OFFSET = TaggedObjectSize();
    ACCESSORS(ChangeListener, CHANGE_LISTENER_OFFSET, REGISTER_INDEX_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(RegisterIndex, uint32_t, REGISTER_INDEX_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(CHANGE_LISTENER_OFFSET, REGISTER_INDEX_OFFSET)
    DECL_DUMP()
};

class ChangeListener : public WeakVector {
public:
    static ChangeListener *Cast(TaggedObject *object)
    {
        return static_cast<ChangeListener *>(object);
    }

    static JSHandle<ChangeListener> Add(const JSThread *thread, const JSHandle<ChangeListener> &array,
                                        const JSHandle<JSHClass> &value, uint32_t *index);

    static uint32_t CheckHole(const JSHandle<ChangeListener> &array);

    JSTaggedValue Get(uint32_t index);
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_IC_PROTOTYPE_CHANGE_DETAILS_H
