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

#include "ecmascript/ecma_vm.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/class_linker/panda_file_translator.h"
#include "ecmascript/class_linker/program_object-inl.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/dfx/vmstat/runtime_stat.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/platform/platform.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/runtime_call_id.h"
#ifndef PANDA_TARGET_WINDOWS
#include "ecmascript/runtime_trampolines.h"
#endif
#include "ecmascript/snapshot/mem/slot_bit.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue-inl.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/template_map.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "include/runtime_notification.h"
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
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static const std::string_view ENTRY_POINTER = "_GLOBAL::func_main_0";
JSRuntimeOptions EcmaVM::options_;  // NOLINT(fuchsia-statically-constructed-objects)

/* static */
EcmaVM *EcmaVM::Create(const JSRuntimeOptions &options)
{
    auto runtime = Runtime::GetCurrent();
    auto vm = runtime->GetInternalAllocator()->New<EcmaVM>(options);
    if (UNLIKELY(vm == nullptr)) {
        LOG_ECMA(ERROR) << "Failed to create jsvm";
        return nullptr;
    }
    auto jsThread = JSThread::Create(runtime, vm);
    vm->thread_ = jsThread;
    vm->Initialize();
    return vm;
}

// static
bool EcmaVM::Destroy(PandaVM *vm)
{
    if (vm != nullptr) {
        auto runtime = Runtime::GetCurrent();
        runtime->GetInternalAllocator()->Delete(vm);
        return true;
    }
    return false;
}

// static
Expected<EcmaVM *, CString> EcmaVM::Create(Runtime *runtime)
{
    EcmaVM *vm = runtime->GetInternalAllocator()->New<EcmaVM>();
    auto jsThread = ecmascript::JSThread::Create(runtime, vm);
    vm->thread_ = jsThread;
    return vm;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
EcmaVM::EcmaVM() : EcmaVM(EcmaVM::GetJSOptions())
{
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
    rendezvous_ = chunk_.New<EmptyRendezvous>();
    snapshotSerializeEnable_ = options_.IsSnapshotSerializeEnabled();
    if (!snapshotSerializeEnable_) {
        snapshotDeserializeEnable_ = options_.IsSnapshotDeserializeEnabled();
    }
    snapshotFileName_ = options_.GetSnapshotFile();
    frameworkAbcFileName_ = options_.GetFrameworkAbcFile();

    auto runtime = Runtime::GetCurrent();
    notificationManager_ = chunk_.New<RuntimeNotificationManager>(runtime->GetInternalAllocator());
    notificationManager_->SetRendezvous(rendezvous_);
}

bool EcmaVM::Initialize()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "EcmaVM::Initialize");
    Platform::GetCurrentPlatform()->Initialize();
#ifndef PANDA_TARGET_WINDOWS
    RuntimeTrampolines::InitializeRuntimeTrampolines(thread_);
