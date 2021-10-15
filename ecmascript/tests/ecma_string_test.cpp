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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class EcmaStringTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TesrDownTestCase()
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(EcmaStringTest, SetCompressedStringsEnabled)
{
    EXPECT_TRUE(EcmaString::GetCompressedStringsEnabled());
    EcmaString::SetCompressedStringsEnabled(false);
    EXPECT_FALSE(EcmaString::GetCompressedStringsEnabled());
    EcmaString::SetCompressedStringsEnabled(true);
    EXPECT_TRUE(EcmaString::GetCompressedStringsEnabled());
}

HWTEST_F_L0(EcmaStringTest, CanBeCompressed)
{
    uint8_t arrayU8[7] = {12, 34, 77, 127, 99, 1};
    uint16_t arrayU16Comp[] = {1, 4, 37, 91, 127, 1};
    uint16_t arrayU16NotComp[] = {72, 43, 337, 961, 1317, 65535};
    EXPECT_TRUE(EcmaString::CanBeCompressed(arrayU8));
    EXPECT_TRUE(EcmaString::CanBeCompressed(arrayU16Comp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
    EXPECT_FALSE(EcmaString::CanBeCompressed(arrayU16NotComp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));

    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EXPECT_FALSE(EcmaString::CanBeCompressed(arrayU16NotComp, sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0])));
    /* Set compressedStringsEnabled default, because it is a static boolean that some other functions rely on.The foll-
     * owing HWTEST_F_L0 will come to an unexpected result if we do not set it default in the end of this HWTEST_F_L0.
     */
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, CreateEmptyString)
{
    EcmaVM *ecmaVMPtr = EcmaVM::Cast(instance);

    EcmaString *ecmaStrEmptyPtr = EcmaString::CreateEmptyString(ecmaVMPtr);
    EXPECT_EQ(ecmaStrEmptyPtr->GetLength(), 0);
    EXPECT_TRUE(ecmaStrEmptyPtr->IsUtf8());
    EXPECT_FALSE(ecmaStrEmptyPtr->IsUtf16());

    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrEmptyDisableCompPtr = EcmaString::CreateEmptyString(ecmaVMPtr);
    EXPECT_EQ(ecmaStrEmptyDisableCompPtr->GetLength(), 0);
    EXPECT_TRUE(ecmaStrEmptyDisableCompPtr->IsUtf16());
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, AllocStringObject)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // AllocStringObject( , true, ).
    size_t sizeAllocComp = 5;
    EcmaString *ecmaStrAllocCompPtr = EcmaString::AllocStringObject(sizeAllocComp, true, ecmaVMPtr);
    for (int i = 0; i < sizeAllocComp; i++) {
        EXPECT_EQ(ecmaStrAllocCompPtr->At(i), 0);
    }
    EXPECT_EQ(ecmaStrAllocCompPtr->GetLength(), sizeAllocComp);
    EXPECT_TRUE(ecmaStrAllocCompPtr->IsUtf8());
    EXPECT_FALSE(ecmaStrAllocCompPtr->IsUtf16());

    // AllocStringObject( , false, ).
    size_t sizeAllocNotComp = 5;
    EcmaString *ecmaStrAllocNotCompPtr = EcmaString::AllocStringObject(sizeAllocNotComp, false, ecmaVMPtr);
    for (int i = 0; i < sizeAllocNotComp; i++) {
        EXPECT_EQ(ecmaStrAllocNotCompPtr->At(i), 0);
    }
    EXPECT_EQ(ecmaStrAllocNotCompPtr->GetLength(), sizeAllocNotComp);
    EXPECT_FALSE(ecmaStrAllocNotCompPtr->IsUtf8());
    EXPECT_TRUE(ecmaStrAllocNotCompPtr->IsUtf16());
    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrAllocNotCompDisableCompPtr = EcmaString::AllocStringObject(sizeAllocNotComp, false, ecmaVMPtr);
    EXPECT_TRUE(ecmaStrAllocNotCompDisableCompPtr->IsUtf16());
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, CreateFromUtf8)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);
    uint8_t arrayU8[] = {"xyz123!@#"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    for (int i = 0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(arrayU8[i], ecmaStrU8Ptr->At(i));
    }
    EXPECT_EQ(ecmaStrU8Ptr->GetLength(), lengthEcmaStrU8);
    EXPECT_TRUE(ecmaStrU8Ptr->IsUtf8());
    EXPECT_FALSE(ecmaStrU8Ptr->IsUtf16());
}

HWTEST_F_L0(EcmaStringTest, CreateFromUtf16)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
    size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    EXPECT_EQ(ecmaStrU16CompPtr->GetLength(), lengthEcmaStrU16Comp);
    EXPECT_TRUE(ecmaStrU16CompPtr->IsUtf8());
    EXPECT_FALSE(ecmaStrU16CompPtr->IsUtf16());

    // CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {127, 33, 128, 12, 256, 11100, 65535};
    size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU16NotCompPtr->GetLength(), lengthEcmaStrU16NotComp);
    EXPECT_FALSE(ecmaStrU16NotCompPtr->IsUtf8());
    EXPECT_TRUE(ecmaStrU16NotCompPtr->IsUtf16());
    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrU16NotCompDisableCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0],
        lengthEcmaStrU16NotComp, ecmaVMPtr, false);
    EXPECT_TRUE(ecmaStrU16NotCompDisableCompPtr->IsUtf16());
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, ComputeSizeUtf8)
{
    uint32_t scale = 3333;
    for (uint32_t i = 0x40000000U - 1; i > scale; i = i - scale) {
        uint32_t length = i;
        EXPECT_EQ(EcmaString::ComputeSizeUtf8(length), length + sizeof(EcmaString));
    }
}

HWTEST_F_L0(EcmaStringTest, ComputeDataSizeUtf16)
{
    uint32_t scale = 3333;
    for (uint32_t i = 0x40000000U - 1; i > scale; i = i - scale) {
        uint32_t length = i;
        EXPECT_EQ(EcmaString::ComputeDataSizeUtf16(length), 2 * length);
    }
}

HWTEST_F_L0(EcmaStringTest, ComputeSizeUtf16)
{
    uint32_t scale = 3333;
    for (uint32_t i = 0x40000000U - 1; i > scale; i = i - scale) {
        uint32_t length = i;
        EXPECT_EQ(EcmaString::ComputeSizeUtf16(length), 2 * length + sizeof(EcmaString));
    }
}

HWTEST_F_L0(EcmaStringTest, ObjectSize)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    EcmaString *ecmaStrEmptyPtr = EcmaString::CreateEmptyString(ecmaVMPtr);
    EXPECT_EQ(ecmaStrEmptyPtr->ObjectSize(), sizeof(EcmaString) + 0);

    size_t lengthEcmaStrAllocComp = 5;
    EcmaString *ecmaStrAllocCompPtr = EcmaString::AllocStringObject(lengthEcmaStrAllocComp, true, ecmaVMPtr);
    EXPECT_EQ(ecmaStrAllocCompPtr->ObjectSize(), sizeof(EcmaString) + sizeof(uint8_t) * lengthEcmaStrAllocComp);

    size_t lengthEcmaStrAllocNotComp = 5;
    EcmaString *ecmaStrAllocNotCompPtr = EcmaString::AllocStringObject(lengthEcmaStrAllocNotComp, false, ecmaVMPtr);
    EXPECT_EQ(ecmaStrAllocNotCompPtr->ObjectSize(), sizeof(EcmaString) + sizeof(uint16_t) * lengthEcmaStrAllocNotComp);

    uint8_t arrayU8[] = {"abcde"};
    size_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    EXPECT_EQ(ecmaStrU8Ptr->ObjectSize(), sizeof(EcmaString) + sizeof(uint8_t) * lengthEcmaStrU8);

    // ObjectSize(). EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 23, 45, 67, 127};
    size_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    EXPECT_EQ(ecmaStrU16CompPtr->ObjectSize(), sizeof(EcmaString) + sizeof(uint8_t) * lengthEcmaStrU16Comp);

    // ObjectSize(). EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {127, 128, 256, 11100, 65535};
    size_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU16NotCompPtr->ObjectSize(), sizeof(EcmaString) + sizeof(uint16_t) * lengthEcmaStrU16NotComp);
}

