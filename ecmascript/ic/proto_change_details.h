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

    using HasChangedField = BitField<bool, 0, 1>;
    static ProtoChangeMarker *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsProtoChangeMarker());
        return static_cast<ProtoChangeMarker *>(object);
    }

    static constexpr size_t HAS_CHANGED_OFFSET = TaggedObjectSize();
    SET_GET_PRIMITIVE_FIELD(HasChanged, bool, HAS_CHANGED_OFFSET, SIZE);
    DECL_DUMP()
};

class ProtoChangeDetails : public TaggedObject {
public:
    static constexpr int UNREGISTERED = -1;
    static ProtoChangeDetails *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsProtoChangeDetails());
        return static_cast<ProtoChangeDetails *>(object);
    }

    static constexpr size_t CHANGE_LISTENER_OFFSET = TaggedObjectSize();
    ACCESSORS(ChangeListener, CHANGE_LISTENER_OFFSET, REGISTER_INDEX_OFFSET);
    ACCESSORS(RegisterIndex, REGISTER_INDEX_OFFSET, SIZE);

    DECL_VISIT_OBJECT(CHANGE_LISTENER_OFFSET, SIZE)
    DECL_DUMP()
};

class ChangeListener : public WeakVector {
public:
    static ChangeListener *Cast(ObjectHeader *object)
    {
        return static_cast<ChangeListener *>(object);
    }

    static JSHandle<ChangeListener> Add(const JSThread *thread, const JSHandle<ChangeListener> &array,
                                        const JSHandle<JSHClass> &value, array_size_t *index);

    static array_size_t CheckHole(const JSHandle<ChangeListener> &array);

    JSTaggedValue Get(array_size_t index);
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_IC_PROTOTYPE_CHANGE_DETAILS_H
