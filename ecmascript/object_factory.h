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

#ifndef ECMASCRIPT_OBJECT_FACTORY_H
#define ECMASCRIPT_OBJECT_FACTORY_H

#include "ecmascript/base/error_type.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
class JSObject;
class JSArray;
class JSSymbol;
class JSFunctionBase;
class JSFunction;
class JSBoundFunction;
class JSProxyRevocFunction;
class JSAsyncAwaitStatusFunction;
class JSPrimitiveRef;
class GlobalEnv;
class GlobalEnvConstants;
class AccessorData;
class JSGlobalObject;
class LexicalEnv;
class JSDate;
class JSProxy;
class JSRealm;
class JSArguments;
class TaggedQueue;
class JSForInIterator;
class JSSet;
class JSMap;
class JSRegExp;
class JSSetIterator;
class JSMapIterator;
class JSArrayIterator;
class JSStringIterator;
class JSGeneratorObject;
class CompletionRecord;
class GeneratorContext;
class JSArrayBuffer;
class JSDataView;
class JSPromise;
class JSPromiseReactionsFunction;
class JSPromiseExecutorFunction;
class JSPromiseAllResolveElementFunction;
class PromiseReaction;
class PromiseCapability;
class PromiseIteratorRecord;
class JSAsyncFuncObject;
class JSAsyncFunction;
class PromiseRecord;
class JSLocale;
class ResolvingFunctionsRecord;
class EcmaVM;
class Heap;
class ConstantPool;
class Program;
class LayoutInfo;
class JSIntlBoundFunction;
class FreeObject;
class JSNativePointer;
class TSObjectType;
class TSClassType;
class TSUnionType;
class TSInterfaceType;
class TSTypeTable;
class TSClassInstanceType;
class TSImportType;
class TSObjLayoutInfo;
class TSModuleTable;
class JSAPITreeSet;
class JSAPITreeMap;
class JSAPITreeSetIterator;
class JSAPITreeMapIterator;
class ModuleNamespace;
class ImportEntry;
class ExportEntry;
class SourceTextModule;
class ResolvedBinding;

namespace job {
class MicroJobQueue;
class PendingJob;
}  // namespace job
class TransitionHandler;
class PrototypeHandler;
class PropertyBox;
class ProtoChangeMarker;
class ProtoChangeDetails;
class ProfileTypeInfo;
class MachineCode;
class ClassInfoExtractor;

enum class CompletionRecordType : uint8_t;
enum class PrimitiveType : uint8_t;
enum class IterationKind : uint8_t;

using ErrorType = base::ErrorType;
using base::ErrorType;
using DeleteEntryPoint = void (*)(void *, void *);

enum class RemoveSlots { YES, NO };
class ObjectFactory {
public:
    explicit ObjectFactory(JSThread *thread, Heap *heap);

    JSHandle<ProfileTypeInfo> NewProfileTypeInfo(uint32_t length);
    JSHandle<ConstantPool> NewConstantPool(uint32_t capacity);
    JSHandle<Program> NewProgram();

    JSHandle<JSObject> GetJSError(const ErrorType &errorType, const char *data = nullptr);

    JSHandle<JSObject> NewJSError(const ErrorType &errorType, const JSHandle<EcmaString> &message);

    JSHandle<TransitionHandler> NewTransitionHandler();

    JSHandle<PrototypeHandler> NewPrototypeHandler();

    JSHandle<JSObject> NewEmptyJSObject();

