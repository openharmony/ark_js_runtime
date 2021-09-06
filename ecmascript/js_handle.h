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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JSHANDLE_H
#define PANDA_RUNTIME_ECMASCRIPT_JSHANDLE_H

#include <type_traits>

#include "ecmascript/ecma_handle_scope-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "handle_base.h"

namespace panda::test {
class JSHandleTest;
}  // namespace panda::test

namespace panda::ecmascript {
class TaggedArray;
class LinkedHashMap;
class LinkedHashSet;
class NameDictionary;

template <typename T>
class JSHandle : public HandleBase {
public:
    inline explicit JSHandle() : HandleBase(reinterpret_cast<uintptr_t>(nullptr)) {}
    ~JSHandle() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(JSHandle);
    DEFAULT_COPY_SEMANTIC(JSHandle);

    explicit JSHandle(const JSThread *thread, JSTaggedValue value) : HandleBase()
    {
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), value.GetRawData());
    }

    explicit JSHandle(const JSThread *thread, const ObjectHeader *value) : HandleBase()
    {
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), JSTaggedValue(value).GetRawData());
    }

    explicit JSHandle(const JSThread *thread, const TaggedObject *value) : HandleBase()
    {
        address_ = EcmaHandleScope::NewHandle(const_cast<JSThread *>(thread), JSTaggedValue(value).GetRawData());
    }

    template <typename S>
    explicit JSHandle(const JSHandle<S> &handle) : HandleBase(handle.GetAddress())
    {
    }

    template <typename S>
    inline static JSHandle<T> Cast(const JSHandle<S> &handle)
    {
        T::Cast(handle.GetTaggedValue().GetTaggedObject());
        return JSHandle<T>(handle.GetAddress());
    }

    inline JSTaggedValue GetTaggedValue() const
    {
        ASSERT(GetAddress() != 0U);
        return *(reinterpret_cast<JSTaggedValue *>(GetAddress()));  // NOLINT(clang-analyzer-core.NullDereference)
    }

    inline JSTaggedType GetTaggedType() const
    {
        ASSERT(GetAddress() != 0U);
        return *reinterpret_cast<JSTaggedType *>(GetAddress());  // NOLINT(clang-analyzer-core.NullDereference)
    }

    inline T *operator*() const
    {
        return T::Cast(GetTaggedValue().GetTaggedObject());
    }

    inline T *operator->() const
    {
        return T::Cast(GetTaggedValue().GetTaggedObject());
    }

    inline bool operator==(const JSHandle<T> &other) const
    {
        return GetTaggedType() == other.GetTaggedType();
    }

    inline bool operator!=(const JSHandle<T> &other) const
    {
        return GetTaggedType() != other.GetTaggedType();
    }

    inline bool IsEmpty() const
    {
        return GetAddress() == 0U;
    }

    template <typename R>
    R *GetObject() const
    {
        return reinterpret_cast<R *>(GetTaggedValue().GetTaggedObject());
    }

    inline explicit JSHandle(uintptr_t slot) : HandleBase(slot)
    {
        if (!std::is_convertible<T *, JSTaggedValue *>::value) {
            T::Cast((*reinterpret_cast<JSTaggedValue *>(slot)).GetTaggedObject());
        }
    }

    void Dump(JSThread *thread) const DUMP_API_ATTR
    {
        GetTaggedValue().Dump(thread);
    }

private:
    inline explicit JSHandle(const JSTaggedType *slot) : HandleBase(reinterpret_cast<uintptr_t>(slot)) {}
    inline explicit JSHandle(const T * const *slot) : HandleBase(reinterpret_cast<uintptr_t>(slot)) {}

    friend class EcmaVM;
    friend class GlobalEnv;
    friend class JSHandleTest;
    friend class GlobalHandleCollection;
};

template <>
inline JSTaggedValue *JSHandle<JSTaggedValue>::operator->() const
{
    return reinterpret_cast<JSTaggedValue *>(GetAddress());
}

template <>
inline JSTaggedValue *JSHandle<JSTaggedValue>::operator*() const
{
    return reinterpret_cast<JSTaggedValue *>(GetAddress());
}

template <>
inline JSTaggedNumber *JSHandle<JSTaggedNumber>::operator->() const
{
    return reinterpret_cast<JSTaggedNumber *>(GetAddress());
}

template <>
inline JSTaggedNumber *JSHandle<JSTaggedNumber>::operator*() const
{
    return reinterpret_cast<JSTaggedNumber *>(GetAddress());
}

template <typename T>
class JSMutableHandle : public JSHandle<T> {
public:
    JSMutableHandle() = default;
    ~JSMutableHandle() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(JSMutableHandle);
    DEFAULT_COPY_SEMANTIC(JSMutableHandle);

    explicit JSMutableHandle(const JSThread *thread, JSTaggedValue value) : JSHandle<T>(thread, value) {}
    explicit JSMutableHandle(const JSThread *thread, const TaggedArray *value) : JSHandle<T>(thread, value) {}

    template <typename S>
    explicit JSMutableHandle(const JSHandle<S> &handle) : JSHandle<T>(handle)
    {
    }

    void Update(JSTaggedValue value)
    {
        auto addr = reinterpret_cast<JSTaggedValue *>(this->GetAddress());
        *addr = value;
    }
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JSHANDLE_H
