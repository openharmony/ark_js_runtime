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

#include "object_operator.h"
#include "ecma_vm.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_dictionary.h"
#include "global_dictionary.h"

namespace panda::ecmascript {
void ObjectOperator::HandleKey(const JSHandle<JSTaggedValue> &key)
{
    if (key->IsInt()) {
        int32_t keyInt = key->GetInt();
        if (keyInt >= 0) {
            elementIndex_ = static_cast<uint32_t>(keyInt);
            return;
        }
        key_ = JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread_, JSTaggedValue(keyInt)));
        return;
    }

    if (key->IsString()) {
        uint32_t index = 0;
        if (JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index)) {
            ASSERT(index < JSObject::MAX_ELEMENT_INDEX);
            elementIndex_ = index;
            return;
        }
        if (EcmaString::Cast(key->GetTaggedObject())->IsInternString()) {
            key_ = key;
            return;
        }
        key_ = JSHandle<JSTaggedValue>(thread_, thread_->GetEcmaVM()->GetFactory()->InternString(key));
        return;
    }

    if (key->IsDouble()) {
        double number = key->GetDouble();
        if (number >= 0 && number < JSObject::MAX_ELEMENT_INDEX) {
            auto integer = static_cast<uint32_t>(number);
            if (integer == number) {
                elementIndex_ = static_cast<uint32_t>(number);
                return;
            }
        }
        key_ = JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread_, key.GetTaggedValue()));
        return;
    }

    if (key->IsSymbol()) {
        key_ = key;
        return;
    }

    JSHandle<JSTaggedValue> keyHandle(thread_, JSTaggedValue::ToPrimitive(thread_, key, PREFER_STRING));
    if (key->IsSymbol()) {
        key_ = keyHandle;
        return;
    }
    key_ = JSHandle<JSTaggedValue>(thread_,
                                   thread_->GetEcmaVM()->GetFactory()->InternString(
                                       JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread_, keyHandle))));
}

void ObjectOperator::UpdateHolder()
{
    if (holder_->IsString() &&
        (IsElement() && elementIndex_ < EcmaString::Cast(holder_->GetTaggedObject())->GetLength())) {
        holder_.Update(JSPrimitiveRef::StringCreate(thread_, holder_).GetTaggedValue());
    } else {
        holder_.Update(JSTaggedValue::ToPrototypeOrObj(thread_, holder_).GetTaggedValue());
    }
}

void ObjectOperator::StartLookUp(OperatorType type)
{
    UpdateHolder();

    if (type == OperatorType::OWN) {
        LookupPropertyInHolder();
    } else {
        LookupProperty();
    }
}

void ObjectOperator::StartGlobalLookUp(OperatorType type)
{
    UpdateHolder();

    if (type == OperatorType::OWN) {
        GlobalLookupPropertyInHolder();
    } else {
        GlobalLookupProperty();
    }
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &key, OperatorType type)
    : thread_(thread),
      holder_(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject()),
      receiver_(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject())
{
    HandleKey(key);
    StartGlobalLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &key,
                               OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, holder.GetTaggedValue())
{
    HandleKey(key);
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                               const JSHandle<JSTaggedValue> &key, OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, holder.GetTaggedValue())
{
    HandleKey(key);
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, uint32_t index,
                               OperatorType type)
    : thread_(thread),
      holder_(thread, holder.GetTaggedValue()),
      receiver_(thread, holder.GetTaggedValue()),
      elementIndex_(index)
{
    StartLookUp(type);
}

ObjectOperator::ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                               const JSHandle<JSTaggedValue> &receiver, const JSHandle<JSTaggedValue> &key,
                               OperatorType type)
    : thread_(thread), holder_(thread, holder.GetTaggedValue()), receiver_(thread, receiver.GetTaggedValue())
{
    SetHasReceiver(true);
    HandleKey(key);
    StartLookUp(type);
}