HWTEST_F_L0(EcmaStringTest, Compare_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). Between EcmaString made by CreateFromUtf8().
    uint8_t arrayU8No1[3] = {1, 23};
    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint8_t arrayU8No3[6] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo3 = EcmaString::CreateFromUtf8(&arrayU8No3[0], lengthEcmaStrU8No3, ecmaVMPtr, true);
    EXPECT_EQ(ecmaStrU8PtrNo1->Compare(ecmaStrU8PtrNo2), -1);
    EXPECT_EQ(ecmaStrU8PtrNo2->Compare(ecmaStrU8PtrNo1), 1);
    EXPECT_EQ(ecmaStrU8PtrNo2->Compare(ecmaStrU8PtrNo3), 49 - 45);
    EXPECT_EQ(ecmaStrU8PtrNo3->Compare(ecmaStrU8PtrNo2), 45 - 49);
}

HWTEST_F_L0(EcmaStringTest, Compare_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). Between EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16CompNo1[] = {1, 23};
    uint16_t arrayU16CompNo2[] = {1, 23, 49};
    uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EXPECT_EQ(ecmaStrU16CompPtrNo1->Compare(ecmaStrU16CompPtrNo2), -1);
    EXPECT_EQ(ecmaStrU16CompPtrNo2->Compare(ecmaStrU16CompPtrNo1), 1);
    EXPECT_EQ(ecmaStrU16CompPtrNo2->Compare(ecmaStrU16CompPtrNo3), 49 - 45);
    EXPECT_EQ(ecmaStrU16CompPtrNo3->Compare(ecmaStrU16CompPtrNo2), 45 - 49);
}

HWTEST_F_L0(EcmaStringTest, Compare_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , true).
    uint8_t arrayU8No1[3] = {1, 23};
    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint16_t arrayU16CompNo1[] = {1, 23};
    uint16_t arrayU16CompNo2[] = {1, 23, 49};
    uint16_t arrayU16CompNo3[] = {1, 23, 45, 97, 127};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EXPECT_EQ(ecmaStrU8PtrNo1->Compare(ecmaStrU16CompPtrNo1), 0);
    EXPECT_EQ(ecmaStrU16CompPtrNo1->Compare(ecmaStrU8PtrNo1), 0);
    EXPECT_EQ(ecmaStrU8PtrNo1->Compare(ecmaStrU16CompPtrNo2), -1);
    EXPECT_EQ(ecmaStrU16CompPtrNo2->Compare(ecmaStrU8PtrNo1), 1);
    EXPECT_EQ(ecmaStrU8PtrNo2->Compare(ecmaStrU16CompPtrNo3), 49 - 45);
    EXPECT_EQ(ecmaStrU16CompPtrNo3->Compare(ecmaStrU8PtrNo2), 45 - 49);
}

HWTEST_F_L0(EcmaStringTest, Compare_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). Between EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo1->Compare(ecmaStrU16NotCompPtrNo2), -1);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo2->Compare(ecmaStrU16NotCompPtrNo1), 1);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo2->Compare(ecmaStrU16NotCompPtrNo3), 49 - 456);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo3->Compare(ecmaStrU16NotCompPtrNo2), 456 - 49);
}

HWTEST_F_L0(EcmaStringTest, Compare_005)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayU8No1[3] = {1, 23};
    uint8_t arrayU8No2[4] = {1, 23, 49};
    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU8PtrNo1->Compare(ecmaStrU16NotCompPtrNo1), 0);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo1->Compare(ecmaStrU8PtrNo1), 0);
    EXPECT_EQ(ecmaStrU8PtrNo1->Compare(ecmaStrU16NotCompPtrNo2), -1);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo2->Compare(ecmaStrU8PtrNo1), 1);
    EXPECT_EQ(ecmaStrU8PtrNo2->Compare(ecmaStrU16NotCompPtrNo3), 49 - 456);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo3->Compare(ecmaStrU8PtrNo2), 456 - 49);
}

HWTEST_F_L0(EcmaStringTest, Compare_006)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Compare(). EcmaString made by CreateFromUtf16( , , , true) and EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16CompNo1[] = {1, 23};
    uint16_t arrayU16CompNo2[] = {1, 23, 49};
    uint16_t arrayU16NotCompNo1[] = {1, 23};
    uint16_t arrayU16NotCompNo2[] = {1, 23, 49};
    uint16_t arrayU16NotCompNo3[] = {1, 23, 456, 6789, 65535, 127};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU16CompPtrNo1->Compare(ecmaStrU16NotCompPtrNo1), 0);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo1->Compare(ecmaStrU16CompPtrNo1), 0);
    EXPECT_EQ(ecmaStrU16CompPtrNo1->Compare(ecmaStrU16NotCompPtrNo2), -1);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo2->Compare(ecmaStrU16CompPtrNo1), 1);
    EXPECT_EQ(ecmaStrU16CompPtrNo2->Compare(ecmaStrU16NotCompPtrNo3), 49 - 456);
    EXPECT_EQ(ecmaStrU16NotCompPtrNo3->Compare(ecmaStrU16CompPtrNo2), 456 - 49);
}

HWTEST_F_L0(EcmaStringTest, Concat_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf8().
    uint8_t arrayFrontU8[] = {"abcdef"};
    uint8_t arrayBackU8[] = {"ABCDEF"};
    uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
    uint32_t lengthEcmaStrBackU8 = sizeof(arrayBackU8) - 1;
    EcmaString *ecmaStrFrontU8Ptr = EcmaString::CreateFromUtf8(&arrayFrontU8[0], lengthEcmaStrFrontU8, ecmaVMPtr,
        true);
    EcmaString *ecmaStrBackU8Ptr = EcmaString::CreateFromUtf8(&arrayBackU8[0], lengthEcmaStrBackU8, ecmaVMPtr, true);
    JSHandle<EcmaString> handle_ecmaStrFrontU8Ptr(thread, ecmaStrFrontU8Ptr);
    JSHandle<EcmaString> handle_ecmaStrBackU8Ptr(thread, ecmaStrBackU8Ptr);
    EcmaString *ecmaStrConcatU8Ptr = EcmaString::Concat(handle_ecmaStrFrontU8Ptr, handle_ecmaStrBackU8Ptr, ecmaVMPtr);
    EXPECT_TRUE(ecmaStrConcatU8Ptr->IsUtf8());
    for (int i = 0; i < lengthEcmaStrFrontU8; i++) {
        EXPECT_EQ(ecmaStrConcatU8Ptr->At(i), arrayFrontU8[i]);
    }
    for (int i = 0; i < lengthEcmaStrBackU8; i++) {
        EXPECT_EQ(ecmaStrConcatU8Ptr->At(i + lengthEcmaStrFrontU8), arrayBackU8[i]);
    }
    EXPECT_EQ(ecmaStrConcatU8Ptr->GetLength(), lengthEcmaStrFrontU8 + lengthEcmaStrBackU8);
}

