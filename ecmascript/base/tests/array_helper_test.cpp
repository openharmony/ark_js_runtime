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

#include "ecmascript/base/array_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class ArrayHelperTest : public testing::Test {
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

/**
 * @tc.name: IsConcatSpreadable
 * @tc.desc: Check whether the second parameter is a JsArray type through "IsConcatSpreadable" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ArrayHelperTest, IsConcatSpreadable)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> icConcatSpreadableSymbol(factory->NewWellKnownSymbolWithChar("icConcatSpreadableSymbol"));
    JSHandle<JSTaggedValue> icSymbolValue(thread, JSTaggedValue(1));
    globalEnv->SetIsConcatSpreadableSymbol(thread, icConcatSpreadableSymbol);
    JSHandle<JSTaggedValue> objFunc(globalEnv->GetArrayFunction());

    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleArray(factory->NewJSArray());
    JSHandle<JSTaggedValue> handleObjectArr(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc));
    JSObject::SetProperty(thread, handleObjectArr, icConcatSpreadableSymbol, icSymbolValue);

    EXPECT_FALSE(ArrayHelper::IsConcatSpreadable(thread, handleValue));
    EXPECT_TRUE(ArrayHelper::IsConcatSpreadable(thread, handleArray));
    EXPECT_TRUE(ArrayHelper::IsConcatSpreadable(thread, handleObjectArr));
}

/**
 * @tc.name: SortCompare
 * @tc.desc: Check whether the two data(X,Y) are sorted from large two smalle,both X and Y are Undefined return zero,if
 *           X or Y is Undefined return -1,if X more than the Y return 1,otherwrise return 0.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ArrayHelperTest, SortCompare)
{
    JSHandle<JSTaggedValue> callbackfnHandle(thread, JSTaggedValue::Undefined());
    // callbackfnHandle is Undefined
    JSHandle<JSTaggedValue> handleValueX1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleValueX2(thread, JSTaggedValue(11));
    JSHandle<JSTaggedValue> handleValueX3(thread, JSTaggedValue(12));
    JSHandle<JSTaggedValue> handleValueY1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleValueY2(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> handleValueY3(thread, JSTaggedValue(12));

    int32_t resultValue1 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX1, handleValueY1);
    int32_t resultValue2 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX1, handleValueY2);
    int32_t resultValue3 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX2, handleValueY1);
    int32_t resultValue4 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX2, handleValueY2);
    int32_t resultValue5 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX3, handleValueY3);
    int32_t resultValue6 = ArrayHelper::SortCompare(thread, callbackfnHandle, handleValueX2, handleValueY3);

    EXPECT_EQ(resultValue1, 0); // both X and Y is Undefined
    EXPECT_EQ(resultValue2, 1); // X is Undefined
    EXPECT_EQ(resultValue3, -1); // Y is Undefined
    EXPECT_EQ(resultValue4, 1);  // X > Y
    EXPECT_EQ(resultValue5, 0); // X = Y
    EXPECT_EQ(resultValue6, 0); // X < Y
}

/**
 * @tc.name: GetLength
 * @tc.desc: Check whether the result returned through "GetLength" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ArrayHelperTest, GetLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lengthValue(thread, JSTaggedValue(100.0));

    JSArray *handleArr = JSArray::ArrayCreate(thread, JSTaggedNumber(10)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> arrayHandle(thread, handleArr);
    EXPECT_EQ(ArrayHelper::GetLength(thread, arrayHandle), 10U);

    JSHandle<JSTaggedValue> HandleInt8ArrayFunc(globalEnv->GetInt8ArrayFunction());
    JSHandle<JSTypedArray> handleTypeArray = JSHandle<JSTypedArray>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(HandleInt8ArrayFunc), HandleInt8ArrayFunc));
    handleTypeArray->SetArrayLength(11);
    JSHandle<JSTaggedValue> typeArrayHandle(handleTypeArray);
    EXPECT_EQ(ArrayHelper::GetLength(thread, typeArrayHandle), 11U);

    JSHandle<JSTaggedValue> objFunc(globalEnv->GetArrayFunction());
    JSHandle<JSObject> objectHandle = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    EXPECT_EQ(ArrayHelper::GetLength(thread, JSHandle<JSTaggedValue>(objectHandle)), 0U);

    JSObject::SetProperty(thread, objectHandle, lengthKey, lengthValue);
    EXPECT_EQ(ArrayHelper::GetLength(thread, JSHandle<JSTaggedValue>(objectHandle)),
                                                                     JSTaggedNumber(100.0).GetNumber());
}

/**
 * @tc.name: GetArrayLength
 * @tc.desc: Check whether the result returned through "GetArrayLength" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(ArrayHelperTest, GetArrayLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lengthValue(thread, JSTaggedValue(10.0));

    JSArray *handleArr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> arrayHandle(thread, handleArr);
    EXPECT_EQ(ArrayHelper::GetLength(thread, arrayHandle), 0U);

    JSHandle<JSTaggedValue> objFunc(globalEnv->GetArrayFunction());
    JSHandle<JSObject> objectHandle = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    EXPECT_EQ(ArrayHelper::GetArrayLength(thread, JSHandle<JSTaggedValue>(objectHandle)), 0U);

    JSObject::SetProperty(thread, objectHandle, lengthKey, lengthValue);
    EXPECT_EQ(ArrayHelper::GetArrayLength(thread, JSHandle<JSTaggedValue>(objectHandle)),
                                                                          JSTaggedNumber(10.0).GetNumber());
}
}  // namespace panda::test