// op for fast path
ObjectOperator::ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                               OperatorType type)
    : thread_(thread), holder_(thread, receiver), receiver_(thread, receiver), key_(thread, name)
{
    ASSERT(name.IsStringOrSymbol());
    StartLookUp(type);
}
JSHandle<JSTaggedValue> ObjectOperator::FastGetValue()
{
    ASSERT(IsFound() && !value_.IsEmpty());
    if (value_->IsPropertyBox()) {
        value_.Update(PropertyBox::Cast(value_->GetTaggedObject())->GetValue());
    }
    if (!IsAccessorDescriptor()) {
        return value_;
    }
    AccessorData *accessor = AccessorData::Cast(value_->GetTaggedObject());
    ASSERT(!accessor->IsInternal());
    // 8. Return Call(getter, Receiver).
    return JSHandle<JSTaggedValue>(thread_, JSObject::CallGetter(thread_, accessor, receiver_));
}
ObjectOperator::ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                               const PropertyAttributes &attr)
    : thread_(thread), receiver_(thread, receiver), key_(thread, name)
{
    SetAttr(attr);
}
void ObjectOperator::FastAdd(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                             const JSHandle<JSTaggedValue> &value, const PropertyAttributes &attr)
{
    ObjectOperator op(thread, receiver, name, attr);
    op.AddPropertyInternal(value);
}

void ObjectOperator::ToPropertyDescriptor(PropertyDescriptor &desc) const
{
    DISALLOW_GARBAGE_COLLECTION;
    if (!IsFound()) {
        return;
    }

    if (!IsAccessorDescriptor()) {
        desc.SetWritable(IsWritable());
        JSTaggedValue val = GetValue();
        desc.SetValue(JSHandle<JSTaggedValue>(thread_, val));
    } else {
        AccessorData *accessor = AccessorData::Cast(GetValue().GetTaggedObject());

        if (UNLIKELY(accessor->IsInternal())) {
            desc.SetWritable(IsWritable());
            auto val = accessor->CallInternalGet(thread_, JSHandle<JSObject>::Cast(GetHolder()));
            desc.SetValue(JSHandle<JSTaggedValue>(thread_, val));
        } else {
            desc.SetGetter(JSHandle<JSTaggedValue>(thread_, accessor->GetGetter()));
            desc.SetSetter(JSHandle<JSTaggedValue>(thread_, accessor->GetSetter()));
        }
    }

    desc.SetEnumerable(IsEnumerable());
    desc.SetConfigurable(IsConfigurable());
}

void ObjectOperator::GlobalLookupProperty()
{
    GlobalLookupPropertyInHolder();
    if (IsFound()) {
        return;
    }
    JSTaggedValue proto = JSObject::Cast(holder_->GetTaggedObject())->GetPrototype(thread_);
    if (!proto.IsHeapObject()) {
        return;
    }
    holder_.Update(proto);
    if (holder_->IsJSProxy()) {
        return;
    }
    SetIsOnPrototype(true);
    LookupProperty();
}

void ObjectOperator::LookupProperty()
{
    while (true) {
        LookupPropertyInHolder();
        if (IsFound()) {
            return;
        }

        JSTaggedValue proto = holder_.GetObject<JSObject>()->GetPrototype(thread_);
        if (!proto.IsHeapObject()) {
            return;
        }

        holder_.Update(proto);
        if (holder_->IsJSProxy()) {
            return;
        }

        SetIsOnPrototype(true);
    }
}

void ObjectOperator::LookupGlobal(const JSHandle<JSObject> &obj)
{
    ASSERT(obj->IsJSGlobalObject());
    if (IsElement()) {
        LookupElementInlinedProps(obj);
        return;
    }
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (array->GetLength() == 0) {
        return;
    }
    GlobalDictionary *dict = GlobalDictionary::Cast(array);
    int entry = dict->FindEntry(key_.GetTaggedValue());
    if (entry == -1) {
        return;
    }
    JSTaggedValue value(dict->GetBox(entry));
    uint32_t attr = dict->GetAttributes(entry).GetValue();
    SetFound(entry, value, attr, true);
}

