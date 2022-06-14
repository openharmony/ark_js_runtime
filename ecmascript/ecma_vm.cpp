/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_vm.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/module_data_extractor.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "ecmascript/taskpool/task.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "ecmascript/llvm_stackmap_parser.h"
#ifdef PANDA_TARGET_WINDOWS
#ifdef ERROR
#undef ERROR
#endif
#ifdef GetObject
#undef GetObject
#endif
#endif

namespace panda::ecmascript {
/* static */
EcmaVM *EcmaVM::Create(const JSRuntimeOptions &options)
{
    auto vm = new EcmaVM(options);
    if (UNLIKELY(vm == nullptr)) {
        LOG_ECMA(ERROR) << "Failed to create jsvm";
        return nullptr;
    }
    auto jsThread = JSThread::Create(vm);
    vm->thread_ = jsThread;
    vm->Initialize();
    return vm;
}

// static
bool EcmaVM::Destroy(EcmaVM *vm)
{
    if (vm != nullptr) {
        delete vm;
        vm = nullptr;
        return true;
    }
    return false;
}

EcmaVM::EcmaVM(JSRuntimeOptions options)
    : stringTable_(new EcmaStringTable(this)),
      nativeAreaAllocator_(std::make_unique<NativeAreaAllocator>()),
      heapRegionAllocator_(std::make_unique<HeapRegionAllocator>()),
      chunk_(nativeAreaAllocator_.get()),
      nativePointerList_(&chunk_)
{
    options_ = std::move(options);
    icEnabled_ = options_.EnableIC();
    optionalLogEnabled_ = options_.EnableOptionalLog();
    snapshotFileName_ = options_.GetSnapshotFile().c_str();
    frameworkAbcFileName_ = options_.GetFrameworkAbcFile().c_str();
    options_.ParseAsmInterOption();

    debuggerManager_ = chunk_.New<tooling::JsDebuggerManager>();
}

bool EcmaVM::Initialize()
{
    LOG(INFO, RUNTIME) << "EcmaVM Initialize";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "EcmaVM::Initialize");
    Taskpool::GetCurrentTaskpool()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeStubs::Initialize(thread_);
#endif
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    regExpParserCache_ = new RegExpParserCache();
    heap_ = new Heap(this);
    heap_->Initialize();
    gcStats_ = chunk_.New<GCStats>(heap_, options_.GetLongPauseTime());
    factory_ = chunk_.New<ObjectFactory>(thread_, heap_, &chunk_);
    if (UNLIKELY(factory_ == nullptr)) {
        LOG_ECMA(FATAL) << "alloc factory_ failed";
        UNREACHABLE();
    }
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    if (!options_.EnableSnapshotDeserialize()) {
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run builtins";
        JSHandle<JSHClass> dynClassClassHandle = factory_->InitClassClass();
        JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaDynClass(*dynClassClassHandle,
                                                                      GlobalEnv::SIZE,
                                                                      JSType::GLOBAL_ENV);
        globalConst->Init(thread_, *dynClassClassHandle);
        globalConstInitialized_ = true;
        JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
        globalEnv->Init(thread_);
        globalEnv_ = globalEnv.GetTaggedValue();
        Builtins builtins;
        builtins.Initialize(globalEnv, thread_);
        if (!WIN_OR_MAC_PLATFORM && options_.EnableSnapshotSerialize()) {
            const CString fileName = "builtins.snapshot";
            Snapshot snapshot(this);
            snapshot.SerializeBuiltins(fileName);
        }
    } else {
        const CString fileName = "builtins.snapshot";
        Snapshot snapshot(this);
        if (!WIN_OR_MAC_PLATFORM) {
            snapshot.Deserialize(SnapshotType::BUILTINS, fileName, true);
        }
        globalConst->InitSpecialForSnapshot();
        Builtins builtins;
        builtins.InitializeForSnapshot(thread_);
    }

    SetupRegExpResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    factory_->GenerateInternalNativeMethods();
    thread_->SetGlobalObject(GetGlobalEnv()->GetGlobalObject());
    moduleManager_ = new ModuleManager(this);
    debuggerManager_->Initialize();
    tsLoader_ = new TSLoader(this);
    snapshotEnv_ = new SnapshotEnv(this);
    fileLoader_ = new FileLoader(this);
    if (options_.GetEnableAsmInterpreter()) {
        LoadStubFile();
    }
    if (options_.GetEnableAsmInterpreter() && options_.WasAOTOutputFileSet()) {
        LoadAOTFiles();
    }
    InitializeFinish();
    return true;
}

