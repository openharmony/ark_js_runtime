/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecma_string_table.h"
#include "ecma_vm.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/builtins.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_api_deque.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_plain_array.h"
#include "ecmascript/js_api_plain_array_iterator.h"
#include "ecmascript/js_api_queue.h"
#include "ecmascript/js_api_queue_iterator.h"
#include "ecmascript/js_api_stack.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_api_tree_map.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/record.h"
#include "ecmascript/shared_mm/shared_mm.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/tagged_tree-inl.h"
#include "ecmascript/template_map.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"
#include "ecmascript/ts_types/ts_type.h"
#include "ecmascript/ts_types/ts_type_table.h"


namespace panda::ecmascript {
using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using ErrorType = base::ErrorType;
using ErrorHelper = base::ErrorHelper;

ObjectFactory::ObjectFactory(JSThread *thread, Heap *heap)
    : thread_(thread), vm_(thread->GetEcmaVM()), heap_(heap)
{
}

JSHandle<JSHClass> ObjectFactory::NewEcmaDynClassClass(JSHClass *hclass, uint32_t size, JSType type)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateDynClassClass(hclass, classSize));
    newClass->Initialize(thread_, size, type, 0);

    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::InitClassClass()
{
    JSHandle<JSHClass> dynClassClassHandle = NewEcmaDynClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *dynclass = reinterpret_cast<JSHClass *>(dynClassClassHandle.GetTaggedValue().GetTaggedObject());
    dynclass->SetClass(dynclass);
    return dynClassClassHandle;
}

JSHandle<JSHClass> ObjectFactory::NewEcmaDynClass(JSHClass *hclass, uint32_t size, JSType type, uint32_t inlinedProps)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateNonMovableOrHugeObject(hclass, classSize));
    newClass->Initialize(thread_, size, type, inlinedProps);

    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::NewEcmaDynClass(uint32_t size, JSType type, uint32_t inlinedProps)
{
    return NewEcmaDynClass(JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()),
                           size, type, inlinedProps);
}

void ObjectFactory::InitObjectFields(const TaggedObject *object)
{
    auto *klass = object->GetClass();
    auto objBodySize = klass->GetObjectSize() - TaggedObject::TaggedObjectSize();
    ASSERT(objBodySize % JSTaggedValue::TaggedTypeSize() == 0);
    int numOfFields = static_cast<int>(objBodySize / JSTaggedValue::TaggedTypeSize());
    size_t addr = reinterpret_cast<uintptr_t>(object) + TaggedObject::TaggedObjectSize();
    for (int i = 0; i < numOfFields; i++) {
        auto *fieldAddr = reinterpret_cast<JSTaggedType *>(addr + i * JSTaggedValue::TaggedTypeSize());
        *fieldAddr = JSTaggedValue::Undefined().GetRawData();
    }
}

void ObjectFactory::NewJSArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length)
{
    if (length == 0) {
        return;
    }

    JSTaggedValue data = array->GetArrayBufferData();
    if (data != JSTaggedValue::Undefined()) {
        auto *pointer = JSNativePointer::Cast(data.GetTaggedObject());
        auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(length * sizeof(uint8_t));
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_ECMA(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        pointer->ResetExternalPointer(newData);
        return;
    }

    auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(length * sizeof(uint8_t));
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator());
    array->SetArrayBufferData(thread_, pointer);
}

void ObjectFactory::NewJSSharedArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length)
{
    if (length == 0) {
        return;
    }
    void *newData = nullptr;
    JSSharedMemoryManager::GetInstance()->CreateOrLoad(&newData, length);
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_ECMA(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, JSSharedMemoryManager::RemoveSharedMemory,
                                                           JSSharedMemoryManager::GetInstance());
    array->SetArrayBufferData(thread_, pointer);
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSArrayBuffer(int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> newTarget(constructor);
    JSHandle<JSArrayBuffer> arrayBuffer(NewJSObjectByConstructor(constructor, newTarget));
    arrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(length);
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_ECMA(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                               vm_->GetNativeAreaAllocator());
        arrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        arrayBuffer->ClearBitField();
    }
    return arrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSArrayBuffer(void *buffer, int32_t length, const DeleteEntryPoint &deleter,
                                                        void *data, bool share)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> newTarget(constructor);
    JSHandle<JSArrayBuffer> arrayBuffer(NewJSObjectByConstructor(constructor, newTarget));
    length = buffer == nullptr ? 0 : length;
    arrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(buffer, deleter, data);
        arrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        arrayBuffer->SetShared(share);
    }
    return arrayBuffer;
}

JSHandle<JSDataView> ObjectFactory::NewJSDataView(JSHandle<JSArrayBuffer> buffer, uint32_t offset, uint32_t length)
{
    uint32_t arrayLength = buffer->GetArrayBufferByteLength();
    if (arrayLength - offset < length) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "offset or length error",
                                    JSHandle<JSDataView>(thread_, JSTaggedValue::Undefined()));
    }
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetDataViewFunction());
    JSHandle<JSTaggedValue> newTarget(constructor);
    JSHandle<JSDataView> arrayBuffer(NewJSObjectByConstructor(constructor, newTarget));
    arrayBuffer->SetDataView(thread_, JSTaggedValue::True());
    arrayBuffer->SetViewedArrayBuffer(thread_, buffer.GetTaggedValue());
    arrayBuffer->SetByteLength(length);
    arrayBuffer->SetByteOffset(offset);
    return arrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSSharedArrayBuffer(int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSharedArrayBufferFunction());
    JSHandle<JSTaggedValue> newTarget(constructor);
    JSHandle<JSArrayBuffer> sharedArrayBuffer(NewJSObjectByConstructor(constructor, newTarget));
    sharedArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        NewJSSharedArrayBufferData(sharedArrayBuffer, length);
        sharedArrayBuffer->SetShared(true);
    }
    return sharedArrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSSharedArrayBuffer(void *buffer, int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSharedArrayBufferFunction());
    JSHandle<JSTaggedValue> newTarget(constructor);
    JSHandle<JSArrayBuffer> sharedArrayBuffer(NewJSObjectByConstructor(constructor, newTarget));
    length = buffer == nullptr ? 0 : length;
    sharedArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(buffer, JSSharedMemoryManager::RemoveSharedMemory,
                                                               JSSharedMemoryManager::GetInstance());
        sharedArrayBuffer->SetArrayBufferData(thread_, pointer);
        sharedArrayBuffer->SetShared(true);
    }
    return sharedArrayBuffer;
}

void ObjectFactory::NewJSRegExpByteCodeData(const JSHandle<JSRegExp> &regexp, void *buffer, size_t size)
{
    if (buffer == nullptr) {
        return;
    }

    auto newBuffer = vm_->GetNativeAreaAllocator()->AllocateBuffer(size);
    if (memcpy_s(newBuffer, size, buffer, size) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    JSTaggedValue data = regexp->GetByteCodeBuffer();
    if (data != JSTaggedValue::Undefined()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(newBuffer);
        return;
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newBuffer, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator());
    regexp->SetByteCodeBuffer(thread_, pointer.GetTaggedValue());
    regexp->SetLength(static_cast<uint32_t>(size));
}

JSHandle<JSHClass> ObjectFactory::NewEcmaDynClass(uint32_t size, JSType type, const JSHandle<JSTaggedValue> &prototype)
{
    JSHandle<JSHClass> newClass = NewEcmaDynClass(size, type);
    newClass->SetPrototype(thread_, prototype.GetTaggedValue());
    return newClass;
}

JSHandle<JSObject> ObjectFactory::NewJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_, JSObject::Cast(NewDynObject(jshclass)));
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->InitializeHash();
    obj->SetElements(thread_, emptyArray, SKIP_BARRIER);
    obj->SetProperties(thread_, emptyArray, SKIP_BARRIER);
    return obj;
}

JSHandle<TaggedArray> ObjectFactory::CloneProperties(const JSHandle<TaggedArray> &old)
{
    uint32_t newLength = old->GetLength();
    if (newLength == 0) {
        return EmptyArray();
    }
    NewObjectHook();
    auto klass = old->GetClass();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(klass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }
    return newArray;
}

JSHandle<JSObject> ObjectFactory::CloneObjectLiteral(JSHandle<JSObject> object)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    JSHandle<JSObject> cloneObject = NewJSObject(klass);

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    auto newElements = CloneProperties(elements);
    cloneObject->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    auto newProperties = CloneProperties(properties);
    cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
        cloneObject->SetPropertyInlinedProps(thread_, i, object->GetPropertyInlinedProps(i));
    }
    return cloneObject;
}

JSHandle<JSArray> ObjectFactory::CloneArrayLiteral(JSHandle<JSArray> object)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    JSHandle<JSArray> cloneObject(NewJSObject(klass));
    cloneObject->SetArrayLength(thread_, object->GetArrayLength());

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    auto newElements = CopyArray(elements, elements->GetLength(), elements->GetLength());
    cloneObject->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    auto newProperties = CopyArray(properties, properties->GetLength(), properties->GetLength());
    cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
        cloneObject->SetPropertyInlinedProps(thread_, i, object->GetPropertyInlinedProps(i));
    }
    return cloneObject;
}

JSHandle<TaggedArray> ObjectFactory::CloneProperties(const JSHandle<TaggedArray> &old,
                                                     const JSHandle<JSTaggedValue> &env, const JSHandle<JSObject> &obj,
                                                     const JSHandle<JSTaggedValue> &constpool)
{
    uint32_t newLength = old->GetLength();
    if (newLength == 0) {
        return EmptyArray();
    }
    NewObjectHook();
    auto klass = old->GetClass();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(klass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
        if (!value.IsJSFunction()) {
            newArray->Set(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle, valueHandle->GetFunctionKind());
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, obj);
            newFunc->SetConstantPool(thread_, constpool);
            newArray->Set(thread_, i, newFunc);
        }
    }
    return newArray;
}

JSHandle<JSObject> ObjectFactory::CloneObjectLiteral(JSHandle<JSObject> object, const JSHandle<JSTaggedValue> &env,
                                                     const JSHandle<JSTaggedValue> &constpool, bool canShareHClass)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    if (!canShareHClass) {
        klass = JSHClass::Clone(thread_, klass);
    }

    JSHandle<JSObject> cloneObject = NewJSObject(klass);

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    auto newElements = CloneProperties(elements, env, cloneObject, constpool);
    cloneObject->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    auto newProperties = CloneProperties(properties, env, cloneObject, constpool);
    cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
        JSTaggedValue value = object->GetPropertyInlinedProps(i);
        if (!value.IsJSFunction()) {
            cloneObject->SetPropertyInlinedProps(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle, valueHandle->GetFunctionKind());
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, cloneObject);
            newFunc->SetConstantPool(thread_, constpool);
            cloneObject->SetPropertyInlinedProps(thread_, i, newFunc.GetTaggedValue());
        }
    }
    return cloneObject;
}

