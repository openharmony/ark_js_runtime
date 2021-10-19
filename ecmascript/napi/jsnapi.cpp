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

#include "jsnapi_helper-inl.h"

#include <array>
#include <cstdint>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/ecma_global_storage-inl.h"
#include "ecmascript/ecma_language_context.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_serializer.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "libpandabase/os/library_loader.h"
#include "utils/pandargs.h"

namespace panda {
using ecmascript::CString;
using ecmascript::ECMAObject;
using ecmascript::EcmaString;
using ecmascript::ErrorType;
using ecmascript::FastRuntimeStub;
using ecmascript::GlobalEnv;
using ecmascript::GlobalEnvConstants;
using ecmascript::InternalCallParams;
using ecmascript::JSArray;
using ecmascript::JSArrayBuffer;
using ecmascript::JSDataView;
using ecmascript::JSDate;
using ecmascript::JSFunction;
using ecmascript::JSFunctionBase;
using ecmascript::JSFunctionExtraInfo;
using ecmascript::JSHClass;
using ecmascript::JSMap;
using ecmascript::JSMethod;
using ecmascript::JSNativeObject;
using ecmascript::JSNativePointer;
using ecmascript::JSObject;
using ecmascript::JSPrimitiveRef;
using ecmascript::JSPromise;
using ecmascript::JSRegExp;
using ecmascript::JSSerializer;
using ecmascript::JSSet;
using ecmascript::JSSymbol;
using ecmascript::JSTaggedNumber;
using ecmascript::JSTaggedType;
using ecmascript::JSTaggedValue;
using ecmascript::JSThread;
using ecmascript::ObjectFactory;
using ecmascript::PromiseCapability;
using ecmascript::PropertyDescriptor;
using ecmascript::OperationResult;
using ecmascript::Region;
using ecmascript::TaggedArray;
using ecmascript::JSTypedArray;
using ecmascript::base::BuiltinsBase;
using ecmascript::base::JsonParser;
using ecmascript::base::JsonStringifier;
using ecmascript::base::StringHelper;
using ecmascript::base::TypedArrayHelper;
using ecmascript::JSRuntimeOptions;
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;

namespace {
constexpr uint32_t INTERNAL_POOL_SIZE = 0;
constexpr uint32_t CODE_POOL_SIZE = 0;
constexpr uint32_t COMPILER_POOL_SIZE = 0;
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
constexpr std::string_view ENTRY_POINTER = "_GLOBAL::func_main_0";
}

// ------------------------------------ Panda -----------------------------------------------
bool JSNApi::CreateRuntime(const RuntimeOption &option)
{
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetRuntimeType("ecmascript");

    // GC
    runtimeOptions.SetGcType(option.GetGcType());
    runtimeOptions.SetRunGcInPlace(true);

    // Mem
    runtimeOptions.SetHeapSizeLimit(option.GetGcPoolSize());
    runtimeOptions.SetInternalMemorySizeLimit(INTERNAL_POOL_SIZE);
    runtimeOptions.SetCodeCacheSizeLimit(CODE_POOL_SIZE);
    runtimeOptions.SetCompilerMemorySizeLimit(COMPILER_POOL_SIZE);
    runtimeOptions.SetInternalAllocatorType("malloc");

    // Boot
    runtimeOptions.SetShouldLoadBootPandaFiles(false);
    runtimeOptions.SetShouldInitializeIntrinsics(false);
    runtimeOptions.SetBootClassSpaces({"ecmascript"});

    // Dfx
    runtimeOptions.SetLogLevel(option.GetLogLevel());
    arg_list_t logComponents;
    logComponents.emplace_back("all");
    runtimeOptions.SetLogComponents(logComponents);
    if (option.GetLogBufPrint() != nullptr) {
        runtimeOptions.SetMobileLog(reinterpret_cast<void *>(option.GetLogBufPrint()));
    }

    // Debugger
    runtimeOptions.SetDebuggerLibraryPath(option.GetDebuggerLibraryPath());

    runtimeOptions.SetEnableArkTools(option.GetEnableArkTools());
    SetOptions(runtimeOptions);
    static EcmaLanguageContext lcEcma;
    if (!Runtime::Create(runtimeOptions, {&lcEcma})) {
        std::cerr << "Error: cannot create runtime" << std::endl;
        return false;
    }
    return true;
}

bool JSNApi::DestoryRuntime()
{
    return Runtime::Destroy();
}

EcmaVM *JSNApi::CreateJSVM(const RuntimeOption &option)
{
    auto runtime = Runtime::GetCurrent();
    if (runtime == nullptr) {
        // Art java + Panda js or L2 pure JS app
        if (!CreateRuntime(option)) {
            return nullptr;
        }
        runtime = Runtime::GetCurrent();
        return EcmaVM::Cast(runtime->GetPandaVM());
    }
    JSRuntimeOptions runtimeOptions;

    // GC
    runtimeOptions.SetGcTriggerType("no-gc-for-start-up");  // A non-production gc strategy. Prohibit stw-gc 10 times.

    return EcmaVM::Cast(EcmaVM::Create(runtimeOptions));
}

void JSNApi::DestoryJSVM(EcmaVM *ecmaVm)
{
    auto runtime = Runtime::GetCurrent();
    if (runtime != nullptr) {
        PandaVM *mainVm = runtime->GetPandaVM();
        // Art java + Panda js
        if (mainVm == ecmaVm) {
            DestoryRuntime();
        } else {
            EcmaVM::Destroy(ecmaVm);
        }
    }
}

void JSNApi::TriggerGC(const EcmaVM *vm)
{
    if (vm->GetJSThread() != nullptr && vm->IsInitialized()) {
        vm->CollectGarbage(ecmascript::TriggerGCType::SEMI_GC);
    }
}

void JSNApi::ThrowException(const EcmaVM *vm, Local<JSValueRef> error)
{
    auto thread = vm->GetJSThread();
    thread->SetException(JSNApiHelper::ToJSTaggedValue(*error));
}

bool JSNApi::StartDebugger(const char *library_path, EcmaVM *vm)
{
    auto handle = panda::os::library_loader::Load(std::string(library_path));
    if (!handle) {
        return false;
    }

    using StartDebugger = bool (*)(const std::string &, EcmaVM *);

    auto sym = panda::os::library_loader::ResolveSymbol(handle.Value(), "StartDebug");
    if (!sym) {
        LOG(ERROR, RUNTIME) << sym.Error().ToString();
        return false;
    }

    bool ret = reinterpret_cast<StartDebugger>(sym.Value())("PandaDebugger", vm);
    if (ret) {
        auto runtime = Runtime::GetCurrent();
        runtime->SetDebugMode(true);
        runtime->SetDebuggerLibrary(std::move(handle.Value()));
    }
    return ret;
}

bool JSNApi::Execute(EcmaVM *vm, Local<StringRef> fileName, Local<StringRef> entry)
{
    std::string file = fileName->ToString();
    std::string entryPoint = entry->ToString();
    std::vector<std::string> argv;
    LOG_ECMA(DEBUG) << "start to execute ark file" << file;
    if (!vm->ExecuteFromPf(file, entryPoint, argv)) {
        LOG_ECMA(ERROR) << "Cannot execute ark file" << file;
        std::cerr << "Cannot execute ark file '" << file << "' with entry '" << entryPoint << "'" << std::endl;
        return false;
    }

    return true;
}

bool JSNApi::Execute(EcmaVM *vm, const uint8_t *data, int32_t size, Local<StringRef> entry)
{
    std::string entryPoint = entry->ToString();
    std::vector<std::string> argv;
    if (!vm->ExecuteFromBuffer(data, size, entryPoint, argv)) {
        std::cerr << "Cannot execute panda file from memory "
                  << "' with entry '" << entryPoint << "'" << std::endl;
        return false;
    }

    return true;
}

Local<ObjectRef> JSNApi::GetUncaughtException(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetEcmaUncaughtException());
}

