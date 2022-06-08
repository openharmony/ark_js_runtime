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

#include "ecmascript/tooling/test/utils/test_entry.h"

#include <thread>

#include "ecmascript/tooling/test/utils/testcases/test_list.h"
#include "ecmascript/tooling/test/utils/test_hooks.h"

namespace panda::ecmascript::tooling::test {
static std::thread g_debuggerThread;
static std::unique_ptr<TestHooks> g_hooks = nullptr;

bool StartDebuggerImpl([[maybe_unused]] const std::string &name, EcmaVM *vm, [[maybe_unused]] bool isDebugMode)
{
    std::string testName = GetCurrentTestName();
    g_hooks = std::make_unique<TestHooks>(testName, vm);
    g_debuggerThread = std::thread([] {
        TestUtil::WaitForInit();
        g_hooks->Run();
    });
    return true;
}

bool StopDebuggerImpl([[maybe_unused]] const std::string &name)
{
    g_hooks->TerminateTest();
    g_debuggerThread.join();
    g_hooks.reset();
    return true;
}
}  // namespace panda::ecmascript::tooling::test
