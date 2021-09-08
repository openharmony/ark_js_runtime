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

#ifndef ECMASCRIPT_TOOLING_TEST_JS_SINGLE_STEP_TEST_H
#define ECMASCRIPT_TOOLING_TEST_JS_SINGLE_STEP_TEST_H

#include "ecmascript/tooling/test/test_util.h"

namespace panda::tooling::ecmascript::test {
class JsSingleStepTest : public ApiTest {
public:
    JsSingleStepTest()
    {
        vm_start = [this] {
            location_start_ = TestUtil::GetLocation("Sample.js", 4, panda_file_.c_str());
            location_end_ = TestUtil::GetLocation("Sample.js", 7, panda_file_.c_str());
            return true;
        };

        vm_death = [this]() {
            ASSERT_NE(step_count_, 0);
            ASSERT_EQ(breakpoint_count_, 2);
            return true;
        };

        load_module = [this](std::string_view moduleName) {
            if (flag) {
                if (moduleName.find(panda_file_.c_str()) == std::string_view::npos) {
                    return true;
                }
                flag = 0;
                ASSERT_SUCCESS(debug_interface->SetBreakpoint(location_end_));
            }
            return true;
        };

        breakpoint = [this](PtThread, const PtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, location_end_);
            // Check what step signalled before breakpoint
            ASSERT_LOCATION_EQ(location, location_step_);
            ASSERT_TRUE(collect_steps_);
            breakpoint_count_++;
            // Disable collect steps
            collect_steps_ = false;
            return true;
        };

        single_step = [this](PtThread, const PtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            if (!collect_steps_) {
                if (location_start_ == location) {
                    collect_steps_ = true;
                }
                return true;
            }

            ASSERT_NE(bytecode_offset_, location.GetBytecodeOffset());
            location_step_ = location;
            step_count_++;
            bytecode_offset_ = location.GetBytecodeOffset();
            return true;
        };
    }

    std::pair<CString, CString> GetEntryPoint() override
    {
        return {panda_file_, entry_point_};
    }

private:
    CString panda_file_ = "/data/app/Sample.abc";
    CString entry_point_ = "_GLOBAL::func_main_0";
    PtLocation location_start_{nullptr, PtLocation::EntityId(0), 0};
    PtLocation location_end_{nullptr, PtLocation::EntityId(0), 0};
    PtLocation location_step_{nullptr, PtLocation::EntityId(0), 0};
    int32_t step_count_ = 0;
    int32_t breakpoint_count_ = 0;
    bool collect_steps_ = false;
    uint32_t bytecode_offset_ = std::numeric_limits<uint32_t>::max();
    bool flag = 1;
};

std::unique_ptr<ApiTest> GetJsSingleStepTest()
{
    return std::make_unique<JsSingleStepTest>();
}
}  // namespace panda::tooling::ecmascript::test

#endif  // ECMASCRIPT_TOOLING_TEST_JS_SINGLE_STEP_TEST_H
