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

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSHClassTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSHClassTest, InitializeClass)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    // Call NewEcmaDynClass function set object properties
    JSHandle<JSHClass> objectDynclass =
        factory->NewEcmaDynClass(TaggedArray::SIZE, JSType::TAGGED_ARRAY, nullHandle);
    // Get object properties
    EXPECT_EQ(objectDynclass->GetLayout(), JSTaggedValue::Null());
    EXPECT_EQ(objectDynclass->GetPrototype(), JSTaggedValue::Null());
    EXPECT_EQ(objectDynclass->GetObjectType(), JSType::TAGGED_ARRAY);
    EXPECT_TRUE(objectDynclass->IsExtensible());
    EXPECT_TRUE(!objectDynclass->IsPrototype());
    EXPECT_EQ(objectDynclass->GetTransitions(), JSTaggedValue::Undefined());
    EXPECT_EQ(objectDynclass->GetProtoChangeMarker(), JSTaggedValue::Null());
    EXPECT_EQ(objectDynclass->GetProtoChangeDetails(), JSTaggedValue::Null());
    EXPECT_EQ(objectDynclass->GetEnumCache(), JSTaggedValue::Null());
}

HWTEST_F_L0(JSHClassTest, SizeFromJSHClass)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> objectDynclass = factory->NewEcmaDynClass(TaggedArray::SIZE, JSType::TAGGED_ARRAY, nullHandle);
    EXPECT_TRUE(*objectDynclass != nullptr);
    size_t objectSize;
#ifndef PANDA_TARGET_32
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 40U);
#endif
    objectDynclass = factory->NewEcmaDynClass(EcmaString::SIZE, JSType::STRING, nullHandle);
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 16U);

    objectDynclass = factory->NewEcmaDynClass(MachineCode::SIZE, JSType::MACHINE_CODE_OBJECT, nullHandle);
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 24U);
    // size is an integral multiple of eight
    objectDynclass = factory->NewEcmaDynClass(JSObject::SIZE - 1, JSType::JS_OBJECT, nullHandle);
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 56U);
    
    objectDynclass = factory->NewEcmaDynClass(JSObject::SIZE + 1, JSType::JS_OBJECT, nullHandle);
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 64U);

    objectDynclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    objectSize = objectDynclass->SizeFromJSHClass(*objectDynclass);
    EXPECT_EQ(objectSize, 64U);
}

HWTEST_F_L0(JSHClassTest, HasReferenceField)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> obj1Dynclass = factory->NewEcmaDynClass(TaggedArray::SIZE, JSType::STRING, nullHandle);
    JSHandle<JSHClass> obj2Dynclass =
        factory->NewEcmaDynClass(TaggedArray::SIZE, JSType::JS_NATIVE_POINTER, nullHandle);
    JSHandle<JSHClass> obj3Dynclass = factory->NewEcmaDynClass(TaggedArray::SIZE, JSType::JS_OBJECT, nullHandle);
    EXPECT_FALSE(obj1Dynclass->HasReferenceField());
    EXPECT_FALSE(obj2Dynclass->HasReferenceField());
    EXPECT_TRUE(obj3Dynclass->HasReferenceField());
}

HWTEST_F_L0(JSHClassTest, Clone)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
  
    JSHandle<JSHClass> objectDynclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    // withoutInlinedProperties is false
    JSHandle<JSHClass> cloneDynclass = JSHClass::Clone(thread, objectDynclass, false);
    EXPECT_TRUE(*cloneDynclass != nullptr);
    EXPECT_TRUE(objectDynclass->GetObjectSize() == cloneDynclass->GetObjectSize());
    EXPECT_EQ(cloneDynclass->GetObjectSize(), 64U); // 64 : 64 not missing the size of inlinedproperties
    EXPECT_TRUE(objectDynclass->GetLayout() == cloneDynclass->GetLayout());
    EXPECT_EQ(JSTaggedValue::SameValue(objectDynclass->GetPrototype(), cloneDynclass->GetPrototype()), true);
    EXPECT_TRUE(objectDynclass->GetBitField() == cloneDynclass->GetBitField());
    EXPECT_TRUE(objectDynclass->GetBitField1() == cloneDynclass->GetBitField1());
    EXPECT_TRUE(objectDynclass->NumberOfProps() == cloneDynclass->NumberOfProps());
    EXPECT_EQ(cloneDynclass->GetNextInlinedPropsIndex(), 0); // 0 : 0 mean index
    // withoutInlinedProperties is true
    cloneDynclass = JSHClass::Clone(thread, objectDynclass, true);
    EXPECT_TRUE(*cloneDynclass != nullptr);
    EXPECT_TRUE(objectDynclass->GetObjectSize() > cloneDynclass->GetObjectSize());
    EXPECT_EQ(cloneDynclass->GetObjectSize(), 32U); // 32 : 32 missing the size of inlinedproperties
    EXPECT_TRUE(objectDynclass->GetLayout() == cloneDynclass->GetLayout());
    EXPECT_EQ(JSTaggedValue::SameValue(objectDynclass->GetPrototype(), cloneDynclass->GetPrototype()), true);
    EXPECT_TRUE(objectDynclass->GetBitField() == cloneDynclass->GetBitField());
    EXPECT_TRUE(objectDynclass->GetBitField1() > cloneDynclass->GetBitField1());
    EXPECT_TRUE(objectDynclass->NumberOfProps() == cloneDynclass->NumberOfProps());
    EXPECT_EQ(cloneDynclass->GetNextNonInlinedPropsIndex(), 0); // 0 : 0 mean index
}

