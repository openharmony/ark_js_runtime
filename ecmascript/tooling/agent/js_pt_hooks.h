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

#ifndef ECMASCRIPT_TOOLING_AGENT_JS_PT_HOOKS_H
#define ECMASCRIPT_TOOLING_AGENT_JS_PT_HOOKS_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/pt_js_extractor.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_script.h"
#include "include/tooling/debug_interface.h"

namespace panda::tooling::ecmascript {
class JSBackend;

class JSPtHooks : public PtHooks {
public:
    explicit JSPtHooks(JSBackend *backend) : backend_(backend) {}
    ~JSPtHooks() override = default;

    void Breakpoint(PtThread thread, const PtLocation &location) override;
    void LoadModule(std::string_view pandaFileName) override;
    void Paused(PauseReason reason) override;
    void Exception(PtThread thread, const PtLocation &location, PtObject exceptionObject,
                   const PtLocation &catchLocation) override;
    void SingleStep(PtThread thread, const PtLocation &location) override;

    void ThreadStart(PtThread) override {}
    void ThreadEnd(PtThread) override {}
    void VmStart() override {}
    void VmInitialization(PtThread) override {}
    void VmDeath() override {}
    void ClassLoad(PtThread, PtClass) override {}
    void ClassPrepare(PtThread, PtClass) override {}
    void MonitorWait(PtThread, PtObject, int64_t) override {}
    void MonitorWaited(PtThread, PtObject, bool) override {}
    void MonitorContendedEnter(PtThread, PtObject) override {}
    void MonitorContendedEntered(PtThread, PtObject) override {}
    void ExceptionCatch(PtThread, const PtLocation &, PtObject) override {}
    void PropertyAccess(PtThread, const PtLocation &, PtObject, PtProperty) override {}
    void PropertyModification(PtThread, const PtLocation &, PtObject, PtProperty, PtValue) override {}
    void FramePop(PtThread, PtMethod, bool) override {}
    void GarbageCollectionFinish() override {}
    void GarbageCollectionStart() override {}
    void ObjectAlloc(PtClass, PtObject, PtThread, size_t) override {}
    void MethodEntry(PtThread, PtMethod) override {}
    void MethodExit(PtThread, PtMethod, bool, PtValue) override {}
    void ExceptionRevoked(ExceptionWrapper, ExceptionID) override {}
    void ExecutionContextCreated(ExecutionContextWrapper) override {}
    void ExecutionContextDestroyed(ExecutionContextWrapper) override {}
    void ExecutionContextsCleared() override {}
    void InspectRequested(PtObject, PtObject) override {}

private:
    NO_COPY_SEMANTIC(JSPtHooks);
    NO_MOVE_SEMANTIC(JSPtHooks);

    JSBackend *backend_{nullptr};
    bool firstTime_ {true};
};
}  // namespace panda::tooling::ecmascript
#endif