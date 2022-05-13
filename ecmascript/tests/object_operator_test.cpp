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

#include "ecmascript/object_operator.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ObjectOperatorTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(ObjectOperatorTest, SetAttr)
{
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(2));

    ObjectOperator objectOperator1(thread, handleValue1);
    ObjectOperator objectOperator2(thread, handleValue2);
    uint32_t handleAttr1 = 2;

    objectOperator1.SetAttr(handleAttr1);
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 2);
    EXPECT_FALSE(objectOperator1.IsPrimitiveAttr());

    PropertyAttributes handleAttr2(JSTaggedValue(0));
    objectOperator2.SetAttr(handleAttr2);
    EXPECT_TRUE(objectOperator2.IsPrimitiveAttr());
}

HWTEST_F_L0(ObjectOperatorTest, SetFastMode)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.IsFastMode());
    objectOperator.SetFastMode(false);
    EXPECT_FALSE(objectOperator.IsFastMode());

    objectOperator.SetFastMode(true);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, SetIsOnPrototype)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_TRUE(objectOperator.IsOnPrototype());
    objectOperator.SetIsOnPrototype(true);
    EXPECT_TRUE(objectOperator.IsOnPrototype());

    objectOperator.SetIsOnPrototype(false);
    EXPECT_FALSE(objectOperator.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, SetHasReceiver)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.HasReceiver());
    objectOperator.SetHasReceiver(true);
    EXPECT_TRUE(objectOperator.HasReceiver());

    objectOperator.SetHasReceiver(false);
    EXPECT_FALSE(objectOperator.HasReceiver());
}

HWTEST_F_L0(ObjectOperatorTest, SetIsTransition)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.IsTransition());
    objectOperator.SetIsTransition(true);
    EXPECT_TRUE(objectOperator.IsTransition());

    objectOperator.SetIsTransition(false);
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, SetIndex)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    uint32_t index = 1;
    objectOperator.SetIndex(index);
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_TRUE(objectOperator.IsFound());

    objectOperator.SetIndex(ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_FALSE(objectOperator.IsFound());
}

HWTEST_F_L0(ObjectOperatorTest, SetValue)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(3));
    ObjectOperator objectOperator(thread, handleKey);

    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    objectOperator.SetValue(handleValue1.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);

    objectOperator.SetValue(handleValue2.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 3);
}

HWTEST_F_L0(ObjectOperatorTest, SetAsDefaultAttr)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleKey);
    objectOperator.SetAsDefaultAttr();
    EXPECT_EQ(objectOperator.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
}

HWTEST_F_L0(ObjectOperatorTest, GetHolder)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    
    JSHandle<JSObject> handleHolder = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleHolder, handleKey);

    EXPECT_TRUE(objectOperator.HasHolder());
    EXPECT_TRUE(objectOperator.GetHolder().GetTaggedValue().IsECMAObject());
}

HWTEST_F_L0(ObjectOperatorTest, GetReceiver)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    
    JSHandle<EcmaString> handleNameString = factory->NewFromASCII("name");
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(1));

    ObjectOperator objectOperator(thread, handleReceiver.GetTaggedValue(), handleNameString.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetReceiver()->GetInt(), 1);
}

HWTEST_F_L0(ObjectOperatorTest, GetElementIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2.0));
    JSHandle<JSTaggedValue> handleKey3(factory->NewFromASCII("2"));

    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);
    ObjectOperator objectOperator3(thread, handleKey3);

    EXPECT_EQ(objectOperator1.GetElementIndex(), 1U);
    EXPECT_EQ(objectOperator2.GetElementIndex(), 2U);
    EXPECT_EQ(objectOperator3.GetElementIndex(), 2U);
}

HWTEST_F_L0(ObjectOperatorTest, GetKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("name"));

    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);

    EXPECT_TRUE(objectOperator1.GetKey()->IsUndefined());
    EXPECT_TRUE(objectOperator1.IsElement());

    EXPECT_FALSE(objectOperator2.GetKey()->IsUndefined());
    EXPECT_FALSE(objectOperator2.IsElement());
}