HWTEST_F_L0(JSHClassTest, TransitionElementsToDictionary)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> jsObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSHClass> objectClass(thread, jsObject->GetJSHClass());
    JSHandle<JSTaggedValue> objKey1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> objKey2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> objKey3(factory->NewFromASCII("key3"));

    JSHandle<JSTaggedValue> objValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> objValue2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> objValue3(thread, JSTaggedValue(3));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey1, objValue1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey2, objValue2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), objKey3, objValue3);
    // class is not dictionary mode
    EXPECT_FALSE(jsObject->GetJSHClass()->IsDictionaryMode());
    JSHClass::TransitionElementsToDictionary(thread, jsObject);
    auto resultDict = NameDictionary::Cast(jsObject->GetProperties().GetTaggedObject());
    EXPECT_TRUE(resultDict != nullptr);
    EXPECT_EQ(resultDict->EntriesCount(), 3); // 3 : 3 entry

    JSHandle<JSHClass> dictionaryClass(thread, jsObject->GetJSHClass());
    EXPECT_TRUE(dictionaryClass->IsDictionaryMode());
    EXPECT_EQ(dictionaryClass->GetObjectSize() + 32U, objectClass->GetObjectSize());
    EXPECT_TRUE(dictionaryClass->IsDictionaryElement());
    EXPECT_FALSE(dictionaryClass->IsStableElements());
}

static JSHandle<JSHClass> CreateJSHClass(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objectFuncPrototype = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, objectFuncPrototype);
    return hclass;
}

HWTEST_F_L0(JSHClassTest, SetPropertyOfObjHClass_001)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> accessorData(factory->NewAccessorData());
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII("key");
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> keyHandle4(factory->NewFromASCII("key4"));
    // empty layoutInfo
    JSHandle<JSHClass> parentsDynclass = CreateJSHClass(thread);

    uint32_t length = 6;
    JSHandle<TaggedArray> properties = factory->NewTaggedArray(length);
    for (int i = 0; i < static_cast<int>(length); i++) {
        if (i % 2 == 0) {
            JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
            JSHandle<EcmaString> newKey =
                factory->ConcatFromString(keyHandle, JSTaggedValue::ToString(thread, newValue));
            properties->Set(thread, i, newKey.GetTaggedValue());
            continue;
        }
        properties->Set(thread, i, accessorData.GetTaggedValue());
    }
    JSHandle<JSHClass> childDynclass = factory->SetLayoutInObjHClass(properties, 3, parentsDynclass);
    JSHandle<JSObject> childObj = factory->NewJSObject(childDynclass);

    std::vector<JSTaggedValue> keyVector;
    JSObject::GetAllKeys(childObj, keyVector);
    EXPECT_EQ(keyVector.size(), 3U);
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle0));
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle2));
    EXPECT_TRUE(JSObject::HasProperty(thread, childObj, keyHandle4));
}

HWTEST_F_L0(JSHClassTest, SetPropertyOfObjHClass_002)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSHClass> objDynclass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj1 = factory->NewJSObject(objDynclass);
    JSHandle<JSObject> Obj2 = factory->NewJSObject(objDynclass);
    PropertyAttributes attr = PropertyAttributes::Default();

    JSHandle<JSTaggedValue> keyE(factory->NewFromASCII("e"));
    JSHandle<JSTaggedValue> keyF(factory->NewFromASCII("f"));
    // not empty layoutInfo
    JSObject::SetProperty(thread, Obj1, keyE, JSHandle<JSTaggedValue>(thread, JSTaggedValue(7)));
    JSObject::SetProperty(thread, Obj2, keyF, JSHandle<JSTaggedValue>(thread, JSTaggedValue(8)));

    JSHandle<JSHClass> propertyHclass = JSHClass::SetPropertyOfObjHClass(thread, objDynclass, keyE, attr);
    JSHandle<JSHClass> obj1Class(thread, Obj1->GetClass());
    EXPECT_TRUE(propertyHclass == obj1Class);

    propertyHclass = JSHClass::SetPropertyOfObjHClass(thread, objDynclass, keyF, attr);
    JSHandle<JSHClass> obj2Class(thread, Obj2->GetClass());
    EXPECT_TRUE(propertyHclass == obj2Class);
}

