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

#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_thread.h"
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
    : ManagedThread(NON_INITIALIZED_THREAD_ID, runtime->GetInternalAllocator(), vm,
                    Thread::ThreadType::THREAD_TYPE_MANAGED)
{
    SetLanguageContext(runtime->GetLanguageContext(panda_file::SourceLang::ECMASCRIPT));
    auto chunk = EcmaVM::Cast(vm)->GetChunk();
    globalStorage_ = chunk->New<EcmaGlobalStorage>(chunk);
    internalCallParams_ = new InternalCallParams();
}

JSThread::~JSThread()
{
    for (auto n : handleStorageNodes_) {
        delete n;
    }
    handleStorageNodes_.clear();
    currentHandleStorageIndex_ = -1;
    handleScopeStorageNext_ = handleScopeStorageEnd_ = nullptr;
    EcmaVM::Cast(GetVM())->GetChunk()->Delete(globalStorage_);

    GetRegionFactory()->Free(frameBase_, sizeof(JSTaggedType) * MAX_STACK_SIZE);
    frameBase_ = nullptr;
    regionFactory_ = nullptr;
    delete internalCallParams_;
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
    return EcmaFrameHandler(currentFrame_).GetEnv();
}

void JSThread::Iterate(const RootVisitor &v0, const RootRangeVisitor &v1)
{
    if (!exception_.IsHole()) {
        v0(Root::ROOT_VM, ObjectSlot(ToUintPtr(&exception_)));
    }

    // visit global Constant
    globalConst_.Visitor(v1);
    // visit stack roots
    EcmaFrameHandler(currentFrame_).Iterate(v0, v1);
    // visit internal call params
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
    // visit global handle storage roots
    if (globalStorage_->GetNodes()->empty()) {
        return;
    }
    for (size_t i = 0; i < globalStorage_->GetNodes()->size(); i++) {
        auto block = globalStorage_->GetNodes()->at(i);
        auto size = EcmaGlobalStorage::GLOBAL_BLOCK_SIZE;
        if (i == globalStorage_->GetNodes()->size() - 1) {
            size = globalStorage_->GetCount();
        }

        for (auto j = 0; j < size; j++) {
            JSTaggedValue value(block->at(j).GetObject());
            if (value.IsHeapObject()) {
                v0(ecmascript::Root::ROOT_HANDLE, ecmascript::ObjectSlot(block->at(j).GetObjectAddress()));
            }
        }
    }
}

void JSThread::IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor)
{
    if (globalStorage_->GetWeakNodes()->empty()) {
        return;
    }
    for (size_t i = 0; i < globalStorage_->GetWeakNodes()->size(); i++) {
        auto block = globalStorage_->GetWeakNodes()->at(i);
        auto size = EcmaGlobalStorage::GLOBAL_BLOCK_SIZE;
        if (i == globalStorage_->GetWeakNodes()->size() - 1) {
            size = globalStorage_->GetWeakCount();
        }

        for (auto j = 0; j < size; j++) {
            JSTaggedValue value(block->at(j).GetObject());
            if (value.IsHeapObject()) {
                auto object = value.GetTaggedObject();
                auto fwd = visitor(object);
                if (fwd == nullptr) {
                    // undefind
                    block->at(j).SetObject(JSTaggedValue::Undefined().GetRawData());
                } else if (fwd != object) {
                    // update
                    block->at(j).SetObject(JSTaggedValue(fwd).GetRawData());
                }
            }
        }
    }
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
}  // namespace panda::ecmascript