HWTEST_F_L0(ObjectOperatorTest, GetThread)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleKey);
    EXPECT_TRUE(objectOperator.GetThread() != nullptr);
}

JSTaggedValue TestDefinedGetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(12);
}

JSTaggedValue TestDefinedSetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(12);
}

JSTaggedValue TestBoolSetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(JSTaggedValue::True());
}

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(ObjectOperatorTest, ToPropertyDescriptor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("property"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<AccessorData> handleAccessor = factory->NewAccessorData();
    JSHandle<AccessorData> handleInternalAccessor =
        factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(TestDefinedGetter));
    JSHandle<JSTaggedValue> handleHolder(factory->NewEmptyJSObject());

    PropertyDescriptor handleDesc(thread);
    handleDesc.SetGetter(handleValue);
    handleDesc.SetSetter(handleValue);
    PropertyDescriptor handleDesc1(thread);
    handleDesc1.SetWritable(true);
    PropertyDescriptor handleDesc2(thread);
    handleDesc2.SetConfigurable(true);
    handleDesc2.SetEnumerable(true);
    PropertyDescriptor handleDesc3(thread);
    PropertyAttributes handleAttr(handleDesc);

    ObjectOperator objectOperator(thread, handleHolder, handleKey);
    objectOperator.SetIndex(1);
    // object is not IsAccessorDescriptor
    objectOperator.ToPropertyDescriptor(handleDesc1);
    EXPECT_TRUE(!handleDesc1.IsWritable());
    EXPECT_TRUE(handleDesc1.GetValue()->IsUndefined());
    // object is IsAccessorDescriptor
    objectOperator.SetAttr(handleAttr);
    objectOperator.SetValue(handleAccessor.GetTaggedValue());
    objectOperator.ToPropertyDescriptor(handleDesc2);
    EXPECT_FALSE(handleDesc2.IsEnumerable());
    EXPECT_FALSE(handleDesc2.IsConfigurable());
    EXPECT_TRUE(handleDesc2.GetGetter()->IsUndefined());
    EXPECT_TRUE(handleDesc2.GetSetter()->IsUndefined());

    objectOperator.SetValue(handleInternalAccessor.GetTaggedValue());
    objectOperator.ToPropertyDescriptor(handleDesc3);
    EXPECT_EQ(handleDesc3.GetValue()->GetInt(), 12);
}

