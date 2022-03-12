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

#include "include/managed_thread.h"
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/trampoline/runtime_define.h"

namespace panda::ecmascript {
class EcmaVM;
class HeapRegionAllocator;
class InternalCallParams;
class PropertiesCache;

enum class MarkStatus : uint8_t {
    READY_TO_MARK,
    MARKING,
    MARK_FINISHED,
};

struct BCHandlers {
    static constexpr size_t MAX_BYTECODE_HANDLERS = 0x100;
    Address handlers_[MAX_BYTECODE_HANDLERS];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * MAX_BYTECODE_HANDLERS;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * MAX_BYTECODE_HANDLERS;

    void Set(size_t index, Address addr)
    {
        assert(index < MAX_BYTECODE_HANDLERS);
        handlers_[index] = addr;
    }

    Address* GetAddr()
    {
        return reinterpret_cast<Address*>(handlers_);
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(BCHandlers), BCHandlers::SizeArch32, BCHandlers::SizeArch64);

struct RTInterfaces {
    static constexpr size_t MAX_RUNTIME_FUNCTIONS = RuntimeTrampolineId::RUNTIME_CALL_MAX_ID;
    Address interfaces_[MAX_RUNTIME_FUNCTIONS];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * MAX_RUNTIME_FUNCTIONS;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * MAX_RUNTIME_FUNCTIONS;

    void Set(size_t index, Address addr)
    {
        assert(index < MAX_RUNTIME_FUNCTIONS);
        interfaces_[index] = addr;
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(RTInterfaces), RTInterfaces::SizeArch32, RTInterfaces::SizeArch64);

struct StubEntries {
    Address entries_[kungfu::FAST_STUB_MAXCOUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * kungfu::FAST_STUB_MAXCOUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * kungfu::FAST_STUB_MAXCOUNT;

    void Set(size_t index, Address addr)
    {
        assert(index < kungfu::FAST_STUB_MAXCOUNT);
        entries_[index] = addr;
    }

    Address Get(size_t index)
    {
        assert(index < kungfu::FAST_STUB_MAXCOUNT);
        return entries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(StubEntries), StubEntries::SizeArch32, StubEntries::SizeArch64);

class JSThread : public ManagedThread {
public:
    static constexpr int CONCURRENT_MARKING_BITFIELD_NUM = 2;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    using MarkStatusBits = BitField<MarkStatus, 0, CONCURRENT_MARKING_BITFIELD_NUM>;

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

    const JSTaggedType *GetCurrentSPFrame() const
    {
        return glueData_.currentFrame_;
    }

    void SetCurrentSPFrame(JSTaggedType *sp)
    {
        glueData_.currentFrame_ = sp;
    }

    const JSTaggedType *GetLastLeaveFrame() const
    {
        return glueData_.leaveFrame_;
    }

    void SetLastLeaveFrame(JSTaggedType *sp)
    {
        glueData_.leaveFrame_ = sp;
    }

    bool DoStackOverflowCheck(const JSTaggedType *sp);

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
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
        return glueData_.exception_;
    }

    bool HasPendingException() const
    {
        return !glueData_.exception_.IsHole();
    }

    void ClearException();

    EcmaGlobalStorage *GetEcmaGlobalStorage() const
    {
        return globalStorage_;
    }

    void SetGlobalObject(JSTaggedValue globalObject)
    {
        glueData_.globalObject_ = globalObject;
    }

    const GlobalEnvConstants *GlobalConstants() const
    {
        return &glueData_.globalConst_;
    }

    void NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver);

    bool IsStableArrayElementsGuardiansInvalid() const
    {
        return !stableArrayElementsGuardians_;
    }

    void ResetGuardians();

    JSTaggedValue GetCurrentLexenv() const;

    void SetRuntimeFunction(size_t id, Address addr)
    {
        ASSERT(id < RuntimeTrampolineId::RUNTIME_CALL_MAX_ID);
        glueData_.rtInterfaces_.Set(id, addr);
    }

    Address GetFastStubEntry(uint32_t id)
    {
        return glueData_.stubEntries_.Get(id);
    }

    void SetFastStubEntry(size_t id, Address entry)
    {
        glueData_.stubEntries_.Set(id, entry);
    }

    Address *GetBytecodeHandler()
    {
        return glueData_.bcHandlers_.GetAddr();
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

    void SetMarkStatus(MarkStatus status)
    {
        MarkStatusBits::Set(status, &glueData_.threadStateBitField_);
    }

    bool IsReadyToMark() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
        return status == MarkStatus::READY_TO_MARK;
    }

