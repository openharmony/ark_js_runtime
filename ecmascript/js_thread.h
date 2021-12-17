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

#include "include/thread.h"

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/mem/heap_roots.h"

namespace panda::ecmascript {
class EcmaVM;
class RegionFactory;
class InternalCallParams;
class PropertiesCache;

enum class MarkStatus : uint8_t {
    NOT_BEGIN_MARK,
    MARKING,
    MARK_FINISHED
};

class JSThread : public ManagedThread {
public:
    static constexpr int CONCURRENT_MARKING_BITFIELD_NUM = 2;
    using MarkStatusBits = BitField<MarkStatus, 0, CONCURRENT_MARKING_BITFIELD_NUM>;
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

    const JSTaggedType *GetLastInterpretedFrameSp() const
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

    void HandleScopeCountAdd()
    {
        handleScopeCount_++;
    }

    void HandleScopeCountDec()
    {
        handleScopeCount_--;
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
    PropertiesCache *GetPropertiesCache() const
    {
        return propertiesCache_;
    }

    static constexpr uint32_t GetPropertiesCacheOffset()
    {
        return MEMBER_OFFSET(JSThread, propertiesCache_);
    }

    static constexpr uint32_t GetGlobalConstantsOffset()
    {
        return MEMBER_OFFSET(JSThread, globalConst_);
    }

    static constexpr uint32_t GetGlobalStorageOffset()
    {
        return MEMBER_OFFSET(JSThread, globalStorage_);
    }

    static constexpr uint32_t GetCurrentFrameOffset()
    {
        return MEMBER_OFFSET(JSThread, currentFrame_);
    }

    static constexpr uint32_t GetLastIFrameOffset()
    {
        return MEMBER_OFFSET(JSThread, lastIFrame_);
    }

    static constexpr uint32_t GetRuntimeFunctionsOffset()
    {
        return MEMBER_OFFSET(JSThread, runtimeFunctions_);
    }

    static constexpr uint32_t GetFastStubEntiresOffset()
    {
        return MEMBER_OFFSET(JSThread, fastStubEntires_);
    }

    void SetMarkStatus(MarkStatus status)
    {
        uint64_t newVal = MarkStatusBits::Update(threadStatusBitField_, status);
        threadStatusBitField_ = newVal;
    }

    bool IsNotBeginMark() const
    {
        auto status = MarkStatusBits::Decode(threadStatusBitField_);
        return status == MarkStatus::NOT_BEGIN_MARK;
    }

    bool IsMarking() const
    {
        auto status = MarkStatusBits::Decode(threadStatusBitField_);
        return status == MarkStatus::MARKING;
    }

    bool IsMarkFinished() const
    {
        auto status = MarkStatusBits::Decode(threadStatusBitField_);
        return status == MarkStatus::MARK_FINISHED;
    }

    bool CheckSafepoint() const;

    void SetGetStackSignal(bool isParseStack)
    {
        getStackSignal_ = isParseStack;
    }

    bool GetStackSignal() const
    {
        return getStackSignal_;
    }

    void SetGcState(bool gcState)
    {
        gcState_ = gcState;
    }

    bool GetGcState() const
    {
        return gcState_;
    }
    static constexpr uint32_t GetExceptionOffset()
    {
        return MEMBER_OFFSET(JSThread, exception_);
    }

    uintptr_t GetGlueAddr() const
    {
        return reinterpret_cast<uintptr_t>(this) + GetExceptionOffset();
    }

    static JSThread *GlueToJSThread(uintptr_t glue)
    {
        // very careful to modify here
        return reinterpret_cast<JSThread *>(glue - GetExceptionOffset());
    }

    static constexpr uint32_t MAX_RUNTIME_FUNCTIONS =
        kungfu::EXTERN_RUNTIME_STUB_MAXCOUNT - kungfu::EXTERNAL_RUNTIME_STUB_BEGIN - 1;
    // The sequence must be the same as that of the GLUE members.
    enum class GlueID : uint8_t {
        EXCEPTION = 0U,
        GLOBAL_CONST,
        PROPERTIES_CACHE,
        GLOBAL_STORAGE,
        CURRENT_FRAME,
        LAST_I_FRAME,
        RUNTIME_FUNCTIONS,
        FAST_STUB_ENTIRES,
        NUMBER_OF_GLUE,
    };

private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);

    void DumpStack() DUMP_API_ATTR;

    static constexpr uint32_t MAX_STACK_SIZE = 128 * 1024;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;

    JSTaggedValue stubCode_ {JSTaggedValue::Hole()};
    os::memory::ConditionVariable initializationVar_ GUARDED_BY(initializationLock_);
    os::memory::Mutex initializationLock_;
    int nestedLevel_ = 0;
    JSTaggedType *frameBase_ {nullptr};
    bool isSnapshotMode_ {false};
    bool isEcmaInterpreter_ {false};
    bool getStackSignal_ {false};
    bool gcState_ {false};
    RegionFactory *regionFactory_ {nullptr};
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
    int32_t handleScopeCount_ {0};
    bool stableArrayElementsGuardians_ {true};
    InternalCallParams *internalCallParams_ {nullptr};
    uintptr_t *lastOptCallRuntimePc_ {nullptr};
    volatile uint64_t threadStatusBitField_ {0ULL};

