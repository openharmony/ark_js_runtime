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

#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
using InlinedPropsBit = HandlerBase::InlinedPropsBit;
using OffsetBit = HandlerBase::OffsetBit;
using KindBit = HandlerBase::KindBit;
using IsJSArrayBit = HandlerBase::IsJSArrayBit;
using HandlerKind = HandlerBase::HandlerKind;
using AccessorBit = HandlerBase::AccessorBit;
class ICRuntimeStubTest : public testing::Test {
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

HWTEST_F_L0(ICRuntimeStubTest, LoadGlobalICByName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();

    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> globalValue(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<PropertyBox> handleBoxValue = factory->NewPropertyBox(handleValue);
    JSHandle<JSTaggedValue> propKey(factory->NewFromASCII("x"));

    uint32_t arrayLength = 2U;
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(arrayLength);
    handleTaggedArray->Set(thread, 0, handleBoxValue.GetTaggedValue());
    handleTaggedArray->Set(thread, 1, JSTaggedValue::Undefined());
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleTaggedArray);
    // ProfileTypeInfo get value is HeapObject and then call LoadGlobal function to load
    JSTaggedValue resultValue1 =
        ICRuntimeStub::LoadGlobalICByName(thread, *handleProfileTypeInfo,
                                          JSTaggedValue::Undefined(), JSTaggedValue::Undefined(), 0);
    EXPECT_EQ(resultValue1.GetInt(), 2);
    // the globalValue is jsobject then call loadMiss function can find global variable from global record firstly
    // so need store global record.
    SlowRuntimeStub::StGlobalRecord(thread, propKey.GetTaggedValue(), handleValue.GetTaggedValue(), false);
    JSTaggedValue resultValue2 =
        ICRuntimeStub::LoadGlobalICByName(thread, *handleProfileTypeInfo,
                                          globalValue.GetTaggedValue(), propKey.GetTaggedValue(), 1);
    EXPECT_EQ(resultValue2.GetInt(), 2);
    EXPECT_TRUE(handleProfileTypeInfo->Get(1).IsPropertyBox());
}

HWTEST_F_L0(ICRuntimeStubTest, StoreGlobalICByName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<PropertyBox> handleBoxValue =
        factory->NewPropertyBox(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> propKey(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> globalValue(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSTaggedValue handleValue(2);

    uint32_t arrayLength = 2U;
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(arrayLength);
    handleTaggedArray->Set(thread, 0, handleBoxValue.GetTaggedValue());
    handleTaggedArray->Set(thread, 1, handleValue);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleTaggedArray);
    // ProfileTypeInfo get value is HeapObject and then call LoadGlobal function to load
    JSTaggedValue resultValue1 =
        ICRuntimeStub::StoreGlobalICByName(thread, *handleProfileTypeInfo,
                                           JSTaggedValue::Undefined(), JSTaggedValue::Undefined(), handleValue, 0);
    EXPECT_TRUE(resultValue1.IsUndefined());
    EXPECT_EQ(handleBoxValue->GetValue().GetInt(), 2);
    // the globalValue is jsobject then call loadMiss function can find global variable from global record firstly
    // so need store global record.
    SlowRuntimeStub::StGlobalRecord(thread, propKey.GetTaggedValue(), handleBoxValue.GetTaggedValue(), false);
    JSTaggedValue resultValue2 =
        ICRuntimeStub::StoreGlobalICByName(thread, *handleProfileTypeInfo,
                                           globalValue.GetTaggedValue(), propKey.GetTaggedValue(), handleValue, 1);
    EXPECT_TRUE(resultValue2.IsUndefined());
    EXPECT_TRUE(handleProfileTypeInfo->Get(1).IsPropertyBox());
}

HWTEST_F_L0(ICRuntimeStubTest, CheckPolyHClass)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<JSTaggedValue> handleObj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    TaggedObject *handleTaggedObj = handleObj->GetTaggedObject();
    JSTaggedValue handleTaggedObjVal(handleTaggedObj);
    handleTaggedObjVal.CreateWeakRef();
    TaggedObject *handleWeakObj = TaggedObject::Cast(handleTaggedObjVal.GetWeakReferent());
    JSHClass *handleObjClass = static_cast<JSHClass *>(handleWeakObj);

    array_size_t length = 5U;
    JSHandle<TaggedArray> handleCacheArray = factory->NewTaggedArray(length);
    for (int i = 0; i < static_cast<int>(length); i++) {
        handleCacheArray->Set(thread, i, JSTaggedValue::Undefined());
    }
    JSHandle<JSTaggedValue> handleTaggedObjWeakVal(thread, handleTaggedObjVal);
    handleCacheArray->Set(thread, length - 3U, handleTaggedObjWeakVal.GetTaggedValue()); // set in two
    JSTaggedValue handleWeakCacheValue(handleCacheArray.GetTaggedValue());

    JSTaggedValue resultValue = ICRuntimeStub::CheckPolyHClass(handleWeakCacheValue, handleObjClass);
    EXPECT_TRUE(resultValue.IsUndefined());
}

HWTEST_F_L0(ICRuntimeStubTest,  StoreICAndLoadIC_ByName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> handleObj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleStoreVal(thread, JSTaggedValue(2));

    uint32_t arrayLength = 1U;
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(arrayLength);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleTaggedArray);

    // test receiver is jsobject and ProfileTypeInfo get value is hole in slotId.
    ICRuntimeStub::StoreICByName(thread, *handleProfileTypeInfo, handleObj.GetTaggedValue(),
                                 handleKey.GetTaggedValue(), handleStoreVal.GetTaggedValue(), 0);
    JSTaggedValue resultValue =
        ICRuntimeStub::LoadICByName(thread, *handleProfileTypeInfo, handleObj.GetTaggedValue(),
                                    handleKey.GetTaggedValue(), 0);
    EXPECT_EQ(resultValue.GetInt(), 2);
}

HWTEST_F_L0(ICRuntimeStubTest,  StoreICAndLoadIC_ByValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> typeArrayFun = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> handleTypeArrReceiver = JSHandle<JSTypedArray>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(typeArrayFun), typeArrayFun));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleStoreVal(factory->NewFromASCII("1"));

    uint32_t arrayLength = 1U;
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(arrayLength);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleTaggedArray);
    // test receiver is typedArray
    ICRuntimeStub::StoreICByValue(thread, *handleProfileTypeInfo, handleTypeArrReceiver.GetTaggedValue(),
                                 handleKey.GetTaggedValue(), handleStoreVal.GetTaggedValue(), 0);
    JSTaggedValue resultValue =
        ICRuntimeStub::LoadICByValue(thread, *handleProfileTypeInfo, handleTypeArrReceiver.GetTaggedValue(),
                                     handleKey.GetTaggedValue(), 0);
    EXPECT_TRUE(resultValue.IsString());
    JSHandle<EcmaString> handleEcmaStrTo(JSHandle<JSTaggedValue>(thread, resultValue));
    EXPECT_STREQ("1", CString(handleEcmaStrTo->GetCString().get()).c_str());
}