    bool IsMarking() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
        return status == MarkStatus::MARKING;
    }

    bool IsMarkFinished() const
    {
        auto status = MarkStatusBits::Decode(glueData_.threadStateBitField_);
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

    VmThreadControl *GetVmThreadControl() const
    {
        return vmThreadControl_;
    }

    static constexpr size_t GetGlueDataOffset()
    {
        return MEMBER_OFFSET(JSThread, glueData_);
    }

    uintptr_t GetGlueAddr() const
    {
        return reinterpret_cast<uintptr_t>(this) + GetGlueDataOffset();
    }

    static JSThread *GlueToJSThread(uintptr_t glue)
    {
        // very careful to modify here
        return reinterpret_cast<JSThread *>(glue - GetGlueDataOffset());
    }

    struct GlueData : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                 JSTaggedValue,
                                                 JSTaggedValue,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 BCHandlers,
                                                 RTInterfaces,
                                                 StubEntries,
                                                 base::AlignedUint64,
                                                 base::AlignedPointer,
                                                 GlobalEnvConstants> {
        enum class Index : size_t {
            ExceptionIndex = 0,
            GlobalObjIndex,
            CurrentFrameIndex,
            LeaveFrameIndex,
            BCHandlersIndex,
            RTInterfacesIndex,
            StubEntriesIndex,
            StateBitFieldIndex,
            FrameBaseIndex,
            GlobalConstIndex,
            NumOfMembers
        };
        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

        static size_t GetExceptionOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::ExceptionIndex)>(isArch32);
        }

        static size_t GetGlobalObjOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalObjIndex)>(isArch32);
        }

        static size_t GetGlobalConstOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalConstIndex)>(isArch32);
        }

        static size_t GetStateBitFieldOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StateBitFieldIndex)>(isArch32);
        }

        static size_t GetCurrentFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::CurrentFrameIndex)>(isArch32);
        }

        static size_t GetLeaveFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LeaveFrameIndex)>(isArch32);
        }

        static size_t GetBCHandlersOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BCHandlersIndex)>(isArch32);
        }

        static size_t GetRTInterfacesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::RTInterfacesIndex)>(isArch32);
        }

        static size_t GetStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StubEntriesIndex)>(isArch32);
        }

        static size_t GetFrameBaseOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::FrameBaseIndex)>(isArch32);
        }

        alignas(EAS) JSTaggedValue exception_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedValue globalObject_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedType *currentFrame_ {nullptr};
        alignas(EAS) JSTaggedType *leaveFrame_ {nullptr};
        alignas(EAS) BCHandlers bcHandlers_;
        alignas(EAS) RTInterfaces rtInterfaces_;
        alignas(EAS) StubEntries stubEntries_;
        alignas(EAS) volatile uint64_t threadStateBitField_ {0ULL};
        alignas(EAS) JSTaggedType *frameBase_ {nullptr};
        alignas(EAS) GlobalEnvConstants globalConst_;
    };
    static_assert(MEMBER_OFFSET(GlueData, rtInterfaces_) == ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET);
    static_assert(MEMBER_OFFSET(GlueData, currentFrame_) == ASM_GLUE_CURRENT_FRAME_OFFSET);
    static_assert(MEMBER_OFFSET(GlueData, leaveFrame_) == ASM_GLUE_LEAVE_FRAME_OFFSET);
    STATIC_ASSERT_EQ_ARCH(sizeof(GlueData), GlueData::SizeArch32, GlueData::SizeArch64);

private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);

    void DumpStack() DUMP_API_ATTR;

    static constexpr uint32_t MAX_STACK_SIZE = 128 * 1024;
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;
    static constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;

    // MM: handles, global-handles, and aot-stubs.
    int nestedLevel_ = 0;
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    JSTaggedType *handleScopeStorageNext_ {nullptr};
    JSTaggedType *handleScopeStorageEnd_ {nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_ {};
    int32_t currentHandleStorageIndex_ {-1};
    int32_t handleScopeCount_ {0};
    JSTaggedValue stubCode_ {JSTaggedValue::Hole()};

    PropertiesCache *propertiesCache_ {nullptr};
    EcmaGlobalStorage *globalStorage_ {nullptr};

    // Run-time state
    bool getStackSignal_ {false};
    bool gcState_ {false};
    VmThreadControl *vmThreadControl_ {nullptr};

    bool stableArrayElementsGuardians_ {true};
    InternalCallParams *internalCallParams_ {nullptr};
    GlueData glueData_;

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
