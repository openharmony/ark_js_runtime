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

#include "ecmascript/ic/function_cache.h"
#include "ecmascript/ic/ic_handler-inl.h"
#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue FunctionCache::GetWeakRef(JSTaggedValue value)
{
    return JSTaggedValue(value.CreateAndGetWeakRef());
}

JSTaggedValue FunctionCache::GetRefFromWeak(const JSTaggedValue &value)
{
    return JSTaggedValue(value.GetTaggedWeakRef());
}

bool FunctionCache::AddHandler(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                               const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &dynclass,
                               const JSHandle<JSTaggedValue> &handler, uint16_t index)
{
    if (key->IsNull()) {
        return AddHandlerWithoutKey(thread, cache, dynclass.GetTaggedValue(), handler.GetTaggedValue(), index);
    }
    return AddHandlerWithKey(thread, cache, key.GetTaggedValue(), dynclass.GetTaggedValue(), handler.GetTaggedValue(),
                             index);
}

bool FunctionCache::AddHandlerWithoutKey(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                                         const JSTaggedValue &dynclass, const JSTaggedValue &handler, uint16_t index)
{
    ASSERT(index < MAX_FUNC_CACHE_INDEX - 1);
    if (cache->Get(index) == JSTaggedValue::Undefined()) {
        ASSERT(cache->Get(index + 1) == JSTaggedValue::Undefined());
        cache->Set(thread, index, cache->GetWeakRef(dynclass));
        cache->Set(thread, index + 1, handler);
        return false;
    }
    if (cache->Get(index) == JSTaggedValue::Hole()) {
        ASSERT(cache->Get(index + 1) == JSTaggedValue::Hole());
        return true;  // for MEGA
    }
    JSTaggedValue cache_value = cache->Get(index);
    if (!cache_value.IsWeak() && cache_value.IsTaggedArray()) {  // POLY
        ASSERT(cache->Get(index + 1) == JSTaggedValue::Hole());
        JSHandle<TaggedArray> arr(thread, cache_value);
        const array_size_t STEP = 2;
        array_size_t newLen = arr->GetLength() + STEP;
        if (newLen > CACHE_MAX_LEN) {
            return true;
        }
        JSHandle<JSTaggedValue> dynHandle(thread, dynclass);
        JSHandle<JSTaggedValue> handlerHandle(thread, handler);
        JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(newLen);
        array_size_t i = 0;
        for (; i < arr->GetLength(); i += STEP) {
            newArr->Set(thread, i, arr->Get(i));
            newArr->Set(thread, i + 1, arr->Get(i + 1));
        }
        ASSERT(i == (newLen - STEP));
        newArr->Set(thread, i, cache->GetWeakRef(dynHandle.GetTaggedValue()));
        newArr->Set(thread, i + 1, handlerHandle.GetTaggedValue());
        cache->Set(thread, index, newArr.GetTaggedValue());
        return false;
    }
    // MONO to POLY
    JSHandle<JSTaggedValue> dynHandle(thread, dynclass);
    JSHandle<JSTaggedValue> handlerHandle(thread, handler);
    JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(POLY_DEFAULT_LEN);
    array_size_t arrIndex = 0;
    newArr->Set(thread, arrIndex++, cache->Get(index));
    newArr->Set(thread, arrIndex++, cache->Get(index + 1));
    newArr->Set(thread, arrIndex++, cache->GetWeakRef(dynHandle.GetTaggedValue()));
    newArr->Set(thread, arrIndex, handlerHandle.GetTaggedValue());

    cache->Set(thread, index, newArr.GetTaggedValue());
    cache->Set(thread, index + 1, JSTaggedValue::Hole());
    return false;
}

