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

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSTypedArrayTest : public testing::Test {
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

    const CVector<JSType> cVecJSType {
        JSType::JS_INT8_ARRAY, JSType::JS_UINT8_ARRAY, JSType::JS_UINT8_CLAMPED_ARRAY,
        JSType::JS_INT16_ARRAY, JSType::JS_UINT16_ARRAY,
        JSType::JS_INT32_ARRAY, JSType::JS_UINT32_ARRAY,
        JSType::JS_FLOAT32_ARRAY, JSType::JS_FLOAT64_ARRAY};

    // CVector pushed with JSTaggedValue made from compatible input value for the JSType
    const CVector<JSTaggedValue> cVecHandleTagValValueForTypedArray {
        // Use "(S)(...)" cast to make v in "JSTaggedValue(T v) : coretypes::TaggedValue(v) {}" compatible with S
        JSTaggedValue((int8_t)(-111)), JSTaggedValue((uint8_t)(222)), JSTaggedValue((uint8_t)(222)),
        JSTaggedValue((int16_t)(-31111)), JSTaggedValue((uint16_t)(61111)),
        // int32 : -2147483648->2147483647, uint32 : 0->4294967295
        JSTaggedValue((int32_t)(2111111111)), JSTaggedValue((uint32_t)(4111111111)),
        JSTaggedValue((float)(4321.1234)), JSTaggedValue((double)(987654321.123456789))
    };
};

JSHandle<JSTypedArray> CreateNumberTypedArray(JSThread *thread, JSType jsType)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleTagValFunc = env->GetInt8ArrayFunction();
    switch (jsType) {
        case JSType::JS_INT8_ARRAY:
            break;
        case JSType::JS_UINT8_ARRAY:
            handleTagValFunc = env->GetUint8ArrayFunction();
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            handleTagValFunc = env->GetUint8ClampedArrayFunction();
            break;
        case JSType::JS_INT16_ARRAY:
            handleTagValFunc = env->GetInt16ArrayFunction();
            break;
        case JSType::JS_UINT16_ARRAY:
            handleTagValFunc = env->GetUint16ArrayFunction();
            break;
        case JSType::JS_INT32_ARRAY:
            handleTagValFunc = env->GetInt32ArrayFunction();
            break;
        case JSType::JS_UINT32_ARRAY:
            handleTagValFunc = env->GetUint32ArrayFunction();
            break;
        case JSType::JS_FLOAT32_ARRAY:
            handleTagValFunc = env->GetFloat32ArrayFunction();
            break;
        case JSType::JS_FLOAT64_ARRAY:
            handleTagValFunc = env->GetFloat64ArrayFunction();
            break;
        default:
            ASSERT_PRINT(false, "the second argument is a wrong JSType for CreateNumberTypedArray function");
            break;
    }

    return JSHandle<JSTypedArray>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(handleTagValFunc), handleTagValFunc));
}