    // use for others create, prototype is Function.prototype
    // use for native function
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc = nullptr,
                                       FunctionKind kind = FunctionKind::NORMAL_FUNCTION);
    // use for method
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, JSMethod *method,
                                       FunctionKind kind = FunctionKind::NORMAL_FUNCTION);

    JSHandle<JSFunction> NewJSNativeErrorFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc = nullptr);

    JSHandle<JSFunction> NewSpecificTypedArrayFunction(const JSHandle<GlobalEnv> &env,
                                                       const void *nativeFunc = nullptr);

    JSHandle<JSObject> OrdinaryNewJSObjectCreate(const JSHandle<JSTaggedValue> &proto);

    JSHandle<JSBoundFunction> NewJSBoundFunction(const JSHandle<JSFunctionBase> &target,
                                                 const JSHandle<JSTaggedValue> &boundThis,
                                                 const JSHandle<TaggedArray> &args);

    JSHandle<JSIntlBoundFunction> NewJSIntlBoundFunction(const void *nativeFunc = nullptr, int functionLength = 1);

    JSHandle<JSProxyRevocFunction> NewJSProxyRevocFunction(const JSHandle<JSProxy> &proxy,
                                                           const void *nativeFunc = nullptr);

    JSHandle<JSAsyncAwaitStatusFunction> NewJSAsyncAwaitStatusFunction(const void *nativeFunc = nullptr);
    JSHandle<JSFunction> NewJSGeneratorFunction(JSMethod *method);

    JSHandle<JSAsyncFunction> NewAsyncFunction(JSMethod *method);

    JSHandle<JSGeneratorObject> NewJSGeneratorObject(JSHandle<JSTaggedValue> generatorFunction);

    JSHandle<JSAsyncFuncObject> NewJSAsyncFuncObject();

    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(const JSHandle<JSFunction> &function,
                                               const JSHandle<JSTaggedValue> &object);
    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(PrimitiveType type, const JSHandle<JSTaggedValue> &object);

    // get JSHClass for Ecma ClassLinker
    JSHandle<GlobalEnv> NewGlobalEnv(JSHClass *globalEnvClass);

    // get JSHClass for Ecma ClassLinker
    JSHandle<LexicalEnv> NewLexicalEnv(int numSlots);

    inline LexicalEnv *InlineNewLexicalEnv(int numSlots);

    JSHandle<JSSymbol> NewJSSymbol();

    JSHandle<JSSymbol> NewPrivateSymbol();

    JSHandle<JSSymbol> NewPrivateNameSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewWellKnownSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewPublicSymbol(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewSymbolWithTable(const JSHandle<JSTaggedValue> &name);

    JSHandle<JSSymbol> NewPrivateNameSymbolWithChar(const char *description);

    JSHandle<JSSymbol> NewWellKnownSymbolWithChar(const char *description);

    JSHandle<JSSymbol> NewPublicSymbolWithChar(const char *description);

    JSHandle<JSSymbol> NewSymbolWithTableWithChar(const char *description);

    JSHandle<AccessorData> NewAccessorData();
    JSHandle<AccessorData> NewInternalAccessor(void *setter, void *getter);

    JSHandle<PromiseCapability> NewPromiseCapability();

    JSHandle<PromiseReaction> NewPromiseReaction();

    JSHandle<PromiseRecord> NewPromiseRecord();

    JSHandle<ResolvingFunctionsRecord> NewResolvingFunctionsRecord();

    JSHandle<PromiseIteratorRecord> NewPromiseIteratorRecord(const JSHandle<JSTaggedValue> &itor, bool done);

    JSHandle<job::MicroJobQueue> NewMicroJobQueue();

    JSHandle<job::PendingJob> NewPendingJob(const JSHandle<JSFunction> &func, const JSHandle<TaggedArray> &argv);

    JSHandle<JSArray> NewJSArray();

    JSHandle<JSProxy> NewJSProxy(const JSHandle<JSTaggedValue> &target, const JSHandle<JSTaggedValue> &handler);
    JSHandle<JSRealm> NewJSRealm();

    JSHandle<JSArguments> NewJSArguments();

    JSHandle<JSPrimitiveRef> NewJSString(const JSHandle<JSTaggedValue> &str);

    JSHandle<TaggedArray> NewTaggedArray(uint32_t length, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<TaggedArray> NewTaggedArray(uint32_t length, JSTaggedValue initVal, bool nonMovable);
    JSHandle<TaggedArray> NewTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType);
    JSHandle<TaggedArray> NewDictionaryArray(uint32_t length);
    JSHandle<JSForInIterator> NewJSForinIterator(const JSHandle<JSTaggedValue> &obj);

    JSHandle<PropertyBox> NewPropertyBox(const JSHandle<JSTaggedValue> &name);

    JSHandle<ProtoChangeMarker> NewProtoChangeMarker();

    JSHandle<ProtoChangeDetails> NewProtoChangeDetails();

    // use for copy properties keys's array to another array
    JSHandle<TaggedArray> ExtendArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                      JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<TaggedArray> CopyPartArray(const JSHandle<TaggedArray> &old, uint32_t start, uint32_t end);
    JSHandle<TaggedArray> CopyArray(const JSHandle<TaggedArray> &old, uint32_t oldLength, uint32_t newLength,
                                    JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<TaggedArray> CloneProperties(const JSHandle<TaggedArray> &old);
    JSHandle<TaggedArray> CloneProperties(const JSHandle<TaggedArray> &old, const JSHandle<JSTaggedValue> &env,
                                          const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &constpool);

    JSHandle<LayoutInfo> CreateLayoutInfo(int properties, JSTaggedValue initVal = JSTaggedValue::Hole());

    JSHandle<LayoutInfo> ExtendLayoutInfo(const JSHandle<LayoutInfo> &old, int properties,
                                          JSTaggedValue initVal = JSTaggedValue::Hole());

    JSHandle<LayoutInfo> CopyLayoutInfo(const JSHandle<LayoutInfo> &old);

    JSHandle<LayoutInfo> CopyAndReSort(const JSHandle<LayoutInfo> &old, int end, int capacity);

    JSHandle<EcmaString> GetEmptyString() const;

    JSHandle<TaggedArray> EmptyArray() const;

    FreeObject *FillFreeObject(uintptr_t address, size_t size, RemoveSlots removeSlots = RemoveSlots::NO);

    TaggedObject *NewDynObject(const JSHandle<JSHClass> &dynclass);

    TaggedObject *NewNonMovableDynObject(const JSHandle<JSHClass> &dynclass, int inobjPropCount = 0);

    void InitializeExtraProperties(const JSHandle<JSHClass> &dynclass, TaggedObject *obj, int inobjPropCount);

    JSHandle<TaggedQueue> NewTaggedQueue(uint32_t length);

    JSHandle<TaggedQueue> GetEmptyTaggedQueue() const;

    JSHandle<JSSetIterator> NewJSSetIterator(const JSHandle<JSSet> &set, IterationKind kind);

    JSHandle<JSMapIterator> NewJSMapIterator(const JSHandle<JSMap> &map, IterationKind kind);

    JSHandle<JSArrayIterator> NewJSArrayIterator(const JSHandle<JSObject> &array, IterationKind kind);

    JSHandle<CompletionRecord> NewCompletionRecord(CompletionRecordType type, JSHandle<JSTaggedValue> value);

    JSHandle<GeneratorContext> NewGeneratorContext();

    JSHandle<JSPromiseReactionsFunction> CreateJSPromiseReactionsFunction(const void *nativeFunc);

    JSHandle<JSPromiseExecutorFunction> CreateJSPromiseExecutorFunction(const void *nativeFunc);

    JSHandle<JSPromiseAllResolveElementFunction> NewJSPromiseAllResolveElementFunction(const void *nativeFunc);

    JSHandle<JSObject> CloneObjectLiteral(JSHandle<JSObject> object, const JSHandle<JSTaggedValue> &env,
                                          const JSHandle<JSTaggedValue> &constpool, bool canShareHClass = true);
    JSHandle<JSObject> CloneObjectLiteral(JSHandle<JSObject> object);
    JSHandle<JSArray> CloneArrayLiteral(JSHandle<JSArray> object);
    JSHandle<JSFunction> CloneJSFuction(JSHandle<JSFunction> obj, FunctionKind kind);
    JSHandle<JSFunction> CloneClassCtor(JSHandle<JSFunction> ctor, const JSHandle<JSTaggedValue> &lexenv,
                                        bool canShareHClass);

    void NewJSArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length);

    JSHandle<JSArrayBuffer> NewJSArrayBuffer(int32_t length);

    JSHandle<JSArrayBuffer> NewJSArrayBuffer(void *buffer, int32_t length, const DeleteEntryPoint &deleter, void *data,
                                             bool share = false);

    JSHandle<JSDataView> NewJSDataView(JSHandle<JSArrayBuffer> buffer, uint32_t offset, uint32_t length);

    void NewJSRegExpByteCodeData(const JSHandle<JSRegExp> &regexp, void *buffer, size_t size);

    template<typename T, typename S>
    inline void NewJSIntlIcuData(const JSHandle<T> &obj, const S &icu, const DeleteEntryPoint &callback);

    EcmaString *InternString(const JSHandle<JSTaggedValue> &key);

    inline JSHandle<JSNativePointer> NewJSNativePointer(void *externalPointer,
                                                        const DeleteEntryPoint &callBack = nullptr,
                                                        void *data = nullptr,
                                                        bool nonMovable = false);

    JSHandle<JSObject> NewJSObjectByClass(const JSHandle<TaggedArray> &properties, size_t length);

    // only use for creating Function.prototype and Function
    JSHandle<JSFunction> NewJSFunctionByDynClass(JSMethod *method, const JSHandle<JSHClass> &clazz,
                                                 FunctionKind kind = FunctionKind::NORMAL_FUNCTION);
    EcmaString *ResolveString(uint32_t stringId);

    void ObtainRootClass(const JSHandle<GlobalEnv> &globalEnv);

    const MemManager &GetHeapManager() const
    {
        return heapHelper_;
    }

    // used for creating jsobject by constructor
    JSHandle<JSObject> NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                const JSHandle<JSTaggedValue> &newTarget);

    uintptr_t NewSpaceBySnapShotAllocator(size_t size);
    JSHandle<MachineCode> NewMachineCodeObject(size_t length, const uint8_t *data);
    JSHandle<ClassInfoExtractor> NewClassInfoExtractor(JSMethod *ctorMethod);

    // ----------------------------------- new TSType ----------------------------------------
    JSHandle<TSObjLayoutInfo> CreateTSObjLayoutInfo(int propNum, JSTaggedValue initVal = JSTaggedValue::Hole());
    JSHandle<TSObjectType> NewTSObjectType(uint32_t numOfKeys);
    JSHandle<TSClassType> NewTSClassType();
    JSHandle<TSUnionType> NewTSUnionType(uint32_t length);
    JSHandle<TSInterfaceType> NewTSInterfaceType();
    JSHandle<TSImportType> NewTSImportType();
    JSHandle<TSClassInstanceType> NewTSClassInstanceType();
    JSHandle<TSTypeTable> NewTSTypeTable(uint32_t length);
    JSHandle<TSModuleTable> NewTSModuleTable(uint32_t length);

    ~ObjectFactory() = default;

    // ----------------------------------- new string ----------------------------------------
    JSHandle<EcmaString> NewFromString(const CString &data);
    JSHandle<EcmaString> NewFromCanBeCompressString(const CString &data);

    JSHandle<EcmaString> NewFromStdString(const std::string &data);
    JSHandle<EcmaString> NewFromStdStringUnCheck(const std::string &data, bool canBeCompress);

    JSHandle<EcmaString> NewFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> NewFromUtf8UnCheck(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);

    JSHandle<EcmaString> NewFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> NewFromUtf16UnCheck(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);

    JSHandle<EcmaString> NewFromUtf8Literal(const uint8_t *utf8Data, uint32_t utf8Len);
    JSHandle<EcmaString> NewFromUtf8LiteralUnCheck(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);

    JSHandle<EcmaString> NewFromUtf16Literal(const uint16_t *utf16Data, uint32_t utf16Len);
    JSHandle<EcmaString> NewFromUtf16LiteralUnCheck(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);

    JSHandle<EcmaString> NewFromString(EcmaString *str);
    JSHandle<EcmaString> ConcatFromString(const JSHandle<EcmaString> &prefix, const JSHandle<EcmaString> &suffix);

    // used for creating Function
    JSHandle<JSObject> NewJSObject(const JSHandle<JSHClass> &jshclass);

    // used for creating jshclass in Builtins, Function, Class_Linker
    JSHandle<JSHClass> NewEcmaDynClass(uint32_t size, JSType type, const JSHandle<JSTaggedValue> &prototype);

    // It is used to provide iterators for non ECMA standard jsapi containers.
    JSHandle<JSAPITreeMapIterator> NewJSAPITreeMapIterator(const JSHandle<JSAPITreeMap> &map, IterationKind kind);
    JSHandle<JSAPITreeSetIterator> NewJSAPITreeSetIterator(const JSHandle<JSAPITreeSet> &set, IterationKind kind);
    // --------------------------------------module--------------------------------------------
    JSHandle<ModuleNamespace> NewModuleNamespace();
    JSHandle<ImportEntry> NewImportEntry();
    JSHandle<ImportEntry> NewImportEntry(JSHandle<JSTaggedValue> &moduleRequest, JSHandle<JSTaggedValue> &importName,
                                         JSHandle<JSTaggedValue> &localName);
    JSHandle<ExportEntry> NewExportEntry();
    JSHandle<ExportEntry> NewExportEntry(JSHandle<JSTaggedValue> &exportName, JSHandle<JSTaggedValue> &moduleRequest,
                                         JSHandle<JSTaggedValue> &importName, JSHandle<JSTaggedValue> &localName);
    JSHandle<SourceTextModule> NewSourceTextModule();
    JSHandle<ResolvedBinding> NewResolvedBindingRecord();
    JSHandle<ResolvedBinding> NewResolvedBindingRecord(JSHandle<SourceTextModule> module,
                                                       JSHandle<JSTaggedValue> bindingName);