HWTEST_F_L0(ObjectOperatorTest, handleKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    
    JSHandle<JSTaggedValue> handleString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleObject(factory->NewEmptyJSObject());
    JSHandle<JSTaggedValue> handleSymbol(factory->NewJSSymbol());
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(-1.11));
    JSHandle<JSTaggedValue> handleKey3(thread, handleString.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey4(thread, handleSymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey5(thread, handleObject.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey6(thread, JSTaggedValue(1.11));
   
    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);
    ObjectOperator objectOperator3(thread, handleKey3);
    ObjectOperator objectOperator4(thread, handleKey4);
    ObjectOperator objectOperator5(thread, handleKey5);
    ObjectOperator objectOperator6(thread, handleKey6);
   
    JSHandle<EcmaString> handleEcmaStrTo1(objectOperator1.GetKey());
    EXPECT_STREQ("-1", CString(handleEcmaStrTo1->GetCString().get()).c_str());

    JSHandle<EcmaString> handleEcmaStrTo2(objectOperator2.GetKey());
    EXPECT_STREQ("-1.11", CString(handleEcmaStrTo2->GetCString().get()).c_str());

    EcmaString *str1 = EcmaString::Cast(objectOperator3.GetKey()->GetTaggedObject());
    EXPECT_TRUE(str1->IsInternString());
    
    EXPECT_TRUE(objectOperator4.GetKey()->IsSymbol());

    EcmaString *str2 = EcmaString::Cast(objectOperator5.GetKey()->GetTaggedObject());
    EXPECT_TRUE(str2->IsInternString());

    JSHandle<EcmaString> handleEcmaStrTo3(objectOperator6.GetKey());
    EXPECT_STREQ("1.11", CString(handleEcmaStrTo3->GetCString().get()).c_str());
}

HWTEST_F_L0(ObjectOperatorTest, FastGetValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    
    JSHandle<EcmaString> handleNameString = factory->NewFromASCII("name");
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(123));
    JSHandle<JSTaggedValue> handleValue1(factory->NewPropertyBox(handleReceiver));
    JSHandle<JSTaggedValue> handleValue2(handleAccessorData);
    JSHandle<JSTaggedValue> handleValue3(thread, JSTaggedValue(1));

    ObjectOperator objectOperator1(thread, handleValue3);
    objectOperator1.SetIndex(1);
    objectOperator1.SetValue(handleValue1.GetTaggedValue());
    EXPECT_EQ(objectOperator1.FastGetValue()->GetInt(), 123);

    // op for fast path
    ObjectOperator objectOperator2(thread, handleReceiver.GetTaggedValue(), handleNameString.GetTaggedValue());
    objectOperator2.SetIndex(1);
    objectOperator2.SetValue(handleValue2.GetTaggedValue());
    PropertyDescriptor handleDesc(thread);
    handleDesc.SetGetter(handleValue2);
    handleDesc.SetSetter(handleValue2);
    objectOperator2.SetAttr(PropertyAttributes(handleDesc));
    EXPECT_TRUE(objectOperator2.FastGetValue()->IsUndefined());

    JSHandle<JSFunction> handleGetter = factory->NewJSFunction(env, reinterpret_cast<void *>(TestDefinedGetter));
    handleAccessorData->SetGetter(thread, handleGetter.GetTaggedValue());
    EXPECT_EQ(objectOperator2.FastGetValue()->GetInt(), 12);
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleName(factory->NewFromASCII("123"));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSString(handleName));
    // Receiver is string
    ObjectOperator objectOperator1(thread, handleHolder, handleReceiver, handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), 2U);
    EXPECT_TRUE(objectOperator1.GetValue().IsString());
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 2);
    EXPECT_TRUE(objectOperator1.IsFastMode());
    // Receiver is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), 2U);
    EXPECT_EQ(objectOperator2.GetValue().GetInt(), 2);
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_TRUE(objectOperator2.IsFastMode());
    // Receiver is DictionaryMode
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    ObjectOperator objectOperator3(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator3.ReLookupPropertyInReceiver(); // no key find
    EXPECT_EQ(objectOperator3.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator3.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator3.GetAttr().GetPropertyMetaData(), 0);
    EXPECT_FALSE(objectOperator3.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleReceiver(globalObj);
    PropertyAttributes handleAttr(4);
    // Receiver is JSGlobalObject(no properties)
    ObjectOperator objectOperator1(thread, handleHolder, JSHandle<JSTaggedValue>(handleReceiver), handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator1.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 0);
    EXPECT_FALSE(objectOperator1.IsFastMode());
    // Receiver is JSGlobalObject(properties)
    JSMutableHandle<GlobalDictionary> receiverDict(thread, handleReceiver->GetProperties());
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4); // numberofElements = 4
    receiverDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, JSTaggedValue(4));
    JSHandle<GlobalDictionary> handleProperties =
        GlobalDictionary::PutIfAbsent(thread, receiverDict, handleKey, JSHandle<JSTaggedValue>(cellHandle), handleAttr);
    handleReceiver->SetProperties(thread, handleProperties.GetTaggedValue());
    int keyEntry = handleProperties->FindEntry(handleKey.GetTaggedValue());
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleReceiver), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), static_cast<uint32_t>(keyEntry));
    EXPECT_TRUE(objectOperator2.GetValue().IsPropertyBox());
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), handleAttr.GetPropertyMetaData());
    EXPECT_TRUE(objectOperator2.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleKey1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey1, handleKey1);
    // Receiver is not DictionaryMode
    ObjectOperator objectOperator1(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), 0U);
    EXPECT_EQ(objectOperator1.GetValue().GetInt(), 1);
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 7); // default attribute
    EXPECT_TRUE(objectOperator1.IsFastMode());
    // Receiver is DictionaryMode
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator2.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), 0); // default attribute
    EXPECT_FALSE(objectOperator2.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, LookupProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleValue);
    JSHandle<JSObject> handleObject1 = JSObject::ObjectCreate(thread, handleObject);

    ObjectOperator objectOperator(thread, handleObject1, handleKey);
    objectOperator.LookupProperty();
    EXPECT_TRUE(objectOperator.IsOnPrototype());
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, GlobalLookupProperty)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleGlobalObject), handleKey, handleValue);
    JSHandle<JSObject> handleObject = JSObject::ObjectCreate(thread, handleGlobalObject);
    JSHandle<JSTaggedValue> handleGlobalObj(JSHandle<JSGlobalObject>::Cast(handleObject));

    ObjectOperator objectOperator(thread, handleGlobalObj, handleKey);
    objectOperator.GlobalLookupProperty();
    EXPECT_TRUE(objectOperator.IsOnPrototype());
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor1)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    EXPECT_TRUE(objectOperator1.GetReceiver()->IsJSGlobalObject());
    EXPECT_FALSE(objectOperator1.GetHolder()->IsJSGlobalObject());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
    EXPECT_TRUE(objectOperator2.GetReceiver()->IsJSGlobalObject());
    EXPECT_TRUE(objectOperator2.GetHolder()->IsJSGlobalObject());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleHolder = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSObject>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor3)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> symbolFunc = env->GetSymbolFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, symbolFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReceiver(thread, symbolFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor4)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> stringFunc = env->GetStringFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, stringFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReceiver(thread, stringFunc.GetTaggedValue());
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleReceiver, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleReceiver, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor5)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> boolFunc = env->GetBooleanFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, boolFunc.GetTaggedValue());
    uint32_t index = 1;
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), index, type)
    ObjectOperator objectOperator1(thread, handleHolder, index, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, index, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor6)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> numFunc = env->GetNumberFunction();
    JSHandle<JSTaggedValue> handleReceiver(thread, numFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleName(factory->NewFromASCII("name"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSTaggedVale(), JSTaggedValue(), type)
    ObjectOperator objectOperator1(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), type);
    EXPECT_FALSE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor7)
{
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleName(thread, JSTaggedValue(2));
    PropertyAttributes handleAttr(4);
    // ObjectOperator(thread, JSTaggedVale(), JSTaggedValue(), PropertyAttributes())
    ObjectOperator objectOperator(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), handleAttr);
    EXPECT_EQ(objectOperator.GetReceiver()->GetInt(), 1);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_EQ(objectOperator.GetKey()->GetInt(), 2);
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDateValue_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(5));
    ObjectOperator objectOperator1(thread, handleValue);
    objectOperator1.SetIndex(1);

    // object is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator1.UpdateDataValue(handleObject, handleValue, false));
    auto *resultElements =TaggedArray::Cast(handleObject->GetElements().GetTaggedObject());
    EXPECT_EQ(resultElements->Get(objectOperator1.GetIndex()).GetInt(), 4);

    // object is DictionaryMode
    JSObject::DeleteProperty(thread, handleObject, handleKey2);
    EXPECT_TRUE(objectOperator1.UpdateDataValue(handleObject, handleValue1, false));
    auto *resultDict = NumberDictionary::Cast(handleObject->GetElements().GetTaggedObject());
    EXPECT_EQ(resultDict->GetValue(objectOperator1.GetIndex()).GetInt(), 5);

    // objcet value is InternalAccessor
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    JSHandle<JSNativePointer> handleSetter = factory->NewJSNativePointer(reinterpret_cast<void *>(TestBoolSetter));
    handleAccessorData->SetSetter(thread, handleSetter.GetTaggedValue());
    JSHandle<JSTaggedValue> handleValue2(handleAccessorData);
    ObjectOperator objectOperator2(thread, handleKey);
    objectOperator2.SetValue(handleAccessorData.GetTaggedValue());
    objectOperator2.SetIndex(1);
    EXPECT_TRUE(objectOperator2.UpdateDataValue(handleObject, handleValue, true));
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDataValue_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(100));
    // object is JSGlobalObject
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);

    JSMutableHandle<GlobalDictionary> holderDict(thread, handleGlobalObject->GetProperties());
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4); // numberofElements = 4
    holderDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, JSTaggedValue(4));
    JSHandle<GlobalDictionary> handleProperties =
        GlobalDictionary::PutIfAbsent(thread, holderDict, handleKey,
                                      JSHandle<JSTaggedValue>(cellHandle), PropertyAttributes(4));
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue()); // Set Properties
    int keyEntry = handleProperties->FindEntry(handleKey.GetTaggedValue());

    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);
    objectOperator.SetIndex(keyEntry);
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleGlobalObject, handleValue, false));
    auto *resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties().GetTaggedObject());
    PropertyBox *resultCell = resultDict->GetBox(objectOperator.GetIndex());
    EXPECT_EQ(resultCell->GetValue().GetInt(), 100);
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDataValue_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<EcmaString> handleKey1 = factory->NewFromASCII("value");
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("value1"));

    ObjectOperator objectOperator(thread, handleKey);
    objectOperator.SetIndex(1);
    PropertyDescriptor handleDesc(thread);
    PropertyAttributes handleAttr(handleDesc);
    handleAttr.SetIsInlinedProps(true);
    objectOperator.SetAttr(PropertyAttributes(handleDesc));

    // object is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(handleKey1, JSTaggedValue::ToString(thread, newValue));
        JSHandle<JSTaggedValue> newKey(thread, newString.GetTaggedValue());
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newValue);
    }
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleObject, handleValue1, false));
    TaggedArray *resultElements1 = TaggedArray::Cast(handleObject->GetProperties().GetTaggedObject());
    EXPECT_EQ(resultElements1->Get(objectOperator.GetIndex()).GetInt(), 3);

    // object is DictionaryMode
    JSObject::DeleteProperty(thread, handleObject, handleKey2);
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleObject, handleValue, false));
    TaggedArray *resultElements2 = TaggedArray::Cast(handleObject->GetProperties().GetTaggedObject());
    auto *resultDict = NumberDictionary::Cast(resultElements2);
    EXPECT_EQ(resultDict->GetValue(objectOperator.GetIndex()).GetInt(), 4);
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    uint32_t index = 1;
    PropertyDescriptor handleDesc(thread);
    ObjectOperator objectOperator(thread, handleKey);
    PropertyAttributes handleAttr(4);
    handleDesc.SetConfigurable(true); // Desc Set Configurable
    objectOperator.SetAttr(PropertyAttributes(3));
    objectOperator.SetIndex(index);
    // object class is not DictionaryElement and object is Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator.WriteDataProperty(handleObject, handleDesc));
    auto resultDict = NumberDictionary::Cast(handleObject->GetElements().GetTaggedObject());
    int resultEntry = resultDict->FindEntry(JSTaggedValue(index));
    int resultAttrValue = resultDict->GetAttributes(resultEntry).GetPropertyMetaData();

    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), resultAttrValue);
    EXPECT_EQ(objectOperator.GetAttr().GetDictionaryOrder(), 1U);
    EXPECT_TRUE(objectOperator.GetAttr().IsConfigurable());
    EXPECT_EQ(objectOperator.GetIndex(), static_cast<uint32_t>(resultEntry));
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_TRUE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(2));
    JSHandle<PropertyBox> cellHandle1 = factory->NewPropertyBox(handleValue1);
    JSHandle<PropertyBox> cellHandle2 = factory->NewPropertyBox(handleValue2);
    PropertyDescriptor handleDesc(thread);
    handleDesc.SetConfigurable(true);
    PropertyAttributes handleAttr(2);
    handleAttr.SetConfigurable(true);
    // object is JSGlobalObject and not Element
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);

    JSMutableHandle<GlobalDictionary> globalDict(thread, handleGlobalObject->GetProperties());
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4);
    globalDict.Update(handleDict.GetTaggedValue());
    JSHandle<GlobalDictionary> handleProperties = GlobalDictionary::PutIfAbsent(
        thread, globalDict, handleKey, JSHandle<JSTaggedValue>(cellHandle1), PropertyAttributes(4));
    handleProperties->SetAttributes(thread, handleAttr.GetDictionaryOrder(), handleAttr);
    handleProperties->SetValue(thread, handleAttr.GetDictionaryOrder(), cellHandle2.GetTaggedValue());
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue());
    objectOperator.SetIndex(handleProperties->FindEntry(handleKey.GetTaggedValue()));
    
    EXPECT_TRUE(objectOperator.WriteDataProperty(handleGlobalObject, handleDesc));
    auto resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties().GetTaggedObject());
    EXPECT_EQ(resultDict->GetAttributes(objectOperator.GetIndex()).GetPropertyMetaData(), 4);
    EXPECT_TRUE(resultDict->GetAttributes(objectOperator.GetIndex()).IsConfigurable());

    int resultEntry = resultDict->GetAttributes(objectOperator.GetIndex()).GetDictionaryOrder();
    EXPECT_EQ(resultDict->GetAttributes(resultEntry).GetBoxType(), PropertyBoxType::MUTABLE);
    EXPECT_EQ(resultDict->GetValue(resultEntry).GetInt(), 1);
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    PropertyDescriptor handleDesc(thread, handleValue);
    handleDesc.SetSetter(handleValue); // Desc is AccessorDescriptor
    handleDesc.SetGetter(handleValue);
    // object is not DictionaryMode and not Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleValue);
    ObjectOperator objectOperator1(thread, handleKey);
    objectOperator1.SetAttr(PropertyAttributes(1));

    EXPECT_TRUE(objectOperator1.WriteDataProperty(handleObject, handleDesc));
    auto resultDict = NameDictionary::Cast(handleObject->GetProperties().GetTaggedObject());
    int resultEntry = resultDict->FindEntry(handleKey.GetTaggedValue());
    EXPECT_TRUE(resultDict->GetValue(resultEntry).IsAccessorData());
    EXPECT_EQ(resultDict->GetAttributes(resultEntry).GetValue(), objectOperator1.GetAttr().GetValue());
    // object is DictionaryMode and not Element
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    JSHandle<JSTaggedValue> handleSetter(factory->NewJSNativePointer(reinterpret_cast<void *>(TestDefinedSetter)));
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    handleDesc.SetSetter(handleSetter);
    ObjectOperator objectOperator2(thread, handleKey);
    objectOperator2.SetAttr(PropertyAttributes(handleDesc));
    objectOperator2.SetValue(handleAccessorData.GetTaggedValue());
    EXPECT_TRUE(objectOperator2.WriteDataProperty(handleObject, handleDesc));
    JSHandle<AccessorData> resultAccessorData(thread, objectOperator2.GetValue());
    EXPECT_EQ(resultAccessorData->GetGetter().GetInt(), 2);
    EXPECT_TRUE(resultAccessorData->GetSetter().IsJSNativePointer());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));
    int32_t elementIndex = 2;
    PropertyAttributes handleAttr(elementIndex);
    // object is JSArray and Element
    JSHandle<JSArray> handleArr = factory->NewJSArray();
    handleArr->SetArrayLength(thread, (elementIndex - 1));
    JSHandle<JSTaggedValue> handleArrObj(thread, handleArr.GetTaggedValue());
    ObjectOperator objectOperator1(thread, handleArrObj, elementIndex);
    EXPECT_TRUE(objectOperator1.AddProperty(JSHandle<JSObject>(handleArrObj), handleValue, handleAttr));
    EXPECT_EQ(handleArr->GetArrayLength(), 3U); // (elementIndex - 1) + 2
    // object is DictionaryElement and Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    JSObject::DeleteProperty(thread, (handleObject), handleKey); // Delete key2
    ObjectOperator objectOperator2(thread, JSHandle<JSTaggedValue>(handleObject), elementIndex);
    EXPECT_TRUE(objectOperator2.AddProperty(handleObject, handleValue, handleAttr));
    auto resultDict = NumberDictionary::Cast(handleObject->GetElements().GetTaggedObject());
    int resultEntry = resultDict->FindEntry(JSTaggedValue(static_cast<uint32_t>(elementIndex)));
    EXPECT_EQ(resultDict->GetKey(resultEntry).GetInt(), elementIndex);
    EXPECT_EQ(resultDict->GetValue(resultEntry).GetInt(), 3);
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));
    int32_t elementIndex = 4;
    PropertyAttributes handleDefaultAttr(elementIndex);
    PropertyAttributes handleAttr(elementIndex);
    handleDefaultAttr.SetDefaultAttributes();
    // object is not DictionaryMode and DefaultAttr
    JSHandle<JSObject> handleObject1 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), newKey, newKey);
    }
    ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(handleObject1), elementIndex);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject1, handleValue, handleDefaultAttr));
    TaggedArray *resultArray = TaggedArray::Cast(handleObject1->GetElements().GetTaggedObject());
    EXPECT_EQ(resultArray->Get(elementIndex).GetInt(), 3);
    EXPECT_EQ(resultArray->GetLength(), 7U);
    EXPECT_EQ(handleObject1->GetJSHClass()->GetElementRepresentation(), Representation::INT);
    // object is not DictionaryMode and not DefaultAttr
    JSHandle<JSObject> handleObject2 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 4; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator.AddProperty(handleObject2, handleString, handleAttr));
    auto resultDict = NumberDictionary::Cast(handleObject2->GetElements().GetTaggedObject());
    int resultEntry = resultDict->FindEntry(JSTaggedValue(static_cast<uint32_t>(elementIndex)));
    EXPECT_EQ(resultDict->GetKey(resultEntry).GetInt(), elementIndex);
    EXPECT_TRUE(resultDict->GetValue(resultEntry).IsString());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    int32_t handleAttrOffset = 4;
    PropertyAttributes handleAttr(handleAttrOffset);
    handleAttr.SetOffset(handleAttrOffset);
    // object is JSGlobalObject and not Element
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj); // no properties
    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleGlobalObject, handleValue, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetBoxType(), PropertyBoxType::CONSTANT);
    EXPECT_EQ(objectOperator.FastGetValue()->GetInt(), 4);
    EXPECT_EQ(objectOperator.GetIndex(), 0U);
    EXPECT_TRUE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handledUndefinedVal(thread, JSTaggedValue::Undefined());
    int32_t handleAttrOffset = 4;
    PropertyAttributes handleAttr(handleAttrOffset);
    handleAttr.SetOffset(handleAttrOffset);
    // object is not DictionaryMode and not Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i< 4; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_EQ(handleObject->GetJSHClass()->GetInlinedProperties(), 4U);
    ObjectOperator objectOperator(thread, handleObject, handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject, handleValue, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 4);
    EXPECT_EQ(objectOperator.GetIndex(), 0U); // 0 = 4 - 4
    EXPECT_TRUE(objectOperator.IsFastMode());
    EXPECT_TRUE(objectOperator.IsTransition());
    // object is DictionaryMode and not Element
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject, handledUndefinedVal, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator.GetIndex(), 0U);
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, Property_DeleteElement)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    uint32_t index = 1; // key value
    PropertyAttributes handleAttr(index);

    // object is not DictionaryMode
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject1 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), handleKey1, handleKey1);
    TaggedArray *handleElements = TaggedArray::Cast(handleObject1->GetElements().GetTaggedObject());
    EXPECT_EQ(handleElements->Get(index).GetInt(), 1);
    
    ObjectOperator objectOperator1(thread, JSHandle<JSTaggedValue>(handleObject1), index);
    objectOperator1.DeletePropertyInHolder();
    TaggedArray *resultElements = TaggedArray::Cast(handleObject1->GetElements().GetTaggedObject());
    EXPECT_EQ(resultElements->Get(index).GetInt(), 0);
    auto resultDict1 = NumberDictionary::Cast(handleObject1->GetElements().GetTaggedObject());
    EXPECT_TRUE(resultDict1->IsDictionaryMode());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject1, handleKey1).GetValue()->IsUndefined());
    // object is DictionaryMode
    JSHandle<JSObject> handleObject2 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), handleKey1, handleKey1);
    JSObject::DeleteProperty(thread, (handleObject2), handleKey1);
    ObjectOperator objectOperator2(thread, JSHandle<JSTaggedValue>(handleObject2), index - 1);
    objectOperator2.DeletePropertyInHolder();
    auto resultDict2 = NumberDictionary::Cast(handleObject2->GetElements().GetTaggedObject());
    EXPECT_TRUE(resultDict2->GetKey(index - 1U).IsUndefined());
    EXPECT_TRUE(resultDict2->GetValue(index - 1U).IsUndefined());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject2, handleKey0).GetValue()->IsUndefined());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject2, handleKey1).GetValue()->IsUndefined());
}