JSHandle<JSFunction> ObjectFactory::CloneJSFuction(JSHandle<JSFunction> obj, FunctionKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> jshclass(thread_, obj->GetJSHClass());
    JSHandle<JSFunction> cloneFunc = NewJSFunctionByDynClass(obj->GetCallTarget(), jshclass, kind);
    if (kind == FunctionKind::GENERATOR_FUNCTION) {
        JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
        JSHandle<JSObject> initialGeneratorFuncPrototype =
            NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
        JSObject::SetPrototype(thread_, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
        cloneFunc->SetProtoOrDynClass(thread_, initialGeneratorFuncPrototype);
    }

    JSTaggedValue length = obj->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
    cloneFunc->SetPropertyInlinedProps(thread_, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    return cloneFunc;
}

JSHandle<JSFunction> ObjectFactory::CloneClassCtor(JSHandle<JSFunction> ctor, const JSHandle<JSTaggedValue> &lexenv,
                                                   bool canShareHClass)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> constpool(thread_, ctor->GetConstantPool());
    JSHandle<JSHClass> hclass(thread_, ctor->GetClass());

    if (!canShareHClass) {
        hclass = JSHClass::Clone(thread_, hclass);
    }

    FunctionKind kind = ctor->GetFunctionKind();
    ASSERT_PRINT(kind == FunctionKind::CLASS_CONSTRUCTOR || kind == FunctionKind::DERIVED_CONSTRUCTOR,
                 "cloned function is not class");
    JSHandle<JSFunction> cloneCtor = NewJSFunctionByDynClass(ctor->GetCallTarget(), hclass, kind);

    for (uint32_t i = 0; i < hclass->GetInlinedProperties(); i++) {
        JSTaggedValue value = ctor->GetPropertyInlinedProps(i);
        if (!value.IsJSFunction()) {
            cloneCtor->SetPropertyInlinedProps(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle, valueHandle->GetFunctionKind());
            newFunc->SetLexicalEnv(thread_, lexenv);
            newFunc->SetHomeObject(thread_, cloneCtor);
            newFunc->SetConstantPool(thread_, constpool);
            cloneCtor->SetPropertyInlinedProps(thread_, i, newFunc.GetTaggedValue());
        }
    }

    JSHandle<TaggedArray> elements(thread_, ctor->GetElements());
    auto newElements = CloneProperties(elements, lexenv, JSHandle<JSObject>(cloneCtor), constpool);
    cloneCtor->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, ctor->GetProperties());
    auto newProperties = CloneProperties(properties, lexenv, JSHandle<JSObject>(cloneCtor), constpool);
    cloneCtor->SetProperties(thread_, newProperties.GetTaggedValue());

    cloneCtor->SetConstantPool(thread_, constpool);

    return cloneCtor;
}

JSHandle<JSObject> ObjectFactory::NewNonMovableJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_,
                           JSObject::Cast(NewNonMovableDynObject(jshclass, jshclass->GetInlinedProperties())));
    obj->SetElements(thread_, EmptyArray(), SKIP_BARRIER);
    obj->SetProperties(thread_, EmptyArray(), SKIP_BARRIER);
    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(const JSHandle<JSHClass> &dynKlass,
                                                          const JSHandle<JSTaggedValue> &object)
{
    JSHandle<JSPrimitiveRef> obj = JSHandle<JSPrimitiveRef>::Cast(NewJSObject(dynKlass));
    obj->SetValue(thread_, object);
    return obj;
}

JSHandle<JSArray> ObjectFactory::NewJSArray()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = env->GetArrayFunction();

    return JSHandle<JSArray>(NewJSObjectByConstructor(JSHandle<JSFunction>(function), function));
}

JSHandle<JSForInIterator> ObjectFactory::NewJSForinIterator(const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass(env->GetForinIteratorClass());

    JSHandle<JSForInIterator> it = JSHandle<JSForInIterator>::Cast(NewJSObject(dynclass));
    it->SetObject(thread_, obj);
    it->SetVisitedKeys(thread_->GlobalConstants()->GetEmptyTaggedQueue());
    it->SetRemainingKeys(thread_->GlobalConstants()->GetEmptyTaggedQueue());
    it->ClearBitField();
    return it;
}

JSHandle<JSHClass> ObjectFactory::CreateJSRegExpInstanceClass(JSHandle<JSTaggedValue> proto)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> regexpDynclass = NewEcmaDynClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);

    uint32_t fieldOrder = 0;
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(1);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, 0, globalConst->GetLastIndexString(), attributes);
    }

    {
        regexpDynclass->SetLayout(thread_, layoutInfoHandle);
        regexpDynclass->SetNumberOfProps(fieldOrder);
    }

    return regexpDynclass;
}

JSHandle<JSHClass> ObjectFactory::CreateJSArrayInstanceClass(JSHandle<JSTaggedValue> proto)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> arrayDynclass = NewEcmaDynClass(JSArray::SIZE, JSType::JS_ARRAY, proto);

    uint32_t fieldOrder = 0;
    ASSERT(JSArray::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(1);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, 0, globalConst->GetLengthString(), attributes);
    }

    {
        arrayDynclass->SetLayout(thread_, layoutInfoHandle);
        arrayDynclass->SetNumberOfProps(fieldOrder);
    }
    arrayDynclass->SetIsStableElements(true);
    arrayDynclass->SetHasConstructor(false);

    return arrayDynclass;
}

JSHandle<JSHClass> ObjectFactory::CreateJSArguments()
{
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    JSHandle<JSHClass> argumentsDynclass = NewEcmaDynClass(JSArguments::SIZE, JSType::JS_ARGUMENTS, proto);

    uint32_t fieldOrder = 0;
    ASSERT(JSArguments::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(JSArguments::LENGTH_OF_INLINE_PROPERTIES);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, globalConst->GetLengthString(),
                                 attributes);
    }

    ASSERT(JSArguments::ITERATOR_INLINE_PROPERTY_INDEX == fieldOrder);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 env->GetIteratorSymbol().GetTaggedValue(), attributes);
    }

    {
        ASSERT(JSArguments::CALLER_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetIsAccessor(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::CALLER_INLINE_PROPERTY_INDEX,
                                 thread_->GlobalConstants()->GetHandledCallerString().GetTaggedValue(), attributes);
    }

    {
        ASSERT(JSArguments::CALLEE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetIsAccessor(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::CALLEE_INLINE_PROPERTY_INDEX,
                                 thread_->GlobalConstants()->GetHandledCalleeString().GetTaggedValue(), attributes);
    }

    {
        argumentsDynclass->SetLayout(thread_, layoutInfoHandle);
        argumentsDynclass->SetNumberOfProps(fieldOrder);
    }
    argumentsDynclass->SetIsStableElements(true);
    return argumentsDynclass;
}

JSHandle<JSArguments> ObjectFactory::NewJSArguments()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetArgumentsClass());
    JSHandle<JSArguments> obj = JSHandle<JSArguments>::Cast(NewJSObject(dynclass));
    return obj;
}

JSHandle<JSObject> ObjectFactory::GetJSError(const ErrorType &errorType, const char *data)
{
    ASSERT_PRINT(errorType == ErrorType::ERROR || errorType == ErrorType::EVAL_ERROR ||
                     errorType == ErrorType::RANGE_ERROR || errorType == ErrorType::REFERENCE_ERROR ||
                     errorType == ErrorType::SYNTAX_ERROR || errorType == ErrorType::TYPE_ERROR ||
                     errorType == ErrorType::URI_ERROR,
                 "The error type is not in the valid range.");
    if (data != nullptr) {
        JSHandle<EcmaString> handleMsg = NewFromUtf8(data);
        return NewJSError(errorType, handleMsg);
    }
    JSHandle<EcmaString> emptyString(thread_->GlobalConstants()->GetHandledEmptyString());
    return NewJSError(errorType, emptyString);
}

JSHandle<JSObject> ObjectFactory::NewJSError(const ErrorType &errorType, const JSHandle<EcmaString> &message)
{
    // if there have exception in thread, then return current exception, no need to new js error.
    if (thread_->HasPendingException()) {
        JSHandle<JSObject> obj(thread_, thread_->GetException());
        return obj;
    }

    // current frame may be entry frame, in this case sp = the prev frame (interpreter frame).
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    if (FrameHandler(sp).GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME) {
        InterpretedFrameHandler frameHandler(sp);
        frameHandler.PrevInterpretedFrame();
        thread_->SetCurrentSPFrame(frameHandler.GetSp());
    }

    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> nativeConstructor;
    switch (errorType) {
        case ErrorType::RANGE_ERROR:
            nativeConstructor = env->GetRangeErrorFunction();
            break;
        case ErrorType::EVAL_ERROR:
            nativeConstructor = env->GetEvalErrorFunction();
            break;
        case ErrorType::REFERENCE_ERROR:
            nativeConstructor = env->GetReferenceErrorFunction();
            break;
        case ErrorType::TYPE_ERROR:
            nativeConstructor = env->GetTypeErrorFunction();
            break;
        case ErrorType::URI_ERROR:
            nativeConstructor = env->GetURIErrorFunction();
            break;
        case ErrorType::SYNTAX_ERROR:
            nativeConstructor = env->GetSyntaxErrorFunction();
            break;
        default:
            nativeConstructor = env->GetErrorFunction();
            break;
    }
    JSHandle<JSFunction> nativeFunc = JSHandle<JSFunction>::Cast(nativeConstructor);
    JSHandle<JSTaggedValue> nativePrototype(thread_, nativeFunc->GetFunctionPrototype());
    JSHandle<JSTaggedValue> ctorKey = globalConst->GetHandledConstructorString();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_, undefined, nativePrototype, undefined, 1);
    info.SetCallArg(message.GetTaggedValue());
    JSTaggedValue obj = JSFunction::Invoke(&info, ctorKey);
    JSHandle<JSObject> handleNativeInstanceObj(thread_, obj);
    return handleNativeInstanceObj;
}

