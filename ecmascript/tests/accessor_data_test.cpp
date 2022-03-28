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

#include "ecmascript/global_env.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class AccessorDataTest : public testing::Test {
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
 * @tc.name: Cast
 * @tc.desc: Convert an object of type ObjectHeader to type AccessorData object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, AccessorData_Cast)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> accDynclassHandle =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    ObjectHeader *accObject = factory->NewDynObject(accDynclassHandle);
    EXPECT_TRUE(JSTaggedValue(accObject).IsAccessorData());
    AccessorData *acc = AccessorData::Cast(accObject);
    EXPECT_TRUE(JSTaggedValue(acc).IsAccessorData());

    JSHandle<JSHClass> internalAccDynClassHandle =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    ObjectHeader *internalAccObject = factory->NewDynObject(internalAccDynClassHandle);
    EXPECT_TRUE(JSTaggedValue(internalAccObject).IsInternalAccessor());
    AccessorData *internalAcc = AccessorData::Cast(internalAccObject);
    EXPECT_TRUE(JSTaggedValue(internalAcc).IsInternalAccessor());
}

/**
 * @tc.name: IsInternal
 * @tc.desc: Judge whether the accessor is internal.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, IsInternal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> accHandle = factory->NewAccessorData();
    EXPECT_EQ(accHandle->IsInternal(), false);

    void *setter = nullptr;
    void *getter = nullptr;
    JSHandle<AccessorData> internalAccHancle = factory->NewInternalAccessor(setter, getter);
    EXPECT_EQ(internalAccHancle->IsInternal(), true);

    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accDynclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    ObjectHeader *accObject = factory->NewDynObject(accDynclass);
    AccessorData *acc = AccessorData::Cast(accObject);
    EXPECT_EQ(acc->IsInternal(), false);

    JSHandle<JSHClass> internalAccDynClass =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    ObjectHeader *internalAccObject = factory->NewDynObject(internalAccDynClass);
    AccessorData *internalAcc = AccessorData::Cast(internalAccObject);
    EXPECT_EQ(internalAcc->IsInternal(), true);
}

/**
 * @tc.name: HasSetter
 * @tc.desc: Judge whether the accessor have a undefined type "Setter".
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, HasSetter)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> normalFunction = globalEnv->GetNormalFunctionClass();

    // 1.Create normal AccessorData object by NewAccessorData function.
    JSHandle<AccessorData> accHandle = factory->NewAccessorData();
    EXPECT_EQ(accHandle->HasSetter(), false);
    accHandle->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(accHandle->HasSetter(), false);
    accHandle->SetSetter(thread, normalFunction);
    EXPECT_EQ(accHandle->HasSetter(), true);

    // 2.Create internal AccessorData object by NewInternalAccessor function.
    void *setter = nullptr;
    void *getter = nullptr;
    JSHandle<AccessorData> internalAccHandle = factory->NewInternalAccessor(setter, getter);
    EXPECT_EQ(internalAccHandle->HasSetter(), false);
    internalAccHandle->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(internalAccHandle->HasSetter(), false);
    internalAccHandle->SetSetter(thread, normalFunction);
    EXPECT_EQ(internalAccHandle->HasSetter(), true);

    // 3.Create normal AccessorData object from dynamic class.
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accDynclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::ACCESSOR_DATA, nullHandle);
    ObjectHeader *accObject = factory->NewDynObject(accDynclass);
    AccessorData *acc = AccessorData::Cast(accObject);
    EXPECT_EQ(acc->HasSetter(), true);
    acc->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(acc->HasSetter(), false);
    acc->SetSetter(thread, normalFunction);
    EXPECT_EQ(acc->HasSetter(), true);

    // 4.Create internal AccessorData object from dynamic class.
    JSHandle<JSHClass> internalAccDynClass =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullHandle);
    ObjectHeader *internalAccObject = factory->NewDynObject(internalAccDynClass);
    AccessorData *internalAcc = AccessorData::Cast(internalAccObject);
    EXPECT_EQ(internalAcc->HasSetter(), true);
    internalAcc->SetSetter(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(internalAcc->HasSetter(), false);
    internalAcc->SetSetter(thread, normalFunction);
    EXPECT_EQ(internalAcc->HasSetter(), true);
}

/**
 * @tc.name: CallInternalGet
 * @tc.desc: Call internal get function to get object prototype.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, CallInternalGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();

    // Construct objects and specify specific prototypes.
    JSFunction *func1 = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSFunction *func2 = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> funcTagVal1(thread, func1);
    JSHandle<JSTaggedValue> funcTagVal2(thread, func2);
    JSHandle<JSObject> jsObjectHandle1 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(funcTagVal1), funcTagVal1);
    JSHandle<JSObject> jsObjectHandle2 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(funcTagVal2), funcTagVal2);
    char array1[] = "x";
    char array2[] = "y";
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString(array1));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString(array2));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(100));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObjectHandle1), key1, value);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObjectHandle2), key2, value);

    JSHandle<JSTaggedValue> nullPrototypeHandle(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> undefPrototypeHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSHClass> accDynclass1 =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullPrototypeHandle);
    JSHandle<JSHClass> accDynclass2 =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, undefPrototypeHandle);
    ObjectHeader *accObject1 = factory->NewDynObject(accDynclass1);
    ObjectHeader *accObject2 = factory->NewDynObject(accDynclass2);
    AccessorData *acc1 = AccessorData::Cast(accObject1);
    AccessorData *acc2 = AccessorData::Cast(accObject2);
    JSHandle<JSNativePointer> prototypeGetterFuncNativePtrHandle =
        factory->NewJSNativePointer(reinterpret_cast<void *>(JSFunction::PrototypeGetter), nullptr, nullptr, true);
    acc1->SetGetter(thread, prototypeGetterFuncNativePtrHandle);
    acc2->SetGetter(thread, prototypeGetterFuncNativePtrHandle);
    ObjectOperator op1(thread, jsObjectHandle1, key1, OperatorType::OWN);
    ObjectOperator op2(thread, jsObjectHandle2, key2, OperatorType::OWN);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    op1.AddProperty(jsObjectHandle1, nullPrototypeHandle, attr);
    op2.AddProperty(jsObjectHandle2, undefPrototypeHandle, attr);
    JSHandle<JSTaggedValue> holder1 = op1.GetHolder();
    JSHandle<JSTaggedValue> holder2 = op2.GetHolder();

    // Call CallInternalGet function to get prototypes.
    JSTaggedValue valNullPrototype = acc1->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder1));
    JSTaggedValue valUndefPrototype = acc2->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder2));
    EXPECT_EQ(valNullPrototype.GetInt(), JSTaggedValue::Null().GetInt());
    EXPECT_EQ(valUndefPrototype.GetInt(), JSTaggedValue::Undefined().GetInt());
}

/**
 * @tc.name: CallInternalSet
 * @tc.desc: Call internal set function to set object prototype.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, CallInternalSet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();

    // Construct objects and specify specific prototypes.
    JSFunction *func1 = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> funcTagVal1(thread, func1);
    JSHandle<JSObject> jsObjectHandle1 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(funcTagVal1), funcTagVal1);
    char array1[] = "x";
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString(array1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(100));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObjectHandle1), key1, value);

    // Call the CallInternalGet method to inspect prototype.
    JSHandle<JSTaggedValue> nullPrototypeHandle(thread, JSTaggedValue::Null());
    JSHandle<JSHClass> accDynclass1 =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::INTERNAL_ACCESSOR, nullPrototypeHandle);
    ObjectHeader *accObject1 = factory->NewDynObject(accDynclass1);
    AccessorData *acc1 = AccessorData::Cast(accObject1);
    JSHandle<JSNativePointer> prototypeGetterFuncNativePtrHandle =
        factory->NewJSNativePointer(reinterpret_cast<void *>(JSFunction::PrototypeGetter), nullptr, nullptr, true);
    acc1->SetGetter(thread, prototypeGetterFuncNativePtrHandle);
    ObjectOperator op1(thread, jsObjectHandle1, key1, OperatorType::OWN);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    op1.AddProperty(jsObjectHandle1, nullPrototypeHandle, attr);
    JSHandle<JSTaggedValue> holder1 = op1.GetHolder();

    JSTaggedValue valNullPrototype = acc1->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder1));
    EXPECT_EQ(valNullPrototype.GetInt(), JSTaggedValue::Null().GetInt());

    // Call the CallInternalSet method to set new prototype.
    JSHandle<JSTaggedValue> undefPrototypeHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSNativePointer> prototypeSetterFuncNativePtrHandle =
        factory->NewJSNativePointer(reinterpret_cast<void *>(JSFunction::PrototypeSetter), nullptr, nullptr, true);
    acc1->SetSetter(thread, prototypeSetterFuncNativePtrHandle);
    bool res1 = acc1->CallInternalSet(thread, JSHandle<JSObject>::Cast(holder1), undefPrototypeHandle);
    EXPECT_TRUE(res1);

    // Call the CallInternalGet method to check the changed prototype.
    valNullPrototype = acc1->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder1));
    EXPECT_EQ(valNullPrototype.GetInt(), JSTaggedValue::Undefined().GetInt());
}

/**
 * @tc.name: Cast
 * @tc.desc: Convert an object of type ObjectHeader to type CompletionRecord object.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, CompletionRecord_Cast)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    JSHandle<JSHClass> comRecordDynclassHandle =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::COMPLETION_RECORD, nullHandle);
    ObjectHeader *comRecordObject = factory->NewDynObject(comRecordDynclassHandle);
    EXPECT_TRUE(JSTaggedValue(comRecordObject).IsCompletionRecord());
    CompletionRecord *comRecord = CompletionRecord::Cast(comRecordObject);
    EXPECT_TRUE(JSTaggedValue(comRecord).IsCompletionRecord());
}

/**
 * @tc.name: IsThrow
 * @tc.desc: Judge whether the completion record is THROW type.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(AccessorDataTest, IsThrow)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> exceptionHandle(thread, thread->GetException());

    JSHandle<CompletionRecord> normalComRecHandle =
        factory->NewCompletionRecord(CompletionRecordType::NORMAL, exceptionHandle);
    EXPECT_TRUE(!normalComRecHandle->IsThrow());

    JSHandle<CompletionRecord> throwComRecHandle =
        factory->NewCompletionRecord(CompletionRecordType::THROW, exceptionHandle);
    EXPECT_TRUE(throwComRecHandle->IsThrow());
}
}  // namespace panda::test
