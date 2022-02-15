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

#ifndef ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H
#define ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H

#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
// TSObjLayoutInfo contains keys and TsTypeId of Properties.
class TSObjLayoutInfo : private TaggedArray {
public:
    static constexpr int MIN_PROPERTIES_LENGTH = JSObject::MIN_PROPERTIES_LENGTH;
    static constexpr int MAX_PROPERTIES_LENGTH = PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES;
    static constexpr int NUMBER_OF_PROPERTIES_INDEX = 0;
    static constexpr int ELEMENTS_START_INDEX = 1;

    inline static TSObjLayoutInfo *Cast(ObjectHeader *obj)
    {
        ASSERT(JSTaggedValue(obj).IsTaggedArray());
        return reinterpret_cast<TSObjLayoutInfo*>(obj);
    }

    int GetPropertiesCapacity() const;
    void SetNumberOfElements(const JSThread *thread, int propertiesNum);
    int NumberOfElements() const;
    uint32_t GetKeyIndex(int index) const;
    uint32_t GetTypeIdIndex(int index) const;
    void SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key, const JSTaggedValue &tsTypeId);
    JSTaggedValue GetKey(int index) const;
    JSTaggedValue GetTypeId(int index) const;
    void SetKey(const JSThread *thread, int index, const JSTaggedValue &key, const JSTaggedValue &typeId);

    inline uint32_t GetLength() const
    {
        return TaggedArray::GetLength();
    }

    static inline uint32_t ComputeArrayLength(uint32_t properties_number)
    {
        return (properties_number << 1U) + ELEMENTS_START_INDEX;
    }

    static inline uint32_t ComputeGrowCapacity(uint32_t old_capacity)
    {
        uint32_t new_capacity = old_capacity * 2  + ELEMENTS_START_INDEX;
        return new_capacity > MAX_PROPERTIES_LENGTH ? MAX_PROPERTIES_LENGTH : new_capacity;
    }

    int FindElement(JSTaggedValue key, int propertiesNumber);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H
