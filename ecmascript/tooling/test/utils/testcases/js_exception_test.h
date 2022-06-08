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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_EXCEPTION_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_EXCEPTION_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsExceptionTest : public TestEvents {
public:
    JsExceptionTest()
    {
        vmStart = [this] {
            location_ = TestUtil::GetLocation("exception.js", 22, 0, pandaFile_.c_str());
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            return true;
        };

        breakpoint = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, location_);
            ++breakpointCounter_;
            std::vector<std::unique_ptr<CallFrame>> callFrames;
            ASSERT_TRUE(debugger_->GenerateCallFrames(&callFrames));
            ASSERT_TRUE(callFrames.size() > 0);
            auto jsLocation = callFrames[0]->GetLocation();
            ASSERT_TRUE(jsLocation != nullptr);
            ASSERT_EQ(jsLocation->GetLine(), 22);
            ASSERT_EQ(jsLocation->GetColumn(), 0);
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, location);
            return true;
        };

        exception = [this](const JSPtLocation &location) {
            auto sourceLocation = TestUtil::GetSourceLocation(location, pandaFile_.c_str());
            ASSERT_EQ(sourceLocation.line, 17);
            ASSERT_EQ(sourceLocation.column, 27);
            ++exceptionCounter_;
            std::vector<std::unique_ptr<CallFrame>> callFrames;
            ASSERT_TRUE(debugger_->GenerateCallFrames(&callFrames));
            ASSERT_TRUE(callFrames.size() > 0);
            auto jsLocation = callFrames[0]->GetLocation();
            ASSERT_TRUE(jsLocation != nullptr);
            ASSERT_EQ(jsLocation->GetLine(), 17);
            ASSERT_EQ(jsLocation->GetColumn(), 27);
            TestUtil::SuspendUntilContinue(DebugEvent::EXCEPTION, location);
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            if (flag_) {
                if (moduleName != pandaFile_) {
                    return true;
                }
                ASSERT_TRUE(debugger_->NotifyScriptParsed(0, pandaFile_));
                flag_ = false;
                auto condFuncRef = FunctionRef::Undefined(vm_);
                auto ret = debugInterface_->SetBreakpoint(location_, condFuncRef);
                ASSERT_TRUE(ret);
            }
            return true;
        };

        scenario = [this]() {
            ASSERT_BREAKPOINT_SUCCESS(location_);
            TestUtil::Continue();
            TestUtil::WaitForException();
            TestUtil::Continue();
            auto ret = debugInterface_->RemoveBreakpoint(location_);
            ASSERT_TRUE(ret);
            ASSERT_EXITED();
            return true;
        };

        vmDeath = [this]() {
            ASSERT_EQ(breakpointCounter_, 1U);
            ASSERT_EQ(exceptionCounter_, 1U);
            return true;
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }
    ~JsExceptionTest() = default;

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "exception.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
    size_t exceptionCounter_ = 0;
    bool flag_ = true;
};

std::unique_ptr<TestEvents> GetJsExceptionTest()
{
    return std::make_unique<JsExceptionTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_EXCEPTION_TEST_H