void EcmaVM::InitializeEcmaScriptRunStat()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char *runtimeCallerNames[] = {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_NAME(name) "Interpreter::" #name,
    INTERPRETER_CALLER_LIST(INTERPRETER_CALLER_NAME)  // NOLINTNEXTLINE(bugprone-suspicious-missing-comma)
#undef INTERPRETER_CALLER_NAME
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_NAME(class, name) "BuiltinsApi::" #class "_" #name,
    BUILTINS_API_LIST(BUILTINS_API_NAME)
#undef BUILTINS_API_NAME
#define ABSTRACT_OPERATION_NAME(class, name) "AbstractOperation::" #class "_" #name,
    ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_NAME)
#undef ABSTRACT_OPERATION_NAME
#define MEM_ALLOCATE_AND_GC_NAME(name) "Memory::" #name,
    MEM_ALLOCATE_AND_GC_LIST(MEM_ALLOCATE_AND_GC_NAME)
#undef MEM_ALLOCATE_AND_GC_NAME
#define DEF_RUNTIME_ID(name) "Runtime::" #name,
    RUNTIME_STUB_WITH_GC_LIST(DEF_RUNTIME_ID)
#undef DEF_RUNTIME_ID
    };
    static_assert(sizeof(runtimeCallerNames) == sizeof(const char *) * ecmascript::RUNTIME_CALLER_NUMBER,
                  "Invalid runtime caller number");
    runtimeStat_ = chunk_.New<EcmaRuntimeStat>(runtimeCallerNames, ecmascript::RUNTIME_CALLER_NUMBER);
    if (UNLIKELY(runtimeStat_ == nullptr)) {
        LOG_ECMA(FATAL) << "alloc runtimeStat_ failed";
        UNREACHABLE();
    }
}

void EcmaVM::SetRuntimeStatEnable(bool flag)
{
    static uint64_t start = 0;
    if (flag) {
        start = PandaRuntimeTimer::Now();
        if (runtimeStat_ == nullptr) {
            InitializeEcmaScriptRunStat();
        }
    } else {
        LOG(INFO, RUNTIME) << "Runtime State duration:" << PandaRuntimeTimer::Now() - start << "(ns)";
        if (runtimeStat_->IsRuntimeStatEnabled()) {
            runtimeStat_->Print();
            runtimeStat_->ResetAllCount();
        }
    }
    runtimeStat_->SetRuntimeStatEnabled(flag);
}

bool EcmaVM::InitializeFinish()
{
    vmInitialized_ = true;
    return true;
}

EcmaVM::~EcmaVM()
{
    LOG(INFO, RUNTIME) << "Destruct ecma_vm, vm address is: " << this;
    vmInitialized_ = false;
    Taskpool::GetCurrentTaskpool()->Destroy();

    if (runtimeStat_ != nullptr && runtimeStat_->IsRuntimeStatEnabled()) {
        runtimeStat_->Print();
    }

    // clear c_address: c++ pointer delete
    ClearBufferData();

    if (gcStats_ != nullptr) {
        if (options_.EnableGCStatsPrint()) {
            gcStats_->PrintStatisticResult(true);
        }
        chunk_.Delete(gcStats_);
        gcStats_ = nullptr;
    }

    if (heap_ != nullptr) {
        heap_->Destroy();
        delete heap_;
        heap_ = nullptr;
    }

    delete regExpParserCache_;
    regExpParserCache_ = nullptr;

    if (debuggerManager_ != nullptr) {
        chunk_.Delete(debuggerManager_);
        debuggerManager_ = nullptr;
    }

    if (factory_ != nullptr) {
        chunk_.Delete(factory_);
        factory_ = nullptr;
    }

    if (stringTable_ != nullptr) {
        delete stringTable_;
        stringTable_ = nullptr;
    }

    if (runtimeStat_ != nullptr) {
        chunk_.Delete(runtimeStat_);
        runtimeStat_ = nullptr;
    }

    if (moduleManager_ != nullptr) {
        delete moduleManager_;
        moduleManager_ = nullptr;
    }

    if (tsLoader_ != nullptr) {
        delete tsLoader_;
        tsLoader_ = nullptr;
    }

    if (snapshotEnv_ != nullptr) {
        delete snapshotEnv_;
        snapshotEnv_ = nullptr;
    }

    if (fileLoader_ != nullptr) {
        delete fileLoader_;
        fileLoader_  = nullptr;
    }

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
}

