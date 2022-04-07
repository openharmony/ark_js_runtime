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

#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/weak_vector.h"

using namespace panda::ecmascript;

namespace panda::test {
class ProtoChangeDetailsTest : public testing::Test {
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
 * @tc.name: SetBitField
 * @tc.desc: Check whether the result returned through "GetBitField" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, SetBitField)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ProtoChangeMarker> handleProtoChangeMarker = factory->NewProtoChangeMarker();

    handleProtoChangeMarker->SetBitField(true);
    EXPECT_TRUE(handleProtoChangeMarker->GetBitField());

    handleProtoChangeMarker->SetBitField(false);
    EXPECT_FALSE(handleProtoChangeMarker->GetBitField());
}

/**
 * @tc.name: SetHasChanged
 * @tc.desc: Check whether the result returned through "GetHasChanged" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, SetHasChanged)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ProtoChangeMarker> handleProtoChangeMarker = factory->NewProtoChangeMarker();

    handleProtoChangeMarker->SetHasChanged(true);
    EXPECT_TRUE(handleProtoChangeMarker->GetHasChanged());

    handleProtoChangeMarker->SetHasChanged(false);
    EXPECT_FALSE(handleProtoChangeMarker->GetHasChanged());
}

/**
 * @tc.name: SetChangeListener
 * @tc.desc: Check whether the result returned through "GetChangeListener" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, SetChangeListener)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    
    array_size_t weakVectorCapacity = 100;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    JSHandle<ChangeListener> handleChangeListener = JSHandle<ChangeListener>::Cast(weakVector);
    JSHandle<ProtoChangeDetails> handleChangeDetails = factory->NewProtoChangeDetails();

    handleChangeDetails->SetChangeListener(thread, handleValue.GetTaggedValue());
    EXPECT_EQ(handleChangeDetails->GetChangeListener().GetInt(), 1);
    handleChangeDetails->SetChangeListener(thread, handleChangeListener.GetTaggedValue());
    EXPECT_EQ(handleChangeDetails->GetChangeListener(), handleChangeListener.GetTaggedValue());
}

/**
 * @tc.name: SetRegisterIndex
 * @tc.desc: Check whether the result returned through "GetRegisterIndex" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, SetRegisterIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ProtoChangeDetails> handleChangeDetails = factory->NewProtoChangeDetails();

    handleChangeDetails->SetRegisterIndex(static_cast<uint32_t>(1));
    EXPECT_EQ(handleChangeDetails->GetRegisterIndex(), 1U);
}

/**
 * @tc.name: Add_001
 * @tc.desc: Create a weakvector object with a length of ten,Call the "pushback" function to set the value of the
 *           vector.the vector is not fully set,convert the weakvector object into a ChangeListener object.In this case,
 *           call the "add" function to add a jshclass object. The added jshclass object will create and get weakref and
 *           return the location of the added object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, Add_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    array_size_t weakVectorCapacity = 10;
    array_size_t index = 2;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    for (int i = 0; i < 9; i++) {
        weakVector->PushBack(thread, JSTaggedValue(i));
    } // weakVector is not full
    JSHandle<ChangeListener> handleChangeListenerArr = JSHandle<ChangeListener>::Cast(weakVector);
    JSHandle<ChangeListener> resultListenerArray =
        ChangeListener::Add(thread, handleChangeListenerArr, JSHandle<JSHClass>(objDynclassVal), &index);
    EXPECT_EQ(index, 9U);
    JSTaggedValue weakRefValue(objDynclassVal.GetTaggedValue());
    weakRefValue.CreateWeakRef();
    EXPECT_EQ(resultListenerArray->Get(index).GetTaggedObject(), weakRefValue.GetTaggedWeakRef());
}

/**
 * @tc.name: Add_002
 * @tc.desc: Create a weakvector object with a length of ten,Call the "pushback" function to set the value of the
 *           vector the vector is fully set but exist hole,convert the weakvector object into a ChangeListener object
 *           in this case call the "add" function to add a jshclass object. The added jshclass object will create and
 *           get weakref and return the location of the added object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, Add_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    array_size_t weakVectorCapacity = 10;
    array_size_t index = 2;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    for (int i = 0; i < 10; i++) {
        weakVector->PushBack(thread, JSTaggedValue(i));
    }
    JSHandle<ChangeListener> handleChangeListenerArr = JSHandle<ChangeListener>::Cast(weakVector);
    // weakVector exist hole
    weakVector->Delete(thread, 1); // delete in one
    JSHandle<ChangeListener> resultListenerArray =
    ChangeListener::Add(thread, handleChangeListenerArr, JSHandle<JSHClass>(objDynclassVal), &index);
    EXPECT_EQ(index, 1U);
    JSTaggedValue weakRefValue(objDynclassVal.GetTaggedValue());
    weakRefValue.CreateWeakRef();
    EXPECT_EQ(resultListenerArray->Get(index).GetTaggedObject(), weakRefValue.GetTaggedWeakRef());
}

/**
 * @tc.name: Add_003
 * @tc.desc: Create a weakvector object with a length of ten,Call the "pushback" function to set the value of the
 *           vector the vector is fully set and not exist hole,convert the weakvector object into a ChangeListener
 *           object.in this case call the "add" function increase the length to add the jshclass object. The added
 *           jshclass object will create and get weakref and return the location of the added object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, Add_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    array_size_t weakVectorCapacity = 10;
    array_size_t index = 2;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    for (int i = 0; i < 10; i++) {
        weakVector->PushBack(thread, JSTaggedValue(i));
    }
    JSHandle<ChangeListener> handleChangeListenerArr = JSHandle<ChangeListener>::Cast(weakVector);
    // weakVector is full and no hole exists.
    JSHandle<ChangeListener> resultListenerArray =
    ChangeListener::Add(thread, handleChangeListenerArr, JSHandle<JSHClass>(objDynclassVal), &index);
    EXPECT_EQ(index, 10U);
    JSTaggedValue weakRefValue(objDynclassVal.GetTaggedValue());
    weakRefValue.CreateWeakRef();
    EXPECT_EQ(resultListenerArray->Get(index).GetTaggedObject(), weakRefValue.GetTaggedWeakRef());
}

/**
 * @tc.name: CheckHole
 * @tc.desc: Create a weakvector object with a length of ten,Call the "pushback" function to set the value of the
 *           vector the vector is fully set,convert the weakvector object into a ChangeListener object.ChangeListener
 *           object call the "CheckHole" function to check whether there is a hole. If there is, return the location
 *           of the hole If not, return the maximum value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, CheckHole)
{
    array_size_t weakVectorCapacity = 10;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    for (int i = 0; i < 10; i++) {
        weakVector->PushBack(thread, JSTaggedValue(i)); // Set Value and End
    }
    JSHandle<ChangeListener> handleChangeListenerArr = JSHandle<ChangeListener>::Cast(weakVector);
    EXPECT_EQ(ChangeListener::CheckHole(handleChangeListenerArr), TaggedArray::MAX_ARRAY_INDEX);
    weakVector->Delete(thread, 1);
    EXPECT_EQ(ChangeListener::CheckHole(handleChangeListenerArr), 1U);
}

/**
 * @tc.name: Get
 * @tc.desc: Create a weakvector object with a length of three,Call the "pushback" function to set the value of the
 *           vector the vector is fully set,convert the weakvector object into a ChangeListener object.check whether
 *           the return value of calling "Get" function is the same as the value set by calling the "Set" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProtoChangeDetailsTest, Get)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue objValue(handleObj.GetTaggedValue());
    objValue.CreateWeakRef();
    JSHandle<JSTaggedValue> weakRefValue(thread, objValue);

    array_size_t weakVectorCapacity = 3;
    JSHandle<WeakVector> weakVector = WeakVector::Create(thread, weakVectorCapacity);
    weakVector->Set(thread, 0, JSTaggedValue(0));
    weakVector->Set(thread, 1, weakRefValue.GetTaggedValue());
    weakVector->Set(thread, 2, JSTaggedValue::Undefined());

    JSHandle<ChangeListener> handleChangeListenerArr = JSHandle<ChangeListener>::Cast(weakVector);
    EXPECT_TRUE(*handleChangeListenerArr != nullptr);
    EXPECT_EQ(handleChangeListenerArr->Get(0).GetInt(), 0);
    EXPECT_NE(handleChangeListenerArr->Get(1), weakRefValue.GetTaggedValue());
    EXPECT_TRUE(handleChangeListenerArr->Get(2).IsUndefined());
}
} // namespace panda::test