    // GLUE members start, very careful to modify here
    JSTaggedValue exception_ {JSTaggedValue::Hole()};
    GlobalEnvConstants globalConst_;  // Place-Holder
    PropertiesCache *propertiesCache_ {nullptr};
    EcmaGlobalStorage *globalStorage_ {nullptr};
    JSTaggedType *currentFrame_ {nullptr};
    JSTaggedType *lastIFrame_ {nullptr};
    Address runtimeFunctions_[MAX_RUNTIME_FUNCTIONS];
    Address fastStubEntires_[kungfu::FAST_STUB_MAXCOUNT];

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};

static constexpr uint32_t GLUE_EXCEPTION_OFFSET_32 = 0U;
static constexpr uint32_t GLUE_GLOBAL_CONSTANTS_OFFSET_32 = GLUE_EXCEPTION_OFFSET_32 + sizeof(JSTaggedValue);
static constexpr uint32_t GLUE_PROPERTIES_CACHE_OFFSET_32 =
    GLUE_GLOBAL_CONSTANTS_OFFSET_32 + sizeof(JSTaggedValue) * static_cast<uint32_t>(ConstantIndex::CONSTATNT_COUNT);
static constexpr uint32_t GLUE_GLOBAL_STORAGE_OFFSET_32 = GLUE_PROPERTIES_CACHE_OFFSET_32 + sizeof(int32_t);
static constexpr uint32_t GLUE_CURRENT_FRAME_OFFSET_32 = GLUE_GLOBAL_STORAGE_OFFSET_32 + sizeof(int32_t);
static constexpr uint32_t GLUE_LAST_IFRAME_OFFSET_32 = GLUE_CURRENT_FRAME_OFFSET_32 + sizeof(int32_t);
static constexpr uint32_t GLUE_RUNTIME_FUNCTIONS_OFFSET_32 = GLUE_LAST_IFRAME_OFFSET_32 + sizeof(int32_t);
static constexpr uint32_t GLUE_FASTSTUB_ENTRIES_OFFSET_32 =
    GLUE_RUNTIME_FUNCTIONS_OFFSET_32 + sizeof(int32_t) * JSThread::MAX_RUNTIME_FUNCTIONS;

static constexpr uint32_t GLUE_EXCEPTION_OFFSET_64 = 0U;
static constexpr uint32_t GLUE_GLOBAL_CONSTANTS_OFFSET_64 = GLUE_EXCEPTION_OFFSET_64 + sizeof(JSTaggedValue);
static constexpr uint32_t GLUE_PROPERTIES_CACHE_OFFSET_64 =
    GLUE_GLOBAL_CONSTANTS_OFFSET_64 + sizeof(JSTaggedValue) * static_cast<uint32_t>(ConstantIndex::CONSTATNT_COUNT);
static constexpr uint32_t GLUE_GLOBAL_STORAGE_OFFSET_64 = GLUE_PROPERTIES_CACHE_OFFSET_64 + sizeof(int64_t);
static constexpr uint32_t GLUE_CURRENT_FRAME_OFFSET_64 = GLUE_GLOBAL_STORAGE_OFFSET_64 + sizeof(int64_t);
static constexpr uint32_t GLUE_LAST_IFRAME_OFFSET_64 = GLUE_CURRENT_FRAME_OFFSET_64 + sizeof(int64_t);
static constexpr uint32_t GLUE_RUNTIME_FUNCTIONS_OFFSET_64 = GLUE_LAST_IFRAME_OFFSET_64 + sizeof(int64_t);
static constexpr uint32_t GLUE_FASTSTUB_ENTRIES_OFFSET_64 =
    GLUE_RUNTIME_FUNCTIONS_OFFSET_64 + sizeof(int64_t) * JSThread::MAX_RUNTIME_FUNCTIONS;

#ifdef PANDA_TARGET_32
static_assert(GLUE_PROPERTIES_CACHE_OFFSET_32 ==
    (JSThread::GetPropertiesCacheOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_GLOBAL_STORAGE_OFFSET_32 == (JSThread::GetGlobalStorageOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_CURRENT_FRAME_OFFSET_32 == (JSThread::GetCurrentFrameOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_LAST_IFRAME_OFFSET_32 == (JSThread::GetLastIFrameOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_GLOBAL_CONSTANTS_OFFSET_32 ==
    (JSThread::GetGlobalConstantsOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_EXCEPTION_OFFSET_32 == (JSThread::GetExceptionOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_RUNTIME_FUNCTIONS_OFFSET_32 ==
    (JSThread::GetRuntimeFunctionsOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_FASTSTUB_ENTRIES_OFFSET_32 ==
    (JSThread::GetFastStubEntiresOffset() - JSThread::GetExceptionOffset()));
#endif
#ifdef PANDA_TARGET_64
static_assert(GLUE_PROPERTIES_CACHE_OFFSET_64 ==
    (JSThread::GetPropertiesCacheOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_GLOBAL_STORAGE_OFFSET_64 == (JSThread::GetGlobalStorageOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_CURRENT_FRAME_OFFSET_64 == (JSThread::GetCurrentFrameOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_LAST_IFRAME_OFFSET_64 == (JSThread::GetLastIFrameOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_GLOBAL_CONSTANTS_OFFSET_64 ==
    (JSThread::GetGlobalConstantsOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_EXCEPTION_OFFSET_64 == (JSThread::GetExceptionOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_RUNTIME_FUNCTIONS_OFFSET_64 ==
    (JSThread::GetRuntimeFunctionsOffset() - JSThread::GetExceptionOffset()));
static_assert(GLUE_FASTSTUB_ENTRIES_OFFSET_64 ==
    (JSThread::GetFastStubEntiresOffset() - JSThread::GetExceptionOffset()));
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