bool FunctionCache::AddHandlerWithKey(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                                      const JSTaggedValue &key, const JSTaggedValue &dynclass,
                                      const JSTaggedValue &handler, uint16_t index)
{
    if (cache->Get(index) == JSTaggedValue::Undefined() && cache->Get(index + 1) == JSTaggedValue::Undefined()) {
        cache->Set(thread, index, cache->GetWeakRef(key));
        JSHandle<JSTaggedValue> dynHandle(thread, dynclass);
        JSHandle<JSTaggedValue> handlerHandle(thread, handler);
        const int ARRAY_LENGTH = 2;
        JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(ARRAY_LENGTH);
        newArr->Set(thread, 0, cache->GetWeakRef(dynHandle.GetTaggedValue()));
        newArr->Set(thread, 1, handlerHandle.GetTaggedValue());
        cache->Set(thread, index + 1, newArr.GetTaggedValue());
        return false;
    }
    if (cache->Get(index) == JSTaggedValue::Hole() && cache->Get(index + 1) == JSTaggedValue::Hole()) {
        return true;  // for MEGA
    }
    if (key != cache->GetRefFromWeak(cache->Get(index))) {
        return false;
    }
    JSTaggedValue patchValue = cache->Get(index + 1);
    ASSERT(patchValue.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread, patchValue);
    const array_size_t STEP = 2;
    if (arr->GetLength() > STEP) {  // POLY
        array_size_t newLen = arr->GetLength() + STEP;
        if (newLen > CACHE_MAX_LEN) {
            return true;
        }
        JSHandle<JSTaggedValue> dynHandle(thread, dynclass);
        JSHandle<JSTaggedValue> handlerHandle(thread, handler);
        JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(newLen);
        array_size_t i = 0;
        for (; i < arr->GetLength(); i += STEP) {
            newArr->Set(thread, i, arr->Get(i));
            newArr->Set(thread, i + 1, arr->Get(i + 1));
        }
        ASSERT(i == (newLen - STEP));
        newArr->Set(thread, i, cache->GetWeakRef(dynHandle.GetTaggedValue()));
        newArr->Set(thread, i + 1, handlerHandle.GetTaggedValue());
        cache->Set(thread, index + 1, newArr.GetTaggedValue());
        return false;
    }
    // MONO
    JSHandle<JSTaggedValue> dynHandle(thread, dynclass);
    JSHandle<JSTaggedValue> handlerHandle(thread, handler);
    JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(POLY_DEFAULT_LEN);
    array_size_t arrIndex = 0;
    newArr->Set(thread, arrIndex++, arr->Get(0));
    newArr->Set(thread, arrIndex++, arr->Get(1));
    newArr->Set(thread, arrIndex++, cache->GetWeakRef(dynHandle.GetTaggedValue()));
    newArr->Set(thread, arrIndex++, handlerHandle.GetTaggedValue());

    cache->Set(thread, index + 1, newArr.GetTaggedValue());
    return false;
}

void FunctionCache::TransToMega(const JSThread *thread, bool flag, const JSTaggedValue &key,
                                const JSTaggedValue &dynclass, const JSTaggedValue &handler, uint16_t index)
{
    Set(thread, index, JSTaggedValue::Hole());
    Set(thread, index + 1, JSTaggedValue::Hole());
}

JSTaggedValue FunctionCache::GetGlobalHandlerByValue(const JSTaggedValue &key, uint16_t index)
{
    JSTaggedValue indexVal = Get(index);
    if (indexVal.IsTaggedArray()) {
        TaggedArray *arr = TaggedArray::Cast(indexVal.GetTaggedObject());
        for (array_size_t i = 0; i < arr->GetLength(); i += 2) {  // 2: skip a pair of key and value
            if (GetRefFromWeak(arr->Get(i)) == key) {
                return arr->Get(i + 1);
            }
        }
    }
    return JSTaggedValue::Null();
}

JSTaggedValue FunctionCache::GetGlobalHandlerByIndex(uint16_t index)
{
    JSTaggedValue indexVal = Get(index);
    if (!indexVal.IsUndefined()) {
        ASSERT(!indexVal.IsTaggedArray());
        return indexVal;
    }
    return JSTaggedValue::Null();
}

JSTaggedValue FunctionCache::GetHandlerByName(const JSTaggedValue &name, const JSTaggedValue &dynclass, uint16_t index)
{
    if (Get(index) == JSTaggedValue::Undefined() && Get(index + 1) == JSTaggedValue::Undefined()) {
        return JSTaggedValue::Null();
    }
    if (Get(index) == JSTaggedValue::Hole() && Get(index + 1) == JSTaggedValue::Hole()) {
        return JSTaggedValue::Hole();  // MEGA return Hole to store/load inline cache
    }
    if (name != GetRefFromWeak(Get(index))) {
        return JSTaggedValue::Null();
    }
    JSTaggedValue patchValue = Get(index + 1);
    ASSERT(patchValue.IsTaggedArray());
    TaggedArray *arr = TaggedArray::Cast(patchValue.GetTaggedObject());
    const array_size_t STEP = 2;
    if (arr->GetLength() == STEP) {  // MONO
        if (GetRefFromWeak(arr->Get(0)) == dynclass) {
            return arr->Get(1);
        }
        return JSTaggedValue::Null();
    }
    if (arr->GetLength() > STEP) {  // POLY
        for (array_size_t i = 0; i < arr->GetLength(); i += STEP) {
            if (GetRefFromWeak(arr->Get(i)) == dynclass) {
                return arr->Get(i + 1);
            }
        }
        return JSTaggedValue::Null();
    }
    UNREACHABLE();
}

