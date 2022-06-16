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

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
void ProfileTypeAccessor::AddElementHandler(JSHandle<JSTaggedValue> dynclass, JSHandle<JSTaggedValue> handler) const
{
    auto profileData = profileTypeInfo_->Get(slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        profileTypeInfo_->Set(thread_, index, GetWeakRef(dynclass.GetTaggedValue()));
        profileTypeInfo_->Set(thread_, index + 1, handler.GetTaggedValue());
        return;
    }
    // clear key ic
    if (!profileData.IsWeak() && (profileData.IsString() || profileData.IsSymbol())) {
        profileTypeInfo_->Set(thread_, index, GetWeakRef(dynclass.GetTaggedValue()));
        profileTypeInfo_->Set(thread_, index + 1, handler.GetTaggedValue());
        return;
    }
    AddHandlerWithoutKey(dynclass, handler);
}

void ProfileTypeAccessor::AddHandlerWithoutKey(JSHandle<JSTaggedValue> dynclass, JSHandle<JSTaggedValue> handler) const
{
    auto index = slotId_;
    if (IsNamedGlobalIC(GetKind())) {
        profileTypeInfo_->Set(thread_, index, handler.GetTaggedValue());
        return;
    }
    auto profileData = profileTypeInfo_->Get(slotId_);
    ASSERT(!profileData.IsHole());
    if (profileData.IsUndefined()) {
        profileTypeInfo_->Set(thread_, index, GetWeakRef(dynclass.GetTaggedValue()));
        profileTypeInfo_->Set(thread_, index + 1, handler.GetTaggedValue());
        return;
    }
    if (!profileData.IsWeak() && profileData.IsTaggedArray()) {  // POLY
        ASSERT(profileTypeInfo_->Get(index + 1) == JSTaggedValue::Hole());
        JSHandle<TaggedArray> arr(thread_, profileData);
        const uint32_t step = 2;
        uint32_t newLen = arr->GetLength() + step;
        if (newLen > CACHE_MAX_LEN) {
            profileTypeInfo_->Set(thread_, index, JSTaggedValue::Hole());
            profileTypeInfo_->Set(thread_, index + 1, JSTaggedValue::Hole());
            return;
        }
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
        uint32_t i = 0;
        for (; i < arr->GetLength(); i += step) {
            newArr->Set(thread_, i, arr->Get(i));
            newArr->Set(thread_, i + 1, arr->Get(i + 1));
        }
        newArr->Set(thread_, i, GetWeakRef(dynclass.GetTaggedValue()));
        newArr->Set(thread_, i + 1, handler.GetTaggedValue());
        profileTypeInfo_->Set(thread_, index, newArr.GetTaggedValue());
        profileTypeInfo_->Set(thread_, index + 1, JSTaggedValue::Hole());
        return;
    }
    // MONO to POLY
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, profileTypeInfo_->Get(index));
    newArr->Set(thread_, arrIndex++, profileTypeInfo_->Get(index + 1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(dynclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex, handler.GetTaggedValue());

    profileTypeInfo_->Set(thread_, index, newArr.GetTaggedValue());
    profileTypeInfo_->Set(thread_, index + 1, JSTaggedValue::Hole());
}

void ProfileTypeAccessor::AddHandlerWithKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> dynclass,
                                            JSHandle<JSTaggedValue> handler) const
{
    if (IsValueGlobalIC(GetKind())) {
        AddGlobalHandlerKey(key, handler);
        return;
    }
    auto profileData = profileTypeInfo_->Get(slotId_);
    ASSERT(!profileData.IsHole());
    auto index = slotId_;
    if (profileData.IsUndefined()) {
        ASSERT(profileTypeInfo_->Get(index + 1) == JSTaggedValue::Undefined());
        profileTypeInfo_->Set(thread_, index, key.GetTaggedValue());
        const int arrayLength = 2;
        JSHandle<TaggedArray> newArr = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(arrayLength);
        newArr->Set(thread_, 0, GetWeakRef(dynclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        profileTypeInfo_->Set(thread_, index + 1, newArr.GetTaggedValue());
        return;
    }
    // for element ic, profileData may dynclass or taggedarray
    if (key.GetTaggedValue() != profileData) {
        profileTypeInfo_->Set(thread_, index, JSTaggedValue::Hole());
        profileTypeInfo_->Set(thread_, index + 1, JSTaggedValue::Hole());
        return;
    }
    JSTaggedValue patchValue = profileTypeInfo_->Get(index + 1);
    ASSERT(patchValue.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, patchValue);
    const uint32_t step = 2;
    if (arr->GetLength() > step) {  // POLY
        uint32_t newLen = arr->GetLength() + step;
        if (newLen > CACHE_MAX_LEN) {
            profileTypeInfo_->Set(thread_, index, JSTaggedValue::Hole());
            profileTypeInfo_->Set(thread_, index + 1, JSTaggedValue::Hole());
            return;
        }
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
        newArr->Set(thread_, 0, GetWeakRef(dynclass.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        for (uint32_t i = 0; i < arr->GetLength(); i += step) {
            newArr->Set(thread_, i + step, arr->Get(i));
            newArr->Set(thread_, i + step + 1, arr->Get(i + 1));
        }
        profileTypeInfo_->Set(thread_, index + 1, newArr.GetTaggedValue());
        return;
    }
    // MONO
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(POLY_CASE_NUM);
    uint32_t arrIndex = 0;
    newArr->Set(thread_, arrIndex++, arr->Get(0));
    newArr->Set(thread_, arrIndex++, arr->Get(1));
    newArr->Set(thread_, arrIndex++, GetWeakRef(dynclass.GetTaggedValue()));
    newArr->Set(thread_, arrIndex++, handler.GetTaggedValue());

    profileTypeInfo_->Set(thread_, index + 1, newArr.GetTaggedValue());
}

void ProfileTypeAccessor::AddGlobalHandlerKey(JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> handler) const
{
    auto index = slotId_;
    const uint8_t step = 2;  // key and value pair
    JSTaggedValue indexVal = profileTypeInfo_->Get(index);
    if (indexVal.IsUndefined()) {
        auto factory = thread_->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> newArr = factory->NewTaggedArray(step);
        newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
        newArr->Set(thread_, 1, handler.GetTaggedValue());
        profileTypeInfo_->Set(thread_, index, newArr.GetTaggedValue());
        return;
    }
    ASSERT(indexVal.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread_, indexVal);
    uint32_t newLen = arr->GetLength() + step;
    if (newLen > CACHE_MAX_LEN) {
        profileTypeInfo_->Set(thread_, index, JSTaggedValue::Hole());
        return;
    }
    auto factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArr = factory->NewTaggedArray(newLen);
    newArr->Set(thread_, 0, GetWeakRef(key.GetTaggedValue()));
    newArr->Set(thread_, 1, handler.GetTaggedValue());

    for (uint32_t i = 0; i < arr->GetLength(); i += step) {
        newArr->Set(thread_, i + step, arr->Get(i));
        newArr->Set(thread_, i + step + 1, arr->Get(i + 1));
    }
    profileTypeInfo_->Set(thread_, index, newArr.GetTaggedValue());
}

void ProfileTypeAccessor::AddGlobalRecordHandler(JSHandle<JSTaggedValue> handler) const
{
    uint32_t index = slotId_;
    profileTypeInfo_->Set(thread_, index, handler.GetTaggedValue());
}

void ProfileTypeAccessor::SetAsMega() const
{
    profileTypeInfo_->Set(thread_, slotId_, JSTaggedValue::Hole());
    profileTypeInfo_->Set(thread_, slotId_ + 1, JSTaggedValue::Hole());
}

std::string ICKindToString(ICKind kind)
{
    switch (kind) {
        case ICKind::NamedLoadIC:
            return "NamedLoadIC";
        case ICKind::NamedStoreIC:
            return "NamedStoreIC";
        case ICKind::LoadIC:
            return "LoadIC";
        case ICKind::StoreIC:
            return "StoreIC";
        case ICKind::NamedGlobalLoadIC:
            return "NamedGlobalLoadIC";
        case ICKind::NamedGlobalStoreIC:
            return "NamedGlobalStoreIC";
        case ICKind::GlobalLoadIC:
            return "GlobalLoadIC";
        case ICKind::GlobalStoreIC:
            return "GlobalStoreIC";
        default:
            UNREACHABLE();
            break;
    }
}

std::string ProfileTypeAccessor::ICStateToString(ProfileTypeAccessor::ICState state)
{
    switch (state) {
        case ICState::UNINIT:
            return "uninit";
        case ICState::MONO:
            return "mono";
        case ICState::POLY:
            return "poly";
        case ICState::MEGA:
            return "mega";
        default:
            UNREACHABLE();
            break;
    }
}

ProfileTypeAccessor::ICState ProfileTypeAccessor::GetICState() const
{
    auto profileData = profileTypeInfo_->Get(slotId_);
    if (profileData.IsUndefined()) {
        return ICState::UNINIT;
    }

    if (profileData.IsHole()) {
        return ICState::MEGA;
    }

    switch (kind_) {
        case ICKind::NamedLoadIC:
        case ICKind::NamedStoreIC:
            if (profileData.IsWeak()) {
                return ICState::MONO;
            }
            ASSERT(profileData.IsTaggedArray());
            return ICState::POLY;
        case ICKind::LoadIC:
        case ICKind::StoreIC: {
            if (profileData.IsWeak()) {
                return ICState::MONO;
            }
            if (profileData.IsTaggedArray()) {
                TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
                return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
            }
            profileData = profileTypeInfo_->Get(slotId_ + 1);
            TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
        }
        case ICKind::NamedGlobalLoadIC:
        case ICKind::NamedGlobalStoreIC:
            ASSERT(profileData.IsPropertyBox());
            return ICState::MONO;
        case ICKind::GlobalLoadIC:
        case ICKind::GlobalStoreIC: {
            ASSERT(profileData.IsTaggedArray());
            TaggedArray *array = TaggedArray::Cast(profileData.GetTaggedObject());
            return array->GetLength() == MONO_CASE_NUM ? ICState::MONO : ICState::POLY; // 2 : test case
        }
        default:
            UNREACHABLE();
            break;
    }
    return ICState::UNINIT;
}
}  // namespace panda::ecmascript