HWTEST_F_L0(ICRuntimeStubTest, TryStoreICAndLoadIC_ByName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue handleFirstObjClassVal(handleObj->GetClass());
    TaggedObject *handleTaggedObject = handleFirstObjClassVal.GetWeakReferentUnChecked();
    JSHClass *handleNewObjClass = static_cast<JSHClass *>(handleTaggedObject);
    JSHandle<JSObject> handleNewObj = factory->NewJSObject(JSHandle<JSHClass>(thread, handleNewObjClass));

    JSHandle<TaggedArray> handleArr = factory->NewTaggedArray(1);
    handleArr->Set(thread, 0, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleBoxValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handlerPropertyBoxVal(factory->NewPropertyBox(handleBoxValue));

    JSTaggedValue handleFirstArrVal(handleArr.GetTaggedValue());
    JSTaggedValue handleSecondPropertyBoxVal = handlerPropertyBoxVal.GetTaggedValue();
    JSTaggedValue handleReceiver = handleNewObj.GetTaggedValue();
    JSTaggedValue handleStoreVal(2);

    // fistValue GetWeakReferentUnChecked is equal the receiver class
    ICRuntimeStub::TryStoreICByName(thread, handleReceiver, handleFirstObjClassVal,
                                                            handleSecondPropertyBoxVal, handleStoreVal);
    JSTaggedValue resultValue1 = ICRuntimeStub::TryLoadICByName(thread, handleReceiver, handleFirstObjClassVal,
                                                                handleSecondPropertyBoxVal);
    EXPECT_EQ(resultValue1.GetInt(), handleStoreVal.GetInt());
    // fistValue GetWeakReferent is not equal the receiver class
    ICRuntimeStub::TryStoreICByName(thread, handleObj.GetTaggedValue(), handleFirstArrVal,
                                                            handleSecondPropertyBoxVal, handleStoreVal);
    JSTaggedValue resultValue2 = ICRuntimeStub::TryLoadICByName(thread, handleObj.GetTaggedValue(),
                                                                handleFirstArrVal, handleSecondPropertyBoxVal);
    EXPECT_TRUE(resultValue2.IsHole());
}

HWTEST_F_L0(ICRuntimeStubTest,  TryStoreICAndLoadIC_ByValue1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue handleObjClassVal(handleObj->GetClass());
    TaggedObject *handleTaggedObject = handleObjClassVal.GetWeakReferentUnChecked();
    JSHClass *handleNewObjClass = static_cast<JSHClass *>(handleTaggedObject);

    JSHandle<JSObject> handleNewObj = factory->NewJSObject(JSHandle<JSHClass>(thread, handleNewObjClass));
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(1);
    handleNewObj->SetElements(thread, handleTaggedArray.GetTaggedValue());

    uint32_t handler = 0U;
    KindBit::Set<uint32_t>(HandlerKind::ELEMENT, &handler);
    JSTaggedValue handleFirstVal(handleTaggedObject);
    JSTaggedValue handleSecondHandlerVal(handler);
    JSTaggedValue handleReceiver = handleNewObj.GetTaggedValue();
    JSTaggedValue handleStoreVal(2);

    // fistValue GetWeakReferentUnChecked is equal the receiver class
    ICRuntimeStub::TryStoreICByValue(thread, handleReceiver, JSTaggedValue(0),
                                     handleFirstVal, handleSecondHandlerVal, handleStoreVal);
    JSTaggedValue resultValue = ICRuntimeStub::TryLoadICByValue(thread, handleReceiver, JSTaggedValue(0),
                                                                handleFirstVal, handleSecondHandlerVal);
    EXPECT_EQ(resultValue.GetInt(), handleStoreVal.GetInt());
}

HWTEST_F_L0(ICRuntimeStubTest,  TryStoreICAndLoadIC_ByValue2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue handleObjClassVal(handleObj->GetClass());
    handleObjClassVal.CreateWeakRef();
    TaggedObject *handleTaggedObject = TaggedObject::Cast(handleObjClassVal.GetWeakReferent());
    JSHClass *handleNewObjClass = static_cast<JSHClass *>(handleTaggedObject);
    JSHandle<TaggedArray> handleTypeArr = factory->NewTaggedArray(1);
    JSHandle<JSObject> handleNewObj = factory->NewJSObject(JSHandle<JSHClass>(thread, handleNewObjClass));
    handleNewObj->SetProperties(thread, handleTypeArr.GetTaggedValue());

    uint32_t handler = 0U;
    KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);
    JSHandle<TaggedArray> handleSecondValArr = factory->NewTaggedArray(2);
    handleSecondValArr->Set(thread, 0, handleObjClassVal);
    handleSecondValArr->Set(thread, 1, JSTaggedValue(handler));
    JSTaggedValue handleReceiver = handleNewObj.GetTaggedValue();
    JSTaggedValue handleSecondArrVal = handleSecondValArr.GetTaggedValue();
    JSTaggedValue handleStoreVal(2);

    // fistvalue is equal the key value.
    ICRuntimeStub::TryStoreICByValue(thread, handleReceiver, JSTaggedValue(0),
                                     JSTaggedValue(0), handleSecondArrVal, handleStoreVal);
    JSTaggedValue resultValue = ICRuntimeStub::TryLoadICByValue(thread, handleReceiver, JSTaggedValue(0),
                                                                JSTaggedValue(0), handleSecondArrVal);
    EXPECT_EQ(resultValue.GetInt(), handleStoreVal.GetInt());
}

