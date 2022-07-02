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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsStepIntoTest : public TestEvents {
public:
    JsStepIntoTest()
    {
        vmStart = [this] {
            location1_ = TestUtil::GetLocation("StepInto.js", 28, 0, pandaFile_.c_str());  // 28: line number
            location2_ = TestUtil::GetLocation("StepInto.js", 16, 0, pandaFile_.c_str());  // 16: line number
            return true;
        };

        vmDeath = [this]() {
            ASSERT_EQ(breakpointCounter_, 1);
            ASSERT_EQ(stepCompleteCounter_, 1);
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            ASSERT_EQ(moduleName, pandaFile_);
            debugger_->NotifyScriptParsed(0, moduleName.data());
            auto condFuncRef = FunctionRef::Undefined(vm_);
            auto ret = debugInterface_->SetBreakpoint(location1_, condFuncRef);
            ASSERT_TRUE(ret);
            return true;
        };

        breakpoint = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, location1_);
            ++breakpointCounter_;
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, location);
            debugger_->StepInto(StepIntoParams());
            return true;
        };

        singleStep = [this](const JSPtLocation &location) {
            if (debugger_->NotifySingleStep(location)) {
                ASSERT_TRUE(location.GetMethodId().IsValid());
                ASSERT_LOCATION_EQ(location, location2_);
                stepCompleteCounter_++;
                TestUtil::Event(DebugEvent::STEP_COMPLETE);
                return true;
            }
            return false;
        };

        scenario = [this]() {
            ASSERT_BREAKPOINT_SUCCESS(location1_);
            TestUtil::Continue();
            auto ret = debugInterface_->RemoveBreakpoint(location1_);
            TestUtil::WaitForStepComplete();
            ASSERT_TRUE(ret);
            ASSERT_EXITED();
            return true;
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "StepInto.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location1_ {nullptr, JSPtLocation::EntityId(0), 0};
    JSPtLocation location2_ {nullptr, JSPtLocation::EntityId(0), 0};
    int32_t breakpointCounter_ = 0;
    int32_t stepCompleteCounter_ = 0;
};

std::unique_ptr<TestEvents> GetJsStepIntoTest()
{
    return std::make_unique<JsStepIntoTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_STEP_INTO_TEST_H