HWTEST_F_L0(EcmaStringTest, Concat_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Concat(). EcmaString made by CreateFromUtf16( , , , false) and EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayFrontU16NotComp[] = {128, 129, 256, 11100, 65535, 100};
    uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
    uint32_t lengthEcmaStrFrontU16NotComp = sizeof(arrayFrontU16NotComp) / sizeof(arrayFrontU16NotComp[0]);
    uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
    EcmaString *ecmaStrFrontU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayFrontU16NotComp[0],
        lengthEcmaStrFrontU16NotComp, ecmaVMPtr, false);
    EcmaString *ecmaStrBackU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayBackU16NotComp[0],
        lengthEcmaStrBackU16NotComp, ecmaVMPtr, false);
    JSHandle<EcmaString> handle_ecmaStrFrontU16NotCompPtr(thread, ecmaStrFrontU16NotCompPtr);
    JSHandle<EcmaString> handle_ecmaStrBackU16NotCompPtr(thread, ecmaStrBackU16NotCompPtr);
    EcmaString *ecmaStrConcatU16NotCompPtr = EcmaString::Concat(handle_ecmaStrFrontU16NotCompPtr,
        handle_ecmaStrBackU16NotCompPtr, ecmaVMPtr);
    EXPECT_TRUE(ecmaStrConcatU16NotCompPtr->IsUtf16());
    for (int i = 0; i < lengthEcmaStrFrontU16NotComp; i++) {
        EXPECT_EQ(ecmaStrConcatU16NotCompPtr->At(i), arrayFrontU16NotComp[i]);
    }
    for (int i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
        EXPECT_EQ(ecmaStrConcatU16NotCompPtr->At(i + lengthEcmaStrFrontU16NotComp), arrayBackU16NotComp[i]);
    }
    EXPECT_EQ(ecmaStrConcatU16NotCompPtr->GetLength(), lengthEcmaStrFrontU16NotComp + lengthEcmaStrBackU16NotComp);
}

HWTEST_F_L0(EcmaStringTest, Concat_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // Concat(). EcmaString made by CreateFromUtf8() and EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayFrontU8[] = {"abcdef"};
    uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
    uint32_t lengthEcmaStrFrontU8 = sizeof(arrayFrontU8) - 1;
    uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
    EcmaString *ecmaStrFrontU8Ptr = EcmaString::CreateFromUtf8(&arrayFrontU8[0], lengthEcmaStrFrontU8, ecmaVMPtr,
        true);
    EcmaString *ecmaStrBackU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayBackU16NotComp[0],
        lengthEcmaStrBackU16NotComp, ecmaVMPtr, false);
    JSHandle<EcmaString> handle_ecmaStrFrontU8Ptr(thread, ecmaStrFrontU8Ptr);
    JSHandle<EcmaString> handle_ecmaStrBackU16NotCompPtr(thread, ecmaStrBackU16NotCompPtr);
    EcmaString *ecmaStrConcatU8U16NotCompPtr = EcmaString::Concat(handle_ecmaStrFrontU8Ptr,
        handle_ecmaStrBackU16NotCompPtr, ecmaVMPtr);
    EXPECT_TRUE(ecmaStrConcatU8U16NotCompPtr->IsUtf16());
    for (int i = 0; i < lengthEcmaStrFrontU8; i++) {
        EXPECT_EQ(ecmaStrConcatU8U16NotCompPtr->At(i), arrayFrontU8[i]);
    }
    for (int i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
        EXPECT_EQ(ecmaStrConcatU8U16NotCompPtr->At(i + lengthEcmaStrFrontU8), arrayBackU16NotComp[i]);
    }
    EXPECT_EQ(ecmaStrConcatU8U16NotCompPtr->GetLength(), lengthEcmaStrFrontU8 + lengthEcmaStrBackU16NotComp);
}

HWTEST_F_L0(EcmaStringTest, Concat_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    /* Concat() after SetCompressedStringsEnabled(false). EcmaString made by CreateFromUtf16( , , , false) and
     * EcmaString made by CreateFromUtf16( , , , false).
     */
    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    uint16_t arrayFrontU16NotComp[] = {128, 129, 256, 11100, 65535, 100};
    uint16_t arrayBackU16NotComp[] = {88, 768, 1, 270, 345, 333};
    uint32_t lengthEcmaStrFrontU16NotComp = sizeof(arrayFrontU16NotComp) / sizeof(arrayFrontU16NotComp[0]);
    uint32_t lengthEcmaStrBackU16NotComp = sizeof(arrayBackU16NotComp) / sizeof(arrayBackU16NotComp[0]);
    EcmaString *ecmaStrFrontU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayFrontU16NotComp[0],
        lengthEcmaStrFrontU16NotComp, ecmaVMPtr, false);
    EcmaString *ecmaStrBackU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayBackU16NotComp[0],
        lengthEcmaStrBackU16NotComp, ecmaVMPtr, false);
    JSHandle<EcmaString> handle_ecmaStrFrontU16NotCompPtr(thread, ecmaStrFrontU16NotCompPtr);
    JSHandle<EcmaString> handle_ecmaStrBackU16NotCompPtr(thread, ecmaStrBackU16NotCompPtr);
    EcmaString *ecmaStrConcatU16NotCompAfterSetFalsePtr = EcmaString::Concat(handle_ecmaStrFrontU16NotCompPtr,
        handle_ecmaStrBackU16NotCompPtr, ecmaVMPtr);
    EXPECT_TRUE(ecmaStrConcatU16NotCompAfterSetFalsePtr->IsUtf16());
    for (int i = 0; i < lengthEcmaStrFrontU16NotComp; i++) {
        EXPECT_EQ(ecmaStrConcatU16NotCompAfterSetFalsePtr->At(i), arrayFrontU16NotComp[i]);
    }
    for (int i = 0; i < lengthEcmaStrBackU16NotComp; i++) {
        EXPECT_EQ(ecmaStrConcatU16NotCompAfterSetFalsePtr->At(i + lengthEcmaStrFrontU16NotComp),
            arrayBackU16NotComp[i]);
    }
    EXPECT_EQ(ecmaStrConcatU16NotCompAfterSetFalsePtr->GetLength(),
        lengthEcmaStrFrontU16NotComp + lengthEcmaStrBackU16NotComp);
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, FastSubString_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // FastSubString(). From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    JSHandle<EcmaString> handle_ecmaStrU8Ptr(thread, ecmaStrU8Ptr);
    uint32_t indexStartSubU8 = 2;
    uint32_t lengthSubU8 = 2;
    EcmaString *ecmaStrSubU8Ptr = EcmaString::FastSubString(handle_ecmaStrU8Ptr, indexStartSubU8, lengthSubU8,
        ecmaVMPtr);
    for (int i = 0; i < lengthSubU8; i++) {
        EXPECT_EQ(ecmaStrSubU8Ptr->At(i), ecmaStrU8Ptr->At(i + indexStartSubU8));
    }
    EXPECT_EQ(ecmaStrSubU8Ptr->GetLength(), lengthSubU8);
}

