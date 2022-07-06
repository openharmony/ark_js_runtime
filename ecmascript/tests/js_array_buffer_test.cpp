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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JsArrayBufferTest : public testing::Test {
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
 * @tc.name: CopyDataBlockBytes
 * @tc.desc: Copies the specified array buffer data block to a memory space with the specified size.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsArrayBufferTest, CopyDataBlockBytes)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    uint8_t value = 100;
    size_t length = 5;
    void *toBuffer = vm->GetNativeAreaAllocator()->AllocateBuffer(length);
    JSHandle<JSNativePointer> toNativePointer = factory->NewJSNativePointer(toBuffer, nullptr, nullptr);
    uint8_t *data = static_cast<uint8_t *>(vm->GetNativeAreaAllocator()->AllocateBuffer(length));
    if (memset_s(data, length, value, length) != EOK) {
        UNREACHABLE();
    }
    void *formBuffer = vm->GetNativeAreaAllocator()->AllocateBuffer(length);
    JSHandle<JSNativePointer> fromNativePointer =
        factory->NewJSNativePointer(formBuffer, nullptr, reinterpret_cast<void *>(data));
    int32_t fromIndex = 0;
    int32_t count = 5;
    JSArrayBuffer::CopyDataBlockBytes(JSHandle<JSTaggedValue>::Cast(toNativePointer).GetTaggedValue(),
        JSHandle<JSTaggedValue>::Cast(fromNativePointer).GetTaggedValue(), fromIndex, count);
    auto toData = reinterpret_cast<uint8_t *>(toNativePointer->GetExternalPointer());
    auto fromData = reinterpret_cast<uint8_t *>(fromNativePointer->GetExternalPointer());
    for (uint32_t i = 0; i < length; i++) {
        EXPECT_EQ(*(toData + i), *(fromData + i));
    }
}

/**
 * @tc.name: Attach & Detach & IsDetach
 * @tc.desc: Attach the specified array buffer to the specified data, then detach the data, and verify the detach
 *           result using the isdetach method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsArrayBufferTest, Attach_Detach_IsDetach)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();

    size_t length = 5;
    uint8_t value = 100;
    void *buffer = vm->GetNativeAreaAllocator()->AllocateBuffer(100);
    uint8_t *data = static_cast<uint8_t *>(vm->GetNativeAreaAllocator()->AllocateBuffer(length));
    if (memset_s(data, length, value, length) != EOK) {
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> nativePointer =
        factory->NewJSNativePointer(buffer, nullptr, reinterpret_cast<void *>(data));
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(5);
    arrBuf->Attach(thread, length + 1, JSHandle<JSTaggedValue>::Cast(nativePointer).GetTaggedValue());
    EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 6U);
    EXPECT_EQ(arrBuf->GetArrayBufferData().GetRawData(),
        JSHandle<JSTaggedValue>::Cast(nativePointer).GetTaggedValue().GetRawData());

    arrBuf->Detach(thread);
    EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0U);
    EXPECT_EQ(arrBuf->GetArrayBufferData().GetRawData(), JSTaggedValue::Null().GetRawData());
    EXPECT_TRUE(arrBuf->IsDetach());
}
} // namespace panda::test
