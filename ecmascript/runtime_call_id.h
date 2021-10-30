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

#ifndef ECMASCRIPT_RUNTIME_CALL_ID_H
#define ECMASCRIPT_RUNTIME_CALL_ID_H

#include "ecmascript/base/config.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_LIST(V)  \
    V(RunInternal)                  \
    V(Ldnan)                        \
    V(Ldinfinity)                   \
    V(Ldglobalthis)                 \
    V(Ldundefined)                  \
    V(Ldboolean)                    \
    V(Ldnumber)                     \
    V(Ldstring)                     \
    V(Ldbigint)                     \
    V(Ldnull)                       \
    V(Ldsymbol)                     \
    V(Ldfunction)                   \
    V(Ldglobal)                     \
    V(Ldtrue)                       \
    V(Ldfalse)                      \
    V(Tonumber)                     \
    V(Toboolean)                    \
    V(Add2Dyn)                      \
    V(Sub2Dyn)                      \
    V(Mul2Dyn)                      \
    V(Div2Dyn)                      \
    V(Mod2Dyn)                      \
    V(EqDyn)                        \
    V(NotEqDyn)                     \
    V(LessDyn)                      \
    V(LessEqDyn)                    \
    V(GreaterDyn)                   \
    V(GreaterEqDyn)                 \
    V(StrictNotEqDyn)               \
    V(StrictEqDyn)                  \
    V(Shl2Dyn)                      \
    V(Shr2Dyn)                      \
    V(Ashr2Dyn)                     \
    V(And2Dyn)                      \
    V(Or2Dyn)                       \
    V(Xor2Dyn)                      \
    V(NegDyn)                       \
    V(NotDyn)                       \
    V(IncDyn)                       \
    V(DecDyn)                       \
    V(ExpDyn)                       \
    V(ThrowDyn)                     \
    V(LdObjByIndexDyn)              \
    V(StObjByIndexDyn)              \
    V(LdObjByNameDyn)               \
    V(StObjByNameDyn)               \
    V(LdObjByValueDyn)              \
    V(StObjByValueDyn)              \
    V(StOwnByNameDyn)               \
    V(StOwnByIdDyn)                 \
    V(StOwnByValueDyn)              \
    V(Trygetobjprop)                \
    V(Delobjprop)                   \
    V(Defineglobalvar)              \
    V(Definelocalvar)               \
    V(Definefuncexpr)               \
    V(DefinefuncDyn)                \
    V(DefineNCFuncDyn)              \
    V(NewobjDynrange)               \
    V(RefeqDyn)                     \
    V(TypeofDyn)                    \
    V(LdnewobjrangeDyn)             \
    V(IsInDyn)                      \
    V(InstanceofDyn)                \
    V(NewobjspreadDyn)              \
    V(CallArg0Dyn)                  \
    V(CallArg1Dyn)                  \
    V(CallArg2Dyn)                  \
    V(CallArg3Dyn)                  \
    V(CallThisRangeDyn)             \
    V(CallRangeDyn)                 \
    V(CallSpreadDyn)                \
    V(NewlexenvDyn)                 \
    V(StlexvarDyn)                  \
    V(LdlexvarDyn)                  \
    V(LdlexenvDyn)                  \
    V(GetPropIterator)              \
    V(CreateIterResultObj)          \
    V(DefineGeneratorFunc)          \
    V(SuspendGenerator)             \
    V(ResumeGenerator)              \
    V(GetResumeMode)                \
    V(CreateGeneratorObj)           \
    V(DefineAsyncFunc)              \
    V(DefineGetterSetterByValue)    \
    V(AsyncFunctionEnter)           \
    V(AsyncFunctionAwaitUncaught)   \
    V(AsyncFunctionResolveOrReject) \
    V(ThrowUndefined)               \
    V(ThrowConstAssignment)         \
    V(ThrowUndefinedIfHole)         \
    V(Copyrestargs)                 \
    V(Trystobjprop)                 \
    V(GetTemplateObject)            \
    V(GetIterator)                  \
    V(ThrowIfNotObject)             \
    V(ThrowThrowNotExists)          \
    V(CreateObjectWithExcludedKeys) \
    V(ThrowPatternNonCoercible)     \
    V(IterNext)                     \
    V(CloseIterator)                \
    V(StArraySpread)                \
    V(GetCallSpreadArgs)            \
    V(TryLoadICByName)              \
    V(LoadICByName)                 \
    V(GetPropertyByName)            \
    V(TryLoadICByValue)             \
    V(LoadICByValue)                \
    V(TryStoreICByName)             \
    V(StoreICByName)                \
    V(TryStoreICByValue)            \
    V(StoreICByValue)               \
    V(NotifyInlineCache)            \
    V(CompressCollector_RunPhases)  \
    V(OldSpaceCollector_RunPhases)  \
    V(SemiSpaceCollector_RunPhases) \
    V(LoadGlobalICByName)           \
    V(StoreGlobalICByName)          \
    V(StoreICWithHandler)           \
    V(StorePrototype)               \
    V(StoreWithTransition)          \
    V(StoreField)                   \
    V(StoreGlobal)                  \
    V(LoadPrototype)                \
    V(LoadICWithHandler)            \
    V(StoreElement)                 \
    V(CallGetter)                   \
    V(CallSetter)                   \
    V(AddPropertyByName)            \
    V(AddPropertyByIndex)           \
    V(GetPropertyByIndex)           \
    V(GetPropertyByValue)           \
    V(SetPropertyByIndex)           \
    V(SetPropertyByValue)           \
    V(FastTypeOf)                   \
    V(FastSetPropertyByIndex)       \
    V(FastSetPropertyByValue)       \
    V(FastGetPropertyByName)        \
    V(FastGetPropertyByValue)       \
    V(FastGetPropertyByIndex)       \
    V(NewLexicalEnvDyn)             \
    V(SetElement)                   \
    V(SetGlobalOwnProperty)         \
    V(SetOwnPropertyByName)         \
    V(SetOwnElement)                \
    V(FastSetProperty)              \
    V(FastGetProperty)              \
    V(FindOwnProperty)              \
    V(HasOwnProperty)               \
    V(ExecuteNative)                \
    V(Execute)                      \
    V(ToJSTaggedValueWithInt32)     \
    V(ToJSTaggedValueWithUint32)    \
    V(ThrowIfSuperNotCorrectCall)   \
    V(CreateEmptyArray)             \
    V(CreateEmptyObject)            \
    V(CreateObjectWithBuffer)       \
    V(CreateObjectHavingMethod)     \
    V(SetObjectWithProto)           \
    V(ImportModule)                 \
    V(StModuleVar)                  \
    V(CopyModule)                   \
    V(LdModvarByName)               \
    V(CreateRegExpWithLiteral)      \
    V(CreateArrayWithBuffer)        \
    V(GetNextPropName)              \
    V(CopyDataProperties)           \
    V(GetUnmapedArgs)               \
    V(TryStGlobalByName)            \
    V(LdGlobalVar)                  \
    V(StGlobalVar)                  \
    V(TryUpdateGlobalRecord)        \
    V(LdGlobalRecord)               \
    V(StGlobalRecord)               \
    V(ThrowReferenceError)          \
    V(ThrowTypeError)               \
    V(ThrowSyntaxError)             \
    V(NewClassFunc)                 \
    V(DefineClass)                  \
    V(SuperCall)                    \
    V(SuperCallSpread)              \
    V(DefineMethod)                 \
    V(LdSuperByValue)               \
    V(StSuperByValue)               \
    V(ThrowDeleteSuperProperty)     \
    V(GetIteratorNext)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUITINS_API_LIST(V)                   \
    V(Array, Constructor)                     \
    V(Array, From)                            \
    V(Array, Of)                              \
    V(Array, IsArray)                         \
    V(Array, Entries)                         \
    V(Array, Species)                         \
    V(Array, Concat)                          \
    V(Array, CopyWithin)                      \
    V(Array, Fill)                            \
    V(Array, Filter)                          \
    V(Array, Find)                            \
    V(Array, FindIndex)                       \
    V(Array, IndexOf)                         \
    V(Array, Join)                            \
    V(Array, Keys)                            \
    V(Array, LastIndexOf)                     \
    V(Array, Map)                             \
    V(Array, Pop)                             \
    V(Array, Push)                            \
    V(Array, Reduce)                          \
    V(Array, ReduceRight)                     \
    V(Array, Reverse)                         \
    V(Array, Shift)                           \
    V(Array, Slice)                           \
    V(Array, Some)                            \
    V(Array, Sort)                            \
    V(Array, Splice)                          \
    V(Array, ToLocaleString)                  \
    V(Array, ToString)                        \
    V(Array, Unshift)                         \
    V(Array, Values)                          \
    V(ArrayBuffer, Constructor)               \
    V(ArrayBuffer, Slice)                     \
    V(ArrayBuffer, GetValueFromBuffer)        \
    V(ArrayBuffer, SetValueInBuffer)          \
    V(ArrayBuffer, CloneArrayBuffer)          \
    V(ArrayBuffer, AllocateArrayBuffer)       \
    V(AsyncFunction, Constructor)             \
    V(Boolean, Constructor)                   \
    V(Boolean, ThisBooleanValue)              \
    V(DataView, Constructor)                  \
    V(DataView, GetBuffer)                    \
    V(DataView, GetByteLength)                \
    V(DataView, GetOffset)                    \
    V(DataView, GetViewValue)                 \
    V(DataView, SetViewValue)                 \
    V(Date, Constructor)                      \
    V(Date, Now)                              \
    V(Date, UTC)                              \
    V(Date, Parse)                            \
    V(Date, GetDateField)                     \
    V(Date, GetTime)                          \
    V(Date, SetTime)                          \
    V(Date, ToJSON)                           \
    V(Date, ValueOf)                          \
    V(Date, ToPrimitive)                      \
    V(Function, Constructor)                  \
    V(Function, PrototypeApply)               \
    V(Function, PrototypeBind)                \
    V(Function, PrototypeCall)                \
    V(Function, PrototypeToString)            \
    V(Function, PrototypeHasInstance)         \
    V(Generator, Constructor)                 \
    V(Generator, PrototypeNext)               \
    V(Generator, PrototypeReturn)             \
    V(Generator, PrototypeThrow)              \
    V(Global, IsFinite)                       \
    V(Global, IsNaN)                          \
    V(Global, PrintEntryPoint)                \
    V(Global, NewobjDynrange)                 \
    V(Global, CallJsBoundFunction)            \
    V(Global, CallJsProxy)                    \
    V(Global, DecodeURI)                      \
    V(Global, EncodeURI)                      \
    V(Global, DecodeURIComponent)             \
    V(Global, EncodeURIComponent)             \
    V(Iterator, Constructor)                  \
    V(Iterator, Next)                         \
    V(Iterator, Throw)                        \
    V(Iterator, Return)                       \
    V(Iterator, GetObj)                       \
    V(Json, Parse)                            \
    V(Json, Stringify)                        \
    V(Map, Constructor)                       \
    V(Map, Species)                           \
    V(Map, Clear)                             \
    V(Map, Delete)                            \
    V(Map, Entries)                           \
    V(Map, Get)                               \
    V(Map, Has)                               \
    V(Map, Keys)                              \
    V(Map, Set)                               \
    V(Map, GetSize)                           \
    V(Map, Values)                            \
    V(Math, Abs)                              \
    V(Math, Acos)                             \
    V(Math, Acosh)                            \
    V(Math, Asin)                             \
    V(Math, Asinh)                            \
    V(Math, Atan)                             \
    V(Math, Atanh)                            \
    V(Math, Atan2)                            \
    V(Math, Cbrt)                             \
    V(Math, Ceil)                             \
    V(Math, Clz32)                            \
    V(Math, Cos)                              \
    V(Math, Cosh)                             \
    V(Math, Exp)                              \
    V(Math, Expm1)                            \
    V(Math, Floor)                            \
    V(Math, Fround)                           \
    V(Math, Hypot)                            \
    V(Math, Imul)                             \
    V(Math, Log)                              \
    V(Math, Log1p)                            \
    V(Math, Log10)                            \
    V(Math, Log2)                             \
    V(Math, Max)                              \
    V(Math, Min)                              \
    V(Math, Pow)                              \
    V(Math, Random)                           \
    V(Math, Round)                            \
    V(Math, Sign)                             \
    V(Math, Sin)                              \
    V(Math, Sinh)                             \
    V(Math, Sqrt)                             \
    V(Math, Tan)                              \
    V(Math, Tanh)                             \
    V(Math, Trunc)                            \
    V(Number, Constructor)                    \
    V(Number, IsFinite)                       \
    V(Number, IsInteger)                      \
    V(Number, IsNaN)                          \
    V(Number, IsSafeInteger)                  \
    V(Number, ParseFloat)                     \
    V(Number, ParseInt)                       \
    V(Number, ToExponential)                  \
    V(Number, ToFixed)                        \
    V(Number, ToLocaleString)                 \
    V(Number, ToPrecision)                    \
    V(Number, ToString)                       \
    V(Number, ValueOf)                        \
    V(Number, ThisNumberValue)                \
    V(Object, Constructor)                    \
    V(Object, Assign)                         \
    V(Object, Create)                         \
    V(Object, DefineProperties)               \
    V(Object, DefineProperty)                 \
    V(Object, Freeze)                         \
    V(Object, GetOwnPropertyDesciptor)        \
    V(Object, GetOwnPropertyKeys)             \
    V(Object, GetOwnPropertyNames)            \
    V(Object, GetOwnPropertySymbols)          \
    V(Object, GetPrototypeOf)                 \
    V(Object, Is)                             \
    V(Object, Keys)                           \
    V(Object, PreventExtensions)              \
    V(Object, Seal)                           \
    V(Object, SetPrototypeOf)                 \
    V(Object, HasOwnProperty)                 \
    V(Object, IsPrototypeOf)                  \
    V(Object, ToLocaleString)                 \
    V(Object, GetBuiltinTag)                  \
    V(Object, ToString)                       \
    V(Object, ValueOf)                        \
    V(Object, ProtoGetter)                    \
    V(Object, ProtoSetter)                    \
    V(PromiseHandler, Resolve)                \
    V(PromiseHandler, Reject)                 \
    V(PromiseHandler, Executor)               \
    V(PromiseHandler, ResolveElementFunction) \
    V(PromiseJob, Reaction)                   \
    V(PromiseJob, ResolveThenableJob)         \
    V(Promise, Constructor)                   \
    V(Promise, All)                           \
    V(Promise, Race)                          \
    V(Promise, Reject)                        \
    V(Promise, Resolve)                       \
    V(Promise, GetSpecies)                    \
    V(Promise, Catch)                         \
    V(Promise, Then)                          \
    V(Promise, PerformPromiseThen)            \
    V(Proxy, Constructor)                     \
    V(Proxy, Revocable)                       \
    V(Proxy, InvalidateProxyFunction)         \
    V(Reflect, Apply)                         \
    V(Reflect, Constructor)                   \
    V(Reflect, DefineProperty)                \
    V(Reflect, DeleteProperty)                \
    V(Reflect, Get)                           \
    V(Reflect, GetOwnPropertyDescriptor)      \
    V(Reflect, GetPrototypeOf)                \
    V(Reflect, Has)                           \
    V(Reflect, OwnKeys)                       \
    V(Reflect, PreventExtensions)             \
    V(Reflect, Set)                           \
    V(Reflect, SetPrototypeOf)                \
    V(RegExp, Constructor)                    \
    V(RegExp, Exec)                           \
    V(RegExp, Test)                           \
    V(RegExp, ToString)                       \
    V(RegExp, GetFlags)                       \
    V(RegExp, GetSpecies)                     \
    V(RegExp, Match)                          \
    V(RegExp, Replace)                        \
    V(RegExp, Search)                         \
    V(RegExp, Split)                          \
    V(RegExp, Create)                         \
    V(Set, Constructor)                       \
    V(Set, Species)                           \
    V(Set, Add)                               \
    V(Set, Clear)                             \
    V(Set, Delete)                            \
    V(Set, Entries)                           \
    V(Set, Has)                               \
    V(Set, GetSize)                           \
    V(Set, Values)                            \
    V(StringIterator, Next)                   \
    V(String, Constructor)                    \
    V(String, FromCharCode)                   \
    V(String, FromCodePoint)                  \
    V(String, Raw)                            \
    V(String, GetSubstitution)                \
    V(String, CharAt)                         \
    V(String, CharCodeAt)                     \
    V(String, CodePointAt)                    \
    V(String, Concat)                         \
    V(String, EndsWith)                       \
    V(String, Includes)                       \
    V(String, IndexOf)                        \
    V(String, LastIndexOf)                    \
    V(String, LocaleCompare)                  \
    V(String, Match)                          \
    V(String, Normalize)                      \
    V(String, Repeat)                         \
    V(String, Replace)                        \
    V(String, Search)                         \
    V(String, Slice)                          \
    V(String, Split)                          \
    V(String, StartsWith)                     \
    V(String, Substring)                      \
    V(String, ToLocaleLowerCase)              \
    V(String, ToLocaleUpperCase)              \
    V(String, ToLowerCase)                    \
    V(String, ToString)                       \
    V(String, ToUpperCase)                    \
    V(String, Trim)                           \
    V(String, GetStringIterator)              \
    V(String, SubStr)                         \
    V(Symbol, Constructor)                    \
    V(Symbol, ToString)                       \
    V(Symbol, ValueOf)                        \
    V(Symbol, For)                            \
    V(Symbol, KeyFor)                         \
    V(Symbol, DescriptionGetter)              \
    V(Symbol, ThisSymbolValue)                \
    V(Symbol, ToPrimitive)                    \
    V(Symbol, SymbolDescriptiveString)        \
    V(TypedArray, BaseConstructor)            \
    V(TypedArray, From)                       \
    V(TypedArray, Of)                         \
    V(TypedArray, Species)                    \
    V(TypedArray, GetBuffer)                  \
    V(TypedArray, GetByteLength)              \
    V(TypedArray, GetByteOffset)              \
    V(TypedArray, CopyWithin)                 \
    V(TypedArray, Entries)                    \
    V(TypedArray, Every)                      \
    V(TypedArray, Filter)                     \
    V(TypedArray, ForEach)                    \
    V(TypedArray, Keys)                       \
    V(TypedArray, GetLength)                  \
    V(TypedArray, Map)                        \
    V(TypedArray, Set)                        \
    V(TypedArray, Slice)                      \
    V(TypedArray, Sort)                       \
    V(TypedArray, Subarray)                   \
    V(TypedArray, Values)                     \
    V(TypedArray, ToStringTag)                \
    V(WeakMap, Constructor)                   \
    V(WeakMap, Delete)                        \
    V(WeakMap, Get)                           \
    V(WeakMap, Has)                           \
    V(WeakMap, Set)                           \
    V(WeakSet, Constructor)                   \
    V(WeakSet, Delete)                        \
    V(WeakSet, Add)                           \
    V(WeakSet, Has)

