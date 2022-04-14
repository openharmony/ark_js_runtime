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
    if (transitions.IsUndefined()) {
        JSTaggedValue weakChild = JSTaggedValue(child.GetTaggedValue().CreateAndGetWeakRef());
        parent->SetTransitions(thread, weakChild);
        return;
    }
    JSMutableHandle<TransitionsDictionary> dict(thread, JSTaggedValue::Undefined());
    if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        uint32_t last = cachedHClass->NumberOfProps() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout().GetTaggedObject());
        auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(layoutInfo->GetAttr(last).GetPropertyMetaData()));
        auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(last));
        auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
        dict.Update(TransitionsDictionary::Create(thread));
        transitions = TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass, attr).GetTaggedValue();
    }
    auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(attributes.GetPropertyMetaData()));
    dict.Update(transitions);
    transitions =
        TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child), attr).GetTaggedValue();
    parent->SetTransitions(thread, transitions);
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
    JSMutableHandle<TransitionsDictionary> dict(thread, JSTaggedValue::Undefined());
    if (transitions.IsUndefined()) {
        transitions = TransitionsDictionary::Create(thread).GetTaggedValue();
    } else if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        uint32_t last = cachedHClass->NumberOfProps() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout().GetTaggedObject());
        auto attr = JSHandle<JSTaggedValue>(thread, JSTaggedValue(layoutInfo->GetAttr(last).GetPropertyMetaData()));
        auto lastKey = JSHandle<JSTaggedValue>(thread, layoutInfo->GetKey(last));
        auto lastHClass = JSHandle<JSTaggedValue>(thread, cachedHClass);
        dict.Update(TransitionsDictionary::Create(thread));
        transitions = TransitionsDictionary::PutIfAbsent(thread, dict, lastKey, lastHClass, attr).GetTaggedValue();
    }

    dict.Update(transitions);
    transitions =
        TransitionsDictionary::PutIfAbsent(thread, dict, key, JSHandle<JSTaggedValue>(child), proto).GetTaggedValue();
    parent->SetTransitions(thread, transitions);
}

inline JSHClass *JSHClass::FindTransitions(const JSTaggedValue &key, const JSTaggedValue &attributes)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions();
    if (transitions.IsUndefined()) {
        return nullptr;
    }
    if (transitions.IsWeak()) {
        auto cachedHClass = JSHClass::Cast(transitions.GetTaggedWeakRef());
        int last = cachedHClass->NumberOfProps() - 1;
        LayoutInfo *layoutInfo = LayoutInfo::Cast(cachedHClass->GetLayout().GetTaggedObject());
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

    JSTaggedValue ret = dict->GetValue(entry);
    if (ret.IsUndefined()) {
        return nullptr;
    }

    return JSHClass::Cast(ret.GetTaggedWeakRef());
}

inline JSHClass *JSHClass::FindProtoTransitions(const JSTaggedValue &key, const JSTaggedValue &proto)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue transitions = GetTransitions();
    if (transitions.IsWeak() || !transitions.IsTaggedArray()) {
        ASSERT(transitions.IsUndefined() || transitions.IsWeak());
        return nullptr;
    }
    ASSERT(transitions.IsTaggedArray());
    TransitionsDictionary *dict = TransitionsDictionary::Cast(transitions.GetTaggedObject());
    auto entry = dict->FindEntry(key, proto);
    if (entry == -1) {
        return nullptr;
    }

    JSTaggedValue ret = dict->GetValue(entry);
    if (ret.IsUndefined()) {
        return nullptr;
    }

    return JSHClass::Cast(ret.GetTaggedWeakRef());
}

inline void JSHClass::UpdatePropertyMetaData(const JSThread *thread, [[maybe_unused]] const JSTaggedValue &key,
                                             const PropertyAttributes &metaData)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(!GetLayout().IsNull());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(GetLayout().GetTaggedObject());
    ASSERT(layoutInfo->GetLength() != 0);
    uint32_t entry = metaData.GetOffset();

    layoutInfo->SetNormalAttr(thread, entry, metaData);
}

inline bool JSHClass::HasReferenceField()
{
    auto type = GetObjectType();
    switch (type) {
        case JSType::STRING:
        case JSType::JS_NATIVE_POINTER:
            return false;
        default:
            return true;
    }
}

inline size_t JSHClass::SizeFromJSHClass(TaggedObject *header)
{
    auto type = GetObjectType();
    size_t size = 0;
    switch (type) {
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
            size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(),
                reinterpret_cast<TaggedArray *>(header)->GetLength());
            break;
        case JSType::STRING:
            size = reinterpret_cast<EcmaString *>(header)->ObjectSize();
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        case JSType::MACHINE_CODE_OBJECT:
            size = reinterpret_cast<MachineCode *>(header)->GetMachineCodeObjectSize();
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            break;
        default:
            ASSERT(GetObjectSize() != 0);
            size = GetObjectSize();
            break;
    }
    ASSERT(AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) == size);
    return size;
}

inline void JSHClass::Copy(const JSThread *thread, const JSHClass *jshclass)
{
    DISALLOW_GARBAGE_COLLECTION;

    // copy jshclass
    SetPrototype(thread, jshclass->GetPrototype());
    SetBitField(jshclass->GetBitField());
    SetNumberOfProps(jshclass->NumberOfProps());
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_HCLASS_INL_H