JSHandle<JSObject> ObjectFactory::NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                           const JSHandle<JSTaggedValue> &newTarget)
{
    JSHandle<JSHClass> jshclass;
    if (!constructor->HasFunctionPrototype() ||
        (constructor->GetProtoOrDynClass().IsHeapObject() && constructor->GetFunctionPrototype().IsECMAObject())) {
        jshclass = JSFunction::GetInstanceJSHClass(thread_, constructor, newTarget);
    } else {
        JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
        jshclass = JSFunction::GetInstanceJSHClass(thread_, JSHandle<JSFunction>(env->GetObjectFunction()), newTarget);
    }
    // Check this exception elsewhere
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread_);

    JSHandle<JSObject> obj = NewJSObject(jshclass);
    {
        JSType type = jshclass->GetObjectType();
        switch (type) {
            case JSType::JS_OBJECT:
            case JSType::JS_ERROR:
            case JSType::JS_EVAL_ERROR:
            case JSType::JS_RANGE_ERROR:
            case JSType::JS_REFERENCE_ERROR:
            case JSType::JS_TYPE_ERROR:
            case JSType::JS_URI_ERROR:
            case JSType::JS_SYNTAX_ERROR:
            case JSType::JS_ITERATOR:
            case JSType::JS_INTL:
            case JSType::JS_LOCALE:
            case JSType::JS_DATE_TIME_FORMAT:
            case JSType::JS_NUMBER_FORMAT:
            case JSType::JS_RELATIVE_TIME_FORMAT:
            case JSType::JS_COLLATOR:
            case JSType::JS_PLURAL_RULES: {
                break;
            }
            case JSType::JS_DISPLAYNAMES: {
                JSDisplayNames::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
                JSDisplayNames::Cast(*obj)->SetType(TypednsOption::EXCEPTION);
                JSDisplayNames::Cast(*obj)->SetStyle(StyOption::EXCEPTION);
                JSDisplayNames::Cast(*obj)->SetFallback(FallbackOption::EXCEPTION);
                JSDisplayNames::Cast(*obj)->SetIcuLDN(thread_, JSTaggedValue::Undefined());
                break;
            }
            case JSType::JS_ARRAY: {
                JSArray::Cast(*obj)->SetLength(thread_, JSTaggedValue(0));
                auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
                JSArray::Cast(*obj)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
                break;
            }
            case JSType::JS_DATE:
                JSDate::Cast(*obj)->SetTimeValue(thread_, JSTaggedValue(0.0));
                JSDate::Cast(*obj)->SetLocalOffset(thread_, JSTaggedValue(JSDate::MAX_DOUBLE));
                break;
            case JSType::JS_INT8_ARRAY:
            case JSType::JS_UINT8_ARRAY:
            case JSType::JS_UINT8_CLAMPED_ARRAY:
            case JSType::JS_INT16_ARRAY:
            case JSType::JS_UINT16_ARRAY:
            case JSType::JS_INT32_ARRAY:
            case JSType::JS_UINT32_ARRAY:
            case JSType::JS_FLOAT32_ARRAY:
            case JSType::JS_FLOAT64_ARRAY:
            case JSType::JS_BIGINT64_ARRAY:
            case JSType::JS_BIGUINT64_ARRAY:
                JSTypedArray::Cast(*obj)->SetViewedArrayBuffer(thread_, JSTaggedValue::Undefined());
                JSTypedArray::Cast(*obj)->SetTypedArrayName(thread_, JSTaggedValue::Undefined());
                JSTypedArray::Cast(*obj)->SetByteLength(thread_, JSTaggedValue(0));
                JSTypedArray::Cast(*obj)->SetByteOffset(thread_, JSTaggedValue(0));
                JSTypedArray::Cast(*obj)->SetArrayLength(thread_, JSTaggedValue(0));
                JSTypedArray::Cast(*obj)->SetContentType(ContentType::None);
                break;
            case JSType::JS_REG_EXP:
                JSRegExp::Cast(*obj)->SetByteCodeBuffer(thread_, JSTaggedValue::Undefined());
                JSRegExp::Cast(*obj)->SetOriginalSource(thread_, JSTaggedValue::Undefined());
                JSRegExp::Cast(*obj)->SetOriginalFlags(thread_, JSTaggedValue(0));
                JSRegExp::Cast(*obj)->SetLength(0);
                break;
            case JSType::JS_PRIMITIVE_REF:
                JSPrimitiveRef::Cast(*obj)->SetValue(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_SET:
                JSSet::Cast(*obj)->SetLinkedSet(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_MAP:
                JSMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_WEAK_MAP:
                JSWeakMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_WEAK_SET:
                JSWeakSet::Cast(*obj)->SetLinkedSet(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_GENERATOR_OBJECT:
                JSGeneratorObject::Cast(*obj)->SetGeneratorContext(thread_, JSTaggedValue::Undefined());
                JSGeneratorObject::Cast(*obj)->SetResumeResult(thread_, JSTaggedValue::Undefined());
                JSGeneratorObject::Cast(*obj)->SetGeneratorState(JSGeneratorState::UNDEFINED);
                JSGeneratorObject::Cast(*obj)->SetResumeMode(GeneratorResumeMode::UNDEFINED);
                break;
            case JSType::JS_STRING_ITERATOR:
                JSStringIterator::Cast(*obj)->SetStringIteratorNextIndex(0);
                JSStringIterator::Cast(*obj)->SetIteratedString(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_ARRAY_BUFFER:
                JSArrayBuffer::Cast(*obj)->SetArrayBufferData(thread_, JSTaggedValue::Undefined());
                JSArrayBuffer::Cast(*obj)->SetArrayBufferByteLength(0);
                JSArrayBuffer::Cast(*obj)->ClearBitField();
                break;
            case JSType::JS_SHARED_ARRAY_BUFFER:
                JSArrayBuffer::Cast(*obj)->SetArrayBufferData(thread_, JSTaggedValue::Undefined());
                JSArrayBuffer::Cast(*obj)->SetArrayBufferByteLength(0);
                JSArrayBuffer::Cast(*obj)->SetShared(true);
                break;
            case JSType::JS_PROMISE:
                JSPromise::Cast(*obj)->SetPromiseState(PromiseState::PENDING);
                JSPromise::Cast(*obj)->SetPromiseResult(thread_, JSTaggedValue::Undefined());
                JSPromise::Cast(*obj)->SetPromiseRejectReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());
                JSPromise::Cast(*obj)->SetPromiseFulfillReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());

                JSPromise::Cast(*obj)->SetPromiseIsHandled(false);
                break;
            case JSType::JS_DATA_VIEW:
                JSDataView::Cast(*obj)->SetDataView(thread_, JSTaggedValue(false));
                JSDataView::Cast(*obj)->SetViewedArrayBuffer(thread_, JSTaggedValue::Undefined());
                JSDataView::Cast(*obj)->SetByteLength(0);
                JSDataView::Cast(*obj)->SetByteOffset(0);
                break;
            // non ECMA standard jsapi container
            case JSType::JS_API_ARRAY_LIST:
                JSAPIArrayList::Cast(*obj)->SetLength(thread_, JSTaggedValue(0));
                break;
            case JSType::JS_API_TREE_MAP:
                JSAPITreeMap::Cast(*obj)->SetTreeMap(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_API_TREE_SET:
                JSAPITreeSet::Cast(*obj)->SetTreeSet(thread_, JSTaggedValue::Undefined());
                break;
            case JSType::JS_API_QUEUE:
                JSAPIQueue::Cast(*obj)->SetLength(thread_, JSTaggedValue(0));
                JSAPIQueue::Cast(*obj)->SetFront(0);
                JSAPIQueue::Cast(*obj)->SetTail(0);
                break;
            case JSType::JS_API_PLAIN_ARRAY:
                JSAPIPlainArray::Cast(*obj)->SetLength(0);
                JSAPIPlainArray::Cast(*obj)->SetValues(thread_, JSTaggedValue(0));
                JSAPIPlainArray::Cast(*obj)->SetKeys(thread_, JSTaggedValue(0));
                break;
            case JSType::JS_API_STACK:
                JSAPIStack::Cast(*obj)->SetTop(0);
                break;
            case JSType::JS_API_DEQUE:
                JSAPIDeque::Cast(*obj)->SetFirst(0);
                JSAPIDeque::Cast(*obj)->SetLast(0);
                break;
            case JSType::JS_FUNCTION:
            case JSType::JS_GENERATOR_FUNCTION:
            case JSType::JS_FORIN_ITERATOR:
            case JSType::JS_MAP_ITERATOR:
            case JSType::JS_SET_ITERATOR:
            case JSType::JS_API_ARRAYLIST_ITERATOR:
            case JSType::JS_API_TREEMAP_ITERATOR:
            case JSType::JS_API_TREESET_ITERATOR:
            case JSType::JS_API_QUEUE_ITERATOR:
            case JSType::JS_API_DEQUE_ITERATOR:
            case JSType::JS_API_STACK_ITERATOR:
            case JSType::JS_ARRAY_ITERATOR:
            case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            default:
                UNREACHABLE();
        }
    }
    return obj;
}

FreeObject *ObjectFactory::FillFreeObject(uintptr_t address, size_t size, RemoveSlots removeSlots)
{
    FreeObject *object = nullptr;
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    if (size >= FreeObject::SIZE_OFFSET && size < FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(JSHClass::Cast(globalConst->GetFreeObjectWithOneFieldClass().GetTaggedObject()));
        object->SetNext(nullptr);
    } else if (size >= FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        bool firstHClassLoaded = false;
        if (!firstHClassLoaded && !globalConst->GetFreeObjectWithTwoFieldClass().GetRawData()) {
            object->SetClassWithoutBarrier(nullptr);
            firstHClassLoaded = true;
        } else {
            object->SetClassWithoutBarrier(
                JSHClass::Cast(globalConst->GetFreeObjectWithTwoFieldClass().GetTaggedObject()));
        }
        object->SetAvailable(size);
        object->SetNext(nullptr);
    } else if (size == FreeObject::NEXT_OFFSET) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithNoneFieldClass().GetTaggedObject()));
    } else {
        LOG_ECMA(DEBUG) << "Fill free object size is smaller";
    }

    if (removeSlots == RemoveSlots::YES) {
        Region *region = Region::ObjectAddressToRange(object);
        if (!region->InYoungGeneration()) {
            heap_->ClearSlotsRange(region, address, address + size);
        }
    }
    return object;
}

TaggedObject *ObjectFactory::NewDynObject(const JSHandle<JSHClass> &dynclass)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(*dynclass);
    uint32_t inobjPropCount = dynclass->GetInlinedProperties();
    if (inobjPropCount > 0) {
        InitializeExtraProperties(dynclass, header, inobjPropCount);
    }
    return header;
}

TaggedObject *ObjectFactory::NewNonMovableDynObject(const JSHandle<JSHClass> &dynclass, int inobjPropCount)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(*dynclass);
    if (inobjPropCount > 0) {
        InitializeExtraProperties(dynclass, header, inobjPropCount);
    }
    return header;
}

void ObjectFactory::InitializeExtraProperties(const JSHandle<JSHClass> &dynclass, TaggedObject *obj, int inobjPropCount)
{
    ASSERT(inobjPropCount * JSTaggedValue::TaggedTypeSize() < dynclass->GetObjectSize());
    auto paddr = reinterpret_cast<uintptr_t>(obj) + dynclass->GetObjectSize();
    JSTaggedType initVal = JSTaggedValue::Undefined().GetRawData();
    for (int i = 0; i < inobjPropCount; ++i) {
        paddr -= JSTaggedValue::TaggedTypeSize();
        *reinterpret_cast<JSTaggedType *>(paddr) = initVal;
    }
}

JSHandle<JSObject> ObjectFactory::OrdinaryNewJSObjectCreate(const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSTaggedValue> protoValue(proto);
    JSHandle<JSHClass> protoDyn = NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, protoValue);
    JSHandle<JSObject> newObj = NewJSObject(protoDyn);
    newObj->GetJSHClass()->SetExtensible(true);
    return newObj;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc,
                                                  FunctionKind kind)
{
    JSMethod *target = vm_->GetMethodForNativeFunction(nativeFunc);
    return NewJSFunction(env, target, kind);
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env, JSMethod *method, FunctionKind kind)
{
    JSHandle<JSHClass> dynclass;
    if (kind == FunctionKind::BASE_CONSTRUCTOR) {
        dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    } else if (JSFunction::IsConstructorKind(kind)) {
        dynclass = JSHandle<JSHClass>::Cast(env->GetConstructorFunctionClass());
    } else {
        dynclass = JSHandle<JSHClass>::Cast(env->GetNormalFunctionClass());
    }

    return NewJSFunctionByDynClass(method, dynclass, kind);
}