JSTaggedValue TestSetter(EcmaRuntimeCallInfo *argv)
{
    // 2 : 2 arg value
    if (argv->GetArgsNumber() == 1U && argv->GetCallArg(0).GetTaggedValue() == JSTaggedValue(2)) {
        JSThread *thread = argv->GetThread();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSObject> obj(BuiltinsBase::GetThis(argv));
        JSHandle<JSTaggedValue> keyHandle(factory->NewFromASCII("key"));
        JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(2)); // 2 : 2 property value
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), keyHandle, valueHandle);
        return JSTaggedValue(JSTaggedValue::True());
    }
    return JSTaggedValue(JSTaggedValue::False());
}

HWTEST_F_L0(ICRuntimeStubTest, StoreICWithHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> keyHandle(factory->NewFromASCII("key"));
    JSHandle<PropertyBox> boxHandler = factory->NewPropertyBox(keyHandle);
    JSHandle<JSFunction> setter = factory->NewJSFunction(env, reinterpret_cast<void *>(TestSetter));
    JSHandle<AccessorData> handleAccessor = factory->NewAccessorData();
    handleAccessor->SetSetter(thread, setter.GetTaggedValue());

    uint32_t handler = 0U;
    uint32_t bitOffset = 1U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);
    AccessorBit::Set<uint32_t>(true, &handler);

    array_size_t arrayLength = bitOffset + 1U;
    JSHandle<JSObject> handleHolder = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleTaggedArr->Set(thread, bitOffset, handleAccessor.GetTaggedValue());
    handleHolder->SetProperties(thread, handleTaggedArr.GetTaggedValue());
    JSHandle<JSObject> handleReceiver = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    // handler is Init and HandlerBase Is Accessor,then call CallSetter function.
    JSTaggedValue resultValue1 =
        ICRuntimeStub::StoreICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                          handleHolder.GetTaggedValue(), JSTaggedValue(2), JSTaggedValue(handler));
    EXPECT_TRUE(resultValue1.IsUndefined());
    EXPECT_EQ(JSObject::GetProperty(thread,
                                    JSHandle<JSTaggedValue>(handleReceiver), keyHandle).GetValue()->GetInt(), 2);
    // handler is PropertyBox and then call StoreGlobal function.
    JSTaggedValue resultValue2 =
        ICRuntimeStub::StoreICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                          handleHolder.GetTaggedValue(), JSTaggedValue(2), boxHandler.GetTaggedValue());
    EXPECT_TRUE(resultValue2.IsUndefined());
    EXPECT_EQ(ICRuntimeStub::LoadGlobal(boxHandler.GetTaggedValue()).GetInt(), 2);
    // HandlerBase Is NonExist and This situation is not judged.
    KindBit::Set<uint32_t>(HandlerKind::NON_EXIST, &handler);
    JSTaggedValue resultValue3 =
        ICRuntimeStub::StoreICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                          handleHolder.GetTaggedValue(), JSTaggedValue(2), JSTaggedValue(handler));
    EXPECT_TRUE(resultValue3.IsUndefined());
}