void JSNApi::EnableUserUncaughtErrorHandler(EcmaVM *vm)
{
    return vm->EnableUserUncaughtErrorHandler();
}

Local<ObjectRef> JSNApi::GetGlobalObject(const EcmaVM *vm)
{
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(vm->GetJSThread(), globalEnv->GetGlobalObject());
    return JSNApiHelper::ToLocal<ObjectRef>(global);
}

void JSNApi::ExecutePendingJob(const EcmaVM *vm)
{
    vm->ExecutePromisePendingJob();
}

uintptr_t JSNApi::GetHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return ecmascript::EcmaHandleScope::NewHandle(vm->GetJSThread(), value);
}

uintptr_t JSNApi::GetGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return vm->GetJSThread()->GetEcmaGlobalStorage()->NewGlobalHandle(value);
}

uintptr_t JSNApi::SetWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    return vm->GetJSThread()->GetEcmaGlobalStorage()->SetWeak(localAddress);
}

bool JSNApi::IsWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return false;
    }
    return vm->GetJSThread()->GetEcmaGlobalStorage()->IsWeak(localAddress);
}

void JSNApi::DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0) {
        return;
    }
    vm->GetJSThread()->GetEcmaGlobalStorage()->DisposeGlobalHandle(addr);
}

void *JSNApi::SerializeValue(const EcmaVM *vm, Local<JSValueRef> value, Local<JSValueRef> transfer)
{
    ecmascript::JSThread *thread = vm->GetJSThread();
    ecmascript::Serializer serializer(thread);
    JSHandle<JSTaggedValue> arkValue = JSNApiHelper::ToJSHandle(value);
    JSHandle<JSTaggedValue> arkTransfer = JSNApiHelper::ToJSHandle(transfer);
    std::unique_ptr<ecmascript::SerializationData> data;
    if (serializer.WriteValue(thread, arkValue, arkTransfer)) {
        data = serializer.Release();
    }
    return reinterpret_cast<void *>(data.release());
}

Local<JSValueRef> JSNApi::DeserializeValue(const EcmaVM *vm, void *recoder)
{
    ecmascript::JSThread *thread = vm->GetJSThread();
    std::unique_ptr<ecmascript::SerializationData> data(reinterpret_cast<ecmascript::SerializationData *>(recoder));
    ecmascript::Deserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> result = deserializer.ReadValue();
    return JSNApiHelper::ToLocal<ObjectRef>(result);
}

void JSNApi::DeleteSerializationData(void *data)
{
    ecmascript::SerializationData *value = reinterpret_cast<ecmascript::SerializationData *>(data);
    delete value;
}

bool JSNApi::ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file)
{
    auto moduleManager = vm->GetModuleManager();
    moduleManager->SetCurrentExportModuleName(file);
    // Update Current Module
    vm->GetJSThread()->SetIsEcmaInterpreter(true);
    std::vector<std::string> argv;
    if (!vm->ExecuteFromBuffer(data, size, ENTRY_POINTER, argv)) {
        std::cerr << "Cannot execute panda file from memory" << std::endl;
        moduleManager->RestoreCurrentExportModuleName();
        return false;
    }

    // Restore Current Module
    moduleManager->RestoreCurrentExportModuleName();
    return true;
}

