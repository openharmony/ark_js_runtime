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

#ifndef ECMASCRIPT_JS_THREAD_H
#define ECMASCRIPT_JS_THREAD_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/mem/heap_roots.h"
#include "include/thread.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript {
class EcmaVM;
class RegionFactory;
class InternalCallParams;

class JSThread : public ManagedThread {
public:
    using Address = uintptr_t;
    static JSThread *Cast(ManagedThread *thread)
    {
        ASSERT(thread != nullptr);
        return reinterpret_cast<JSThread *>(thread);
    }

    JSThread(Runtime *runtime, PandaVM *vm);

    ~JSThread() override;

    EcmaVM *GetEcmaVM() const;

    static JSThread *Create(Runtime *runtime, PandaVM *vm);

    int GetNestedLevel() const
    {
        return nestedLevel_;
    }

    void SetNestedLevel(int level)
    {
        nestedLevel_ = level;
    }

    bool IsSnapshotMode() const
    {
        return isSnapshotMode_;
    }

    void SetIsSnapshotMode(bool value)
    {
        isSnapshotMode_ = value;
    }

    const JSTaggedType *GetCurrentSPFrame() const
    {
        return currentFrame_;
    }

    void SetCurrentSPFrame(JSTaggedType *sp)
    {
        currentFrame_ = sp;
    }

    const JSTaggedType *GetLastIFrameSp() const
    {
        return lastIFrame_;
    }

    void SetLastIFrameSp(JSTaggedType *sp)
    {
        lastIFrame_ = sp;
    }

    bool DoStackOverflowCheck(const JSTaggedType *sp);

    bool IsEcmaInterpreter() const
    {
        return isEcmaInterpreter_;
    }

    void SetIsEcmaInterpreter(bool value)
    {
        isEcmaInterpreter_ = value;
    }

    RegionFactory *GetRegionFactory() const
    {
        return regionFactory_;
    }

    void Iterate(const RootVisitor &v0, const RootRangeVisitor &v1);

    uintptr_t *ExpandHandleStorage();
    void ShrinkHandleStorage(int prevIndex);

    JSTaggedType *GetHandleScopeStorageNext() const
    {
        return handleScopeStorageNext_;
    }

    void SetHandleScopeStorageNext(JSTaggedType *value)
    {
        handleScopeStorageNext_ = value;
    }

    JSTaggedType *GetHandleScopeStorageEnd() const
    {
        return handleScopeStorageEnd_;
    }

    void SetHandleScopeStorageEnd(JSTaggedType *value)
    {
        handleScopeStorageEnd_ = value;
    }

    int GetCurrentHandleStorageIndex()
    {
        return currentHandleStorageIndex_;
    }

    void SetException(JSTaggedValue exception);

    JSTaggedValue GetException() const
    {
        return exception_;
    }

    bool HasPendingException() const
    {
        return !exception_.IsHole();
    }

    void ClearException();

    EcmaGlobalStorage *GetEcmaGlobalStorage() const
    {
        return globalStorage_;
    }

    const GlobalEnvConstants *GlobalConstants() const
    {
        return &globalConst_;
    }

    void NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver);

    bool IsStableArrayElementsGuardiansInvalid() const
    {
        return !stableArrayElementsGuardians_;
    }

    void ResetGuardians();

    JSTaggedValue GetCurrentLexenv() const;

    void SetRuntimeFunction(uint32_t id, Address functionAddress)
    {
        ASSERT(id < MAX_RUNTIME_FUNCTIONS);
        runtimeFunctions_[id] = functionAddress;
    }

    Address GetFastStubEntry(uint32_t id)
    {
        ASSERT(id < kungfu::FAST_STUB_MAXCOUNT);
        return fastStubEntires_[id];
    }

    void SetFastStubEntry(uint32_t id, Address entry)
    {
        ASSERT(id < kungfu::FAST_STUB_MAXCOUNT);
        fastStubEntires_[id] = entry;
    }

    void InitializeFastRuntimeStubs();

    void LoadFastStubModule(const char *moduleFile);

    static uint32_t GetRuntimeFunctionsOffset()
    {
        return MEMBER_OFFSET(JSThread, runtimeFunctions_);
    }

    static uint64_t GetCurrentFrameOffset()
    {
        return MEMBER_OFFSET(JSThread, currentFrame_);
    }

    static uint64_t GetGlobalConstantOffset()
    {
        return MEMBER_OFFSET(JSThread, globalConst_);
    }

    static uint64_t GetFastStubEntryOffset()
    {
        return MEMBER_OFFSET(JSThread, fastStubEntires_);
    }

    InternalCallParams *GetInternalCallParams() const
    {
        return internalCallParams_;
    }

    ThreadId GetThreadId() const
    {
        return GetId();
    }

    static ThreadId GetCurrentThreadId()
    {
        return os::thread::GetCurrentThreadId();
    }

    void IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor);

    uintptr_t* GetLastOptCallRuntimePc() const
    {
        return lastOptCallRuntimePc_;
    }
    void SetLastOptCallRuntimePc(uintptr_t* pc)
    {
        lastOptCallRuntimePc_ = pc;
    }
private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);

    void DumpStack() DUMP_API_ATTR;

    static constexpr uint32_t MAX_STACK_SIZE = 128 * 1024;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr uint32_t MAX_RUNTIME_FUNCTIONS =
        kungfu::EXTERN_RUNTIME_STUB_MAXCOUNT - kungfu::EXTERNAL_RUNTIME_STUB_BEGIN - 1;

    Address runtimeFunctions_[MAX_RUNTIME_FUNCTIONS];
    Address fastStubEntires_[kungfu::FAST_STUB_MAXCOUNT];
    JSTaggedValue stubCode_ {JSTaggedValue::Hole()};
    EcmaGlobalStorage *globalStorage_ {nullptr};

    os::memory::ConditionVariable initializationVar_ GUARDED_BY(initializationLock_);
    os::memory::Mutex initializationLock_;
    int nestedLevel_ = 0;
    JSTaggedType *currentFrame_ {nullptr};
    JSTaggedType *lastIFrame_ {nullptr};
    JSTaggedType *frameBase_ {nullptr};
    bool isSnapshotMode_ {false};
    bool isEcmaInterpreter_ {false};
    RegionFactory *regionFactory_ {nullptr};
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
    JSTaggedValue exception_ {JSTaggedValue::Hole()};
    bool stableArrayElementsGuardians_ {true};
    GlobalEnvConstants globalConst_;  // Place-Holder
    InternalCallParams *internalCallParams_ {nullptr};
    uintptr_t *lastOptCallRuntimePc_ {nullptr};

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