#endif

    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    regExpParserCache_ = new RegExpParserCache();
    heap_ = new Heap(this);
    heap_->Initialize();
    gcStats_ = chunk_.New<GCStats>(heap_);
    factory_ = chunk_.New<ObjectFactory>(thread_, heap_);
    if (UNLIKELY(factory_ == nullptr)) {
        LOG_ECMA(FATAL) << "alloc factory_ failed";
        UNREACHABLE();
    }

    [[maybe_unused]] EcmaHandleScope scope(thread_);
    if (!snapshotDeserializeEnable_ || !VerifyFilePath(snapshotFileName_)) {
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run builtins";

        JSHandle<JSHClass> dynClassClassHandle =
            factory_->NewEcmaDynClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
        JSHClass *dynclass = reinterpret_cast<JSHClass *>(dynClassClassHandle.GetTaggedValue().GetTaggedObject());
        dynclass->SetClass(dynclass);
        JSHandle<JSHClass> globalEnvClass =
            factory_->NewEcmaDynClass(*dynClassClassHandle, GlobalEnv::SIZE, JSType::GLOBAL_ENV);

        JSHandle<GlobalEnv> globalEnvHandle = factory_->NewGlobalEnv(*globalEnvClass);
        globalEnv_ = globalEnvHandle.GetTaggedValue();
        auto globalEnv = GlobalEnv::Cast(globalEnv_.GetTaggedObject());

        // init global env
        globalConst->InitRootsClass(thread_, *dynClassClassHandle);
        globalConst->InitGlobalConstant(thread_);
        globalEnv->SetEmptyArray(thread_, factory_->NewEmptyArray());
        globalEnv->SetEmptyLayoutInfo(thread_, factory_->CreateLayoutInfo(0));
        globalEnv->SetRegisterSymbols(thread_, SymbolTable::Create(thread_));
        globalEnv->SetGlobalRecord(thread_, GlobalDictionary::Create(thread_));
        JSTaggedValue emptyStr = thread_->GlobalConstants()->GetEmptyString();
        stringTable_->InternEmptyString(EcmaString::Cast(emptyStr.GetTaggedObject()));
        globalEnv->SetEmptyTaggedQueue(thread_, factory_->NewTaggedQueue(0));
        globalEnv->SetTemplateMap(thread_, TemplateMap::Create(thread_));
        globalEnv->SetRegisterSymbols(GetJSThread(), SymbolTable::Create(GetJSThread()));
#ifdef ECMASCRIPT_ENABLE_STUB_AOT
        std::string moduleFile = options_.GetStubModuleFile();
        thread_->LoadStubModule(moduleFile.c_str());
#endif
        SetupRegExpResultCache();
        microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();
        {
            Builtins builtins;
            builtins.Initialize(globalEnvHandle, thread_);
        }
    } else {
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run snapshot";
        SnapShot snapShot(this);
        frameworkPandaFile_ = snapShot.DeserializeGlobalEnvAndProgram(snapshotFileName_, frameworkAbcFileName_);
        globalConst->InitGlobalUndefined();
#else
        LOG_ECMA(FATAL) << "Don't support snapshot now.";
#endif
    }

    thread_->SetGlobalObject(GetGlobalEnv()->GetGlobalObject());
    moduleManager_ = new ModuleManager(this);
    tsLoader_ = new TSLoader(this);
    InitializeFinish();
    notificationManager_->VmStartEvent();
    notificationManager_->VmInitializationEvent(thread_->GetThreadId());
    return true;
}

JSPandaFileManager *EcmaVM::GetJSPandaFileManager()
{
    static JSPandaFileManager jsFileManager;
    return &jsFileManager;
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
    BUITINS_API_LIST(BUILTINS_API_NAME)
#undef BUILTINS_API_NAME
#define ABSTRACT_OPERATION_NAME(class, name) "AbstractOperation::" #class "_" #name,
    ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_NAME)
#undef ABSTRACT_OPERATION_NAME
#define MEM_ALLOCATE_AND_GC_NAME(name) "Memory::" #name,
    MEM_ALLOCATE_AND_GC_LIST(MEM_ALLOCATE_AND_GC_NAME)
#undef MEM_ALLOCATE_AND_GC_NAME
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
    Platform::GetCurrentPlatform()->Destroy();
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

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
}

bool EcmaVM::ExecuteFromPf(const std::string &filename, std::string_view entryPoint,
                           const std::vector<std::string> &args, bool isModule)
{
    const JSPandaFile *jsPandaFile = GetJSPandaFileManager()->LoadJSPandaFile(filename);
    if (jsPandaFile == nullptr) {
        return false;
    }

    return Execute(jsPandaFile, entryPoint, args);
}

bool EcmaVM::CollectInfoOfPandaFile(const std::string &filename, std::vector<BytecodeTranslationInfo> *infoList)
{
    const JSPandaFile *jsPandaFile = GetJSPandaFileManager()->LoadAotInfoFromPf(filename, infoList);
    if (jsPandaFile == nullptr) {
        return false;
    }

    if (GetJSOptions().IsEnableTsAot()) {
        TSLoader *tsLoader = GetTSLoader();
        tsLoader->DecodeTSTypes(*jsPandaFile->GetPandaFile());
    }
    return true;
}

