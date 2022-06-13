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

#include "ecmascript/snapshot/mem/snapshot_processor.h"

#include "ecmascript/base/error_type.h"
#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_atomics.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtin_cjs_exports.h"
#include "ecmascript/builtins/builtin_cjs_module.h"
#include "ecmascript/builtins/builtin_cjs_require.h"
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_displaynames.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_finalization_registry.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_list_format.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_ref.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_deque.h"
#include "ecmascript/containers/containers_linked_list.h"
#include "ecmascript/containers/containers_list.h"
#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_queue.h"
#include "ecmascript/containers/containers_stack.h"
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/containers/containers_vector.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_linked_list_iterator.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_api_plain_array_iterator.h"
#include "ecmascript/js_api_queue_iterator.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_api_vector_iterator.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"

namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using BuiltinsBigInt = builtins::BuiltinsBigInt;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using DisplayNames = builtins::BuiltinsDisplayNames;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
using BuiltinsWeakRef = builtins::BuiltinsWeakRef;
using BuiltinsFinalizationRegistry = builtins::BuiltinsFinalizationRegistry;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsTypedArray = builtins::BuiltinsTypedArray;
using BuiltinsIterator = builtins::BuiltinsIterator;
using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using ErrorType = base::ErrorType;
using Global = builtins::BuiltinsGlobal;
using BuiltinsString = builtins::BuiltinsString;
using StringIterator = builtins::BuiltinsStringIterator;
using RegExp = builtins::BuiltinsRegExp;
using Function = builtins::BuiltinsFunction;
using Math = builtins::BuiltinsMath;
using Atomics = builtins::BuiltinsAtomics;
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using SharedArrayBuffer = builtins::BuiltinsSharedArrayBuffer;
using Json = builtins::BuiltinsJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
using ListFormat = builtins::BuiltinsListFormat;
using CjsExports = builtins::BuiltinsCjsExports;
using CjsModule = builtins::BuiltinsCjsModule;
using CjsRequire = builtins::BuiltinsCjsRequire;
using ArkTools = builtins::BuiltinsArkTools;

using ErrorType = base::ErrorType;
using DataView = builtins::BuiltinsDataView;
using Intl = builtins::BuiltinsIntl;
using Locale = builtins::BuiltinsLocale;
using DateTimeFormat = builtins::BuiltinsDateTimeFormat;
using NumberFormat = builtins::BuiltinsNumberFormat;
using RelativeTimeFormat = builtins::BuiltinsRelativeTimeFormat;
using Collator = builtins::BuiltinsCollator;
using PluralRules = builtins::BuiltinsPluralRules;
using ArrayList = containers::ContainersArrayList;
using TreeMap = containers::ContainersTreeMap;
using TreeSet = containers::ContainersTreeSet;
using Vector = containers::ContainersVector;
using Queue = containers::ContainersQueue;
using List = containers::ContainersList;
using LinkedList = containers::ContainersLinkedList;
using PlainArray = containers::ContainersPlainArray;
using Deque = containers::ContainersDeque;
using ContainerStack = panda::ecmascript::containers::ContainersStack;
using ContainersPrivate = containers::ContainersPrivate;