JSHandle<JSHClass> ObjectFactory::CreateFunctionClass(FunctionKind kind, uint32_t size, JSType type,
                                                      const JSHandle<JSTaggedValue> &prototype)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> functionClass = NewEcmaDynClass(size, type, prototype);
    {
        functionClass->SetCallable(true);
        // FunctionKind = BASE_CONSTRUCTOR
        if (JSFunction::IsConstructorKind(kind)) {
            functionClass->SetConstructor(true);
        }
        functionClass->SetExtensible(true);
    }

    uint32_t fieldOrder = 0;
    ASSERT(JSFunction::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(JSFunction::LENGTH_OF_INLINE_PROPERTIES);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetLengthString(), attributes);
        fieldOrder++;
    }

    ASSERT(JSFunction::NAME_INLINE_PROPERTY_INDEX == fieldOrder);
    // not set name in-object property on class which may have a name() method
    if (!JSFunction::IsClassConstructor(kind)) {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder,
                                 thread_->GlobalConstants()->GetHandledNameString().GetTaggedValue(), attributes);
        fieldOrder++;
    }

    if (JSFunction::HasPrototype(kind) && !JSFunction::IsClassConstructor(kind)) {
        ASSERT(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetPrototypeString(), attributes);
        fieldOrder++;
    } else if (JSFunction::IsClassConstructor(kind)) {
        ASSERT(JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetPrototypeString(), attributes);
        fieldOrder++;
    }

    {
        functionClass->SetLayout(thread_, layoutInfoHandle);
        functionClass->SetNumberOfProps(fieldOrder);
    }
    return functionClass;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunctionByDynClass(JSMethod *method, const JSHandle<JSHClass> &clazz,
                                                            FunctionKind kind)
{
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeJSFunction(thread_, function, kind);
    function->SetCallTarget(thread_, method);
    return function;
}

JSHandle<JSFunction> ObjectFactory::NewJSNativeErrorFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc)
{
    JSMethod *target = vm_->GetMethodForNativeFunction(nativeFunc);
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetNativeErrorFunctionClass());
    return NewJSFunctionByDynClass(target, dynclass, FunctionKind::BUILTIN_CONSTRUCTOR);
}

JSHandle<JSFunction> ObjectFactory::NewSpecificTypedArrayFunction(const JSHandle<GlobalEnv> &env,
                                                                  const void *nativeFunc)
{
    JSMethod *target = vm_->GetMethodForNativeFunction(nativeFunc);
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetSpecificTypedArrayFunctionClass());
    return NewJSFunctionByDynClass(target, dynclass, FunctionKind::BUILTIN_CONSTRUCTOR);
}

JSHandle<JSBoundFunction> ObjectFactory::NewJSBoundFunction(const JSHandle<JSFunctionBase> &target,
                                                            const JSHandle<JSTaggedValue> &boundThis,
                                                            const JSHandle<TaggedArray> &args)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
    JSHandle<JSHClass> dynclass = NewEcmaDynClass(JSBoundFunction::SIZE, JSType::JS_BOUND_FUNCTION, proto);

    JSHandle<JSBoundFunction> bundleFunction = JSHandle<JSBoundFunction>::Cast(NewJSObject(dynclass));
    bundleFunction->SetBoundTarget(thread_, target);
    bundleFunction->SetBoundThis(thread_, boundThis);
    bundleFunction->SetBoundArguments(thread_, args);
    dynclass->SetCallable(true);
    if (target.GetTaggedValue().IsConstructor()) {
        bundleFunction->SetConstructor(true);
    }
    JSMethod *method =
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsBoundFunction));
    bundleFunction->SetCallTarget(thread_, method);
    return bundleFunction;
}

JSHandle<JSIntlBoundFunction> ObjectFactory::NewJSIntlBoundFunction(const void *nativeFunc, int functionLength)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetJSIntlBoundFunctionClass());

    JSHandle<JSIntlBoundFunction> intlBoundFunc = JSHandle<JSIntlBoundFunction>::Cast(NewJSObject(dynclass));
    JSMethod *method = vm_->GetMethodForNativeFunction(nativeFunc);
    intlBoundFunc->SetCallTarget(thread_, method);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(intlBoundFunc);
    JSFunction::InitializeJSFunction(thread_, function, FunctionKind::NORMAL_FUNCTION);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(functionLength));
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> emptyString = globalConst->GetHandledEmptyString();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread_, emptyString, false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>::Cast(function), nameKey, nameDesc);
    return intlBoundFunc;
}

JSHandle<JSProxyRevocFunction> ObjectFactory::NewJSProxyRevocFunction(const JSHandle<JSProxy> &proxy,
                                                                      const void *nativeFunc)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetProxyRevocFunctionClass());

    JSHandle<JSProxyRevocFunction> revocFunction = JSHandle<JSProxyRevocFunction>::Cast(NewJSObject(dynclass));
    revocFunction->SetRevocableProxy(thread_, proxy);

    JSMethod *target = vm_->GetMethodForNativeFunction(nativeFunc);
    revocFunction->SetCallTarget(thread_, target);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(revocFunction);
    JSFunction::InitializeJSFunction(thread_, function, FunctionKind::NORMAL_FUNCTION);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(0));
    JSHandle<JSTaggedValue> emptyString = globalConst->GetHandledEmptyString();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread_, emptyString, false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>::Cast(function), nameKey, nameDesc);
    return revocFunction;
}

JSHandle<JSAsyncAwaitStatusFunction> ObjectFactory::NewJSAsyncAwaitStatusFunction(const void *nativeFunc)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetAsyncAwaitStatusFunctionClass());

    JSHandle<JSAsyncAwaitStatusFunction> awaitFunction =
        JSHandle<JSAsyncAwaitStatusFunction>::Cast(NewJSObject(dynclass));

    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(awaitFunction));
    JSMethod *target = vm_->GetMethodForNativeFunction(nativeFunc);
    awaitFunction->SetCallTarget(thread_, target);
    return awaitFunction;
}

JSHandle<JSFunction> ObjectFactory::NewJSGeneratorFunction(JSMethod *method)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(NewJSObject(dynclass));
    JSFunction::InitializeJSFunction(thread_, generatorFunc, FunctionKind::GENERATOR_FUNCTION);
    generatorFunc->SetCallTarget(thread_, method);
    return generatorFunc;
}

JSHandle<JSGeneratorObject> ObjectFactory::NewJSGeneratorObject(JSHandle<JSTaggedValue> generatorFunction)
{
    JSHandle<JSTaggedValue> proto(thread_, JSHandle<JSFunction>::Cast(generatorFunction)->GetProtoOrDynClass());
    if (!proto->IsECMAObject()) {
        JSHandle<GlobalEnv> realmHandle = JSObject::GetFunctionRealm(thread_, generatorFunction);
        proto = realmHandle->GetGeneratorPrototype();
    }
    JSHandle<JSHClass> dynclass = NewEcmaDynClass(JSGeneratorObject::SIZE, JSType::JS_GENERATOR_OBJECT, proto);
    JSHandle<JSGeneratorObject> generatorObject = JSHandle<JSGeneratorObject>::Cast(NewJSObject(dynclass));
    return generatorObject;
}

JSHandle<JSAsyncFunction> ObjectFactory::NewAsyncFunction(JSMethod *method)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    JSHandle<JSAsyncFunction> asyncFunction = JSHandle<JSAsyncFunction>::Cast(NewJSObject(dynclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(asyncFunction));
    asyncFunction->SetCallTarget(thread_, method);
    return asyncFunction;
}

JSHandle<JSAsyncFuncObject> ObjectFactory::NewJSAsyncFuncObject()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetInitialGenerator();
    JSHandle<JSHClass> dynclass = NewEcmaDynClass(JSAsyncFuncObject::SIZE, JSType::JS_ASYNC_FUNC_OBJECT, proto);
    JSHandle<JSAsyncFuncObject> asyncFuncObject = JSHandle<JSAsyncFuncObject>::Cast(NewJSObject(dynclass));
    return asyncFuncObject;
}

JSHandle<CompletionRecord> ObjectFactory::NewCompletionRecord(CompletionRecordType type, JSHandle<JSTaggedValue> value)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCompletionRecordClass().GetTaggedObject()));
    JSHandle<CompletionRecord> obj(thread_, header);
    obj->SetType(type);
    obj->SetValue(thread_, value);
    return obj;
}

JSHandle<GeneratorContext> ObjectFactory::NewGeneratorContext()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetGeneratorContextClass().GetTaggedObject()));
    JSHandle<GeneratorContext> obj(thread_, header);
    obj->SetRegsArray(thread_, JSTaggedValue::Undefined());
    obj->SetMethod(thread_, JSTaggedValue::Undefined());
    obj->SetAcc(thread_, JSTaggedValue::Undefined());
    obj->SetGeneratorObject(thread_, JSTaggedValue::Undefined());
    obj->SetLexicalEnv(thread_, JSTaggedValue::Undefined());
    obj->SetNRegs(0);
    obj->SetBCOffset(0);
    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(const JSHandle<JSFunction> &function,
                                                          const JSHandle<JSTaggedValue> &object)
{
    JSHandle<JSPrimitiveRef> obj(NewJSObjectByConstructor(function, JSHandle<JSTaggedValue>(function)));
    obj->SetValue(thread_, object);

    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    if (function.GetTaggedValue() == env->GetStringFunction().GetTaggedValue()) {
        JSHandle<JSTaggedValue> lengthStr = globalConst->GetHandledLengthString();

        uint32_t length = EcmaString::Cast(object.GetTaggedValue().GetTaggedObject())->GetLength();
        PropertyDescriptor desc(thread_, JSHandle<JSTaggedValue>(thread_, JSTaggedValue(length)), false, false, false);
        JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>(obj), lengthStr, desc);
    }

    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(PrimitiveType type, const JSHandle<JSTaggedValue> &object)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> function;
    switch (type) {
        case PrimitiveType::PRIMITIVE_NUMBER:
            function = env->GetNumberFunction();
            break;
        case PrimitiveType::PRIMITIVE_STRING:
            function = env->GetStringFunction();
            break;
        case PrimitiveType::PRIMITIVE_SYMBOL:
            function = env->GetSymbolFunction();
            break;
        case PrimitiveType::PRIMITIVE_BOOLEAN:
            function = env->GetBooleanFunction();
            break;
        case PrimitiveType::PRIMITIVE_BIGINT:
            function = env->GetBigIntFunction();
            break;
        default:
            break;
    }
    JSHandle<JSFunction> funcHandle(function);
    return factory->NewJSPrimitiveRef(funcHandle, object);
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSString(const JSHandle<JSTaggedValue> &str)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> stringFunc = env->GetStringFunction();

    JSHandle<JSPrimitiveRef> obj =
        JSHandle<JSPrimitiveRef>::Cast(NewJSObjectByConstructor(JSHandle<JSFunction>(stringFunc), stringFunc));
    obj->SetValue(thread_, str);
    return obj;
}