/*
 * Feature: JSTypedArray
 * Function: ToPropKey
 * SubFunction: EcmaString::GetCString
 * FunctionPoints: TaggedType Signs To EcmaString Signs
 * CaseDescription: Check whether the EcmaStrings transformed through calling ToPropKey function from TaggedTypes are
 *                  within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, ToPropKey_001)
{
    JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleHole(thread, JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo1 = JSTypedArray::ToPropKey(thread, handleUndefined);
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo2 = JSTypedArray::ToPropKey(thread, handleHole);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo1 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo1);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo2 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo2);
    EXPECT_NE(0U, sizeof(handleUndefined));
    EXPECT_NE(0U, sizeof(handleHole));
    std::unique_ptr<char[]> uniCharArrTo1(handleEcmaStrPropKeyTo1->GetCString());
    std::unique_ptr<char[]> uniCharArrTo2(handleEcmaStrPropKeyTo2->GetCString());
    EXPECT_EQ(uniCharArrTo1[0], 'u');
    EXPECT_EQ(uniCharArrTo1[1], 'n');
    EXPECT_EQ(uniCharArrTo1[2], 'd');
    EXPECT_EQ(uniCharArrTo1[3], 'e');
    EXPECT_EQ(uniCharArrTo1[4], 'f');
    EXPECT_EQ(uniCharArrTo1[5], 'i');
    EXPECT_EQ(uniCharArrTo1[6], 'n');
    EXPECT_EQ(uniCharArrTo1[7], 'e');
    EXPECT_EQ(uniCharArrTo1[8], 'd');
    EXPECT_EQ(uniCharArrTo1[9], 0); // "undefined"
    EXPECT_EQ(uniCharArrTo2[0], 0); // ""
}

/*
 * Feature: JSTypedArray
 * Function: ToPropKey
 * SubFunction: EcmaString::GetCString
 * FunctionPoints: Number Signs To EcmaString Signs
 * CaseDescription: Check whether the EcmaStrings transformed through calling ToPropKey function from Numbers are
 *                  within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, ToPropKey_002)
{
    JSHandle<JSTaggedValue> handleTagVal1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleTagVal2(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> handleTagVal3(thread, JSTaggedValue(1.789));
    JSHandle<JSTaggedValue> handleTagVal4(thread, JSTaggedValue(-789.1));
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo1 = JSTypedArray::ToPropKey(thread, handleTagVal1);
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo2 = JSTypedArray::ToPropKey(thread, handleTagVal2);
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo3 = JSTypedArray::ToPropKey(thread, handleTagVal3);
    JSHandle<JSTaggedValue> hnadleTagValEcmaStrPropKeyTo4 = JSTypedArray::ToPropKey(thread, handleTagVal4);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo1 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo1);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo2 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo2);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo3 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo3);
    JSHandle<EcmaString> handleEcmaStrPropKeyTo4 = JSHandle<EcmaString>::Cast(hnadleTagValEcmaStrPropKeyTo4);
    std::unique_ptr<char[]> uniCharArrTo1(handleEcmaStrPropKeyTo1->GetCString());
    std::unique_ptr<char[]> uniCharArrTo2(handleEcmaStrPropKeyTo2->GetCString());
    std::unique_ptr<char[]> uniCharArrTo3(handleEcmaStrPropKeyTo3->GetCString());
    std::unique_ptr<char[]> uniCharArrTo4(handleEcmaStrPropKeyTo4->GetCString());
    EXPECT_EQ(uniCharArrTo1[0], '0');
    EXPECT_EQ(uniCharArrTo1[1], 0); // "0"
    EXPECT_EQ(uniCharArrTo2[0], '-');
    EXPECT_EQ(uniCharArrTo2[1], '1');
    EXPECT_EQ(uniCharArrTo2[2], 0); // "-1"
    EXPECT_EQ(uniCharArrTo3[0], '1');
    EXPECT_EQ(uniCharArrTo3[1], '.');
    EXPECT_EQ(uniCharArrTo3[2], '7');
    EXPECT_EQ(uniCharArrTo3[3], '8');
    EXPECT_EQ(uniCharArrTo3[4], '9');
    EXPECT_EQ(uniCharArrTo3[5], 0); // "1.789"
    EXPECT_EQ(uniCharArrTo4[0], '-');
    EXPECT_EQ(uniCharArrTo4[1], '7');
    EXPECT_EQ(uniCharArrTo4[2], '8');
    EXPECT_EQ(uniCharArrTo4[3], '9');
    EXPECT_EQ(uniCharArrTo4[4], '.');
    EXPECT_EQ(uniCharArrTo4[5], '1');
    EXPECT_EQ(uniCharArrTo4[6], 0); // "-789.1"
}

/*
 * Feature: JSTypedArray
 * Function: CreateJSTypedArray(GlobalEnv::GetInt8ArrayFunction.../ObjectFactory::NewJSObjectByConstructor)
 * SubFunction: JSTaggedValue::IsTypedArray/IsJSInt8Array...
 * FunctionPoints: Create JSTypedArray(JSInt8Array...)
 * CaseDescription: Check whether the bools returned through calling IsTypedArray/IsInt8Array... functions from the
 *                  JSTypedArrays created through calling NewJSObjectByConstructor function are within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, TypedArrayCreate)
{
    JSHandle<JSTypedArray> handleInt8Array = CreateNumberTypedArray(thread, JSType::JS_INT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt8Array = JSHandle<JSTaggedValue>::Cast(handleInt8Array);
    EXPECT_TRUE(handleTagValInt8Array->IsJSInt8Array() && handleTagValInt8Array->IsTypedArray());

    JSHandle<JSTypedArray> handleUint8Array = CreateNumberTypedArray(thread, JSType::JS_UINT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8Array = JSHandle<JSTaggedValue>::Cast(handleUint8Array);
    EXPECT_TRUE(handleTagValUint8Array->IsJSUint8Array() && handleTagValUint8Array->IsTypedArray());

    JSHandle<JSTypedArray> handleUint8ClampedArray = CreateNumberTypedArray(thread, JSType::JS_UINT8_CLAMPED_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8ClampedArray = JSHandle<JSTaggedValue>::Cast(handleUint8ClampedArray);
    EXPECT_TRUE(handleTagValUint8ClampedArray->IsJSUint8ClampedArray() &&
        handleTagValUint8ClampedArray->IsTypedArray());

    JSHandle<JSTypedArray> handleInt16Array = CreateNumberTypedArray(thread, JSType::JS_INT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt16Array = JSHandle<JSTaggedValue>::Cast(handleInt16Array);
    EXPECT_TRUE(handleTagValInt16Array->IsJSInt16Array() && handleTagValInt16Array->IsTypedArray());

    JSHandle<JSTypedArray> handleUint16Array = CreateNumberTypedArray(thread, JSType::JS_UINT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint16Array = JSHandle<JSTaggedValue>::Cast(handleUint16Array);
    EXPECT_TRUE(handleTagValUint16Array->IsJSUint16Array() && handleTagValUint16Array->IsTypedArray());

    JSHandle<JSTypedArray> handleInt32Array = CreateNumberTypedArray(thread, JSType::JS_INT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt32Array = JSHandle<JSTaggedValue>::Cast(handleInt32Array);
    EXPECT_TRUE(handleTagValInt32Array->IsJSInt32Array() && handleTagValInt32Array->IsTypedArray());

    JSHandle<JSTypedArray> handleUint32Array = CreateNumberTypedArray(thread, JSType::JS_UINT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint32Array = JSHandle<JSTaggedValue>::Cast(handleUint32Array);
    EXPECT_TRUE(handleTagValUint32Array->IsJSUint32Array() && handleTagValUint32Array->IsTypedArray());

    JSHandle<JSTypedArray> handleFloat32Array = CreateNumberTypedArray(thread, JSType::JS_FLOAT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValFloat32Array = JSHandle<JSTaggedValue>::Cast(handleFloat32Array);
    EXPECT_TRUE(handleTagValFloat32Array->IsJSFloat32Array() && handleTagValFloat32Array->IsTypedArray());

    JSHandle<JSTypedArray> handleFloat64Array = CreateNumberTypedArray(thread, JSType::JS_FLOAT64_ARRAY);
    JSHandle<JSTaggedValue> handleTagValFloat64Array = JSHandle<JSTaggedValue>::Cast(handleFloat64Array);
    EXPECT_TRUE(handleTagValFloat64Array->IsJSFloat64Array() && handleTagValFloat64Array->IsTypedArray());
}

/*
 * Feature: JSTypedArray
 * Function: SetViewedArrayBuffer
 * SubFunction: GetViewedArrayBuffer/ObjectFactory::NewJSArrayBuffer
 * FunctionPoints: Set ViewedArrayBuffer
 * CaseDescription: Check whether the JSArrayBuffer returned through calling GetViewedArrayBuffer function from the
 *                  JSTypedArray changed through calling SetViewedArrayBuffer function is within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, SetViewedArrayBuffer)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> handleArrayBufferFrom = factory->NewJSArrayBuffer(10);
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(handleArrayBufferFrom);

    for (size_t i = 0; i < cVecJSType.size(); i++) {
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(i));

        EXPECT_EQ(handleTypedArray->GetViewedArrayBuffer(), JSTaggedValue::Undefined());
        handleTypedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
        EXPECT_EQ(handleTypedArray->GetViewedArrayBuffer(), handleTagValArrayBufferFrom.GetTaggedValue());
    }
}

/*
 * Feature: JSTypedArray
 * Function: SetTypedArrayName
 * SubFunction: GetTypedArrayName
 * FunctionPoints: Set TypedArrayName
 * CaseDescription: Check whether the JSTaggedValue returned through calling GetTypedArrayName function from the
 *                  JSTypedArray changed through calling SetTypedArrayName function is within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, SetTypedArrayName)
{
    CString cStrName = "cStrName";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> handleEcmaStrNameFrom = factory->NewFromASCII(cStrName);
    JSHandle<JSTaggedValue> handleTagValEcmaStrNameFrom = JSHandle<JSTaggedValue>::Cast(handleEcmaStrNameFrom);

    for (size_t i = 0; i < cVecJSType.size(); i++) {
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(i));

        EXPECT_EQ(handleTypedArray->GetTypedArrayName(), JSTaggedValue::Undefined());
        handleTypedArray->SetTypedArrayName(thread, handleTagValEcmaStrNameFrom);
        EXPECT_EQ(handleTypedArray->GetTypedArrayName(), handleTagValEcmaStrNameFrom.GetTaggedValue());
    }
}

/*
 * Feature: JSTypedArray
 * Function: SetByteLength
 * SubFunction: GetByteLength
 * FunctionPoints: Set ByteLength
 * CaseDescription: Check whether the Number returned through calling GetByteLength function from the JSTypedArray
 *                  changed through calling SetByteLength function is within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, SetByteLength)
{
    uint32_t u32ByteLength = 2;
    JSHandle<JSTaggedValue> handleTagValByteLengthFrom(thread, JSTaggedValue(u32ByteLength));

    for (size_t i = 0; i < cVecJSType.size(); i++) {
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(i));

        EXPECT_EQ(handleTypedArray->GetByteLength(), JSTaggedValue(0));
        handleTypedArray->SetByteLength(thread, handleTagValByteLengthFrom);
        EXPECT_EQ(handleTypedArray->GetByteLength(), handleTagValByteLengthFrom.GetTaggedValue());
    }
}

/*
 * Feature: JSTypedArray
 * Function: SetByteOffset
 * SubFunction: GetByteOffset
 * FunctionPoints: Set ByteOffset
 * CaseDescription: Check whether the Number returned through calling GetByteOffset function from the JSTypedArray
 *                  changed through calling SetByteOffset function is within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, SetByteOffset)
{
    uint32_t u32ByteOffset = 2;
    JSHandle<JSTaggedValue> handleTagValByteOffsetFrom(thread, JSTaggedValue(u32ByteOffset));

    for (size_t i = 0; i < cVecJSType.size(); i++) {
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(i));

        EXPECT_EQ(handleTypedArray->GetByteOffset(), JSTaggedValue(0));
        handleTypedArray->SetByteOffset(thread, handleTagValByteOffsetFrom);
        EXPECT_EQ(handleTypedArray->GetByteOffset(), handleTagValByteOffsetFrom.GetTaggedValue());
    }
}

/*
 * Feature: JSTypedArray
 * Function: SetArrayLength
 * SubFunction: GetArrayLength
 * FunctionPoints: Set ArrayLength
 * CaseDescription: Check whether the Number returned through calling GetArrayLength function from the JSTypedArray
 *                  changed through calling SetArrayLength function is within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, SetArrayLength)
{
    uint32_t u32ArrayLength = 2;
    JSHandle<JSTaggedValue> handleTagValArrayLengthFrom(thread, JSTaggedValue(u32ArrayLength));

    for (size_t i = 0; i < cVecJSType.size(); i++) {
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(i));

        EXPECT_EQ(handleTypedArray->GetArrayLength(), JSTaggedValue(0));
        handleTypedArray->SetArrayLength(thread, handleTagValArrayLengthFrom);
        EXPECT_EQ(handleTypedArray->GetArrayLength(), handleTagValArrayLengthFrom.GetTaggedValue());
    }
}

/*
 * Feature: JSTypedArray
 * Function: IntegerIndexedElementSet
 * SubFunction: IntegerIndexedElementGet
 * FunctionPoints: Set Element At Integer Index(JSTaggedValue) Of JSTypedArray
 * CaseDescription: Check whether the OperationResults returned through calling IntegerIndexedElementGet function from
 *                  the JSTypedArray changed through calling IntegerIndexedElementSet function are within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int8Array_001)
{
    int numElementsInt8Array = 256;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt8Array = CreateNumberTypedArray(thread, JSType::JS_INT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt8Array = JSHandle<JSTaggedValue>::Cast(handleInt8Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt8Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt8Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt8Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt8Array->SetArrayLength(thread, JSTaggedValue(numElementsInt8Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsInt8Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(std::numeric_limits<int8_t>::min() + i))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsInt8Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(), std::numeric_limits<int8_t>::min() + i);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(numElementsInt8Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array,
        JSTaggedValue(numElementsInt8Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Int8Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int8Array_002)
{
    int numElementsInt8Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt8Array = CreateNumberTypedArray(thread, JSType::JS_INT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt8Array = JSHandle<JSTaggedValue>::Cast(handleInt8Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt8Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt8Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt8Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt8Array->SetArrayLength(thread, JSTaggedValue(numElementsInt8Array));

    int64_t value1 = -129; // to int8 : 127
    int64_t value2 = 128; // to int8 : -128
    double value3 = 13.4; // to int8 : 13
    double value4 = 13.6; // to int8 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt8Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt8Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(127, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(-128, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint8Array_001)
{
    int numElementsUint8Array = 256;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint8Array = CreateNumberTypedArray(thread, JSType::JS_UINT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8Array = JSHandle<JSTaggedValue>::Cast(handleUint8Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint8Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint8Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint8Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint8Array->SetArrayLength(thread, JSTaggedValue(numElementsUint8Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsUint8Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(std::numeric_limits<uint8_t>::min() + i))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsUint8Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(), std::numeric_limits<uint8_t>::min() + i);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(numElementsUint8Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array,
        JSTaggedValue(numElementsUint8Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Uint8Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint8Array_002)
{
    int numElementsUint8Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint8Array = CreateNumberTypedArray(thread, JSType::JS_UINT8_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8Array = JSHandle<JSTaggedValue>::Cast(handleUint8Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint8Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint8Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint8Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint8Array->SetArrayLength(thread, JSTaggedValue(numElementsUint8Array));

    int64_t value1 = -1; // to uint8 : 255
    int64_t value2 = 256; // to uint8 : 0
    double value3 = 13.4; // to uint8 : 13
    double value4 = 13.6; // to uint8 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(255, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(0, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint8ClampedArray_001)
{
    int numElementsUint8ClampedArray = 256;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint8ClampedArray = CreateNumberTypedArray(thread, JSType::JS_UINT8_CLAMPED_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8ClampedArray = JSHandle<JSTaggedValue>::Cast(handleUint8ClampedArray);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint8ClampedArray));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint8ClampedArray;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint8ClampedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint8ClampedArray->SetArrayLength(thread, JSTaggedValue(numElementsUint8ClampedArray));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsUint8ClampedArray; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(std::numeric_limits<uint8_t>::min() + i))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsUint8ClampedArray; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(), std::numeric_limits<uint8_t>::min() + i);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(numElementsUint8ClampedArray));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(numElementsUint8ClampedArray), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Uint8ClampedArray
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint8ClampedArray_002)
{
    int numElementsUint8ClampedArray = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint8ClampedArray = CreateNumberTypedArray(thread, JSType::JS_UINT8_CLAMPED_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint8ClampedArray = JSHandle<JSTaggedValue>::Cast(handleUint8ClampedArray);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint8ClampedArray));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint8ClampedArray;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint8ClampedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint8ClampedArray->SetArrayLength(thread, JSTaggedValue(numElementsUint8ClampedArray));

    int64_t value1 = -1; // to uint8_clamped : 0
    int64_t value2 = 256; // to uint8_clamped : 255
    double value3 = 13.4; // to uint8_clamped : 13
    double value4 = 13.6; // to uint8_clamped : 14
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint8ClampedArray, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint8ClampedArray,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(0, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(255, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(14, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int16Array_001)
{
    int numElementsInt16Array = 100;
    int16_t scaleForInt16ValueSet = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt16Array = CreateNumberTypedArray(thread, JSType::JS_INT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt16Array = JSHandle<JSTaggedValue>::Cast(handleInt16Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt16Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt16Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt16Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt16Array->SetArrayLength(thread, JSTaggedValue(numElementsInt16Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsInt16Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(std::numeric_limits<int16_t>::min() + i * scaleForInt16ValueSet))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsInt16Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            std::numeric_limits<int16_t>::min() + i * scaleForInt16ValueSet);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(numElementsInt16Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array,
        JSTaggedValue(numElementsInt16Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Int16Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int16Array_002)
{
    int numElementsInt16Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt16Array = CreateNumberTypedArray(thread, JSType::JS_INT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt16Array = JSHandle<JSTaggedValue>::Cast(handleInt16Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt16Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt16Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt16Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt16Array->SetArrayLength(thread, JSTaggedValue(numElementsInt16Array));

    int64_t value1 = -32769; // to int16 : 32767
    int64_t value2 = 32768; // to int16 : -32768
    double value3 = 13.4; // to int16 : 13
    double value4 = 13.6; // to int16 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt16Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt16Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(32767, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(-32768, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint16Array_001)
{
    int numElementsUint16Array = 100;
    uint32_t scaleForUint16ValueSet = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint16Array = CreateNumberTypedArray(thread, JSType::JS_UINT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint16Array = JSHandle<JSTaggedValue>::Cast(handleUint16Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint16Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint16Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint16Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint16Array->SetArrayLength(thread, JSTaggedValue(numElementsUint16Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsUint16Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(std::numeric_limits<uint16_t>::min() + i * scaleForUint16ValueSet))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsUint16Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            std::numeric_limits<uint16_t>::min() + i * scaleForUint16ValueSet);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(numElementsUint16Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array,
        JSTaggedValue(numElementsUint16Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Uint16Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint16Array_002)
{
    int numElementsUint16Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint16Array = CreateNumberTypedArray(thread, JSType::JS_UINT16_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint16Array = JSHandle<JSTaggedValue>::Cast(handleUint16Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint16Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint16Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint16Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint16Array->SetArrayLength(thread, JSTaggedValue(numElementsUint16Array));

    int64_t value1 = -1; // to uint16 : 65535
    int64_t value2 = 65536; // to uint16 : 0
    double value3 = 13.4; // to uint16 : 13
    double value4 = 13.6; // to uint16 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint16Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint16Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(65535, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(0, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int32Array_001)
{
    int numElementsInt32Array = 100;
    int32_t scaleForInt32ValueSet = 100000;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt32Array = CreateNumberTypedArray(thread, JSType::JS_INT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt32Array = JSHandle<JSTaggedValue>::Cast(handleInt32Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt32Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt32Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt32Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt32Array->SetArrayLength(thread, JSTaggedValue(numElementsInt32Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsInt32Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(std::numeric_limits<int32_t>::min() + i * scaleForInt32ValueSet))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsInt32Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            std::numeric_limits<int32_t>::min() + i * scaleForInt32ValueSet);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(numElementsInt32Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array,
        JSTaggedValue(numElementsInt32Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Int32Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Int32Array_002)
{
    int numElementsInt32Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleInt32Array = CreateNumberTypedArray(thread, JSType::JS_INT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValInt32Array = JSHandle<JSTaggedValue>::Cast(handleInt32Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleInt32Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsInt32Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleInt32Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleInt32Array->SetArrayLength(thread, JSTaggedValue(numElementsInt32Array));

    int64_t value1 = -2147483649; // to int32 : 2147483647
    int64_t value2 = 2147483648; // to int32 : -2147483648
    double value3 = 13.4; // to int32 : 13
    double value4 = 13.6; // to int32 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValInt32Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValInt32Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(2147483647, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(-2147483648, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint32Array_001)
{
    int numElementsUint32Array = 100;
    uint32_t scaleForUint32ValueSet = 100000;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint32Array = CreateNumberTypedArray(thread, JSType::JS_UINT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint32Array = JSHandle<JSTaggedValue>::Cast(handleUint32Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint32Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint32Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint32Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint32Array->SetArrayLength(thread, JSTaggedValue(numElementsUint32Array));

    CVector<OperationResult> cVecOpResult = {};
    for (int i = 0; i < numElementsUint32Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(std::numeric_limits<uint32_t>::min() + i * scaleForUint32ValueSet))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsUint32Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            std::numeric_limits<uint32_t>::min() + i * scaleForUint32ValueSet);
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(numElementsUint32Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array,
        JSTaggedValue(numElementsUint32Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

// Nonstandard input value for Uint32Array
HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Uint32Array_002)
{
    int32_t numElementsUint32Array = 16;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleUint32Array = CreateNumberTypedArray(thread, JSType::JS_UINT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValUint32Array = JSHandle<JSTaggedValue>::Cast(handleUint32Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleUint32Array));
    int32_t byteLengthViewdArrayBuffer = sizeElement * numElementsUint32Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleUint32Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleUint32Array->SetArrayLength(thread, JSTaggedValue(numElementsUint32Array));

    int64_t value1 = -1; // to uint32 : 4294967295
    int64_t value2 = 4294967296; // to uint32 : 0
    double value3 = 13.4; // to uint32 : 13
    double value4 = 13.6; // to uint32 : 13
    JSHandle<JSTaggedValue> handleTagValValueSet1(thread, JSTaggedValue(value1));
    JSHandle<JSTaggedValue> handleTagValValueSet2(thread, JSTaggedValue(value2));
    JSHandle<JSTaggedValue> handleTagValValueSet3(thread, JSTaggedValue(value3));
    JSHandle<JSTaggedValue> handleTagValValueSet4(thread, JSTaggedValue(value4));

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(0),
        handleTagValValueSet1));
    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(0),
        handleTagValValueSet2));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(0),
        handleTagValValueSet3));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(0));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValUint32Array, JSTaggedValue(0),
        handleTagValValueSet4));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValUint32Array,
        JSTaggedValue(0));

    EXPECT_NE(value1, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value2, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value3, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_NE(value4, opResult4.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(4294967295, opResult1.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(0, opResult2.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult3.GetValue().GetTaggedValue().GetNumber());
    EXPECT_EQ(13, opResult4.GetValue().GetTaggedValue().GetNumber());
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Float32Array)
{
    int numElementsFloat32Array = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleFloat32Array = CreateNumberTypedArray(thread, JSType::JS_FLOAT32_ARRAY);
    JSHandle<JSTaggedValue> handleTagValFloat32Array = JSHandle<JSTaggedValue>::Cast(handleFloat32Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleFloat32Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsFloat32Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleFloat32Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleFloat32Array->SetArrayLength(thread, JSTaggedValue(numElementsFloat32Array));

    CVector<OperationResult> cVecOpResult = {};
    float floatMaxValue = std::numeric_limits<float>::max();
    for (int i = 0; i < numElementsFloat32Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat32Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread,
                JSTaggedValue(floatMaxValue - (i * (floatMaxValue / numElementsFloat32Array))))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat32Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsFloat32Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            floatMaxValue - (i * (floatMaxValue / numElementsFloat32Array)));
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat32Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat32Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat32Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat32Array,
        JSTaggedValue(numElementsFloat32Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat32Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat32Array,
        JSTaggedValue(numElementsFloat32Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

HWTEST_F_L0(JSTypedArrayTest, IntegerIndexedElementSet_Float64Array)
{
    int numElementsFloat64Array = 100;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTypedArray> handleFloat64Array = CreateNumberTypedArray(thread, JSType::JS_FLOAT64_ARRAY);
    JSHandle<JSTaggedValue> handleTagValFloat64Array = JSHandle<JSTaggedValue>::Cast(handleFloat64Array);

    uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
        JSHandle<JSObject>::Cast(handleFloat64Array));
    uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsFloat64Array;
    JSHandle<JSTaggedValue> handleTagValArrayBufferFrom = JSHandle<JSTaggedValue>::Cast(
        factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
    handleFloat64Array->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
    handleFloat64Array->SetArrayLength(thread, JSTaggedValue(numElementsFloat64Array));

    CVector<OperationResult> cVecOpResult = {};
    double doubleMaxValue = std::numeric_limits<double>::max();
    for (int i = 0; i < numElementsFloat64Array; i++) {
        EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat64Array, JSTaggedValue(i),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(
                doubleMaxValue - (i * (doubleMaxValue / numElementsFloat64Array))))));
        OperationResult opResult = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat64Array,
            JSTaggedValue(i));
        cVecOpResult.push_back(opResult);
    }
    for (int i = 0; i < numElementsFloat64Array; i++) {
        EXPECT_EQ(cVecOpResult.at(i).GetValue().GetTaggedValue().GetNumber(),
            doubleMaxValue - (i * (doubleMaxValue / numElementsFloat64Array)));
    }
    cVecOpResult.clear();

    OperationResult opResult1 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat64Array,
        JSTaggedValue(-1));
    OperationResult opResult2 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat64Array,
        JSTaggedValue(-0.0));
    OperationResult opResult3 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat64Array,
        JSTaggedValue(1.1));
    OperationResult opResult4 = JSTypedArray::IntegerIndexedElementGet(thread, handleTagValFloat64Array,
        JSTaggedValue(numElementsFloat64Array));
    EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult3.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(opResult4.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());

    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat64Array, JSTaggedValue(-1),
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValFloat64Array,
        JSTaggedValue(numElementsFloat64Array), JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
}

/*
 * Feature: JSTypedArray
 * Function: FastElementGet
 * SubFunction: IntegerIndexedElementSet
 * FunctionPoints: Get Element At Index(uint32_t) Of JSTypedArray
 * CaseDescription: Check whether the OperationResults returned through calling FastElementGet function from the
 *                  JSTypedArray changed through calling IntegerIndexedElementSet function are within expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, FastElementGet_TypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (uint32_t j = 0; j < cVecJSType.size(); j++) {
        int numElementsTypedArray = 10;
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(j));
        JSHandle<JSTaggedValue> handleTagValTypedArray = JSHandle<JSTaggedValue>::Cast(handleTypedArray);

        uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
            JSHandle<JSObject>::Cast(handleTypedArray));
        uint32_t byteLengthViewdArrayBuffer = sizeElement * numElementsTypedArray;
        JSHandle<JSTaggedValue> handleTagValArrayBufferFrom =
            JSHandle<JSTaggedValue>::Cast(factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
        handleTypedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
        handleTypedArray->SetArrayLength(thread, JSTaggedValue(numElementsTypedArray));

        JSHandle<JSTaggedValue> handleTagValValueSet(thread, JSTaggedValue(cVecHandleTagValValueForTypedArray.at(j)));
        for (int i = 0; i < numElementsTypedArray; i++) {
            EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValTypedArray, JSTaggedValue(i),
                handleTagValValueSet));
        }
        for (int i = 0; i < numElementsTypedArray; i++) {
            OperationResult opResult = JSTypedArray::FastElementGet(thread, handleTagValTypedArray, i);
            EXPECT_EQ(opResult.GetValue().GetTaggedValue().GetNumber(),
                      handleTagValValueSet.GetTaggedValue().GetNumber());
        }

        OperationResult opResult1 = JSTypedArray::FastElementGet(thread, handleTagValTypedArray, -1);
        OperationResult opResult2 = JSTypedArray::FastElementGet(thread, handleTagValTypedArray,
            numElementsTypedArray);
        EXPECT_EQ(opResult1.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
        EXPECT_EQ(opResult2.GetValue().GetTaggedValue(), JSTaggedValue::Undefined());
    }
}

/*
 * Feature: JSTypedArray
 * Function: DefineOwnProperty
 * SubFunction: GetOwnProperty/HasProperty
 *              PropertyDescriptor::HasWritable/HasEnumerable/HasConfigurable/IsWritable/IsEnumerable/IsConfigurable
 * FunctionPoints: Define Own Property For Element At Index(JSTaggedValue) Of JSTypedArray
 * CaseDescription: Call DefineOwnProperty function with a JSTypedArray, a index(JSTaggedValue) and a source
 *                  PropertyDescriptor, check whether the bool returned through calling HasProperty function with the
 *                  JSTypedArray and the index(JSTaggedValue) is within expectations, check whether the target
 *                  PropertyDescriptor changed through GetOwnProperty function is with expectations.
 */
