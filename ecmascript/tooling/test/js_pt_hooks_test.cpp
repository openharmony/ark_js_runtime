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

#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/agent/js_pt_hooks.h"
#include "ecmascript/tooling/interface/js_debugger.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/dispatcher.h"
using namespace panda::ecmascript;
using namespace panda::tooling::ecmascript;
using namespace panda::tooling;

namespace panda::test {
class JSPtHooksTest : public testing::Test {
public:
    using EntityId = panda_file::File::EntityId;
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance,   thread,   scope);
        ecmaVm = EcmaVM::Cast(instance);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm,   scope);
    }

protected:
    EcmaVM *ecmaVm {nullptr};
    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSPtHooksTest, BreakpointTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread1(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation1(pandaFile, methodId, bytecodeOffset);
    jspthooks->Breakpoint(ptThread1, ptLocation1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, LoadModuleTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->LoadModule("pandafile/test.abc");
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, PausedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    PauseReason reason1 = PauseReason::EXCEPTION;
    jspthooks->Paused(reason1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExceptionTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread2(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation2(pandaFile, methodId, bytecodeOffset);
    PtReference *ref = nullptr;
    PtObject ptObject1(ref);
    PtLocation ptLocation3(pandaFile, methodId, bytecodeOffset);
    jspthooks->Exception(ptThread2, ptLocation2, ptObject1, ptLocation3);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, SingleStepTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread3(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation4(pandaFile, methodId, bytecodeOffset);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ThreadStartTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread4(id);
    jspthooks->ThreadStart(ptThread4);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ThreadEndTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread5(id);
    jspthooks->ThreadEnd(ptThread5);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, VmStartTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->VmStart();
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, VmInitializationTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread6(id);
    jspthooks->VmInitialization(ptThread6);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, VmDeathTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->VmDeath();
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ClassLoadTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread7(id);
    PtReference *ref = nullptr;
    PtClass ptClass1(ref);
    jspthooks->ClassLoad(ptThread7, ptClass1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ClassPrepareTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread8(id);
    PtReference *ref = nullptr;
    PtClass ptClass2(ref);
    jspthooks->ClassPrepare(ptThread8, ptClass2);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MonitorWaitTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread9(id);
    PtReference *ref = nullptr;
    PtObject ptObject2(ref);
    int64_t timeout(0);
    jspthooks->MonitorWait(ptThread9, ptObject2, timeout);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MonitorWaitedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread10(id);
    PtReference *ref = nullptr;
    PtObject ptObject3(ref);
    bool flag1 = true;
    jspthooks->MonitorWaited(ptThread10, ptObject3, flag1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MonitorContendedEnterTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread11(id);
    PtReference *ref = nullptr;
    PtObject ptObject4(ref);
    jspthooks->MonitorContendedEnter(ptThread11, ptObject4);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MonitorContendedEnteredTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread12(id);
    PtReference *ref = nullptr;
    PtObject ptObject5(ref);
    jspthooks->MonitorContendedEntered(ptThread12, ptObject5);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExceptionCatchTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread13(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation5(pandaFile, methodId, bytecodeOffset);
    PtReference *ref = nullptr;
    PtObject ptObject6(ref);
    jspthooks->ExceptionCatch(ptThread13, ptLocation5, ptObject6);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, PropertyAccessTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread14(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation6(pandaFile, methodId, bytecodeOffset);
    PtReference *ref = nullptr;
    PtObject ptObject7(ref);
    void *data = nullptr;
    PtProperty ptProperty1(data);
    jspthooks->PropertyAccess(ptThread14, ptLocation6, ptObject7, ptProperty1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, PropertyModificationTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread15(id);
    const char *pandaFile = " ";
    EntityId methodId(0);
    uint32_t bytecodeOffset = 0;
    PtLocation ptLocation7(pandaFile, methodId, bytecodeOffset);
    PtReference *ref = nullptr;
    PtObject ptObject8(ref);
    void *data = nullptr;
    PtProperty ptProperty2(data);
    int64_t value = 0;
    PtValueMeta meta = PtValueMeta();
    PtValue ptValue1(value, meta);
    jspthooks->PropertyModification(ptThread15, ptLocation7, ptObject8, ptProperty2, ptValue1);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, FramePopTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread16(id);
    void *data = nullptr;
    PtMethod ptMethod1(data);
    bool flag2 = true;
    jspthooks->FramePop(ptThread16, ptMethod1, flag2);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, GarbageCollectionFinishTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->GarbageCollectionFinish();
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, GarbageCollectionStartTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->GarbageCollectionStart();
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ObjectAllocTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    PtReference *ref = nullptr;
    PtClass ptClass3(ref);
    PtObject ptObject9(ref);
    uint32_t id(0);
    PtThread ptThread17(id);
    size_t size(0);
    jspthooks->ObjectAlloc(ptClass3, ptObject9, ptThread17, size);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MethodEntryTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread17(id);
    void *data = nullptr;
    PtMethod ptMethod2(data);
    jspthooks->MethodEntry(ptThread17, ptMethod2);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, MethodExitTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    uint32_t id(0);
    PtThread ptThread18(id);
    void *data = nullptr;
    PtMethod ptMethod3(data);
    bool flag3 = true;
    int64_t value = 0;
    PtValueMeta meta = PtValueMeta();
    PtValue ptValue2(value, meta);
    jspthooks->MethodExit(ptThread18, ptMethod3, flag3, ptValue2);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExceptionRevokedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->ExceptionRevoked(std::string(), panda_file::File::EntityId());
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExecutionContextCreatedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    struct ExecutionContextWrapper a = {panda_file::File::EntityId(), "origin", "name"};
    jspthooks->ExecutionContextCreated(a);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExecutionContextDestroyedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    struct ExecutionContextWrapper a = {panda_file::File::EntityId(), "origin", "name"};
    jspthooks->ExecutionContextDestroyed(a);
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, ExecutionContextsClearedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    jspthooks->ExecutionContextsCleared();
    ASSERT_NE(jspthooks, nullptr);
}

HWTEST_F_L0(JSPtHooksTest, InspectRequestedTest)
{
    auto backend = std::make_unique<JSBackend>(ecmaVm);
    std::unique_ptr<JSPtHooks>jspthooks = std::make_unique<JSPtHooks>(backend.get());
    PtReference *ref = nullptr;
    PtObject ptObject10(ref);
    PtObject ptObject11(ref);
    jspthooks->InspectRequested(ptObject10, ptObject11);
    ASSERT_NE(jspthooks, nullptr);
}
}