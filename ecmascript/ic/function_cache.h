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

#ifndef PANDA_RUNTIME_FUNCTION_CACHE_H
#define PANDA_RUNTIME_FUNCTION_CACHE_H

#include "ecmascript/js_function.h"
#include "ecmascript/tagged_array.h"
namespace panda {
namespace ecmascript {
class FunctionCache : public TaggedArray {
public:
    static const array_size_t MAX_FUNC_CACHE_INDEX = std::numeric_limits<uint16_t>::max();
    static constexpr uint16_t INVALID_SLOT_INDEX = 0xFF;
    static constexpr array_size_t CACHE_MAX_LEN = 8;
    static constexpr array_size_t POLY_DEFAULT_LEN = 4;

    static FunctionCache *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<FunctionCache *>(object);
    }

    static inline FunctionCache *GetCurrent(JSThread *thread)
    {
        JSTaggedValue funcValue = EcmaFrameHandler(thread).GetFunction();
        JSFunction *func = JSFunction::Cast(funcValue.GetTaggedObject());
        return FunctionCache::Cast(func->GetFunctionCache().GetTaggedObject());
    }

    static JSHandle<FunctionCache> Create(const JSThread *thread, uint16_t capacity);

    JSTaggedValue GetWeakRef(JSTaggedValue value);

    JSTaggedValue GetRefFromWeak(const JSTaggedValue &value);

    // return true means trans to MEGA
    static bool AddHandler(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                           const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &dynclass,
                           const JSHandle<JSTaggedValue> &handler, uint16_t index);

    static bool AddHandlerWithoutKey(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                                     const JSTaggedValue &dynclass, const JSTaggedValue &handler, uint16_t index);

    static bool AddHandlerWithKey(const JSThread *thread, const JSHandle<FunctionCache> &cache,
                                  const JSTaggedValue &key, const JSTaggedValue &dynclass, const JSTaggedValue &handler,
                                  uint16_t index);

    static void AddGlobalHandlerWithKey(JSThread *thread, const JSHandle<FunctionCache> &cache,
                                        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &handler,
                                        uint16_t index);

    static void AddGlobalHandlerWithoutKey(JSThread *thread, const JSHandle<FunctionCache> &cache,
                                           const JSHandle<JSTaggedValue> &handler, uint16_t index);

    void TransToMega(const JSThread *thread, bool flag, const JSTaggedValue &key, const JSTaggedValue &dynclass,
                     const JSTaggedValue &handler, uint16_t index);  // true for Load, false for store

    JSTaggedValue GetHandlerByName(const JSTaggedValue &name, const JSTaggedValue &dynclass, uint16_t index);

    JSTaggedValue GetHandlerByIndex(const JSTaggedValue &dynclass, uint16_t index);

    JSTaggedValue GetGlobalHandlerByValue(const JSTaggedValue &key, uint16_t index);

    JSTaggedValue GetGlobalHandlerByIndex(uint16_t index);

    JSTaggedValue GetLoadHandler(JSThread *thread, JSTaggedValue key, const JSTaggedValue &dynclass, uint16_t slotId);

    JSTaggedValue GetStoreHandler(JSThread *thread, JSTaggedValue key, const JSTaggedValue &dynclass, uint16_t slotId);

    static void AddLoadHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key, const JSHandle<JSHClass> &dynclass,
                               const JSHandle<JSTaggedValue> &handler, uint16_t slotId);

    static void AddGlobalHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                 const JSHandle<JSTaggedValue> &handler, uint16_t index);

    static void AddStoreHandler(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                const JSHandle<JSHClass> &dynclass, const JSHandle<JSTaggedValue> &handler,
                                uint16_t slotId);
};
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_FUNCTION_CACHE_H