HWTEST_F_L0(JSTypedArrayTest, DefineOwnProperty_TypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (size_t j = 0; j < cVecJSType.size(); j++) {
        int numElementsTypedArray = 10;
        JSHandle<JSTaggedValue> handleTagValValueDef(thread, cVecHandleTagValValueForTypedArray.at(j));
        PropertyDescriptor descFrom1(thread, handleTagValValueDef, true, true, true);
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(j));
        JSHandle<JSTaggedValue> handleTagValTypedArray = JSHandle<JSTaggedValue>::Cast(handleTypedArray);

        uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
            JSHandle<JSObject>::Cast(handleTypedArray));
        int32_t byteLengthViewdArrayBuffer = sizeElement * numElementsTypedArray;
        JSHandle<JSTaggedValue> handleTagValArrayBufferFrom =
            JSHandle<JSTaggedValue>::Cast(factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
        handleTypedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
        handleTypedArray->SetArrayLength(thread, JSTaggedValue(numElementsTypedArray));

        for (int i = 0; i < numElementsTypedArray; i++) {
            JSHandle<JSTaggedValue> handleTagValKey(thread, JSTaggedValue(i));
            EXPECT_FALSE(JSTypedArray::HasProperty(thread, handleTagValTypedArray, handleTagValKey));
            EXPECT_TRUE(JSTypedArray::DefineOwnProperty(thread, handleTagValTypedArray, handleTagValKey, descFrom1));
            EXPECT_TRUE(JSTypedArray::HasProperty(thread, handleTagValTypedArray, handleTagValKey));
            EXPECT_TRUE(JSTaggedValue::StrictEqual(thread, handleTagValValueDef,
                JSTypedArray::GetProperty(thread, handleTagValTypedArray, handleTagValKey).GetValue()));

            PropertyDescriptor descTo1(thread);
            EXPECT_FALSE(descTo1.HasWritable() || descTo1.HasEnumerable() || descTo1.HasConfigurable());
            EXPECT_TRUE(JSTypedArray::GetOwnProperty(thread, handleTagValTypedArray, handleTagValKey, descTo1));
            EXPECT_TRUE(descTo1.HasWritable() && descTo1.HasEnumerable() && descTo1.HasConfigurable());
            EXPECT_TRUE(descTo1.IsWritable() && descTo1.IsEnumerable() && descTo1.IsConfigurable());
            EXPECT_TRUE(JSTaggedValue::StrictEqual(thread, descTo1.GetValue(), handleTagValValueDef));
        }
    }
}

