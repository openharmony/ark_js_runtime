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

#include "ecmascript/js_hclass-inl.h"

#include <algorithm>

#include "ecmascript/global_env.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/weak_vector-inl.h"

namespace panda::ecmascript {
#define ENABLE_IC 0  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

// class TransitionsDictionary
TransitionsDictionary *TransitionsDictionary::PutIfAbsent(const JSThread *thread,
                                                          const JSHandle<TransitionsDictionary> &dictionary,
                                                          const JSHandle<JSTaggedValue> &key,
                                                          const JSHandle<JSTaggedValue> &value,
                                                          const JSHandle<JSTaggedValue> &metaData)
{
    int hash = TransitionsDictionary::Hash(key.GetTaggedValue(), metaData.GetTaggedValue());

    /* no need to add key if exist */
    int entry = dictionary->FindEntry(key.GetTaggedValue(), metaData.GetTaggedValue());
    if (entry != -1) {
        return *dictionary;
    }

    // Check whether the dictionary should be extended.
    TransitionsDictionary *newDictionary = HashTableT::GrowHashTable(thread, dictionary);
    // Compute the key object.
    entry = newDictionary->FindInsertIndex(hash);
    newDictionary->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), metaData.GetTaggedValue());

    newDictionary->IncreaseEntries(thread);
    return newDictionary;
}

int TransitionsDictionary::FindEntry(const JSTaggedValue &key, const JSTaggedValue &metaData)
{
    size_t size = Size();
    int count = 1;
    int hash = TransitionsDictionary::Hash(key, metaData);
    // GrowHashTable will guarantee the hash table is never full.
    for (int entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
        JSTaggedValue element = GetKey(entry);
        if (element.IsHole()) {
            continue;
        }
        if (element.IsUndefined()) {
            return -1;
        }

        if (TransitionsDictionary::IsMatch(key, metaData, element, GetAttributes(entry))) {
            return entry;
        }
    }
    return -1;
}

TransitionsDictionary *TransitionsDictionary::Remove(const JSThread *thread,
                                                     const JSHandle<TransitionsDictionary> &table,
                                                     const JSHandle<JSTaggedValue> &key, const JSTaggedValue &metaData)
{
    int entry = table->FindEntry(key.GetTaggedValue(), metaData);
    if (entry == -1) {
        return *table;
    }

    table->RemoveElement(thread, entry);
    return TransitionsDictionary::Shrink(thread, table);
}

void TransitionsDictionary::Rehash(const JSThread *thread, TransitionsDictionary *newTable)
{
    DISALLOW_GARBAGE_COLLECTION;
    if ((newTable == nullptr) || (newTable->Size() < EntriesCount())) {
        return;
    }
    int size = this->Size();
    // Rehash elements to new table
    for (int i = 0; i < size; i++) {
        int fromIndex = GetEntryIndex(i);
        JSTaggedValue k = this->GetKey(i);
        if (!IsKey(k)) {
            continue;
        }
        int hash = TransitionsDictionary::Hash(k, this->GetAttributes(i));
        int insertionIndex = GetEntryIndex(newTable->FindInsertIndex(hash));
        JSTaggedValue tv = Get(fromIndex);
        newTable->Set(thread, insertionIndex, tv);
        for (int j = 1; j < TransitionsDictionary::ENTRY_SIZE; j++) {
            tv = Get(fromIndex + j);
            newTable->Set(thread, insertionIndex + j, tv);
        }
    }
    newTable->SetEntriesCount(thread, EntriesCount());
    newTable->SetHoleEntriesCount(thread, 0);
}