void ObjectOperator::LookupPropertyInlinedProps(const JSHandle<JSObject> &obj)
{
    if (IsElement()) {
        LookupElementInlinedProps(obj);
        return;
    }

    if (obj->IsJSGlobalObject()) {
        TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
        if (array->GetLength() == 0) {
            return;
        }

        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        int entry = dict->FindEntry(key_.GetTaggedValue());
        if (entry == -1) {
            return;
        }

        JSTaggedValue value(dict->GetBox(entry));
        uint32_t attr = dict->GetAttributes(entry).GetValue();
        SetFound(entry, value, attr, true);
        return;
    }

    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *jshclass = obj->GetJSHClass();
        JSTaggedValue attrs = jshclass->GetAttributes();
        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propsNumber = jshclass->GetPropertiesNumber();
        int entry = layoutInfo->FindElementWithCache(thread_, jshclass, key_.GetTaggedValue(), propsNumber);
        if (entry == -1) {
            return;
        }
        PropertyAttributes attr(layoutInfo->GetAttr(entry));
        ASSERT(entry == static_cast<int>(attr.GetOffset()));
        JSTaggedValue value;
        if (attr.IsInlinedProps()) {
            value = obj->GetPropertyInlinedProps(entry);
        } else {
            entry -= JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS;
            value = array->Get(entry);
        }

        SetFound(entry, value, attr.GetValue(), true);
        return;
    }

    NameDictionary *dict = NameDictionary::Cast(array);
    int entry = dict->FindEntry(key_.GetTaggedValue());
    if (entry == -1) {
        return;
    }

    JSTaggedValue value = dict->GetValue(entry);
    uint32_t attr = dict->GetAttributes(entry).GetValue();
    SetFound(entry, value, attr, false);
}

void ObjectOperator::TransitionForAttributeChanged(const JSHandle<JSObject> &receiver, PropertyAttributes attr)
{
    if (IsElement()) {
        uint32_t index = GetIndex();
        if (!receiver->GetJSHClass()->IsDictionaryElement()) {
            JSObject::ElementsToDictionary(thread_, receiver);
            auto dict = NumberDictionary::Cast(receiver->GetElements().GetTaggedObject());
            index = dict->FindEntry(JSTaggedValue(index));
            PropertyAttributes origin = dict->GetAttributes(index);
            attr.SetDictionaryOrder(origin.GetDictionaryOrder());
            dict->SetAttributes(thread_, index, attr);
        } else {
            auto dict = NumberDictionary::Cast(receiver->GetElements().GetTaggedObject());
            dict->SetAttributes(thread_, index, attr);
        }
        // update found result
        UpdateFound(index, attr.GetValue(), false, true);
    } else if (receiver->IsJSGlobalObject()) {
        JSHandle<GlobalDictionary> dictHandle(thread_, receiver->GetProperties());
        GlobalDictionary::InvalidatePropertyBox(thread_, dictHandle, GetIndex(), attr);
    } else {
        uint32_t index = GetIndex();
        if (!receiver->GetJSHClass()->IsDictionaryMode()) {
            JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread_, receiver));
            index = dict->FindEntry(key_.GetTaggedValue());
            PropertyAttributes origin = dict->GetAttributes(index);
            attr.SetDictionaryOrder(origin.GetDictionaryOrder());
            dict->SetAttributes(thread_, index, attr);
        } else {
            auto dict = NameDictionary::Cast(receiver->GetProperties().GetTaggedObject());
            dict->SetAttributes(thread_, index, attr);
        }
        // update found result
        UpdateFound(index, attr.GetValue(), false, true);
    }
}

bool ObjectOperator::UpdateValueAndDetails(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                           PropertyAttributes attr, bool attrChanged)
{
    bool isInternalAccessor = IsAccessorDescriptor() && AccessorData::Cast(GetValue().GetHeapObject())->IsInternal();
    if (attrChanged) {
        TransitionForAttributeChanged(receiver, attr);
    }
    return UpdateDataValue(receiver, value, isInternalAccessor);
}

bool ObjectOperator::UpdateDataValue(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                     bool isInternalAccessor, bool mayThrow)
{
    if (IsElement()) {
        TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
        if (!elements->IsDictionaryMode()) {
            elements->Set(thread_, GetIndex(), value.GetTaggedValue());
            return true;
        }

        NumberDictionary *dict = NumberDictionary::Cast(elements);
        dict->UpdateValue(thread_, GetIndex(), value.GetTaggedValue());
        return true;
    }

    if (receiver->IsJSGlobalObject()) {
        // need update cell type ?
        auto *dict = GlobalDictionary::Cast(receiver->GetProperties().GetTaggedObject());
        PropertyBox *cell = dict->GetBox(GetIndex());
        cell->SetValue(thread_, value.GetTaggedValue());
        return true;
    }

    if (isInternalAccessor) {
        auto accessor = AccessorData::Cast(GetValue().GetHeapObject());
        if (accessor->HasSetter()) {
            return accessor->CallInternalSet(thread_, JSHandle<JSObject>(receiver), value, mayThrow);
        }
    }

    TaggedArray *properties = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
    if (!properties->IsDictionaryMode()) {
        PropertyAttributes attr = GetAttr();
        if (attr.IsInlinedProps()) {
            receiver->SetPropertyInlinedProps(thread_, GetIndex(), value.GetTaggedValue());
        } else {
            properties->Set(thread_, GetIndex(), value.GetTaggedValue());
        }
    } else {
        NameDictionary::Cast(properties)->UpdateValue(thread_, GetIndex(), value.GetTaggedValue());
    }
    return true;
}