/*
 * Feature: JSTypedArray
 * Function: SetProperty
 * SubFunction: GetProperty/HasProperty
 * FunctionPoints: Set Property For Element At Index(JSTaggedValue) Of JSTypedArray
 * CaseDescription: Call SetProperty function with a JSTypedArray, a index(JSTaggedValue) and a source
 *                  value(JSTaggedValue), check whether the bool returned through calling HasProperty function with the
 *                  JSTypedArray and the index(JSTaggedValue) is within expectations, check whether the
 *                  value(JSTaggedValue) of the OperationResult returned through calling GetProperty function with the
 *                  JSTypedArray and the index(JSTaggedValue) is the same with the source value(JSTaggedValue).
 */
HWTEST_F_L0(JSTypedArrayTest, SetProperty_TypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (size_t j = 0; j < cVecJSType.size(); j++) {
        int numElementsTypedArray = 10;
        JSHandle<JSTaggedValue> handleTagValValueSet(thread, cVecHandleTagValValueForTypedArray.at(j));
        JSHandle<JSTypedArray> handleTypedArray = CreateNumberTypedArray(thread, cVecJSType.at(j));
        JSHandle<JSTaggedValue> handleTagValTypedArray = JSHandle<JSTaggedValue>::Cast(handleTypedArray);

        uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
            JSHandle<JSObject>::Cast(handleTypedArray));
        int32_t byteLengthViewdArrayBuffer = sizeElement * numElementsTypedArray;
        JSHandle<JSTaggedValue> handleTagValArrayBufferFrom =
            JSHandle<JSTaggedValue>::Cast(factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
        handleTypedArray->SetViewedArrayBuffer(thread, handleTagValArrayBufferFrom);
        handleTypedArray->SetArrayLength(thread, JSTaggedValue(numElementsTypedArray));

        for (int i = 0; i < numElementsTypedArray; i++) {
            JSHandle<JSTaggedValue> handleTagValKey(thread, JSTaggedValue(i));
            EXPECT_FALSE(JSTypedArray::HasProperty(thread, handleTagValTypedArray, handleTagValKey));
            EXPECT_TRUE(JSTypedArray::SetProperty(thread, handleTagValTypedArray, handleTagValKey,
                handleTagValValueSet));
            EXPECT_TRUE(JSTypedArray::HasProperty(thread, handleTagValTypedArray, handleTagValKey));
            EXPECT_TRUE(JSTaggedValue::StrictEqual(thread, handleTagValValueSet,
                JSTypedArray::GetProperty(thread, handleTagValTypedArray, handleTagValKey).GetValue()));
        }
    }
}