HWTEST_F_L0(EcmaStringTest, FastSubString_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // FastSubString(). From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    JSHandle<EcmaString> handle_ecmaStrU16CompPtr(thread, ecmaStrU16CompPtr);
    uint32_t indexStartSubU16Comp = 0;
    uint32_t lengthSubU16Comp = 2;
    EcmaString *ecmaStrSubU16CompPtr = EcmaString::FastSubString(handle_ecmaStrU16CompPtr, indexStartSubU16Comp,
        lengthSubU16Comp, ecmaVMPtr);
    for (int i = 0; i < lengthSubU16Comp; i++) {
        EXPECT_EQ(ecmaStrSubU16CompPtr->At(i), ecmaStrU16CompPtr->At(i + indexStartSubU16Comp));
    }
    EXPECT_EQ(ecmaStrSubU16CompPtr->GetLength(), lengthSubU16Comp);
}

HWTEST_F_L0(EcmaStringTest, FastSubString_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // FastSubString(). From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    JSHandle<EcmaString> handle_ecmaStrU16NotCompPtr(thread, ecmaStrU16NotCompPtr);
    uint32_t indexStartSubU16NotComp = 0;
    uint32_t lengthSubU16NotComp = 2;
    EcmaString *ecmaStrSubU16NotCompPtr = EcmaString::FastSubString(handle_ecmaStrU16NotCompPtr,
        indexStartSubU16NotComp, lengthSubU16NotComp, ecmaVMPtr);
    for (int i = 0; i < lengthSubU16NotComp; i++) {
        EXPECT_EQ(ecmaStrSubU16NotCompPtr->At(i), ecmaStrU16NotCompPtr->At(i + indexStartSubU16NotComp));
    }
    EXPECT_EQ(ecmaStrSubU16NotCompPtr->GetLength(), lengthSubU16NotComp);
}

HWTEST_F_L0(EcmaStringTest, WriteData_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // WriteData(). From EcmaString made by CreateFromUtf8() to EcmaString made by AllocStringObject( , true, ).
    uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
    EcmaString *ecmaStrU8WriteFromPtr = EcmaString::CreateFromUtf8(&arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom,
        ecmaVMPtr, true);
    size_t sizeEcmaStrU8WriteTo = 5;
    EcmaString *ecmaStrAllocTrueWriteToPtr = EcmaString::AllocStringObject(sizeEcmaStrU8WriteTo, true, ecmaVMPtr);
    uint32_t indexStartWriteFromArrayU8 = 2;
    uint32_t lengthWriteFromArrayU8 = 2;
    ecmaStrAllocTrueWriteToPtr->WriteData(ecmaStrU8WriteFromPtr, indexStartWriteFromArrayU8, sizeEcmaStrU8WriteTo,
        lengthWriteFromArrayU8);
    for (int i = 0; i < lengthWriteFromArrayU8; i++) {
        EXPECT_EQ(ecmaStrAllocTrueWriteToPtr->At(i + indexStartWriteFromArrayU8), ecmaStrU8WriteFromPtr->At(i));
    }
}

HWTEST_F_L0(EcmaStringTest, WriteData_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // WriteData(). From char to EcmaString made by AllocStringObject( , true, ).
    char u8Write = 'a';
    size_t sizeEcmaStrU8WriteTo = 5;
    EcmaString *ecmaStrAllocTrueWriteToPtr = EcmaString::AllocStringObject(sizeEcmaStrU8WriteTo, true, ecmaVMPtr);
    uint32_t indexAtWriteFromU8 = 4;
    ecmaStrAllocTrueWriteToPtr->WriteData(u8Write, indexAtWriteFromU8);
    EXPECT_EQ(ecmaStrAllocTrueWriteToPtr->At(indexAtWriteFromU8), u8Write);
}

HWTEST_F_L0(EcmaStringTest, WriteData_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    /* WriteData(). From EcmaString made by CreateFromUtf16( , , , false) to EcmaStringU16 made by
     * AllocStringObject( , false, ).
     */
    uint16_t arrayU16WriteFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16WriteFrom = sizeof(arrayU16WriteFrom) / sizeof(arrayU16WriteFrom[0]);
    EcmaString *ecmaStrU16WriteFromPtr = EcmaString::CreateFromUtf16(&arrayU16WriteFrom[0], lengthEcmaStrU16WriteFrom,
        ecmaVMPtr, false);
    size_t sizeEcmaStrU16WriteTo = 10;
    EcmaString *ecmaStrU16WriteToPtr = EcmaString::AllocStringObject(sizeEcmaStrU16WriteTo, false, ecmaVMPtr);
    uint32_t indexStartWriteFromArrayU16 = 3;
    uint32_t numBytesWriteFromArrayU16 = 2 * 3;
    ecmaStrU16WriteToPtr->WriteData(ecmaStrU16WriteFromPtr, indexStartWriteFromArrayU16, sizeEcmaStrU16WriteTo,
        numBytesWriteFromArrayU16);
    for (int i = 0; i < (numBytesWriteFromArrayU16 / 2); i++) {
        EXPECT_EQ(ecmaStrU16WriteToPtr->At(i + indexStartWriteFromArrayU16), ecmaStrU16WriteFromPtr->At(i));
    }
}

HWTEST_F_L0(EcmaStringTest, WriteData_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // WriteData(). From EcmaString made by CreateFromUtf8() to EcmaString made by AllocStringObject( , false, ).
    uint8_t arrayU8WriteFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8WriteFrom = sizeof(arrayU8WriteFrom) - 1;
    EcmaString *ecmaStrU8WriteFromPtr = EcmaString::CreateFromUtf8(&arrayU8WriteFrom[0], lengthEcmaStrU8WriteFrom,
        ecmaVMPtr, true);
    size_t sizeEcmaStrU16WriteTo = 10;
    EcmaString *ecmaStrU16WriteToPtr = EcmaString::AllocStringObject(sizeEcmaStrU16WriteTo, false, ecmaVMPtr);
    uint32_t indexStartWriteFromU8ToU16 = 1;
    uint32_t numBytesWriteFromU8ToU16 = 4;
    ecmaStrU16WriteToPtr->WriteData(ecmaStrU8WriteFromPtr, indexStartWriteFromU8ToU16, sizeEcmaStrU16WriteTo,
        numBytesWriteFromU8ToU16);
    for (int i = 0; i < numBytesWriteFromU8ToU16; i++) {
        EXPECT_EQ(ecmaStrU16WriteToPtr->At(i + indexStartWriteFromU8ToU16), ecmaStrU8WriteFromPtr->At(i));
    }
}

HWTEST_F_L0(EcmaStringTest, WriteData_005)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // WriteData(). From char to EcmaString made by AllocStringObject( , false, ).
    size_t sizeEcmaStrU16WriteTo = 10;
    EcmaString *ecmaStrU16WriteToPtr = EcmaString::AllocStringObject(sizeEcmaStrU16WriteTo, false, ecmaVMPtr);
    char u8Write = 'a';
    uint32_t indexAt = 4;
    ecmaStrU16WriteToPtr->WriteData(u8Write, indexAt);
    EXPECT_EQ(ecmaStrU16WriteToPtr->At(indexAt), u8Write);
}

HWTEST_F_L0(EcmaStringTest, GetUtf8Length)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);
    uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
    uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
    uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU8Ptr->GetUtf8Length(), lengthEcmaStrU8 + 1);
    EXPECT_EQ(ecmaStrU16CompPtr->GetUtf8Length(), lengthEcmaStrU16Comp + 1);
    EXPECT_EQ(ecmaStrU16NotCompPtr->GetUtf8Length(), 2 * lengthEcmaStrU16NotComp + 1);
}

