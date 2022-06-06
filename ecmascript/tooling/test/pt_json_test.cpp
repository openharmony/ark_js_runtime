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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/base/pt_json.h"

using namespace panda::ecmascript::tooling;

namespace panda::test {
class PtJsonTest : public testing::Test {
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
    }

    void TearDown() override
    {
    }
};

HWTEST_F_L0(PtJsonTest, FalseTest)
{
    std::string str = "false";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsBool());
    EXPECT_FALSE(json->GetBool());
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, TrueTest)
{
    std::string str = "true";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsBool());
    EXPECT_TRUE(json->GetBool());
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, IntTest)
{
    std::string str = "100";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsNumber());
    EXPECT_EQ(json->GetInt(), 100);
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, Int64Test)
{
    std::string str = "123456789012345";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsNumber());
    EXPECT_EQ(json->GetInt64(), 123456789012345);
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, DoubleTest)
{
    std::string str = "12345.6789";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsNumber());
    EXPECT_EQ(json->GetDouble(), 12345.6789);
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, StringTest)
{
    std::string str = "\"abcdefg\"";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsString());
    EXPECT_EQ(json->GetString(), "abcdefg");
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, ArrayTest)
{
    std::string str = "[\"a\",\"b\",200]";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsArray());
    EXPECT_EQ(json->GetSize(), 3);
    EXPECT_EQ(json->Get(0)->GetString(), "a");
    EXPECT_EQ(json->Get(1)->GetString(), "b");
    EXPECT_EQ(json->Get(2)->GetInt(), 200);
    EXPECT_EQ(json->Stringify(), str);
    json->ReleaseRoot();
}

HWTEST_F_L0(PtJsonTest, ObjectTest)
{
    auto child1 = PtJson::CreateObject();
    child1->Add("ch", "child_1");
    ASSERT_TRUE(child1->Contains("ch"));

    auto child2 = PtJson::CreateObject();
    child2->Add("ch", "child_2");
    ASSERT_TRUE(child2->Contains("ch"));

    auto arr = PtJson::CreateArray();
    arr->Push(false);
    arr->Push(100);
    arr->Push(100.2);
    arr->Push(static_cast<int64_t>(200));
    arr->Push("abc");
    arr->Push(child1);
    EXPECT_EQ(arr->GetSize(), 6);

    auto root = PtJson::CreateObject();
    root->Add("a", false);
    root->Add("b", 100);
    root->Add("c", 100.2);
    root->Add("d", static_cast<int64_t>(200));
    root->Add("e", "abc");
    root->Add("f", child2);
    root->Add("g", arr);

    EXPECT_FALSE(root->GetBool("a"));
    EXPECT_EQ(root->GetInt("b"), 100);
    EXPECT_EQ(root->GetDouble("c"), 100.2);
    EXPECT_EQ(root->GetInt64("d"), static_cast<int64_t>(200));
    EXPECT_EQ(root->GetString("e"), "abc");
    EXPECT_EQ(root->GetObject("f")->GetString("ch"), "child_2");
    ASSERT_TRUE(root->GetArray("g")->IsArray());
    EXPECT_FALSE(root->GetArray("g")->Get(0)->GetBool());
    EXPECT_EQ(root->GetArray("g")->Get(1)->GetInt(), 100);
    EXPECT_EQ(root->GetArray("g")->Get(2)->GetDouble(), 100.2);
    EXPECT_EQ(root->GetArray("g")->Get(3)->GetInt64(), static_cast<int64_t>(200));
    EXPECT_EQ(root->GetArray("g")->Get(4)->GetString(), "abc");
    EXPECT_EQ(root->GetArray("g")->Get(5)->GetString("ch"), "child_1");

    EXPECT_EQ(root->Stringify(),
        "{\"a\":false,\"b\":100,\"c\":100.2,\"d\":200,\"e\":\"abc\",\"f\":{\"ch\":\"child_2\"},"
        "\"g\":[false,100,100.2,200,\"abc\",{\"ch\":\"child_1\"}]}");
    root->ReleaseRoot();
}
}