#define ABSTRACT_OPERATION_LIST(V) \
    V(JSTaggedValue, ToString)     \

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_CALLER_ID(name) INTERPRETER_ID_##name,
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_ID(class, name) BUILTINS_ID_##class##_##name,
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GC_RUNPHASE_ID(name) name##_GC_TRACE_RUNPHASE,
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_ID(class, name) ABSTRACT_ID_##class##_##name,

enum EcmaRuntimeCallerId {
    INTERPRETER_CALLER_LIST(INTERPRETER_CALLER_ID) BUITINS_API_LIST(BUILTINS_API_ID)
    ABSTRACT_OPERATION_LIST(ABSTRACT_OPERATION_ID)
    GC_INITIALIZE,
    RUNTIME_CALLER_NUMBER,
};

#if ECMASCRIPT_ENABLE_RUNTIME_STAT
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_TRACE(thread, name)                                                        \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                           \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat(); \
    RuntimeTimerScope interpret_##name##_scope_(thread, INTERPRETER_CALLER_ID(name) _run_stat_)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BUILTINS_API_TRACE(thread, class, name)                                                \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                           \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat(); \
    RuntimeTimerScope builtins_##class##name##_scope_(thread, BUILTINS_API_ID(class, name) _run_stat_)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_TRACE(thread, class, name)                                          \
    [[maybe_unused]] JSThread *_js_thread_ = thread;                                           \
    [[maybe_unused]] EcmaRuntimeStat *_run_stat_ = _js_thread_->GetEcmaVM()->GetRuntimeStat(); \
    RuntimeTimerScope abstract_##class##name##_scope_(thread, ABSTRACT_OPERATION_ID(class, name) _run_stat_)

#else
#define INTERPRETER_TRACE(thread, name) static_cast<void>(0)  // NOLINT(cppcoreguidelines-macro-usage)
#define BUILTINS_API_TRACE(thread, class, name) static_cast<void>(0)  // NOLINT(cppcoreguidelines-macro-usage)
#define ABSTRACT_OPERATION_TRACE(thread, class, name) static_cast<void>(0)  // NOLINT(cppcoreguidelines-macro-usage)
#endif  // ECMASCRIPT_ENABLE_RUNTIME_STAT
}  // namespace panda::ecmascript
#endif