HWTEST_F_L0(EcmaStringTest, GetUtf16Length)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    uint8_t arrayU8[6] = {3, 7, 19, 54, 99};
    uint16_t arrayU16Comp[] = {1, 12, 34, 56, 127};
    uint16_t arrayU16NotComp[] = {19, 54, 256, 11100, 65535};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU8Ptr->GetUtf16Length(), lengthEcmaStrU8);
    EXPECT_EQ(ecmaStrU16CompPtr->GetUtf16Length(), lengthEcmaStrU16Comp);
    EXPECT_EQ(ecmaStrU16NotCompPtr->GetUtf16Length(), lengthEcmaStrU16NotComp);
}

HWTEST_F_L0(EcmaStringTest, GetDataUtf8)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[] = {"abcde"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    const uint8_t *arrayU8FromEcmaStrU8Ptr = ecmaStrU8Ptr->GetDataUtf8();
    for (int i =0; i < lengthEcmaStrU8; i++) {
        EXPECT_EQ(*(arrayU8FromEcmaStrU8Ptr + i), arrayU8[i]);
    }

    // From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {3, 1, 34, 123, 127, 111, 42, 3, 20, 10};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    const uint8_t *arrayU8FromEcmaStrU16CompPtr = ecmaStrU16CompPtr->GetDataUtf8();
    for (int i = 0; i < sizeof(arrayU16Comp) / arrayU16Comp[0]; i++) {
        EXPECT_EQ(*(arrayU8FromEcmaStrU16CompPtr + i), arrayU16Comp[i]);
    }
}

HWTEST_F_L0(EcmaStringTest, GetDataUtf16)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    const uint16_t *arrayU16FromEcmaStrU16NotCompPtr = ecmaStrU16NotCompPtr->GetDataUtf16();
    for (int i = 0; i < lengthEcmaStrU16NotComp; i++) {
        EXPECT_EQ(*(arrayU16FromEcmaStrU16NotCompPtr + i), arrayU16NotComp[i]);
    }
}

HWTEST_F_L0(EcmaStringTest, CopyDataRegionUtf8)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // CopyDataRegionUtf8(). From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8CopyFrom[6] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU8CopyFrom = sizeof(arrayU8CopyFrom) - 1;
    EcmaString *ecmaStrU8CopyFromPtr = EcmaString::CreateFromUtf8(&arrayU8CopyFrom[0], lengthEcmaStrU8CopyFrom,
        ecmaVMPtr, true);
    const size_t lengthEcmaStrU8CopyTo = 7;
    uint8_t defaultByteForU8CopyTo = 1;
    uint8_t arrayU8CopyTo[lengthEcmaStrU8CopyTo];
    memset_s(&arrayU8CopyTo[0], lengthEcmaStrU8CopyTo, defaultByteForU8CopyTo, lengthEcmaStrU8CopyTo);
    size_t indexStartFromArrayU8 = 2;
    size_t lengthCopyToEcmaStrU8 = 3;
    size_t lengthReturnU8 = ecmaStrU8CopyFromPtr->CopyDataRegionUtf8(arrayU8CopyTo, indexStartFromArrayU8,
        lengthCopyToEcmaStrU8, lengthEcmaStrU8CopyTo);
    EXPECT_EQ(lengthReturnU8, lengthCopyToEcmaStrU8);
    EXPECT_EQ(sizeof(arrayU8CopyTo), lengthEcmaStrU8CopyTo);
    for (int i = 0; i < lengthCopyToEcmaStrU8; i++) {
        EXPECT_EQ(arrayU8CopyTo[i], ecmaStrU8CopyFromPtr->At(i + indexStartFromArrayU8));
    }
    for (int i = lengthCopyToEcmaStrU8; i < lengthEcmaStrU8CopyTo; i++) {
        EXPECT_EQ(arrayU8CopyTo[i], defaultByteForU8CopyTo);
    }

    // CopyDataRegionUtf8(). From EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16CompCopyFrom[] = {1, 12, 34, 56, 127};
    uint32_t lengthEcmaStrU16CompCopyFrom = sizeof(arrayU16CompCopyFrom) / sizeof(arrayU16CompCopyFrom[0]);
    EcmaString *ecmaStrU16CompCopyFromPtr = EcmaString::CreateFromUtf16(&arrayU16CompCopyFrom[0],
        lengthEcmaStrU16CompCopyFrom, ecmaVMPtr, true);
    const size_t lengthEcmaStrU16CompCopyTo = 8;
    uint8_t defaultByteForU16CompCopyTo = 1;
    uint8_t arrayU16CompCopyTo[lengthEcmaStrU16CompCopyTo];
    memset_s(&arrayU16CompCopyTo[0], lengthEcmaStrU16CompCopyTo, defaultByteForU16CompCopyTo,
        lengthEcmaStrU16CompCopyTo);
    size_t indexStartFromArrayU16Comp = 2;
    size_t lengthCopyToEcmaStrU16Comp = 3;
    size_t lengthReturnU16Comp = ecmaStrU16CompCopyFromPtr->CopyDataRegionUtf8(&arrayU16CompCopyTo[0],
        indexStartFromArrayU16Comp, lengthCopyToEcmaStrU16Comp, lengthEcmaStrU16CompCopyTo);
    EXPECT_EQ(lengthReturnU16Comp, lengthCopyToEcmaStrU16Comp);
    EXPECT_EQ(sizeof(arrayU16CompCopyTo) / sizeof(arrayU16CompCopyTo[0]), lengthEcmaStrU16CompCopyTo);
    for (int i = 0; i < lengthCopyToEcmaStrU16Comp; i++) {
        EXPECT_EQ(arrayU16CompCopyTo[i], ecmaStrU16CompCopyFromPtr->At(i + indexStartFromArrayU16Comp));
    }
    for (int i = lengthCopyToEcmaStrU16Comp; i < lengthEcmaStrU16CompCopyTo; i++) {
        EXPECT_EQ(arrayU16CompCopyTo[i], defaultByteForU16CompCopyTo);
    }
}

HWTEST_F_L0(EcmaStringTest, CopyDataRegionUtf16)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // CopyDataRegionUtf16(). From EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotCompCopyFrom[10] = {67, 777, 1999, 1, 45, 66, 23456, 65535, 127, 333};
    uint32_t lengthEcmaStrU16NotCompCopyFrom = sizeof(arrayU16NotCompCopyFrom) / sizeof(arrayU16NotCompCopyFrom[0]);
    EcmaString *ecmaStrU16NotCompCopyFromPtr = EcmaString::CreateFromUtf16(&arrayU16NotCompCopyFrom[0],
        lengthEcmaStrU16NotCompCopyFrom, ecmaVMPtr, false);
    const size_t lengthOfArrayU16NotCompCopyTo = 13;
    uint8_t defaultOneByteValueOfArrayU16NotCompCopyTo = 244;
    uint16_t arrayU16NotCompCopyTo[lengthOfArrayU16NotCompCopyTo];
    memset_s(&arrayU16NotCompCopyTo[0], sizeof(uint16_t) * lengthOfArrayU16NotCompCopyTo,
        defaultOneByteValueOfArrayU16NotCompCopyTo, sizeof(uint16_t) * lengthOfArrayU16NotCompCopyTo);
    size_t startIndexFromArrayU16NotComp = 2;
    size_t byteLengthCopyFromArrayU16NotComp = 3;
    size_t lengthReturnU16NotComp = ecmaStrU16NotCompCopyFromPtr->CopyDataRegionUtf16(&arrayU16NotCompCopyTo[0],
        startIndexFromArrayU16NotComp, byteLengthCopyFromArrayU16NotComp, lengthOfArrayU16NotCompCopyTo);
    EXPECT_EQ(lengthReturnU16NotComp, byteLengthCopyFromArrayU16NotComp);
    EXPECT_EQ(sizeof(arrayU16NotCompCopyTo) / sizeof(arrayU16NotCompCopyTo[0]), lengthOfArrayU16NotCompCopyTo);
    for (int i = 0; i < byteLengthCopyFromArrayU16NotComp; i++) {
        EXPECT_EQ(arrayU16NotCompCopyTo[i], ecmaStrU16NotCompCopyFromPtr->At(i + startIndexFromArrayU16NotComp));
    }
    for (int i = byteLengthCopyFromArrayU16NotComp; i < lengthOfArrayU16NotCompCopyTo; i++) {
        EXPECT_EQ(arrayU16NotCompCopyTo[i], ((uint16_t)defaultOneByteValueOfArrayU16NotCompCopyTo) * (1 + (1 << 8)));
    }
}

