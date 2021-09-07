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

#ifndef ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H
#define ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::base {
class JsonStringifier {
public:
    explicit JsonStringifier() = default;

    explicit JsonStringifier(JSThread *thread) : thread_(thread) {}

    ~JsonStringifier() = default;
    NO_COPY_SEMANTIC(JsonStringifier);
    NO_MOVE_SEMANTIC(JsonStringifier);

    JSHandle<JSTaggedValue> Stringify(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer,
                                      const JSHandle<JSTaggedValue> &gap);

private:
    CString ValueToQuotedString(CString str);

    bool IsFastValueToQuotedString(const char *value);

    void AddDeduplicateProp(const JSHandle<JSTaggedValue> &property);

    JSTaggedValue SerializeJSONProperty(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);
    JSTaggedValue GetSerializeValue(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);
    void SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent);

    bool SerializeJSONObject(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);

    bool SerializeJSArray(const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &replacer);
    bool SerializeJSProxy(const JSHandle<JSTaggedValue> &object, const JSHandle<JSTaggedValue> &replacer);

    void SerilaizePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef);

    bool PushValue(const JSHandle<JSTaggedValue> &value);

    void PopValue();

    bool CalculateNumberGap(JSTaggedValue gap);

    bool CalculateStringGap(const JSHandle<EcmaString> &primString);
    bool AppendJsonString(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);
    bool SerializeElements(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);
    bool SerializeKeys(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &replacer, bool hasContent);

    static void FastStorePropertyByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value);

    CString gap_;
    CString result_;
    CString indent_;
    JSThread *thread_{nullptr};
    ObjectFactory *factory_{nullptr};
    CVector<JSHandle<JSTaggedValue>> stack_;
    CVector<JSHandle<JSTaggedValue>> propList_;
    JSMutableHandle<JSTaggedValue> handleKey_{};
    JSMutableHandle<JSTaggedValue> handleValue_{};
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_JSON_STRINGIFY_INL_H
