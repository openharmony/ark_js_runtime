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

#ifndef ECMASCRIPT_OBJECT_OPERATOR_H
#define ECMASCRIPT_OBJECT_OPERATOR_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/property_attributes.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
class PropertyDescriptor;

enum class OperatorType : uint8_t {
    PROTOTYPE_CHAIN,
    OWN,
};

class ObjectOperator final {
public:
    explicit ObjectOperator() = default;

    explicit ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    explicit ObjectOperator(JSThread *thread, const JSHandle<JSObject> &holder, const JSHandle<JSTaggedValue> &key,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    explicit ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, const JSHandle<JSTaggedValue> &key,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    explicit ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder,
                            const JSHandle<JSTaggedValue> &receiver, const JSHandle<JSTaggedValue> &key,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);

    explicit ObjectOperator(JSThread *thread, const JSHandle<JSTaggedValue> &holder, uint32_t index,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);
    // op for fast path, name can only string and symbol, and can't be number.
    explicit ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                            OperatorType type = OperatorType::PROTOTYPE_CHAIN);
    // op for fast add
    explicit ObjectOperator(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                            const PropertyAttributes &attr);

    static void FastAdd(JSThread *thread, const JSTaggedValue &receiver, const JSTaggedValue &name,
                        const JSHandle<JSTaggedValue> &value, const PropertyAttributes &attr);

    NO_COPY_SEMANTIC(ObjectOperator);
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(ObjectOperator);
    ~ObjectOperator() = default;

    /**
     * Create ObjectOperator instance by new operator is forbidden, for the member holder is a JSHandle type. it must
     * be created and destroyed on stack
     */
    void *operator new([[maybe_unused]] size_t t) = delete;
    void operator delete([[maybe_unused]] void *ptr) = delete;

    inline bool IsFound() const
    {
        return index_ != NOT_FOUND_INDEX;
    }

    inline bool IsFastMode() const
    {
        return IsFastModeField::Get(metaData_);
    }

    inline void SetFastMode(bool flag)
    {
        IsFastModeField::Set(flag, &metaData_);
    }

    inline bool IsElement() const
    {
        return key_.IsEmpty();
    }

    inline bool IsOnPrototype() const
    {
        return IsOnPrototypeField::Get(metaData_);
    }

    inline void SetIsOnPrototype(bool flag)
    {
        IsOnPrototypeField::Set(flag, &metaData_);
    }

    inline bool HasReceiver() const
    {
        return HasReceiverField::Get(metaData_);
    }

    inline void SetHasReceiver(bool flag)
    {
        HasReceiverField::Set(flag, &metaData_);
    }

    inline bool IsTransition() const
    {
        return IsTransitionField::Get(metaData_);
    }

    inline void SetIsTransition(bool flag)
    {
        IsTransitionField::Set(flag, &metaData_);
    }

    inline PropertyAttributes GetAttr() const
    {
        return attributes_;
    }

    inline void SetAttr(uint32_t attr)
    {
        attributes_ = PropertyAttributes(attr);
    }

    inline void SetAttr(const PropertyAttributes &attr)
    {
        attributes_ = PropertyAttributes(attr);
    }

    inline bool IsPrimitiveAttr() const
    {
        return !attributes_.GetValue();
    }

    inline bool IsWritable() const
    {
        return GetAttr().IsWritable();
    }

    inline bool IsEnumerable() const
    {
        return GetAttr().IsEnumerable();
    }

    inline bool IsConfigurable() const
    {
        return GetAttr().IsConfigurable();
    }

    inline bool IsAccessorDescriptor() const
    {
        return GetAttr().IsAccessor();
    }

    inline bool IsInlinedProps() const
    {
        return GetAttr().IsInlinedProps();
    }

    inline JSTaggedValue GetValue() const
    {
        if (value_.IsEmpty()) {
            return JSTaggedValue::Undefined();
        }
        return value_.GetTaggedValue();
    }

    JSHandle<JSTaggedValue> FastGetValue();
    inline void SetValue(JSTaggedValue value)
    {
        if (value_.IsEmpty()) {
            value_ = JSMutableHandle<JSTaggedValue>(thread_, value);
        }
        value_.Update(value);
    }

    inline void SetIndex(uint32_t index)
    {
        index_ = index;
    }

    inline uint32_t GetIndex() const
    {
        return index_;
    }

    inline bool HasHolder() const
    {
        return !holder_.IsEmpty();
    }

    inline JSHandle<JSTaggedValue> GetHolder() const
    {
        return holder_;
    }

    inline JSHandle<JSTaggedValue> GetReceiver() const
    {
        return receiver_;
    }

    inline JSHandle<JSTaggedValue> GetKey() const
    {
        if (key_.IsEmpty()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return key_;
    }

    inline uint32_t GetElementIndex() const
    {
        return elementIndex_;
    }

    inline JSThread *GetThread() const
    {
        return thread_;
    }

    void ToPropertyDescriptor(PropertyDescriptor &desc) const;
    void LookupProperty();
    void GlobalLookupProperty();
    inline void ReLookupPropertyInReceiver()
    {
        ResetState();
        return LookupPropertyInlinedProps(JSHandle<JSObject>(receiver_));
    }
    inline void SetAsDefaultAttr()
    {
        SetFound(NOT_FOUND_INDEX, JSTaggedValue::Undefined(), PropertyAttributes::GetDefaultAttributes(), false, false);
    }
    bool UpdateDataValue(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                         bool isInternalAccessor, bool mayThrow = false);
    bool WriteDataPropertyInHolder(const PropertyDescriptor &desc)
    {
        JSHandle<JSObject> receiver(holder_);
        return WriteDataProperty(receiver, desc);
    }
    bool WriteDataProperty(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc);
    bool AddProperty(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value, PropertyAttributes attr);
    inline bool AddPropertyInHolder(const JSHandle<JSTaggedValue> &value, PropertyAttributes attr)
    {
        JSHandle<JSObject> obj(holder_);
        return AddProperty(obj, value, attr);
    }
    void DeletePropertyInHolder();
    static constexpr uint32_t NOT_FOUND_INDEX = std::numeric_limits<uint32_t>::max();
    static JSTaggedValue ToHolder(const JSHandle<JSTaggedValue> &holder);
    void AddPropertyInternal(const JSHandle<JSTaggedValue> &value);
    void DefineSetter(const JSHandle<JSTaggedValue> &value);
    void DefineGetter(const JSHandle<JSTaggedValue> &value);

private:
    static constexpr uint64_t ATTR_LENGTH = 5;
    static constexpr uint64_t INDEX_LENGTH = 32;

    using IsFastModeField = BitField<bool, 0, 1>;
    using IsOnPrototypeField = IsFastModeField::NextFlag;  // 1: on prototype
    using HasReceiverField = IsOnPrototypeField::NextFlag;
    using IsTransitionField = HasReceiverField::NextFlag;

    void UpdateHolder();
    void StartLookUp(OperatorType type);
    void StartGlobalLookUp(OperatorType type);
    void HandleKey(const JSHandle<JSTaggedValue> &key);
    uint32_t ComputeElementCapacity(uint32_t oldCapacity);
    void SetFound(uint32_t index, JSTaggedValue value, uint32_t attr, bool mode, bool transition = false);
    void UpdateFound(uint32_t index, uint32_t attr, bool mode, bool transition);
    void ResetState();
    inline void LookupPropertyInHolder()
    {
        JSHandle<JSObject> obj(holder_);
        LookupPropertyInlinedProps(obj);
    }
    inline void GlobalLookupPropertyInHolder()
    {
        JSHandle<JSObject> obj(holder_);
        LookupGlobal(obj);
    }
    void LookupGlobal(const JSHandle<JSObject> &obj);
    void LookupPropertyInlinedProps(const JSHandle<JSObject> &obj);
    void LookupElementInlinedProps(const JSHandle<JSObject> &obj);
    void WriteElement(const JSHandle<JSObject> &receiver, const PropertyDescriptor &desc);
    void WriteElement(const JSHandle<JSObject> &receiver, JSTaggedValue value) const;
    void DeleteElementInHolder() const;
    bool UpdateValueAndDetails(const JSHandle<JSObject> &receiver, const JSHandle<JSTaggedValue> &value,
                               PropertyAttributes attr, bool attrChanged);
    void TransitionForAttributeChanged(const JSHandle<JSObject> &receiver, PropertyAttributes attr);
    JSThread *thread_{nullptr};
    JSMutableHandle<JSTaggedValue> value_{};
    JSMutableHandle<JSTaggedValue> holder_{};
    JSMutableHandle<JSTaggedValue> receiver_{};
    JSHandle<JSTaggedValue> key_{};
    uint32_t elementIndex_{NOT_FOUND_INDEX};
    uint32_t index_{NOT_FOUND_INDEX};
    PropertyAttributes attributes_;
    uint32_t metaData_{0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_OPERATOR_H
