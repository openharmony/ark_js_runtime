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

#ifndef ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_INL_H_
#define ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_INL_H_

#include "ts_obj_layout_info.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
inline int TSObjLayoutInfo::GetPropertiesCapacity() const
{
    return static_cast<int>((GetLength() - ELEMENTS_START_INDEX) >> 1U);
}

inline void TSObjLayoutInfo::SetNumberOfElements(const JSThread *thread, int propertiesNum)
{
    return TaggedArray::Set(thread, ELEMENTS_START_INDEX - 1, JSTaggedValue(propertiesNum));
}

inline int TSObjLayoutInfo::NumberOfElements() const
{
    return TaggedArray::Get(ELEMENTS_START_INDEX - 1).GetInt();
}

inline uint32_t TSObjLayoutInfo::GetKeyIndex(int index) const
{
    return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) << 1U);
}

inline uint32_t TSObjLayoutInfo::GetTypeIdIndex(int index) const
{
    return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) << 1U) + 1;
}

inline void TSObjLayoutInfo::SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key,
                                             const JSTaggedValue &tsTypeId)
{
    uint32_t idxInArray = GetKeyIndex(index);
    TaggedArray::Set(thread, idxInArray, key);
    TaggedArray::Set(thread, idxInArray + 1, tsTypeId);
}

inline JSTaggedValue TSObjLayoutInfo::GetKey(int index) const
{
    uint32_t idxInArray = GetKeyIndex(index);
    return TaggedArray::Get(idxInArray);
}

inline JSTaggedValue TSObjLayoutInfo::GetTypeId(int index) const
{
    uint32_t idxInArray = GetTypeIdIndex(index);
    return TaggedArray::Get(idxInArray);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_INL_H_