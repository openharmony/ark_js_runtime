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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_THROW_EXCEPTION_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_THROW_EXCEPTION_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsThrowExceptionTest : public TestEvents {
public:
    JsThrowExceptionTest()
    {
        vmStart = [this] {
            location_ = TestUtil::GetLocation("throwException.js", 28, 0, pandaFile_.c_str()); // 28: breakpointer line
            std::cout<<"vmStart1"<<std::endl;
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            std::cout<<"vmStart2"<<std::endl;
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
            ASSERT_EQ(jsLocation->GetLine(), 28); // 28: breakpointer line
            ASSERT_EQ(jsLocation->GetColumn(), 0);
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, location);
            return true;
        };

        exception = [this](const JSPtLocation &location) {
            auto sourceLocation = TestUtil::GetSourceLocation(location, pandaFile_.c_str());
            ASSERT_EQ(sourceLocation.line, 19); // 19: exception line
            ASSERT_EQ(sourceLocation.column, 8); // 8: exception column
            ++exceptionCounter_;
            std::vector<std::unique_ptr<CallFrame>> callFrames;
            ASSERT_TRUE(debugger_->GenerateCallFrames(&callFrames));
            ASSERT_TRUE(callFrames.size() > 0);
            auto jsLocation = callFrames[0]->GetLocation();
            ASSERT_TRUE(jsLocation != nullptr);
            ASSERT_EQ(jsLocation->GetLine(), 19); // 19: exception line
            ASSERT_EQ(jsLocation->GetColumn(), 8); // 8: exception column
            ++exceptionCounter_;
            TestUtil::SuspendUntilContinue(DebugEvent::EXCEPTION, location);
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            ASSERT_EQ(moduleName, pandaFile_);
            ASSERT_TRUE(debugger_->NotifyScriptParsed(0, pandaFile_));
            auto condFuncRef = FunctionRef::Undefined(vm_);
            auto ret = debugInterface_->SetBreakpoint(location_, condFuncRef);
            ASSERT_TRUE(ret);
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
            ASSERT_EQ(exceptionCounter_, 2U);
            return true;
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }
    ~JsThrowExceptionTest() = default;

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "throwException.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation location_ {nullptr, JSPtLocation::EntityId(0), 0};
    size_t breakpointCounter_ = 0;
    size_t exceptionCounter_ = 0;
};

std::unique_ptr<TestEvents> GetJsThrowExceptionTest()
{
    return std::make_unique<JsThrowExceptionTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_THROW_EXCEPTION_TEST_H
