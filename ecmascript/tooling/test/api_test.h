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

#ifndef ECMASCRIPT_TOOLING_TEST_API_TEST_H
#define ECMASCRIPT_TOOLING_TEST_API_TEST_H

#include <utility>
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/agent/js_pt_hooks.h"

namespace panda::tooling::ecmascript::test {
using BreakpointCallback = std::function<bool(PtThread, const PtLocation &)>;
using LoadModuleCallback = std::function<bool(std::string_view)>;
using PausedCallback = std::function<bool(PauseReason)>;
using ExceptionCallback = std::function<bool(PtThread, const PtLocation &)>;
using SingleStepCallback = std::function<bool(PtThread, const PtLocation &)>;
using VmStartCallback = std::function<bool()>;
using VmInitializationCallback = std::function<bool()>;
using VmDeathCallback = std::function<bool()>;
using MethodEntryCallback = std::function<bool(PtThread, PtMethod)>;
using Scenario = std::function<bool()>;

enum class DebugEvent {
    BREAKPOINT,
    LOAD_MODULE,
    PAUSED,
    EXCEPTION,
    METHOD_ENTRY,
    SINGLE_STEP,
    VM_START,
    VM_INITIALIZATION,
    VM_DEATH,
    UNINITIALIZED
};

std::ostream &operator<<(std::ostream &out, DebugEvent value);

struct ApiTest {
    BreakpointCallback breakpoint;
    LoadModuleCallback load_module;
    PausedCallback paused;
    ExceptionCallback exception;
    MethodEntryCallback method_entry;
    SingleStepCallback single_step;
    VmStartCallback vm_start;
    VmInitializationCallback vm_init;
    VmDeathCallback vm_death;

    Scenario scenario;
    JSDebugger *debug_interface {nullptr};
    JSBackend *backend {nullptr};
    ApiTest();
    virtual ~ApiTest() = default;

    virtual std::pair<CString, CString> GetEntryPoint() = 0;
};
}  // namespace panda::tooling::ecmascript::test

#endif  // ECMASCRIPT_TOOLING_TEST_API_TEST_H
