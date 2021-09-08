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

#ifndef ECMASCRIPT_JS_HCLASS_INL_H
#define ECMASCRIPT_JS_HCLASS_INL_H

#include "ecmascript/js_hclass.h"

#include "ecmascript/layout_info-inl.h"
#include "ecmascript/transitions_dictionary.h"
#include "ecmascript/mem/assert_scope.h"

namespace panda::ecmascript {
inline JSHClass *JSHClass::Cast(const TaggedObject *object)
{
    ASSERT(JSTaggedValue(object).IsJSHClass());
    return static_cast<JSHClass *>(const_cast<TaggedObject *>(object));
}

void JSHClass::AddTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent, const JSHandle<JSHClass> &child,
                              const JSHandle<JSTaggedValue> &key, PropertyAttributes attributes)
{
    JSTaggedValue transitions = parent->GetTransitions();
    if (transitions.IsNull()) {
        parent->SetTransitions(thread, child.GetTaggedValue());
        child->SetParent(thread, parent.GetTaggedValue());
        return;
    }
    if (transitions.IsJSHClass()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedObject());
        int last = cachedHClass->GetPropertiesNumber() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetAttributes().GetTaggedObject());
        auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(layoutInfo->GetAttr(last).GetPropertyMetaData()));
        auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(last));
        auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
        auto dict = JSHandle<TransitionsDictionary>(thread, TransitionsDictionary::Create(thread));
        transitions = JSTaggedValue(TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass, attr));
    }
    auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(attributes.GetPropertyMetaData()));
    JSHandle<TransitionsDictionary> dict(thread, transitions);
    transitions =
        JSTaggedValue(TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child), attr));
    parent->SetTransitions(thread, transitions);
    child->SetParent(thread, parent.GetTaggedValue());
}

void JSHClass::AddExtensionTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent,
                                       const JSHandle<JSHClass> &child, const JSHandle<JSTaggedValue> &key)
{
    auto attr = JSHandle<JSTaggedValue>(thread, PropertyAttributes(0).GetTaggedValue());
    AddProtoTransitions(thread, parent, child, key, attr);
}

void JSHClass::AddProtoTransitions(const JSThread *thread, const JSHandle<JSHClass> &parent,
                                   const JSHandle<JSHClass> &child, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &proto)
{
    JSTaggedValue transitions = parent->GetTransitions();
    if (transitions.IsNull()) {
        transitions = JSTaggedValue(TransitionsDictionary::Create(thread));
    } else if (transitions.IsJSHClass()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedObject());
        int last = cachedHClass->GetPropertiesNumber() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetAttributes().GetTaggedObject());
        auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(layoutInfo->GetAttr(last).GetPropertyMetaData()));
        auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(last));
        auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
        auto dict = JSHandle<TransitionsDictionary>(thread, TransitionsDictionary::Create(thread));
        transitions = JSTaggedValue(TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass, attr));
    }

    JSHandle<TransitionsDictionary> dict(thread, transitions);
    transitions =
        JSTaggedValue(TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child), proto));
    parent->SetTransitions(thread, transitions);
    child->SetParent(thread, parent.GetTaggedValue());
}

inline JSHClass *JSHClass::FindTransitions(const JSTaggedValue &key, const JSTaggedValue &attributes)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions();
    if (transitions.IsNull()) {
        return nullptr;
    }
    if (transitions.IsJSHClass()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedObject());
        int last = cachedHClass->GetPropertiesNumber() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetAttributes().GetTaggedObject());
        auto attr = layoutInfo->GetAttr(last).GetPropertyMetaData();
        auto cachedKey = layoutInfo->GetKey(last);
        if (attr == attributes.GetInt() && key == cachedKey) {
            return cachedHClass;
        }
        return nullptr;
    }

    ASSERT(transitions.IsTaggedArray());
    TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
    auto entry = dict->FindEntry(key, attributes);
    if (entry == -1) {
        return nullptr;
    }
    return JSHClass::Cast(dict->GetValue(entry).GetTaggedObject());
}

inline JSHClass *JSHClass::FindProtoTransitions(const JSTaggedValue &key, const JSTaggedValue &proto)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions();
    if (!transitions.IsTaggedArray()) {
        ASSERT(transitions.IsNull() || transitions.IsJSHClass());
        return nullptr;
    }
    ASSERT(transitions.IsTaggedArray());
    TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
    auto entry = dict->FindEntry(key, proto);
    if (entry == -1) {
        return nullptr;
    }
    return JSHClass::Cast(dict->GetValue(entry).GetTaggedObject());
}

inline void JSHClass::UpdatePropertyMetaData(const JSThread *thread, [[maybe_unused]] const JSTaggedValue &key,
                                             const PropertyAttributes &metaData)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(!GetAttributes().IsNull());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(GetAttributes().GetTaggedObject());
    ASSERT(layoutInfo->GetLength() != 0);
    int entry = metaData.GetOffset();

    layoutInfo->SetNormalAttr(thread, entry, metaData);
}

inline bool JSHClass::HasReferenceField(JSType type)
{
    return type != JSType::STRING && type != JSType::JS_NATIVE_POINTER;
}

inline size_t JSHClass::SizeFromJSHClass(JSType type, TaggedObject *header)
{
    if (type == JSType::TAGGED_ARRAY || type == JSType::TAGGED_DICTIONARY) {
        auto length = reinterpret_cast<TaggedArray *>(header)->GetLength();
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    }

    if (type == JSType::STRING) {
        return reinterpret_cast<EcmaString *>(header)->ObjectSize();
    }
    if (type == JSType::MACHINE_CODE_OBJECT) {
        return reinterpret_cast<MachineCode *>(header)->GetMachineCodeObjectSize();
    }
    ASSERT(GetObjectSize() != 0);
    return GetObjectSize();
}

inline void JSHClass::Copy(const JSThread *thread, const JSHClass *jshcalss)
{
    DISALLOW_GARBAGE_COLLECTION;

    // copy jshclass
    SetPrototype(thread, jshcalss->GetPrototype());
    SetBitField(jshcalss->GetBitField());
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_HCLASS_INL_H