JSHandle<GlobalEnv> ObjectFactory::NewGlobalEnv(JSHClass *globalEnvClass)
{
    NewObjectHook();
    // Note: Global env must be allocated in non-movable heap, since its getters will directly return
    //       the offsets of the properties as the address of Handles.
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(globalEnvClass);
    InitObjectFields(header);
    return JSHandle<GlobalEnv>(thread_, GlobalEnv::Cast(header));
}

JSHandle<LexicalEnv> ObjectFactory::NewLexicalEnv(int numSlots)
{
    NewObjectHook();
    size_t size = LexicalEnv::ComputeSize(numSlots);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetEnvClass().GetTaggedObject()), size);
    JSHandle<LexicalEnv> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), numSlots + LexicalEnv::RESERVED_ENV_LENGTH);
    return array;
}

JSHandle<JSSymbol> ObjectFactory::NewJSSymbol()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetDescription(thread_, JSTaggedValue::Undefined());
    obj->SetFlags(0);
    obj->SetHashField(SymbolTable::Hash(obj.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateSymbol()
{
    JSHandle<JSSymbol> obj = NewJSSymbol();
    obj->SetPrivate();
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateNameSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetPrivateNameSymbol();
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewWellKnownSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetWellKnownSymbol();
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPublicSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewSymbolWithTable(const JSHandle<JSTaggedValue> &name)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<SymbolTable> tableHandle(env->GetRegisterSymbols());
    if (tableHandle->ContainsKey(name.GetTaggedValue())) {
        JSTaggedValue objValue = tableHandle->GetSymbol(name.GetTaggedValue());
        return JSHandle<JSSymbol>(thread_, objValue);
    }

    JSHandle<JSSymbol> obj = NewPublicSymbol(name);
    JSHandle<JSTaggedValue> valueHandle(obj);
    JSHandle<JSTaggedValue> keyHandle(name);
    JSHandle<SymbolTable> table = SymbolTable::Insert(thread_, tableHandle, keyHandle, valueHandle);
    env->SetRegisterSymbols(thread_, table);
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateNameSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPrivateNameSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewWellKnownSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewWellKnownSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewPublicSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPublicSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewSymbolWithTableWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewSymbolWithTable(JSHandle<JSTaggedValue>(string));
}

JSHandle<AccessorData> ObjectFactory::NewAccessorData()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetAccessorDataClass().GetTaggedObject()));
    JSHandle<AccessorData> acc(thread_, AccessorData::Cast(header));
    acc->SetGetter(thread_, JSTaggedValue::Undefined());
    acc->SetSetter(thread_, JSTaggedValue::Undefined());
    return acc;
}

JSHandle<AccessorData> ObjectFactory::NewInternalAccessor(void *setter, void *getter)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetInternalAccessorClass().GetTaggedObject()));
    JSHandle<AccessorData> obj(thread_, AccessorData::Cast(header));
    if (setter != nullptr) {
        JSHandle<JSNativePointer> setFunc = NewJSNativePointer(setter, nullptr, nullptr, true);
        obj->SetSetter(thread_, setFunc.GetTaggedValue());
    } else {
        JSTaggedValue setFunc = JSTaggedValue::Undefined();
        obj->SetSetter(thread_, setFunc);
        ASSERT(!obj->HasSetter());
    }
    JSHandle<JSNativePointer> getFunc = NewJSNativePointer(getter, nullptr, nullptr, true);
    obj->SetGetter(thread_, getFunc);
    return obj;
}

JSHandle<PromiseCapability> ObjectFactory::NewPromiseCapability()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCapabilityRecordClass().GetTaggedObject()));
    JSHandle<PromiseCapability> obj(thread_, header);
    obj->SetPromise(thread_, JSTaggedValue::Undefined());
    obj->SetResolve(thread_, JSTaggedValue::Undefined());
    obj->SetReject(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<PromiseReaction> ObjectFactory::NewPromiseReaction()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetReactionsRecordClass().GetTaggedObject()));
    JSHandle<PromiseReaction> obj(thread_, header);
    obj->SetPromiseCapability(thread_, JSTaggedValue::Undefined());
    obj->SetHandler(thread_, JSTaggedValue::Undefined());
    obj->SetType(PromiseType::RESOLVE);
    return obj;
}

JSHandle<PromiseIteratorRecord> ObjectFactory::NewPromiseIteratorRecord(const JSHandle<JSTaggedValue> &itor, bool done)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseIteratorRecordClass().GetTaggedObject()));
    JSHandle<PromiseIteratorRecord> obj(thread_, header);
    obj->SetIterator(thread_, itor.GetTaggedValue());
    obj->SetDone(done);
    return obj;
}

JSHandle<job::MicroJobQueue> ObjectFactory::NewMicroJobQueue()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMicroJobQueueClass().GetTaggedObject()));
    JSHandle<job::MicroJobQueue> obj(thread_, header);
    obj->SetPromiseJobQueue(thread_, GetEmptyTaggedQueue().GetTaggedValue());
    obj->SetScriptJobQueue(thread_, GetEmptyTaggedQueue().GetTaggedValue());
    return obj;
}

JSHandle<job::PendingJob> ObjectFactory::NewPendingJob(const JSHandle<JSFunction> &func,
                                                       const JSHandle<TaggedArray> &argv)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPendingJobClass().GetTaggedObject()));
    JSHandle<job::PendingJob> obj(thread_, header);
    obj->SetJob(thread_, func.GetTaggedValue());
    obj->SetArguments(thread_, argv.GetTaggedValue());
    return obj;
}

JSHandle<JSProxy> ObjectFactory::NewJSProxy(const JSHandle<JSTaggedValue> &target,
                                            const JSHandle<JSTaggedValue> &handler)
{
    NewObjectHook();
    TaggedObject *header = nullptr;
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();

    if (target->IsCallable()) {
        auto jsProxyCallableClass = JSHClass::Cast(globalConst->GetJSProxyCallableClass().GetTaggedObject());
        auto jsProxyConstructClass = JSHClass::Cast(globalConst->GetJSProxyConstructClass().GetTaggedObject());
        header = target->IsConstructor() ? heap_->AllocateYoungOrHugeObject(jsProxyConstructClass)
                                         : heap_->AllocateYoungOrHugeObject(jsProxyCallableClass);
    } else {
        header = heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetJSProxyOrdinaryClass().GetTaggedObject()));
    }

    JSHandle<JSProxy> proxy(thread_, header);
    JSMethod *method = nullptr;
    if (target->IsCallable()) {
        JSMethod *nativeMethod =
            vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(builtins::BuiltinsGlobal::CallJsProxy));
        proxy->SetCallTarget(thread_, nativeMethod);
    }
    proxy->SetMethod(method);

    proxy->SetTarget(thread_, target.GetTaggedValue());
    proxy->SetHandler(thread_, handler.GetTaggedValue());
    return proxy;
}

JSHandle<JSRealm> ObjectFactory::NewJSRealm()
{
    JSHandle<JSHClass> dynClassClassHandle = NewEcmaDynClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *dynclass = reinterpret_cast<JSHClass *>(dynClassClassHandle.GetTaggedValue().GetTaggedObject());
    dynclass->SetClass(dynclass);
    JSHandle<JSHClass> realmEnvClass = NewEcmaDynClass(*dynClassClassHandle, GlobalEnv::SIZE, JSType::GLOBAL_ENV);
    JSHandle<GlobalEnv> realmEnvHandle = NewGlobalEnv(*realmEnvClass);

    auto result = TemplateMap::Create(thread_);
    realmEnvHandle->SetTemplateMap(thread_, result);

    Builtins builtins;
    builtins.Initialize(realmEnvHandle, thread_);
    JSHandle<JSTaggedValue> protoValue = thread_->GlobalConstants()->GetHandledJSRealmClass();
    JSHandle<JSHClass> dynHandle = NewEcmaDynClass(JSRealm::SIZE, JSType::JS_REALM, protoValue);
    JSHandle<JSRealm> realm(NewJSObject(dynHandle));
    realm->SetGlobalEnv(thread_, realmEnvHandle.GetTaggedValue());

    JSHandle<JSTaggedValue> realmObj = realmEnvHandle->GetJSGlobalObject();
    JSHandle<JSTaggedValue> realmkey(thread_->GlobalConstants()->GetHandledGlobalString());
    PropertyDescriptor realmDesc(thread_, JSHandle<JSTaggedValue>::Cast(realmObj), true, false, true);
    [[maybe_unused]] bool status =
        JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(realm), realmkey, realmDesc);
    ASSERT_PRINT(status == true, "Realm defineOwnProperty failed");

    return realm;
}

JSHandle<TaggedArray> ObjectFactory::NewEmptyArray()
{
    NewObjectHook();
    auto header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), TaggedArray::SIZE);
    JSHandle<TaggedArray> array(thread_, header);
    array->SetLength(0);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal, bool nonMovable)
{
    if (nonMovable) {
        return NewTaggedArray(length, initVal, MemSpaceType::NON_MOVABLE);
    }
    return NewTaggedArray(length, initVal, MemSpaceType::SEMI_SPACE);
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header = nullptr;
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SEMI_SPACE:
            header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::OLD_SPACE:
            header = heap_->AllocateOldOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::NON_MOVABLE:
            header = heap_->AllocateNonMovableOrHugeObject(arrayClass, size);
            break;
        default:
            UNREACHABLE();
    }

    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(initVal, length);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(initVal, length);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewDictionaryArray(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetDictionaryClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);

    return array;
}

JSHandle<TaggedArray> ObjectFactory::ExtendArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                                 JSTaggedValue initVal)
{
    ASSERT(length > old->GetLength());
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(length);

    uint32_t oldLength = old->GetLength();
    for (uint32_t i = 0; i < oldLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }

    for (uint32_t i = oldLength; i < length; i++) {
        newArray->Set(thread_, i, initVal);
    }

    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::CopyPartArray(const JSHandle<TaggedArray> &old, uint32_t start,
                                                   uint32_t end)
{
    ASSERT(start <= end);
    ASSERT(end <= old->GetLength());

    uint32_t newLength = end - start;
    if (newLength == 0) {
        return EmptyArray();
    }

    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i + start);
        if (value.IsHole()) {
            break;
        }
        newArray->Set(thread_, i, value);
    }
    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::CopyArray(const JSHandle<TaggedArray> &old,
                                               [[maybe_unused]] uint32_t oldLength, uint32_t newLength,
                                               JSTaggedValue initVal)
{
    if (newLength == 0) {
        return EmptyArray();
    }
    if (newLength > oldLength) {
        return ExtendArray(old, newLength, initVal);
    }

    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }

    return newArray;
}

