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
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/compiler/call_signature.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"
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
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/runtime_call_id.h"
#ifndef PANDA_TARGET_WINDOWS
#include "ecmascript/stubs/runtime_stubs.h"
#endif
#include "ecmascript/snapshot/mem/encode_bit.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tooling/interface/notification_manager.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "libpandafile/file.h"
#ifdef PANDA_TARGET_WINDOWS
#include <shlwapi.h>
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
      nativePointerList_(&chunk_),
      nativeMethods_(&chunk_)
{
    options_ = std::move(options);
    icEnable_ = options_.IsIcEnable();
    optionalLogEnabled_ = options_.IsEnableOptionalLog();
    snapshotSerializeEnable_ = options_.IsSnapshotSerializeEnabled();
    if (!snapshotSerializeEnable_) {
        snapshotDeserializeEnable_ = options_.IsSnapshotDeserializeEnabled();
    }
    snapshotFileName_ = options_.GetSnapshotFile().c_str();
    frameworkAbcFileName_ = options_.GetFrameworkAbcFile().c_str();
    options_.ParseAsmInterOption();

    notificationManager_ = chunk_.New<tooling::NotificationManager>();
    debuggerManager_ = chunk_.New<tooling::JsDebuggerManager>();
}

void EcmaVM::TryLoadSnapshotFile()
{
    if (VerifyFilePath("snapshot")) {
        SnapShot snapShot(this);
        snapShot.SnapShotDeserialize(SnapShotType::TS_LOADER, "snapshot");
    }
}

bool EcmaVM::Initialize()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "EcmaVM::Initialize");
    Taskpool::GetCurrentTaskpool()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeStubs::Initialize(thread_);
#endif
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    regExpParserCache_ = new RegExpParserCache();
    MemMapAllocator::GetInstance()->Initialize(options_.TotalSpaceCapacity(), DEFAULT_REGION_SIZE);
    heap_ = new Heap(this);
    heap_->Initialize();
    gcStats_ = chunk_.New<GCStats>(heap_);
    factory_ = chunk_.New<ObjectFactory>(thread_, heap_);
    if (UNLIKELY(factory_ == nullptr)) {
        LOG_ECMA(FATAL) << "alloc factory_ failed";
        UNREACHABLE();
    }
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    LOG_ECMA(DEBUG) << "EcmaVM::Initialize run builtins";
    JSHandle<JSHClass> dynClassClassHandle = factory_->InitClassClass();
    JSHandle<JSHClass> globalEnvClass = factory_->NewEcmaDynClass(*dynClassClassHandle,
                                                                  GlobalEnv::SIZE,
                                                                  JSType::GLOBAL_ENV);
    globalConst->InitRootsClass(thread_, *dynClassClassHandle);
    tsLoader_ = new TSLoader(this);
    aotInfo_ = new AotCodeInfo();
    if (options_.EnableTSAot()) {
        TryLoadSnapshotFile();
        std::string file = options_.GetAOTOutputFile();
        LoadAOTFile(file);
    }
    globalConst->InitGlobalConstant(thread_);
    JSHandle<GlobalEnv> globalEnv = factory_->NewGlobalEnv(*globalEnvClass);
    globalEnv->Init(thread_);
    globalEnv_ = globalEnv.GetTaggedValue();
    if (options_.IsEnableStubAot()) {
        LoadStubs();
    }
    SetupRegExpResultCache();
    microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
    Builtins builtins;
    builtins.Initialize(globalEnv, thread_);
    thread_->SetGlobalObject(GetGlobalEnv()->GetGlobalObject());
    moduleManager_ = new ModuleManager(this);
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
    if (flag) {
        if (runtimeStat_ == nullptr) {
            InitializeEcmaScriptRunStat();
        }
    } else {
        if (runtimeStatEnabled_) {
            runtimeStat_->Print();
            runtimeStat_->ResetAllCount();
        }
    }
    runtimeStatEnabled_ = flag;
}

bool EcmaVM::InitializeFinish()
{
    vmInitialized_ = true;
    return true;
}

