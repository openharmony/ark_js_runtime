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

#ifndef ECMASCRIPT_TOOLING_BACKEND_JS_PT_HOOKS_H
#define ECMASCRIPT_TOOLING_BACKEND_JS_PT_HOOKS_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/backend/js_pt_extractor.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_script.h"
#include "ecmascript/tooling/backend/js_debugger_interface.h"

namespace panda::ecmascript::tooling {
class DebuggerImpl;

class JSPtHooks : public PtHooks {
public:
    explicit JSPtHooks(DebuggerImpl *debugger) : debugger_(debugger) {}
    ~JSPtHooks() override = default;

    void Breakpoint(const JSPtLocation &location) override;
    void LoadModule(std::string_view pandaFileName) override;
    void Exception(const JSPtLocation &location) override;
    bool SingleStep(const JSPtLocation &location) override;
    void PendingJobEntry() override;
    void VmStart() override {}
    void VmDeath() override {}

private:
    NO_COPY_SEMANTIC(JSPtHooks);
    NO_MOVE_SEMANTIC(JSPtHooks);

    DebuggerImpl *debugger_ {nullptr};
    bool firstTime_ {true};
};
}  // namespace panda::ecmascript::tooling
#endif  // ECMASCRIPT_TOOLING_BACKEND_JS_PT_HOOKS_H