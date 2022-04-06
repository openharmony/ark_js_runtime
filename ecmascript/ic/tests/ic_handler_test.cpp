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

#include "ecmascript/ic/ic_handler-inl.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
using HandlerKind = ecmascript::HandlerBase::HandlerKind;
class ICHandlerTest : public testing::Test {
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

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: LoadElement
 * @tc.desc: Call "LoadElement" function,check whether the Element is loaded successfully by checking returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, LoadElement)
{
    JSTaggedValue result = LoadHandler::LoadElement(thread).GetTaggedValue();
    EXPECT_TRUE(HandlerBase::IsElement(result.GetInt()));
    EXPECT_EQ(HandlerBase::GetKind(result.GetInt()), HandlerKind::ELEMENT);
}

/**
 * @tc.name: LoadProperty
 * @tc.desc: Call "LoadProperty" function,check whether the Property is loaded successfully by checking returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, LoadProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetArrayFunction();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromCanBeCompressString("key"));
    int index = 1;
    PropertyAttributes handleAttriButes(2);
    handleAttriButes.SetIsInlinedProps(true);
    // test op is not Found
    ObjectOperator handleOp1(thread, handleKey);
    JSHandle<JSTaggedValue> handlerInfo1 = LoadHandler::LoadProperty(thread, handleOp1);
    EXPECT_TRUE(HandlerBase::IsNonExist(handlerInfo1->GetInt()));
    EXPECT_EQ(HandlerBase::GetKind(handlerInfo1->GetInt()), HandlerKind::NON_EXIST);
    // test op is Found and FastMode
    JSHandle<JSTaggedValue> handleHolder(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    ObjectOperator handleOp2(thread, handleHolder, handleKey);
    handleOp2.SetFastMode(true);
    handleOp2.SetIndex(index);
    JSHandle<JSTaggedValue> handlerInfo2 = LoadHandler::LoadProperty(thread, handleOp2);
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo2->GetInt()), 1);
    // test op is Found and InlinedProps
    handleOp2.SetAttr(handleAttriButes);
    JSHandle<JSTaggedValue> handlerInfo3 = LoadHandler::LoadProperty(thread, handleOp2);
    EXPECT_EQ(HandlerBase::GetKind(handlerInfo3->GetInt()), HandlerKind::FIELD);
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo3->GetInt()), 5);
    EXPECT_TRUE(HandlerBase::IsInlinedProps(handlerInfo3->GetInt()));
}

/**
 * @tc.name: StoreElement
 * @tc.desc: Call "StoreElement" function,check whether the Element is stored successfully by checking returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, StoreElement)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSArray *handleArr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> handleReceiver1(thread, handleArr);
    JSHandle<JSTaggedValue> handleReceiver2(factory->NewJSArray());
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> handlerInfo1 = StoreHandler::StoreElement(thread, handleReceiver1);
    JSHandle<JSTaggedValue> handlerInfo2 = StoreHandler::StoreElement(thread, handleReceiver2);
    JSHandle<JSTaggedValue> handlerInfo3 = StoreHandler::StoreElement(thread, handleValue);

    EXPECT_EQ(HandlerBase::GetKind(handlerInfo1->GetInt()), HandlerKind::ELEMENT);
    EXPECT_EQ(HandlerBase::GetKind(handlerInfo2->GetInt()), HandlerKind::ELEMENT);
    EXPECT_EQ(HandlerBase::GetKind(handlerInfo3->GetInt()), HandlerKind::ELEMENT);

    EXPECT_TRUE(HandlerBase::IsJSArray(handlerInfo1->GetInt()));
    EXPECT_TRUE(HandlerBase::IsJSArray(handlerInfo2->GetInt()));
    EXPECT_FALSE(HandlerBase::IsJSArray(handlerInfo3->GetInt()));
}

/**
 * @tc.name: StoreProperty
 * @tc.desc: Call "StoreProperty" function,check whether the Property is stored successfully by checking returned value.
 *           according to the ObjectOperation object,the stored Property is different,the stored ObjectOperation object
 *           is Found.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, StoreProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromCanBeCompressString("key"));
    int index = 2;
    PropertyAttributes handleAttriButes(2);
    handleAttriButes.SetIsInlinedProps(true);

    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    ObjectOperator handleOp1(thread, handleKey);
    handleOp1.SetValue(cellHandle.GetTaggedValue());
    // test op value is PropertyBox
    JSHandle<JSTaggedValue> handlerInfo1 = StoreHandler::StoreProperty(thread, handleOp1);
    EXPECT_TRUE(handlerInfo1->IsPropertyBox());
    // test op is FastMode/Found and not AccessorDescriptor
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    ObjectOperator handleOp2(thread, handleReceiver, handleKey);
    handleOp2.SetFastMode(true);
    handleOp2.SetIndex(index);
    JSHandle<JSTaggedValue> handlerInfo2 = StoreHandler::StoreProperty(thread, handleOp2);
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo2->GetInt()), 2);
    EXPECT_FALSE(HandlerBase::IsAccessor(handlerInfo2->GetInt()));
    // test op is InlinedProps/Found and not AccessorDescriptor
    handleOp2.SetAttr(handleAttriButes);
    JSHandle<JSTaggedValue> handlerInfo3 = StoreHandler::StoreProperty(thread, handleOp2);
    EXPECT_EQ(HandlerBase::GetKind(handlerInfo3->GetInt()), HandlerKind::FIELD);
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo3->GetInt()), 6);
    EXPECT_TRUE(HandlerBase::IsInlinedProps(handlerInfo3->GetInt()));
}

/**
 * @tc.name: StoreTransition
 * @tc.desc: Call "StoreTransition" function,check whether the Transition is stored successfully by checking
 *           returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, StoreTransition)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromCanBeCompressString("key"));

    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    JSHandle<JSTaggedValue> handleHolder(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    ObjectOperator handleOp(thread, handleHolder, handleKey);
    handleOp.SetValue(cellHandle.GetTaggedValue());

    JSHandle<JSTaggedValue> handlerValue = TransitionHandler::StoreTransition(thread, handleOp);
    JSHandle<TransitionHandler> handler = JSHandle<TransitionHandler>::Cast(handlerValue);
    EXPECT_TRUE(handler->GetHandlerInfo().IsPropertyBox());
    EXPECT_TRUE(handler->GetTransitionHClass().IsHeapObject());
}

/**
 * @tc.name: LoadPrototype
 * @tc.desc: Call "LoadPrototype" function,check whether the Prototype is loaded successfully by checking returned value
 *           according to the ObjectOperation object,the stored Property is different.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, LoadPrototype)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleKey(factory->NewFromCanBeCompressString("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));

    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> handleObj1 = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSObject> handleObj2 = JSObject::ObjectCreate(thread, handleObj1);

    JSHandle<JSHClass> obj1Dynclass(thread, handleObj1->GetJSHClass());
    JSHandle<JSHClass> obj2Dynclass(thread, handleObj2->GetJSHClass());

    ObjectOperator handleOp1(thread, handleKey, OperatorType::OWN);
    ObjectOperator handleOp2(thread, handleKey, OperatorType::OWN);
    handleOp1.SetFastMode(true);
    handleOp2.SetFastMode(true);
    handleOp2.SetIndex(2);
    // test op is not Found and hclass has no Prototype
    JSHandle<JSTaggedValue> handlerValue1 = LoadHandler::LoadProperty(thread, handleOp1);
    EXPECT_TRUE(HandlerBase::IsNonExist(handlerValue1->GetInt()));
    // test op is Found and hclass has Prototype
    JSHandle<JSTaggedValue> handlerValue2 = PrototypeHandler::LoadPrototype(thread, handleOp2, obj2Dynclass);
    JSHandle<PrototypeHandler> handler2 =  JSHandle<PrototypeHandler>::Cast(handlerValue2);
    JSHandle<JSTaggedValue> handlerInfo2(thread, handler2->GetHandlerInfo());
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo2->GetInt()), 2);
    JSHandle<JSTaggedValue> resultMarker(thread, handler2->GetProtoCell());
    EXPECT_TRUE(resultMarker->IsProtoChangeMarker());
    EXPECT_TRUE(handler2->GetHolder().IsJSGlobalObject());
}

/**
 * @tc.name: StorePrototype
 * @tc.desc: Call StorePrototype function,check whether the Prototype is stored successfully by checking returned value
 *           according to the ObjectOperation object,the stored Property is different.the stored ObjectOperation object
 *           must be Found.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ICHandlerTest, StorePrototype)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleKey(factory->NewFromCanBeCompressString("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));

    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> nullObj = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSObject> handleObj = JSObject::ObjectCreate(thread, nullObj);

    JSHandle<JSHClass> objDynclass(thread, handleObj->GetJSHClass());

    ObjectOperator handleOp(thread, handleKey, OperatorType::OWN);
    handleOp.SetFastMode(true);
    handleOp.SetIndex(2);
    // test hclass has Prototype
    JSHandle<JSTaggedValue> handlerValue = PrototypeHandler::StorePrototype(thread, handleOp, objDynclass);
    JSHandle<PrototypeHandler> handler =  JSHandle<PrototypeHandler>::Cast(handlerValue);
    JSHandle<JSTaggedValue> handlerInfo(thread, handler->GetHandlerInfo());
    EXPECT_EQ(HandlerBase::GetOffset(handlerInfo->GetInt()), 2);
    JSHandle<JSTaggedValue> resultMarker(thread, handler->GetProtoCell());
    EXPECT_TRUE(resultMarker->IsProtoChangeMarker());
    EXPECT_TRUE(handler->GetHolder().IsJSGlobalObject());
}
} // namespace panda::test