private:
    friend class GlobalEnv;
    friend class GlobalEnvConstants;
    friend class EcmaString;
    JSHandle<JSFunction> NewJSFunctionImpl(JSMethod *method);

    void InitObjectFields(const TaggedObject *object);

    JSThread *thread_ {nullptr};
    bool isTriggerGc_ {false};
    bool triggerSemiGC_ {false};
    MemManager heapHelper_;

    JSHClass *hclassClass_ {nullptr};
    JSHClass *stringClass_ {nullptr};
    JSHClass *arrayClass_ {nullptr};
    JSHClass *dictionaryClass_ {nullptr};
    JSHClass *freeObjectWithNoneFieldClass_ {nullptr};
    JSHClass *freeObjectWithOneFieldClass_ {nullptr};
    JSHClass *freeObjectWithTwoFieldClass_ {nullptr};

    JSHClass *completionRecordClass_ {nullptr};
    JSHClass *generatorContextClass_ {nullptr};
    JSHClass *envClass_ {nullptr};
    JSHClass *symbolClass_ {nullptr};
    JSHClass *accessorDataClass_ {nullptr};
    JSHClass *internalAccessorClass_ {nullptr};
    JSHClass *capabilityRecordClass_ {nullptr};
    JSHClass *reactionsRecordClass_ {nullptr};
    JSHClass *promiseIteratorRecordClass_ {nullptr};
    JSHClass *microJobQueueClass_ {nullptr};
    JSHClass *pendingJobClass_ {nullptr};
    JSHClass *jsProxyOrdinaryClass_ {nullptr};
    JSHClass *jsProxyCallableClass_ {nullptr};
    JSHClass *jsProxyConstructClass_ {nullptr};
    JSHClass *PropertyBoxClass_ {nullptr};
    JSHClass *protoChangeDetailsClass_ {nullptr};
    JSHClass *protoChangeMarkerClass_ {nullptr};
    JSHClass *promiseRecordClass_ {nullptr};
    JSHClass *promiseResolvingFunctionsRecord_ {nullptr};
    JSHClass *jsNativePointerClass_ {nullptr};
    JSHClass *transitionHandlerClass_ {nullptr};
    JSHClass *prototypeHandlerClass_ {nullptr};
    JSHClass *jsRealmClass_ {nullptr};
    JSHClass *programClass_ {nullptr};
    JSHClass *machineCodeClass_ {nullptr};
    JSHClass *classInfoExtractorHClass_ {nullptr};
    JSHClass *tsObjectTypeClass_ {nullptr};
    JSHClass *tsClassTypeClass_ {nullptr};
    JSHClass *tsInterfaceTypeClass_ {nullptr};
    JSHClass *tsUnionTypeClass_ {nullptr};
    JSHClass *tsClassInstanceTypeClass_ {nullptr};
    JSHClass *tsImportTypeClass_ {nullptr};
    JSHClass *importEntryClass_ {nullptr};
    JSHClass *exportEntryClass_ {nullptr};
    JSHClass *sourceTextModuleClass_ {nullptr};
    JSHClass *resolvedBindingClass_ {nullptr};

    EcmaVM *vm_ {nullptr};
    Heap *heap_ {nullptr};

    NO_COPY_SEMANTIC(ObjectFactory);
    NO_MOVE_SEMANTIC(ObjectFactory);

    void NewObjectHook() const;

    // used for creating jshclass in Builtins, Function, Class_Linker
    JSHandle<JSHClass> NewEcmaDynClass(uint32_t size, JSType type,
                                       uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
    // used for creating jshclass in GlobalEnv, EcmaVM
    JSHandle<JSHClass> NewEcmaDynClassClass(JSHClass *hclass, uint32_t size, JSType type);
    JSHandle<JSHClass> NewEcmaDynClass(JSHClass *hclass, uint32_t size, JSType type,
                                       uint32_t inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);

    // used to create nonmovable js_object
    JSHandle<JSObject> NewNonMovableJSObject(const JSHandle<JSHClass> &jshclass);

    // used for creating Function
    JSHandle<JSFunction> NewJSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &dynKlass);
    JSHandle<JSHClass> CreateObjectClass(const JSHandle<TaggedArray> &properties, size_t length);
    JSHandle<JSHClass> CreateFunctionClass(FunctionKind kind, uint32_t size, JSType type,
                                           const JSHandle<JSTaggedValue> &prototype);

    // used for creating ref.prototype in buildins, such as Number.prototype
    JSHandle<JSPrimitiveRef> NewJSPrimitiveRef(const JSHandle<JSHClass> &dynKlass,
                                               const JSHandle<JSTaggedValue> &object);

    JSHandle<EcmaString> GetStringFromStringTable(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const;
    // For MUtf-8 string data
    EcmaString *GetRawStringFromStringTable(const uint8_t *mutf8Data, uint32_t utf16Len, bool canBeCompressed) const;

    JSHandle<EcmaString> GetStringFromStringTable(const uint16_t *utf16Data, uint32_t utf16Len,
                                                  bool canBeCompress) const;

    JSHandle<EcmaString> GetStringFromStringTable(EcmaString *string) const;

    inline EcmaString *AllocStringObject(size_t size);
    inline EcmaString *AllocNonMovableStringObject(size_t size);
    JSHandle<TaggedArray> NewEmptyArray();  // only used for EcmaVM.

    JSHandle<JSHClass> CreateJSArguments();
    JSHandle<JSHClass> CreateJSArrayInstanceClass(JSHandle<JSTaggedValue> proto);
    JSHandle<JSHClass> CreateJSRegExpInstanceClass(JSHandle<JSTaggedValue> proto);

    friend class Builtins;    // create builtins object need dynclass
    friend class JSFunction;  // create prototype_or_dynclass need dynclass
    friend class JSHClass;    // HC transition need dynclass
    friend class EcmaVM;      // hold the factory instance
    friend class JsVerificationTest;
    friend class PandaFileTranslator;
    friend class LiteralDataExtractor;
    friend class RuntimeTrampolines;
    friend class ClassInfoExtractor;
    friend class TSObjectType;
    friend class TSClassType;
    friend class TSUnionType;
    friend class TSClassInstanceType;
    friend class TSImportType;
    friend class ModuleDataExtractor;
};

class ClassLinkerFactory {
private:
    friend class GlobalEnv;  // root class in class_linker need dynclass
    friend class EcmaVM;     // root class in class_linker need dynclass
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_OBJECT_FACTORY_H
