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
#include "ecmascript/cpu_profiler/cpu_profiler.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/jobs/micro_job_queue.h"
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
#include "ecmascript/runtime_trampolines.h"
#include "ecmascript/snapshot/mem/slot_bit.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_serialize.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue-inl.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/template_map.h"
#include "ecmascript/vmstat/runtime_stat.h"
#include "include/runtime_notification.h"
#include "libpandafile/file.h"

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
    isTestMode_ = true;
}

EcmaVM::EcmaVM(JSRuntimeOptions options)
    : stringTable_(new EcmaStringTable(this)),
      regionFactory_(std::make_unique<RegionFactory>()),
      chunk_(regionFactory_.get()),
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
    RuntimeTrampolines::InitializeRuntimeTrampolines(thread_);

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
        factory_->ObtainRootClass(GetGlobalEnv());
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
        thread_->LoadFastStubModule(moduleFile.c_str());
#endif
        SetupRegExpResultCache();
        microJobQueue_ = factory_->NewMicroJobQueue().GetTaggedValue();

        {
            Builtins builtins;
            builtins.Initialize(globalEnvHandle, thread_);
        }
    } else {
        LOG_ECMA(DEBUG) << "EcmaVM::Initialize run snapshot";
        SnapShot snapShot(this);
        std::unique_ptr<const panda_file::File> pf = snapShot.DeserializeGlobalEnvAndProgram(snapshotFileName_);
        frameworkPandaFile_ = pf.get();
        AddPandaFile(pf.release(), false);
        SetProgram(Program::Cast(frameworkProgram_.GetTaggedObject()), frameworkPandaFile_);
        globalConst->InitGlobalUndefined();

        factory_->ObtainRootClass(GetGlobalEnv());
    }

    moduleManager_ = new ModuleManager(this);
    InitializeFinish();
    notificationManager_->VmStartEvent();
    notificationManager_->VmInitializationEvent(thread_->GetThreadId());
    Platform::GetCurrentPlatform()->PostTask(std::make_unique<TrimNewSpaceLimitTask>(heap_));
    return true;
}

bool EcmaVM::TrimNewSpaceLimitTask::Run(uint32_t threadIndex)
{
    for (uint32_t i = 0; i < THREAD_SLEEP_COUNT; i++) {
        if (IsTerminate()) {
            return false;
        }
        usleep(THREAD_SLEEP_TIME);
    }

    if (!IsTerminate() && heap_->GetMemController()->IsDelayGCMode()) {
        heap_->SetFromSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
        heap_->SetNewSpaceMaximumCapacity(SEMI_SPACE_SIZE_CAPACITY);
        heap_->ResetDelayGCMode();
    }
    return true;
}

void EcmaVM::InitializeEcmaScriptRunStat()
{
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    static const char *runtimeCallerNames[] = {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_NAME(name) "InterPreter::" #name,
        INTERPRETER_CALLER_LIST(INTERPRETER_CALLER_NAME)  // NOLINTNEXTLINE(bugprone-suspicious-missing-comma)
#undef INTERPRETER_CALLER_NAME
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_NAME(class, name) "BuiltinsApi::" #class "_" #name,
        BUITINS_API_LIST(BUILTINS_API_NAME)
#undef BUILTINS_API_NAME
#define ABSTRACT_OPERATION_NAME(class, name) "AbstractOperation::" #class "_" #name,
            ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_NAME)
#undef ABSTRACT_OPERATION_NAME
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

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }

    extractorCache_.clear();
    frameworkProgramMethods_.clear();
}

bool EcmaVM::ExecuteFromPf(std::string_view filename, std::string_view entryPoint, const std::vector<std::string> &args,
                           bool isModule)
{
    const panda_file::File *pf_ptr = nullptr;
    if (frameworkPandaFile_ == nullptr || !IsFrameworkPandaFile(filename)) {
        auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
        if (pf == nullptr) {
            return false;
        }
        pf_ptr = pf.get();
        AddPandaFile(pf.release(), isModule);  // Store here prevent from being automatically cleared
    } else {
        pf_ptr = frameworkPandaFile_;
    }

    return Execute(*pf_ptr, entryPoint, args);
}

bool EcmaVM::ExecuteFromBuffer(const void *buffer, size_t size, std::string_view entryPoint,
                               const std::vector<std::string> &args)
{
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        return false;
    }
    const panda_file::File *pf_ptr = pf.get();
    AddPandaFile(pf.release(), false);  // Store here prevent from being automatically cleared

    return Execute(*pf_ptr, entryPoint, args);
}

