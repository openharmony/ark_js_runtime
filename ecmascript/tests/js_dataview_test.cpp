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

#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSDataViewTest : public testing::Test {
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

/*
 * Feature: JSDataView
 * Function: GetElementSize
 * SubFunction: N/A
 * FunctionPoints: Get ElementSize
 * CaseDescription: Check whether the returned value through "GetElementSize" function is within expectations.
 */
HWTEST_F_L0(JSDataViewTest, GetElementSize)
{
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::INT8), 1U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::UINT8), 1U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::UINT8_CLAMPED), 1U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::INT16), 2U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::UINT16), 2U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::INT32), 4U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::UINT32), 4U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::FLOAT32), 4U);
    EXPECT_EQ(JSDataView::GetElementSize(DataViewType::FLOAT64), 8U);
}

/*
 * Feature: JSDataView
 * Function: SetDataView
 * SubFunction: GetDataView
 * FunctionPoints: Set DataView
 * CaseDescription: Check whether the returned value through "GetDataView" function is within expectations after
 *                  calling "SetDataView" function.
 */
HWTEST_F_L0(JSDataViewTest, SetDataView)
{
    EcmaVM *ecmaVMPtr = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVMPtr->GetFactory();
    JSHandle<GlobalEnv> handleGlobalEnv = ecmaVMPtr->GetGlobalEnv();

    uint32_t lengthDataArrayBuf = 8;
    uint32_t offsetDataView = 4;
    uint32_t lengthDataView = 4;
    JSHandle<JSFunction> handleFuncArrayBuf(handleGlobalEnv->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> handleTagValFuncArrayBuf(handleFuncArrayBuf);
    JSHandle<JSArrayBuffer> handleArrayBuf(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf, handleTagValFuncArrayBuf));
    handleArrayBuf->SetArrayBufferByteLength(lengthDataArrayBuf);

    // Call "SetDataView" function through "NewJSDataView" function of "object_factory.cpp"
    JSHandle<JSDataView> handleDataView = factory->NewJSDataView(handleArrayBuf, offsetDataView,
        lengthDataView);
    EXPECT_TRUE(handleDataView->GetDataView().IsTrue());

    // Call "SetDataView" function in this HWTEST_F_L0.
    handleDataView->SetDataView(thread, JSTaggedValue::False());
    EXPECT_TRUE(handleDataView->GetDataView().IsFalse());
    handleDataView->SetDataView(thread, JSTaggedValue::True());
    EXPECT_TRUE(handleDataView->GetDataView().IsTrue());
}

/*
 * Feature: JSDataView
 * Function: SetViewedArrayBuffer
 * SubFunction: GetViewedArrayBuffer
 * FunctionPoints: Set ViewedArrayBuffer
 * CaseDescription: Check whether the returned value through "GetViewedArrayBuffer" function is within expectations
 *                  after calling "SetViewedArrayBuffer" function.
 */
