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

#ifndef ECMASCRIPT_ECMA_VM_H
#define ECMASCRIPT_ECMA_VM_H

#include <tuple>

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_handle_collection.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap_roots.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "ecmascript/tooling/pt_js_extractor.h"
#include "include/panda_vm.h"
#include "libpandabase/macros.h"
#include "libpandabase/os/library_loader.h"

namespace panda {
class RuntimeNotificationManager;
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class GlobalEnv;
class ObjectFactory;
class PropertiesCache;
class RegExpParserCache;
class EcmaRuntimeStat;
class EcmaHeapManager;
class Heap;
class HeapTracker;
class Program;

namespace job {
class MicroJobQueue;
}  // namespace job

template<typename T>
class JSHandle;
class JSArrayBuffer;
class JSFunction;
class Program;
class ModuleManager;
class EcmaModule;

class EcmaVM : public PandaVM {
    using PtJSExtractor = tooling::ecmascript::PtJSExtractor;

public:
    static EcmaVM *Cast(PandaVM *object)
    {
        return reinterpret_cast<EcmaVM *>(object);
    }

    static EcmaVM *Create(const JSRuntimeOptions &options);

    static bool Destroy(PandaVM *vm);

    explicit EcmaVM(JSRuntimeOptions options);

    static Expected<EcmaVM *, CString> Create([[maybe_unused]] Runtime *runtime);

    EcmaVM();

    ~EcmaVM() override;

    bool ExecuteFromPf(std::string_view filename, std::string_view entryPoint, const std::vector<std::string> &args,
                       bool isModule = false);

    bool ExecuteFromBuffer(const void *buffer, size_t size, std::string_view entryPoint,
                           const std::vector<std::string> &args);

    PtJSExtractor *GetDebugInfoExtractor(const panda_file::File *file);

    bool IsInitialized() const
    {
        return vmInitialized_;
    }

    ObjectFactory *GetFactory() const
    {
        return factory_;
    }

    bool Initialize() override;

    bool InitializeFinish() override;
    void UninitializeThreads() override {}
    void PreStartup() override {}
    void PreZygoteFork() override {}
    void PostZygoteFork() override {}
    void InitializeGC() override {}
    void StartGC() override {}
    void StopGC() override {}

    void VisitVmRoots([[maybe_unused]] const GCRootVisitor &visitor) override {}
    void UpdateVmRefs() override {}

    PandaVMType GetPandaVMType() const override
    {
        return PandaVMType::ECMA_VM;
    }

    LanguageContext GetLanguageContext() const override
    {
        return Runtime::GetCurrent()->GetLanguageContext(panda_file::SourceLang::ECMASCRIPT);
    }

    panda::mem::HeapManager *GetHeapManager() const override
    {
        return nullptr;
    }

    panda::mem::GC *GetGC() const override
    {
        return nullptr;
    }

    panda::mem::GCTrigger *GetGCTrigger() const override
    {
        return nullptr;
    }

    StringTable *GetStringTable() const override
    {
        return nullptr;
    }

    panda::mem::GCStats *GetGCStats() const override
    {
        return nullptr;
    }

    panda::mem::MemStatsType *GetMemStats() const override
    {
        return nullptr;
    }

    GCStats *GetEcmaGCStats() const
    {
        return gcStats_;
    }

    panda::mem::GlobalObjectStorage *GetGlobalObjectStorage() const override
    {
        return nullptr;
    }

    MonitorPool *GetMonitorPool() const override
    {
        return nullptr;
    }

    ThreadManager *GetThreadManager() const override
    {
        return nullptr;
    }

    ManagedThread *GetAssociatedThread() const override
    {
        return thread_;
    }

    JSThread *GetAssociatedJSThread() const
    {
        return thread_;
    }

    CompilerInterface *GetCompiler() const override
    {
        return nullptr;
    }

    Rendezvous *GetRendezvous() const override
    {
        return rendezvous_;
    }

    ObjectHeader *GetOOMErrorObject() override
    {
        // preallocated OOM is not implemented for JS
        UNREACHABLE();
    }

    panda::mem::ReferenceProcessor *GetReferenceProcessor() const override
    {
        return nullptr;
    }

    const RuntimeOptions &GetOptions() const override
    {
        return Runtime::GetOptions();
    }

    static const JSRuntimeOptions &GetJSOptions()
    {
        return options_;
    }

    JSHandle<GlobalEnv> GetGlobalEnv() const;

    JSHandle<job::MicroJobQueue> GetMicroJobQueue() const;

    bool ExecutePromisePendingJob() const;

    PropertiesCache *GetPropertiesCache() const
    {
        return propertiesCache_;
    }

    RegExpParserCache *GetRegExpParserCache() const
    {
        ASSERT(regExpParserCache_ != nullptr);
        return regExpParserCache_;
    }

    JSMethod *GetMethodForNativeFunction(const void *func);

    EcmaStringTable *GetEcmaStringTable() const
    {
        ASSERT(stringTable_ != nullptr);
        return stringTable_;
    }

    JSThread *GetJSThread() const
    {
#if defined(ECMASCRIPT_ENABLE_THREAD_CHECK) && ECMASCRIPT_ENABLE_THREAD_CHECK
        ThreadPool *pool = GetHeap()->GetThreadPool();
        // Exclude GC thread
        if (!pool->IsInThreadPool(std::this_thread::get_id()) &&
            thread_->GetThreadId() != JSThread::GetCurrentThreadId()) {
                LOG(FATAL, RUNTIME) << "Fatal: ecma_vm cannot run in multi-thread!";
        }
#endif
        return thread_;
    }

    bool ICEnable() const
    {
        return icEnable_;
    }

    void PushToArrayDataList(JSNativePointer *array);
    void RemoveArrayDataList(JSNativePointer *array);