/*
 * Feature: JSTypedArray
 * Function: FastCopyElementToArray
 * SubFunction: IntegerIndexedElementSet/TaggedArray::Get
 * FunctionPoints: Copy All Elements Of JSTypedArray To TaggedArray Fast
 * CaseDescription: Create a source JSTypedArray and a target TaggedArray, init the elements of the source JSTypedArray
 *                  with a certain value(JSTaggedValue) through calling IntegerIndexedElementSet function. Call
 *                  FastCopyElementToArray function with the source JSTypedArray and the target TaggedArray. Check
 *                  whether the values(JSTaggedValue) returned through Get(TaggedArray) function are the same with the
 *                  certain value(JSTaggedValue).
 */
HWTEST_F_L0(JSTypedArrayTest, FastCopyElementToArray_TypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (size_t j = 0; j < cVecJSType.size(); j++) {
        int numElementsTypedArray = 10;
        JSHandle<JSTaggedValue> handleTagValValueSet(thread, cVecHandleTagValValueForTypedArray.at(j));
        JSHandle<JSTypedArray> handleTypedArrayFrom =
            CreateNumberTypedArray(thread, cVecJSType.at(j));
        JSHandle<JSTaggedValue> handleTagValTypedArrayFrom = JSHandle<JSTaggedValue>::Cast(handleTypedArrayFrom);

        uint32_t sizeElement = ecmascript::base::TypedArrayHelper::GetElementSize(
            JSHandle<JSObject>::Cast(handleTypedArrayFrom));
        int32_t byteLengthViewdArrayBuffer = sizeElement * numElementsTypedArray;
        JSHandle<JSTaggedValue> handleTagValArrayBuffer =
            JSHandle<JSTaggedValue>::Cast(factory->NewJSArrayBuffer(byteLengthViewdArrayBuffer));
        JSHandle<TaggedArray> handleTagArrTo = factory->NewTaggedArray(byteLengthViewdArrayBuffer);
        handleTypedArrayFrom->SetViewedArrayBuffer(thread, handleTagValArrayBuffer);
        handleTypedArrayFrom->SetArrayLength(thread, JSTaggedValue(numElementsTypedArray));

        for (int i = 0; i < numElementsTypedArray; i++) {
            EXPECT_TRUE(JSTypedArray::IntegerIndexedElementSet(thread, handleTagValTypedArrayFrom, JSTaggedValue(i),
                handleTagValValueSet));
        }
        EXPECT_TRUE(JSTypedArray::FastCopyElementToArray(thread, handleTagValTypedArrayFrom, handleTagArrTo));
        for (int i = 0; i < numElementsTypedArray; i++) {
            EXPECT_EQ(handleTagArrTo->Get(i), handleTagValValueSet.GetTaggedValue());
        }
    }
}
}  // namespace panda::test
