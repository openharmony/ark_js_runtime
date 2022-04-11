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

#include "ecmascript/js_for_in_iterator.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;

bool JSForInIterator::CheckObjProto(const JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> object(thread, it->GetObject());
    if (!object->IsJSObject()) {
        return false;
    }
    auto *hclass = object->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    if (jsType != JSType::JS_OBJECT) {
        return false;
    }
    JSTaggedValue proto = hclass->GetPrototype();
    if (!proto.IsJSObject()) {
        return false;
    }
    return hclass->GetPrototype().GetTaggedObject()->GetClass() ==
           env->GetObjectFunctionPrototypeClass().GetTaggedValue().GetTaggedObject()->GetClass();
}

void JSForInIterator::FastGetAllEnumKeys(const JSThread *thread, const JSHandle<JSForInIterator> &it,
                                         const JSHandle<JSTaggedValue> &object)
{
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj(object);
    uint32_t numOfElements = obj->GetNumberOfElements();
    uint32_t numOfKeys = obj->GetNumberOfKeys();
    JSHandle<TaggedQueue> remaining = factory->NewTaggedQueue(numOfElements + numOfKeys + 1);
    if (numOfElements > 0) {
        uint32_t elementIndex = 0;
        if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
            elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength();
            for (uint32_t i = 0; i < elementIndex; i++) {
                value.Update(factory->NewFromASCII(ToCString(i)).GetTaggedValue());
                TaggedQueue::PushFixedQueue(thread, remaining, value);
            }
        } else {
            JSHandle<TaggedArray> elements(thread, obj->GetElements());
            if (!elements->IsDictionaryMode()) {
                uint32_t elementsLen = elements->GetLength();
                for (uint32_t i = 0; i < elementsLen; ++i) {
                    if (!elements->Get(i).IsHole()) {
                        value.Update(factory->NewFromASCII(ToCString(i)).GetTaggedValue());
                        TaggedQueue::PushFixedQueue(thread, remaining, value);
                    }
                }
            } else {
                JSHandle<NumberDictionary> numberDic(elements);
                int size = numberDic->Size();
                CVector<JSTaggedValue> sortArr;
                for (int hashIndex = 0; hashIndex < size; hashIndex++) {
                    JSTaggedValue key = numberDic->GetKey(hashIndex);
                    if (!key.IsUndefined() && !key.IsHole()) {
                        PropertyAttributes attr = numberDic->GetAttributes(hashIndex);
                        if (attr.IsEnumerable()) {
                            sortArr.push_back(JSTaggedValue(static_cast<uint32_t>(key.GetInt())));
                        }
                    }
                }
                std::sort(sortArr.begin(), sortArr.end(), NumberDictionary::CompKey);
                for (const auto &entry : sortArr) {
                    value.Update(factory->NewFromASCII(ToCString(entry.GetInt())).GetTaggedValue());
                    TaggedQueue::PushFixedQueue(thread, remaining, value);
                }
            }
        }
    }
    if (numOfKeys > 0) {
        if (obj->IsJSGlobalObject()) {
            GlobalDictionary *dict = GlobalDictionary::Cast(obj->GetProperties().GetTaggedObject());
            int size = dict->Size();
            CVector<std::pair<JSTaggedValue, uint32_t>> sortArr;
            for (int hashIndex = 0; hashIndex < size; hashIndex++) {
                JSTaggedValue key = dict->GetKey(hashIndex);
                if (!key.IsUndefined() && !key.IsHole()) {
                    PropertyAttributes attr = dict->GetAttributes(hashIndex);
                    if (attr.IsEnumerable()) {
                        std::pair<JSTaggedValue, uint32_t> pair(key, attr.GetOffset());
                        sortArr.emplace_back(pair);
                    }
                }
            }
            std::sort(sortArr.begin(), sortArr.end(), GlobalDictionary::CompKey);
            for (const auto &entry : sortArr) {
                JSTaggedValue nameKey = entry.first;
                if (nameKey.IsString()) {
                    value.Update(nameKey);
                    TaggedQueue::PushFixedQueue(thread, remaining, value);
                }
            }
        } else {
            JSHandle<TaggedArray> propertiesArr(thread, obj->GetProperties());
            if (!propertiesArr->IsDictionaryMode()) {
                JSHClass *jsHclass = obj->GetJSHClass();
                JSTaggedValue enumCache = jsHclass->GetEnumCache();
                if (!enumCache.IsNull()) {
                    JSHandle<TaggedArray> cache(thread, enumCache);
                    uint32_t length = cache->GetLength();
                    if (length != numOfKeys) {
                        JSHandle<LayoutInfo> layoutInfoHandle(thread, jsHclass->GetLayout());
                        for (uint32_t i = 0; i < numOfKeys; i++) {
                            JSTaggedValue key = layoutInfoHandle->GetKey(i);
                            if (key.IsString()) {
                                value.Update(key);
                                if (layoutInfoHandle->GetAttr(i).IsEnumerable()) {
                                    TaggedQueue::PushFixedQueue(thread, remaining, value);
                                }
                            }
                        }
                    } else {
                        for (uint32_t i = 0; i < length; i++) {
                            JSTaggedValue key = cache->Get(i);
                            if (key.IsString()) {
                                value.Update(key);
                                TaggedQueue::PushFixedQueue(thread, remaining, value);
                            }
                        }
                    }
                } else {
                    JSHandle<LayoutInfo> layoutInfoHandle(thread, jsHclass->GetLayout());
                    for (uint32_t i = 0; i < numOfKeys; i++) {
                        JSTaggedValue key = layoutInfoHandle->GetKey(i);
                        if (key.IsString()) {
                            value.Update(key);
                            if (layoutInfoHandle->GetAttr(i).IsEnumerable()) {
                                TaggedQueue::PushFixedQueue(thread, remaining, value);
                            }
                        }
                    }
                }
            } else {
                JSHandle<NameDictionary> nameDic(propertiesArr);
                int size = nameDic->Size();
                CVector<std::pair<JSTaggedValue, PropertyAttributes>> sortArr;
                for (int hashIndex = 0; hashIndex < size; hashIndex++) {
                    JSTaggedValue key = nameDic->GetKey(hashIndex);
                    if (key.IsString()) {
                        PropertyAttributes attr = nameDic->GetAttributes(hashIndex);
                        if (attr.IsEnumerable()) {
                            std::pair<JSTaggedValue, PropertyAttributes> pair(key, attr);
                            sortArr.emplace_back(pair);
                        }
                    }
                }
                std::sort(sortArr.begin(), sortArr.end(), NameDictionary::CompKey);
                for (const auto &entry : sortArr) {
                    value.Update(entry.first);
                    TaggedQueue::PushFixedQueue(thread, remaining, value);
                }
            }
        }
    }
    it->SetRemainingKeys(thread, remaining);
    it->SetWasVisited(true);
}