    JSHandle<ecmascript::JSTaggedValue> GetEcmaUncaughtException() const;
    void EnableUserUncaughtErrorHandler();

    template<typename Callback>
    void EnumeratePandaFiles(Callback cb) const
    {
        for (const auto &iter : pandaFileWithProgram_) {
            if (!cb(std::get<0>(iter), std::get<1>(iter))) {
                break;
            }
        }
    }

    template<typename Callback>
    void EnumerateProgram(Callback cb, std::string pandaFile) const
    {
        for (const auto &iter : pandaFileWithProgram_) {
            if (pandaFile == std::get<1>(iter)->GetFilename()) {
                cb(std::get<0>(iter));
                break;
            }
        }
    }

    EcmaRuntimeStat *GetRuntimeStat() const
    {
        return runtimeStat_;
    }

    void SetRuntimeStatEnable(bool flag);

    bool IsRuntimeStatEnabled() const
    {
        return runtimeStatEnabled_;
    }

    void Iterate(const RootVisitor &v);

    const Heap *GetHeap() const
    {
        return heap_;
    }

    void CollectGarbage(TriggerGCType gcType) const;

    void StartHeapTracking(HeapTracker *tracker);

    void StopHeapTracking();

    RegionFactory *GetRegionFactory() const
    {
        return regionFactory_.get();
    }

    Chunk *GetChunk() const
    {
        return const_cast<Chunk *>(&chunk_);
    }

    void ProcessReferences(const WeakRootVisitor &v0);

    JSHandle<JSTaggedValue> GetModuleByName(JSHandle<JSTaggedValue> moduleName);

    void ExecuteModule(std::string_view moduleFile, std::string_view entryPoint, const std::vector<std::string> &args);

    ModuleManager *GetModuleManager() const
    {
        return moduleManager_;
    }

    void SetupRegExpResultCache();
    JSHandle<JSTaggedValue> GetRegExpCache()
    {
        return JSHandle<JSTaggedValue>(reinterpret_cast<uintptr_t>(&regexpCache_));
    }

    RuntimeNotificationManager *GetNotificationManager() const
    {
        return notificationManager_;
    }

protected:
    bool CheckEntrypointSignature([[maybe_unused]] Method *entrypoint) override
    {
        return true;
    }

    Expected<int, Runtime::Error> InvokeEntrypointImpl(Method *entrypoint,
                                                       const std::vector<std::string> &args) override;

    void HandleUncaughtException(ObjectHeader *exception) override;

    void PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo);

private:
    void AddPandaFile(const panda_file::File *pf, bool isModule);
    void SetProgram(Program *program, const panda_file::File *pf);
    bool IsFrameworkPandaFile(std::string_view filename) const;

    void SetGlobalEnv(GlobalEnv *global);

    void SetMicroJobQueue(job::MicroJobQueue *queue);

    bool Execute(const panda_file::File &pf, std::string_view entryPoint, const std::vector<std::string> &args);

    Expected<int, Runtime::Error> InvokeEcmaEntrypoint(const panda_file::File &pf, const CString &methodName,
                                                       const std::vector<std::string> &args);

    void InitializeEcmaScriptRunStat();

    void RedirectMethod(const panda_file::File &pf);

    bool VerifyFilePath(const CString &filePath) const;

    void ClearBufferData();

    void ClearNativeMethodsData();

    NO_MOVE_SEMANTIC(EcmaVM);
    NO_COPY_SEMANTIC(EcmaVM);

    // init EcmaVM construct
    static JSRuntimeOptions options_;
    EcmaStringTable *stringTable_;
    std::unique_ptr<RegionFactory> regionFactory_;
    Chunk chunk_;
    ChunkVector<JSMethod *> nativeMethods_;
    bool icEnable_{true};
    GCStats *gcStats_ = nullptr;
    Rendezvous *rendezvous_{nullptr};
    bool snapshotSerializeEnable_{false};
    bool snapshotDeserializeEnable_{false};
    CString fileName_;
    CString frameworkAbcFileName_;

    bool isTestMode_{false};
    RuntimeNotificationManager *notificationManager_ {nullptr};

    // init EcmaVM Create
    JSThread *thread_{nullptr};

    // init EcmaVM Initialize
    PropertiesCache *propertiesCache_{nullptr};
    RegExpParserCache *regExpParserCache_{nullptr};
    Heap *heap_{nullptr};
    ObjectFactory *factory_{nullptr};
    JSTaggedValue globalEnv_{JSTaggedValue::Hole()};
    JSTaggedValue regexpCache_{JSTaggedValue::Hole()};
    JSTaggedValue microJobQueue_{JSTaggedValue::Hole()};
    ModuleManager *moduleManager_{nullptr};
    bool vmInitialized_{false};

    // Runtime initialization
    CUnorderedMap<const panda_file::File *, std::unique_ptr<PtJSExtractor>> extractorCache_;
    const panda_file::File *frameworkPandaFile_ {nullptr};
    CVector<JSMethod *> frameworkProgramMethods_;
    EcmaRuntimeStat *runtimeStat_ {nullptr};  // maybe nullptr
    bool runtimeStatEnabled_ {false};
    bool isUncaughtExceptionRegistered_ {false};

    // weak reference need Redirect address
    JSTaggedValue frameworkProgram_ {JSTaggedValue::Hole()};  // no mark for gc
    CVector<JSNativePointer *> arrayBufferDataList_;
    CVector<std::tuple<Program *, const panda_file::File *, bool>> pandaFileWithProgram_;

    friend class SnapShotSerialize;
    friend class ObjectFactory;
    friend class ValueSerializer;
    friend class panda::JSNApi;
};
}  // namespace ecmascript
}  // namespace panda

#endif