HWTEST_F_L0(EcmaStringTest, IndexOf_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // IndexOf(). Find EcmaString made by CreateFromUtf8() From EcmaString made by CreateFromUtf8().
    uint8_t arrayU8From[7] = {23, 25, 1, 3, 39, 80};
    uint8_t arrayU8Target[4] = {1, 3, 39};
    uint32_t lengthEcmaStrU8From = sizeof(arrayU8From) - 1;
    uint32_t lengthEcmaStrU8Target = sizeof(arrayU8Target) - 1;
    EcmaString *ecmaStrU8FromPtr = EcmaString::CreateFromUtf8(&arrayU8From[0], lengthEcmaStrU8From, ecmaVMPtr, true);
    EcmaString *ecmaStrU8TargetPtr = EcmaString::CreateFromUtf8(&arrayU8Target[0], lengthEcmaStrU8Target, ecmaVMPtr,
        true);
    int32_t posStart = 0;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU8TargetPtr, posStart), 2);
    EXPECT_EQ(ecmaStrU8TargetPtr->IndexOf(ecmaStrU8FromPtr, posStart), -1);
    posStart = -1;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU8TargetPtr, posStart), 2);
    posStart = 1;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU8TargetPtr, posStart), 2);
    posStart = 2;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU8TargetPtr, posStart), 2);
    posStart = 3;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU8TargetPtr, posStart), -1);
}

HWTEST_F_L0(EcmaStringTest, IndexOf_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // IndexOf(). Find EcmaString made by CreateFromUtf8() From EcmaString made by CreateFromUtf16( , , , false).
    uint8_t arrayU8Target[4] = {1, 3, 39};
    uint16_t arrayU16NotCompFromNo1[] = {67, 65535, 127, 777, 1453, 44, 1, 3, 39, 80, 333};
    uint32_t lengthEcmaStrU8Target = sizeof(arrayU8Target) - 1;
    uint32_t lengthEcmaStrU16NotCompFromNo1 = sizeof(arrayU16NotCompFromNo1) / sizeof(arrayU16NotCompFromNo1[0]);
    EcmaString *ecmaStrU8TargetPtr = EcmaString::CreateFromUtf8(&arrayU8Target[0], lengthEcmaStrU8Target, ecmaVMPtr,
        true);
    EcmaString *ecmaStrU16NotCompFromPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompFromNo1[0],
        lengthEcmaStrU16NotCompFromNo1, ecmaVMPtr, false);
    int32_t posStart = 0;
    EXPECT_EQ(ecmaStrU16NotCompFromPtrNo1->IndexOf(ecmaStrU8TargetPtr, posStart), 6);
    EXPECT_EQ(ecmaStrU8TargetPtr->IndexOf(ecmaStrU16NotCompFromPtrNo1, posStart), -1);
    posStart = -1;
    EXPECT_EQ(ecmaStrU16NotCompFromPtrNo1->IndexOf(ecmaStrU8TargetPtr, posStart), 6);
    posStart = 1;
    EXPECT_EQ(ecmaStrU16NotCompFromPtrNo1->IndexOf(ecmaStrU8TargetPtr, posStart), 6);
    posStart = 6;
    EXPECT_EQ(ecmaStrU16NotCompFromPtrNo1->IndexOf(ecmaStrU8TargetPtr, posStart), 6);
    posStart = 7;
    EXPECT_EQ(ecmaStrU16NotCompFromPtrNo1->IndexOf(ecmaStrU8TargetPtr, posStart), -1);
}

HWTEST_F_L0(EcmaStringTest, IndexOf_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    /* IndexOf(). Find EcmaString made by CreateFromUtf16( , , , false) From EcmaString made by
     * CreateFromUtf16( , , , false).
     */
    uint16_t arrayU16NotCompTarget[] = {1453, 44};
    uint16_t arrayU16NotCompFrom[] = {67, 65535, 127, 777, 1453, 44, 1, 3, 39, 80, 333};
    uint32_t lengthEcmaStrU16NotCompTarget = sizeof(arrayU16NotCompTarget) / sizeof(arrayU16NotCompTarget[0]);
    uint32_t lengthEcmaStrU16NotCompFrom = sizeof(arrayU16NotCompFrom) / sizeof(arrayU16NotCompFrom[0]);
    EcmaString *ecmaStrU16NotCompTargetPtr = EcmaString::CreateFromUtf16(&arrayU16NotCompTarget[0],
        lengthEcmaStrU16NotCompTarget, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompFromPtr = EcmaString::CreateFromUtf16(&arrayU16NotCompFrom[0],
        lengthEcmaStrU16NotCompFrom, ecmaVMPtr, false);
    int32_t posStart = 0;
    EXPECT_EQ(ecmaStrU16NotCompFromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 4);
    EXPECT_EQ(ecmaStrU16NotCompTargetPtr->IndexOf(ecmaStrU16NotCompFromPtr, posStart), -1);
    posStart = -1;
    EXPECT_EQ(ecmaStrU16NotCompFromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 4);
    posStart = 1;
    EXPECT_EQ(ecmaStrU16NotCompFromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 4);
    posStart = 4;
    EXPECT_EQ(ecmaStrU16NotCompFromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 4);
    posStart = 5;
    EXPECT_EQ(ecmaStrU16NotCompFromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), -1);
}