JSHandle<LayoutInfo> ObjectFactory::CreateLayoutInfo(int properties, JSTaggedValue initVal)
{
    uint32_t arrayLength = LayoutInfo::ComputeArrayLength(LayoutInfo::ComputeGrowCapacity(properties));
    JSHandle<LayoutInfo> layoutInfoHandle = JSHandle<LayoutInfo>::Cast(NewTaggedArray(arrayLength, initVal));
    layoutInfoHandle->SetNumberOfElements(thread_, 0);
    return layoutInfoHandle;
}

JSHandle<LayoutInfo> ObjectFactory::ExtendLayoutInfo(const JSHandle<LayoutInfo> &old, int properties,
                                                     JSTaggedValue initVal)
{
    ASSERT(properties > old->NumberOfElements());
    uint32_t arrayLength = LayoutInfo::ComputeArrayLength(LayoutInfo::ComputeGrowCapacity(properties));
    return JSHandle<LayoutInfo>(ExtendArray(JSHandle<TaggedArray>(old), arrayLength, initVal));
}

JSHandle<LayoutInfo> ObjectFactory::CopyLayoutInfo(const JSHandle<LayoutInfo> &old)
{
    uint32_t newLength = old->GetLength();
    return JSHandle<LayoutInfo>(CopyArray(JSHandle<TaggedArray>::Cast(old), newLength, newLength));
}

JSHandle<LayoutInfo> ObjectFactory::CopyAndReSort(const JSHandle<LayoutInfo> &old, int end, int capacity)
{
    ASSERT(capacity >= end);
    JSHandle<LayoutInfo> newArr = CreateLayoutInfo(capacity);
    Span<struct Properties> sp(old->GetProperties(), end);
    int i = 0;
    for (; i < end; i++) {
        newArr->AddKey(thread_, i, sp[i].key_, PropertyAttributes(sp[i].attr_));
    }

    return newArr;
}

JSHandle<ConstantPool> ObjectFactory::NewConstantPool(uint32_t capacity)
{
    NewObjectHook();
    if (capacity == 0) {
        return JSHandle<ConstantPool>::Cast(EmptyArray());
    }
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), capacity);
    auto header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<ConstantPool> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), capacity);
    return array;
}

JSHandle<Program> ObjectFactory::NewProgram()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProgramClass().GetTaggedObject()));
    JSHandle<Program> p(thread_, header);
    p->SetMainFunction(thread_, JSTaggedValue::Undefined());
    return p;
}

JSHandle<ModuleNamespace> ObjectFactory::NewModuleNamespace()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetModuleNamespaceClass());
    JSHandle<JSObject> obj = NewJSObject(dynclass);

    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    moduleNamespace->SetModule(thread_, JSTaggedValue::Undefined());
    moduleNamespace->SetExports(thread_, JSTaggedValue::Undefined());
    return moduleNamespace;
}

JSHandle<EcmaString> ObjectFactory::GetEmptyString() const
{
    return JSHandle<EcmaString>(thread_->GlobalConstants()->GetHandledEmptyString());
}

JSHandle<TaggedArray> ObjectFactory::EmptyArray() const
{
    return JSHandle<TaggedArray>(thread_->GlobalConstants()->GetHandledEmptyArray());
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint8_t *utf8Data, uint32_t utf8Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(utf8Data, utf8Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint16_t *utf16Data, uint32_t utf16Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf16Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(utf16Data, utf16Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(EcmaString *string) const
{
    ASSERT(string != nullptr);
    if (string->GetLength() == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(string));
}

// NB! don't do special case for C0 80, it means '\u0000', so don't convert to UTF-8
EcmaString *ObjectFactory::GetRawStringFromStringTable(const uint8_t *mutf8Data,
                                                       uint32_t utf16Len, bool canBeCompressed) const
{
    NewObjectHook();
    if (UNLIKELY(utf16Len == 0)) {
        return *GetEmptyString();
    }

    if (canBeCompressed) {
        return EcmaString::Cast(vm_->GetEcmaStringTable()->GetOrInternString(mutf8Data, utf16Len, true));
    }

    CVector<uint16_t> utf16Data(utf16Len);
    auto len = utf::ConvertRegionMUtf8ToUtf16(mutf8Data, utf16Data.data(), utf::Mutf8Size(mutf8Data), utf16Len, 0);
    return EcmaString::Cast(vm_->GetEcmaStringTable()->GetOrInternString(utf16Data.data(), len, false));
}

JSHandle<PropertyBox> ObjectFactory::NewPropertyBox(const JSHandle<JSTaggedValue> &value)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPropertyBoxClass().GetTaggedObject()));
    JSHandle<PropertyBox> box(thread_, header);
    box->SetValue(thread_, value);
    return box;
}

JSHandle<ProtoChangeMarker> ObjectFactory::NewProtoChangeMarker()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProtoChangeMarkerClass().GetTaggedObject()));
    JSHandle<ProtoChangeMarker> marker(thread_, header);
    marker->ClearBitField();
    return marker;
}

JSHandle<ProtoChangeDetails> ObjectFactory::NewProtoChangeDetails()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProtoChangeDetailsClass().GetTaggedObject()));
    JSHandle<ProtoChangeDetails> protoInfo(thread_, header);
    protoInfo->SetChangeListener(thread_, JSTaggedValue::Undefined());
    protoInfo->SetRegisterIndex(ProtoChangeDetails::UNREGISTERED);
    return protoInfo;
}

JSHandle<ProfileTypeInfo> ObjectFactory::NewProfileTypeInfo(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<ProfileTypeInfo> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);

    return array;
}

JSHandle<BigInt> ObjectFactory::NewBigInt()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    JSHandle<BigInt> obj(thread_, BigInt::Cast(header));
    obj->SetData(thread_, JSTaggedValue::Undefined());
    obj->SetSign(false);
    return obj;
}

// static
void ObjectFactory::NewObjectHook() const
{
#ifndef NDEBUG
    if (vm_->GetJSOptions().IsEnableForceGC() && vm_->IsInitialized()) {
        if (vm_->GetJSOptions().IsForceFullGC()) {
            vm_->CollectGarbage(TriggerGCType::SEMI_GC);
            vm_->CollectGarbage(TriggerGCType::OLD_GC);
            vm_->CollectGarbage(TriggerGCType::FULL_GC);
        } else {
            vm_->CollectGarbage(TriggerGCType::SEMI_GC);
            vm_->CollectGarbage(TriggerGCType::OLD_GC);
        }
    }
#endif
}

JSHandle<TaggedQueue> ObjectFactory::NewTaggedQueue(uint32_t length)
{
    uint32_t queueLength = TaggedQueue::QueueToArrayIndex(length);
    auto queue = JSHandle<TaggedQueue>::Cast(NewTaggedArray(queueLength, JSTaggedValue::Hole()));
    queue->SetStart(thread_, JSTaggedValue(0));  // equal to 0 when add 1.
    queue->SetEnd(thread_, JSTaggedValue(0));
    queue->SetCapacity(thread_, JSTaggedValue(length));

    return queue;
}

JSHandle<TaggedQueue> ObjectFactory::GetEmptyTaggedQueue() const
{
    return JSHandle<TaggedQueue>(thread_->GlobalConstants()->GetHandledEmptyTaggedQueue());
}

JSHandle<JSSetIterator> ObjectFactory::NewJSSetIterator(const JSHandle<JSSet> &set, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetSetIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSSetIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSSetIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedSet(thread_, set->GetLinkedSet());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSMapIterator> ObjectFactory::NewJSMapIterator(const JSHandle<JSMap> &map, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetMapIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSMapIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSMapIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedMap(thread_, map->GetLinkedMap());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSArrayIterator> ObjectFactory::NewJSArrayIterator(const JSHandle<JSObject> &array, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetArrayIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSArrayIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSArrayIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedArray(thread_, array);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSPromiseReactionsFunction> ObjectFactory::CreateJSPromiseReactionsFunction(const void *nativeFunc)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetPromiseReactionFunctionClass());

    JSHandle<JSPromiseReactionsFunction> reactionsFunction =
        JSHandle<JSPromiseReactionsFunction>::Cast(NewJSObject(dynclass));
    reactionsFunction->SetPromise(thread_, JSTaggedValue::Hole());
    reactionsFunction->SetAlreadyResolved(thread_, JSTaggedValue::Hole());
    JSMethod *method = vm_->GetMethodForNativeFunction(nativeFunc);
    reactionsFunction->SetCallTarget(thread_, method);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(reactionsFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(1));
    return reactionsFunction;
}

JSHandle<JSPromiseExecutorFunction> ObjectFactory::CreateJSPromiseExecutorFunction(const void *nativeFunc)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetPromiseExecutorFunctionClass());
    JSHandle<JSPromiseExecutorFunction> executorFunction =
        JSHandle<JSPromiseExecutorFunction>::Cast(NewJSObject(dynclass));
    executorFunction->SetCapability(thread_, JSTaggedValue::Hole());
    JSMethod *method = vm_->GetMethodForNativeFunction(nativeFunc);
    executorFunction->SetCallTarget(thread_, method);
    executorFunction->SetCapability(thread_, JSTaggedValue::Undefined());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(executorFunction);
    JSFunction::InitializeJSFunction(thread_, function, FunctionKind::NORMAL_FUNCTION);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(FunctionLength::TWO));
    return executorFunction;
}

JSHandle<JSPromiseAllResolveElementFunction> ObjectFactory::NewJSPromiseAllResolveElementFunction(
    const void *nativeFunc)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetPromiseAllResolveElementFunctionClass());
    JSHandle<JSPromiseAllResolveElementFunction> function =
        JSHandle<JSPromiseAllResolveElementFunction>::Cast(NewJSObject(dynclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSMethod *method = vm_->GetMethodForNativeFunction(nativeFunc);
    function->SetCallTarget(thread_, method);
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

EcmaString *ObjectFactory::InternString(const JSHandle<JSTaggedValue> &key)
{
    EcmaString *str = EcmaString::Cast(key->GetTaggedObject());
    if (str->IsInternString()) {
        return str;
    }

    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    return stringTable->GetOrInternString(str);
}

JSHandle<TransitionHandler> ObjectFactory::NewTransitionHandler()
{
    NewObjectHook();
    TransitionHandler *handler =
        TransitionHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetTransitionHandlerClass().GetTaggedObject())));
    return JSHandle<TransitionHandler>(thread_, handler);
}

JSHandle<PrototypeHandler> ObjectFactory::NewPrototypeHandler()
{
    NewObjectHook();
    PrototypeHandler *header =
        PrototypeHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetPrototypeHandlerClass().GetTaggedObject())));
    JSHandle<PrototypeHandler> handler(thread_, header);
    handler->SetHandlerInfo(thread_, JSTaggedValue::Undefined());
    handler->SetProtoCell(thread_, JSTaggedValue::Undefined());
    handler->SetHolder(thread_, JSTaggedValue::Undefined());
    return handler;
}

