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
#include "ecmascript/js_thread.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript {
using CommonStubCSigns = panda::ecmascript::kungfu::CommonStubCSigns;
using BytecodeStubCSigns = panda::ecmascript::kungfu::BytecodeStubCSigns;
// static
JSThread *JSThread::Create(EcmaVM *vm)
{
    auto jsThread = new JSThread(vm);
    AsmInterParsedOption asmInterOpt = vm->GetJSOptions().GetAsmInterParsedOption();
    if (asmInterOpt.enableAsm) {
        jsThread->EnableAsmInterpreter();
    }

    jsThread->nativeAreaAllocator_ = vm->GetNativeAreaAllocator();
    jsThread->heapRegionAllocator_ = vm->GetHeapRegionAllocator();
    // algin with 16
    jsThread->glueData_.frameBase_ = static_cast<JSTaggedType *>(
        vm->GetNativeAreaAllocator()->Allocate(sizeof(JSTaggedType) * MAX_STACK_SIZE));
    jsThread->glueData_.currentFrame_ = jsThread->glueData_.frameBase_ + MAX_STACK_SIZE;
    EcmaInterpreter::InitStackFrame(jsThread);
    return jsThread;
}

JSThread::JSThread(EcmaVM *vm) : id_(os::thread::GetCurrentThreadId()), vm_(vm)
{
    auto chunk = vm->GetChunk();
    globalStorage_ = chunk->New<EcmaGlobalStorage>(chunk);
    propertiesCache_ = new PropertiesCache();
    vmThreadControl_ = new VmThreadControl();
}

JSThread::~JSThread()
{
    for (auto n : handleStorageNodes_) {
        delete n;
    }
    handleStorageNodes_.clear();
    currentHandleStorageIndex_ = -1;
    handleScopeCount_ = 0;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;
    GetEcmaVM()->GetChunk()->Delete(globalStorage_);

    GetNativeAreaAllocator()->Free(glueData_.frameBase_, sizeof(JSTaggedType) * MAX_STACK_SIZE);
    glueData_.frameBase_ = nullptr;
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;
    if (propertiesCache_ != nullptr) {
        delete propertiesCache_;
        propertiesCache_ = nullptr;
    }
    if (vmThreadControl_ != nullptr) {
        delete vmThreadControl_;
        vmThreadControl_ = nullptr;
    }
}

void JSThread::SetException(JSTaggedValue exception)
{
    glueData_.exception_ = exception;
}

void JSThread::ClearException()
{
    glueData_.exception_ = JSTaggedValue::Hole();
}

JSTaggedValue JSThread::GetCurrentLexenv() const
{
    FrameHandler frameHandler(this);
    return frameHandler.GetEnv();
}

const JSTaggedType *JSThread::GetCurrentFrame() const
{
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    return GetLastLeaveFrame();
#else
    return GetCurrentSPFrame();
#endif
}

void JSThread::SetCurrentFrame(JSTaggedType *sp)
{
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    return SetLastLeaveFrame(sp);
#else
    return SetCurrentSPFrame(sp);
#endif
}

const JSTaggedType *JSThread::GetCurrentInterpretedFrame() const
{
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_RSP_STACK
    auto frameHandler = FrameHandler(this);
    return frameHandler.GetSp();
#else
    return GetCurrentSPFrame();
#endif
}

void JSThread::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (propertiesCache_ != nullptr) {
        propertiesCache_->Clear();
    }

    if (!glueData_.exception_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&glueData_.exception_)));
    }
    if (!coStubCode_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&coStubCode_)));
    }
    if (!bcStubCode_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&bcStubCode_)));
    }
    // visit global Constant
    glueData_.globalConst_.VisitRangeSlot(v1);
    // visit stack roots
    FrameHandler frameHandler(this);
    frameHandler.Iterate(v0, v1);
    // visit tagged handle storage roots
    if (currentHandleStorageIndex_ != -1) {
        int32_t nid = currentHandleStorageIndex_;
        for (int32_t i = 0; i <= nid; ++i) {
            auto node = handleStorageNodes_.at(i);
            auto start = node->data();
            auto end = (i != nid) ? &(node->data()[NODE_BLOCK_SIZE]) : handleScopeStorageNext_;
            v1(ecmascript::Root::ROOT_HANDLE, ObjectSlot(ToUintPtr(start)), ObjectSlot(ToUintPtr(end)));
        }
    }
    globalStorage_->IterateUsageGlobal([v0](EcmaGlobalStorage::Node *node) {
        JSTaggedValue value(node->GetObject());
        if (value.IsHeapObject()) {
            v0(ecmascript::Root::ROOT_HANDLE, ecmascript::ObjectSlot(node->GetObjectAddress()));
        }
    });
}