constexpr int METHOD_SIZE = sizeof(JSMethod);

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static uintptr_t g_nativeTable[] = {
    reinterpret_cast<uintptr_t>(nullptr),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Species),
    reinterpret_cast<uintptr_t>(StringIterator::Next),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeInvokeSelf),
    reinterpret_cast<uintptr_t>(Function::FunctionConstructor),
    reinterpret_cast<uintptr_t>(JSFunction::AccessCallerArgumentsThrowTypeError),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeApply),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeBind),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeCall),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeToString),
    reinterpret_cast<uintptr_t>(Object::ObjectConstructor),
    reinterpret_cast<uintptr_t>(Object::FromEntries),
    reinterpret_cast<uintptr_t>(Error::ErrorConstructor),
    reinterpret_cast<uintptr_t>(Error::ToString),
    reinterpret_cast<uintptr_t>(RangeError::RangeErrorConstructor),
    reinterpret_cast<uintptr_t>(RangeError::ToString),
    reinterpret_cast<uintptr_t>(ReferenceError::ReferenceErrorConstructor),
    reinterpret_cast<uintptr_t>(ReferenceError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::TypeErrorConstructor),
    reinterpret_cast<uintptr_t>(TypeError::ToString),
    reinterpret_cast<uintptr_t>(TypeError::ThrowTypeError),
    reinterpret_cast<uintptr_t>(URIError::URIErrorConstructor),
    reinterpret_cast<uintptr_t>(URIError::ToString),
    reinterpret_cast<uintptr_t>(SyntaxError::SyntaxErrorConstructor),
    reinterpret_cast<uintptr_t>(SyntaxError::ToString),
    reinterpret_cast<uintptr_t>(EvalError::EvalErrorConstructor),
    reinterpret_cast<uintptr_t>(EvalError::ToString),
    reinterpret_cast<uintptr_t>(Number::NumberConstructor),
    reinterpret_cast<uintptr_t>(Number::ToExponential),
    reinterpret_cast<uintptr_t>(Number::ToFixed),
    reinterpret_cast<uintptr_t>(Number::ToLocaleString),
    reinterpret_cast<uintptr_t>(Number::ToPrecision),
    reinterpret_cast<uintptr_t>(Number::ToString),
    reinterpret_cast<uintptr_t>(Number::ValueOf),
    reinterpret_cast<uintptr_t>(Number::IsFinite),
    reinterpret_cast<uintptr_t>(Number::IsInteger),
    reinterpret_cast<uintptr_t>(Number::IsNaN),
    reinterpret_cast<uintptr_t>(Number::IsSafeInteger),
    reinterpret_cast<uintptr_t>(Number::ParseFloat),
    reinterpret_cast<uintptr_t>(Number::ParseInt),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::BigIntConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsUintN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::AsIntN),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsBigInt::ValueOf),
    reinterpret_cast<uintptr_t>(Symbol::SymbolConstructor),
    reinterpret_cast<uintptr_t>(Symbol::For),
    reinterpret_cast<uintptr_t>(Symbol::KeyFor),
    reinterpret_cast<uintptr_t>(Symbol::DescriptionGetter),
    reinterpret_cast<uintptr_t>(Symbol::ToPrimitive),
    reinterpret_cast<uintptr_t>(Symbol::ToString),
    reinterpret_cast<uintptr_t>(Symbol::ValueOf),
    reinterpret_cast<uintptr_t>(Function::FunctionPrototypeHasInstance),
    reinterpret_cast<uintptr_t>(Date::DateConstructor),
    reinterpret_cast<uintptr_t>(Date::GetDate),
    reinterpret_cast<uintptr_t>(Date::GetDay),
    reinterpret_cast<uintptr_t>(Date::GetFullYear),
    reinterpret_cast<uintptr_t>(Date::GetHours),
    reinterpret_cast<uintptr_t>(Date::GetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetMinutes),
    reinterpret_cast<uintptr_t>(Date::GetMonth),
    reinterpret_cast<uintptr_t>(Date::GetSeconds),
    reinterpret_cast<uintptr_t>(Date::GetTime),
    reinterpret_cast<uintptr_t>(Date::GetTimezoneOffset),
    reinterpret_cast<uintptr_t>(Date::GetUTCDate),
    reinterpret_cast<uintptr_t>(Date::GetUTCDay),
    reinterpret_cast<uintptr_t>(Date::GetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::GetUTCHours),
    reinterpret_cast<uintptr_t>(Date::GetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::GetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::GetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::GetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::SetDate),
    reinterpret_cast<uintptr_t>(Date::SetFullYear),
    reinterpret_cast<uintptr_t>(Date::SetHours),
    reinterpret_cast<uintptr_t>(Date::SetMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetMinutes),
    reinterpret_cast<uintptr_t>(Date::SetMonth),
    reinterpret_cast<uintptr_t>(Date::SetSeconds),
    reinterpret_cast<uintptr_t>(Date::SetTime),
    reinterpret_cast<uintptr_t>(Date::SetUTCDate),
    reinterpret_cast<uintptr_t>(Date::SetUTCFullYear),
    reinterpret_cast<uintptr_t>(Date::SetUTCHours),
    reinterpret_cast<uintptr_t>(Date::SetUTCMilliseconds),
    reinterpret_cast<uintptr_t>(Date::SetUTCMinutes),
    reinterpret_cast<uintptr_t>(Date::SetUTCMonth),
    reinterpret_cast<uintptr_t>(Date::SetUTCSeconds),
    reinterpret_cast<uintptr_t>(Date::ToDateString),
    reinterpret_cast<uintptr_t>(Date::ToISOString),
    reinterpret_cast<uintptr_t>(Date::ToJSON),
    reinterpret_cast<uintptr_t>(Date::ToLocaleDateString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleString),
    reinterpret_cast<uintptr_t>(Date::ToLocaleTimeString),
    reinterpret_cast<uintptr_t>(Date::ToString),
    reinterpret_cast<uintptr_t>(Date::ToTimeString),
    reinterpret_cast<uintptr_t>(Date::ToUTCString),
    reinterpret_cast<uintptr_t>(Date::ValueOf),
    reinterpret_cast<uintptr_t>(Date::ToPrimitive),
    reinterpret_cast<uintptr_t>(Date::Now),
    reinterpret_cast<uintptr_t>(Date::Parse),
    reinterpret_cast<uintptr_t>(Date::UTC),
    reinterpret_cast<uintptr_t>(DisplayNames::DisplayNamesConstructor),
    reinterpret_cast<uintptr_t>(DisplayNames::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(DisplayNames::Of),
    reinterpret_cast<uintptr_t>(DisplayNames::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Object::Assign),
    reinterpret_cast<uintptr_t>(Object::Create),
    reinterpret_cast<uintptr_t>(Object::DefineProperties),
    reinterpret_cast<uintptr_t>(Object::DefineProperty),
    reinterpret_cast<uintptr_t>(Object::Freeze),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyNames),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertySymbols),
    reinterpret_cast<uintptr_t>(Object::GetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::Is),
    reinterpret_cast<uintptr_t>(Object::IsExtensible),
    reinterpret_cast<uintptr_t>(Object::IsFrozen),
    reinterpret_cast<uintptr_t>(Object::IsSealed),
    reinterpret_cast<uintptr_t>(Object::Keys),
    reinterpret_cast<uintptr_t>(Object::PreventExtensions),
    reinterpret_cast<uintptr_t>(Object::Seal),
    reinterpret_cast<uintptr_t>(Object::SetPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::HasOwnProperty),
    reinterpret_cast<uintptr_t>(Object::IsPrototypeOf),
    reinterpret_cast<uintptr_t>(Object::PropertyIsEnumerable),
    reinterpret_cast<uintptr_t>(Object::ToLocaleString),
    reinterpret_cast<uintptr_t>(Object::ToString),
    reinterpret_cast<uintptr_t>(Object::ValueOf),
    reinterpret_cast<uintptr_t>(Object::ProtoGetter),
    reinterpret_cast<uintptr_t>(Object::ProtoSetter),
    reinterpret_cast<uintptr_t>(Object::CreateRealm),
    reinterpret_cast<uintptr_t>(Object::Entries),
    reinterpret_cast<uintptr_t>(Boolean::BooleanConstructor),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeToString),
    reinterpret_cast<uintptr_t>(Boolean::BooleanPrototypeValueOf),
    reinterpret_cast<uintptr_t>(RegExp::RegExpConstructor),
    reinterpret_cast<uintptr_t>(RegExp::Exec),
    reinterpret_cast<uintptr_t>(RegExp::Test),
    reinterpret_cast<uintptr_t>(RegExp::ToString),
    reinterpret_cast<uintptr_t>(RegExp::GetFlags),
    reinterpret_cast<uintptr_t>(RegExp::GetSource),
    reinterpret_cast<uintptr_t>(RegExp::GetGlobal),
    reinterpret_cast<uintptr_t>(RegExp::GetIgnoreCase),
    reinterpret_cast<uintptr_t>(RegExp::GetMultiline),
    reinterpret_cast<uintptr_t>(RegExp::GetDotAll),
    reinterpret_cast<uintptr_t>(RegExp::GetSticky),
    reinterpret_cast<uintptr_t>(RegExp::GetUnicode),
    reinterpret_cast<uintptr_t>(RegExp::Split),
    reinterpret_cast<uintptr_t>(RegExp::Search),
    reinterpret_cast<uintptr_t>(RegExp::Match),
    reinterpret_cast<uintptr_t>(RegExp::MatchAll),
    reinterpret_cast<uintptr_t>(RegExp::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsSet::SetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsSet::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Values),
    reinterpret_cast<uintptr_t>(BuiltinsSet::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsSet::Species),
    reinterpret_cast<uintptr_t>(BuiltinsMap::MapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Clear),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsMap::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Values),
    reinterpret_cast<uintptr_t>(BuiltinsMap::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsMap::GetSize),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::WeakMapConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Set),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Has),
    reinterpret_cast<uintptr_t>(BuiltinsWeakMap::Get),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::WeakSetConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Add),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Delete),
    reinterpret_cast<uintptr_t>(BuiltinsWeakSet::Has),
    reinterpret_cast<uintptr_t>(BuiltinsWeakRef::WeakRefConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsWeakRef::Deref),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::FinalizationRegistryConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::Register),
    reinterpret_cast<uintptr_t>(BuiltinsFinalizationRegistry::Unregister),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Pop),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Push),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Shift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Splice),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Unshift),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsArray::IsArray),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Unscopables),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsArray::Flat),
    reinterpret_cast<uintptr_t>(BuiltinsArray::FlatMap),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::TypedArrayBaseConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::CopyWithin),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Entries),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Every),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Fill),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Filter),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Find),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::FindIndex),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ForEach),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Join),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Keys),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Map),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reduce),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ReduceRight),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Reverse),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Set),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Some),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Sort),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Subarray),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToLocaleString),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Values),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetBuffer),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetByteOffset),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::GetLength),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::ToStringTag),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::From),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Of),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Species),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ClampedArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::BigInt64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::BigUint64ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::StringConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CharCodeAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::CodePointAt),
    reinterpret_cast<uintptr_t>(BuiltinsString::Concat),
    reinterpret_cast<uintptr_t>(BuiltinsString::EndsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Includes),
    reinterpret_cast<uintptr_t>(BuiltinsString::IndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LastIndexOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::LocaleCompare),
    reinterpret_cast<uintptr_t>(BuiltinsString::Match),
    reinterpret_cast<uintptr_t>(BuiltinsString::MatchAll),
    reinterpret_cast<uintptr_t>(BuiltinsString::Normalize),
    reinterpret_cast<uintptr_t>(BuiltinsString::PadEnd),
    reinterpret_cast<uintptr_t>(BuiltinsString::PadStart),
    reinterpret_cast<uintptr_t>(BuiltinsString::Repeat),
    reinterpret_cast<uintptr_t>(BuiltinsString::Replace),
    reinterpret_cast<uintptr_t>(BuiltinsString::Search),
    reinterpret_cast<uintptr_t>(BuiltinsString::Slice),
    reinterpret_cast<uintptr_t>(BuiltinsString::Split),
    reinterpret_cast<uintptr_t>(BuiltinsString::StartsWith),
    reinterpret_cast<uintptr_t>(BuiltinsString::Substring),
    reinterpret_cast<uintptr_t>(BuiltinsString::SubStr),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLocaleUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToLowerCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToString),
    reinterpret_cast<uintptr_t>(BuiltinsString::ToUpperCase),
    reinterpret_cast<uintptr_t>(BuiltinsString::Trim),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimStart),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimEnd),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimLeft),
    reinterpret_cast<uintptr_t>(BuiltinsString::TrimRight),
    reinterpret_cast<uintptr_t>(BuiltinsString::ValueOf),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetStringIterator),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCharCode),
    reinterpret_cast<uintptr_t>(BuiltinsString::FromCodePoint),
    reinterpret_cast<uintptr_t>(BuiltinsString::Raw),
    reinterpret_cast<uintptr_t>(BuiltinsString::GetLength),
    reinterpret_cast<uintptr_t>(ArrayBuffer::ArrayBufferConstructor),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Slice),
    reinterpret_cast<uintptr_t>(ArrayBuffer::IsView),
    reinterpret_cast<uintptr_t>(ArrayBuffer::Species),
    reinterpret_cast<uintptr_t>(ArrayBuffer::GetByteLength),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::SharedArrayBufferConstructor),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::IsSharedArrayBuffer),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::Species),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::GetByteLength),
    reinterpret_cast<uintptr_t>(SharedArrayBuffer::Slice),
    reinterpret_cast<uintptr_t>(DataView::DataViewConstructor),
    reinterpret_cast<uintptr_t>(DataView::GetFloat32),
    reinterpret_cast<uintptr_t>(DataView::GetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetInt8),
    reinterpret_cast<uintptr_t>(DataView::GetInt16),
    reinterpret_cast<uintptr_t>(DataView::GetInt32),
    reinterpret_cast<uintptr_t>(DataView::GetUint8),
    reinterpret_cast<uintptr_t>(DataView::GetUint16),
    reinterpret_cast<uintptr_t>(DataView::GetUint32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat32),
    reinterpret_cast<uintptr_t>(DataView::SetFloat64),
    reinterpret_cast<uintptr_t>(DataView::GetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::GetBigUint64),
    reinterpret_cast<uintptr_t>(DataView::SetInt8),
    reinterpret_cast<uintptr_t>(DataView::SetInt16),
    reinterpret_cast<uintptr_t>(DataView::SetInt32),
    reinterpret_cast<uintptr_t>(DataView::SetUint8),
    reinterpret_cast<uintptr_t>(DataView::SetUint16),
    reinterpret_cast<uintptr_t>(DataView::SetUint32),
    reinterpret_cast<uintptr_t>(DataView::GetBuffer),
    reinterpret_cast<uintptr_t>(DataView::GetByteLength),
    reinterpret_cast<uintptr_t>(DataView::GetOffset),
    reinterpret_cast<uintptr_t>(DataView::SetBigInt64),
    reinterpret_cast<uintptr_t>(DataView::SetBigUint64),
    reinterpret_cast<uintptr_t>(Global::PrintEntrypoint),
    reinterpret_cast<uintptr_t>(Global::NotSupportEval),
    reinterpret_cast<uintptr_t>(Global::IsFinite),
    reinterpret_cast<uintptr_t>(Global::IsNaN),
    reinterpret_cast<uintptr_t>(Global::DecodeURI),
    reinterpret_cast<uintptr_t>(Global::DecodeURIComponent),
    reinterpret_cast<uintptr_t>(Global::EncodeURI),
    reinterpret_cast<uintptr_t>(Global::EncodeURIComponent),
    reinterpret_cast<uintptr_t>(Math::Abs),
    reinterpret_cast<uintptr_t>(Math::Acos),
    reinterpret_cast<uintptr_t>(Math::Acosh),
    reinterpret_cast<uintptr_t>(Math::Asin),
    reinterpret_cast<uintptr_t>(Math::Asinh),
    reinterpret_cast<uintptr_t>(Math::Atan),
    reinterpret_cast<uintptr_t>(Math::Atanh),
    reinterpret_cast<uintptr_t>(Math::Atan2),
    reinterpret_cast<uintptr_t>(Math::Cbrt),
    reinterpret_cast<uintptr_t>(Math::Ceil),
    reinterpret_cast<uintptr_t>(Math::Clz32),
    reinterpret_cast<uintptr_t>(Math::Cos),
    reinterpret_cast<uintptr_t>(Math::Cosh),
    reinterpret_cast<uintptr_t>(Math::Exp),
    reinterpret_cast<uintptr_t>(Math::Expm1),
    reinterpret_cast<uintptr_t>(Math::Floor),
    reinterpret_cast<uintptr_t>(Math::Fround),
    reinterpret_cast<uintptr_t>(Math::Hypot),
    reinterpret_cast<uintptr_t>(Math::Imul),
    reinterpret_cast<uintptr_t>(Math::Log),
    reinterpret_cast<uintptr_t>(Math::Log1p),
    reinterpret_cast<uintptr_t>(Math::Log10),
    reinterpret_cast<uintptr_t>(Math::Log2),
    reinterpret_cast<uintptr_t>(Math::Max),
    reinterpret_cast<uintptr_t>(Math::Min),
    reinterpret_cast<uintptr_t>(Math::Pow),
    reinterpret_cast<uintptr_t>(Math::Random),
    reinterpret_cast<uintptr_t>(Math::Round),
    reinterpret_cast<uintptr_t>(Math::Sign),
    reinterpret_cast<uintptr_t>(Math::Sin),
    reinterpret_cast<uintptr_t>(Math::Sinh),
    reinterpret_cast<uintptr_t>(Math::Sqrt),
    reinterpret_cast<uintptr_t>(Math::Tan),
    reinterpret_cast<uintptr_t>(Math::Tanh),
    reinterpret_cast<uintptr_t>(Math::Trunc),
    reinterpret_cast<uintptr_t>(Atomics::Wait),
    reinterpret_cast<uintptr_t>(Atomics::Exchange),
    reinterpret_cast<uintptr_t>(Atomics::CompareExchange),
    reinterpret_cast<uintptr_t>(Atomics::IsLockFree),
    reinterpret_cast<uintptr_t>(Atomics::Store),
    reinterpret_cast<uintptr_t>(Atomics::Load),
    reinterpret_cast<uintptr_t>(Atomics::Notify),
    reinterpret_cast<uintptr_t>(Atomics::Xor),
    reinterpret_cast<uintptr_t>(Atomics::Or),
    reinterpret_cast<uintptr_t>(Atomics::Sub),
    reinterpret_cast<uintptr_t>(Atomics::And),
    reinterpret_cast<uintptr_t>(Atomics::Add),
    reinterpret_cast<uintptr_t>(Json::Parse),
    reinterpret_cast<uintptr_t>(Json::Stringify),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Next),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Return),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Throw),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSForInIterator::Next),
    reinterpret_cast<uintptr_t>(JSRegExpIterator::Next),
    reinterpret_cast<uintptr_t>(JSSetIterator::Next),
    reinterpret_cast<uintptr_t>(JSMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSArrayIterator::Next),
    reinterpret_cast<uintptr_t>(Proxy::ProxyConstructor),
    reinterpret_cast<uintptr_t>(Proxy::Revocable),
    reinterpret_cast<uintptr_t>(Reflect::ReflectApply),
    reinterpret_cast<uintptr_t>(Reflect::ReflectConstruct),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDefineProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectDeleteProperty),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetOwnPropertyDescriptor),
    reinterpret_cast<uintptr_t>(Reflect::ReflectGetPrototypeOf),
    reinterpret_cast<uintptr_t>(Reflect::ReflectHas),
    reinterpret_cast<uintptr_t>(Reflect::ReflectIsExtensible),
    reinterpret_cast<uintptr_t>(Reflect::ReflectOwnKeys),
    reinterpret_cast<uintptr_t>(Reflect::ReflectPreventExtensions),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSet),
    reinterpret_cast<uintptr_t>(Reflect::ReflectSetPrototypeOf),
    reinterpret_cast<uintptr_t>(AsyncFunction::AsyncFunctionConstructor),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeNext),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeReturn),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorPrototypeThrow),
    reinterpret_cast<uintptr_t>(GeneratorObject::GeneratorFunctionConstructor),
    reinterpret_cast<uintptr_t>(Promise::PromiseConstructor),
    reinterpret_cast<uintptr_t>(Promise::All),
    reinterpret_cast<uintptr_t>(Promise::Race),
    reinterpret_cast<uintptr_t>(Promise::Resolve),
    reinterpret_cast<uintptr_t>(Promise::Reject),
    reinterpret_cast<uintptr_t>(Promise::Catch),
    reinterpret_cast<uintptr_t>(Promise::Then),
    reinterpret_cast<uintptr_t>(Promise::GetSpecies),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseReactionJob),
    reinterpret_cast<uintptr_t>(BuiltinsPromiseJob::PromiseResolveThenableJob),
    reinterpret_cast<uintptr_t>(Intl::GetCanonicalLocales),
    reinterpret_cast<uintptr_t>(Locale::LocaleConstructor),
    reinterpret_cast<uintptr_t>(Locale::Maximize),
    reinterpret_cast<uintptr_t>(Locale::Minimize),
    reinterpret_cast<uintptr_t>(Locale::ToString),
    reinterpret_cast<uintptr_t>(Locale::GetBaseName),
    reinterpret_cast<uintptr_t>(Locale::GetCalendar),
    reinterpret_cast<uintptr_t>(Locale::GetCaseFirst),
    reinterpret_cast<uintptr_t>(Locale::GetCollation),
    reinterpret_cast<uintptr_t>(Locale::GetHourCycle),
    reinterpret_cast<uintptr_t>(Locale::GetNumeric),
    reinterpret_cast<uintptr_t>(Locale::GetNumberingSystem),
    reinterpret_cast<uintptr_t>(Locale::GetLanguage),
    reinterpret_cast<uintptr_t>(Locale::GetScript),
    reinterpret_cast<uintptr_t>(Locale::GetRegion),
    reinterpret_cast<uintptr_t>(DateTimeFormat::DateTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(DateTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(DateTimeFormat::Format),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(DateTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRange),
    reinterpret_cast<uintptr_t>(DateTimeFormat::FormatRangeToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::NumberFormatConstructor),
    reinterpret_cast<uintptr_t>(NumberFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(NumberFormat::Format),
    reinterpret_cast<uintptr_t>(NumberFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(NumberFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(NumberFormat::NumberFormatInternalFormatNumber),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::RelativeTimeFormatConstructor),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::Format),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(RelativeTimeFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(Collator::CollatorConstructor),
    reinterpret_cast<uintptr_t>(Collator::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(Collator::Compare),
    reinterpret_cast<uintptr_t>(Collator::ResolvedOptions),
    reinterpret_cast<uintptr_t>(PluralRules::PluralRulesConstructor),
    reinterpret_cast<uintptr_t>(PluralRules::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(PluralRules::Select),
    reinterpret_cast<uintptr_t>(PluralRules::ResolvedOptions),
    reinterpret_cast<uintptr_t>(ListFormat::ListFormatConstructor),
    reinterpret_cast<uintptr_t>(ListFormat::SupportedLocalesOf),
    reinterpret_cast<uintptr_t>(ListFormat::Format),
    reinterpret_cast<uintptr_t>(ListFormat::FormatToParts),
    reinterpret_cast<uintptr_t>(ListFormat::ResolvedOptions),
    reinterpret_cast<uintptr_t>(CjsExports::CjsExportsConstructor),
    reinterpret_cast<uintptr_t>(CjsModule::CjsModuleConstructor),
    reinterpret_cast<uintptr_t>(CjsModule::Compiler),
    reinterpret_cast<uintptr_t>(CjsModule::Load),
    reinterpret_cast<uintptr_t>(CjsModule::Require),
    reinterpret_cast<uintptr_t>(CjsModule::GetExportsForCircularRequire),
    reinterpret_cast<uintptr_t>(CjsModule::UpdateChildren),
    reinterpret_cast<uintptr_t>(CjsModule::ResolveFilename),
    reinterpret_cast<uintptr_t>(CjsRequire::CjsRequireConstructor),
    reinterpret_cast<uintptr_t>(CjsRequire::Main),
    reinterpret_cast<uintptr_t>(CjsRequire::Resolve),
    reinterpret_cast<uintptr_t>(ArkTools::ObjectDump),
    reinterpret_cast<uintptr_t>(ArkTools::CompareHClass),
    reinterpret_cast<uintptr_t>(ArkTools::DumpHClass),

    // non ECMA standard jsapi containers.
    reinterpret_cast<uintptr_t>(ContainersPrivate::Load),
    reinterpret_cast<uintptr_t>(ArrayList::ArrayListConstructor),
    reinterpret_cast<uintptr_t>(ArrayList::Add),
    reinterpret_cast<uintptr_t>(ArrayList::Insert),
    reinterpret_cast<uintptr_t>(ArrayList::Clear),
    reinterpret_cast<uintptr_t>(ArrayList::Clone),
    reinterpret_cast<uintptr_t>(ArrayList::Has),
    reinterpret_cast<uintptr_t>(ArrayList::GetCapacity),
    reinterpret_cast<uintptr_t>(ArrayList::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(ArrayList::TrimToCurrentLength),
    reinterpret_cast<uintptr_t>(ArrayList::GetIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::IsEmpty),
    reinterpret_cast<uintptr_t>(ArrayList::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByIndex),
    reinterpret_cast<uintptr_t>(ArrayList::Remove),
    reinterpret_cast<uintptr_t>(ArrayList::RemoveByRange),
    reinterpret_cast<uintptr_t>(ArrayList::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(ArrayList::SubArrayList),
    reinterpret_cast<uintptr_t>(ArrayList::ConvertToArray),
    reinterpret_cast<uintptr_t>(ArrayList::ForEach),
    reinterpret_cast<uintptr_t>(ArrayList::GetIteratorObj),
    reinterpret_cast<uintptr_t>(ArrayList::Get),
    reinterpret_cast<uintptr_t>(ArrayList::Set),
    reinterpret_cast<uintptr_t>(ArrayList::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIArrayListIterator::Next),
    reinterpret_cast<uintptr_t>(TreeMap::TreeMapConstructor),
    reinterpret_cast<uintptr_t>(TreeMap::Set),
    reinterpret_cast<uintptr_t>(TreeMap::Get),
    reinterpret_cast<uintptr_t>(TreeMap::Remove),
    reinterpret_cast<uintptr_t>(TreeMap::GetFirstKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLastKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetLowerKey),
    reinterpret_cast<uintptr_t>(TreeMap::GetHigherKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasKey),
    reinterpret_cast<uintptr_t>(TreeMap::HasValue),
    reinterpret_cast<uintptr_t>(TreeMap::SetAll),
    reinterpret_cast<uintptr_t>(TreeMap::Replace),
    reinterpret_cast<uintptr_t>(TreeMap::Keys),
    reinterpret_cast<uintptr_t>(TreeMap::Values),
    reinterpret_cast<uintptr_t>(TreeMap::Entries),
    reinterpret_cast<uintptr_t>(TreeMap::ForEach),
    reinterpret_cast<uintptr_t>(TreeMap::Clear),
    reinterpret_cast<uintptr_t>(TreeMap::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeMap::GetLength),
    reinterpret_cast<uintptr_t>(TreeSet::TreeSetConstructor),
    reinterpret_cast<uintptr_t>(TreeSet::Add),
    reinterpret_cast<uintptr_t>(TreeSet::Has),
    reinterpret_cast<uintptr_t>(TreeSet::Remove),
    reinterpret_cast<uintptr_t>(TreeSet::GetFirstValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLastValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetLowerValue),
    reinterpret_cast<uintptr_t>(TreeSet::GetHigherValue),
    reinterpret_cast<uintptr_t>(TreeSet::PopFirst),
    reinterpret_cast<uintptr_t>(TreeSet::PopLast),
    reinterpret_cast<uintptr_t>(TreeSet::IsEmpty),
    reinterpret_cast<uintptr_t>(TreeSet::Values),
    reinterpret_cast<uintptr_t>(TreeSet::Entries),
    reinterpret_cast<uintptr_t>(TreeSet::ForEach),
    reinterpret_cast<uintptr_t>(TreeSet::Clear),
    reinterpret_cast<uintptr_t>(TreeSet::GetLength),
    reinterpret_cast<uintptr_t>(JSAPITreeMapIterator::Next),
    reinterpret_cast<uintptr_t>(JSAPITreeSetIterator::Next),
    reinterpret_cast<uintptr_t>(Deque::DequeConstructor),
    reinterpret_cast<uintptr_t>(Deque::InsertFront),
    reinterpret_cast<uintptr_t>(Deque::InsertEnd),
    reinterpret_cast<uintptr_t>(Deque::GetFirst),
    reinterpret_cast<uintptr_t>(Deque::GetLast),
    reinterpret_cast<uintptr_t>(Deque::Has),
    reinterpret_cast<uintptr_t>(Deque::PopFirst),
    reinterpret_cast<uintptr_t>(Deque::PopLast),
    reinterpret_cast<uintptr_t>(Deque::ForEach),
    reinterpret_cast<uintptr_t>(Deque::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Deque::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIDequeIterator::Next),
    reinterpret_cast<uintptr_t>(Vector::VectorConstructor),
    reinterpret_cast<uintptr_t>(Vector::Add),
    reinterpret_cast<uintptr_t>(Vector::Insert),
    reinterpret_cast<uintptr_t>(Vector::SetLength),
    reinterpret_cast<uintptr_t>(Vector::GetCapacity),
    reinterpret_cast<uintptr_t>(Vector::IncreaseCapacityTo),
    reinterpret_cast<uintptr_t>(Vector::Get),
    reinterpret_cast<uintptr_t>(Vector::GetIndexOf),
    reinterpret_cast<uintptr_t>(Vector::GetIndexFrom),
    reinterpret_cast<uintptr_t>(Vector::IsEmpty),
    reinterpret_cast<uintptr_t>(Vector::GetLastElement),
    reinterpret_cast<uintptr_t>(Vector::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(Vector::GetLastIndexFrom),
    reinterpret_cast<uintptr_t>(Vector::Remove),
    reinterpret_cast<uintptr_t>(Vector::RemoveByIndex),
    reinterpret_cast<uintptr_t>(Vector::RemoveByRange),
    reinterpret_cast<uintptr_t>(Vector::Set),
    reinterpret_cast<uintptr_t>(Vector::SubVector),
    reinterpret_cast<uintptr_t>(Vector::ToString),
    reinterpret_cast<uintptr_t>(Vector::GetSize),
    reinterpret_cast<uintptr_t>(Vector::ForEach),
    reinterpret_cast<uintptr_t>(Vector::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(Vector::TrimToCurrentLength),
    reinterpret_cast<uintptr_t>(Vector::Clear),
    reinterpret_cast<uintptr_t>(Vector::Clone),
    reinterpret_cast<uintptr_t>(Vector::Has),
    reinterpret_cast<uintptr_t>(Vector::GetFirstElement),
    reinterpret_cast<uintptr_t>(Vector::CopyToArray),
    reinterpret_cast<uintptr_t>(Vector::ConvertToArray),
    reinterpret_cast<uintptr_t>(Vector::Sort),
    reinterpret_cast<uintptr_t>(Vector::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSAPIVectorIterator::Next),
    reinterpret_cast<uintptr_t>(Queue::QueueConstructor),
    reinterpret_cast<uintptr_t>(Queue::Add),
    reinterpret_cast<uintptr_t>(Queue::GetFirst),
    reinterpret_cast<uintptr_t>(Queue::Pop),
    reinterpret_cast<uintptr_t>(Queue::ForEach),
    reinterpret_cast<uintptr_t>(Queue::GetIteratorObj),
    reinterpret_cast<uintptr_t>(Queue::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIQueueIterator::Next),
    reinterpret_cast<uintptr_t>(PlainArray::PlainArrayConstructor),
    reinterpret_cast<uintptr_t>(PlainArray::Add),
    reinterpret_cast<uintptr_t>(PlainArray::Clear),
    reinterpret_cast<uintptr_t>(PlainArray::Clone),
    reinterpret_cast<uintptr_t>(PlainArray::Has),
    reinterpret_cast<uintptr_t>(PlainArray::Get),
    reinterpret_cast<uintptr_t>(PlainArray::GetIteratorObj),
    reinterpret_cast<uintptr_t>(PlainArray::ForEach),
    reinterpret_cast<uintptr_t>(PlainArray::ToString),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfKey),
    reinterpret_cast<uintptr_t>(PlainArray::GetIndexOfValue),
    reinterpret_cast<uintptr_t>(PlainArray::IsEmpty),
    reinterpret_cast<uintptr_t>(PlainArray::GetKeyAt),
    reinterpret_cast<uintptr_t>(PlainArray::Remove),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveAt),
    reinterpret_cast<uintptr_t>(PlainArray::RemoveRangeFrom),
    reinterpret_cast<uintptr_t>(PlainArray::SetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetValueAt),
    reinterpret_cast<uintptr_t>(PlainArray::GetSize),
    reinterpret_cast<uintptr_t>(JSAPIPlainArrayIterator::Next),
    reinterpret_cast<uintptr_t>(ContainerStack::StackConstructor),
    reinterpret_cast<uintptr_t>(ContainerStack::Iterator),
    reinterpret_cast<uintptr_t>(ContainerStack::IsEmpty),
    reinterpret_cast<uintptr_t>(ContainerStack::Push),
    reinterpret_cast<uintptr_t>(ContainerStack::Peek),
    reinterpret_cast<uintptr_t>(ContainerStack::Pop),
    reinterpret_cast<uintptr_t>(ContainerStack::Locate),
    reinterpret_cast<uintptr_t>(ContainerStack::ForEach),
    reinterpret_cast<uintptr_t>(ContainerStack::GetLength),
    reinterpret_cast<uintptr_t>(JSAPIStackIterator::Next),
    reinterpret_cast<uintptr_t>(List::ListConstructor),
    reinterpret_cast<uintptr_t>(List::Add),
    reinterpret_cast<uintptr_t>(List::GetFirst),
    reinterpret_cast<uintptr_t>(List::GetLast),
    reinterpret_cast<uintptr_t>(List::Insert),
    reinterpret_cast<uintptr_t>(List::Clear),
    reinterpret_cast<uintptr_t>(List::RemoveByIndex),
    reinterpret_cast<uintptr_t>(List::Remove),
    reinterpret_cast<uintptr_t>(List::Has),
    reinterpret_cast<uintptr_t>(List::IsEmpty),
    reinterpret_cast<uintptr_t>(List::Get),
    reinterpret_cast<uintptr_t>(List::GetIndexOf),
    reinterpret_cast<uintptr_t>(List::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(List::Set),
    reinterpret_cast<uintptr_t>(List::ForEach),
    reinterpret_cast<uintptr_t>(List::ReplaceAllElements),
    reinterpret_cast<uintptr_t>(List::GetIteratorObj),
    reinterpret_cast<uintptr_t>(List::Equal),
    reinterpret_cast<uintptr_t>(List::Sort),
    reinterpret_cast<uintptr_t>(List::ConvertToArray),
    reinterpret_cast<uintptr_t>(List::GetSubList),
    reinterpret_cast<uintptr_t>(List::Length),
    reinterpret_cast<uintptr_t>(JSAPIListIterator::Next),
    reinterpret_cast<uintptr_t>(LinkedList::LinkedListConstructor),
    reinterpret_cast<uintptr_t>(LinkedList::Add),
    reinterpret_cast<uintptr_t>(LinkedList::GetFirst),
    reinterpret_cast<uintptr_t>(LinkedList::GetLast),
    reinterpret_cast<uintptr_t>(LinkedList::Insert),
    reinterpret_cast<uintptr_t>(LinkedList::AddFirst),
    reinterpret_cast<uintptr_t>(LinkedList::Clear),
    reinterpret_cast<uintptr_t>(LinkedList::Clone),
    reinterpret_cast<uintptr_t>(LinkedList::Has),
    reinterpret_cast<uintptr_t>(LinkedList::Get),
    reinterpret_cast<uintptr_t>(LinkedList::GetIndexOf),
    reinterpret_cast<uintptr_t>(LinkedList::GetLastIndexOf),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveByIndex),
    reinterpret_cast<uintptr_t>(LinkedList::Remove),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveFirst),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveLast),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveFirstFound),
    reinterpret_cast<uintptr_t>(LinkedList::RemoveLastFound),
    reinterpret_cast<uintptr_t>(LinkedList::Set),
    reinterpret_cast<uintptr_t>(LinkedList::ConvertToArray),
    reinterpret_cast<uintptr_t>(LinkedList::ForEach),
    reinterpret_cast<uintptr_t>(JSAPILinkedListIterator::Next),

    // not builtins method
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeSetter),
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeGetter),
    reinterpret_cast<uintptr_t>(JSFunction::NameGetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthSetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthGetter),
    reinterpret_cast<uintptr_t>(JSPandaFileManager::RemoveJSPandaFile),
    reinterpret_cast<uintptr_t>(JSPandaFileManager::GetInstance)
};

void SnapshotProcessor::Initialize()
{
    auto heap = const_cast<Heap *>(vm_->GetHeap());
    size_t oldSpaceCapacity = heap->GetOldSpace()->GetInitialCapacity();
    oldLocalSpace_ = new LocalSpace(heap, oldSpaceCapacity, oldSpaceCapacity);
    size_t nonMovableCapacity = heap->GetNonMovableSpace()->GetInitialCapacity();
    nonMovableLocalSpace_ = new LocalSpace(heap, nonMovableCapacity, nonMovableCapacity);
    size_t machineCodeCapacity = heap->GetMachineCodeSpace()->GetInitialCapacity();
    machineCodeLocalSpace_ = new LocalSpace(heap, machineCodeCapacity, machineCodeCapacity);
    size_t snapshotSpaceCapacity = heap->GetSnapshotSpace()->GetMaximumCapacity();
    snapshotLocalSpace_ = new SnapshotSpace(heap, snapshotSpaceCapacity, snapshotSpaceCapacity);
}

void SnapshotProcessor::StopAllocate()
{
    oldLocalSpace_->Stop();
    nonMovableLocalSpace_->Stop();
    machineCodeLocalSpace_->Stop();
    snapshotLocalSpace_->Stop();
}

void SnapshotProcessor::WriteObjectToFile(std::fstream &writer)
{
    WriteSpaceObjectToFile(oldLocalSpace_, writer);
    WriteSpaceObjectToFile(nonMovableLocalSpace_, writer);
    WriteSpaceObjectToFile(machineCodeLocalSpace_, writer);
    WriteSpaceObjectToFile(snapshotLocalSpace_, writer);
}

void SnapshotProcessor::WriteSpaceObjectToFile(Space* space, std::fstream &writer)
{
    size_t regionCount = space->GetRegionCount();
    if (regionCount > 0) {
        size_t alignedRegionObjSize = AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        auto lastRegion = space->GetCurrentRegion();
        space->EnumerateRegions([&writer, lastRegion, alignedRegionObjSize](Region *current) {
            if (current != lastRegion) {
                // fixme: Except for the last region of a space,
                // currently the snapshot feature assumes that every serialized region must have fixed size.
                // The original region size plus the aligned region object size should not exceed DEFAULT_REGION_SIZE.
                // Currently we even harden it to make them exactly equal to avoid writing dirty / invalid data to the
                // file. Because in the snapshot file the region object and the associated region will be serialized
                // together to an area which has the fixed size of DEFAULT_REGION_SIZE.
                // Need to relax this assumption / limitation.
                ASSERT(alignedRegionObjSize + (current->end_ - ToUintPtr(current->markGCBitset_)) ==
                       DEFAULT_REGION_SIZE);

                // Firstly, serialize the region object into the file;
                writer.write(reinterpret_cast<char *>(current), alignedRegionObjSize);
                // Secondly, write the valid region memory (from the GC bit set at the beginning to end).
                writer.write(reinterpret_cast<char *>(current->markGCBitset_),
                             DEFAULT_REGION_SIZE - alignedRegionObjSize);
                writer.flush();
            }
        });
        // Firstly, serialize the region object into the file;
        writer.write(reinterpret_cast<char *>(lastRegion), alignedRegionObjSize);
        // Secondly, write the valid region memory (from the GC bit set at the beginning to high water mark).
        writer.write(reinterpret_cast<char *>(lastRegion->markGCBitset_),
                     lastRegion->highWaterMark_ - ToUintPtr(lastRegion->markGCBitset_));
        writer.flush();
        space->ReclaimRegions();
    }
}

std::vector<uint32_t> SnapshotProcessor::StatisticsObjectSize()
{
    std::vector<uint32_t> objSizeVector;
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(oldLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(nonMovableLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(machineCodeLocalSpace_));
    objSizeVector.emplace_back(StatisticsSpaceObjectSize(snapshotLocalSpace_));
    return objSizeVector;
}

uint32_t SnapshotProcessor::StatisticsSpaceObjectSize(Space* space)
{
    size_t regionCount = space->GetRegionCount();
    size_t objSize = 0U;
    if (regionCount > 0) {
        auto lastRegion = space->GetCurrentRegion();
        size_t alignedRegionObjSize = AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        size_t lastRegionSize = lastRegion->highWaterMark_ - ToUintPtr(lastRegion->markGCBitset_);
        // fixme: Except for the last region of a space,
        // currently the snapshot feature assumes that every serialized region must have fixed size.
        // The original region size plus the aligned region object size should not exceed DEFAULT_REGION_SIZE.
        // Because in the snapshot file the region object and the associated region will be serialized
        // together to an area which has the fixed size of DEFAULT_REGION_SIZE.
        // Need to relax this assumption / limitation.
        objSize = (regionCount - 1) * DEFAULT_REGION_SIZE + alignedRegionObjSize + lastRegionSize;
    }
    ASSERT(objSize <= Constants::MAX_UINT_32);
    return static_cast<uint32_t>(objSize);
}

void SnapshotProcessor::ProcessObjectQueue(CQueue<TaggedObject *> *queue,
                                           std::unordered_map<uint64_t, ObjectEncode> *data)
{
    while (!queue->empty()) {
        auto taggedObject = queue->front();
        if (taggedObject == nullptr) {
            break;
        }
        queue->pop();
        SerializeObject(taggedObject, queue, data);
    }

    StopAllocate();
}

uintptr_t SnapshotProcessor::AllocateObjectToLocalSpace(Space *space, size_t objectSize)
{
    uintptr_t newObj = 0;
    if (space->GetSpaceType() != MemSpaceType::SNAPSHOT_SPACE) {
        newObj = reinterpret_cast<LocalSpace *>(space)->Allocate(objectSize);
    } else {
        newObj = reinterpret_cast<SnapshotSpace *>(space)->Allocate(objectSize);
    }
    auto current = space->GetCurrentRegion();
    if (newObj == current->GetBegin()) {
        regionIndex_++;
        current->GetMarkGCBitset()->SetGCWords(regionIndex_-1);
    }
    return newObj;
}

void SnapshotProcessor::SetObjectEncodeField(uintptr_t obj, size_t offset, uint64_t value)
{
    *reinterpret_cast<uint64_t *>(obj + offset) = value;
}

void SnapshotProcessor::DeserializeObjectExcludeString(uintptr_t oldSpaceBegin, size_t oldSpaceObjSize,
                                                       size_t nonMovableObjSize, size_t machineCodeObjSize,
                                                       size_t snapshotObjSize)
{
    uintptr_t nonMovableBegin = oldSpaceBegin + oldSpaceObjSize;
    uintptr_t machineCodeBegin = nonMovableBegin + nonMovableObjSize;
    uintptr_t snapshotBegin = machineCodeBegin + machineCodeObjSize;
    auto heap = vm_->GetHeap();
    auto oldSpace = heap->GetOldSpace();
    auto nonMovableSpace = heap->GetNonMovableSpace();
    auto machineCodeSpace = heap->GetMachineCodeSpace();
    auto snapshotSpace = heap->GetSnapshotSpace();
    DeserializeSpaceObject(oldSpaceBegin, oldSpace, oldSpaceObjSize);
    DeserializeSpaceObject(nonMovableBegin, nonMovableSpace, nonMovableObjSize);
    DeserializeSpaceObject(machineCodeBegin, machineCodeSpace, machineCodeObjSize);
    DeserializeSpaceObject(snapshotBegin, snapshotSpace, snapshotObjSize);
    snapshotSpace->ResetAllocator();
}

void SnapshotProcessor::DeserializeSpaceObject(uintptr_t beginAddr, Space* space, size_t spaceObjSize)
{
    size_t numberOfRegions = 0U;
    if (spaceObjSize != 0) {
        numberOfRegions = (spaceObjSize - 1) / DEFAULT_REGION_SIZE + 1; // round up
    }
    for (size_t i = 0; i < numberOfRegions; i++) {
        Region *region = vm_->GetHeapRegionAllocator()->AllocateAlignedRegion(
            space, DEFAULT_REGION_SIZE, vm_->GetAssociatedJSThread());
        auto fileRegion = ToNativePtr<Region>(beginAddr + i * DEFAULT_REGION_SIZE);
        uintptr_t oldMarkGCBitsetAddr =
            ToUintPtr(fileRegion) + AlignUp(sizeof(Region),  static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        uint32_t regionIndex = *(reinterpret_cast<GCBitset *>(oldMarkGCBitsetAddr)->Words());
        regionIndexMap_.emplace(regionIndex, region);

        size_t copyBytes = fileRegion->highWaterMark_ - fileRegion->begin_;
        // Retrieve the data beginning address based on the serialized data format.
        uintptr_t copyFrom = oldMarkGCBitsetAddr + (fileRegion->begin_ - ToUintPtr(fileRegion->markGCBitset_));
        ASSERT(copyBytes <= region->end_ - region->begin_);

        if (memcpy_s(ToVoidPtr(region->begin_),
                     copyBytes,
                     ToVoidPtr(copyFrom),
                     copyBytes) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }

        region->highWaterMark_ = region->begin_ + copyBytes;
        // Other information like aliveObject size, wasted size etc. in the region object to restore.
        region->aliveObject_ = fileRegion->AliveObject();
        region->wasted_ = fileRegion->wasted_;

        region->SetGCFlag(RegionGCFlags::NEED_RELOCATE);

        size_t liveObjectSize = region->GetHighWaterMark() - region->GetBegin();
        if (space->GetSpaceType() != MemSpaceType::SNAPSHOT_SPACE) {
            auto sparseSpace = reinterpret_cast<SparseSpace *>(space);
            region->InitializeFreeObjectSets();
            sparseSpace->FreeLiveRange(region, region->GetHighWaterMark(), region->GetEnd(), true);
            sparseSpace->IncreaseLiveObjectSize(liveObjectSize);
            sparseSpace->IncreaseAllocatedSize(liveObjectSize);
            sparseSpace->AddRegionToFront(region);
        } else {
            auto snapshotSpace = reinterpret_cast<SnapshotSpace *>(space);
            snapshotSpace->IncreaseLiveObjectSize(liveObjectSize);
            snapshotSpace->AddRegion(region);
        }
    }
}

void SnapshotProcessor::DeserializeString(uintptr_t stringBegin, uintptr_t stringEnd)
{
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    ASSERT(stringVector_.empty());
    auto oldSpace = const_cast<Heap *>(vm_->GetHeap())->GetOldSpace();
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    auto stringClass = globalConst->GetStringClass();
    while (stringBegin < stringEnd) {
        EcmaString *str = reinterpret_cast<EcmaString *>(stringBegin);
        size_t strSize = str->ObjectSize();
        strSize = AlignUp(strSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        auto strFromTable = stringTable->GetString(str);

        if (strFromTable) {
            stringVector_.emplace_back(ToUintPtr(strFromTable));
        } else {
            uintptr_t newObj = oldSpace->Allocate(strSize);
            if (newObj == 0) {
                LOG_ECMA_MEM(FATAL) << "Snapshot Allocate OldLocalSpace OOM";
            }
            if (memcpy_s(ToVoidPtr(newObj), strSize, str, strSize) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            str = reinterpret_cast<EcmaString *>(newObj);
            str->SetClass(reinterpret_cast<JSHClass *>(stringClass.GetTaggedObject()));
            str->ClearInternStringFlag();
            stringTable->GetOrInternString(str);
            stringVector_.emplace_back(newObj);
        }
        stringBegin += strSize;
    }
}

void SnapshotProcessor::DeserializePandaMethod(uintptr_t begin, uintptr_t end, JSMethod *methods,
                                               size_t &methodNums, size_t &others)
{
    for (size_t i = 0; i < others; i++) {
        pandaMethod_.emplace_back(begin);
        auto method = reinterpret_cast<JSMethod *>(begin);
        if (memcpy_s(methods + (--methodNums), METHOD_SIZE, method, METHOD_SIZE) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
        begin += METHOD_SIZE;
        if (begin >= end) {
            others = others - i - 1;
        }
    }
}

void SnapshotProcessor::HandleRootObject(SnapshotType type, uintptr_t rootObjectAddr,
                                         size_t objType, size_t &constSpecialIndex)
{
    switch (type) {
        case SnapshotType::VM_ROOT:
            if (JSType(objType) == JSType::GLOBAL_ENV) {
                vm_->SetGlobalEnv(reinterpret_cast<GlobalEnv *>(rootObjectAddr));
            } else if (JSType(objType) == JSType::MICRO_JOB_QUEUE) {
                vm_->SetMicroJobQueue(reinterpret_cast<job::MicroJobQueue *>(rootObjectAddr));
            }
            break;
        case SnapshotType::BUILTINS: {
            JSTaggedValue result(rootObjectAddr);
            auto constants = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
            size_t constCount = constants->GetConstantCount();
            while (constants->IsSpecialOrUndefined(constSpecialIndex)) {
                constSpecialIndex++; // Skip special or undefined value
            }
            if (constSpecialIndex < constCount) {
                constants->SetConstant(ConstantIndex(constSpecialIndex), result);
            } else {
                vm_->SetGlobalEnv(reinterpret_cast<GlobalEnv *>(rootObjectAddr));
            }
            constSpecialIndex++;
            break;
        }
        default:
            break;
    }
}

void SnapshotProcessor::SerializeObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                        std::unordered_map<uint64_t, ObjectEncode> *data)
{
    auto hclass = objectHeader->GetClass();
    JSType objectType = hclass->GetObjectType();
    uintptr_t snapshotObj = 0;
    if (UNLIKELY(data->find(ToUintPtr(objectHeader)) == data->end())) {
        LOG_ECMA(FATAL) << "Data map can not find object";
        UNREACHABLE();
    } else {
        snapshotObj = data->find(ToUintPtr(objectHeader))->second.first;
    }

    // header
    EncodeBit encodeBit = SerializeObjectHeader(objectHeader, static_cast<size_t>(objectType), queue, data);
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());

    auto visitor = [this, snapshotObj, queue, data](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                    bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            if (isNative) {
                auto nativePointer = *reinterpret_cast<void **>(slot.SlotAddress());
                SetObjectEncodeField(snapshotObj, slot.SlotAddress() - ToUintPtr(root),
                                     NativePointerToEncodeBit(nativePointer).GetValue());
            } else {
                auto fieldAddr = reinterpret_cast<JSTaggedType *>(slot.SlotAddress());
                SetObjectEncodeField(snapshotObj, slot.SlotAddress() - ToUintPtr(root),
                                     SerializeTaggedField(fieldAddr, queue, data));
            }
        }
    };

    objXRay_.VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

void SnapshotProcessor::Relocate(SnapshotType type, const JSPandaFile *jsPandaFile, uint64_t rootObjSize)
{
    size_t methodNums = 0;
    JSMethod *methods = nullptr;
    if (jsPandaFile) {
        methodNums = jsPandaFile->GetNumMethods();
        methods = jsPandaFile->GetMethods();
    }

    auto heap = vm_->GetHeap();
    auto oldSpace = heap->GetOldSpace();
    auto nonMovableSpace = heap->GetNonMovableSpace();
    auto machineCodeSpace = heap->GetMachineCodeSpace();
    auto snapshotSpace = heap->GetSnapshotSpace();

    RelocateSpaceObject(oldSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(nonMovableSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(machineCodeSpace, type, methods, methodNums, rootObjSize);
    RelocateSpaceObject(snapshotSpace, type, methods, methodNums, rootObjSize);
}

void SnapshotProcessor::RelocateSpaceObject(Space* space, SnapshotType type, JSMethod* methods,
                                            size_t methodNums, size_t rootObjSize)
{
    size_t others = 0;
    size_t objIndex = 0;
    size_t constSpecialIndex = 0;
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    space->EnumerateRegions([stringTable, &others, &objIndex, &rootObjSize, &constSpecialIndex,
                            &type, this, methods, &methodNums](Region *current) {
        if (!current->NeedRelocate()) {
            return;
        }
        current->ClearGCFlag(RegionGCFlags::NEED_RELOCATE);
        size_t allocated = current->GetAllocatedBytes();
        uintptr_t begin = current->GetBegin();
        uintptr_t end = begin + allocated;
        while (begin < end) {
            if (others != 0) {
                DeserializePandaMethod(begin, end, methods, methodNums, others);
                break;
            }
            EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(begin));
            auto objType = encodeBit.GetObjectType();
            if (objType == Constants::MASK_METHOD_SPACE_BEGIN) {
                begin += sizeof(uint64_t);
                others = encodeBit.GetNativeOrGlobalIndex();
                DeserializePandaMethod(begin, end, methods, methodNums, others);
                break;
            }
            TaggedObject *objectHeader = reinterpret_cast<TaggedObject *>(begin);
            DeserializeClassWord(objectHeader);
            DeserializeField(objectHeader);
            if (builtinsDeserialize_ && JSType(objType) == JSType::STRING) {
                auto str = reinterpret_cast<EcmaString *>(begin);
                str->ClearInternStringFlag();
                stringTable->InsertStringIfNotExist(str);
            }
            if (objIndex < rootObjSize) {
                HandleRootObject(type, begin, objType, constSpecialIndex);
            }
            begin = begin + AlignUp(objectHeader->GetClass()->SizeFromJSHClass(objectHeader),
                                    static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            objIndex++;
        }
    });
}

EncodeBit SnapshotProcessor::SerializeObjectHeader(TaggedObject *objectHeader, size_t objectType,
                                                   CQueue<TaggedObject *> *queue,
                                                   std::unordered_map<uint64_t, ObjectEncode> *data)
{
    auto hclass = objectHeader->GetClass();
    ASSERT(hclass != nullptr);
    EncodeBit encodeBit(0);
    if (data->find(ToUintPtr(hclass)) == data->end()) {
        encodeBit = EncodeTaggedObject(hclass, queue, data);
    } else {
        ObjectEncode objectEncodePair = data->find(ToUintPtr(hclass))->second;
        encodeBit = objectEncodePair.second;
    }
    encodeBit.SetObjectType(objectType);
    return encodeBit;
}

uint64_t SnapshotProcessor::SerializeTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                                                 std::unordered_map<uint64_t, ObjectEncode> *data)
{
    JSTaggedValue taggedValue(*tagged);
    if (taggedValue.IsWeak()) {
        EncodeBit special(JSTaggedValue::Undefined().GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();
    }

    if (taggedValue.IsSpecial()) {
        EncodeBit special(taggedValue.GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();  // special encode bit
    }

    if (!taggedValue.IsHeapObject()) {
        return taggedValue.GetRawData();  // not object
    }

    EncodeBit encodeBit(0);
    if (data->find(*tagged) == data->end()) {
        encodeBit = EncodeTaggedObject(taggedValue.GetTaggedObject(), queue, data);
    } else {
        ObjectEncode objectEncodePair = data->find(taggedValue.GetRawData())->second;
        encodeBit = objectEncodePair.second;
    }

    if (taggedValue.IsString()) {
        encodeBit.SetReferenceToString(true);
    }
    return encodeBit.GetValue();  // object
}

void SnapshotProcessor::DeserializeTaggedField(uint64_t *value)
{
    EncodeBit encodeBit(*value);
    if (!builtinsDeserialize_ && encodeBit.IsGlobalEnvConst()) {
        size_t index = encodeBit.GetNativeOrGlobalIndex();
        auto globalEnv = vm_->GetGlobalEnv();
        auto globalEnvObjectValue = globalEnv->GetGlobalEnvObjectByIndex(index);
        *value = ToUintPtr(globalEnvObjectValue->GetTaggedObject());
        return;
    }
    if (encodeBit.IsReference() && !encodeBit.IsSpecial()) {
        uintptr_t taggedObjectAddr = TaggedObjectEncodeBitToAddr(encodeBit);
        *value = taggedObjectAddr;
        return;
    }

    if (encodeBit.IsSpecial()) {
        encodeBit.ClearObjectSpecialFlag();
        *value = encodeBit.GetValue();
    }
}

void SnapshotProcessor::DeserializeClassWord(TaggedObject *object)
{
    EncodeBit encodeBit(*reinterpret_cast<uint64_t *>(object));
    if (!builtinsDeserialize_ && encodeBit.IsGlobalEnvConst()) {
        size_t hclassIndex = encodeBit.GetNativeOrGlobalIndex();
        auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
        JSTaggedValue hclassValue = globalConst->GetGlobalConstantObject(hclassIndex);
        ASSERT(hclassValue.IsJSHClass());
        object->SetClass(JSHClass::Cast(hclassValue.GetTaggedObject()));
        return;
    }
    uintptr_t hclassAddr = TaggedObjectEncodeBitToAddr(encodeBit);
    object->SetClass(reinterpret_cast<JSHClass *>(hclassAddr));
}

void SnapshotProcessor::DeserializeField(TaggedObject *objectHeader)
{
    auto visitor = [this]([[maybe_unused]] TaggedObject *root, ObjectSlot start, ObjectSlot end, bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto encodeBitAddr = reinterpret_cast<uint64_t *>(slot.SlotAddress());
            if (isNative) {
                DeserializeNativePointer(encodeBitAddr);
            } else {
                DeserializeTaggedField(encodeBitAddr);
            }
        }
    };

    objXRay_.VisitObjectBody<VisitType::SNAPSHOT_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

EncodeBit SnapshotProcessor::NativePointerToEncodeBit(void *nativePointer)
{
    EncodeBit native(0);
    if (nativePointer != nullptr) {  // nativePointer
        size_t index = Constants::MAX_C_POINTER_INDEX;

        if (programSerialize_) {
            pandaMethod_.emplace_back(ToUintPtr(nativePointer));
            ASSERT(pandaMethod_.size() + GetNativeTableSize() <= Constants::MAX_UINT_16);
            // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
            index = pandaMethod_.size() + GetNativeTableSize() - 1;
        } else {
            index = SearchNativeMethodIndex(nativePointer);
        }

        LOG_IF(index > Constants::MAX_C_POINTER_INDEX, FATAL, RUNTIME) << "MAX_C_POINTER_INDEX: " + ToCString(index);
        native.SetNativeOrGlobalIndex(index);
    }
    return native;
}

void *SnapshotProcessor::NativePointerEncodeBitToAddr(EncodeBit nativeBit)
{
    size_t index = nativeBit.GetNativeOrGlobalIndex();
    void *addr = nullptr;
    size_t nativeTableSize = GetNativeTableSize();

    if (index < nativeTableSize - Constants::PROGRAM_NATIVE_METHOD_BEGIN) {
        addr = reinterpret_cast<void *>(vm_->GetFactory()->nativeMethods_.at(index));
    } else if (index < nativeTableSize) {
        addr = reinterpret_cast<void *>(g_nativeTable[index]);
    } else {
        addr = ToVoidPtr(pandaMethod_.at(index - nativeTableSize));
    }
    return addr;
}

size_t SnapshotProcessor::SearchNativeMethodIndex(void *nativePointer)
{
    size_t nativeMethodSize = GetNativeTableSize() - Constants::PROGRAM_NATIVE_METHOD_BEGIN;
    for (size_t i = 0; i < Constants::PROGRAM_NATIVE_METHOD_BEGIN; i++) {
        if (nativePointer == reinterpret_cast<void *>(g_nativeTable[i + nativeMethodSize])) {
            return i + nativeMethodSize;
        }
    }

    // not found
    auto nativeMethod = reinterpret_cast<JSMethod *>(nativePointer)->GetNativePointer();
    for (size_t i = 0; i < nativeMethodSize; i++) {
        if (nativeMethod == reinterpret_cast<void *>(g_nativeTable[i])) {
            return i;
        }
    }

    LOG_ECMA(FATAL) << "native method did not register in g_table, please register it first";
    UNREACHABLE();
}

uintptr_t SnapshotProcessor::TaggedObjectEncodeBitToAddr(EncodeBit taggedBit)
{
    ASSERT(taggedBit.IsReference());
    if (!builtinsDeserialize_ && taggedBit.IsReferenceToString()) {
        size_t stringIndex = taggedBit.GetStringIndex();
        return stringVector_[stringIndex];
    }
    size_t regionIndex = taggedBit.GetRegionIndex();
    if (UNLIKELY(regionIndexMap_.find(regionIndex) == regionIndexMap_.end())) {
        LOG_ECMA(FATAL) << "Snapshot deserialize can not find region by index";
    }
    Region *region = regionIndexMap_.find(regionIndex)->second;
    size_t objectOffset  = taggedBit.GetObjectOffsetInRegion();
    return ToUintPtr(region) + objectOffset;
}

void SnapshotProcessor::DeserializeNativePointer(uint64_t *value)
{
    EncodeBit native(*value);
    size_t index = native.GetNativeOrGlobalIndex();
    uintptr_t addr = 0U;
    size_t nativeTableSize = GetNativeTableSize();

    if (index < nativeTableSize - Constants::PROGRAM_NATIVE_METHOD_BEGIN) {
        addr = reinterpret_cast<uintptr_t>(vm_->GetFactory()->nativeMethods_.at(index));
    } else if (index < nativeTableSize) {
        addr = g_nativeTable[index];
    } else {
        addr = pandaMethod_.at(index - nativeTableSize);
    }
    *value = addr;
}

void SnapshotProcessor::SerializePandaFileMethod()
{
    EncodeBit encodeBit(0);
    encodeBit.SetObjectType(Constants::MASK_METHOD_SPACE_BEGIN);
    encodeBit.SetNativeOrGlobalIndex(pandaMethod_.size());

    ObjectFactory *factory = vm_->GetFactory();
    // panda method space begin
    uintptr_t snapshotObj = factory->NewSpaceBySnapshotAllocator(sizeof(uint64_t));
    if (snapshotObj == 0) {
        LOG(ERROR, RUNTIME) << "SnapshotAllocator OOM";
        return;
    }
    SetObjectEncodeField(snapshotObj, 0, encodeBit.GetValue());  // methods

    // panda methods
    for (auto &it : pandaMethod_) {
        // write method
        size_t methodObjSize = METHOD_SIZE;
        uintptr_t methodObj = factory->NewSpaceBySnapshotAllocator(methodObjSize);
        if (methodObj == 0) {
            LOG(ERROR, RUNTIME) << "SnapshotAllocator OOM";
            return;
        }
        if (memcpy_s(ToVoidPtr(methodObj), methodObjSize, ToVoidPtr(it), METHOD_SIZE) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

EncodeBit SnapshotProcessor::EncodeTaggedObject(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, ObjectEncode> *data)
{
    if (!builtinsSerialize_) {
        // String duplicate
        if (objectHeader->GetClass()->GetObjectType() == JSType::STRING) {
            ASSERT(stringVector_.size() < Constants::MAX_STRING_SIZE);
            EncodeBit encodeBit(stringVector_.size());
            stringVector_.emplace_back(ToUintPtr(objectHeader));
            data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
            return encodeBit;
        }

        // builtins object reuse
        size_t globalEnvIndex = vm_->GetSnapshotEnv()->GetEnvObjectIndex(ToUintPtr(objectHeader));
        if (globalEnvIndex != SnapshotEnv::MAX_UINT_32) {
            EncodeBit encodeBit(0);
            encodeBit.SetGlobalEnvConst();
            encodeBit.SetNativeOrGlobalIndex(globalEnvIndex);
            data->emplace(ToUintPtr(objectHeader), std::make_pair(0U, encodeBit));
            return encodeBit;
        }
    }
    queue->emplace(objectHeader);
    size_t objectSize = objectHeader->GetClass()->SizeFromJSHClass(objectHeader);
    if (objectSize > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        LOG_ECMA_MEM(FATAL) << "It is a huge object. Not Support.";
    }

    if (objectSize == 0) {
        LOG_ECMA_MEM(FATAL) << "It is a zero object. Not Support.";
    }
    uintptr_t newObj = 0;
    if (builtinsSerialize_) {
        newObj = AllocateObjectToLocalSpace(snapshotLocalSpace_, objectSize);
    } else {
        auto region = Region::ObjectAddressToRange(objectHeader);
        if (region->InYoungOrOldSpace()) {
            newObj = AllocateObjectToLocalSpace(oldLocalSpace_, objectSize);
        } else if (region->InMachineCodeSpace()) {
            newObj = AllocateObjectToLocalSpace(machineCodeLocalSpace_, objectSize);
        } else if (region->InNonMovableSpace()) {
            newObj = AllocateObjectToLocalSpace(nonMovableLocalSpace_, objectSize);
        } else {
            newObj = AllocateObjectToLocalSpace(snapshotLocalSpace_, objectSize);
        }
    }

    if (newObj == 0) {
        LOG_ECMA_MEM(FATAL) << "Snapshot Allocate OOM";
    }
    if (memcpy_s(ToVoidPtr(newObj), objectSize, objectHeader, objectSize) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    auto currentRegion = Region::ObjectAddressToRange(newObj);
    size_t regionIndex = *(currentRegion->GetMarkGCBitset()->Words());
    size_t objOffset = newObj - ToUintPtr(currentRegion);
    EncodeBit encodeBit(static_cast<uint64_t>(regionIndex));
    encodeBit.SetObjectOffsetInRegion(objOffset);
    data->emplace(ToUintPtr(objectHeader), std::make_pair(newObj, encodeBit));
    return encodeBit;
}

void SnapshotProcessor::EncodeTaggedObjectRange(ObjectSlot start, ObjectSlot end, CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, ObjectEncode> *data)
{
    while (start < end) {
        JSTaggedValue object(start.GetTaggedType());
        start++;
        if (object.IsHeapObject()) {
            EncodeBit encodeBit(0);
            if (data->find(object.GetRawData()) == data->end()) {
                encodeBit = EncodeTaggedObject(object.GetTaggedObject(), queue, data);
            }
        }
    }
}

void SnapshotProcessor::GeneratedNativeMethod()  // NOLINT(readability-function-size)
{
    size_t nativeMethodSize = GetNativeTableSize() - Constants::PROGRAM_NATIVE_METHOD_BEGIN;
    for (size_t i = 0; i < nativeMethodSize; i++) {
        vm_->GetFactory()->NewMethodForNativeFunction(reinterpret_cast<void *>(g_nativeTable[i]));
    }
}

size_t SnapshotProcessor::GetNativeTableSize() const
{
    return sizeof(g_nativeTable) / sizeof(g_nativeTable[0]);
}
}  // namespace panda::ecmascript