bool EcmaVM::ExecuteFromBuffer(const void *buffer, size_t size, std::string_view entryPoint,
                               const std::vector<std::string> &args, const std::string &filename)
{
    const JSPandaFile *jsPandaFile = GetJSPandaFileManager()->LoadJSPandaFile(filename, buffer, size);
    if (jsPandaFile == nullptr) {
        return false;
    }
    // Get ClassName and MethodName
    size_t pos = entryPoint.find_last_of("::");
    if (pos == std::string_view::npos) {
        LOG_ECMA(ERROR) << "EntryPoint:" << entryPoint << " is illegal";
        return false;
    }
    CString methodName(entryPoint.substr(pos + 1));
    InvokeEcmaEntrypoint(jsPandaFile, methodName, args);
    return true;
}

tooling::ecmascript::PtJSExtractor *EcmaVM::GetDebugInfoExtractor(const panda_file::File *file)
{
    tooling::ecmascript::PtJSExtractor *res = GetJSPandaFileManager()->GetOrCreatePtJSExtractor(file);
    return res;
}

bool EcmaVM::Execute(const JSPandaFile *jsPandaFile, std::string_view entryPoint, const std::vector<std::string> &args)
{
    // Get ClassName and MethodName
    size_t pos = entryPoint.find_last_of("::");
    if (pos == std::string_view::npos) {
        LOG_ECMA(ERROR) << "EntryPoint:" << entryPoint << " is illegal";
        return false;
    }
    CString methodName(entryPoint.substr(pos + 1));
    // For Ark application startup
    InvokeEcmaEntrypoint(jsPandaFile, methodName, args);
    return true;
}

JSHandle<GlobalEnv> EcmaVM::GetGlobalEnv() const
{
    return JSHandle<GlobalEnv>(reinterpret_cast<uintptr_t>(&globalEnv_));
}

JSHandle<job::MicroJobQueue> EcmaVM::GetMicroJobQueue() const
{
    return JSHandle<job::MicroJobQueue>(reinterpret_cast<uintptr_t>(&microJobQueue_));
}

JSMethod *EcmaVM::GetMethodForNativeFunction(const void *func)
{
    // signature: any foo(any function_obj, any this)
    uint32_t accessFlags = ACC_PUBLIC | ACC_STATIC | ACC_FINAL | ACC_NATIVE;
    uint32_t numArgs = 2;  // function object and this

    auto method = chunk_.New<JSMethod>(nullptr, nullptr, panda_file::File::EntityId(0), panda_file::File::EntityId(0),
                                       accessFlags, numArgs, nullptr);
    method->SetNativePointer(const_cast<void *>(func));
    method->SetNativeBit(true);

    nativeMethods_.push_back(method);
    return nativeMethods_.back();
}