JSHandle<PromiseRecord> ObjectFactory::NewPromiseRecord()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseRecordClass().GetTaggedObject()));
    JSHandle<PromiseRecord> obj(thread_, header);
    obj->SetValue(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<ResolvingFunctionsRecord> ObjectFactory::NewResolvingFunctionsRecord()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseResolvingFunctionsRecordClass().GetTaggedObject()));
    JSHandle<ResolvingFunctionsRecord> obj(thread_, header);
    obj->SetResolveFunction(thread_, JSTaggedValue::Undefined());
    obj->SetRejectFunction(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<JSHClass> ObjectFactory::CreateObjectClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    uint32_t fieldOrder = 0;
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(length);
    while (fieldOrder < length) {
        key.Update(properties->Get(fieldOrder * 2));  // 2: Meaning to double
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
        PropertyAttributes attributes = PropertyAttributes::Default();

        if (properties->Get(fieldOrder * 2 + 1).IsAccessor()) {  // 2: Meaning to double
            attributes.SetIsAccessor(true);
        }

        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, key.GetTaggedValue(), attributes);
        fieldOrder++;
    }
    ASSERT(fieldOrder <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES);
    JSHandle<JSHClass> objClass = NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, fieldOrder);
    objClass->SetPrototype(thread_, proto.GetTaggedValue());
    {
        objClass->SetExtensible(true);
        objClass->SetIsLiteral(true);
        objClass->SetLayout(thread_, layoutInfoHandle);
        objClass->SetNumberOfProps(fieldOrder);
    }
    return objClass;
}

JSHandle<JSHClass> ObjectFactory::SetLayoutInObjHClass(const JSHandle<TaggedArray> &properties, size_t length,
                                                       const JSHandle<JSHClass> &objClass)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSHandle<JSHClass> newObjHclass(objClass);

    for (size_t fieldOffset = 0; fieldOffset < length; fieldOffset++) {
        key.Update(properties->Get(fieldOffset * 2)); // 2 : pair of key and value
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
        PropertyAttributes attributes = PropertyAttributes::Default();
        if (properties->Get(fieldOffset * 2 + 1).IsAccessor()) {  // 2: Meaning to double
            attributes.SetIsAccessor(true);
        }
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(fieldOffset);
        newObjHclass = JSHClass::SetPropertyOfObjHClass(thread_, newObjHclass, key, attributes);
    }
    return newObjHclass;
}

JSHandle<JSHClass> ObjectFactory::GetObjectLiteralHClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    // 64 : If object literal gets too many properties, create hclass directly.
    const int HCLASS_CACHE_SIZE = 64;
    if (length >= HCLASS_CACHE_SIZE) {
        return CreateObjectClass(properties, length);
    }
    JSHandle<JSTaggedValue> maybeCache = env->GetObjectLiteralHClassCache();
    ASSERT(length > 0);
    if (maybeCache->IsHole()) {
        JSHandle<TaggedArray> cacheArr = NewTaggedArray(HCLASS_CACHE_SIZE);
        env->SetObjectLiteralHClassCache(thread_, cacheArr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> hclassCache = env->GetObjectLiteralHClassCache();
    JSHandle<TaggedArray> hclassCacheArr = JSHandle<TaggedArray>::Cast(hclassCache);
    JSTaggedValue maybeHClass = hclassCacheArr->Get(length);
    if (maybeHClass.IsHole()) {
        JSHandle<JSHClass> objHClass = NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, length);
        objHClass->SetPrototype(thread_, proto.GetTaggedValue());
        {
            objHClass->SetNumberOfProps(0);
            objHClass->SetExtensible(true);
            objHClass->SetIsLiteral(true);
        }
        hclassCacheArr->Set(thread_, length, objHClass);
        return SetLayoutInObjHClass(properties, length, objHClass);
    }
    return SetLayoutInObjHClass(properties, length, JSHandle<JSHClass>(thread_, maybeHClass));
}

JSHandle<JSObject> ObjectFactory::GetObjectLiteralByHClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<JSHClass> dynclass = GetObjectLiteralHClass(properties, length);
    JSHandle<JSObject> obj = NewJSObject(dynclass);
    return obj;
}

JSHandle<JSObject> ObjectFactory::NewEmptyJSObject()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> builtinObj = env->GetObjectFunction();
    return NewJSObjectByConstructor(JSHandle<JSFunction>(builtinObj), builtinObj);
}

EcmaString *ObjectFactory::ResolveString(uint32_t stringId)
{
    JSMethod *caller = InterpretedFrameHandler(thread_).GetMethod();
    auto *pf = caller->GetPandaFile();
    auto id = panda_file::File::EntityId(stringId);
    auto foundStr = pf->GetStringData(id);

    return GetRawStringFromStringTable(foundStr.data, foundStr.utf16_length, foundStr.is_ascii);
}

uintptr_t ObjectFactory::NewSpaceBySnapShotAllocator(size_t size)
{
    NewObjectHook();
    return heap_->AllocateSnapShotSpace(size);
}

JSHandle<MachineCode> ObjectFactory::NewMachineCodeObject(size_t length, const uint8_t *data)
{
    NewObjectHook();
    TaggedObject *obj = heap_->AllocateMachineCodeObject(JSHClass::Cast(
        thread_->GlobalConstants()->GetMachineCodeClass().GetTaggedObject()), length + MachineCode::SIZE);
    MachineCode *code = MachineCode::Cast(obj);
    if (code == nullptr) {
        LOG_ECMA(FATAL) << "machine code cast failed";
        UNREACHABLE();
    }
    code->SetInstructionSizeInBytes(static_cast<uint32_t>(length));
    if (data != nullptr) {
        code->SetData(data, length);
    }
    JSHandle<MachineCode> codeObj(thread_, code);
    return codeObj;
}

JSHandle<ClassInfoExtractor> ObjectFactory::NewClassInfoExtractor(JSMethod *ctorMethod)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetClassInfoExtractorHClass().GetTaggedObject()));
    JSHandle<ClassInfoExtractor> obj(thread_, header);
    obj->ClearBitField();
    obj->SetConstructorMethod(ctorMethod);
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->SetPrototypeHClass(thread_, JSTaggedValue::Undefined());
    obj->SetNonStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticElements(thread_, emptyArray, SKIP_BARRIER);
    obj->SetConstructorHClass(thread_, JSTaggedValue::Undefined());
    obj->SetStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticElements(thread_, emptyArray, SKIP_BARRIER);
    return obj;
}

// ----------------------------------- new TSType ----------------------------------------
JSHandle<TSObjLayoutInfo> ObjectFactory::CreateTSObjLayoutInfo(int propNum, JSTaggedValue initVal)
{
    uint32_t arrayLength = TSObjLayoutInfo::ComputeArrayLength(propNum);
    JSHandle<TSObjLayoutInfo> tsPropInfoHandle = JSHandle<TSObjLayoutInfo>::Cast(NewTaggedArray(arrayLength, initVal));
    tsPropInfoHandle->SetNumberOfElements(thread_, 0);
    return tsPropInfoHandle;
}

JSHandle<TSObjectType> ObjectFactory::NewTSObjectType(uint32_t numOfKeys)
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSObjectTypeClass().GetTaggedObject()));
    JSHandle<TSObjectType> objectType(thread_, header);

    objectType->SetGTRef(GlobalTSTypeRef::Default());

    JSHandle<TSObjLayoutInfo> tsPropInfo = CreateTSObjLayoutInfo(numOfKeys);
    objectType->SetObjLayoutInfo(thread_, tsPropInfo);

    objectType->SetHClass(thread_, JSTaggedValue::Undefined());

    return objectType;
}

JSHandle<TSClassType> ObjectFactory::NewTSClassType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSClassTypeClass().GetTaggedObject()));
    JSHandle<TSClassType> classType(thread_, header);

    classType->SetGTRef(GlobalTSTypeRef::Default());
    classType->SetInstanceType(thread_, JSTaggedValue::Undefined());
    classType->SetConstructorType(thread_, JSTaggedValue::Undefined());
    classType->SetPrototypeType(thread_, JSTaggedValue::Undefined());
    classType->SetExtensionType(thread_, JSTaggedValue::Undefined());

    return classType;
}

JSHandle<TSInterfaceType> ObjectFactory::NewTSInterfaceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSInterfaceTypeClass().GetTaggedObject()));
    JSHandle<TSInterfaceType> interfaceType(thread_, header);

    JSHandle<TaggedArray> extends = EmptyArray();
    interfaceType->SetGTRef(GlobalTSTypeRef::Default());
    interfaceType->SetExtends(thread_, extends);
    interfaceType->SetFields(thread_, JSTaggedValue::Undefined());

    return interfaceType;
}


JSHandle<TSUnionType> ObjectFactory::NewTSUnionType(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSUnionTypeClass().GetTaggedObject()));
    JSHandle<TSUnionType> unionType(thread_, header);

    unionType->SetGTRef(GlobalTSTypeRef::Default());
    JSHandle<TaggedArray> componentTypes = NewTaggedArray(length, JSTaggedValue::Undefined());
    unionType->SetComponentTypes(thread_, componentTypes);

    return unionType;
}

JSHandle<TSClassInstanceType> ObjectFactory::NewTSClassInstanceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSClassInstanceTypeClass().GetTaggedObject()));
    JSHandle<TSClassInstanceType> classInstanceType(thread_, header);

    classInstanceType->SetGTRef(GlobalTSTypeRef::Default());
    classInstanceType->SetClassRefGT(GlobalTSTypeRef::Default());

    return classInstanceType;
}

JSHandle<TSImportType> ObjectFactory::NewTSImportType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSImportTypeClass().GetTaggedObject()));
    JSHandle<TSImportType> importType(thread_, header);

    importType->SetGTRef(GlobalTSTypeRef::Default());
    importType->SetTargetRefGT(GlobalTSTypeRef::Default());
    importType->SetImportPath(thread_, JSTaggedValue::Undefined());

    return importType;
}

JSHandle<TSFunctionType> ObjectFactory::NewTSFunctionType(uint32_t length)
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSFunctionTypeClass().GetTaggedObject()));
    JSHandle<TSFunctionType> functionType(thread_, header);

    JSHandle<TaggedArray> parameterTypes = NewTaggedArray(length + TSFunctionType::DEFAULT_LENGTH,
                                                          JSTaggedValue::Undefined());
    functionType->SetGTRef(GlobalTSTypeRef::Default());
    functionType->SetParameterTypes(thread_, parameterTypes);

    return functionType;
}

JSHandle<TSArrayType> ObjectFactory::NewTSArrayType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSArrayTypeClass().GetTaggedObject()));

    JSHandle<TSArrayType> arrayType(thread_, header);
    arrayType->SetElementTypeRef(0);

    return arrayType;
}

JSHandle<TSTypeTable> ObjectFactory::NewTSTypeTable(uint32_t length)
{
    NewObjectHook();

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length + TSTypeTable::RESERVE_TABLE_LENGTH);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    auto header = heap_->AllocateOldOrHugeObject(arrayClass, size);

    JSHandle<TSTypeTable> table(thread_, header);
    table->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length + TSTypeTable::RESERVE_TABLE_LENGTH);

    return table;
}

