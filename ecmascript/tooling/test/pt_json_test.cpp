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

HWTEST_F_L0(PtJsonTest, ArrayTest1)
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

HWTEST_F_L0(PtJsonTest, ArrayTest2)
{
    std::string str = "[\"a\",\"b\",200,10.5,{}]";
    std::unique_ptr<PtJson> json = PtJson::Parse(str.c_str());
    ASSERT_TRUE(json->IsArray());
    EXPECT_EQ(json->GetSize(), 5);
    EXPECT_EQ(json->Get(0)->GetString(), "a");
    EXPECT_EQ(json->Get(1)->GetString(), "b");
    EXPECT_EQ(json->Get(2)->GetInt(), 200);
    EXPECT_EQ(json->Get(3)->GetDouble(), 10.5);
    EXPECT_TRUE(json->Get(4)->IsObject());
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
    arr->Push(100);
    EXPECT_EQ(arr->GetSize(), 1);

    auto root = PtJson::CreateObject();
    root->Add("a", false);
    root->Add("b", 100);
    root->Add("c", 100.2);
    root->Add("d", static_cast<int64_t>(200));
    root->Add("e", "abc");
    root->Add("f", child2);
    root->Add("g", arr);

    bool b;
    int32_t i32;
    int64_t i64;
    double d;
    std::string str;
    std::unique_ptr<PtJson> json;
    ASSERT_EQ(root->GetBool("a", &b), Result::SUCCESS);
    EXPECT_FALSE(b);
    ASSERT_EQ(root->GetInt("b", &i32), Result::SUCCESS);
    EXPECT_EQ(i32, 100);
    ASSERT_EQ(root->GetDouble("c", &d), Result::SUCCESS);
    EXPECT_EQ(d, 100.2);
    ASSERT_EQ(root->GetInt64("d", &i64), Result::SUCCESS);
    EXPECT_EQ(i64, static_cast<int64_t>(200));
    ASSERT_EQ(root->GetString("e", &str), Result::SUCCESS);
    EXPECT_EQ(str, "abc");
    ASSERT_EQ(root->GetObject("f", &json), Result::SUCCESS);
    ASSERT_EQ(json->GetString("ch", &str), Result::SUCCESS);
    EXPECT_EQ(str, "child_2");
    ASSERT_EQ(root->GetArray("g", &json), Result::SUCCESS);
    ASSERT_TRUE(json->IsArray());
    EXPECT_EQ(json->Get(0)->GetInt(), 100);

    EXPECT_EQ(root->Stringify(),
        "{\"a\":false,\"b\":100,\"c\":100.2,\"d\":200,\"e\":\"abc\",\"f\":{\"ch\":\"child_2\"},\"g\":[100]}");
    root->ReleaseRoot();
}
}