Local<ObjectRef> JSNApi::GetExportObject(EcmaVM *vm, const std::string &file, const std::string &itemName)
{
    auto moduleManager = vm->GetModuleManager();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> moduleName(factory->NewFromStdStringUnCheck(file, true));
    JSHandle<JSTaggedValue> moduleObj = moduleManager->GetModule(vm->GetJSThread(), moduleName);
    JSHandle<JSTaggedValue> itemString(factory->NewFromStdString(itemName));
    JSHandle<JSTaggedValue> exportObj = moduleManager->GetModuleItem(vm->GetJSThread(), moduleObj, itemString);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

void JSNApi::SetOptions(const ecmascript::JSRuntimeOptions &options)
{
    ecmascript::EcmaVM::options_ = options;
}
// ----------------------------------- HandleScope -------------------------------------
LocalScope::LocalScope(const EcmaVM *vm) : thread_(vm->GetJSThread())
{
    auto thread = reinterpret_cast<JSThread *>(thread_);
    prevNext_ = thread->GetHandleScopeStorageNext();
    prevEnd_ = thread->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = thread->GetCurrentHandleStorageIndex();
}

LocalScope::LocalScope(const EcmaVM *vm, JSTaggedType value) : thread_(vm->GetJSThread())
{
    auto thread = reinterpret_cast<JSThread *>(thread_);
    prevNext_ = thread->GetHandleScopeStorageNext();
    prevEnd_ = thread->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = thread->GetCurrentHandleStorageIndex();
    ecmascript::EcmaHandleScope::NewHandle(thread, value);
}

LocalScope::~LocalScope()
{
    auto thread = reinterpret_cast<JSThread *>(thread_);
    thread->SetHandleScopeStorageNext(static_cast<JSTaggedType *>(prevNext_));
    if (thread->GetHandleScopeStorageEnd() != prevEnd_) {
        thread->SetHandleScopeStorageEnd(static_cast<JSTaggedType *>(prevEnd_));
        thread->ShrinkHandleStorage(prevHandleStorageIndex_);
    }
}

// ----------------------------------- EscapeLocalScope ------------------------------
EscapeLocalScope::EscapeLocalScope(const EcmaVM *vm) : LocalScope(vm, 0U)
{
    auto thread = vm->GetJSThread();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    escapeHandle_ = ToUintPtr(thread->GetHandleScopeStorageNext() - 1);
}

// ----------------------------------- NumberRef ---------------------------------------
Local<NumberRef> NumberRef::New(const EcmaVM *vm, double input)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

double NumberRef::Value()
{
    return JSTaggedNumber(JSNApiHelper::ToJSTaggedValue(this)).GetNumber();
}

// ----------------------------------- BooleanRef ---------------------------------------
Local<BooleanRef> BooleanRef::New(const EcmaVM *vm, bool input)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> boolean(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<BooleanRef>(boolean);
}

bool BooleanRef::Value()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

// ----------------------------------- IntegerRef ---------------------------------------
Local<IntegerRef> IntegerRef::New(const EcmaVM *vm, int input)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

Local<IntegerRef> IntegerRef::NewFromUnsigned(const EcmaVM *vm, unsigned int input)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

int IntegerRef::Value()
{
    return JSNApiHelper::ToJSTaggedValue(this).GetInt();
}

// ----------------------------------- StringRef ----------------------------------------
Local<StringRef> StringRef::NewFromUtf8(const EcmaVM *vm, const char *utf8, int length)
{
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromString(utf8));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf8(reinterpret_cast<const uint8_t *>(utf8), length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

std::string StringRef::ToString()
{
    return StringHelper::ToStdString(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject()));
}

int32_t StringRef::Length()
{
    return EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetLength();
}

int32_t StringRef::Utf8Length()
{
    return EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetUtf8Length();
}

int StringRef::WriteUtf8(char *buffer, int length)
{
    return EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())
        ->CopyDataUtf8(reinterpret_cast<uint8_t *>(buffer), length);
}

// ----------------------------------- SymbolRef -----------------------------------------
Local<SymbolRef> SymbolRef::New(const EcmaVM *vm, Local<StringRef> description)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSSymbol> symbol = factory->NewJSSymbol();
    JSTaggedValue desc = JSNApiHelper::ToJSTaggedValue(*description);
    symbol->SetDescription(vm->GetJSThread(), desc);
    return JSNApiHelper::ToLocal<SymbolRef>(JSHandle<JSTaggedValue>(symbol));
}

Local<StringRef> SymbolRef::GetDescription(const EcmaVM *vm)
{
    JSTaggedValue description = JSSymbol::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetDescription();
    if (!description.IsString()) {
        auto constants = vm->GetJSThread()->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> descriptionHandle(vm->GetJSThread(), description);
    return JSNApiHelper::ToLocal<StringRef>(descriptionHandle);
}

// -------------------------------- NativePointerRef ------------------------------------
Local<NativePointerRef> NativePointerRef::New(const EcmaVM *vm, void *nativePointer)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativeObject> obj = factory->NewJSNativeObject(nativePointer);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

Local<NativePointerRef> NativePointerRef::New(
    const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativeObject> obj = factory->NewJSNativeObject(nativePointer, callBack, data);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

void *NativePointerRef::Value()
{
    JSHandle<JSTaggedValue> nativePointer = JSNApiHelper::ToJSHandle(this);
    return JSHandle<JSNativeObject>(nativePointer)->GetExternalPointer();
}

// ----------------------------------- ObjectRef ----------------------------------------
Local<ObjectRef> ObjectRef::New(const EcmaVM *vm)
{
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = globalEnv->GetObjectFunction();
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    RETURN_VALUE_IF_ABRUPT(vm->GetJSThread(), JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ObjectRef>(object);
}

bool ObjectRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> valueValue = JSNApiHelper::ToJSHandle(value);
    bool result = JSTaggedValue::SetProperty(vm->GetJSThread(), obj, keyValue, valueValue);
    RETURN_VALUE_IF_ABRUPT(vm->GetJSThread(), false);
    return result;
}

bool ObjectRef::Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value)
{
    Local<JSValueRef> keyValue = NumberRef::New(vm, key);
    return Set(vm, keyValue, value);
}

bool ObjectRef::SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter,
    Local<FunctionRef> setter, PropertyAttribute attribute)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> getterValue = JSNApiHelper::ToJSHandle(getter);
    JSHandle<JSTaggedValue> setterValue = JSNApiHelper::ToJSHandle(setter);
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    desc.SetSetter(setterValue);
    desc.SetGetter(getterValue);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    bool result = JSTaggedValue::DefineOwnProperty(thread, obj, keyValue, desc);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    if (!ret.GetPropertyMetaData().IsFound()) {
        return JSValueRef::Undefined(vm);
    }
    return JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue());
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, int32_t key)
{
    Local<JSValueRef> keyValue = IntegerRef::New(vm, key);
    return Get(vm, keyValue);
}