JSHandle<GlobalEnv> EcmaVM::GetGlobalEnv() const
{
    return JSHandle<GlobalEnv>(reinterpret_cast<uintptr_t>(&globalEnv_));
}

JSHandle<job::MicroJobQueue> EcmaVM::GetMicroJobQueue() const
{
    return JSHandle<job::MicroJobQueue>(reinterpret_cast<uintptr_t>(&microJobQueue_));
}

EcmaVM::CpuProfilingScope::CpuProfilingScope(EcmaVM* vm) : vm_(vm), profiler_(nullptr)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    JSRuntimeOptions options = vm_->GetJSOptions();
    if (options.EnableCpuProfiler()) {
        profiler_ = CpuProfiler::GetInstance();
        profiler_->CpuProfiler::StartCpuProfilerForFile(vm, "");
    }
#endif
}

EcmaVM::CpuProfilingScope::~CpuProfilingScope()
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (profiler_ != nullptr) {
        profiler_->CpuProfiler::StopCpuProfilerForFile();
    }
#endif
}

JSTaggedValue EcmaVM::InvokeEcmaAotEntrypoint(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile)
{
    fileLoader_->UpdateJSMethods(mainFunc, jsPandaFile);
    std::vector<JSTaggedType> args(6, JSTaggedValue::Undefined().GetRawData()); // 6: number of para
    args[0] = mainFunc.GetTaggedValue().GetRawData();
    auto entry = thread_->GetRTInterface(kungfu::RuntimeStubCSigns::ID_JSFunctionEntry);
    auto res = reinterpret_cast<JSFunctionEntryType>(entry)(thread_->GetGlueAddr(),
                                                            reinterpret_cast<uintptr_t>(thread_->GetCurrentSPFrame()),
                                                            static_cast<uint32_t>(args.size()),
                                                            static_cast<uint32_t>(args.size()),
                                                            args.data(),
                                                            mainFunc->GetCodeEntry());
    return JSTaggedValue(res);
}

Expected<JSTaggedValue, bool> EcmaVM::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile)
{
    JSTaggedValue result;
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program;
    if (jsPandaFile != frameworkPandaFile_) {
        program = JSPandaFileManager::GetInstance()->GenerateProgram(this, jsPandaFile);
    } else {
        program = JSHandle<Program>(thread_, frameworkProgram_);
        frameworkProgram_ = JSTaggedValue::Hole();
    }
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(false);
    }
    // for debugger
    debuggerManager_->GetNotificationManager()->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc());

    JSHandle<JSFunction> func = JSHandle<JSFunction>(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    if (jsPandaFile->IsModule()) {
        global = JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        JSHandle<SourceTextModule> module = moduleManager_->HostGetImportedModule(jsPandaFile->GetJSPandaFileDesc());
        func->SetModule(thread_, module);
    }

    if (jsPandaFile->IsLoadedAOT()) {
        thread_->SetPrintBCOffset(true);
        result = InvokeEcmaAotEntrypoint(func, jsPandaFile);
    } else {
        if (jsPandaFile->IsCjs()) {
            CJSExecution(func, jsPandaFile);
        } else {
            JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo info =
                EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);
            EcmaRuntimeStatScope runtimeStatScope(this);
            CpuProfilingScope profilingScope(this);
            EcmaInterpreter::Execute(&info);
        }
    }
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
    }

    // print exception information
    if (thread_->HasPendingException()) {
        auto exception = thread_->GetException();
        HandleUncaughtException(exception.GetTaggedObject());
    }
    return result;
}

JSTaggedValue EcmaVM::FindConstpool(const JSPandaFile *jsPandaFile)
{
    auto iter = cachedConstpools_.find(jsPandaFile);
    if (iter == cachedConstpools_.end()) {
        return JSTaggedValue::Hole();
    }
    return iter->second;
}