bool ObjectOperator::WriteDataProperty(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc)
{
    PropertyAttributes attr = GetAttr();
    bool attrChanged = false;

    // composed new attribute from desc
    if (desc.HasConfigurable() && attr.IsConfigurable() != desc.IsConfigurable()) {
        attr.SetConfigurable(desc.IsConfigurable());
        attrChanged = true;
    }
    if (desc.HasEnumerable() && attr.IsEnumerable() != desc.IsEnumerable()) {
        attr.SetEnumerable(desc.IsEnumerable());
        attrChanged = true;
    }

    if (!desc.IsAccessorDescriptor()) {
        if (desc.HasWritable() && attr.IsWritable() != desc.IsWritable()) {
            attr.SetWritable(desc.IsWritable());
            attrChanged = true;
        }
        if (!desc.HasValue()) {
            if (attrChanged) {
                TransitionForAttributeChanged(receiver, attr);
            }
            return true;
        }

        if (IsAccessorDescriptor()) {
            auto accessor = AccessorData::Cast(GetValue().GetHeapObject());
            if (!accessor->IsInternal() || !accessor->HasSetter()) {
                attr.SetIsAccessor(false);
                attrChanged = true;
            }
        }

        return UpdateValueAndDetails(receiver, desc.GetValue(), attr, attrChanged);
    } else {
        if (IsAccessorDescriptor() && !IsElement()) {
            TaggedArray *properties = TaggedArray::Cast(receiver->GetProperties().GetTaggedObject());
            if (!properties->IsDictionaryMode()) {
                // as some accessorData is in globalEnv, we need to new accessorData.
                JSHandle<AccessorData> accessor = thread_->GetEcmaVM()->GetFactory()->NewAccessorData();

                if (desc.HasGetter()) {
                    accessor->SetGetter(thread_, desc.GetGetter().GetTaggedValue());
                } else {
                    accessor->SetGetter(thread_, JSHandle<AccessorData>::Cast(value_)->GetGetter());
                }
                if (desc.HasSetter()) {
                    accessor->SetSetter(thread_, desc.GetSetter().GetTaggedValue());
                } else {
                    accessor->SetSetter(thread_, JSHandle<AccessorData>::Cast(value_)->GetSetter());
                }

                JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread_, receiver));
                int entry = dict->FindEntry(key_.GetTaggedValue());
                ASSERT(entry != -1);
                dict->UpdateValueAndAttributes(thread_, entry, accessor.GetTaggedValue(), attr);
                return true;
            }
        }

        JSHandle<AccessorData> accessor = IsAccessorDescriptor()
                                              ? JSHandle<AccessorData>::Cast(value_)
                                              : thread_->GetEcmaVM()->GetFactory()->NewAccessorData();
        if (desc.HasGetter()) {
            accessor->SetGetter(thread_, desc.GetGetter().GetTaggedValue());
        }

        if (desc.HasSetter()) {
            accessor->SetSetter(thread_, desc.GetSetter().GetTaggedValue());
        }

        if (!IsAccessorDescriptor()) {
            attr.SetIsAccessor(true);
            attrChanged = true;
        }

        JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>::Cast(accessor);
        return UpdateValueAndDetails(receiver, value, attr, attrChanged);
    }
}

void ObjectOperator::DeletePropertyInHolder()
{
    if (IsElement()) {
        return DeleteElementInHolder();
    }

    JSObject::DeletePropertyInternal(thread_, JSHandle<JSObject>(holder_), key_, GetIndex());
}