HWTEST_F_L0(ObjectOperatorTest, Property_DeleteProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(33));
    // object is JSGlobalObject
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    JSMutableHandle<GlobalDictionary> globalDict(thread, handleGlobalObject->GetProperties());
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4);
    globalDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, handleValue.GetTaggedValue());
    JSHandle<GlobalDictionary> handleProperties = GlobalDictionary::PutIfAbsent(
        thread, globalDict, handleKey, JSHandle<JSTaggedValue>(cellHandle), PropertyAttributes(12));
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue());
    ObjectOperator objectOperator1(thread, handleGlobalObject, handleKey);
    
    objectOperator1.DeletePropertyInHolder();
    auto resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties().GetTaggedObject());
    // key not found
    EXPECT_EQ(resultDict->FindEntry(handleKey.GetTaggedValue()), -1);
    EXPECT_EQ(resultDict->GetAttributes(objectOperator1.GetIndex()).GetValue(), 0U);
    // object is not DictionaryMode
    JSHandle<JSObject> handleObject =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleKey1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey1, handleKey1);
    ObjectOperator objectOperator2(thread, handleObject, handleKey);
    objectOperator2.DeletePropertyInHolder();
    auto resultDict1 = NameDictionary::Cast(handleObject->GetProperties().GetTaggedObject());
    // key not found
    EXPECT_EQ(resultDict1->FindEntry(handleKey.GetTaggedValue()), -1);
}

