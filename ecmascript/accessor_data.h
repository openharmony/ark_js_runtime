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

#ifndef ECMASCRIPT_ACCESSOR_DATA_H
#define ECMASCRIPT_ACCESSOR_DATA_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/record.h"

namespace panda::ecmascript {
class AccessorData final : public Record {
public:
    using InternalGetFunc = JSTaggedValue (*)(JSThread *, const JSHandle<JSObject> &);
    using InternalSetFunc = bool (*)(JSThread *, const JSHandle<JSObject> &, const JSHandle<JSTaggedValue> &, bool);

    static AccessorData *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsAccessorData() || JSTaggedValue(object).IsInternalAccessor());
        return static_cast<AccessorData *>(object);
    }

    inline bool IsInternal() const
    {
        return GetClass()->IsInternalAccessor();
    }

    inline bool HasSetter() const
    {
        return !GetSetter().IsUndefined();
    }

    JSTaggedValue CallInternalGet(JSThread *thread, const JSHandle<JSObject> &obj) const
    {
        ASSERT(GetGetter().IsJSNativePointer());
        JSNativePointer *getter = JSNativePointer::Cast(GetGetter().GetTaggedObject());
        auto getFunc = reinterpret_cast<InternalGetFunc>(getter->GetExternalPointer());
        return getFunc(thread, obj);
    }

    bool CallInternalSet(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &value,
                         bool mayThrow = false) const
    {
        ASSERT(GetSetter().IsJSNativePointer());
        JSNativePointer *setter = JSNativePointer::Cast(GetSetter().GetTaggedObject());
        auto setFunc = reinterpret_cast<InternalSetFunc>(setter->GetExternalPointer());
        return setFunc(thread, obj, value, mayThrow);
    }

    static constexpr size_t GETTER_OFFSET = Record::SIZE;
    ACCESSORS(Getter, GETTER_OFFSET, SETTER_OFFSET);
    ACCESSORS(Setter, SETTER_OFFSET, SIZE);

    DECL_DUMP()
    DECL_VISIT_OBJECT(GETTER_OFFSET, SIZE)
};

enum class CompletionRecordType : uint8_t {
    NORMAL = 0U,
    BREAK,
    CONTINUE,
    RETURN,
    THROW
};

class CompletionRecord final : public Record {
public:
    static CompletionRecord *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsCompletionRecord());
        return static_cast<CompletionRecord *>(object);
    }

    bool IsThrow() const
    {
        return GetType() == CompletionRecordType::THROW;
    }

    static constexpr size_t VALUE_OFFSET = Record::SIZE;
    ACCESSORS(Value, VALUE_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t TYPE_BITS = 3;
    FIRST_BIT_FIELD(BitField, Type, CompletionRecordType, TYPE_BITS)

    DECL_DUMP()

    DECL_VISIT_OBJECT(VALUE_OFFSET, BIT_FIELD_OFFSET)
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ACCESSOR_DATA_H