bool ObjectRef::GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    PropertyDescriptor desc(thread);
    bool ret = JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), keyValue, desc);
    if (!ret) {
        return false;
    }
    property.SetValue(JSNApiHelper::ToLocal<JSValueRef>(desc.GetValue()));
    if (desc.HasGetter()) {
        property.SetGetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetGetter()));
    }
    if (desc.HasSetter()) {
        property.SetSetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetSetter()));
    }
    if (desc.HasWritable()) {
        property.SetWritable(desc.IsWritable());
    }
    if (desc.HasEnumerable()) {
        property.SetEnumerable(desc.IsEnumerable());
    }
    if (desc.HasConfigurable()) {
        property.SetConfigurable(desc.IsConfigurable());
    }

    return true;
}

Local<ArrayRef> ObjectRef::GetOwnPropertyNames(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj(JSNApiHelper::ToJSHandle(this));
    JSHandle<TaggedArray> array(JSTaggedValue::GetOwnPropertyKeys(thread, obj));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<ArrayRef> ObjectRef::GetOwnEnumerablePropertyNames(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> obj(JSNApiHelper::ToJSHandle(this));
    JSHandle<TaggedArray> array(JSObject::EnumerableOwnNames(thread, obj));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<JSValueRef> ObjectRef::GetPrototype(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> prototype(thread, object->GetPrototype(thread));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

bool ObjectRef::DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    bool result = object->DefinePropertyOrThrow(thread, object, keyValue, desc);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    bool result = object->HasProperty(thread, object, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Has(const EcmaVM *vm, uint32_t key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    bool result = object->HasProperty(thread, object, key);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Delete(const EcmaVM *vm, Local<JSValueRef> key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    bool result = object->DeleteProperty(thread, object, keyValue);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool ObjectRef::Delete(const EcmaVM *vm, uint32_t key)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
    bool result = object->DeleteProperty(thread, object, keyHandle);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

void ObjectRef::SetNativePointerFieldCount(int32_t count)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    object->SetNativePointerFieldCount(count);
}

int32_t ObjectRef::GetNativePointerFieldCount()
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    return object->GetNativePointerFieldCount();
}

void *ObjectRef::GetNativePointerField(int32_t index)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    return object->GetNativePointerField(index);
}

void ObjectRef::SetNativePointerField(int32_t index, void *data)
{
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    object->SetNativePointerField(index, data);
}

// ----------------------------------- FunctionRef --------------------------------------
Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc, void *data)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSHandle<JSNativePointer> funcCallback = factory->NewJSNativePointer(reinterpret_cast<void *>(nativeFunc));
    JSHandle<JSNativePointer> dataCaddress = factory->NewJSNativePointer(data);
    JSHandle<JSFunctionExtraInfo> extraInfo(factory->NewFunctionExtraInfo(funcCallback, dataCaddress));
    current->SetFunctionExtraInfo(thread, extraInfo.GetTaggedValue());
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc, Deleter deleter, void *data)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(Callback::RegisterCallback)));
    JSHandle<JSNativePointer> funcCallback = factory->NewJSNativePointer(reinterpret_cast<void *>(nativeFunc));
    JSHandle<JSNativePointer> dataCaddress = factory->NewJSNativePointer(data, deleter, nullptr);
    vm->PushToArrayDataList(*dataCaddress);
    JSHandle<JSFunctionExtraInfo> extraInfo(factory->NewFunctionExtraInfo(funcCallback, dataCaddress));
    current->SetFunctionExtraInfo(thread, extraInfo.GetTaggedValue());
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, FunctionCallbackWithNewTarget nativeFunc, Deleter deleter,
    void *data)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutName());
    JSMethod *method =
        vm->GetMethodForNativeFunction(reinterpret_cast<void *>(Callback::RegisterCallbackWithNewTarget));
    JSHandle<JSFunction> current =
        factory->NewJSFunctionByDynClass(method, dynclass, ecmascript::FunctionKind::CLASS_CONSTRUCTOR);
    JSHandle<JSNativePointer> funcCallback = factory->NewJSNativePointer(reinterpret_cast<void *>(nativeFunc));
    JSHandle<JSNativePointer> dataCaddress(thread, JSTaggedValue::Undefined());
    if (deleter == nullptr) {
        dataCaddress = factory->NewJSNativePointer(data);
    } else {
        dataCaddress = factory->NewJSNativePointer(data, deleter, nullptr);
        vm->PushToArrayDataList(*dataCaddress);
    }
    JSHandle<JSFunctionExtraInfo> extraInfo(factory->NewFunctionExtraInfo(funcCallback, dataCaddress));
    current->SetFunctionExtraInfo(thread, extraInfo.GetTaggedValue());

    JSHandle<JSObject> clsPrototype =
        JSObject::ObjectCreate(thread, JSHandle<JSObject>(env->GetObjectFunctionPrototype()));
    clsPrototype.GetTaggedValue().GetTaggedObject()->GetClass()->SetClassPrototype(true);
    JSHandle<JSTaggedValue>::Cast(current)->GetTaggedObject()->GetClass()->SetClassConstructor(true);
    current->SetClassConstructor(thread, true);
    JSHandle<JSTaggedValue> parent = env->GetFunctionPrototype();
    JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(current), parent);
    JSFunction::MakeClassConstructor(thread, JSHandle<JSTaggedValue>::Cast(current), clsPrototype);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<JSValueRef> FunctionRef::Call(const EcmaVM *vm, Local<JSValueRef> thisObj,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    JSThread *thread = vm->GetJSThread();
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> thisValue = JSNApiHelper::ToJSHandle(thisObj);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(length);
    Span<const Local<JSValueRef>> sp(argv, length);
    for (int i = 0; i < length; ++i) {
        arguments->Set(thread, i, JSNApiHelper::ToJSHandle(sp[i]));
    }
    InternalCallParams *args = thread->GetInternalCallParams();
    args->MakeArgList(*arguments);
    JSTaggedValue result = JSFunction::Call(thread, func, thisValue, arguments->GetLength(), args->GetArgv());
    RETURN_VALUE_IF_ABRUPT_NOT_CLEAR_EXCEPTION(thread, JSValueRef::Exception(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);

    vm->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT_NOT_CLEAR_EXCEPTION(thread, JSValueRef::Exception(vm));

    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

Local<JSValueRef> FunctionRef::Constructor(const EcmaVM *vm,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    JSThread *thread = vm->GetJSThread();
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> newTarget = func;
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(length);
    Span<const Local<JSValueRef>> sp(argv, length);
    for (int i = 0; i < length; ++i) {
        arguments->Set(thread, i, JSNApiHelper::ToJSHandle(sp[i]));
    }
    ecmascript::InternalCallParams *params = thread->GetInternalCallParams();
    params->MakeArgList(*arguments);
    JSTaggedValue result = JSFunction::Construct(thread, func, length, params->GetArgv(), newTarget);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    JSHandle<JSTaggedValue> resultValue(vm->GetJSThread(), result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

Local<JSValueRef> FunctionRef::GetFunctionPrototype(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> prototype(thread, JSHandle<JSFunction>(func)->GetFunctionPrototype());
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

void FunctionRef::SetName(const EcmaVM *vm, Local<StringRef> name)
{
    JSThread *thread = vm->GetJSThread();
    JSFunction *func = JSFunction::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject());
    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSFunction::SetFunctionNameNoPrefix(thread, func, key);
}

Local<StringRef> FunctionRef::GetName(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSFunctionBase> func = JSHandle<JSFunctionBase>(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<JSTaggedValue> name = JSFunctionBase::GetFunctionName(thread, func);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<StringRef>(name);
}

bool FunctionRef::IsNative(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSFunctionBase> func = JSHandle<JSFunctionBase>(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSMethod *method = func->GetMethod();
    return method->IsNative();
}

// ----------------------------------- ArrayRef ----------------------------------------
Local<ArrayRef> ArrayRef::New(const EcmaVM *vm, int32_t length)
{
    JSThread *thread = vm->GetJSThread();
    JSTaggedNumber arrayLen(length);
    JSHandle<JSTaggedValue> array = JSArray::ArrayCreate(thread, arrayLen);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(array);
}

int32_t ArrayRef::Length([[maybe_unused]] const EcmaVM *vm)
{
    return JSArray::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetArrayLength();
}


Local<JSValueRef> ArrayRef::GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index)
{
    JSThread *thread = vm->GetAssociatedJSThread();
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> result = JSArray::FastGetPropertyByValue(thread, object, index);
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

bool ArrayRef::SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value)
{
    JSThread *thread = vm->GetAssociatedJSThread();
    JSHandle<JSTaggedValue> objectHandle = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> valueHandle = JSNApiHelper::ToJSHandle(value);
    return JSArray::FastSetPropertyByValue(thread, objectHandle, index, valueHandle);
}
// ---------------------------------- Promise --------------------------------------
Local<PromiseCapabilityRef> PromiseCapabilityRef::New(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor(globalEnv->GetPromiseFunction());
    JSHandle<JSTaggedValue> capability(JSPromise::NewPromiseCapability(thread, constructor));
    return JSNApiHelper::ToLocal<PromiseCapabilityRef>(capability);
}

Local<PromiseRef> PromiseCapabilityRef::GetPromise(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, capacity->GetPromise()));
}

bool PromiseCapabilityRef::Resolve(const EcmaVM *vm, Local<JSValueRef> value)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(value);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> resolve(thread, capacity->GetResolve());
    JSHandle<JSTaggedValue> undefined(thread, constants->GetUndefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(arg);
    JSFunction::Call(thread, resolve, undefined, 1, arguments->GetArgv());
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return true;
}

bool PromiseCapabilityRef::Reject(const EcmaVM *vm, Local<JSValueRef> reason)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(reason);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> reject(thread, capacity->GetReject());
    JSHandle<JSTaggedValue> undefined(thread, constants->GetUndefined());
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(arg);
    JSFunction::Call(thread, reject, undefined, 1, arguments->GetArgv());
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return true;
}

Local<PromiseRef> PromiseRef::Catch(const EcmaVM *vm, Local<FunctionRef> handler)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> catchKey(thread, constants->GetPromiseCatchString());
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(handler);
    ecmascript::InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(reject);
    JSTaggedValue result = JSFunction::Invoke(thread, promise, catchKey, 1, arguments->GetArgv());

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> handler)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(handler);
    ecmascript::InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(resolver.GetTaggedValue(), constants->GetUndefined());
    JSTaggedValue result = JSFunction::Invoke(thread, promise, thenKey, 2, arguments->GetArgv());  // 2: two args

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected)
{
    JSThread *thread = vm->GetJSThread();
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(onFulfilled);
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(onRejected);
    ecmascript::InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(resolver, reject);
    JSTaggedValue result = JSFunction::Invoke(thread, promise, thenKey, 2, arguments->GetArgv());  // 2: two args

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}
// ---------------------------------- Promise -------------------------------------