HWTEST_F_L0(JSHClassTest, AddProperty)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII("key");
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes::Default();
    attr.SetIsInlinedProps(true);
    // empty layoutInfo
    JSHandle<JSHClass> objDynclass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objDynclass);
    JSHandle<JSHClass> objClass(thread, Obj->GetClass());
    EXPECT_FALSE(objDynclass != objClass);
    int keyLength = 3;
    for (int i = 0; i <keyLength; i++) {
        JSHandle<JSTaggedValue> keyValue(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> keyHandleI(
            factory->ConcatFromString(keyHandle, JSTaggedValue::ToString(thread, keyValue)));
        attr.SetOffset(i);
        JSHClass::AddProperty(thread, Obj, keyHandleI, attr);
    }
    EXPECT_TRUE(objDynclass == objClass);
    std::vector<JSTaggedValue> keyVector;
    JSObject::GetAllKeys(Obj, keyVector);
    EXPECT_EQ(keyVector.size(), 3U);
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle0));
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle1));
    EXPECT_TRUE(JSObject::HasProperty(thread, Obj, keyHandle2));
}

HWTEST_F_L0(JSHClassTest, TransitionExtension)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> preExtensionsKey = thread->GlobalConstants()->GetHandledPreventExtensionsString();
    JSHandle<JSTaggedValue> keyHandle0(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> keyHandle1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> keyHandle2(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes(0);
    attr.SetIsInlinedProps(true);
    JSHandle<JSHClass> obj1DynClass = CreateJSHClass(thread);
    JSHandle<JSHClass> obj2DynClass = CreateJSHClass(thread);
    obj2DynClass->SetExtensible(true);
    JSHandle<JSObject> Obj1 = factory->NewJSObject(obj1DynClass);
    JSHandle<JSObject> Obj2 = factory->NewJSObject(obj2DynClass);
    JSObject::SetProperty(thread, Obj2, keyHandle0, JSHandle<JSTaggedValue>(thread, JSTaggedValue(7)));
    JSObject::SetProperty(thread, Obj2, keyHandle1, JSHandle<JSTaggedValue>(thread, JSTaggedValue(8)));
    JSObject::SetProperty(thread, Obj2, keyHandle2, JSHandle<JSTaggedValue>(thread, JSTaggedValue(9)));
    // obj has key "PreventExtensions"
    JSHClass::AddProperty(thread, Obj1, preExtensionsKey, attr);
    JSHandle<JSHClass> newDynClass1 = JSHClass::TransitionExtension(thread, obj1DynClass);
    JSHandle<JSHClass> objClass(thread, Obj1->GetClass());
    EXPECT_TRUE(newDynClass1 == objClass);
    // obj has no key "PreventExtensions"
    JSHandle<JSHClass> newDynClass2 = JSHClass::TransitionExtension(thread, obj2DynClass);
    EXPECT_FALSE(newDynClass2->IsExtensible());
    JSHandle<TransitionsDictionary> dictionary(thread, obj2DynClass->GetTransitions());
    // find key
    std::vector<JSTaggedValue> keyVector;
    dictionary->GetAllKeysIntoVector(keyVector);
    EXPECT_EQ(keyVector[0], keyHandle0.GetTaggedValue());
    EXPECT_EQ(keyVector[1], preExtensionsKey.GetTaggedValue());
}

HWTEST_F_L0(JSHClassTest, TransitionProto)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env =vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> funcPrototype = env->GetFunctionPrototype();
    JSHandle<JSTaggedValue> prototypeKey = thread->GlobalConstants()->GetHandledPrototypeString();
    JSHandle<JSTaggedValue> obj1Key(factory->NewFromASCII("key0"));
    JSHandle<JSTaggedValue> obj2Key(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> obj3Key(factory->NewFromASCII("key2"));
    PropertyAttributes attr = PropertyAttributes(0);
    attr.SetIsInlinedProps(true);
    JSHandle<JSHClass> objDynClass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objDynClass);
    // obj has no key "prototype"
    JSHClass::AddProperty(thread, Obj, obj1Key, attr);
    JSHClass::AddProperty(thread, Obj, obj2Key, attr);
    JSHClass::AddProperty(thread, Obj, obj3Key, attr);
    JSHandle<JSHClass> newDynClass = JSHClass::TransitionProto(thread, objDynClass, funcPrototype);
    EXPECT_EQ(newDynClass->GetPrototype(), funcPrototype.GetTaggedValue());
    JSHandle<TransitionsDictionary> transitionDictionary(thread, objDynClass->GetTransitions());
    // find key
    std::vector<JSTaggedValue> keyVector;
    transitionDictionary->GetAllKeysIntoVector(keyVector);
    EXPECT_EQ(keyVector.size(), 2U);
    EXPECT_EQ(keyVector[0], obj1Key.GetTaggedValue());
    EXPECT_EQ(keyVector[1], prototypeKey.GetTaggedValue());
}