void JSThread::IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor)
{
    globalStorage_->IterateWeakUsageGlobal([visitor](EcmaGlobalStorage::Node *node) {
        JSTaggedValue value(node->GetObject());
        if (value.IsHeapObject()) {
            auto object = value.GetTaggedObject();
            auto fwd = visitor(object);
            if (fwd == nullptr) {
                // undefind
                node->SetObject(JSTaggedValue::Undefined().GetRawData());
                node->CallWeakCallback();
            } else if (fwd != object) {
                // update
                node->SetObject(JSTaggedValue(fwd).GetRawData());
            }
        }
    });
}

bool JSThread::DoStackOverflowCheck(const JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (UNLIKELY(sp <= glueData_.frameBase_ + RESERVE_STACK_SIZE)) {
        LOG_ECMA(ERROR) << "Stack overflow! Remaining stack size is: " << (sp - glueData_.frameBase_);
        if (LIKELY(!HasPendingException())) {
            ObjectFactory *factory = GetEcmaVM()->GetFactory();
            JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR, "Stack overflow!");
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

uintptr_t *JSThread::ExpandHandleStorage()
{
    uintptr_t *result = nullptr;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
    if (currentHandleStorageIndex_ == lastIndex) {
        auto n = new std::array<JSTaggedType, NODE_BLOCK_SIZE>();
        handleStorageNodes_.push_back(n);
        currentHandleStorageIndex_++;
        result = reinterpret_cast<uintptr_t *>(&n->data()[0]);
        handleScopeStorageEnd_ = &n->data()[NODE_BLOCK_SIZE];
    } else {
        currentHandleStorageIndex_++;
        auto lastNode = handleStorageNodes_[currentHandleStorageIndex_];
        result = reinterpret_cast<uintptr_t *>(&lastNode->data()[0]);
        handleScopeStorageEnd_ = &lastNode->data()[NODE_BLOCK_SIZE];
    }

    return result;
}

void JSThread::ShrinkHandleStorage(int prevIndex)
{
    currentHandleStorageIndex_ = prevIndex;
    int32_t lastIndex = static_cast<int32_t>(handleStorageNodes_.size() - 1);
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(handleScopeStorageEnd_) - ToUintPtr(handleScopeStorageNext_);
    if (memset_s(handleScopeStorageNext_, size, 0, size) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    for (int32_t i = currentHandleStorageIndex_ + 1; i < lastIndex; i++) {
        if (memset_s(handleStorageNodes_[i],
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType), 0,
                     NODE_BLOCK_SIZE * sizeof(JSTaggedType)) !=
                     EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
#endif

    if (lastIndex > MIN_HANDLE_STORAGE_SIZE && currentHandleStorageIndex_ < MIN_HANDLE_STORAGE_SIZE) {
        for (int i = MIN_HANDLE_STORAGE_SIZE; i < lastIndex; i++) {
            auto node = handleStorageNodes_.back();
            delete node;
            handleStorageNodes_.pop_back();
        }
    }
}

void JSThread::NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver)
{
    if (!receiver->GetJSHClass()->IsPrototype()) {
        return;
    }
    if (!stableArrayElementsGuardians_) {
        return;
    }
    auto env = GetEcmaVM()->GetGlobalEnv();
    if (receiver.GetTaggedValue() == env->GetObjectFunctionPrototype().GetTaggedValue() ||
        receiver.GetTaggedValue() == env->GetArrayPrototype().GetTaggedValue()) {
        stableArrayElementsGuardians_ = false;
    }
}

void JSThread::ResetGuardians()
{
    stableArrayElementsGuardians_ = true;
}

void AdjustBCStubAndDebuggerStubEntries(BCStubEntries &bcStubEntries, BCStubEntries &bcDebuggerStubEntries,
    const std::vector<AotCodeInfo::StubDes> &stubs, const AsmInterParsedOption &asmInterOpt)
{
    auto defaultBCStubDes = stubs[BytecodeStubCSigns::SingleStepDebugging];
    auto defaultNonexistentBCStubDes = stubs[BytecodeStubCSigns::HandleOverflow];
    auto defaultBCDebuggerStubDes = stubs[BytecodeStubCSigns::BCDebuggerEntry];
    auto defaultBCDebuggerExceptionStubDes = stubs[BytecodeStubCSigns::BCDebuggerExceptionEntry];
    bcStubEntries.SetUnrealizedBCHandlerStubEntries(defaultBCStubDes.codeAddr_);
    bcStubEntries.SetNonexistentBCHandlerStubEntries(defaultNonexistentBCStubDes.codeAddr_);
#define UNDEF_STUB(name)                                                                               \
    bcStubEntries.Set(BytecodeStubCSigns::ID_##name, defaultBCStubDes.codeAddr_);
    INTERPRETER_IGNORED_BC_STUB_LIST(UNDEF_STUB)
#undef UNDEF_STUB
    for (int i = asmInterOpt.handleStart; i <= asmInterOpt.handleEnd && i >= 0; i++) {
        bcStubEntries.Set(static_cast<size_t>(i), defaultBCStubDes.codeAddr_);
    }
    // bc debugger stub entries
    bcDebuggerStubEntries.SetNonexistentBCHandlerStubEntries(defaultNonexistentBCStubDes.codeAddr_);
    for (size_t i = 0; i < BCStubEntries::EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
        if (i == BytecodeStubCSigns::ID_ExceptionHandler) {
            bcDebuggerStubEntries.Set(i, defaultBCDebuggerExceptionStubDes.codeAddr_);
            continue;
        }
        bcDebuggerStubEntries.Set(i, defaultBCDebuggerStubDes.codeAddr_);
    }
}

void JSThread::LoadCommonStubsFromFile(std::string &fileName)
{
    AotCodeInfo aotInfo;
    if (!aotInfo.DeserializeForStub(this, fileName)) {
        return;
    }
    auto stubs = aotInfo.GetStubs();
    for (size_t i = 0; i < stubs.size(); i++) {
        auto des = stubs[i];
        if (des.IsCommonStub()) {
            glueData_.coStubEntries_.Set(des.indexInKind_, des.codeAddr_);
        }
    }
    coStubCode_ = aotInfo.GetCode().GetTaggedValue();
}

void JSThread::LoadBytecodeHandlerStubsFromFile(std::string &fileName)
{
    AotCodeInfo aotInfo;
    if (!aotInfo.DeserializeForStub(this, fileName)) {
        return;
    }
    auto stubs = aotInfo.GetStubs();
    for (size_t i = 0; i < stubs.size(); i++) {
        auto des = stubs[i];
        if (des.IsBCStub()) {
            // bc helper handler use to adjust bc stub, not init bc stub
            if (des.IsBCHandlerStub()) {
                glueData_.bcStubEntries_.Set(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
                std::cout << "bytecode: " << GetEcmaOpcodeStr(static_cast<EcmaOpcode>(des.indexInKind_))
                    << " addr:" << des.codeAddr_ << std::endl;
#endif
            }
        } else {
            glueData_.rtStubEntries_.Set(des.indexInKind_, des.codeAddr_);
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
                std::cout << "runtime index: " << des.indexInKind_ << " addr:" << des.codeAddr_ << std::endl;
#endif
        }
    }
    AsmInterParsedOption asmInterOpt = GetEcmaVM()->GetJSOptions().GetAsmInterParsedOption();
    AdjustBCStubAndDebuggerStubEntries(glueData_.bcStubEntries_, glueData_.bcDebuggerStubEntries_, stubs, asmInterOpt);
    bcStubCode_ = aotInfo.GetCode().GetTaggedValue();
}

void JSThread::CheckSwitchDebuggerBCStub()
{
    auto isDebug = GetEcmaVM()->GetJsDebuggerManager()->IsDebugMode();
    if (isDebug &&
        glueData_.bcDebuggerStubEntries_.Get(0) == glueData_.bcDebuggerStubEntries_.Get(1)) {
        for (size_t i = 0; i < BCStubEntries::BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
            auto stubEntry = glueData_.bcStubEntries_.Get(i);
            auto debuggerStubEbtry = glueData_.bcDebuggerStubEntries_.Get(i);
            glueData_.bcDebuggerStubEntries_.Set(i, stubEntry);
            glueData_.bcStubEntries_.Set(i, debuggerStubEbtry);
        }
    } else if (!isDebug &&
        glueData_.bcStubEntries_.Get(0) == glueData_.bcStubEntries_.Get(1)) {
        for (size_t i = 0; i < BCStubEntries::BC_HANDLER_STUB_ENTRIES_COUNT; i++) {
            auto stubEntry = glueData_.bcDebuggerStubEntries_.Get(i);
            auto debuggerStubEbtry = glueData_.bcStubEntries_.Get(i);
            glueData_.bcStubEntries_.Set(i, stubEntry);
            glueData_.bcDebuggerStubEntries_.Set(i, debuggerStubEbtry);
        }
    }
}

bool JSThread::CheckSafepoint() const
{
    if (vmThreadControl_->VMNeedSuspension()) {
        vmThreadControl_->SuspendVM();
    }
#ifndef NDEBUG
    GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    return true;
#endif
    if (IsMarkFinished()) {
        auto heap = GetEcmaVM()->GetHeap();
        heap->GetConcurrentMarker()->HandleMarkingFinished();
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