HWTEST_F_L0(EcmaStringTest, IndexOf_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // IndexOf(). Find EcmaString made by CreateFromUtf16( , , , false) From EcmaString made by CreateFromUtf8().
    uint16_t ecmaStrU16NotCompTarget[] = {3, 39, 80};
    uint8_t arrayU8From[7] = {23, 25, 1, 3, 39, 80};
    uint32_t lengthEcmaStrU16NotCompTarget = sizeof(ecmaStrU16NotCompTarget) / sizeof(ecmaStrU16NotCompTarget[0]);
    uint32_t lengthEcmaStrU8From = sizeof(arrayU8From) - 1;
    EcmaString *ecmaStrU16NotCompTargetPtr = EcmaString::CreateFromUtf16(&ecmaStrU16NotCompTarget[0],
        lengthEcmaStrU16NotCompTarget, ecmaVMPtr, false);
    EcmaString *ecmaStrU8FromPtr = EcmaString::CreateFromUtf8(&arrayU8From[0], lengthEcmaStrU8From, ecmaVMPtr, true);
    int32_t posStart = 0;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 3);
    EXPECT_EQ(ecmaStrU16NotCompTargetPtr->IndexOf(ecmaStrU8FromPtr, posStart), -1);
    posStart = -1;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 3);
    posStart = 1;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 3);
    posStart = 3;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), 3);
    posStart = 4;
    EXPECT_EQ(ecmaStrU8FromPtr->IndexOf(ecmaStrU16NotCompTargetPtr, posStart), -1);
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[4] = {45, 92, 78};
    uint8_t arrayU8No3[5] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo3 = EcmaString::CreateFromUtf8(&arrayU8No3[0], lengthEcmaStrU8No3, ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo1, ecmaStrU8PtrNo2));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo1, ecmaStrU8PtrNo3));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo3, ecmaStrU8PtrNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78};
    uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo1, ecmaStrU16CompPtrNo2));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo1, ecmaStrU16CompPtrNo3));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78};
    uint16_t arrayU16CompNo3[] = {45, 92, 78, 1};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqual(ecmaStrU16CompPtrNo1, ecmaStrU16CompPtrNo2));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16CompPtrNo1, ecmaStrU16CompPtrNo3));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16CompPtrNo3, ecmaStrU16CompPtrNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU8PtrNo1, ecmaStrU16NotCompPtrNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16NotCompPtrNo1, ecmaStrU8PtrNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_005)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16CompPtrNo1, ecmaStrU16NotCompPtrNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16NotCompPtrNo1, ecmaStrU16CompPtrNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqual_006)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqual().
    uint16_t arrayU16NotCompNo1[] = {234, 345, 127, 2345, 65535, 5};
    uint16_t arrayU16NotCompNo2[] = {234, 345, 127, 2345, 65535, 5};
    uint16_t arrayU16NotCompNo3[] = {1, 234, 345, 127, 2345, 65535, 5};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EXPECT_TRUE(EcmaString::StringsAreEqual(ecmaStrU16NotCompPtrNo1, ecmaStrU16NotCompPtrNo2));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16NotCompPtrNo1, ecmaStrU16NotCompPtrNo3));
    EXPECT_FALSE(EcmaString::StringsAreEqual(ecmaStrU16NotCompPtrNo3, ecmaStrU16NotCompPtrNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf8_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf8(). EcmaString made by CreateFromUtf8(), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint8_t arrayU8No3[3] = {45, 92};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo3 = EcmaString::CreateFromUtf8(&arrayU8No3[0], lengthEcmaStrU8No3, ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf8(ecmaStrU8PtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU8PtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU8PtrNo2, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU8PtrNo3, &arrayU8No1[0], lengthEcmaStrU8No1, true));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf8_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf8(). EcmaString made by CreateFromUtf16( , , , true), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
    uint16_t arrayU16CompNo3[] = {45, 92};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf8(ecmaStrU16CompPtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16CompPtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16CompPtrNo2, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16CompPtrNo3, &arrayU8No1[0], lengthEcmaStrU8No1, true));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf8_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf8(). EcmaString made by CreateFromUtf16( , , , false), Array:U8.
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint16_t arrayU16NotCompNo2[] = {45, 92, 78, 24};
    uint16_t arrayU16NotCompNo3[] = {45, 92};
    uint16_t arrayU16NotCompNo4[] = {25645, 25692, 25678};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    uint32_t lengthEcmaStrU16NotCompNo4 = sizeof(arrayU16NotCompNo4) / sizeof(arrayU16NotCompNo4[0]);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo4 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo4[0],
        lengthEcmaStrU16NotCompNo4, ecmaVMPtr, false);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf8(ecmaStrU16NotCompPtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16NotCompPtrNo1, &arrayU8No1[0], lengthEcmaStrU8No1, true));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16NotCompPtrNo2, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16NotCompPtrNo3, &arrayU8No1[0], lengthEcmaStrU8No1, false));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf8(ecmaStrU16NotCompPtrNo4, &arrayU8No1[0], lengthEcmaStrU8No1, false));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf16_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf8, Array:U16(1-127).
    uint8_t arrayU8No1[4] = {45, 92, 78};
    uint8_t arrayU8No2[5] = {45, 92, 78, 24};
    uint8_t arrayU8No3[3] = {45, 92};
    uint16_t arrayU16NotCompNo1[] = {45, 92, 78};
    uint32_t lengthEcmaStrU8No1 = sizeof(arrayU8No1) - 1;
    uint32_t lengthEcmaStrU8No2 = sizeof(arrayU8No2) - 1;
    uint32_t lengthEcmaStrU8No3 = sizeof(arrayU8No3) - 1;
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    EcmaString *ecmaStrU8PtrNo1 = EcmaString::CreateFromUtf8(&arrayU8No1[0], lengthEcmaStrU8No1, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo2 = EcmaString::CreateFromUtf8(&arrayU8No2[0], lengthEcmaStrU8No2, ecmaVMPtr, true);
    EcmaString *ecmaStrU8PtrNo3 = EcmaString::CreateFromUtf8(&arrayU8No3[0], lengthEcmaStrU8No3, ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf16(ecmaStrU8PtrNo1, &arrayU16NotCompNo1[0], lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU8PtrNo2, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU8PtrNo3, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf16_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , true), Array:U16(1-127).
    uint16_t arrayU16CompNo1[] = {45, 92, 78};
    uint16_t arrayU16CompNo2[] = {45, 92, 78, 24};
    uint16_t arrayU16CompNo3[] = {45, 92};
    uint16_t arrayU16CompNo4[] = {25645, 25692, 25678}; // 25645 % 256 == 45...
    uint32_t lengthEcmaStrU16CompNo1 = sizeof(arrayU16CompNo1) / sizeof(arrayU16CompNo1[0]);
    uint32_t lengthEcmaStrU16CompNo2 = sizeof(arrayU16CompNo2) / sizeof(arrayU16CompNo2[0]);
    uint32_t lengthEcmaStrU16CompNo3 = sizeof(arrayU16CompNo3) / sizeof(arrayU16CompNo3[0]);
    uint32_t lengthEcmaStrU16CompNo4 = sizeof(arrayU16CompNo4) / sizeof(arrayU16CompNo4[0]);
    EcmaString *ecmaStrU16CompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16CompNo1[0], lengthEcmaStrU16CompNo1,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16CompNo2[0], lengthEcmaStrU16CompNo2,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16CompNo3[0], lengthEcmaStrU16CompNo3,
        ecmaVMPtr, true);
    EcmaString *ecmaStrU16CompPtrNo4 = EcmaString::CreateFromUtf16(&arrayU16CompNo4[0], lengthEcmaStrU16CompNo4,
        ecmaVMPtr, true);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf16(ecmaStrU16CompPtrNo1, &arrayU16CompNo1[0],
        lengthEcmaStrU16CompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16CompPtrNo2, &arrayU16CompNo1[0],
        lengthEcmaStrU16CompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16CompPtrNo3, &arrayU16CompNo1[0],
        lengthEcmaStrU16CompNo1));
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf16(ecmaStrU16CompPtrNo4, &arrayU16CompNo1[0],
        lengthEcmaStrU16CompNo1));
}

