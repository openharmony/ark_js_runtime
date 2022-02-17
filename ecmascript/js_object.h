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

#ifndef ECMASCRIPT_JSOBJECT_H
#define ECMASCRIPT_JSOBJECT_H

#include <vector>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/runtime_trampolines.h"
#include "ecmascript/tagged_array.h"

namespace panda {
namespace ecmascript {
class ObjectOperator;

class JSFunction;
class AccessorData;
class JSArray;

class JSForInIterator;

class LexicalEnv;

// Integrity level for objects
enum IntegrityLevel { SEALED, FROZEN };

enum PositionKind { UNKNOWN = 0, INDEXED_PROPERTY = 1, INLINE_NAMED_PROPERTY = 2, OUT_NAMED_PROPERTY = 3 };
enum PropertyKind { KEY = 0, VALUE, KEY_VALUE };

// ecma6.0 6.2.4 The Property Descriptor Specification Type
class PropertyDescriptor final {
public:
    explicit PropertyDescriptor() = delete;

    ~PropertyDescriptor() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyDescriptor);
    DEFAULT_COPY_SEMANTIC(PropertyDescriptor);

    explicit PropertyDescriptor(const JSThread *thread) : thread_(thread) {}

    explicit PropertyDescriptor(const JSThread *thread, JSHandle<JSTaggedValue> v) : thread_(thread), value_(v) {}

    explicit PropertyDescriptor(const JSThread *thread, JSHandle<JSTaggedValue> v, bool w, bool e, bool c)
        : thread_(thread),
          writable_(w),
          enumerable_(e),
          configurable_(c),
          hasWritable_(true),
          hasEnumerable_(true),
          hasConfigurable_(true),
          value_(v)
    {
    }

    explicit PropertyDescriptor(const JSThread *thread, bool w, bool e, bool c)
        : PropertyDescriptor(thread, JSHandle<JSTaggedValue>(), w, e, c)
    {
    }

    inline JSHandle<JSTaggedValue> GetValue() const
    {
        if (value_.IsEmpty()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return value_;
    }

    inline void SetValue(JSHandle<JSTaggedValue> value)
    {
        value_ = value;
    }

    inline bool IsWritable() const
    {
        return writable_;
    }

    inline void SetWritable(bool flag)
    {
        writable_ = flag;
        hasWritable_ = true;
    }

    inline bool IsEnumerable() const
    {
        return enumerable_;
    }

    inline void SetEnumerable(bool flag)
    {
        enumerable_ = flag;
        hasEnumerable_ = true;
    }

    inline bool IsConfigurable() const
    {
        return configurable_;
    }

    inline void SetConfigurable(bool flag)
    {
        configurable_ = flag;
        hasConfigurable_ = true;
    }

    inline bool HasValue() const
    {
        return !value_.IsEmpty();
    }

    inline bool HasWritable() const
    {
        return hasWritable_;
    }

    inline bool HasConfigurable() const
    {
        return hasConfigurable_;
    }

    inline bool HasEnumerable() const
    {
        return hasEnumerable_;
    }

    inline bool HasGetter() const
    {
        return !getter_.IsEmpty();
    }

    inline bool HasSetter() const
    {
        return !setter_.IsEmpty();
    }

    inline JSHandle<JSTaggedValue> GetGetter() const
    {
        if (getter_->IsNull()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return getter_;
    }

    inline JSHandle<JSTaggedValue> GetSetter() const
    {
        if (setter_->IsNull()) {
            return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        }
        return setter_;
    }

    inline void SetGetter(JSHandle<JSTaggedValue> value)
    {
        getter_ = value;
    }

    inline void SetSetter(JSHandle<JSTaggedValue> value)
    {
        setter_ = value;
    }

    // 6.2.4.1
    inline bool IsAccessorDescriptor() const
    {
        // 2. If both Desc.[[Get]] and Desc.[[Set]] are absent, return false.
        return !(getter_.IsEmpty() && setter_.IsEmpty());
    }

    inline bool IsDataDescriptor() const
    {
        // 2. If both Desc.[[Value]] and Desc.[[Writable]] are absent, return false.
        return !(value_.IsEmpty() && !hasWritable_);
    }

    inline bool IsGenericDescriptor() const
    {
        // 2. If IsAccessorDescriptor(Desc) and IsDataDescriptor(Desc) are both false, return true
        return !IsAccessorDescriptor() && !IsDataDescriptor();
    }

    inline bool IsEmpty() const
    {
        return !hasWritable_ && !hasEnumerable_ && !hasConfigurable_ && !HasValue() && !HasGetter() && !HasSetter();
    }

    static void CompletePropertyDescriptor(const JSThread *thread, PropertyDescriptor &desc);

private:
    const JSThread *thread_{nullptr};

    bool writable_ {false};
    bool enumerable_ {false};
    bool configurable_ {false};
    bool hasWritable_ {false};
    bool hasEnumerable_ {false};
    bool hasConfigurable_ {false};

    JSHandle<JSTaggedValue> value_ {};
    JSHandle<JSTaggedValue> getter_ {};
    JSHandle<JSTaggedValue> setter_ {};
};

enum class ElementTypes { ALLTYPES, STRING_AND_SYMBOL };

class PropertyMetaData {
public:
    using IsFoundField = BitField<bool, 0, 1>;
    using IsInlinedPropsField = IsFoundField::NextFlag;
    using RepresentationField = IsInlinedPropsField::NextField<Representation, 3>;
    using OffsetField = RepresentationField::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;

    explicit PropertyMetaData(uint32_t metaData) : metaData_(metaData) {}

    ~PropertyMetaData() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PropertyMetaData);
    DEFAULT_COPY_SEMANTIC(PropertyMetaData);

    explicit PropertyMetaData(bool isFound)
    {
        SetFound(isFound);
    }

    inline bool IsFound() const
    {
        return IsFoundField::Get(metaData_);
    }

    inline void SetFound(bool flag)
    {
        IsFoundField::Set(flag, &metaData_);
    }

    inline bool GetIsInlinedProps() const
    {
        return IsInlinedPropsField::Get(metaData_);
    }

    inline void SetIsInlinedProps(bool flag)
    {
        IsInlinedPropsField::Set(flag, &metaData_);
    }

    inline Representation GetRepresentation() const
    {
        return RepresentationField::Get(metaData_);
    }

    inline void SetRepresentation(Representation representation)
    {
        RepresentationField::Set<uint32_t>(representation, &metaData_);
    }

    inline void SetOffset(uint32_t offset)
    {
        OffsetField::Set<uint32_t>(offset, &metaData_);
    }

    inline uint32_t GetOffset() const
    {
        return OffsetField::Get(metaData_);
    }

private:
    uint32_t metaData_{0};
};

class OperationResult {
public:
    explicit OperationResult(const JSThread *thread, JSTaggedValue value, PropertyMetaData metaData)
        : metaData_(metaData)
    {
        thread_ = thread;
        value_ = JSHandle<JSTaggedValue>(thread_, value);
    }