void EcmaVM::CJSExecution(JSHandle<JSFunction> &func, const JSPandaFile *jsPandaFile)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    ObjectFactory *factory = GetFactory();

    // create "module", "exports", "require", "filename", "dirname"
    JSHandle<JSCjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<JSCjsExports> exports = factory->NewCjsExports();
    JSMutableHandle<JSTaggedValue> filename(thread_, JSTaggedValue::Undefined());;
    JSMutableHandle<JSTaggedValue> dirname(thread_, JSTaggedValue::Undefined());;
    JSRequireManager::ResolveCurrentPath(thread_, dirname, filename, jsPandaFile);
    CJSInfo cjsInfo(module, require, exports, filename, dirname);
    JSRequireManager::InitializeCommonJS(thread_, cjsInfo);

    // Execute main function
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_,
                                            JSHandle<JSTaggedValue>(func),
                                            global, undefined, 5); // 5 : argument numbers
    info.SetCallArg(cjsInfo.exportsHdl.GetTaggedValue(),
                    cjsInfo.requireHdl.GetTaggedValue(),
                    cjsInfo.moduleHdl.GetTaggedValue(),
                    cjsInfo.filenameHdl.GetTaggedValue(),
                    cjsInfo.dirnameHdl.GetTaggedValue());
    EcmaRuntimeStatScope runtimeStatScope(this);
    CpuProfilingScope profilingScope(this);
    EcmaInterpreter::Execute(&info);

    // Collecting module.exports : exports ---> module.exports --->Module._cache
    JSRequireManager::CollectExecutedExp(thread_, cjsInfo);
    return;
}

void EcmaVM::SetConstpool(const JSPandaFile *jsPandaFile, JSTaggedValue constpool)
{
    ASSERT(constpool.IsTaggedArray());
    ASSERT(cachedConstpools_.find(jsPandaFile) == cachedConstpools_.end());

    cachedConstpools_[jsPandaFile] = constpool;
}

JSHandle<JSTaggedValue> EcmaVM::GetAndClearEcmaUncaughtException() const
{
    JSHandle<JSTaggedValue> exceptionHandle = GetEcmaUncaughtException();
    thread_->ClearException();  // clear for ohos app
    return exceptionHandle;
}

JSHandle<JSTaggedValue> EcmaVM::GetEcmaUncaughtException() const
{
    if (thread_->GetException().IsHole()) {
        return JSHandle<JSTaggedValue>();
    }
    JSHandle<JSTaggedValue> exceptionHandle(thread_, thread_->GetException());
    return exceptionHandle;
}

void EcmaVM::EnableUserUncaughtErrorHandler()
{
    isUncaughtExceptionRegistered_ = true;
}

void EcmaVM::HandleUncaughtException(ObjectHeader *exception)
{
    if (isUncaughtExceptionRegistered_) {
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread_);
    JSHandle<JSTaggedValue> exceptionHandle(thread_, JSTaggedValue(exception));
    // if caught exceptionHandle type is JSError
    thread_->ClearException();
    if (exceptionHandle->IsJSError()) {
        PrintJSErrorInfo(exceptionHandle);
        if (thread_->IsPrintBCOffset() && exceptionBCList_.size() != 0) {
            for (auto info : exceptionBCList_) {
                LOG(ERROR, RUNTIME) << "Exception at function " << info.first << ": " << info.second;
            }
        }
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread_, exceptionHandle);
    CString string = ConvertToString(*result);
    LOG(ERROR, RUNTIME) << string;
}

void EcmaVM::PrintJSErrorInfo(const JSHandle<JSTaggedValue> &exceptionInfo)
{
    JSHandle<JSTaggedValue> nameKey = thread_->GlobalConstants()->GetHandledNameString();
    JSHandle<EcmaString> name(JSObject::GetProperty(thread_, exceptionInfo, nameKey).GetValue());
    JSHandle<JSTaggedValue> msgKey = thread_->GlobalConstants()->GetHandledMessageString();
    JSHandle<EcmaString> msg(JSObject::GetProperty(thread_, exceptionInfo, msgKey).GetValue());
    JSHandle<JSTaggedValue> stackKey = thread_->GlobalConstants()->GetHandledStackString();
    JSHandle<EcmaString> stack(JSObject::GetProperty(thread_, exceptionInfo, stackKey).GetValue());

    CString nameBuffer = ConvertToString(*name);
    CString msgBuffer = ConvertToString(*msg);
    CString stackBuffer = ConvertToString(*stack);
    LOG(ERROR, RUNTIME) << nameBuffer << ": " << msgBuffer << "\n" << stackBuffer;
}