Expected<int, Runtime::Error> EcmaVM::InvokeEcmaEntrypoint(const JSPandaFile *jsPandaFile,
                                                           [[maybe_unused]] const CString &methodName,
                                                           const std::vector<std::string> &args)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program;
    if (snapshotSerializeEnable_) {
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
        program = GetJSPandaFileManager()->GenerateProgram(this, jsPandaFile);

        auto index = jsPandaFile->GetJSPandaFileDesc().find(frameworkAbcFileName_);
        if (index != CString::npos) {
            LOG_ECMA(DEBUG) << "snapShot MakeSnapShotProgramObject abc " << jsPandaFile->GetJSPandaFileDesc();
            SnapShot snapShot(this);
            snapShot.MakeSnapShotProgramObject(*program, jsPandaFile->GetPandaFile(), snapshotFileName_);
#else
        LOG_ECMA(FATAL) << "Don't support snapshot now.";
#endif
        }
    } else {
        if (jsPandaFile != frameworkPandaFile_) {
            program = GetJSPandaFileManager()->GenerateProgram(this, jsPandaFile);
        } else {
            program = JSHandle<Program>(thread_, frameworkProgram_);
            frameworkProgram_ = JSTaggedValue::Hole();
        }
    }
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(Runtime::Error::PANDA_FILE_LOAD_ERROR);
    }
    // for debugger
    notificationManager_->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc());

    JSHandle<JSFunction> func = JSHandle<JSFunction>(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> newTarget(thread_, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> jsargs = factory_->NewTaggedArray(args.size());
    uint32_t i = 0;
    for (const std::string &str : args) {
        JSHandle<JSTaggedValue> strobj(factory_->NewFromStdString(str));
        jsargs->Set(thread_, i++, strobj);
    }

    InternalCallParams *params = thread_->GetInternalCallParams();
    params->MakeArgList(*jsargs);
    JSRuntimeOptions options = this->GetJSOptions();
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
    if (options.IsEnableCpuProfiler()) {
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        CpuProfiler *profiler = CpuProfiler::GetInstance();
        profiler->CpuProfiler::StartCpuProfiler(this, "");
        panda::ecmascript::InvokeJsFunction(thread_, func, global, newTarget, params);
        profiler->CpuProfiler::StopCpuProfiler();
#endif
    } else {
        panda::ecmascript::InvokeJsFunction(thread_, func, global, newTarget, params);
    }
    if (!thread_->HasPendingException()) {
        job::MicroJobQueue::ExecutePendingJob(thread_, GetMicroJobQueue());
    }

    // print exception information
    if (thread_->HasPendingException()) {
        auto exception = thread_->GetException();
        HandleUncaughtException(exception.GetTaggedObject());
    }
    return 0;
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
    [[maybe_unused]] EcmaHandleScope handle_scope(thread_);
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
        } else if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            *iter = JSNativePointer::Cast(fwd);
            ++iter;
        } else {
            ++iter;
        }
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

JSHandle<JSTaggedValue> EcmaVM::GetModuleByName(JSHandle<JSTaggedValue> moduleName)
{
    // only used in testcase, pandaFileWithProgram_ only one item. this interface will delete
    const JSPandaFile *currentJSPandaFile = nullptr;
    GetJSPandaFileManager()->EnumerateJSPandaFiles([&currentJSPandaFile](const JSPandaFile *jsPandaFile) {
        currentJSPandaFile = jsPandaFile;
        return false;
    });
    ASSERT(currentJSPandaFile != nullptr);
    CString currentPathFile = currentJSPandaFile->GetJSPandaFileDesc();
    CString relativeFile = ConvertToString(EcmaString::Cast(moduleName->GetTaggedObject()));

    // generate full path
    CString abcPath = moduleManager_->GenerateAmiPath(currentPathFile, relativeFile);

    // Uniform module name
    JSHandle<EcmaString> abcModuleName = factory_->NewFromString(abcPath);

    JSHandle<JSTaggedValue> module = moduleManager_->GetModule(thread_, JSHandle<JSTaggedValue>::Cast(abcModuleName));
    if (module->IsUndefined()) {
        std::string file = base::StringHelper::ToStdString(abcModuleName.GetObject<EcmaString>());
        std::vector<std::string> argv;
        ExecuteModule(file, ENTRY_POINTER, argv);
        module = moduleManager_->GetModule(thread_, JSHandle<JSTaggedValue>::Cast(abcModuleName));
    }
    return module;
}

void EcmaVM::ExecuteModule(const std::string &moduleFile, std::string_view entryPoint,
                           const std::vector<std::string> &args)
{
    moduleManager_->SetCurrentExportModuleName(moduleFile);
    // Update Current Module
    EcmaVM::ExecuteFromPf(moduleFile, entryPoint, args, true);
    // Restore Current Module
    moduleManager_->RestoreCurrentExportModuleName();
}

void EcmaVM::ClearNativeMethodsData()
{
    for (auto iter : nativeMethods_) {
        chunk_.Delete(iter);
    }
    nativeMethods_.clear();
}

void EcmaVM::SetupRegExpResultCache()
{
    regexpCache_ = builtins::RegExpExecResultCache::CreateCacheTable(thread_);
}
}  // namespace panda::ecmascript