tooling::ecmascript::PtJSExtractor *EcmaVM::GetDebugInfoExtractor(const panda_file::File *file)
{
    tooling::ecmascript::PtJSExtractor *res = nullptr;
    auto it = extractorCache_.find(file);
    if (it == extractorCache_.end()) {
        auto extractor = std::make_unique<tooling::ecmascript::PtJSExtractor>(file);
        res = extractor.get();
        extractorCache_[file] = std::move(extractor);
    } else {
        res = it->second.get();
    }
    return res;
}

bool EcmaVM::Execute(const panda_file::File &pf, std::string_view entryPoint, const std::vector<std::string> &args)
{
    // Get ClassName and MethodName
    size_t pos = entryPoint.find_last_of("::");
    if (pos == std::string_view::npos) {
        LOG_ECMA(ERROR) << "EntryPoint:" << entryPoint << " is illegal";
        return false;
    }
    CString methodName(entryPoint.substr(pos + 1));

    // For Ark application startup
    InvokeEcmaEntrypoint(pf, methodName, args);
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

    nativeMethods_.push_back(method);
    return nativeMethods_.back();
}

void EcmaVM::RedirectMethod(const panda_file::File &pf)
{
    for (auto method : frameworkProgramMethods_) {
        method->SetPandaFile(&pf);
    }
}

Expected<int, Runtime::Error> EcmaVM::InvokeEntrypointImpl(Method *entrypoint, const std::vector<std::string> &args)
{
    // For testcase startup
    const panda_file::File *file = entrypoint->GetPandaFile();
    AddPandaFile(file, false);
    return InvokeEcmaEntrypoint(*file, utf::Mutf8AsCString(entrypoint->GetName().data), args);
}