// ---------------------------------- Buffer -----------------------------------
Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, int32_t length)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(length);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

Local<ArrayBufferRef> ArrayBufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<JSArrayBuffer> arrayBuffer =
        factory->NewJSArrayBuffer(buffer, length, reinterpret_cast<ecmascript::DeleteEntryPoint>(deleter), data);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t ArrayBufferRef::ByteLength(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> length(thread, arrayBuffer->GetArrayBufferByteLength());
    if (!length->IsNumber()) {
        return 0;
    }
    return length->GetNumber();
}

void *ArrayBufferRef::GetBuffer()
{
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
}
// ---------------------------------- Buffer -----------------------------------

// ---------------------------------- DataView -----------------------------------
Local<DataViewRef> DataViewRef::New(
    const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, int32_t byteOffset, int32_t byteLength)
{
    JSThread *thread = vm->GetJSThread();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<JSArrayBuffer> buffer(JSNApiHelper::ToJSHandle(arrayBuffer));
    JSHandle<JSDataView> dataView = factory->NewJSDataView(buffer, byteOffset, byteLength);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<DataViewRef>(JSHandle<JSTaggedValue>(dataView));
}

int32_t DataViewRef::ByteLength()
{
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue length = dataView->GetByteLength();
    if (!length.IsNumber()) {
        return 0;
    }
    return length.GetNumber();
}

