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

#ifndef ECMASCRIPT_JS_API_LIST_H
#define ECMASCRIPT_JS_API_LIST_H

#include "js_object.h"
#include "js_tagged_value-inl.h"
#include "tagged_list.h"

namespace panda::ecmascript {
class JSAPIList : public JSObject {
public:
    static constexpr int DEFAULT_CAPACITY_LENGTH = 10;
    static JSAPIList *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIList());
        return static_cast<JSAPIList *>(object);
    }

    static void Add(JSThread *thread, const JSHandle<JSAPIList> &list, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue Insert(JSThread *thread, const JSHandle<JSAPIList> &list,
                                const JSHandle<JSTaggedValue> &value, const int index);
    static JSTaggedValue Set(JSThread *thread, const JSHandle<JSAPIList> &list,
                             const int index, const JSHandle<JSTaggedValue> &value);
    static JSTaggedValue ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                            const JSHandle<JSTaggedValue> &callbackFn,
                                            const JSHandle<JSTaggedValue> &thisArg);
    static JSTaggedValue Sort(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                              const JSHandle<JSTaggedValue> &callbackFn);
    static JSTaggedValue ConvertToArray(const JSThread *thread, const JSHandle<JSAPIList> &list);
    static JSTaggedValue GetSubList(JSThread *thread, const JSHandle<JSAPIList> &list,
                                    const int fromIndex, const int toIndex);
    static JSTaggedValue RemoveByIndex(JSThread *thread, const JSHandle<JSAPIList> &list, const int &index);
    static JSHandle<TaggedArray> OwnKeys(JSThread *thread, const JSHandle<JSAPIList> &list);
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSAPIList> &list,
                               const JSHandle<JSTaggedValue> &key);
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSAPIList> &list,
                                       const JSHandle<JSTaggedValue> &key);

    JSTaggedValue GetFirst();
    JSTaggedValue GetLast();
    bool IsEmpty();
    JSTaggedValue Get(const int index);
    bool Has(const JSTaggedValue &element);
    JSTaggedValue GetIndexOf(const JSTaggedValue &element);
    JSTaggedValue GetLastIndexOf(const JSTaggedValue &element);
    JSTaggedValue Equal(JSThread *thread, const JSHandle<JSAPIList> &list);
    void Clear(JSThread *thread);
    JSTaggedValue Remove(JSThread *thread, const JSTaggedValue &element);
    inline int Length()
    {
        return TaggedSingleList::Cast(GetSingleList().GetTaggedObject())->Length();
    }

    static constexpr size_t SINGLY_LIST_OFFSET = JSObject::SIZE;
    ACCESSORS(SingleList, SINGLY_LIST_OFFSET, SIZE);
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, SINGLY_LIST_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_API_LIST_H