JSTaggedValue TestGetter(EcmaRuntimeCallInfo *argv)
{
    auto thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj(BuiltinsBase::GetThis(argv));
    JSHandle<JSTaggedValue> keyHandle(factory->NewFromASCII("key"));
    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(obj), keyHandle).GetValue().GetTaggedValue();

    return JSTaggedValue(value.GetInt());
}

HWTEST_F_L0(ICRuntimeStubTest, LoadICWithHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> keyHandle(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(1));
    JSHandle<PropertyBox> boxHandler = factory->NewPropertyBox(keyHandle);
    JSHandle<JSFunction> getter = factory->NewJSFunction(env, reinterpret_cast<void *>(TestGetter));
    JSHandle<AccessorData> handleAccessor = factory->NewAccessorData();
    handleAccessor->SetGetter(thread, getter.GetTaggedValue());

    uint32_t handler = 0U;
    uint32_t bitOffset = 1U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);
    AccessorBit::Set<uint32_t>(true, &handler);

    array_size_t arrayLength = bitOffset + 1U;
    JSHandle<JSObject> handleHolder = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleTaggedArr->Set(thread, bitOffset, handleAccessor.GetTaggedValue());
    handleHolder->SetProperties(thread, handleTaggedArr.GetTaggedValue());
    JSHandle<JSObject> handleReceiver = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleReceiver), keyHandle, valueHandle);
    // handler is Init and HandlerBase Is Accessor,then call CallGetter function.
    JSTaggedValue resultValue1 =
        ICRuntimeStub::LoadICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                         handleHolder.GetTaggedValue(), JSTaggedValue(handler));
    EXPECT_EQ(resultValue1.GetInt(), 1);
    // HandlerBase Is NonExist and This situation is not judged.
    KindBit::Set<uint32_t>(HandlerKind::NON_EXIST, &handler);
    JSTaggedValue resultValue3 =
        ICRuntimeStub::LoadICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                         handleHolder.GetTaggedValue(), JSTaggedValue(handler));
    EXPECT_TRUE(resultValue3.IsUndefined());
    // handler is PropertyBox and then call LoadGlobal function.
    JSTaggedValue resultValue4 =
        ICRuntimeStub::LoadICWithHandler(thread, handleReceiver.GetTaggedValue(),
                                         handleHolder.GetTaggedValue(), boxHandler.GetTaggedValue());
    EXPECT_TRUE(resultValue4.IsString());
}

