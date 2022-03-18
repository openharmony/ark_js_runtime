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

#ifndef ECMASCRIPT_TAGGED_ARRAY_H
#define ECMASCRIPT_TAGGED_ARRAY_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class ObjectFactory;
class JSThread;

class TaggedArray : public TaggedObject {
public:
    static constexpr uint32_t MAX_ARRAY_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t MAX_END_UNUSED = 4;

    inline static TaggedArray *Cast(ObjectHeader *obj)
    {
        ASSERT(JSTaggedValue(obj).IsTaggedArray());
        return static_cast<TaggedArray *>(obj);
    }

    JSTaggedValue Get(uint32_t idx) const;

    uint32_t GetIdx(const JSTaggedValue &value) const;

    template<typename T>
    void Set(const JSThread *thread, uint32_t idx, const JSHandle<T> &value);

    JSTaggedValue Get(const JSThread *thread, uint32_t idx) const;

    void Set(const JSThread *thread, uint32_t idx, const JSTaggedValue &value);

    static inline JSHandle<TaggedArray> Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                               const JSHandle<TaggedArray> &second);
    static inline JSHandle<TaggedArray> AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                       const JSHandle<TaggedArray> &second, uint32_t copyLength);

    static inline size_t ComputeSize(size_t elemSize, uint32_t length)
    {
        ASSERT(elemSize != 0);
        size_t size = DATA_OFFSET + elemSize * length;
#ifdef PANDA_TARGET_32
        size_t sizeLimit = (std::numeric_limits<size_t>::max() - DATA_OFFSET) / elemSize;
        if (UNLIKELY(sizeLimit < static_cast<size_t>(length))) {
            return 0;
        }
#endif
        return size;
    }

    inline JSTaggedType *GetData() const
    {
        return reinterpret_cast<JSTaggedType *>(ToUintPtr(this) + DATA_OFFSET);
    }

    inline bool IsDictionaryMode() const;

    bool HasDuplicateEntry() const;

    static JSHandle<TaggedArray> SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                             uint32_t capa);

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t length);

    static inline bool ShouldTrim(JSThread *thread, uint32_t oldLength, uint32_t newLength)
    {
        return (oldLength - newLength > MAX_END_UNUSED);
    }
    inline void Trim(JSThread *thread, uint32_t newLength);

    static constexpr size_t LENGTH_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t DATA_OFFSET = SIZE;  // DATA_OFFSET equal to Empty Array size

    DECL_VISIT_ARRAY(DATA_OFFSET, GetLength());

private:
    friend class ObjectFactory;
};

static_assert(TaggedArray::LENGTH_OFFSET == sizeof(TaggedObject));
static_assert((TaggedArray::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_H
