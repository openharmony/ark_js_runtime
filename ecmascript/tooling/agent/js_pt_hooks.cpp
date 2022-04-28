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

#include "ecmascript/tooling/agent/js_pt_hooks.h"
#include "ecmascript/tooling/agent/js_backend.h"

namespace panda::ecmascript::tooling {
void JSPtHooks::Breakpoint(const JSPtLocation &location)
{
    LOG(DEBUG, DEBUGGER) << "JSPtHooks: Breakpoint => " << location.GetMethodId() << ": "
                         << location.GetBytecodeOffset();

    [[maybe_unused]] LocalScope scope(backend_->ecmaVm_);
    backend_->NotifyPaused(location, INSTRUMENTATION);
}

void JSPtHooks::Paused(PauseReason reason)
{
    LOG(DEBUG, DEBUGGER) << "JSPtHooks: Paused";

    [[maybe_unused]] LocalScope scope(backend_->ecmaVm_);
    backend_->NotifyPaused({}, reason);
}

void JSPtHooks::Exception([[maybe_unused]] const JSPtLocation &location)
{
    LOG(DEBUG, DEBUGGER) << "JSPtHooks: Exception";
    [[maybe_unused]] LocalScope scope(backend_->ecmaVm_);

    backend_->NotifyPaused({}, EXCEPTION);
}

bool JSPtHooks::SingleStep(const JSPtLocation &location)
{
    LOG(DEBUG, DEBUGGER) << "JSPtHooks: SingleStep => " << location.GetBytecodeOffset();

    [[maybe_unused]] LocalScope scope(backend_->ecmaVm_);
    if (firstTime_) {
        firstTime_ = false;

        backend_->NotifyPaused({}, BREAK_ON_START);
        return true;
    }

    // pause or step complete
    if (backend_->StepComplete(location)) {
        backend_->NotifyPaused({}, OTHER);
        return true;
    }
    return false;
}

void JSPtHooks::LoadModule(std::string_view pandaFileName)
{
    LOG(INFO, DEBUGGER) << "JSPtHooks: LoadModule: " << pandaFileName;

    [[maybe_unused]] LocalScope scope(backend_->ecmaVm_);

    static uint32_t scriptId = 0;
    if (backend_->NotifyScriptParsed(scriptId++, pandaFileName.data())) {
        firstTime_ = true;
    }
}
}  // namespace panda::ecmascript::tooling