HWTEST_F_L0(ICRuntimeStubTest, Prototype_StoreAndLoad)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<ProtoChangeMarker> cellValue = factory->NewProtoChangeMarker();
    EXPECT_TRUE(!cellValue->GetHasChanged());

    uint32_t handler = 0U;
    uint32_t bitOffset = 1U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);
    KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler); // test filed

    array_size_t arrayLength = bitOffset + 1U;
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleObj->SetProperties(thread, handleTaggedArr.GetTaggedValue());

    JSHandle<PrototypeHandler> handleProtoHandler = factory->NewPrototypeHandler();
    handleProtoHandler->SetProtoCell(thread, cellValue.GetTaggedValue());
    handleProtoHandler->SetHandlerInfo(thread, JSTaggedValue(handler));
    handleProtoHandler->SetHolder(thread, handleObj.GetTaggedValue());
    // test storePrototype function
    JSTaggedValue resultValue1 = ICRuntimeStub::StorePrototype(thread, handleObj.GetTaggedValue(),
                                                              JSTaggedValue(1), handleProtoHandler.GetTaggedValue());
    EXPECT_TRUE(resultValue1.IsUndefined());
    // test loadPrototype function
    JSTaggedValue resultValue2 =
        ICRuntimeStub::LoadPrototype(thread, handleObj.GetTaggedValue(), handleProtoHandler.GetTaggedValue());
    EXPECT_EQ(ICRuntimeStub::LoadFromField(*handleObj, handler).GetInt(), resultValue2.GetInt());
}

