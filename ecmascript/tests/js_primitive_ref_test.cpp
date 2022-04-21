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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSPrimitiveRefTest : public testing::Test {
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

HWTEST_F_L0(JSPrimitiveRefTest, StringCreate)
{
    JSHandle<JSTaggedValue> hello(thread->GetEcmaVM()->GetFactory()->NewFromASCII("hello"));
    JSHandle<JSObject> str(JSPrimitiveRef::StringCreate(thread, hello));

    JSHandle<JSTaggedValue> idx(thread->GetEcmaVM()->GetFactory()->NewFromASCII("0"));
    bool status = JSPrimitiveRef::HasProperty(thread, str, idx);
    ASSERT_TRUE(status);

    PropertyDescriptor desc(thread);
    status = JSPrimitiveRef::GetOwnProperty(thread, str, idx, desc);
    ASSERT_TRUE(status);
    JSHandle<EcmaString> h = thread->GetEcmaVM()->GetFactory()->NewFromASCII("h");
    JSHandle<EcmaString> h2 = JSTaggedValue::ToString(thread, desc.GetValue());
    ASSERT_TRUE(h->Compare(*h2) == 0);
}
}  // namespace panda::test