int32_t DataViewRef::ByteOffset()
{
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue offset = dataView->GetByteOffset();
    if (!offset.IsNumber()) {
        return 0;
    }
    return offset.GetNumber();
}

Local<ArrayBufferRef> DataViewRef::GetArrayBuffer(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> arrayBuffer(thread, dataView->GetViewedArrayBuffer());
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}
// ---------------------------------- DataView -----------------------------------

// ---------------------------------- TypedArray -----------------------------------
int32_t TypedArrayRef::ByteLength(const EcmaVM *vm)
{
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue length = typedArray->GetByteLength();
    if (!length.IsNumber()) {
        return 0;
    }
    return length.GetNumber();
}

int32_t TypedArrayRef::ByteOffset(const EcmaVM *vm)
{
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue length = typedArray->GetByteOffset();
    if (!length.IsNumber()) {
        return 0;
    }
    return length.GetNumber();
}

int32_t TypedArrayRef::ArrayLength(const EcmaVM *vm)
{
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue length = typedArray->GetArrayLength();
    if (!length.IsNumber()) {
        return 0;
    }
    return length.GetNumber();
}

Local<ArrayBufferRef> TypedArrayRef::GetArrayBuffer(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSObject> typeArray(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> arrayBuffer(thread, JSTypedArray::Cast(*typeArray)->GetViewedArrayBuffer());
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_ARRAY_NEW(Type)                                                                           \
    Local<Type##Ref> Type##Ref::New(                                                                    \
        const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length)             \
    {                                                                                                   \
        JSThread *thread = vm->GetJSThread();                                                           \
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();                                                   \
                                                                                                        \
        JSHandle<JSTaggedValue> func = env->Get##Type##Function();                                      \
        JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(buffer));                          \
        ecmascript::InternalCallParams *argv = thread->GetInternalCallParams();                         \
        argv->MakeArgv(arrayBuffer.GetTaggedValue(), JSTaggedValue(byteOffset), JSTaggedValue(length)); \
        array_size_t argc = 3;                                                                          \
        JSTaggedValue result = JSFunction::Construct(thread, func, argc, argv->GetArgv(), func);        \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));                                      \
        JSHandle<JSTaggedValue> resultHandle(thread, result);                                           \
        return JSNApiHelper::ToLocal<Type##Ref>(resultHandle);                                          \
    }

TYPED_ARRAY_ALL(TYPED_ARRAY_NEW)

#undef TYPED_ARRAY_NEW
// ---------------------------------- TypedArray -----------------------------------

// ---------------------------------- Error ---------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXCEPTION_ERROR_NEW(name, type)                                                     \
    Local<JSValueRef> Exception::name(const EcmaVM *vm, Local<StringRef> message)           \
    {                                                                                       \
        JSThread *thread = vm->GetJSThread();                                               \
        ObjectFactory *factory = vm->GetFactory();                                          \
                                                                                            \
        JSHandle<EcmaString> messageValue(JSNApiHelper::ToJSHandle(message));               \
        JSHandle<JSTaggedValue> result(factory->NewJSError(ErrorType::type, messageValue)); \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));                          \
        return JSNApiHelper::ToLocal<JSValueRef>(result);                                   \
    }

EXCEPTION_ERROR_ALL(EXCEPTION_ERROR_NEW)

#undef EXCEPTION_ERROR_NEW
// ---------------------------------- Error ---------------------------------------

// ---------------------------------- JSON ------------------------------------------
Local<JSValueRef> JSON::Parse(const EcmaVM *vm, Local<StringRef> string)
{
    JSThread *thread = vm->GetJSThread();
    auto ecmaStr = EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject());
    JSHandle<JSTaggedValue> result;
    if (ecmaStr->IsUtf8()) {
        JsonParser<uint8_t> parser(thread);
        result = parser.ParseUtf8(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject()));
    } else {
        JsonParser<uint16_t> parser(thread);
        result = parser.ParseUtf16(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject()));
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

