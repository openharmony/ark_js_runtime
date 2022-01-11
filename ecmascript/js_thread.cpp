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
#include "ecmascript/ic/properties_cache-inl.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/stub_module.h"
#include "include/panda_vm.h"

namespace panda::ecmascript {
// static
JSThread *JSThread::Create(Runtime *runtime, PandaVM *vm)
{
    auto jsThread = new JSThread(runtime, vm);
    jsThread->regionFactory_ = EcmaVM::Cast(vm)->GetRegionFactory();
    // algin with 16
    jsThread->frameBase_ = static_cast<JSTaggedType *>(
        EcmaVM::Cast(vm)->GetRegionFactory()->Allocate(sizeof(JSTaggedType) * MAX_STACK_SIZE));
    jsThread->currentFrame_ = jsThread->frameBase_ + MAX_STACK_SIZE;
    JSThread::SetCurrent(jsThread);
    EcmaInterpreter::InitStackFrame(jsThread);
    return jsThread;
}

JSThread::JSThread(Runtime *runtime, PandaVM *vm)
    : ManagedThread(GetCurrentThreadId(), runtime->GetInternalAllocator(), vm,
                    Thread::ThreadType::THREAD_TYPE_MANAGED)
{
    SetLanguageContext(runtime->GetLanguageContext(panda_file::SourceLang::ECMASCRIPT));
    auto chunk = EcmaVM::Cast(vm)->GetChunk();
    globalStorage_ = chunk->New<EcmaGlobalStorage>(chunk);
    internalCallParams_ = new InternalCallParams();
    propertiesCache_ = new PropertiesCache();
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
    EcmaVM::Cast(GetVM())->GetChunk()->Delete(globalStorage_);

    GetRegionFactory()->Free(frameBase_, sizeof(JSTaggedType) * MAX_STACK_SIZE);
    frameBase_ = nullptr;
    regionFactory_ = nullptr;
    delete internalCallParams_;
    internalCallParams_ = nullptr;
    delete propertiesCache_;
    propertiesCache_ = nullptr;
}

EcmaVM *JSThread::GetEcmaVM() const
{
    return EcmaVM::Cast(GetVM());
}

void JSThread::SetException(JSTaggedValue exception)
{
    exception_ = exception;
}

void JSThread::ClearException()
{
    exception_ = JSTaggedValue::Hole();
}

JSTaggedValue JSThread::GetCurrentLexenv() const
{
    return InterpretedFrameHandler(currentFrame_).GetEnv();
}

void JSThread::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (propertiesCache_ != nullptr) {
        propertiesCache_->Clear();
    }

    if (!exception_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&exception_)));
    }
    if (!stubCode_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&stubCode_)));
    }
    // visit global Constant
    globalConst_.VisitRangeSlot(v1);
    // visit stack roots
    FrameIterator iterator(currentFrame_, this);
    iterator.Iterate(v0, v1);
    // visit internal call params；
    internalCallParams_->Iterate(v1);
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
    if (UNLIKELY(sp <= frameBase_ + RESERVE_STACK_SIZE)) {
        ObjectFactory *factory = GetEcmaVM()->GetFactory();
        JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR, "Stack overflow!");
        if (LIKELY(!HasPendingException())) {
            SetException(error.GetTaggedValue());
        }
        return true;
    }
    return false;
}

uintptr_t *JSThread::ExpandHandleStorage()
{
    uintptr_t *result = nullptr;
    int32_t lastIndex = handleStorageNodes_.size() - 1;
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
    int32_t lastIndex = handleStorageNodes_.size() - 1;
#if ECMASCRIPT_ENABLE_ZAP_MEM
    uintptr_t size = ToUintPtr(handleScopeStorageEnd_) - ToUintPtr(handleScopeStorageNext_);
    memset_s(handleScopeStorageNext_, size, 0, size);
    for (int32_t i = currentHandleStorageIndex_ + 1; i < lastIndex; i++) {
        memset_s(handleStorageNodes_[i], NODE_BLOCK_SIZE * sizeof(JSTaggedType), 0,
                 NODE_BLOCK_SIZE * sizeof(JSTaggedType));
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

void JSThread::LoadStubModule(const char *moduleFile)
{
    StubModule stubModule;
    std::string fileName(moduleFile);
    stubModule.Load(this, fileName);
    for (uint32_t i = 0; i < kungfu::FAST_STUB_MAXCOUNT; i++) {
        fastStubEntries_[i] = stubModule.GetStubEntry(i);
    }
    for (uint32_t i = 0; i < MAX_BYTECODE_HANDLERS; i++) {
        bytecodeHandlers_[i] = stubModule.GetStubEntry(kungfu::StubId::STUB_SingleStepDebugging);
    }
    bytecodeHandlers_[EcmaOpcode::LDNAN_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdnanPref);
    bytecodeHandlers_[EcmaOpcode::LDINFINITY_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdInfinityPref);
    bytecodeHandlers_[EcmaOpcode::LDUNDEFINED_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdUndefinedPref);
    bytecodeHandlers_[EcmaOpcode::LDNULL_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdNullPref);
    bytecodeHandlers_[EcmaOpcode::LDTRUE_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdTruePref);
    bytecodeHandlers_[EcmaOpcode::LDFALSE_PREF] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdFalsePref);
    bytecodeHandlers_[EcmaOpcode::LDA_DYN_V8] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdaDyn);
    bytecodeHandlers_[EcmaOpcode::STA_DYN_V8] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStaDyn);
    bytecodeHandlers_[EcmaOpcode::LDLEXVARDYN_PREF_IMM4_IMM4] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdLexVarDynPrefImm4Imm4);
    bytecodeHandlers_[EcmaOpcode::LDLEXVARDYN_PREF_IMM8_IMM8] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdLexVarDynPrefImm8Imm8);
    bytecodeHandlers_[EcmaOpcode::LDLEXVARDYN_PREF_IMM16_IMM16] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleLdLexVarDynPrefImm16Imm16);
    bytecodeHandlers_[EcmaOpcode::STLEXVARDYN_PREF_IMM4_IMM4_V8] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStLexVarDynPrefImm4Imm4V8);
    bytecodeHandlers_[EcmaOpcode::STLEXVARDYN_PREF_IMM8_IMM8_V8] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStLexVarDynPrefImm8Imm8V8);
    bytecodeHandlers_[EcmaOpcode::STLEXVARDYN_PREF_IMM16_IMM16_V8] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStLexVarDynPrefImm16Imm16V8);
    bytecodeHandlers_[EcmaOpcode::INCDYN_PREF_V8] = stubModule.GetStubEntry(kungfu::StubId::STUB_HandleIncdynPrefV8);
    bytecodeHandlers_[EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStConstToGlobalRecordPrefId32);
    bytecodeHandlers_[EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStLetToGlobalRecordPrefId32);
    bytecodeHandlers_[EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32] =
        stubModule.GetStubEntry(kungfu::StubId::STUB_HandleStClassToGlobalRecordPrefId32);
#ifdef NDEBUG
    kungfu::LLVMStackMapParser::GetInstance().Print();
#endif
    stubCode_ = stubModule.GetCode();
}

bool JSThread::CheckSafepoint() const
{
#ifndef NDEBUG
    EcmaVM::Cast(GetVM())->CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);
    return true;
#endif
    if (IsMarkFinished()) {
        auto heap = EcmaVM::Cast(GetVM())->GetHeap();
        heap->GetConcurrentMarker()->HandleMarkFinished();
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
