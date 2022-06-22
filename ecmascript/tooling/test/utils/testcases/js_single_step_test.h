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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_SINGLE_STEP_TEST_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_SINGLE_STEP_TEST_H

#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class JsSingleStepTest : public TestEvents {
public:
    JsSingleStepTest()
    {
        vmStart = [this] {
            locationStart_ = TestUtil::GetLocation("Sample.js", 19, 0, pandaFile_.c_str());
            locationEnd_ = TestUtil::GetLocation("Sample.js", 22, 0, pandaFile_.c_str());
            return true;
        };

        vmDeath = [this]() {
            ASSERT_NE(stepCounter_, 0);
            ASSERT_EQ(breakpointCounter_, 2);
            return true;
        };

        loadModule = [this](std::string_view moduleName) {
            if (flag_) {
                if (moduleName != pandaFile_) {
                    return true;
                }
                flag_ = false;
                auto condFuncRef = FunctionRef::Undefined(vm_);
                auto ret = debugInterface_->SetBreakpoint(locationEnd_, condFuncRef);
                ASSERT_TRUE(ret);
            }
            return true;
        };

        breakpoint = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, locationEnd_);
            // Check what step signalled before breakpoint
            ASSERT_LOCATION_EQ(location, locationStep_);
            ASSERT_TRUE(collectSteps_);
            breakpointCounter_++;
            // Disable collect steps
            collectSteps_ = false;
            return true;
        };

        singleStep = [this](const JSPtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            if (!collectSteps_) {
                if (locationStart_ == location) {
                    collectSteps_ = true;
                }
                return false;
            }

            ASSERT_NE(bytecodeOffset_, location.GetBytecodeOffset());
            locationStep_ = location;
            stepCounter_++;
            bytecodeOffset_ = location.GetBytecodeOffset();
            return false;
        };
    }

    std::pair<std::string, std::string> GetEntryPoint() override
    {
        return {pandaFile_, entryPoint_};
    }

private:
    std::string pandaFile_ = DEBUGGER_ABC_DIR "Sample.abc";
    std::string entryPoint_ = "_GLOBAL::func_main_0";
    JSPtLocation locationStart_ {nullptr, JSPtLocation::EntityId(0), 0};
    JSPtLocation locationEnd_ {nullptr, JSPtLocation::EntityId(0), 0};
    JSPtLocation locationStep_ {nullptr, JSPtLocation::EntityId(0), 0};
    int32_t stepCounter_ = 0;
    int32_t breakpointCounter_ = 0;
    bool collectSteps_ = false;
    uint32_t bytecodeOffset_ = std::numeric_limits<uint32_t>::max();
    bool flag_ = true;
};

std::unique_ptr<TestEvents> GetJsSingleStepTest()
{
    return std::make_unique<JsSingleStepTest>();
}
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TESTCASES_JS_SINGLE_STEP_TEST_H