bool ObjectOperator::AddProperty(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                                 PropertyAttributes attr)
{
    if (IsElement()) {
        return JSObject::AddElementInternal(thread_, receiver, elementIndex_, value, attr);
    }

    ResetState();
    receiver_.Update(receiver.GetTaggedValue());
    SetAttr(attr.GetValue());
    AddPropertyInternal(value);
    return true;
}

void ObjectOperator::WriteElement(const JSHandle<JSObject> &receiver, JSTaggedValue value) const
{
    ASSERT(IsElement() && GetIndex() < JSObject::MAX_ELEMENT_INDEX);

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    if (!elements->IsDictionaryMode()) {
        elements->Set(thread_, index_, value);
        receiver->GetJSHClass()->UpdateRepresentation(value);
        return;
    }

    NumberDictionary *dictionary = NumberDictionary::Cast(elements);
    dictionary->UpdateValue(thread_, GetIndex(), value);
}

void ObjectOperator::DeleteElementInHolder() const
{
    JSHandle<JSObject> obj(holder_);

    TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
    if (!elements->IsDictionaryMode()) {
        elements->Set(thread_, index_, JSTaggedValue::Hole());
        JSObject::ElementsToDictionary(thread_, JSHandle<JSObject>(holder_));
    } else {
        JSHandle<NumberDictionary> dictHandle(thread_, elements);
        NumberDictionary *dict = NumberDictionary::Remove(thread_, dictHandle, GetIndex());
        obj->SetElements(thread_, JSTaggedValue(dict));
    }
}

void ObjectOperator::SetFound(uint32_t index, JSTaggedValue value, uint32_t attr, bool mode, bool transition)
{
    SetIndex(index);
    SetValue(value);
    SetFastMode(mode);
    SetIsTransition(transition);
    SetAttr(attr);
}

void ObjectOperator::UpdateFound(uint32_t index, uint32_t attr, bool mode, bool transition)
{
    SetIndex(index);
    SetFastMode(mode);
    SetIsTransition(transition);
    SetAttr(attr);
}

void ObjectOperator::ResetState()
{
    // index may used by element
    SetIndex(NOT_FOUND_INDEX);
    SetValue(JSTaggedValue::Undefined());
    SetFastMode(false);
    SetAttr(0);
    SetIsOnPrototype(false);
    SetHasReceiver(false);
}

void ObjectOperator::LookupElementInlinedProps(const JSHandle<JSObject> &obj)
{
    // if is js string, do special.
    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(obj.GetTaggedValue().GetTaggedObject())->IsString()) {
        PropertyDescriptor desc(thread_);
        bool status = JSPrimitiveRef::StringGetIndexProperty(thread_, obj, elementIndex_, &desc);
        if (status) {
            PropertyAttributes attr(desc);
            SetFound(elementIndex_, desc.GetValue().GetTaggedValue(), attr.GetValue(), true);
            return;
        }
    }
    {
        DISALLOW_GARBAGE_COLLECTION;
        TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
        if (elements->GetLength() == 0) {
            return;  // Empty Array
        }

        if (!elements->IsDictionaryMode()) {
            if (elements->GetLength() <= elementIndex_) {
                return;
            }

            JSTaggedValue value = elements->Get(elementIndex_);
            if (value.IsHole()) {
                return;
            }
            SetFound(elementIndex_, value, PropertyAttributes::GetDefaultAttributes(), true);
        } else {
            NumberDictionary *dictionary = NumberDictionary::Cast(obj->GetElements().GetTaggedObject());
            JSTaggedValue key(static_cast<int>(elementIndex_));
            int entry = dictionary->FindEntry(key);
            if (entry == -1) {
                return;
            }

            uint32_t attr = dictionary->GetAttributes(entry).GetValue();
            SetFound(entry, dictionary->GetValue(entry), attr, false);
        }
    }
}