// class JSHClass
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type, JSTaggedValue proto)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (JSType::JS_OBJECT_BEGIN <= type && type <= JSType::JS_OBJECT_END) {
        SetObjectSize(size + DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize());
        auto env = thread->GetEcmaVM()->GetGlobalEnv();
        SetAttributes(thread, env->GetEmptyLayoutInfo());
    } else {
        SetObjectSize(size);
        SetAttributes(thread, JSTaggedValue::Null());
    }
    SetPrototype(thread, proto);

    ClearBitField();
    SetObjectType(type);
    SetExtensible(true);
    SetIsPrototype(false);
    SetElementRepresentation(Representation::NONE);
    SetUnusedInlinedProps(DEFAULT_CAPACITY_OF_IN_OBJECTS);
    SetUnusedNonInlinedProps(DEFAULT_CAPACITY_OF_OUT_OBJECTS);
    SetTransitions(thread, JSTaggedValue::Null());
    SetParent(thread, JSTaggedValue::Null());
    SetProtoChangeMarker(thread, JSTaggedValue::Null());
    SetProtoChangeDetails(thread, JSTaggedValue::Null());
    SetEnumCache(thread, JSTaggedValue::Null());
}

JSHandle<JSHClass> JSHClass::Clone(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetObjectSize() - DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize();

    JSHandle<JSHClass> newJshclass = thread->GetEcmaVM()->GetFactory()->NewEcmaDynClass(size, type);
    // 3. Copy
    JSHClass::CopyAll(thread, newJshclass, jshclass);
    return newJshclass;
}

void JSHClass::CopyAll(const JSThread *thread, const JSHandle<JSHClass> &newJshclass,
                       const JSHandle<JSHClass> &jshclass)
{
    newJshclass->Copy(thread, *jshclass);

    newJshclass->SetParent(thread, JSTaggedValue::Null());
    newJshclass->SetTransitions(thread, JSTaggedValue::Null());
    newJshclass->SetProtoChangeDetails(thread, JSTaggedValue::Null());
    newJshclass->SetEnumCache(thread, JSTaggedValue::Null());
    // reuse Attributes first.
    newJshclass->SetAttributes(thread, jshclass->GetAttributes());
}

void JSHClass::TransitionElementsToDictionary(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    // property transition to slow first
    if (!obj->GetJSHClass()->IsDictionaryMode()) {
        JSObject::TransitionToDictionary(thread, obj);
    }
    obj->GetJSHClass()->SetIsDictionaryElement(true);
    obj->GetJSHClass()->SetIsStableJSArray(false);
}

void JSHClass::AddProperty(const JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const PropertyAttributes &attr)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    JSHClass *newDyn = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(attr.GetPropertyMetaData()));
    if (newDyn != nullptr) {
#if ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, jshclass, JSHandle<JSHClass>(thread, newDyn));
#endif
        obj->SetClass(newDyn);
        return;
    }

    // 2. Create hclass
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetObjectSize() - DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize();
    JSHandle<JSHClass> newJshclass = factory->NewEcmaDynClass(size, type);

    // 3. Copy field and properties
    JSHClass::CopyAll(thread, newJshclass, jshclass);

    // 4. Add Property and metaData
    bool inObj = attr.IsInlinedProps();
    int offset = attr.GetOffset();
    if (inObj) {
        newJshclass->DecUnusedInlinedProps();
    } else {
        newJshclass->DecUnusedNonInlinedProps();
    }

    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newJshclass->GetAttributes());

        if (layoutInfoHandle->NumberOfElements() != offset) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1).GetTaggedValue());
            newJshclass->SetAttributes(thread, layoutInfoHandle);
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= offset) {  // need to Grow
            layoutInfoHandle.Update(factory->ExtendLayoutInfo(layoutInfoHandle,
                LayoutInfo::ComputeGrowCapacity(offset)).GetTaggedValue());
            newJshclass->SetAttributes(thread, layoutInfoHandle);
        }
        layoutInfoHandle->AddKey(thread, offset, key.GetTaggedValue(), attr);
    }

    // 5. Add newDynclass to old dynclass's transitions.
    AddTransitions(thread, jshclass, newJshclass, key, attr);

    // 6. update hclass in object.
#if ENABLE_IC
    JSHClass::NotifyHclassChanged(thread, jshclass, newJshclass);
#endif
    obj->SetClass(*newJshclass);
}