HWTEST_F_L0(EcmaStringTest, StringsAreEqualUtf16_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // StringsAreEqualUtf16(). EcmaString made by CreateFromUtf16( , , , false), Array:U16(0-65535).
    uint16_t arrayU16NotCompNo1[] = {25645, 25692, 25678};
    uint16_t arrayU16NotCompNo2[] = {25645, 25692, 78}; // 25645 % 256 == 45...
    uint16_t arrayU16NotCompNo3[] = {25645, 25692, 25678, 65535};
    uint16_t arrayU16NotCompNo4[] = {25645, 25692};
    uint32_t lengthEcmaStrU16NotCompNo1 = sizeof(arrayU16NotCompNo1) / sizeof(arrayU16NotCompNo1[0]);
    uint32_t lengthEcmaStrU16NotCompNo2 = sizeof(arrayU16NotCompNo2) / sizeof(arrayU16NotCompNo2[0]);
    uint32_t lengthEcmaStrU16NotCompNo3 = sizeof(arrayU16NotCompNo3) / sizeof(arrayU16NotCompNo3[0]);
    uint32_t lengthEcmaStrU16NotCompNo4 = sizeof(arrayU16NotCompNo4) / sizeof(arrayU16NotCompNo4[0]);
    EcmaString *ecmaStrU16NotCompPtrNo1 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo2 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo3 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo3[0],
        lengthEcmaStrU16NotCompNo3, ecmaVMPtr, false);
    EcmaString *ecmaStrU16NotCompPtrNo4 = EcmaString::CreateFromUtf16(&arrayU16NotCompNo4[0],
        lengthEcmaStrU16NotCompNo4, ecmaVMPtr, false);
    EXPECT_TRUE(EcmaString::StringsAreEqualUtf16(ecmaStrU16NotCompPtrNo1, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16NotCompPtrNo1, &arrayU16NotCompNo2[0],
        lengthEcmaStrU16NotCompNo2));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16NotCompPtrNo2, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16NotCompPtrNo3, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
    EXPECT_FALSE(EcmaString::StringsAreEqualUtf16(ecmaStrU16NotCompPtrNo4, &arrayU16NotCompNo1[0],
        lengthEcmaStrU16NotCompNo1));
}

HWTEST_F_L0(EcmaStringTest, ComputeHashcodeUtf8)
{
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        hashExpect = hashExpect * 31 + arrayU8[i];
    }
    EXPECT_EQ(EcmaString::ComputeHashcodeUtf8(&arrayU8[0], lengthEcmaStrU8), static_cast<int32_t>(hashExpect));
}

HWTEST_F_L0(EcmaStringTest, ComputeHashcodeUtf16)
{
    uint16_t arrayU16[] = {199, 1, 256, 65535, 777};
    uint32_t lengthEcmaStrU16 = sizeof(arrayU16) / sizeof(arrayU16[0]);
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16; i++) {
        hashExpect = hashExpect * 31 + arrayU16[i];
    }
    EXPECT_EQ(EcmaString::ComputeHashcodeUtf16(&arrayU16[0], lengthEcmaStrU16), static_cast<int32_t>(hashExpect));
}

HWTEST_F_L0(EcmaStringTest, GetHashcode_001)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // GetHashcode(). EcmaString made by CreateFromUtf8().
    uint8_t arrayU8[] = {"abc"};
    uint32_t lengthEcmaStrU8 = sizeof(arrayU8) - 1;
    EcmaString *ecmaStrU8Ptr = EcmaString::CreateFromUtf8(&arrayU8[0], lengthEcmaStrU8, ecmaVMPtr, true);
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU8; i++) {
        hashExpect = hashExpect * 31 + arrayU8[i];
    }
    EXPECT_EQ(ecmaStrU8Ptr->GetHashcode(), static_cast<int32_t>(hashExpect));
}

HWTEST_F_L0(EcmaStringTest, GetHashcode_002)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // GetHashcode(). EcmaString made by CreateFromUtf16( , , , true).
    uint16_t arrayU16Comp[] = {45, 92, 78, 24};
    uint32_t lengthEcmaStrU16Comp = sizeof(arrayU16Comp) / sizeof(arrayU16Comp[0]);
    EcmaString *ecmaStrU16CompPtr = EcmaString::CreateFromUtf16(&arrayU16Comp[0], lengthEcmaStrU16Comp, ecmaVMPtr,
        true);
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16Comp; i++) {
        hashExpect = hashExpect * 31 + arrayU16Comp[i];
    }
    EXPECT_EQ(ecmaStrU16CompPtr->GetHashcode(), static_cast<int32_t>(hashExpect));
}

HWTEST_F_L0(EcmaStringTest, GetHashcode_003)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // GetHashcode(). EcmaString made by CreateFromUtf16( , , , false).
    uint16_t arrayU16NotComp[] = {199, 1, 256, 65535, 777};
    uint32_t lengthEcmaStrU16NotComp = sizeof(arrayU16NotComp) / sizeof(arrayU16NotComp[0]);
    EcmaString *ecmaStrU16NotCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0], lengthEcmaStrU16NotComp,
        ecmaVMPtr, false);
    uint32_t hashExpect = 0;
    for (uint32_t i = 0; i < lengthEcmaStrU16NotComp; i++) {
        hashExpect = hashExpect * 31 + arrayU16NotComp[i];
    }
    EXPECT_EQ(ecmaStrU16NotCompPtr->GetHashcode(), static_cast<int32_t>(hashExpect));

    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrU16NotCompDisableCompPtr = EcmaString::CreateFromUtf16(&arrayU16NotComp[0],
        lengthEcmaStrU16NotComp, ecmaVMPtr, false);
    EXPECT_EQ(ecmaStrU16NotCompDisableCompPtr->GetHashcode(), static_cast<int32_t>(hashExpect));
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, GetHashcode_004)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // GetHashcode(). EcmaString made by CreateEmptyString().
    EcmaString *ecmaStrEmptyPtr = EcmaString::CreateEmptyString(ecmaVMPtr);
    EXPECT_EQ(ecmaStrEmptyPtr->GetHashcode(), 0);

    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrEmptyDisableCompPtr = EcmaString::CreateEmptyString(ecmaVMPtr);
    EXPECT_EQ(ecmaStrEmptyDisableCompPtr->GetHashcode(), 0);
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}

HWTEST_F_L0(EcmaStringTest, GetHashcode_005)
{
    EcmaVM* ecmaVMPtr = EcmaVM::Cast(instance);

    // GetHashcode(). EcmaString made by AllocStringObject().
    size_t sizeAlloc = 5;
    EcmaString *ecmaStrAllocCompPtr = EcmaString::AllocStringObject(sizeAlloc, true, ecmaVMPtr);
    EcmaString *ecmaStrAllocNotCompPtr = EcmaString::AllocStringObject(sizeAlloc, false, ecmaVMPtr);
    EXPECT_EQ(ecmaStrAllocCompPtr->GetHashcode(), 0);
    EXPECT_EQ(ecmaStrAllocNotCompPtr->GetHashcode(), 0);

    EcmaString::SetCompressedStringsEnabled(false); // Set compressedStringsEnabled false.
    EcmaString *ecmaStrAllocNotCompDisableCompPtr = EcmaString::AllocStringObject(sizeAlloc, false, ecmaVMPtr);
    EXPECT_EQ(ecmaStrAllocNotCompDisableCompPtr->GetHashcode(), 0);
    EcmaString::SetCompressedStringsEnabled(true); // Set compressedStringsEnabled true(default).
}
}  // namespace panda::ecmascript