Expected<int, Runtime::Error> EcmaVM::InvokeEcmaEntrypoint(const panda_file::File &pf, const CString &methodName,
                                                           const std::vector<std::string> &args)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<Program> program;
    if (snapshotSerializeEnable_) {
        program = PandaFileTranslator::TranslatePandaFile(this, pf, methodName);
        auto string = EcmaString::Cast(program->GetLocation().GetTaggedObject());

        auto index = ConvertToString(string).find(frameworkAbcFileName_);
        if (index != CString::npos) {
            LOG_ECMA(DEBUG) << "snapShot MakeSnapShotProgramObject abc " << ConvertToString(string);
            SnapShot snapShot(this);
            snapShot.MakeSnapShotProgramObject(*program, &pf, snapshotFileName_);
        }
    } else {
        if (&pf != frameworkPandaFile_) {
            program = PandaFileTranslator::TranslatePandaFile(this, pf, methodName);
        } else {
            JSHandle<EcmaString> string = factory_->NewFromStdStringUnCheck(pf.GetFilename(), true);
            program = JSHandle<Program>(thread_, frameworkProgram_);
            program->SetLocation(thread_, string);
            RedirectMethod(pf);
        }
    }

    SetProgram(*program, &pf);
    if (program.IsEmpty()) {
        LOG_ECMA(ERROR) << "program is empty, invoke entrypoint failed";
        return Unexpected(Runtime::Error::PANDA_FILE_LOAD_ERROR);
    }

    JSHandle<JSFunction> func = JSHandle<JSFunction>(thread_, program->GetMainFunction());
    JSHandle<JSTaggedValue> global = GlobalEnv::Cast(globalEnv_.GetTaggedObject())->GetJSGlobalObject();
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
    if (options.IsEnableCpuProfiler()) {
        CpuProfiler *profiler = CpuProfiler::GetInstance();
        profiler->CpuProfiler::StartCpuProfiler(this, "");
        panda::ecmascript::InvokeJsFunction(thread_, func, global, newTarget, params);
        profiler->CpuProfiler::StopCpuProfiler();
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

void EcmaVM::AddPandaFile(const panda_file::File *pf, bool isModule)
{
    ASSERT(pf != nullptr);
    pandaFileWithProgram_.push_back(std::make_tuple(nullptr, pf, isModule));
}

void EcmaVM::SetProgram(Program *program, const panda_file::File *pf)
{
    auto it = std::find_if(pandaFileWithProgram_.begin(), pandaFileWithProgram_.end(),
                           [pf](auto entry) { return std::get<1>(entry) == pf; });
    ASSERT(it != pandaFileWithProgram_.end());
    std::get<0>(*it) = program;
    // for debugger
    notificationManager_->LoadModuleEvent(pf->GetFilename());
}

bool EcmaVM::IsFrameworkPandaFile(std::string_view filename) const
{
    return filename.size() >= frameworkAbcFileName_.size() &&
           filename.substr(filename.size() - frameworkAbcFileName_.size()) == frameworkAbcFileName_;
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
    for (auto iter = arrayBufferDataList_.begin(); iter != arrayBufferDataList_.end();) {
        JSNativePointer *object = *iter;
        auto fwd = v0(reinterpret_cast<TaggedObject *>(object));
        if (fwd == nullptr) {
            object->Destroy();
            iter = arrayBufferDataList_.erase(iter);
        } else if (fwd != reinterpret_cast<TaggedObject *>(object)) {
            *iter = JSNativePointer::Cast(fwd);
            ++iter;
        } else {
            ++iter;
        }
    }

    // program vector
    for (auto iter = pandaFileWithProgram_.begin(); iter != pandaFileWithProgram_.end();) {
        auto object = std::get<0>(*iter);
        if (object != nullptr) {
            auto fwd = v0(object);
            if (fwd == nullptr) {
                object->FreeMethodData(regionFactory_.get());
                auto pf = std::get<1>(*iter);
                extractorCache_.erase(pf);
                delete pf;
                iter = pandaFileWithProgram_.erase(iter);
            } else if (fwd != object) {
                *iter = std::make_tuple(reinterpret_cast<Program *>(fwd), std::get<1>(*iter),
                                        std::get<2>(*iter));  // 2: index
                ++iter;
            } else {
                ++iter;
            }
        } else {
            ++iter;
        }
    }

    // framework program
    if (!frameworkProgram_.IsHole()) {
        auto fwd = v0(frameworkProgram_.GetTaggedObject());
        if (fwd == nullptr) {
            frameworkProgram_ = JSTaggedValue::Undefined();
        } else if (fwd != frameworkProgram_.GetTaggedObject()) {
            frameworkProgram_ = JSTaggedValue(fwd);
        }
    }
}

void EcmaVM::PushToArrayDataList(JSNativePointer *array)
{
    if (std::find(arrayBufferDataList_.begin(), arrayBufferDataList_.end(), array) != arrayBufferDataList_.end()) {
        return;
    }
    arrayBufferDataList_.emplace_back(array);
}

void EcmaVM::RemoveArrayDataList(JSNativePointer *array)
{
    auto iter = std::find(arrayBufferDataList_.begin(), arrayBufferDataList_.end(), array);
    if (iter != arrayBufferDataList_.end()) {
        arrayBufferDataList_.erase(iter);
    }
}

bool EcmaVM::VerifyFilePath(const CString &filePath) const
{
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
}

void EcmaVM::ClearBufferData()
{
    for (auto iter : arrayBufferDataList_) {
        iter->Destroy();
    }
    arrayBufferDataList_.clear();

    for (auto iter = pandaFileWithProgram_.begin(); iter != pandaFileWithProgram_.end();) {
        std::get<0>(*iter)->FreeMethodData(regionFactory_.get());
        auto pf = std::get<1>(*iter);
        // 2 : 2 means the third element.
        if (pf == frameworkPandaFile_ || !isTestMode_ || std::get<2>(*iter)) {
            // In testmode, panda file will free in classlinker
            extractorCache_.erase(pf);
            delete pf;
        }
        iter = pandaFileWithProgram_.erase(iter);
    }
    pandaFileWithProgram_.clear();
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
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&moduleManager_->ecmaModules_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&regexpCache_)));
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
    auto currentFileTuple = pandaFileWithProgram_.back();
    auto currentFileInfo = std::get<1>(currentFileTuple);
    std::string currentPathFile = currentFileInfo->GetFilename();
    CString relativeFile = ConvertToString(EcmaString::Cast(moduleName->GetTaggedObject()));

    // generate full path
    CString abcPath = moduleManager_->GenerateModuleFullPath(currentPathFile, relativeFile);

    // Uniform module name
    JSHandle<EcmaString> abcModuleName = factory_->NewFromString(abcPath);

    JSHandle<JSTaggedValue> module = moduleManager_->GetModule(thread_, JSHandle<JSTaggedValue>::Cast(abcModuleName));
    if (module->IsUndefined()) {
        CString file = ConvertToString(abcModuleName.GetObject<EcmaString>());
        std::vector<std::string> argv;
        ExecuteModule(file, ENTRY_POINTER, argv);
        module = moduleManager_->GetModule(thread_, JSHandle<JSTaggedValue>::Cast(abcModuleName));
    }
    return module;
}

void EcmaVM::ExecuteModule(std::string_view moduleFile, std::string_view entryPoint,
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