JSHandle<JSHClass> JSHClass::TransitionExtension(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPreventExtensionsString());
    {
        auto *newDyn = jshclass->FindTransitions(key.GetTaggedValue(), JSTaggedValue(0));
        if (newDyn != nullptr) {
            return JSHandle<JSHClass>(thread, newDyn);
        }
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. new a hclass
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetObjectSize() - DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize();
    JSHandle<JSHClass> newJshclass = factory->NewEcmaDynClass(size, type);

    // 3. Copy and change
    JSHClass::CopyAll(thread, newJshclass, jshclass);
    newJshclass->SetExtensible(false);

    JSTaggedValue attrs = newJshclass->GetAttributes();
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, attrs);
        layoutInfoHandle.Update(factory->CopyLayoutInfo(layoutInfoHandle).GetTaggedValue());
        newJshclass->SetAttributes(thread, layoutInfoHandle);
    }

    // 4. Add newDynclass to old dynclass's parent's transitions.
    AddExtensionTransitions(thread, jshclass, newJshclass, key);
    // parent is the same as jshclass, already copy
    return newJshclass;
}

JSHandle<JSHClass> JSHClass::TransitionProto(const JSThread *thread, const JSHandle<JSHClass> &jshclass,
                                             const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledPrototypeString());

    {
        auto *newDyn = jshclass->FindProtoTransitions(key.GetTaggedValue(), proto.GetTaggedValue());
        if (newDyn != nullptr) {
            return JSHandle<JSHClass>(thread, newDyn);
        }
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. new a hclass
    JSType type = jshclass->GetObjectType();
    uint32_t size = jshclass->GetObjectSize() - DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize();
    JSHandle<JSHClass> newJshclass = factory->NewEcmaDynClass(size, type);

    // 3. Copy
    JSHClass::CopyAll(thread, newJshclass, jshclass);
    newJshclass->SetPrototype(thread, proto.GetTaggedValue());

    JSTaggedValue attrs = newJshclass->GetAttributes();
    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, attrs);
        layoutInfoHandle.Update(factory->CopyLayoutInfo(layoutInfoHandle).GetTaggedValue());
        newJshclass->SetAttributes(thread, layoutInfoHandle);
    }

    // 4. Add newJshclass to old jshclass's parent's transitions.
    AddProtoTransitions(thread, jshclass, newJshclass, key, proto);

    // parent is the same as jshclass, already copy
    return newJshclass;
}

void JSHClass::SetPrototype(const JSThread *thread, JSTaggedValue proto)
{
    if (proto.IsECMAObject()) {
        JSObject::Cast(proto.GetTaggedObject())->GetJSHClass()->SetIsPrototype(true);
    }
    SetProto(thread, proto);
}
void JSHClass::SetPrototype(const JSThread *thread, const JSHandle<JSTaggedValue> &proto)
{
    if (proto->IsECMAObject()) {
        JSObject::Cast(proto->GetTaggedObject())->GetJSHClass()->SetIsPrototype(true);
    }
    SetProto(thread, proto);
}

void JSHClass::TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    // 1. new a hclass
    JSType type = obj->GetJSHClass()->GetObjectType();
    uint32_t size =
        obj->GetJSHClass()->GetObjectSize() - DEFAULT_CAPACITY_OF_IN_OBJECTS * JSTaggedValue::TaggedTypeSize();
    JSHandle<JSHClass> newJshclass = thread->GetEcmaVM()->GetFactory()->NewEcmaDynClass(size, type);
    {
        DISALLOW_GARBAGE_COLLECTION;
        // 2. Copy
        newJshclass->Copy(thread, obj->GetJSHClass());

        // reset Unused
        newJshclass->SetUnusedInlinedProps(0);
        newJshclass->SetUnusedNonInlinedProps(0);
        newJshclass->SetIsDictionaryMode(true);

        // 3. Add newJshclass to ?
#if ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>(thread, obj->GetJSHClass()), newJshclass);
#endif
        obj->SetClass(newJshclass);
    }
}

