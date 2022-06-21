/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_API_LIGHTWEIGHTMAP_H
#define ECMASCRIPT_JS_API_LIGHTWEIGHTMAP_H

#include "js_object.h"
#include "js_tagged_value-inl.h"

namespace panda::ecmascript {
enum class AccossorsKind { HASH = 0, KEY, VALUE };
struct HashParams {
    JSHandle<TaggedArray> hashArray;
    JSHandle<TaggedArray> keyArray;
    JSTaggedValue *key;
};
class JSAPILightWeightMap : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 8;
    static constexpr int32_t HASH_REBELLION = 0xFFFFFFFF;
    static JSAPILightWeightMap *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPILightWeightMap());
        return static_cast<JSAPILightWeightMap *>(object);
    }
    static void Set(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Get(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                             const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue HasAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                const JSHandle<JSAPILightWeightMap> &newLightWeightMap);
    static JSTaggedValue HasKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue HasValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                  const JSHandle<JSTaggedValue> &value);
    static int32_t GetIndexOfKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                 const JSHandle<JSTaggedValue> &key);
    static int32_t GetIndexOfValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                   const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue GetKeyAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap, int32_t index);
    static void SetAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                       const JSHandle<JSAPILightWeightMap> &newLightWeightMap);
    static JSTaggedValue Remove(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                const JSHandle<JSTaggedValue> &key);
    static JSTaggedValue RemoveAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap, int32_t index);
    static void Clear(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap);
    static JSTaggedValue SetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                    int32_t index, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            int32_t index);
    static JSTaggedValue ToString(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap);
    static JSTaggedValue GetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                    int32_t index);
    static JSTaggedValue GetIteratorObj(JSThread *thread, const JSHandle<JSAPILightWeightMap> &obj, IterationKind type);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPILightWeightMap> &map,
                               const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    JSTaggedValue IsEmpty();
    inline int32_t GetSize() const
    {
        return static_cast<int32_t>(GetLength());
    }

    static constexpr size_t LWP_HASHES_OFFSET = JSObject::SIZE;
    ACCESSORS(Hashes, LWP_HASHES_OFFSET, LWP_KEYS_OFFSET);
    ACCESSORS(Keys, LWP_KEYS_OFFSET, LWP_VALUES_OFFSET);
    ACCESSORS(Values, LWP_VALUES_OFFSET, LWP_LENGTH_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LWP_LENGTH_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LWP_HASHES_OFFSET, LWP_LENGTH_OFFSET)
    DECL_DUMP()

private:
    static inline uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
        return newCapacity > DEFAULT_CAPACITY_LENGTH ? newCapacity : DEFAULT_CAPACITY_LENGTH;
    };
    static JSHandle<TaggedArray> GrowCapacity(const JSThread *thread, const JSHandle<TaggedArray> &oldArray,
                                              uint32_t needCapacity);
    static void RemoveValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap, uint32_t index,
                            AccossorsKind kind);
    static void SetValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                         int32_t index, const JSHandle<JSTaggedValue> &value, AccossorsKind kind);
    static int32_t Hash(JSTaggedValue key);
    static int32_t BinarySearchHashes(JSHandle<TaggedArray> &array, int32_t hash, int32_t size);
    static JSHandle<TaggedArray> GetArrayByKind(const JSThread *thread,
                                                const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                                AccossorsKind kind);
    static int32_t GetHashIndex(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap, int32_t hash,
                                const JSTaggedValue &value, int32_t size);
    static int32_t AvoidHashCollision(HashParams &params, int32_t index, int32_t size, int32_t hash);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_LIGHTWEIGHTMAP_H