    ~OperationResult() = default;
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(OperationResult);
    DEFAULT_COPY_SEMANTIC(OperationResult);

    JSHandle<JSTaggedValue> GetValue() const
    {
        if (value_->IsPropertyBox()) {
            return JSHandle<JSTaggedValue>(thread_,
                                           PropertyBox::Cast(value_.GetTaggedValue().GetTaggedObject())->GetValue());
        }
        return value_;
    }

    JSHandle<JSTaggedValue> GetRawValue() const
    {
        return value_;
    }

    const PropertyMetaData &GetPropertyMetaData() const
    {
        return metaData_;
    }

private:
    const JSThread *thread_ {nullptr};
    JSHandle<JSTaggedValue> value_ {};
    PropertyMetaData metaData_{0U};
};

class ECMAObject : public TaggedObject {
public:
    CAST_CHECK(ECMAObject, IsECMAObject);

    void SetBuiltinsCtorMode();
    bool IsBuiltinsConstructor() const;
    void SetCallable(bool flag);
    bool IsCallable() const;
    JSMethod *GetCallTarget() const;

    static constexpr size_t HASH_OFFSET = TaggedObjectSize();
    static constexpr size_t SIZE = HASH_OFFSET + sizeof(JSTaggedType);

    void SetHash(int32_t hash);
    int32_t GetHash() const;
    void InitializeHash()
    {
        Barriers::SetDynPrimitive<JSTaggedType>(this, ECMAObject::HASH_OFFSET, JSTaggedValue(0).GetRawData());
    }

    void* GetNativePointerField(int32_t index) const;
    void SetNativePointerField(int32_t index, void *nativePointer,
        const DeleteEntryPoint &callBack, void *data);
    int32_t GetNativePointerFieldCount() const;
    void SetNativePointerFieldCount(int32_t count);

    DECL_VISIT_OBJECT(HASH_OFFSET, SIZE);

    void VisitObjects(const EcmaObjectRangeVisitor &visitor)
    {
        // no field in this object
        VisitRangeSlot(visitor);
    }
};

class JSObject : public ECMAObject {
public:
    static constexpr int MIN_ELEMENTS_LENGTH = 3;
    static constexpr int MIN_PROPERTIES_LENGTH = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS;
    static constexpr int PROPERTIES_GROW_SIZE = 4;
    static constexpr int FAST_ELEMENTS_FACTOR = 3;
    static constexpr int MIN_GAP = 256;
    static constexpr int MAX_GAP = 1024;
    static constexpr uint32_t MAX_ELEMENT_INDEX = std::numeric_limits<uint32_t>::max();