HWTEST_F_L0(JSHClassTest, TransitionToDictionary)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> obj1Key(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> obj2Key(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> obj3Key(factory->NewFromASCII("key3"));
    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> objDynclass = CreateJSHClass(thread);
    objDynclass->SetIsPrototype(true);
    JSHandle<JSObject> Obj0 = factory->NewJSObject(objDynclass);
    JSHandle<JSHClass> obj0Dynclass(thread, Obj0->GetJSHClass());
    JSHandle<JSObject> Obj1 = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSObject> Obj2 = JSObject::ObjectCreate(thread, Obj1);
    JSHandle<JSHClass> obj2Dynclass(thread, Obj2->GetJSHClass());
    JSHandle<JSObject> Obj3 = JSObject::ObjectCreate(thread, Obj2);
    JSObject::SetProperty(thread, Obj1, obj1Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)));
    JSObject::SetProperty(thread, Obj2, obj2Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(101)));
    JSObject::SetProperty(thread, Obj3, obj3Key, JSHandle<JSTaggedValue>(thread, JSTaggedValue(102)));
    // empty object
    JSHClass::TransitionToDictionary(thread, Obj0);
    JSHandle<JSHClass> obj0Class(thread, Obj0->GetClass());
    EXPECT_TRUE(obj0Class->GetObjectSize() < obj0Dynclass->GetObjectSize());
    EXPECT_EQ(obj0Class->NumberOfProps(), 0U);
    EXPECT_TRUE(obj0Class->IsDictionaryMode());
    EXPECT_TRUE(obj0Class->IsPrototype());
    // not empty object
    JSHandle<JSHClass> obj3Dynclass(thread, Obj3->GetJSHClass());
    JSHClass::EnableProtoChangeMarker(thread, obj3Dynclass);
    JSHClass::TransitionToDictionary(thread, Obj2);
    // refresh users
    JSHandle<JSHClass> obj1Dynclass(thread, Obj1->GetJSHClass());
    JSTaggedValue protoDetails = obj1Dynclass->GetProtoChangeDetails();
    EXPECT_TRUE(protoDetails.IsProtoChangeDetails());
    JSTaggedValue listenersValue = ProtoChangeDetails::Cast(protoDetails.GetTaggedObject())->GetChangeListener();
    JSHandle<ChangeListener> listeners(thread, listenersValue.GetTaggedObject());
    uint32_t holeIndex = ChangeListener::CheckHole(listeners);
    EXPECT_TRUE(holeIndex == 0U);
    // new class
    JSHandle<JSHClass> newClass(thread, Obj2->GetClass());
    EXPECT_TRUE(newClass->GetObjectSize() < obj2Dynclass->GetObjectSize());
    EXPECT_EQ(newClass->NumberOfProps(), 0U);
    EXPECT_TRUE(newClass->IsDictionaryMode());
    EXPECT_TRUE(newClass->IsPrototype());
}

HWTEST_F_L0(JSHClassTest, UpdatePropertyMetaData)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> objKey(factory->NewFromASCII("key0"));
    PropertyAttributes oldAttr = PropertyAttributes(0);
    PropertyAttributes newAttr = PropertyAttributes(1);
    oldAttr.SetIsInlinedProps(true);
    JSHandle<JSHClass> objDynClass = CreateJSHClass(thread);
    JSHandle<JSObject> Obj = factory->NewJSObject(objDynClass);
    // Set Transitions
    JSHClass::AddProperty(thread, Obj, objKey, oldAttr);
    // update metaData
    objDynClass->UpdatePropertyMetaData(thread, objKey.GetTaggedValue(), newAttr);
    LayoutInfo *layoutInfo = LayoutInfo::Cast(objDynClass->GetLayout().GetTaggedObject());
    EXPECT_EQ(layoutInfo->GetAttr(oldAttr.GetOffset()).GetPropertyMetaData(), newAttr.GetPropertyMetaData());
}

HWTEST_F_L0(JSHClassTest, SetPrototype)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env =vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> objectFuncPrototype = env->GetObjectFunctionPrototype();
  
    JSHandle<JSHClass> objectDynclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);
    EXPECT_EQ(objectDynclass->GetPrototype(), nullHandle.GetTaggedValue());
    objectDynclass->SetPrototype(thread, objectFuncPrototype);
    EXPECT_EQ(objectDynclass->GetPrototype(), objectFuncPrototype.GetTaggedValue());
}
}  // namespace panda::test