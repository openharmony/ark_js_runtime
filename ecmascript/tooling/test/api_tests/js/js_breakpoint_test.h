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

#ifndef ECMASCRIPT_TOOLING_TEST_JS_BREAKPOINT_TEST_H
#define ECMASCRIPT_TOOLING_TEST_JS_BREAKPOINT_TEST_H

#include "ecmascript/tooling/test/test_util.h"
#include "ecmascript/mem/c_string.h"

namespace panda::tooling::ecmascript::test {
class JsBreakpointTest : public ApiTest {
public:
    JsBreakpointTest()
    {
        vm_start = [this] {
            location_ = TestUtil::GetLocation("Sample.js", 7, panda_file_.c_str());
            location_.GetPandaFile();
            ASSERT_TRUE(location_.GetMethodId().IsValid());
            return true;
        };

        breakpoint = [this](PtThread ecmaVm, const PtLocation &location) {
            ASSERT_TRUE(location.GetMethodId().IsValid());
            ASSERT_LOCATION_EQ(location, location_);
            ++breakpoint_counter_;
            TestUtil::SuspendUntilContinue(DebugEvent::BREAKPOINT, ecmaVm, location);
            return true;
        };

        load_module = [this](std::string_view moduleName) {
            if (flag_) {
                if (moduleName.find(panda_file_.c_str()) == std::string_view::npos) {
                    return true;
                }
                ASSERT_TRUE(backend->NotifyScriptParsed(0, panda_file_));
                flag_ = 0;
                location_.GetPandaFile();
                ASSERT_SUCCESS(debug_interface->SetBreakpoint(location_));
                auto error = debug_interface->SetBreakpoint(location_);
                ASSERT_FALSE(!error);
            }
            return true;
        };

        scenario = [this]() {
            ASSERT_BREAKPOINT_SUCCESS(location_);
            ASSERT_SUCCESS(debug_interface->RemoveBreakpoint(location_));
            TestUtil::Continue();
            ASSERT_EXITED();
            return true;
        };

        vm_death = [this]() {
            ASSERT_EQ(breakpoint_counter_, 1U);
            return true;
        };
    }

    std::pair<CString, CString> GetEntryPoint() override
    {
        return {panda_file_, entry_point_};
    }
    ~JsBreakpointTest() = default;
private:
    CString panda_file_ = "/data/app/Sample.abc";
    CString entry_point_ = "_GLOBAL::func_main_0";
    PtLocation location_ {nullptr, PtLocation::EntityId(0), 0};
    size_t breakpoint_counter_ = 0;
    bool flag_ = 1;
};

std::unique_ptr<ApiTest> GetJsBreakpointTest()
{
    return std::make_unique<JsBreakpointTest>();
}
}  // namespace panda::tooling::ecmascript::test

#endif  // ECMASCRIPT_TOOLING_TEST_JS_BREAKPOINT_TEST_H