    CAST_CHECK(JSObject, IsECMAObject);
    CAST_CHECK_TAGGEDVALUE(JSObject, IsECMAObject);

    // ecma6.0 6.2.4.4
    static JSHandle<JSTaggedValue> FromPropertyDescriptor(JSThread *thread, const PropertyDescriptor &desc);

    // ecma6.0 6.2.4.5 ToPropertyDescriptor ( Obj )
    static void ToPropertyDescriptor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc);
    static bool ToPropertyDescriptorFast(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                         PropertyDescriptor &desc);

    // ecma6 7.3 Operations on Objects
    static JSHandle<JSTaggedValue> GetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                             const JSHandle<JSTaggedValue> &key);

    static bool CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value);

    static bool CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                   const JSHandle<JSTaggedValue> &value);

    static bool CreateMethodProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static bool CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj,
                                          const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    static bool CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                          const JSHandle<JSTaggedValue> &value);

    static JSHandle<TaggedArray> EnumerableOwnNames(JSThread *thread, const JSHandle<JSObject> &obj);

    // 7.3.23 EnumerableOwnPropertyNames ( O, kind )
    static JSHandle<TaggedArray> EnumerableOwnPropertyNames(JSThread *thread, const JSHandle<JSObject> &obj,
                                                            PropertyKind kind);

    static JSHandle<GlobalEnv> GetFunctionRealm(JSThread *thread, const JSHandle<JSTaggedValue> &object);

    static bool SetIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level);

    static bool TestIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level);

    static JSHandle<JSTaggedValue> SpeciesConstructor(JSThread *thread, const JSHandle<JSObject> &obj,
                                                      const JSHandle<JSTaggedValue> &defaultConstructort);
    // 7.3.17
    template<ElementTypes types = ElementTypes::ALLTYPES>
    static JSHandle<JSTaggedValue> CreateListFromArrayLike(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    // emca6 9.1
    // [[GetPrototypeOf]]
    JSTaggedValue GetPrototype(JSThread *thread) const;

    // [[SetPrototypeOf]]
    static bool SetPrototype(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &proto);

    // [[IsExtensible]]
    bool IsExtensible() const;

    // [[PreventExtensions]]
    static bool PreventExtensions(JSThread *thread, const JSHandle<JSObject> &obj);

    // [[GetOwnProperty]] -> Undefined | Property Descriptor
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);

    static bool GlobalGetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);

    static bool OrdinaryGetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc);

    // [[DefineOwnProperty]]
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const PropertyDescriptor &desc);

    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                  const PropertyDescriptor &desc);

    static bool OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                          const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);

    static bool OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                          const PropertyDescriptor &desc);

    static bool IsCompatiblePropertyDescriptor(bool extensible, const PropertyDescriptor &desc,
                                               const PropertyDescriptor &current);

    static bool ValidateAndApplyPropertyDescriptor(ObjectOperator *op, bool extensible, const PropertyDescriptor &desc,
                                                   const PropertyDescriptor &current);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                       const JSHandle<JSTaggedValue> &key);

    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index);

    static OperationResult GetPropertyFromGlobal(JSThread *thread, const JSHandle<JSTaggedValue> &key);

    static bool SetProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver,
                            bool mayThrow = false);

    static bool SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                            const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    static bool GlobalSetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                  const JSHandle<JSTaggedValue> &value, bool mayThrow);

    // [[HasProperty]]
    static bool HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key);

    static bool HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index);

    // 9.1.10 [[Delete]]
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key);

    // [[OwnPropertyKeys]]
    static JSHandle<TaggedArray> GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj);

    // 9.1.13 ObjectCreate
    static JSHandle<JSObject> ObjectCreate(JSThread *thread, const JSHandle<JSObject> &proto);

    // 12.9.4 Runtime Semantics: InstanceofOperator(O, C)
    static bool InstanceOf(JSThread *thread, const JSHandle<JSTaggedValue> &object,
                           const JSHandle<JSTaggedValue> &target);

    // 13.7.5.15 EnumerateObjectProperties ( O ); same as [[Enumerate]]
    static JSHandle<JSForInIterator> EnumerateObjectProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    static bool IsRegExp(JSThread *thread, const JSHandle<JSTaggedValue> &argument);

    static JSTaggedValue CallGetter(JSThread *thread, const AccessorData *accessor,
                                    const JSHandle<JSTaggedValue> &receiver);
    static bool CallSetter(JSThread *thread, const AccessorData &accessor, const JSHandle<JSTaggedValue> &receiver,
                           const JSHandle<JSTaggedValue> &value, bool mayThrow = false);

    void FillElementsWithHoles(const JSThread *thread, uint32_t start, uint32_t end);

    JSHClass *GetJSHClass() const;
    bool IsJSGlobalObject() const;
    bool IsConstructor() const;
    bool IsECMAObject() const;
    bool IsJSError() const;
    bool IsArguments() const;
    bool IsDate() const;
    bool IsJSArray() const;
    bool IsJSMap() const;
    bool IsJSSet() const;
    bool IsJSRegExp() const;
    bool IsJSFunction() const;
    bool IsBoundFunction() const;
    bool IsJSIntlBoundFunction() const;
    bool IsProxyRevocFunction() const;
    bool IsAccessorData() const;
    bool IsJSGlobalEnv() const;
    bool IsJSProxy() const;
    bool IsGeneratorObject() const;
    bool IsForinIterator() const;
    bool IsJSSetIterator() const;
    bool IsJSMapIterator() const;
    bool IsJSArrayIterator() const;
    bool IsJSPrimitiveRef() const;
    bool IsElementDict() const;
    bool IsPropertiesDict() const;
    bool IsTypedArray() const;

    static void DefinePropertyByLiteral(JSThread *thread, const JSHandle<JSObject> &obj,
                                        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                        bool useForClass = false);
    static void DefineSetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value);
    static void DefineGetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                             const JSHandle<JSTaggedValue> &value);
    static JSHandle<JSObject> CreateObjectFromProperties(const JSThread *thread,
                                                         const JSHandle<TaggedArray> &properties);
    static void GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                           const JSHandle<TaggedArray> &keyArray);
    static void GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj,
                           std::vector<JSTaggedValue> &keyVector);
    static void GetAllElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                  const JSHandle<TaggedArray> &keyArray);
    static void GetALLElementKeysIntoVector(const JSThread *thread, const JSHandle<JSObject> &obj,
                                            std::vector<JSTaggedValue> &keyVector);
    uint32_t GetNumberOfKeys();
    uint32_t GetNumberOfElements();

    static JSHandle<TaggedArray> GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                    uint32_t numOfElements, uint32_t *keys);
    static JSHandle<TaggedArray> GetAllEnumKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                uint32_t numOfKeys, uint32_t *keys);

    static void AddAccessor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<AccessorData> &value, PropertyAttributes attr);

    static constexpr size_t PROPERTIES_OFFSET = ECMAObject::SIZE;

    ACCESSORS(Properties, PROPERTIES_OFFSET, ELEMENTS_OFFSET);
    ACCESSORS(Elements, ELEMENTS_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(ECMAObject, PROPERTIES_OFFSET, SIZE)

    DECL_DUMP()

    static JSHandle<NameDictionary> TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &receiver);

    inline void SetPropertyInlinedProps(const JSThread *thread, uint32_t index, JSTaggedValue value);
    inline void SetPropertyInlinedProps(const JSThread *thread, const JSHClass *hclass, uint32_t index,
                                        JSTaggedValue value);
    inline JSTaggedValue GetPropertyInlinedProps(uint32_t index) const;
    inline JSTaggedValue GetPropertyInlinedProps(const JSHClass *hclass, uint32_t index) const;
    inline JSTaggedValue GetProperty(const JSHClass *hclass, PropertyAttributes attr) const;
    inline void SetProperty(const JSThread *thread, const JSHClass *hclass, PropertyAttributes attr,
                            JSTaggedValue value);

    static bool IsArrayLengthWritable(JSThread *thread, const JSHandle<JSObject> &receiver);
    bool UpdatePropertyInDictionary(const JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static bool ShouldTransToDict(uint32_t capacity, uint32_t index);
    static JSHandle<TaggedArray> GrowElementsCapacity(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                      uint32_t capacity);

protected:
    static void ElementsToDictionary(const JSThread *thread, JSHandle<JSObject> obj);

private:
    friend class ObjectOperator;
    friend class LoadICRuntime;
    friend class StoreICRuntime;
    friend class FastRuntimeStub;
    friend class ICRuntimeStub;
    friend class RuntimeTrampolines;

    static bool AddElementInternal(
        JSThread *thread, const JSHandle<JSObject> &receiver, uint32_t index, const JSHandle<JSTaggedValue> &value,
        PropertyAttributes attr = PropertyAttributes(PropertyAttributes::GetDefaultAttributes()));

    static JSTaggedValue GetProperty(JSThread *thread, ObjectOperator *op);
    static bool SetProperty(ObjectOperator *op, const JSHandle<JSTaggedValue> &value, bool mayThrow);
    static void DeletePropertyInternal(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, uint32_t index);
    int FindProperty(const JSHandle<JSTaggedValue> &key);

    static uint32_t ComputeElementCapacity(uint32_t oldCapacity);
    static uint32_t ComputePropertyCapacity(uint32_t oldCapacity);

    static JSTaggedValue ShouldGetValueFromBox(ObjectOperator *op);
};
}  // namespace ecmascript
}  // namespace panda

#endif