void ObjectOperator::AddPropertyInternal(const JSHandle<JSTaggedValue> &value)
{
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj(GetReceiver());
    PropertyAttributes attr = GetAttr();
    if (obj->IsJSGlobalObject()) {
        JSMutableHandle<GlobalDictionary> dict(thread_, obj->GetProperties());
        if (dict->GetLength() == 0) {
            dict.Update(JSTaggedValue(GlobalDictionary::Create(thread_)));
        }

        // Add PropertyBox to global dictionary
        JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(key_);
        cellHandle->SetValue(thread_, value.GetTaggedValue());
        PropertyBoxType cellType = value->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
        attr.SetBoxType(cellType);

        JSTaggedValue properties(
            GlobalDictionary::PutIfAbsent(thread_, dict, key_, JSHandle<JSTaggedValue>(cellHandle), attr));
        obj->SetProperties(thread_, properties);
        // index and fastMode is not essential for global obj;
        SetFound(0, cellHandle.GetTaggedValue(), attr.GetValue(), true);
        return;
    }

    if (obj->IsJSArray() && key_.GetTaggedValue() == thread_->GlobalConstants()->GetConstructorString()) {
        obj->GetJSHClass()->SetHasConstructor(true);
    }

    uint32_t unusedInlinedProps = obj->GetJSHClass()->GetUnusedInlinedProps();
    if (unusedInlinedProps != 0) {
        uint32_t order = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS - unusedInlinedProps;
        obj->SetPropertyInlinedProps(thread_, order, value.GetTaggedValue());
        attr.SetOffset(order);
        attr.SetIsInlinedProps(true);
        JSHClass::AddProperty(thread_, obj, key_, attr);
        SetFound(order, value.GetTaggedValue(), attr.GetValue(), true, true);
        return;
    }

    JSMutableHandle<TaggedArray> array(thread_, obj->GetProperties());
    array_size_t length = array->GetLength();
    if (length == 0) {
        length = JSObject::MIN_PROPERTIES_LENGTH;
        array.Update(factory->NewTaggedArray(length).GetTaggedValue());
        obj->SetProperties(thread_, array.GetTaggedValue());
    }

    if (!array->IsDictionaryMode()) {
        attr.SetIsInlinedProps(false);

        array_size_t nonInlinedProps =
            JSHClass::DEFAULT_CAPACITY_OF_OUT_OBJECTS - obj->GetJSHClass()->GetUnusedNonInlinedProps();
        ASSERT(length >= nonInlinedProps);
        // if array is full, grow array or change to dictionary mode
        if (length == nonInlinedProps) {
            if (length >= JSHClass::DEFAULT_CAPACITY_OF_OUT_OBJECTS) {
                // change to dictionary and add one.
                JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread_, obj));
                attr.SetDictionaryOrder(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES);
                auto result = JSTaggedValue(NameDictionary::PutIfAbsent(thread_, dict, key_, value, attr));
                obj->SetProperties(thread_, result);
                // index is not essential when fastMode is false;
                SetFound(0, value.GetTaggedValue(), attr.GetValue(), false);
                return;
            }
            // Grow properties array size
            array_size_t capacity = JSObject::ComputePropertyCapacity(length);
            array.Update(factory->CopyArray(array, length, capacity).GetTaggedValue());
            obj->SetProperties(thread_, array.GetTaggedValue());
        }

        attr.SetOffset(nonInlinedProps + JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
        JSHClass::AddProperty(thread_, obj, key_, attr);
        array->Set(thread_, nonInlinedProps, value.GetTaggedValue());
        SetFound(nonInlinedProps, value.GetTaggedValue(), attr.GetValue(), true, true);
        return;
    }

    JSHandle<NameDictionary> dictHandle(array);
    auto result = JSTaggedValue(NameDictionary::PutIfAbsent(thread_, dictHandle, key_, value, attr));
    obj->SetProperties(thread_, result);
    SetFound(0, value.GetTaggedValue(), attr.GetValue(), false);
}

void ObjectOperator::DefineSetter(const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsAccessorDescriptor());
    JSHandle<AccessorData> accessor = JSHandle<AccessorData>::Cast(value_);
    accessor->SetSetter(thread_, value.GetTaggedValue());
    UpdateDataValue(JSHandle<JSObject>::Cast(receiver_), JSHandle<JSTaggedValue>::Cast(accessor), false);
}

void ObjectOperator::DefineGetter(const JSHandle<JSTaggedValue> &value)
{
    ASSERT(IsAccessorDescriptor());
    JSHandle<AccessorData> accessor = JSHandle<AccessorData>::Cast(value_);
    accessor->SetGetter(thread_, value.GetTaggedValue());
    UpdateDataValue(JSHandle<JSObject>::Cast(receiver_), JSHandle<JSTaggedValue>::Cast(accessor), false);
}
}  // namespace panda::ecmascript