Local<JSValueRef> JSON::Stringify(const EcmaVM *vm, Local<JSValueRef> json)
{
    JSThread *thread = vm->GetJSThread();
    auto constants = thread->GlobalConstants();
    JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> str = stringifier.Stringify(
        JSNApiHelper::ToJSHandle(json), constants->GetHandledUndefined(), constants->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(str);
}

Local<StringRef> RegExpRef::GetOriginalSource(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue source = regExp->GetOriginalSource();
    if (!source.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> sourceHandle(thread, source);
    return JSNApiHelper::ToLocal<StringRef>(sourceHandle);
}

Local<StringRef> DateRef::ToString(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue dateStr = date->ToString(thread);
    if (!dateStr.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> dateStrHandle(thread, dateStr);
    return JSNApiHelper::ToLocal<StringRef>(dateStrHandle);
}

int32_t MapRef::GetSize()
{
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return map->GetSize();
}

int32_t SetRef::GetSize()
{
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    return set->GetSize();
}

// ----------------------------------- FunctionCallback ---------------------------------
JSTaggedValue Callback::RegisterCallback(ecmascript::EcmaRuntimeCallInfo *info)
{
    // Constructor
    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(info);
    if (!constructor->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> function(constructor);
    JSHandle<JSTaggedValue> extraInfoValue(thread, function->GetFunctionExtraInfo());
    if (!extraInfoValue->IsJSFunctionExtraInfo()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunctionExtraInfo> extraInfo(extraInfoValue);
    // vm
    Region *region = Region::ObjectAddressToRange(extraInfo.GetTaggedValue().GetTaggedObject());
    if (region == nullptr) {
        return JSTaggedValue::False();
    }
    EcmaVM *vm = region->GetSpace()->GetHeap()->GetEcmaVM();
    // data
    JSHandle<JSTaggedValue> data(thread, extraInfo->GetData());
    if (!data->IsHeapObject()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> dataObj(data);
    // callBack
    JSHandle<JSTaggedValue> callBack(thread, extraInfo->GetCallback());
    if (!callBack->IsHeapObject()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> callBackObj(callBack);
    FunctionCallback nativeFunc = (reinterpret_cast<FunctionCallback>(callBackObj->GetExternalPointer()));

    // this
    JSHandle<JSTaggedValue> thisValue(BuiltinsBase::GetThis(info));

    // arguments
    std::vector<Local<JSValueRef>> arguments;
    array_size_t length = info->GetArgsNumber();
    for (array_size_t i = 0; i < length; ++i) {
        arguments.emplace_back(JSNApiHelper::ToLocal<JSValueRef>(BuiltinsBase::GetCallArg(info, i)));
    }

    Local<JSValueRef> result = nativeFunc(vm,
        JSNApiHelper::ToLocal<JSValueRef>(thisValue),
        arguments.data(),
        arguments.size(),
        dataObj->GetExternalPointer());
    return JSNApiHelper::ToJSHandle(result).GetTaggedValue();
}

JSTaggedValue Callback::RegisterCallbackWithNewTarget(ecmascript::EcmaRuntimeCallInfo *info)
{
    // Constructor
    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(info);
    if (!constructor->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunction> function(constructor);
    JSHandle<JSTaggedValue> extraInfoValue(thread, function->GetFunctionExtraInfo());
    if (!extraInfoValue->IsJSFunctionExtraInfo()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSFunctionExtraInfo> extraInfo(extraInfoValue);
    // vm
    Region *region = Region::ObjectAddressToRange(extraInfo.GetTaggedValue().GetTaggedObject());
    if (region == nullptr) {
        return JSTaggedValue::False();
    }
    EcmaVM *vm = region->GetSpace()->GetHeap()->GetEcmaVM();
    // data
    JSHandle<JSTaggedValue> data(thread, extraInfo->GetData());
    if (!data->IsHeapObject()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> dataObj(data);
    // callBack
    JSHandle<JSTaggedValue> callBack(thread, extraInfo->GetCallback());
    if (!callBack->IsHeapObject()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> callBackObj(callBack);
    FunctionCallbackWithNewTarget nativeFunc =
        (reinterpret_cast<FunctionCallbackWithNewTarget>(callBackObj->GetExternalPointer()));

    // newTarget
    JSHandle<JSTaggedValue> newTarget(BuiltinsBase::GetNewTarget(info));

    // this
    JSHandle<JSTaggedValue> thisValue(BuiltinsBase::GetThis(info));

    // arguments
    std::vector<Local<JSValueRef>> arguments;
    array_size_t length = info->GetArgsNumber();
    for (array_size_t i = 0; i < length; ++i) {
        arguments.emplace_back(JSNApiHelper::ToLocal<JSValueRef>(BuiltinsBase::GetCallArg(info, i)));
    }

    Local<JSValueRef> result = nativeFunc(vm,
        JSNApiHelper::ToLocal<JSValueRef>(thisValue),
        JSNApiHelper::ToLocal<JSValueRef>(newTarget),
        arguments.data(),
        arguments.size(),
        dataObj->GetExternalPointer());
    return JSNApiHelper::ToJSHandle(result).GetTaggedValue();
}

// -------------------------------------  JSExecutionScope ------------------------------
JSExecutionScope::JSExecutionScope(const EcmaVM *vm)
{
    (void)vm;
}

JSExecutionScope::~JSExecutionScope()
{
    last_current_thread_ = nullptr;
    is_revert_ = false;
}

// ----------------------------------- JSValueRef --------------------------------------
Local<PrimitiveRef> JSValueRef::Undefined(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Undefined()));
}

Local<PrimitiveRef> JSValueRef::Null(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Null()));
}

Local<PrimitiveRef> JSValueRef::True(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::True()));
}

Local<PrimitiveRef> JSValueRef::False(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::False()));
}

Local<JSValueRef> JSValueRef::Exception(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Exception()));
}

Local<ObjectRef> JSValueRef::ToObject(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    if (IsUndefined() || IsNull()) {
        return Exception(vm);
    }
    JSHandle<JSTaggedValue> obj(JSTaggedValue::ToObject(thread, JSNApiHelper::ToJSHandle(this)));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<ObjectRef>(obj);
}

Local<StringRef> JSValueRef::ToString(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    if (!obj->IsString()) {
        obj = JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, obj));
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    }
    return JSNApiHelper::ToLocal<StringRef>(obj);
}