JSTaggedValue FunctionCache::GetHandlerByIndex(const JSTaggedValue &dynclass, uint16_t index)
{
    JSTaggedValue slot1 = Get(index);
    if (slot1.IsWeak()) {
        if (GetRefFromWeak(slot1) == dynclass) {
            return Get(index + 1);
        }
        return JSTaggedValue::Null();
    }
    if (slot1.IsUndefined()) {
        return JSTaggedValue::Null();
    }
    if (slot1.IsHole()) {
        return JSTaggedValue::Hole();  // MEGA return Hole to store/load inline cache
    }
    ASSERT(slot1.IsTaggedArray());  // POLY
    ASSERT(Get(index + 1) == JSTaggedValue::Hole());
    TaggedArray *arr = TaggedArray::Cast(slot1.GetTaggedObject());
    array_size_t length = arr->GetLength();
    const array_size_t STEP = 2;
    for (array_size_t i = 0; i < length; i += STEP) {
        if (GetRefFromWeak(arr->Get(i)) == dynclass) {
            return arr->Get(i + 1);
        }
    }
    return JSTaggedValue::Null();
}

JSHandle<FunctionCache> FunctionCache::Create(const JSThread *thread, uint16_t capacity)
{
    ASSERT(capacity > 0);

    auto length = static_cast<array_size_t>(capacity);
    JSHandle<FunctionCache> cache = thread->GetEcmaVM()->GetFactory()->NewFunctionCache(length);
    return cache;
}

JSTaggedValue FunctionCache::GetLoadHandler(JSThread *thread, JSTaggedValue key, const JSTaggedValue &dynclass,
                                            uint16_t slotId)
{
    JSTaggedValue handler = GetHandlerByName(key, dynclass, slotId);
    return handler;
}

void FunctionCache::AddLoadHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSHClass> &dynclass, const JSHandle<JSTaggedValue> &handler,
                                   uint16_t slotId)
{
    JSHandle<FunctionCache> cache(thread, GetCurrent(thread));
    bool toMega = AddHandler(thread, cache, key, JSHandle<JSTaggedValue>(dynclass), handler, slotId);
    if (toMega && !key->IsNull()) {
        cache->TransToMega(thread, true, key.GetTaggedValue(), dynclass.GetTaggedValue(), handler.GetTaggedValue(),
                           slotId);
    }
}

void FunctionCache::AddGlobalHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                     const JSHandle<JSTaggedValue> &handler, uint16_t index)
{
    JSHandle<FunctionCache> cache(thread, GetCurrent(thread));
    if (key->IsNull()) {
        return AddGlobalHandlerWithoutKey(thread, cache, handler, index);
    }
    return AddGlobalHandlerWithKey(thread, cache, key, handler, index);
}

void FunctionCache::AddGlobalHandlerWithKey(JSThread *thread, const JSHandle<FunctionCache> &cache,
                                            const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &handler,
                                            uint16_t index)
{
    const uint8_t pairSize = 2;  // key and value pair
    JSTaggedValue indexVal = cache->Get(index);
    if (indexVal.IsUndefined()) {
        JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(pairSize);
        newArr->Set(thread, 0, cache->GetWeakRef(key.GetTaggedValue()));
        newArr->Set(thread, 1, handler.GetTaggedValue());
        cache->Set(thread, index, newArr.GetTaggedValue());
        return;
    }
    ASSERT(indexVal.IsTaggedArray());
    JSHandle<TaggedArray> arr(thread, indexVal);
    array_size_t newLen = arr->GetLength() + pairSize;
    if (newLen > CACHE_MAX_LEN) {
        return;
    }
    JSHandle<TaggedArray> newArr = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(newLen);
    array_size_t i = 0;
    for (; i < arr->GetLength(); i += pairSize) {
        newArr->Set(thread, i, arr->Get(i));
        newArr->Set(thread, i + 1, arr->Get(i + 1));
    }
    ASSERT(i == (newLen - pairSize));
    newArr->Set(thread, i, cache->GetWeakRef(key.GetTaggedValue()));
    newArr->Set(thread, i + 1, handler.GetTaggedValue());
    cache->Set(thread, index, newArr.GetTaggedValue());
}

void FunctionCache::AddGlobalHandlerWithoutKey(JSThread *thread, const JSHandle<FunctionCache> &cache,
                                               const JSHandle<JSTaggedValue> &handler, uint16_t index)
{
    cache->Set(thread, index, handler.GetTaggedValue());
}

JSTaggedValue FunctionCache::GetStoreHandler(JSThread *thread, JSTaggedValue key, const JSTaggedValue &dynclass,
                                             uint16_t slotId)
{
    JSTaggedValue handler = GetHandlerByName(key, dynclass, slotId);
    return handler;
}

void FunctionCache::AddStoreHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSHClass> &dynclass, const JSHandle<JSTaggedValue> &handler,
                                    uint16_t slotId)
{
    JSHandle<FunctionCache> cache(thread, GetCurrent(thread));
    bool toMega = AddHandler(thread, cache, key, JSHandle<JSTaggedValue>(dynclass), handler, slotId);
    if (toMega && !key->IsNull()) {
        cache->TransToMega(thread, false, key.GetTaggedValue(), dynclass.GetTaggedValue(), handler.GetTaggedValue(),
                           slotId);
    }
}
}  // namespace panda::ecmascript