void EcmaVM::ProcessNativeDelete(const WeakRootVisitor &v0)
{
    auto iter = nativePointerList_.begin();
    while (iter != nativePointerList_.end()) {
        JSNativePointer *object = *iter;
        auto fwd = v0(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            object->Destroy();
            iter = nativePointerList_.erase(iter);
        } else {
            ++iter;
        }
    }
}
void EcmaVM::ProcessReferences(const WeakRootVisitor &v0)
{
    if (regExpParserCache_ != nullptr) {
        regExpParserCache_->Clear();
    }

    // array buffer
    for (auto iter = nativePointerList_.begin(); iter != nativePointerList_.end();) {
        JSNativePointer *object = *iter;
        auto fwd = v0(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            object->Destroy();
            iter = nativePointerList_.erase(iter);
            continue;
        }
        if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            *iter = JSNativePointer::Cast(fwd);
        }
        ++iter;
    }

    // program maps
    for (auto iter = cachedConstpools_.begin(); iter != cachedConstpools_.end();) {
        auto object = iter->second;
        if (object.IsHeapObject()) {
            TaggedObject *obj = object.GetTaggedObject();
            auto fwd = v0(obj);
            if (fwd == nullptr) {
                iter = cachedConstpools_.erase(iter);
                continue;
            } else if (fwd != obj) {
                iter->second = JSTaggedValue(fwd);
            }
        }
        ++iter;
    }
}

void EcmaVM::PushToNativePointerList(JSNativePointer *array)
{
    nativePointerList_.emplace_back(array);
}

void EcmaVM::RemoveFromNativePointerList(JSNativePointer *array)
{
    auto iter = std::find(nativePointerList_.begin(), nativePointerList_.end(), array);
    if (iter != nativePointerList_.end()) {
        JSNativePointer *object = *iter;
        object->Destroy();
        nativePointerList_.erase(iter);
    }
}

void EcmaVM::ClearBufferData()
{
    for (auto iter : nativePointerList_) {
        iter->Destroy();
    }
    nativePointerList_.clear();

    cachedConstpools_.clear();
}

bool EcmaVM::ExecutePromisePendingJob()
{
    if (isProcessingPendingJob_) {
        LOG(ERROR, RUNTIME) << "EcmaVM::ExecutePromisePendingJob can not reentrant";
        return false;
    }
    if (!thread_->HasPendingException()) {
        isProcessingPendingJob_ = true;
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
        isProcessingPendingJob_ = false;
        return true;
    }
    return false;
}

void EcmaVM::CollectGarbage(TriggerGCType gcType) const
{
    heap_->CollectGarbage(gcType);
}

void EcmaVM::StartHeapTracking(HeapTracker *tracker)
{
    heap_->StartHeapTracking(tracker);
}

void EcmaVM::StopHeapTracking()
{
    heap_->StopHeapTracking();
}

void EcmaVM::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalEnv_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&microJobQueue_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&frameworkProgram_)));
    moduleManager_->Iterate(v);
    tsLoader_->Iterate(v);
    fileLoader_->Iterate(v);
    if (!WIN_OR_MAC_PLATFORM) {
        snapshotEnv_->Iterate(v);
    }
}

void EcmaVM::SetGlobalEnv(GlobalEnv *global)
{
    ASSERT(global != nullptr);
    globalEnv_ = JSTaggedValue(global);
}

void EcmaVM::SetMicroJobQueue(job::MicroJobQueue *queue)
{
    ASSERT(queue != nullptr);
    microJobQueue_ = JSTaggedValue(queue);
}

void EcmaVM::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}

void EcmaVM::LoadStubFile()
{
    std::string file = options_.GetStubFile();
    LOG(INFO, RUNTIME) << "Try to load stub file" << file.c_str();
    fileLoader_->LoadStubFile(file);
}

void EcmaVM::LoadAOTFiles()
{
    std::string file = options_.GetAOTOutputFile();
    LOG(INFO, RUNTIME) << "Try to load aot file" << file.c_str();
    fileLoader_->LoadAOTFile(file);
    fileLoader_->TryLoadSnapshotFile();
}

void EcmaVM::SetAOTFuncEntry(uint32_t hash, uint32_t methodId, uint64_t funcEntry)
{
    fileLoader_->SetAOTFuncEntry(hash, methodId, funcEntry);
}
}  // namespace panda::ecmascript