JSHandle<TSModuleTable> ObjectFactory::NewTSModuleTable(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    auto header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
    JSHandle<TSModuleTable> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);
    array->InitializeNumberOfTSTypeTable(thread_);

    return array;
}
// ----------------------------------- new string ----------------------------------------
JSHandle<EcmaString> ObjectFactory::NewFromASCII(const CString &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    ASSERT(EcmaString::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTable(utf8Data, data.length(), true);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(const CString &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    bool canBeCompress = EcmaString::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.length(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromStdString(const std::string &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    bool canBeCompress = EcmaString::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.size(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len)
{
    bool canBeCompress = EcmaString::CanBeCompressed(utf8Data, utf8Len);
    return GetStringFromStringTable(utf8Data, utf8Len, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool canBeCompress = EcmaString::CanBeCompressed(utf16Data, utf16Len);
    return GetStringFromStringTable(utf16Data, utf16Len, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16Compress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    ASSERT(EcmaString::CanBeCompressed(utf16Data, utf16Len));
    return GetStringFromStringTable(utf16Data, utf16Len, true);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16NotCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    ASSERT(!EcmaString::CanBeCompressed(utf16Data, utf16Len));
    return GetStringFromStringTable(utf16Data, utf16Len, false);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8Literal(const uint8_t *utf8Data, uint32_t utf8Len)
{
    NewObjectHook();
    bool canBeCompress = EcmaString::CanBeCompressed(utf8Data, utf8Len);
    return JSHandle<EcmaString>(thread_, EcmaString::CreateFromUtf8(utf8Data, utf8Len, vm_, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8LiteralCompress(const uint8_t *utf8Data, uint32_t utf8Len)
{
    NewObjectHook();
    ASSERT(EcmaString::CanBeCompressed(utf8Data, utf8Len));
    return JSHandle<EcmaString>(thread_, EcmaString::CreateFromUtf8(utf8Data, utf8Len, vm_, true));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16Literal(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    bool canBeCompress = EcmaString::CanBeCompressed(utf16Data, utf16Len);
    return JSHandle<EcmaString>(thread_, EcmaString::CreateFromUtf16(utf16Data, utf16Len, vm_, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16LiteralCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    ASSERT(EcmaString::CanBeCompressed(utf16Data, utf16Len));
    return JSHandle<EcmaString>(thread_, EcmaString::CreateFromUtf16(utf16Data, utf16Len, vm_, true));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16LiteralNotCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    ASSERT(!EcmaString::CanBeCompressed(utf16Data, utf16Len));
    return JSHandle<EcmaString>(thread_, EcmaString::CreateFromUtf16(utf16Data, utf16Len, vm_, false));
}

JSHandle<EcmaString> ObjectFactory::ConcatFromString(const JSHandle<EcmaString> &firstString,
                                                     const JSHandle<EcmaString> &secondString)
{
    if (firstString->GetLength() == 0) {
        return secondString;
    }
    if (secondString->GetLength() == 0) {
        return firstString;
    }
    return GetStringFromStringTable(firstString, secondString);
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const JSHandle<EcmaString> &firstString,
                                                             const JSHandle<EcmaString> &secondString)
{
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(firstString, secondString));
}

JSHandle<JSAPIArrayList> ObjectFactory::NewJSAPIArrayList(uint32_t capacity)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> builtinObj(thread_, thread_->GlobalConstants()->GetArrayListFunction());
    JSHandle<JSAPIArrayList> obj =
        JSHandle<JSAPIArrayList>(NewJSObjectByConstructor(JSHandle<JSFunction>(builtinObj), builtinObj));
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    obj->SetElements(thread_, factory->NewTaggedArray(capacity));

    return obj;
}

JSHandle<JSAPIArrayListIterator> ObjectFactory::NewJSAPIArrayListIterator(const JSHandle<JSAPIArrayList> &arrayList)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetArrayListIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPIArrayListIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIArrayListIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedArrayList(thread_, arrayList);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPIPlainArray> ObjectFactory::NewJSAPIPlainArray(array_size_t capacity)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> builtinObj(thread_, thread_->GlobalConstants()->GetPlainArrayFunction());

    JSHandle<JSAPIPlainArray> obj =
        JSHandle<JSAPIPlainArray>(NewJSObjectByConstructor(JSHandle<JSFunction>(builtinObj), builtinObj));
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(capacity);
    JSHandle<TaggedArray> valueArray = factory->NewTaggedArray(capacity);
    obj->SetKeys(thread_, keyArray);
    obj->SetValues(thread_, valueArray);

    return obj;
}

JSHandle<JSAPIPlainArrayIterator> ObjectFactory::NewJSAPIPlainArrayIterator(const JSHandle<JSAPIPlainArray> &plainarray,
                                                                            IterationKind kind)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetPlainArrayIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPIPlainArrayIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIPlainArrayIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedPlainArray(thread_, plainarray);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPIStackIterator> ObjectFactory::NewJSAPIStackIterator(const JSHandle<JSAPIStack> &stack)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetStackIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPIStackIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIStackIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedStack(thread_, stack);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<TaggedArray> ObjectFactory::CopyDeque(const JSHandle<TaggedArray> &old, uint32_t newLength,
                                               [[maybe_unused]] uint32_t oldLength, uint32_t first, uint32_t last)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);

    uint32_t curIndex = first;
    // newIndex use in new TaggedArray, 0 : New TaggedArray index
    uint32_t newIndex = 0;
    uint32_t oldCapacity = old->GetLength();
    newArray->SetLength(newLength);
    while (curIndex != last) {
        JSTaggedValue value = old->Get(curIndex);
        newArray->Set(thread_, newIndex, value);
        ASSERT(oldCapacity != 0);
        curIndex = (curIndex + 1) % oldCapacity;
        newIndex = newIndex + 1;
    }
    return newArray;
}

JSHandle<JSAPIDequeIterator> ObjectFactory::NewJSAPIDequeIterator(const JSHandle<JSAPIDeque> &deque)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetDequeIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPIDequeIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIDequeIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedDeque(thread_, deque);
    iter->SetNextIndex(deque->GetFirst());
    return iter;
}

JSHandle<TaggedArray> ObjectFactory::CopyQueue(const JSHandle<TaggedArray> &old, uint32_t oldLength,
                                               uint32_t newLength, [[maybe_unused]] uint32_t front,
                                               [[maybe_unused]] uint32_t tail)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);

    for (uint32_t i = 0; i < oldLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }

    return newArray;
}

JSHandle<JSAPIQueueIterator> ObjectFactory::NewJSAPIQueueIterator(const JSHandle<JSAPIQueue> &queue)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetQueueIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPIQueueIteratorClass());
    dynHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIQueueIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedQueue(thread_, queue); // IteratedQueue
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPITreeMapIterator> ObjectFactory::NewJSAPITreeMapIterator(const JSHandle<JSAPITreeMap> &map,
                                                                      IterationKind kind)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> proto(thread_, thread_->GlobalConstants()->GetTreeMapIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPITreeMapIteratorClass());
    dynHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPITreeMapIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedMap(thread_, map);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    JSHandle<TaggedTreeMap> tmap(thread_, map->GetTreeMap());
    JSHandle<TaggedArray> entries = TaggedTreeMap::GetArrayFromMap(thread_, tmap);
    iter->SetEntries(thread_, entries);
    return iter;
}

JSHandle<JSAPITreeSetIterator> ObjectFactory::NewJSAPITreeSetIterator(const JSHandle<JSAPITreeSet> &set,
                                                                      IterationKind kind)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> proto(thread_, thread_->GlobalConstants()->GetTreeSetIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> dynHandle(globalConst->GetHandledJSAPITreeSetIteratorClass());
    dynHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPITreeSetIterator> iter(NewJSObject(dynHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedSet(thread_, set);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    JSHandle<TaggedTreeSet> tset(thread_, set->GetTreeSet());
    JSHandle<TaggedArray> entries = TaggedTreeSet::GetArrayFromSet(thread_, tset);
    iter->SetEntries(thread_, entries);
    return iter;
}

JSHandle<ImportEntry> ObjectFactory::NewImportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewImportEntry(defautValue, defautValue, defautValue);
}

JSHandle<ImportEntry> ObjectFactory::NewImportEntry(const JSHandle<JSTaggedValue> &moduleRequest,
                                                    const JSHandle<JSTaggedValue> &importName,
                                                    const JSHandle<JSTaggedValue> &localName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetImportEntryClass().GetTaggedObject()));
    JSHandle<ImportEntry> obj(thread_, header);
    obj->SetModuleRequest(thread_, moduleRequest);
    obj->SetImportName(thread_, importName);
    obj->SetLocalName(thread_, localName);
    return obj;
}

JSHandle<ExportEntry> ObjectFactory::NewExportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewExportEntry(defautValue, defautValue, defautValue, defautValue);
}

JSHandle<ExportEntry> ObjectFactory::NewExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                    const JSHandle<JSTaggedValue> &moduleRequest,
                                                    const JSHandle<JSTaggedValue> &importName,
                                                    const JSHandle<JSTaggedValue> &localName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetExportEntryClass().GetTaggedObject()));
    JSHandle<ExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetModuleRequest(thread_, moduleRequest);
    obj->SetImportName(thread_, importName);
    obj->SetLocalName(thread_, localName);
    return obj;
}

JSHandle<SourceTextModule> ObjectFactory::NewSourceTextModule()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSourceTextModuleClass().GetTaggedObject()));
    JSHandle<SourceTextModule> obj(thread_, header);
    JSTaggedValue undefinedValue = thread_->GlobalConstants()->GetUndefined();
    obj->SetEnvironment(thread_, undefinedValue);
    obj->SetNamespace(thread_, undefinedValue);
    obj->SetRequestedModules(thread_, undefinedValue);
    obj->SetImportEntries(thread_, undefinedValue);
    obj->SetLocalExportEntries(thread_, undefinedValue);
    obj->SetIndirectExportEntries(thread_, undefinedValue);
    obj->SetStarExportEntries(thread_, undefinedValue);
    obj->SetNameDictionary(thread_, undefinedValue);
    obj->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetEvaluationError(SourceTextModule::UNDEFINED_INDEX);
    obj->SetStatus(ModuleStatus::UNINSTANTIATED);
    return obj;
}

JSHandle<ResolvedBinding> ObjectFactory::NewResolvedBindingRecord()
{
    JSTaggedValue undefinedValue = thread_->GlobalConstants()->GetUndefined();
    JSHandle<SourceTextModule> ecmaModule(thread_, undefinedValue);
    JSHandle<JSTaggedValue> bindingName(thread_, undefinedValue);
    return NewResolvedBindingRecord(ecmaModule, bindingName);
}

JSHandle<ResolvedBinding> ObjectFactory::NewResolvedBindingRecord(const JSHandle<SourceTextModule> &module,
                                                                  const JSHandle<JSTaggedValue> &bindingName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedBindingClass().GetTaggedObject()));
    JSHandle<ResolvedBinding> obj(thread_, header);
    obj->SetModule(thread_, module);
    obj->SetBindingName(thread_, bindingName);
    return obj;
}
}  // namespace panda::ecmascript
