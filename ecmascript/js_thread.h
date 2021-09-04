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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_THREAD_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_THREAD_H

#include "ecmascript/global_env_constants.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/mem/heap_roots.h"
#include "global_handle_storage-inl.h"
#include "include/thread.h"

namespace panda::ecmascript {
class EcmaVM;
class RegionFactory;

class JSThread : public ManagedThread {
public:
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

    JSTaggedType *GetCurrentSPFrame() const
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
    void ShrunkHandleStorage(const JSTaggedType *end);

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

    GlobalHandleStorage<JSTaggedType> *GetGlobalHandleStorage() const
    {
        return globalHandleStorage_;
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

private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);

    void DumpStack() DUMP_API_ATTR;

    static constexpr uint32_t MAX_STACK_SIZE = 256 * 1024;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    static const uint32_t NODE_BLOCK_SIZE_LOG2 = 10;
    static const uint32_t NODE_BLOCK_SIZE = 1U << NODE_BLOCK_SIZE_LOG2;

    GlobalHandleStorage<JSTaggedType> *globalHandleStorage_{nullptr};

    os::memory::ConditionVariable initializationVar_ GUARDED_BY(initializationLock_);
    os::memory::Mutex initializationLock_;
    int nestedLevel_ = 0;
    JSTaggedType *currentFrame_{nullptr};
    JSTaggedType *frameBase_{nullptr};
    bool isSnapshotMode_{false};
    bool isEcmaInterpreter_{false};
    RegionFactory *regionFactory_{nullptr};
    JSTaggedType *handleScopeStorageNext_{nullptr};
    JSTaggedType *handleScopeStorageEnd_{nullptr};
    std::vector<std::array<JSTaggedType, NODE_BLOCK_SIZE> *> handleStorageNodes_{};
    int32_t currentHandleStorageIndex_{-1};
    JSTaggedValue exception_{JSTaggedValue::Hole()};
    bool stableArrayElementsGuardians_{true};
    GlobalEnvConstants globalConst_;  // Place-Holder

    friend class EcmaHandleScope;
    friend class GlobalHandleCollection;
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_THREAD_H
