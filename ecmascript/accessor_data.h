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

#ifndef PANDA_RUNTIME_ECMA_ACCESSOR_DATA_H
#define PANDA_RUNTIME_ECMA_ACCESSOR_DATA_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/record.h"

namespace panda::ecmascript {
class AccessorData final : public Record {
public:
    using InternalGetFunc = JSTaggedValue (*)(JSThread *, const JSHandle<JSObject> &);
    using InternalSetFunc = bool (*)(JSThread *, const JSHandle<JSObject> &, const JSHandle<JSTaggedValue> &, bool);

    static AccessorData *Cast(ObjectHeader *object)
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

class CompletionRecord final : public Record {
public:
    enum : uint8_t { NORMAL = 0U, BREAK, CONTINUE, RETURN, THROW };

    static CompletionRecord *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsCompletionRecord());
        return static_cast<CompletionRecord *>(object);
    }

    bool IsThrow() const
    {
        return JSTaggedValue::SameValue(this->GetType(), JSTaggedValue(static_cast<int32_t>(THROW)));
    }

    static constexpr size_t TYPE_OFFSET = ObjectHeaderSize();
    ACCESSORS(Type, TYPE_OFFSET, VALUE_OFFSET);
    ACCESSORS(Value, VALUE_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(TYPE_OFFSET, SIZE)
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMA_ACCESSOR_DATA_H