JSHandle<JSTaggedValue> JSHClass::EnableProtoChangeMarker(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSTaggedValue proto = jshclass->GetPrototype();
    if (!proto.IsECMAObject()) {
        // Return JSTaggedValue directly. No proto check is needed.
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(false));
    }
    JSHandle<JSObject> protoHandle(thread, proto);
    JSHandle<JSHClass> protoDyncalss(thread, protoHandle->GetJSHClass());
    RegisterOnProtoChain(thread, protoDyncalss);
    JSTaggedValue protoChangeMarker = protoDyncalss->GetProtoChangeMarker();
    if (protoChangeMarker.IsProtoChangeMarker()) {
        JSHandle<ProtoChangeMarker> markerHandle(thread, ProtoChangeMarker::Cast(protoChangeMarker.GetTaggedObject()));
        if (!markerHandle->GetHasChanged()) {
            return JSHandle<JSTaggedValue>(markerHandle);
        }
    }
    JSHandle<ProtoChangeMarker> markerHandle = thread->GetEcmaVM()->GetFactory()->NewProtoChangeMarker();
    markerHandle->SetHasChanged(false);
    protoDyncalss->SetProtoChangeMarker(thread, markerHandle.GetTaggedValue());
    return JSHandle<JSTaggedValue>(markerHandle);
}

void JSHClass::NotifyHclassChanged(const JSThread *thread, JSHandle<JSHClass> oldHclass, JSHandle<JSHClass> newHclass)
{
    if (!oldHclass->IsPrototype()) {
        return;
    }
    // The old hclass is the same as new one
    if (oldHclass.GetTaggedValue() == newHclass.GetTaggedValue()) {
        return;
    }
    newHclass->SetIsPrototype(true);
    JSHClass::NoticeThroughChain(thread, oldHclass);
    JSHClass::RefreshUsers(thread, oldHclass, newHclass);
}

void JSHClass::RegisterOnProtoChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    ASSERT(jshclass->IsPrototype());
    JSHandle<JSHClass> user = jshclass;
    JSHandle<ProtoChangeDetails> userDetails = GetProtoChangeDetails(thread, user);

    while (true) {
        // Find the prototype chain as far as the hclass has not been registered.
        if (userDetails->GetRegisterIndex() != JSTaggedValue(ProtoChangeDetails::UNREGISTERED)) {
            return;
        }

        JSTaggedValue proto = user->GetPrototype();
        if (!proto.IsHeapObject()) {
            return;
        }
        if (proto.IsJSProxy()) {
            return;
        }
        ASSERT(proto.IsECMAObject());
        JSHandle<JSObject> protoHandle(thread, proto);
        JSHandle<ProtoChangeDetails> protoDetails =
            GetProtoChangeDetails(thread, JSHandle<JSHClass>(thread, protoHandle->GetJSHClass()));
        JSTaggedValue listeners = protoDetails->GetChangeListener();
        JSHandle<ChangeListener> listenersHandle;
        if (listeners == JSTaggedValue(0)) {
            listenersHandle = JSHandle<ChangeListener>(ChangeListener::Create(thread));
        } else {
            listenersHandle = JSHandle<ChangeListener>(thread, listeners);
        }
        array_size_t registerIndex = 0;
        JSHandle<ChangeListener> newListeners = ChangeListener::Add(thread, listenersHandle, user, &registerIndex);
        userDetails->SetRegisterIndex(thread, JSTaggedValue(registerIndex));
        protoDetails->SetChangeListener(thread, newListeners.GetTaggedValue());
        userDetails = protoDetails;
        user = JSHandle<JSHClass>(thread, protoHandle->GetJSHClass());
    }
}

