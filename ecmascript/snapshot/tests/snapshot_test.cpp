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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot.h"

using namespace panda::ecmascript;

namespace panda::test {
class SnapShotTest : public testing::Test {
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
        JSRuntimeOptions options;
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootIntrinsicSpaces({"ecmascript"});
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetEnableForceGC(true);
        ecmaVm = EcmaVM::Create(options);
        ecmaVm->SetEnableForceGC(true);
        ASSERT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        thread->ClearException();
        ecmaVm->SetEnableForceGC(false);
        delete scope;
        [[maybe_unused]] bool success = EcmaVM::Destroy(ecmaVm);
        ASSERT_TRUE(success) << "Cannot destroy Runtime";
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};
}  // namespace panda::test