HWTEST_F_L0(ICRuntimeStubTest, StoreWithTransition_In_Filed)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> arrFun = env->GetArrayFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSObject> handleArrObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrFun), arrFun);
    auto hclass = handleArrObj->GetJSHClass();

    uint32_t handler = 0U;
    uint32_t bitOffset = 1U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);
    KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);

    array_size_t arrayLength = bitOffset + 1U;
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleObj->SetProperties(thread, handleTaggedArr.GetTaggedValue());

    JSHandle<TransitionHandler> handleTranHandler = factory->NewTransitionHandler();
    handleTranHandler->SetTransitionHClass(thread, JSTaggedValue(hclass));
    handleTranHandler->SetHandlerInfo(thread, JSTaggedValue(handler));

    // test handler is InlinedProps and store in filed
    InlinedPropsBit::Set<uint32_t>(true, &handler);
    ICRuntimeStub::StoreWithTransition(thread, *handleObj, JSTaggedValue(2), handleTranHandler.GetTaggedValue());
    auto resultArray = TaggedArray::Cast(handleObj->GetProperties().GetTaggedObject());
    EXPECT_EQ(resultArray->Get(bitOffset).GetInt(), 2);
}

HWTEST_F_L0(ICRuntimeStubTest, StoreWithTransition)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> arrFun = env->GetArrayFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSObject> handleArrObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrFun), arrFun);
    auto hclass = handleArrObj->GetJSHClass();

    uint32_t handler = 0U;
    uint32_t bitOffset = 1U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);
    KindBit::Set<uint32_t>(HandlerKind::FIELD, &handler);

    array_size_t arrayLength = bitOffset + 2U;
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleObj->SetProperties(thread, handleTaggedArr.GetTaggedValue());

    JSHandle<TransitionHandler> handleTransitionHandler = factory->NewTransitionHandler();
    handleTransitionHandler->SetTransitionHClass(thread, JSTaggedValue(hclass));
    handleTransitionHandler->SetHandlerInfo(thread, JSTaggedValue(handler));

    // test handler is not InlinedProps and length more than the handler offset
    ICRuntimeStub::StoreWithTransition(thread, *handleObj, JSTaggedValue(2), handleTransitionHandler.GetTaggedValue());
    EXPECT_EQ(handleTaggedArr->Get(bitOffset).GetInt(), 2);

    // test handler offset more than the length and length is zero
    arrayLength = 0U;
    JSHandle<TaggedArray> handleEmptyTaggedArr = factory->NewTaggedArray(arrayLength);
    handleObj->SetProperties(thread, handleEmptyTaggedArr.GetTaggedValue());

    ICRuntimeStub::StoreWithTransition(thread, *handleObj, JSTaggedValue(3), handleTransitionHandler.GetTaggedValue());
    auto resultProperties = TaggedArray::Cast(handleObj->GetProperties().GetTaggedObject());
    EXPECT_EQ(static_cast<int>(resultProperties->GetLength()), JSObject::MIN_PROPERTIES_LENGTH);
    EXPECT_EQ(resultProperties->Get(bitOffset).GetInt(), 3);
}

HWTEST_F_L0(ICRuntimeStubTest, Field_StoreAndLoad)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    uint32_t handler = 0U;
    uint32_t bitOffset = 2U;
    OffsetBit::Set<uint32_t>(bitOffset, &handler);

    array_size_t arrayLength = bitOffset + 1U;
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleTaggedArr->Set(thread, bitOffset, JSTaggedValue::Undefined());

    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    handleObj->SetProperties(thread, handleTaggedArr.GetTaggedValue());
    // test handler is not InlinedProps
    ICRuntimeStub::StoreField(thread, *handleObj, JSTaggedValue(3), handler);
    JSTaggedValue resultValue1 = ICRuntimeStub::LoadFromField(*handleObj, handler);
    EXPECT_EQ(resultValue1.GetInt(), 3);
    // test handler is InlinedProps
    InlinedPropsBit::Set<uint32_t>(true, &handler);
    ICRuntimeStub::StoreField(thread, *handleObj, JSTaggedValue(2), handler);
    JSTaggedValue resultValue = ICRuntimeStub::LoadFromField(*handleObj, handler);
    EXPECT_EQ(resultValue.GetInt(), 2);
}

