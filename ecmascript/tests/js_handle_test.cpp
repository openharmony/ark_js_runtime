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
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_handle_collection.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSHandleTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    CVector<JSHandle<EcmaString>> TestGlobalHandleNewBlock(GlobalHandleCollection global);
    void TestGlobalHandleFree(GlobalHandleCollection global);
};

CVector<JSHandle<EcmaString>> JSHandleTest::TestGlobalHandleNewBlock(GlobalHandleCollection global)
{
    ecmascript::EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString test = "test";
    CVector<JSHandle<EcmaString>> vec;
    // 10 : test case
    for (int i = 0; i < 10; i++) {
        test.append(ToCString(i));
        vec.push_back(global.NewHandle<EcmaString>(factory->NewFromString(test).GetTaggedType()));
    }

    [[maybe_unused]] auto storage = thread->GetGlobalHandleStorage();
    ASSERT(storage->GetNodes()->size() == 1);
    return vec;
}

HWTEST_F_L0(JSHandleTest, GlobalHandleNewBlock)
{
    GlobalHandleCollection global(thread);
    CVector<JSHandle<EcmaString>> vec = TestGlobalHandleNewBlock(global);

    // trigger GC
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    factory->NewFromString("trigger gc");

    for (auto v : vec) {
        global.Dispose(v);
    }
}

void JSHandleTest::TestGlobalHandleFree(GlobalHandleCollection global)
{
    EcmaHandleScope scope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString test = "test";
    // 10 : test case
    for (int i = 0; i < 10; i++) {
        test.append(ToCString(i));
        JSHandle<EcmaString> str = global.NewHandle<EcmaString>(factory->NewFromString(test).GetTaggedType());
        global.Dispose(str);
    }

    [[maybe_unused]] auto storage = thread->GetGlobalHandleStorage();
    ASSERT(storage->GetNodes()->size() == 1);
}

HWTEST_F_L0(JSHandleTest, GlobalHandleFree)
{
    GlobalHandleCollection global(thread);
    TestGlobalHandleFree(global);
}

HWTEST_F_L0(JSHandleTest, HandleScopeFree)
{
    // enable this check after add zapfree in ecma handlescope
    JSHandle<EcmaString> string = thread->GetEcmaVM()->GetFactory()->NewFromString("test1");
    {
        ecmascript::EcmaHandleScope scope(thread);
        JSHandle<EcmaString> string2 = thread->GetEcmaVM()->GetFactory()->NewFromString("test2");
        string = string2;
    }
}

HWTEST_F_L0(JSHandleTest, NewHandle)
{
    JSHandle<EcmaString> string = thread->GetEcmaVM()->GetFactory()->NewFromString("test1");
    JSHandle<EcmaString> string2(string);
    JSHandle<EcmaString> string3 = thread->GetEcmaVM()->GetFactory()->NewFromString("test2");
    string = string3;
    ASSERT_TRUE(string2.GetTaggedValue().GetTaggedObject() != string.GetTaggedValue().GetTaggedObject());
    JSMutableHandle<EcmaString> mutable_string(
        thread, thread->GetEcmaVM()->GetFactory()->NewFromString("test1").GetTaggedValue());
    JSHandle<EcmaString> mutable_string2(mutable_string);
    JSHandle<EcmaString> mutable_string3 = thread->GetEcmaVM()->GetFactory()->NewFromString("test2");
    mutable_string.Update(mutable_string3.GetTaggedValue());
    ASSERT_TRUE(mutable_string.GetTaggedValue().GetTaggedObject() ==
                mutable_string2.GetTaggedValue().GetTaggedObject());
}

HWTEST_F_L0(JSHandleTest, NewHandleScope)
{
    EcmaHandleScope scope(thread);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromString("test");
    for (int32_t i = 10; i > 0; i--) {
        JSHandle<EcmaString>(thread, str.GetTaggedValue());
    }

    for (int i = 5; i > 0; i--) {
        int32_t loop = 2;
        while (loop) {
            ecmascript::EcmaHandleScope scope2(thread);

            JSHandle<EcmaString> str2 = thread->GetEcmaVM()->GetFactory()->NewFromString("test2");
            for (int32_t i = 1024; i > 0; i--) {
                JSHandle<EcmaString>(thread, str.GetTaggedValue());
            }
            str2 = thread->GetEcmaVM()->GetFactory()->NewFromString("test3");
            loop--;
        }
    }
}
}  // namespace panda::test
