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

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ProfileTypeInfoTest : public testing::Test {
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
 * @tc.name: ICKindToString
 * @tc.desc: Convert its name into a string according to different types of IC.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, ICKindToString)
{
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedLoadIC)).c_str(), "NamedLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::LoadIC)).c_str(), "LoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::NamedGlobalLoadIC)).c_str(), "NamedGlobalLoadIC");
    EXPECT_STREQ(CString(ICKindToString(ICKind::GlobalLoadIC)).c_str(), "GlobalLoadIC");
}

/**
 * @tc.name: GetICState
 * @tc.desc: Define a TaggedArray object with a length of six. Set different values and convert it into a
 *           profiletypeinfo object.Then define different profiletypeaccessor objects according to each
 *           element in the array, and get the IC state through the "GetICState" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetICState)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> newArray(factory->NewTaggedArray(2)); // 2 : test case
    JSHandle<JSTaggedValue> newBox(factory->NewPropertyBox(newValue));

    uint32_t arrayLength = 6;
    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(arrayLength);
    handleDetailsArray->Set(thread, 0, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 1, JSTaggedValue::Hole());
    handleDetailsArray->Set(thread, 2, newArray.GetTaggedValue());
    handleDetailsArray->Set(thread, 3, newArray.GetTaggedValue());
    handleDetailsArray->Set(thread, 4, newBox.GetTaggedValue());
    handleDetailsArray->Set(thread, 5, newArray.GetTaggedValue());

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);
    // slotId = 0
    ProfileTypeAccessor handleProfileTypeAccessor0(thread, handleProfileTypeInfo, 0, ICKind::StoreIC);
    // slotId = 1
    ProfileTypeAccessor handleProfileTypeAccessor1(thread, handleProfileTypeInfo, 1, ICKind::GlobalLoadIC);
    // slotId = 2
    ProfileTypeAccessor handleProfileTypeAccessor2(thread, handleProfileTypeInfo, 2, ICKind::NamedLoadIC);
    // slotId = 3
    ProfileTypeAccessor handleProfileTypeAccessor3(thread, handleProfileTypeInfo, 3, ICKind::LoadIC);
    // slotId = 4
    ProfileTypeAccessor handleProfileTypeAccessor4(thread, handleProfileTypeInfo, 4, ICKind::NamedGlobalLoadIC);
    // slotId = 5
    ProfileTypeAccessor handleProfileTypeAccessor5(thread, handleProfileTypeInfo, 5, ICKind::GlobalStoreIC);

    EXPECT_EQ(handleProfileTypeAccessor0.GetICState(), ProfileTypeAccessor::ICState::UNINIT);
    EXPECT_EQ(handleProfileTypeAccessor1.GetICState(), ProfileTypeAccessor::ICState::MEGA);
    EXPECT_EQ(handleProfileTypeAccessor2.GetICState(), ProfileTypeAccessor::ICState::POLY);
    EXPECT_EQ(handleProfileTypeAccessor3.GetICState(), ProfileTypeAccessor::ICState::MONO);
    EXPECT_EQ(handleProfileTypeAccessor4.GetICState(), ProfileTypeAccessor::ICState::MONO);
    EXPECT_EQ(handleProfileTypeAccessor5.GetICState(), ProfileTypeAccessor::ICState::MONO);
}

/**
 * @tc.name: AddHandlerWithoutKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddHandlerWithoutKey" function to add Handler,This function cannot pass the key value.
 *           When different profiletypeaccessor objects call the AddHandlerWithoutKey function,the handlers stored
 *           in the array are different.Check whether the data stored in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddHandlerWithoutKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(2));

    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(1));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(2));

    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(4);
    handleDetailsArray->Set(thread, 0, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 1, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 2, JSTaggedValue::Hole());
    handleDetailsArray->Set(thread, 3, JSTaggedValue::Undefined());
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);

    uint32_t slotId = 0; // test profileData is Undefined
    ProfileTypeAccessor handleProfileTypeAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor0.AddHandlerWithoutKey(objDynclassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId).IsWeak());
    EXPECT_EQ(handleProfileTypeInfo->Get(slotId + 1).GetInt(), 2);

    slotId = 1; // test POLY
    handleProfileTypeInfo->Set(thread, slotId, handleTaggedArray.GetTaggedValue()); // Reset Value
    ProfileTypeAccessor handleProfileTypeAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor1.AddHandlerWithoutKey(objDynclassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId).IsTaggedArray());
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(slotId));
    EXPECT_EQ(resultArr1->GetLength(), 4); // 4 = 2 + 2
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId + 1).IsHole());

    slotId = 2; // test MONO to POLY
    handleProfileTypeInfo->Set(thread, slotId, HandlerValue.GetTaggedValue()); // Reset Value
    ProfileTypeAccessor handleProfileTypeAccessor2(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor2.AddHandlerWithoutKey(objDynclassVal, HandlerValue);
    JSHandle<TaggedArray> resultArr2(thread, handleProfileTypeInfo->Get(slotId));
    EXPECT_EQ(resultArr2->GetLength(), 4); // POLY_DEFAULT_LEN
    EXPECT_EQ(resultArr2->Get(0).GetInt(), 2);
    EXPECT_TRUE(resultArr2->Get(1).IsUndefined());
    EXPECT_TRUE(resultArr2->Get(2).IsWeak());
    EXPECT_EQ(resultArr2->Get(3).GetInt(), 2);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId + 1).IsHole());
}

/**
 * @tc.name: AddElementHandler
 * @tc.desc: Define a TaggedArray object with a length of four.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddElementHandler" function to add Handler,This function pass the class value/key value.
 *           When different profiletypeaccessor objects call the AddHandlerWithoutKey function,the handlers stored
 *           in the array are different.Check whether the data stored in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddElementHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(3));

    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(4);
    handleDetailsArray->Set(thread, 0, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 1, JSTaggedValue::Undefined());
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);

    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor0.AddElementHandler(objDynclassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(0).IsWeak());
    EXPECT_EQ(handleProfileTypeInfo->Get(1).GetInt(), 3);
}

/**
 * @tc.name: AddHandlerWithKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,profiletypeaccessor
 *           object call "AddHandlerWithKey" function to add Handler,This function pass the key value/class value
 *           and handler value.When different profiletypeaccessor objects call the AddHandlerWithoutKey function,
 *           the handlers stored in the array are different.Check whether the data stored in the array is the same
 *           as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddHandlerWithKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> objDynclassVal(thread, handleObj->GetJSHClass());
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> HandleKey0(factory->NewFromCanBeCompressString("key0"));
    JSHandle<JSTaggedValue> HandleKey1(factory->NewFromCanBeCompressString("key1"));

    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(1));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(2));

    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(4);
    handleDetailsArray->Set(thread, 0, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 1, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 2, handleTaggedArray.GetTaggedValue());
    handleDetailsArray->Set(thread, 3, handleTaggedArray.GetTaggedValue());
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);

    uint32_t slotId = 0; // test profileData is Undefined
    ProfileTypeAccessor handleProfileTypeAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor0.AddHandlerWithKey(HandleKey1, objDynclassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(0).IsString());
    EXPECT_TRUE(handleProfileTypeInfo->Get(1).IsTaggedArray());

    slotId = 1; // test profileData is equal the key
    handleProfileTypeInfo->Set(thread, slotId, HandleKey1.GetTaggedValue()); // Reset Value
    ProfileTypeAccessor handleProfileTypeAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor1.AddHandlerWithKey(HandleKey1, objDynclassVal, HandlerValue);
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(slotId + 1));
    EXPECT_EQ(resultArr1->GetLength(), 4); // 4 = 2 + 2

    slotId = 2; // test profileData is not equal the key
    ProfileTypeAccessor handleProfileTypeAccessor2(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor2.AddHandlerWithKey(HandleKey0, objDynclassVal, HandlerValue);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId + 1).IsHole());
}

/**
 * @tc.name: AddGlobalHandlerKey
 * @tc.desc: Define a TaggedArray object with a length of two.Set values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,different
 *           ProfileTypeAccessor objects call the "AddGlobalHandlerKey" function,the data stored in the array in
 *           the object is different.This function pass the key value/handler value,Check whether the data stored
 *           in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddGlobalHandlerKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> HandlerValue(thread, JSTaggedValue(222));
    JSHandle<TaggedArray> handleTaggedArray = factory->NewTaggedArray(2);
    handleTaggedArray->Set(thread, 0, JSTaggedValue(111));
    handleTaggedArray->Set(thread, 1, JSTaggedValue(333));
    JSHandle<JSTaggedValue> arrayKey(factory->NewFromCanBeCompressString("array"));

    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(2);
    handleDetailsArray->Set(thread, 0, JSTaggedValue::Undefined());
    handleDetailsArray->Set(thread, 1, handleTaggedArray.GetTaggedValue());
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);

    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor0(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor0.AddGlobalHandlerKey(arrayKey, HandlerValue);
    JSHandle<TaggedArray> resultArr1(thread, handleProfileTypeInfo->Get(slotId));
    EXPECT_EQ(resultArr1->GetLength(), 2);
    EXPECT_TRUE(resultArr1->Get(0).IsWeak());
    EXPECT_EQ(resultArr1->Get(1).GetInt(), 222);
    
    slotId = 1;
    ProfileTypeAccessor handleProfileTypeAccessor1(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor1.AddGlobalHandlerKey(arrayKey, HandlerValue);
    JSHandle<TaggedArray> resultArr2(thread, handleProfileTypeInfo->Get(slotId));
    EXPECT_EQ(resultArr2->GetLength(), 4);
    EXPECT_TRUE(resultArr2->Get(0).IsWeak());
    EXPECT_EQ(resultArr2->Get(1).GetInt(), 222);
    EXPECT_EQ(resultArr2->Get(2).GetInt(), 111);
    EXPECT_EQ(resultArr2->Get(3).GetInt(), 333);
}

/**
 * @tc.name: AddGlobalRecordHandler
 * @tc.desc: Define a TaggedArray object with a length of two.Set no values and convert it into a profiletypeinfo
 *           object.Then define profiletypeaccessor objects according to element in the array,different
 *           ProfileTypeAccessor objects call the "AddGlobalRecordHandler" function,the data stored in the array in
 *           the object is the same.This function pass the handler value,Check whether the data stored
 *           in the array is the same as expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, AddGlobalRecordHandler)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> HandlerValue1(thread, JSTaggedValue(232));
    JSHandle<JSTaggedValue> HandlerValue2(thread, JSTaggedValue(5));
    
    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(2);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);
    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor.AddGlobalRecordHandler(HandlerValue1);
    EXPECT_EQ(handleProfileTypeInfo->Get(slotId).GetInt(), 232);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId + 1).IsHole());

    handleProfileTypeAccessor.AddGlobalRecordHandler(HandlerValue2);
    EXPECT_EQ(handleProfileTypeInfo->Get(slotId).GetInt(), 5);
    EXPECT_TRUE(handleProfileTypeInfo->Get(slotId + 1).IsHole());
}

/**
 * @tc.name: SetAsMega
 * @tc.desc: The function of the "SetAsMega" function is to set the element of the array in the handleProfileTypeInfo
 *           object as hole,so call the "SetAsMega" function to check whether the element of the array is hole.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, SetAsMega)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(arrayLength);
    handleDetailsArray->Set(thread, 0, JSTaggedValue(111));
    handleDetailsArray->Set(thread, 1, JSTaggedValue(222));

    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);
    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    handleProfileTypeAccessor.SetAsMega();

    EXPECT_TRUE(handleProfileTypeInfo->Get(0).IsHole());
    EXPECT_TRUE(handleProfileTypeInfo->Get(1).IsHole());
}

/**
 * @tc.name: GetWeakRef
 * @tc.desc: The function of this function is to create and get the weakref for The elements in the array of the defined
 *           handleProfileTypeInfo object.call the "GetWeakRef" function is to check whether the elements in the array
 *           have weak data.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetWeakRef)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> handleObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue weakRefValue(handleObj.GetTaggedValue());

    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(arrayLength);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);

    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    EXPECT_TRUE(handleProfileTypeAccessor.GetWeakRef(weakRefValue).IsWeak());
}

/**
 * @tc.name: GetRefFromWeak
 * @tc.desc: The function of this function is to get the weakref.The elements in the array of the defined
 *           handleProfileTypeInfo object call the "CreateWeakRef" function to create the data of the weakref.
 *           call the "GetRefFromWeak" function is to check whether the elements in the array have data of calling
 *           "CreateWeakRef".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ProfileTypeInfoTest, GetRefFromWeak)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    
    JSHandle<JSObject> handleProfileTypeValue = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSTaggedValue handleProfileType(handleProfileTypeValue.GetTaggedValue());
    handleProfileType.CreateWeakRef();

    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> handleDetailsArray = factory->NewTaggedArray(arrayLength);
    JSHandle<ProfileTypeInfo> handleProfileTypeInfo = JSHandle<ProfileTypeInfo>::Cast(handleDetailsArray);
    EXPECT_TRUE(*handleProfileTypeInfo != nullptr);

    uint32_t slotId = 0;
    ProfileTypeAccessor handleProfileTypeAccessor(thread, handleProfileTypeInfo, slotId, ICKind::StoreIC);
    EXPECT_EQ(handleProfileTypeAccessor.GetRefFromWeak(handleProfileType).GetTaggedObject(),
                                                                           handleProfileType.GetTaggedWeakRef());
}
} // namespace panda::test