Local<NativePointerRef> JSValueRef::ToNativePointer(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<NativePointerRef>(obj);
}

bool JSValueRef::BooleaValue()
{
    return JSNApiHelper::ToJSTaggedValue(this).ToBoolean();
}

int64_t JSValueRef::IntegerValue(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSTaggedNumber number = JSTaggedValue::ToInteger(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number.GetNumber();
}

uint32_t JSValueRef::Uint32Value(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    uint32_t number = JSTaggedValue::ToUint32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

int32_t JSValueRef::Int32Value(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    int32_t number = JSTaggedValue::ToInt32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

Local<BooleanRef> JSValueRef::ToBoolean(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> booleanObj = JSHandle<JSTaggedValue>(thread, JSTaggedValue(obj->ToBoolean()));
    return JSNApiHelper::ToLocal<BooleanRef>(booleanObj);
}

Local<NumberRef> JSValueRef::ToNumber(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue::ToNumber(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Exception(vm));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

bool JSValueRef::IsStrictEquals(const EcmaVM *vm, Local<JSValueRef> value)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> xValue = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> yValue = JSNApiHelper::ToJSHandle(value);
    return JSTaggedValue::StrictEqual(thread, xValue, yValue);
}

Local<StringRef> JSValueRef::Typeof(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    JSTaggedValue value = FastRuntimeStub::FastTypeOf(thread, JSNApiHelper::ToJSTaggedValue(this));
    return JSNApiHelper::ToLocal<StringRef>(JSHandle<JSTaggedValue>(thread, value));
}

bool JSValueRef::InstanceOf(const EcmaVM *vm, Local<JSValueRef> value)
{
    JSThread *thread = vm->GetJSThread();
    JSHandle<JSTaggedValue> origin = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSTaggedValue> target = JSNApiHelper::ToJSHandle(value);
    bool result = JSObject::InstanceOf(thread, origin, target);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    return result;
}

bool JSValueRef::IsUndefined()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsUndefined();
}

bool JSValueRef::IsNull()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNull();
}

bool JSValueRef::IsHole()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsHole();
}

bool JSValueRef::IsTrue()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

bool JSValueRef::IsFalse()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsFalse();
}

bool JSValueRef::IsNumber()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNumber();
}

bool JSValueRef::IsInt()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsInt();
}

bool JSValueRef::WithinInt32()
{
    return JSNApiHelper::ToJSTaggedValue(this).WithinInt32();
}

bool JSValueRef::IsBoolean()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsBoolean();
}

bool JSValueRef::IsString()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsString();
}

bool JSValueRef::IsSymbol()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsSymbol();
}

bool JSValueRef::IsObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsECMAObject();
}

bool JSValueRef::IsArray(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    return JSNApiHelper::ToJSTaggedValue(this).IsArray(thread);
}

bool JSValueRef::IsConstructor()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsConstructor();
}

bool JSValueRef::IsFunction()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsCallable();
}

bool JSValueRef::IsProxy()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSProxy();
}

bool JSValueRef::IsException()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsException();
}

bool JSValueRef::IsPromise()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPromise();
}

bool JSValueRef::IsDataView()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDataView();
}

bool JSValueRef::IsTypedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTypedArray();
}

bool JSValueRef::IsNativePointer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNativePointer();
}

bool JSValueRef::IsNativeObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNativeObject();
}

bool JSValueRef::IsDate()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDate();
}

bool JSValueRef::IsError()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSError();
}

bool JSValueRef::IsMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMap();
}

bool JSValueRef::IsSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSet();
}

bool JSValueRef::IsWeakMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakMap();
}

bool JSValueRef::IsWeakSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakSet();
}

bool JSValueRef::IsRegExp()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRegExp();
}

bool JSValueRef::IsArrayIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArrayIterator();
}

bool JSValueRef::IsStringIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsStringIterator();
}

bool JSValueRef::IsSetIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSetIterator();
}

bool JSValueRef::IsMapIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMapIterator();
}

bool JSValueRef::IsArrayBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsUint8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8Array();
}

bool JSValueRef::IsInt8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt8Array();
}

bool JSValueRef::IsUint8ClampedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8ClampedArray();
}

bool JSValueRef::IsInt16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt16Array();
}

bool JSValueRef::IsUint16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint16Array();
}

bool JSValueRef::IsInt32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt32Array();
}

bool JSValueRef::IsUint32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint32Array();
}

bool JSValueRef::IsFloat32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat32Array();
}

bool JSValueRef::IsFloat64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat64Array();
}

bool JSValueRef::IsJSPrimitiveRef()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPrimitiveRef();
}

bool JSValueRef::IsJSPrimitiveNumber()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    return JSPrimitiveRef::Cast(obj->GetHeapObject())->IsNumber();
}

bool JSValueRef::IsJSPrimitiveInt()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    return JSPrimitiveRef::Cast(obj->GetHeapObject())->IsInt();
}

bool JSValueRef::IsJSPrimitiveBoolean()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    return JSPrimitiveRef::Cast(obj->GetHeapObject())->IsBoolean();
}

bool JSValueRef::IsJSPrimitiveString()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    return JSPrimitiveRef::Cast(obj->GetHeapObject())->IsString();
}
}  // namespace panda