bool JSHClass::UnregisterOnProtoChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    ASSERT(jshclass->IsPrototype());
    if (!jshclass->GetProtoChangeDetails().IsProtoChangeDetails()) {
        return false;
    }
    if (!jshclass->GetPrototype().IsECMAObject()) {
        JSTaggedValue listeners =
            ProtoChangeDetails::Cast(jshclass->GetProtoChangeDetails().GetTaggedObject())->GetChangeListener();
        return listeners != JSTaggedValue(0);
    }
    JSHandle<ProtoChangeDetails> currentDetails = GetProtoChangeDetails(thread, jshclass);
    array_size_t index = currentDetails->GetRegisterIndex().GetArrayLength();
    if (JSTaggedValue(index) == JSTaggedValue(ProtoChangeDetails::UNREGISTERED)) {
        return false;
    }
    JSTaggedValue proto = jshclass->GetPrototype();
    ASSERT(proto.IsECMAObject());
    JSTaggedValue protoDetailsValue = JSObject::Cast(proto.GetTaggedObject())->GetJSHClass()->GetProtoChangeDetails();
    ASSERT(protoDetailsValue.IsProtoChangeDetails());
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener();
    ASSERT(listenersValue != JSTaggedValue(0));
    JSHandle<ChangeListener> listeners(thread, listenersValue.GetTaggedObject());
    ASSERT(listeners->Get(index) == jshclass.GetTaggedValue());
    listeners->Delete(thread, index);
    return true;
}

JSHandle<ProtoChangeDetails> JSHClass::GetProtoChangeDetails(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    JSTaggedValue protoDetails = jshclass->GetProtoChangeDetails();
    if (protoDetails.IsProtoChangeDetails()) {
        return JSHandle<ProtoChangeDetails>(thread, protoDetails);
    }
    JSHandle<ProtoChangeDetails> protoDetailsHandle = thread->GetEcmaVM()->GetFactory()->NewProtoChangeDetails();
    jshclass->SetProtoChangeDetails(thread, protoDetailsHandle.GetTaggedValue());
    return protoDetailsHandle;
}

JSHandle<ProtoChangeDetails> JSHClass::GetProtoChangeDetails(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
    return GetProtoChangeDetails(thread, jshclass);
}

void JSHClass::NoticeRegisteredUser([[maybe_unused]] const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    ASSERT(jshclass->IsPrototype());
    JSTaggedValue markerValue = jshclass->GetProtoChangeMarker();
    if (markerValue.IsProtoChangeMarker()) {
        ProtoChangeMarker *protoChangeMarker = ProtoChangeMarker::Cast(markerValue.GetTaggedObject());
        protoChangeMarker->SetHasChanged(true);
    }
}

void JSHClass::NoticeThroughChain(const JSThread *thread, const JSHandle<JSHClass> &jshclass)
{
    NoticeRegisteredUser(thread, jshclass);
    JSTaggedValue protoDetailsValue = jshclass->GetProtoChangeDetails();
    if (!protoDetailsValue.IsProtoChangeDetails()) {
        return;
    }
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetailsValue.GetTaggedObject())->GetChangeListener();
    if (!listenersValue.IsTaggedArray()) {
        return;
    }
    ChangeListener *listeners = ChangeListener::Cast(listenersValue.GetTaggedObject());
    for (array_size_t i = 0; i < listeners->GetEnd(); i++) {
        JSTaggedValue temp = listeners->Get(i);
        if (temp.IsJSHClass()) {
            NoticeThroughChain(thread, JSHandle<JSHClass>(thread, listeners->Get(i).GetTaggedObject()));
        }
    }
}

void JSHClass::RefreshUsers(const JSThread *thread, const JSHandle<JSHClass> &oldHclass,
                            const JSHandle<JSHClass> &newHclass)
{
    ASSERT(oldHclass->IsPrototype());
    ASSERT(newHclass->IsPrototype());
    bool onceRegistered = UnregisterOnProtoChain(thread, oldHclass);

    newHclass->SetProtoChangeDetails(thread, oldHclass->GetProtoChangeDetails());
    oldHclass->SetProtoChangeDetails(thread, JSTaggedValue(0));
    if (onceRegistered) {
        if (newHclass->GetProtoChangeDetails().IsProtoChangeDetails()) {
            ProtoChangeDetails::Cast(newHclass->GetProtoChangeDetails().GetTaggedObject())
                ->SetRegisterIndex(thread, JSTaggedValue(ProtoChangeDetails::UNREGISTERED));
        }
        RegisterOnProtoChain(thread, newHclass);
    }
}
}  // namespace panda::ecmascript