HWTEST_F_L0(JSDataViewTest, SetViewedArrayBuffer)
{
    EcmaVM *ecmaVMPtr = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVMPtr->GetFactory();
    JSHandle<JSFunction> handleFuncArrayBuf(ecmaVMPtr->GetGlobalEnv()->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> handleTagValFuncArrayBuf(handleFuncArrayBuf);

    uint32_t lengthDataArrayBuf1 = 8;
    uint32_t lengthDataArrayBuf2 = 16;
    uint32_t offsetDataView = 4;
    uint32_t lengthDataView = 4;
    JSHandle<JSArrayBuffer> handleArrayBuf1(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf, handleTagValFuncArrayBuf));
    JSHandle<JSArrayBuffer> handleArrayBuf2(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf, handleTagValFuncArrayBuf));
    handleArrayBuf1->SetArrayBufferByteLength(lengthDataArrayBuf1);
    handleArrayBuf2->SetArrayBufferByteLength(lengthDataArrayBuf2);

    // Call "SetViewedArrayBuffer" function through "NewJSDataView" function of "object_factory.cpp"
    JSHandle<JSDataView> handleDataView = factory->NewJSDataView(handleArrayBuf1, offsetDataView,
        lengthDataView);
    JSHandle<JSTaggedValue> handleTagValDataViewFrom1(thread, handleArrayBuf1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValDataViewTo1(thread, handleDataView->GetViewedArrayBuffer());
    EXPECT_TRUE(JSTaggedValue::Equal(thread, handleTagValDataViewFrom1, handleTagValDataViewTo1));

    // Call "SetViewedArrayBuffer" function in this HWTEST_F_L0.
    handleDataView->SetViewedArrayBuffer(thread, handleArrayBuf2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValDataViewFrom2(thread, handleArrayBuf2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleTagValDataViewTo2(thread, handleDataView->GetViewedArrayBuffer());
    EXPECT_TRUE(JSTaggedValue::Equal(thread, handleTagValDataViewFrom2, handleTagValDataViewTo2));
    EXPECT_FALSE(JSTaggedValue::Equal(thread, handleTagValDataViewFrom1, handleTagValDataViewFrom2));
}

/*
 * Feature: JSDataView
 * Function: SetByteLength
 * SubFunction: GetByteLength
 * FunctionPoints: Set ByteLength
 * CaseDescription: Check whether the returned value through "GetByteLength" function is within expectations after
 *                  calling "SetByteLength" function.
 */
HWTEST_F_L0(JSDataViewTest, SetByteLength)
{
    EcmaVM *ecmaVMPtr = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVMPtr->GetFactory();
    JSHandle<JSFunction> handleFuncArrayBuf(ecmaVMPtr->GetGlobalEnv()->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> handleTagValFuncArrayBuf(handleFuncArrayBuf);

    uint32_t lengthDataArrayBuf = 8;
    uint32_t offsetDataView = 4;
    uint32_t lengthDataView1 = 4;
    uint32_t lengthDataView2 = 2;
    JSHandle<JSArrayBuffer> handleArrayBuf(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf, handleTagValFuncArrayBuf));
    handleArrayBuf->SetArrayBufferByteLength(lengthDataArrayBuf);

    // Call "SetByteLength" function through "NewJSDataView" function of "object_factory.cpp"
    JSHandle<JSDataView> handleDataView = factory->NewJSDataView(handleArrayBuf, offsetDataView,
        lengthDataView1);
    EXPECT_EQ(handleDataView->GetByteLength(), lengthDataView1);

    // Call "SetByteLength" function in this HWTEST_F_L0.
    handleDataView->SetByteLength(lengthDataView2);
    EXPECT_EQ(handleDataView->GetByteLength(), lengthDataView2);
}

/*
 * Feature: JSDataView
 * Function: SetByteOffset
 * SubFunction: GetByteOffset
 * FunctionPoints: Set ByteOffset
 * CaseDescription: Check whether the returned value through "GetByteOffset" function is within expectations after
 *                  calling "SetByteOffset" function.
 */
HWTEST_F_L0(JSDataViewTest, SetByteOffset)
{
    EcmaVM *ecmaVMPtr = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVMPtr->GetFactory();
    JSHandle<JSFunction> handleFuncArrayBuf1(ecmaVMPtr->GetGlobalEnv()->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> handleTagValFuncArrayBuf1(handleFuncArrayBuf1);

    uint32_t lengthDataArrayBuf = 8;
    uint32_t offsetDataView1 = 4;
    uint32_t offsetDataView2 = 6;
    uint32_t lengthDataView = 2;
    JSHandle<JSArrayBuffer> handleArrayBuf(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf1, handleTagValFuncArrayBuf1));
    handleArrayBuf->SetArrayBufferByteLength(lengthDataArrayBuf);

    // Call "SetByteOffset" function through "NewJSDataView" function of "object_factory.cpp"
    JSHandle<JSDataView> handleDataView = factory->NewJSDataView(handleArrayBuf, offsetDataView1,
        lengthDataView);
    EXPECT_EQ(handleDataView->GetByteOffset(), offsetDataView1);

    // Call "SetByteOffset" function in this HWTEST_F_L0.
    handleDataView->SetByteOffset(offsetDataView2);
    EXPECT_EQ(handleDataView->GetByteOffset(), offsetDataView2);
}
}  // namespace panda::test
