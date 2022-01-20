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

#ifndef ECMASCRIPT_IC_PROFILE_TYPE_INFO_H
#define ECMASCRIPT_IC_PROFILE_TYPE_INFO_H

#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum class ICKind : uint32_t {
    NamedLoadIC,
    NamedStoreIC,
    LoadIC,
    StoreIC,
    NamedGlobalLoadIC,
    NamedGlobalStoreIC,
    GlobalLoadIC,
    GlobalStoreIC,
};

static inline bool IsNamedGlobalIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalLoadIC) || (kind == ICKind::NamedGlobalStoreIC);
}

static inline bool IsValueGlobalIC(ICKind kind)
{
    return (kind == ICKind::GlobalLoadIC) || (kind == ICKind::GlobalStoreIC);
}

static inline bool IsValueNormalIC(ICKind kind)
{
    return (kind == ICKind::LoadIC) || (kind == ICKind::StoreIC);
}

static inline bool IsValueIC(ICKind kind)
{
    return IsValueNormalIC(kind) || IsValueGlobalIC(kind);
}

static inline bool IsNamedNormalIC(ICKind kind)
{
    return (kind == ICKind::NamedLoadIC) || (kind == ICKind::NamedStoreIC);
}

static inline bool IsNamedIC(ICKind kind)
{
    return IsNamedNormalIC(kind) || IsNamedGlobalIC(kind);
}

static inline bool IsGlobalLoadIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalLoadIC) || (kind == ICKind::GlobalLoadIC);
}

static inline bool IsGlobalStoreIC(ICKind kind)
{
    return (kind == ICKind::NamedGlobalStoreIC) || (kind == ICKind::GlobalStoreIC);
}

static inline bool IsGlobalIC(ICKind kind)
{
    return IsValueGlobalIC(kind) || IsNamedGlobalIC(kind);
}

std::string ICKindToString(ICKind kind);

class ProfileTypeInfo : public TaggedArray {
public:
    static const array_size_t MAX_FUNC_CACHE_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t INVALID_SLOT_INDEX = 0xFF;

    static ProfileTypeInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<ProfileTypeInfo *>(object);
    }
};


class ProfileTypeAccessor {
public:
    static constexpr size_t CACHE_MAX_LEN = 8;
    static constexpr size_t MONO_CASE_NUM = 2;
    static constexpr size_t POLY_CASE_NUM = 4;

    enum ICState {
        UNINIT,
        MONO,
        POLY,
        MEGA,
    };

    ProfileTypeAccessor(JSThread* thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : thread_(thread), profileTypeInfo_(profileTypeInfo), slotId_(slotId), kind_(kind)
    {
    }
    ~ProfileTypeAccessor() = default;

    ICState GetICState() const;
    static std::string ICStateToString(ICState state);
    void AddHandlerWithoutKey(JSHandle<JSTaggedValue> dynclass, JSHandle<JSTaggedValue> handler) const;
    void AddElementHandler(JSHandle<JSTaggedValue> dynclass, JSHandle<JSTaggedValue> handler) const;
    void AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> dynclass,
                           JSHandle<JSTaggedValue> handler) const;
    void AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const;
    void AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const;

    JSTaggedValue GetWeakRef(JSTaggedValue value) const
    {
        return JSTaggedValue(value.CreateAndGetWeakRef());
    }

    JSTaggedValue GetRefFromWeak(const JSTaggedValue &value) const
    {
        return JSTaggedValue(value.GetWeakReferent());
    }
    void SetAsMega() const;

    ICKind GetKind() const
    {
        return kind_;
    }

private:
    JSThread* thread_;
    JSHandle<ProfileTypeInfo> profileTypeInfo_;
    uint32_t slotId_;
    ICKind kind_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_PROFILE_TYPE_INFO_H