EcmaVM::~EcmaVM()
{
    vmInitialized_ = false;
    Taskpool::GetCurrentTaskpool()->Destroy();
    ClearNativeMethodsData();

    if (runtimeStat_ != nullptr && runtimeStatEnabled_) {
        runtimeStat_->Print();
    }

    // clear c_address: c++ pointer delete
    ClearBufferData();

    if (gcStats_ != nullptr) {
        if (options_.IsEnableGCStatsPrint()) {
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

    if (notificationManager_ != nullptr) {
        chunk_.Delete(notificationManager_);
        notificationManager_ = nullptr;
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

    if (aotInfo_ != nullptr) {
        delete aotInfo_;
        aotInfo_  = nullptr;
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
    if (options.IsEnableCpuProfiler()) {
        profiler_ = CpuProfiler::GetInstance();
        profiler_->CpuProfiler::StartCpuProfiler(vm, "");
    }
#endif
}

EcmaVM::CpuProfilingScope::~CpuProfilingScope()
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (profiler_ != nullptr) {
        profiler_->CpuProfiler::StopCpuProfiler();
    }
#endif
}

JSMethod *EcmaVM::GetMethodForNativeFunction(const void *func)
{
    uint32_t numArgs = 2;  // function object and this
    auto method = chunk_.New<JSMethod>(nullptr, panda_file::File::EntityId(0));
    method->SetNativePointer(const_cast<void *>(func));

    method->SetNativeBit(true);
    method->SetNumArgsWithCallField(numArgs);
    nativeMethods_.push_back(method);
    return nativeMethods_.back();
}

JSTaggedValue EcmaVM::InvokeEcmaAotEntrypoint()
{
    const std::string funcName = "func_main_0";
    auto ptr = static_cast<uintptr_t>(aotInfo_->GetAOTFuncEntry(funcName));
    std::vector<JSTaggedType> args(6, JSTaggedValue::Undefined().GetRawData()); // 6: number of para
    auto res = JSFunctionEntry(thread_->GetGlueAddr(),
                               reinterpret_cast<uintptr_t>(thread_->GetCurrentSPFrame()),
                               static_cast<uint32_t>(args.size()),
                               static_cast<uint32_t>(args.size()),
                               args.data(),
                               ptr);
    std::cout << " LoadAOTFile call func_main_0 res: " << res << std::endl;
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
    notificationManager_->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc());

    JSHandle<JSFunction> func = JSHandle<JSFunction>(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    if (jsPandaFile->IsModule()) {
        global = JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        JSHandle<SourceTextModule> module = moduleManager_->HostGetImportedModule(jsPandaFile->GetJSPandaFileDesc());
        func->SetModule(thread_, module);
    }

    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_, JSHandle<JSTaggedValue>(func), global, undefined, 0);

    auto options = GetJSOptions();
    if (options.EnableTSAot()) {
        result = InvokeEcmaAotEntrypoint();
    } else {
        CpuProfilingScope profilingScope(this);
        result = EcmaInterpreter::Execute(&info);
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
        if (object.IsObject()) {
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
    if (std::find(nativePointerList_.begin(), nativePointerList_.end(), array) != nativePointerList_.end()) {
        return;
    }
    nativePointerList_.emplace_back(array);
}

void EcmaVM::RemoveFromNativePointerList(JSNativePointer *array)
{
    auto iter = std::find(nativePointerList_.begin(), nativePointerList_.end(), array);
    if (iter != nativePointerList_.end()) {
        nativePointerList_.erase(iter);
    }
}

// Do not support snapshot on windows
bool EcmaVM::VerifyFilePath(const CString &filePath) const
{
#ifndef PANDA_TARGET_WINDOWS
    if (filePath.size() > PATH_MAX) {
        return false;
    }

    CVector<char> resolvedPath(PATH_MAX);
    auto result = realpath(filePath.c_str(), resolvedPath.data());
    if (result == nullptr) {
        return false;
    }
    std::ifstream file(resolvedPath.data());
    if (!file.good()) {
        return false;
    }
    file.close();
    return true;
#else
    return false;
#endif
}

void EcmaVM::ClearBufferData()
{
    for (auto iter : nativePointerList_) {
        iter->Destroy();
    }
    nativePointerList_.clear();

    cachedConstpools_.clear();
}

bool EcmaVM::ExecutePromisePendingJob() const
{
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
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
    aotInfo_->Iterate(v);
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

void EcmaVM::ClearNativeMethodsData()
{
    for (auto iter : nativeMethods_) {
        chunk_.Delete(iter);
    }
    nativeMethods_.clear();
}

void EcmaVM::LoadStubs()
{
    std::string comStubFile = options_.GetComStubFile();
    thread_->LoadStubsFromFile(comStubFile);
    std::string bcStubFile = options_.GetBcStubFile();
    thread_->LoadStubsFromFile(bcStubFile);
}

void EcmaVM::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}

void EcmaVM::LoadAOTFile(const std::string &fileName)
{
    if (!aotInfo_->Deserialize(this, fileName)) {
        return;
    }
}
}  // namespace panda::ecmascript
