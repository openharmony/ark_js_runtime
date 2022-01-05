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
#include "ecmascript/mem/object_xray.h"

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
        ASSERT(id < EXTERNAL_RUNTIME_STUB_MAXCOUNT);
        runtimeFunctions_[id] = functionAddress;
    }

    Address GetFastStubEntry(uint32_t id)
    {
        ASSERT(id < kungfu::FAST_STUB_MAXCOUNT);
        return fastStubEntries_[id];
    }

    void SetFastStubEntry(uint32_t id, Address entry)
    {
        ASSERT(id < kungfu::FAST_STUB_MAXCOUNT);
        fastStubEntries_[id] = entry;
    }

    Address *GetBytecodeHandler()
    {
        return &bytecodeHandlers_[0];
    }

    void InitializeFastRuntimeStubs();

    void LoadStubModule(const char *moduleFile);

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

    static constexpr uint32_t GetRuntimeFunctionsOffset()
    {
        return MEMBER_OFFSET(JSThread, runtimeFunctions_);
    }

    static constexpr uint32_t GetFastStubEntriesOffset()
    {
        return MEMBER_OFFSET(JSThread, fastStubEntries_);
    }

    static constexpr uint32_t GetBytecodeHandlersOffset()
    {
        return MEMBER_OFFSET(JSThread, bytecodeHandlers_);
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

    static constexpr uint32_t MAX_RUNTIME_FUNCTIONS = kungfu::EXTERNAL_RUNTIME_STUB_MAXCOUNT;
    static constexpr uint32_t MAX_BYTECODE_HANDLERS = 0x100;
    // The sequence must be the same as that of the GLUE members.
    enum class GlueID : uint8_t {
        EXCEPTION = 0U,
        GLOBAL_CONST,
        PROPERTIES_CACHE,
        GLOBAL_STORAGE,
        CURRENT_FRAME,

        BYTECODE_HANDLERS,
        RUNTIME_FUNCTIONS,
        FAST_STUB_ENTRIES,
        FRAME_STATE_SIZE,
        OPT_LEAVE_FRAME_SIZE,
        OPT_LEAVE_FRAME_PREV_OFFSET,
        GLUE_FRAME_CONSTPOOL,
        GLUE_FRAME_PROFILE,
        GLUE_FRAME_ACC,
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
    Address bytecodeHandlers_[MAX_BYTECODE_HANDLERS];
    Address runtimeFunctions_[MAX_RUNTIME_FUNCTIONS];
    Address fastStubEntries_[kungfu::FAST_STUB_MAXCOUNT];

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};

#define GLUE_OFFSET_LIST(V)                                                                      \
    V(GLOBAL_CONSTANTS, GlobalConstants, EXCEPTION,                                              \
        JSTaggedValue::TaggedTypeSize(), JSTaggedValue::TaggedTypeSize())                        \
    V(PROPERTIES_CACHE, PropertiesCache, GLOBAL_CONSTANTS,                                       \
        static_cast<uint32_t>(ConstantIndex::CONSTATNT_COUNT) * JSTaggedValue::TaggedTypeSize(), \
        static_cast<uint32_t>(ConstantIndex::CONSTATNT_COUNT) * JSTaggedValue::TaggedTypeSize()) \
    V(GLOBAL_STORAGE, GlobalStorage, PROPERTIES_CACHE, sizeof(uint32_t), sizeof(uint64_t))       \
    V(CURRENT_FRAME, CurrentFrame, GLOBAL_STORAGE, sizeof(uint32_t), sizeof(uint64_t))           \
    V(BYTECODE_HANDLERS, BytecodeHandlers, CURRENT_FRAME, sizeof(uint32_t), sizeof(uint64_t))      \
    V(RUNTIME_FUNCTIONS, RuntimeFunctions, BYTECODE_HANDLERS,                                    \
        JSThread::MAX_BYTECODE_HANDLERS * sizeof(uint32_t),                                      \
        JSThread::MAX_BYTECODE_HANDLERS * sizeof(uint64_t))                                      \
    V(FASTSTUB_ENTRIES, FastStubEntries, RUNTIME_FUNCTIONS,                                      \
        JSThread::MAX_RUNTIME_FUNCTIONS * sizeof(uint32_t),                                      \
        JSThread::MAX_RUNTIME_FUNCTIONS * sizeof(uint64_t))                                      \

static constexpr uint32_t GLUE_EXCEPTION_OFFSET_32 = 0U;
static constexpr uint32_t GLUE_EXCEPTION_OFFSET_64 = 0U;
#define GLUE_OFFSET_MACRO(name, camelName, lastName, lastSize32, lastSize64)                        \
    static constexpr uint32_t GLUE_##name##_OFFSET_32 = GLUE_##lastName##_OFFSET_32 + (lastSize32); \
    static constexpr uint32_t GLUE_##name##_OFFSET_64 = GLUE_##lastName##_OFFSET_64 + (lastSize64);
GLUE_OFFSET_LIST(GLUE_OFFSET_MACRO)
#undef GLUE_OFFSET_MACRO

static constexpr uint32_t GLUE_FRAME_STATE_SIZE_64 =
     2 * sizeof(int64_t) + 5 * sizeof(int64_t) + sizeof(int64_t) + sizeof(int64_t);

static constexpr uint32_t GLUE_FRAME_CONSTPOOL_OFFSET_64 = 2 * sizeof(int64_t);
static constexpr uint32_t GLUE_FRAME_PROFILE_OFFSET_64 = GLUE_FRAME_CONSTPOOL_OFFSET_64 + 2 * JSTaggedValue::TaggedTypeSize();
static constexpr uint32_t GLUE_FRAME_ACC_OFFSET_64 = GLUE_FRAME_PROFILE_OFFSET_64 + JSTaggedValue::TaggedTypeSize();
static constexpr uint32_t GLUE_OPT_LEAVE_FRAME_SIZE_64 = 5 * sizeof(uint64_t);
static constexpr uint32_t GLUE_OPT_LEAVE_FRAME_PREV_OFFSET_64 = sizeof(uint64_t);

static constexpr uint32_t GLUE_FRAME_CONSTPOOL_OFFSET_32 = 2 * sizeof(int32_t);
static constexpr uint32_t GLUE_FRAME_PROFILE_OFFSET_32 = GLUE_FRAME_CONSTPOOL_OFFSET_32 + 2 * JSTaggedValue::TaggedTypeSize();
static constexpr uint32_t GLUE_FRAME_ACC_OFFSET_32 = GLUE_FRAME_PROFILE_OFFSET_32 + JSTaggedValue::TaggedTypeSize();
static constexpr uint32_t GLUE_FRAME_STATE_SIZE_32 =
    2 * sizeof(int32_t) + 5 * sizeof(int64_t) + sizeof(int32_t) + sizeof(int32_t) + sizeof(int64_t);
static constexpr uint32_t GLUE_OPT_LEAVE_FRAME_SIZE_32 = sizeof(uint64_t) + 4 * sizeof(uint32_t) + sizeof(uint64_t);
static constexpr uint32_t GLUE_OPT_LEAVE_FRAME_PREV_OFFSET_32 = sizeof(uint64_t);

#ifdef PANDA_TARGET_32
#define GLUE_OFFSET_MACRO(name, camelName, lastName, lastSize32, lastSize64)                   \
static_assert(GLUE_##name##_OFFSET_32 ==                                                       \
    (JSThread::Get##camelName##Offset() - JSThread::GetExceptionOffset()));
GLUE_OFFSET_LIST(GLUE_OFFSET_MACRO)
#undef GLUE_OFFSET_MACRO

static_assert(GLUE_FRAME_STATE_SIZE_32 == sizeof(struct panda::ecmascript::InterpretedFrame));
static_assert(GLUE_OPT_LEAVE_FRAME_SIZE_32 == sizeof(struct panda::ecmascript::OptLeaveFrame));
static_assert(GLUE_OPT_LEAVE_FRAME_PREV_OFFSET_32 == MEMBER_OFFSET(panda::ecmascript::OptLeaveFrame, prev));
#endif

#ifdef PANDA_TARGET_64

static_assert(GLUE_FRAME_STATE_SIZE_64 == sizeof(struct panda::ecmascript::InterpretedFrame));
static_assert(GLUE_OPT_LEAVE_FRAME_SIZE_64 == sizeof(struct panda::ecmascript::OptLeaveFrame));
static_assert(GLUE_OPT_LEAVE_FRAME_PREV_OFFSET_64 == MEMBER_OFFSET(panda::ecmascript::OptLeaveFrame, prev));
#define GLUE_OFFSET_MACRO(name, camelName, lastName, lastSize32, lastSize64)                   \
static_assert(GLUE_##name##_OFFSET_64 ==                                                       \
    (JSThread::Get##camelName##Offset() - JSThread::GetExceptionOffset()));
GLUE_OFFSET_LIST(GLUE_OFFSET_MACRO)
#undef GLUE_OFFSET_MACRO
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
