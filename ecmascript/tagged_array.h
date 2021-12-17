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
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class ObjectFactory;

class TaggedArray : public TaggedObject {
public:
    static constexpr array_size_t MAX_ARRAY_INDEX = std::numeric_limits<array_size_t>::max();
    static constexpr array_size_t MAX_END_UNUSED = 4;

    inline static TaggedArray *Cast(ObjectHeader *obj)
    {
        ASSERT(JSTaggedValue(obj).IsTaggedArray());
        return static_cast<TaggedArray *>(obj);
    }

    array_size_t GetLength() const;

    JSTaggedValue Get(array_size_t idx) const;

    array_size_t GetIdx(const JSTaggedValue &value) const;

    template<typename T>
    void Set(const JSThread *thread, array_size_t idx, const JSHandle<T> &value);

    JSTaggedValue Get(const JSThread *thread, array_size_t idx) const;

    void Set(const JSThread *thread, array_size_t idx, const JSTaggedValue &value);

    static inline JSHandle<TaggedArray> Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                               const JSHandle<TaggedArray> &second);
    static inline JSHandle<TaggedArray> AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                       const JSHandle<TaggedArray> &second, array_size_t copyLength);

    static inline size_t ComputeSize(size_t elemSize, array_size_t length)
    {
        ASSERT(elemSize != 0);
        size_t size = sizeof(TaggedArray) + elemSize * length;
#ifdef PANDA_TARGET_32
        size_t sizeLimit = (std::numeric_limits<size_t>::max() - sizeof(TaggedArray)) / elemSize;
        if (UNLIKELY(sizeLimit < static_cast<size_t>(length))) {
            return 0;
        }
#endif
        return size;
    }

    JSTaggedType *GetData()
    {
        return data_;
    }

    const JSTaggedType *GetData() const
    {
        return data_;
    }

    inline bool IsDictionaryMode() const;

    bool HasDuplicateEntry() const;

    static JSHandle<TaggedArray> SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                             array_size_t capa);

    static constexpr uint32_t GetLengthOffset()
    {
        return MEMBER_OFFSET(TaggedArray, length_);
    }

    static constexpr uint32_t GetDataOffset()
    {
        return MEMBER_OFFSET(TaggedArray, data_);
    }

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, array_size_t length);

    static inline bool ShouldTrim(JSThread *thread, array_size_t oldLength, array_size_t newLength)
    {
        return (oldLength - newLength > MAX_END_UNUSED) && thread->IsNotBeginMark();
    }
    inline void Trim(JSThread *thread, array_size_t newLength);
    void VisitRangeSlot(const EcmaObjectRangeVisitor &v)
    {
        uintptr_t dataAddr = ToUintPtr(this) + TaggedArray::GetDataOffset();
        v(this, ObjectSlot(dataAddr), ObjectSlot(dataAddr + GetLength() * JSTaggedValue::TaggedTypeSize()));
    }

private:
    friend class ObjectFactory;

    void SetLength(array_size_t length)
    {
        length_ = length;
    }

    array_size_t length_;
    uint32_t placeholder_;  // Add for 8bits align of data_
    // should align by 8, 32bit using TaggedType also
    __extension__ alignas(sizeof(JSTaggedType)) JSTaggedType data_[0];  // NOLINT(modernize-avoid-c-arrays)
};

static_assert(TaggedArray::GetLengthOffset() == sizeof(TaggedObject));
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_H