HWTEST_F_L0(ICRuntimeStubTest, Global_StoreAndLoad)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleUndefinedVal(thread, JSTaggedValue::Undefined());
    JSHandle<PropertyBox> handlerValue = factory->NewPropertyBox(handleUndefinedVal);

    JSTaggedValue resultValue1 = ICRuntimeStub::StoreGlobal(thread, JSTaggedValue(2), handlerValue.GetTaggedValue());
    EXPECT_TRUE(resultValue1.IsUndefined());
    JSTaggedValue resultValue2 = ICRuntimeStub::LoadGlobal(handlerValue.GetTaggedValue());
    EXPECT_EQ(resultValue2.GetInt(), 2);

    handlerValue->Clear(thread);
    JSTaggedValue resultValue3 = ICRuntimeStub::StoreGlobal(thread, JSTaggedValue(3), handlerValue.GetTaggedValue());
    EXPECT_TRUE(resultValue3.IsHole());

    JSTaggedValue resultValue4 = ICRuntimeStub::LoadGlobal(handlerValue.GetTaggedValue());
    EXPECT_TRUE(resultValue4.IsHole());
}

HWTEST_F_L0(ICRuntimeStubTest, Element_StoreAndLoad)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSTaggedValue hanldeIntKey(4);

    uint32_t handlerInfo = 0U;
    KindBit::Set<uint32_t>(HandlerKind::ELEMENT, &handlerInfo);
    IsJSArrayBit::Set<uint32_t>(true, &handlerInfo);

    array_size_t arrayLength = 3U;
    JSArray *handleArr = JSArray::ArrayCreate(thread, JSTaggedNumber(arrayLength)).GetObject<JSArray>();
    JSHandle<JSObject> handleArrObj(thread, handleArr);
    JSHandle<TaggedArray> handleTaggedArr = factory->NewTaggedArray(arrayLength);
    handleArrObj->SetProperties(thread, handleTaggedArr.GetTaggedValue());

    JSTaggedValue resultValue =
        ICRuntimeStub::StoreElement(thread, *handleArrObj, hanldeIntKey, JSTaggedValue(3), JSTaggedValue(handlerInfo));

    EXPECT_TRUE(resultValue.IsUndefined());
    EXPECT_EQ(ICRuntimeStub::LoadElement(*handleArrObj, hanldeIntKey).GetInt(), 3);
    EXPECT_EQ(JSObject::GetProperty(thread, handleArrObj, lengthKey).GetValue()->GetInt(), 5);
    EXPECT_TRUE(ICRuntimeStub::LoadElement(*handleArrObj, JSTaggedValue(2)).IsHole());
}

HWTEST_F_L0(ICRuntimeStubTest, TryToElementsIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSTaggedValue hanldeIntKey1(0);
    JSTaggedValue hanldeIntKey2(1);
    JSTaggedValue handleDoubleKey1(1.00);
    JSTaggedValue handleDoubleKey2(1.11);
    JSHandle<JSTaggedValue> handleStrKey1(factory->NewFromASCII("1234"));
    JSHandle<JSTaggedValue> handleStrKey2(factory->NewFromASCII("xy"));

    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(hanldeIntKey1), 0);
    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(hanldeIntKey2), 1);
    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(handleDoubleKey1), 1);
    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(handleDoubleKey2), -1);
    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(handleStrKey1.GetTaggedValue()), 1234);
    EXPECT_EQ(ICRuntimeStub::TryToElementsIndex(handleStrKey2.GetTaggedValue()), -1);
}
} // namespace panda::test