/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUG_INTERFACE_H
#define ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUG_INTERFACE_H

#include <string_view>

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tooling/interface/js_pt_location.h"
#include "libpandabase/macros.h"
#include "libpandabase/utils/expected.h"
#include "libpandafile/file.h"

namespace panda::ecmascript::tooling {
struct JSPtStepRange {
    uint32_t startBcOffset {0};
    uint32_t endBcOffset {0};
};

enum PauseReason {
    AMBIGUOUS,
    ASSERT,
    DEBUGCOMMAND,
    DOM,
    EVENTLISTENER,
    EXCEPTION,
    INSTRUMENTATION,
    OOM,
    OTHER,
    PROMISEREJECTION,
    XHR,
    BREAK_ON_START
};

class PtHooks {
public:
    PtHooks() = default;

    /**
     * \brief called by the ecmavm when breakpoint hits. Thread where breakpoint hits is stopped until
     * continue or step event will be received
     * @param thread Identifier of the thread where breakpoint hits. Now the callback is called in the same
     * thread
     * @param location Breakpoint location
     */
    virtual void Breakpoint(const JSPtLocation &location) = 0;

    /**
     * \brief called by the ecmavm when panda file is loaded
     * @param pandaFileName Path to panda file that is loaded
     */
    virtual void LoadModule(std::string_view pandaFileName) = 0;

    /**
     * \brief called by the ecmavm when virtual machine start initialization
     */
    virtual void VmStart() = 0;

    /**
     * \brief called by the ecmavm when virtual machine death
     */
    virtual void VmDeath() = 0;

    virtual void Paused(PauseReason reason) = 0;

    virtual void Exception(const JSPtLocation &location) = 0;

    virtual void SingleStep(const JSPtLocation &location) = 0;

    virtual ~PtHooks() = default;

    NO_COPY_SEMANTIC(PtHooks);
    NO_MOVE_SEMANTIC(PtHooks);
};

class JSDebugInterface {
public:
    JSDebugInterface() = default;

    /**
     * \brief Register debug hooks in the ecmavm
     * @param hooks Pointer to object that implements PtHooks interface
     * @return Error if any errors occur
     */
    virtual void RegisterHooks(PtHooks *hooks) = 0;

    /**
     * \brief Unregister debug hooks in the ecmavm
     * @return Error if any errors occur
     */
    virtual void UnregisterHooks() = 0;

    /**
     * \brief Set breakpoint to \param location with an optional \param condition
     * @param location Breakpoint location
     * @param condition Optional condition
     * @return Error if any errors occur
     */
    virtual bool SetBreakpoint(const JSPtLocation &location, const Local<FunctionRef> &condFuncRef) = 0;

    /**
     * \brief Remove breakpoint from \param location
     * @param location Breakpoint location
     * @return Error if any errors occur
     */
    virtual bool RemoveBreakpoint(const JSPtLocation &location) = 0;

    virtual ~JSDebugInterface() = default;

    NO_COPY_SEMANTIC(JSDebugInterface);
    NO_MOVE_SEMANTIC(JSDebugInterface);
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUG_INTERFACE_H