void JSForInIterator::SlowGetAllEnumKeys(JSThread *thread, const JSHandle<JSForInIterator> &it,
                                         const JSHandle<JSTaggedValue> &object)
{
    JSMutableHandle<TaggedQueue> visited(thread, it->GetVisitedKeys());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedQueue> remaining(thread, it->GetRemainingKeys());
    JSHandle<TaggedArray> arr = JSTaggedValue::GetOwnPropertyKeys(thread, object);
    uint32_t len = arr->GetLength();
    for (uint32_t i = 0; i < len; i++) {
        value.Update(arr->Get(i));
        if (value->IsString()) {
            TaggedQueue *newQueue = TaggedQueue::Push(thread, remaining, value);
            remaining.Update(JSTaggedValue(newQueue));
        }
    }
    it->SetRemainingKeys(thread, remaining);
    it->SetVisitedKeys(thread, visited);
    it->SetWasVisited(true);
}

std::pair<JSTaggedValue, bool> JSForInIterator::NextInternal(JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    bool notModiObjProto = true;
    notModiObjProto = CheckObjProto(thread, it);
    while (true) {
        JSHandle<JSTaggedValue> object(thread, it->GetObject());
        if (object->IsNull() || object->IsUndefined()) {
            return std::make_pair(JSTaggedValue::Undefined(), true);
        }
        if (!it->GetWasVisited()) {
            if (object->IsJSObject() && notModiObjProto) {
                FastGetAllEnumKeys(thread, it, object);
            } else {
                SlowGetAllEnumKeys(thread, it, object);
            }
        }

        JSHandle<TaggedQueue> remaining(thread, it->GetRemainingKeys());
        JSMutableHandle<TaggedQueue> visited(thread, it->GetVisitedKeys());
        while (!remaining->Empty()) {
            JSTaggedValue r = remaining->Pop(thread);
            if (object->IsJSObject() && notModiObjProto) {
                return std::make_pair(r, false);
            }
            JSHandle<JSTaggedValue> key(thread, r);
            bool has_same = false;
            uint32_t len = visited->Size();
            for (uint32_t i = 0; i < len; i++) {
                if (JSTaggedValue::SameValue(r, visited->Get(i))) {
                    has_same = true;
                    break;
                }
            }
            if (has_same) {
                continue;
            }
            PropertyDescriptor desc(thread);
            bool has = JSTaggedValue::GetOwnProperty(thread, object, key, desc);
            if (has) {
                auto newQueue = JSTaggedValue(TaggedQueue::Push(thread, visited, key));
                visited.Update(newQueue);
                it->SetVisitedKeys(thread, newQueue);
                if (desc.IsEnumerable()) {
                    return std::make_pair(key.GetTaggedValue(), false);
                }
            }
        }
        if (notModiObjProto) {
            return std::make_pair(JSTaggedValue::Undefined(), true);
        }
        JSTaggedValue proto = JSTaggedValue::GetPrototype(thread, object);
        it->SetObject(thread, proto);
        it->SetWasVisited(false);
    }
}

// 13.7.5.16.2.1 %ForInIteratorPrototype%.next ( )
JSTaggedValue JSForInIterator::Next(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSForInIterator> it(BuiltinsBase::GetThis(msg));
    ASSERT(it->IsForinIterator());
    std::pair<JSTaggedValue, bool> res = NextInternal(thread, it);
    return res.first;
}
}  // namespace panda::ecmascript