HWTEST_F_L0(ObjectOperatorTest, Define_SetterAndGettetr)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(2));
    JSHandle<EcmaString> handleKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> handleKey1(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("value1"));
    // object is not DictionaryMode
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(handleKey, JSTaggedValue::ToString(thread, newValue));
        JSHandle<JSTaggedValue> newKey(thread, newString.GetTaggedValue());
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newValue);
    }
    // object is not Element
    ObjectOperator objectOperator(thread, handleObject, handleKey1);
    objectOperator.SetIndex(1);
    objectOperator.SetValue(handleAccessorData.GetTaggedValue());
    PropertyDescriptor handleDesc(thread, handleValue);
    handleDesc.SetSetter(handleValue);
    handleDesc.SetGetter(handleValue);
    objectOperator.SetAttr(PropertyAttributes(handleDesc));
    objectOperator.DefineSetter(handleValue1);
    objectOperator.DefineGetter(handleValue);

    JSHandle<JSObject> resultObj1(objectOperator.GetReceiver());
    TaggedArray *properties = TaggedArray::Cast(resultObj1->GetProperties().GetTaggedObject());
    JSHandle<AccessorData> resultAccessorData1(thread, properties->Get(objectOperator.GetIndex()));
    EXPECT_EQ(resultAccessorData1->GetGetter().GetInt(), 0);
    EXPECT_EQ(resultAccessorData1->GetSetter().GetInt(), 2);
    // object is DictionaryMode
    JSObject::DeleteProperty(thread, handleObject, handleKey2);
    objectOperator.DefineSetter(handleValue);
    objectOperator.DefineGetter(handleValue1);
    JSHandle<JSObject> resultObj2(objectOperator.GetReceiver());
    auto resultDict = NameDictionary::Cast(resultObj2->GetProperties().GetTaggedObject());
    JSHandle<AccessorData> resultAccessorData2(thread, resultDict->GetValue(objectOperator.GetIndex()));
    EXPECT_EQ(resultAccessorData2->GetGetter().GetInt(), 2);
    EXPECT_EQ(resultAccessorData2->GetSetter().GetInt(), 0);
}
} // namespace panda::test