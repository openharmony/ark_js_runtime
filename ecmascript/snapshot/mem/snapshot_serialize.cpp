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

#include "ecmascript/snapshot/mem/snapshot_serialize.h"

#include "ecmascript/base/error_type.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
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
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/containers/containers_treeset.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
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
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using Json = builtins::BuiltinsJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
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

constexpr int TAGGED_SIZE = JSTaggedValue::TaggedTypeSize();
constexpr int OBJECT_HEADER_SIZE = TaggedObject::TaggedObjectSize();
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
    reinterpret_cast<uintptr_t>(Object::Assign),
    reinterpret_cast<uintptr_t>(Object::Create),
    reinterpret_cast<uintptr_t>(Object::DefineProperties),
    reinterpret_cast<uintptr_t>(Object::DefineProperty),
    reinterpret_cast<uintptr_t>(Object::Freeze),
    reinterpret_cast<uintptr_t>(Object::GetOwnPropertyDesciptor),
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
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint8ClampedArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint16ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Int32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Uint32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float32ArrayConstructor),
    reinterpret_cast<uintptr_t>(BuiltinsTypedArray::Float64ArrayConstructor),
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
    reinterpret_cast<uintptr_t>(BuiltinsString::Normalize),
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
    reinterpret_cast<uintptr_t>(DataView::SetInt8),
    reinterpret_cast<uintptr_t>(DataView::SetInt16),
    reinterpret_cast<uintptr_t>(DataView::SetInt32),
    reinterpret_cast<uintptr_t>(DataView::SetUint8),
    reinterpret_cast<uintptr_t>(DataView::SetUint16),
    reinterpret_cast<uintptr_t>(DataView::SetUint32),
    reinterpret_cast<uintptr_t>(DataView::GetBuffer),
    reinterpret_cast<uintptr_t>(DataView::GetByteLength),
    reinterpret_cast<uintptr_t>(DataView::GetOffset),
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
    reinterpret_cast<uintptr_t>(Json::Parse),
    reinterpret_cast<uintptr_t>(Json::Stringify),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Next),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Return),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::Throw),
    reinterpret_cast<uintptr_t>(BuiltinsIterator::GetIteratorObj),
    reinterpret_cast<uintptr_t>(JSForInIterator::Next),
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

    // non ECMA standard jsapi containers.
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

    // not builtins method
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeSetter),
    reinterpret_cast<uintptr_t>(JSFunction::PrototypeGetter),
    reinterpret_cast<uintptr_t>(JSFunction::NameGetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthSetter),
    reinterpret_cast<uintptr_t>(JSArray::LengthGetter),
};

SnapShotSerialize::SnapShotSerialize(EcmaVM *vm, bool serialize) : vm_(vm), serialize_(serialize)
{
    objectArraySize_ = OBJECT_SIZE_EXTEND_PAGE;
    if (serialize_) {
        addressSlot_ = ToUintPtr(vm_->GetNativeAreaAllocator()->Allocate(sizeof(uintptr_t) * OBJECT_SIZE_EXTEND_PAGE));
    } else {
        addressSlot_ = ToUintPtr(vm_->GetNativeAreaAllocator()->Allocate(sizeof(uintptr_t) * objectArraySize_));
    }
}

SnapShotSerialize::~SnapShotSerialize()
{
    if (serialize_) {
        vm_->GetNativeAreaAllocator()->Free(ToVoidPtr(addressSlot_), sizeof(uintptr_t) * OBJECT_SIZE_EXTEND_PAGE);
    } else {
        vm_->GetNativeAreaAllocator()->Free(ToVoidPtr(addressSlot_), sizeof(uintptr_t) * objectArraySize_);
    }
}

void SnapShotSerialize::SetObjectSlotField(uintptr_t obj, size_t offset, uint64_t value)
{
    *reinterpret_cast<uint64_t *>(obj + offset) = value;
}

void SnapShotSerialize::SetObjectSlotFieldUint32(uintptr_t obj, size_t offset, uint32_t value)
{
    *reinterpret_cast<uint32_t *>(obj + offset) = value;
}

void SnapShotSerialize::Serialize(TaggedObject *objectHeader, CQueue<TaggedObject *> *queue,
                                  std::unordered_map<uint64_t, SlotBit> *data)
{
    uint8_t objectType = SerializeHelper::GetObjectType(objectHeader);
    size_t objectSize = objectHeader->GetClass()->SizeFromJSHClass(objectHeader);
    if (objectSize > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        LOG_ECMA_MEM(FATAL) << "It is a huge object. Not Support.";
    }

    if (objectSize == 0) {
        LOG_ECMA_MEM(FATAL) << "It is a zero object. Not Support.";
    }

    uintptr_t snapshotObj = vm_->GetFactory()->NewSpaceBySnapShotAllocator(objectSize);
    if (snapshotObj == 0) {
        LOG_ECMA(DEBUG) << "SnapShotAllocator OOM";
        return;
    }
    if (memcpy_s(ToVoidPtr(snapshotObj), objectSize, objectHeader, objectSize) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }

    // header
    SlotBit headSlot = HandleObjectHeader(objectHeader, objectType, objectSize, queue, data);
    SetObjectSlotField(snapshotObj, 0, headSlot.GetValue());

    count_++;
    LOG_IF(count_ > MAX_OBJECT_INDEX, FATAL, RUNTIME) << "objectCount: " + ToCString(count_);
    LOG_IF(objectSize > MAX_OBJECT_SIZE_INDEX, FATAL, RUNTIME) << "objectSize: " + ToCString(objectSize);
    switch (JSType(objectType)) {
        case JSType::HCLASS:
            DynClassSerialize(objectHeader, snapshotObj, objectSize, queue, data);
            break;
        case JSType::STRING:
            DynStringSerialize(objectHeader, snapshotObj);
            break;
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
            DynArraySerialize(objectHeader, snapshotObj, queue, data);
            break;
        case JSType::JS_NATIVE_POINTER:
            NativePointerSerialize(objectHeader, snapshotObj);
            break;
        case JSType::PROGRAM:
            DynProgramSerialize(objectHeader, snapshotObj, queue, data);
            break;
        case JSType::JS_FUNCTION_BASE:
        case JSType::JS_FUNCTION:
        case JSType::JS_PROXY_REVOC_FUNCTION:
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
        case JSType::JS_GENERATOR_FUNCTION:
        case JSType::JS_ASYNC_FUNCTION:
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
        case JSType::JS_BOUND_FUNCTION:
            JSFunctionBaseSerialize(objectHeader, snapshotObj, objectSize, queue, data);
            break;
        case JSType::JS_PROXY:
            JSProxySerialize(objectHeader, snapshotObj, objectSize, queue, data);
            break;
        default:
            JSObjectSerialize(objectHeader, snapshotObj, objectSize, queue, data);
            break;
    }
}

void SnapShotSerialize::ExtendObjectArray()
{
    int countNow = objectArraySize_;
    objectArraySize_ = objectArraySize_ + OBJECT_SIZE_EXTEND_PAGE;

    auto addr = vm_->GetNativeAreaAllocator()->Allocate(sizeof(uintptr_t) * objectArraySize_);
    int size = countNow * ADDRESS_SIZE;
    if (memcpy_s(addr, size, ToVoidPtr(addressSlot_), size) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }

    vm_->GetNativeAreaAllocator()->Free(ToVoidPtr(addressSlot_), sizeof(uintptr_t) * objectArraySize_);
    addressSlot_ = ToUintPtr(addr);
}

void SnapShotSerialize::RedirectSlot(const panda_file::File *pf)
{
    SnapShotSpace *space = vm_->GetHeap()->GetSnapShotSpace();
    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();

    size_t others = 0;
    space->EnumerateRegions([stringTable, &others, this, pf](Region *current) {
        size_t allocated = current->GetAllocatedBytes();
        uintptr_t begin = current->GetBegin();
        uintptr_t end = begin + allocated;
        while (begin < end) {
            if (others != 0) {
                for (size_t i = 0; i < others; i++) {
                    pandaMethod_.emplace_back(begin);
                    auto method = reinterpret_cast<JSMethod *>(begin);
                    method->SetPandaFile(pf);
                    method->SetBytecodeArray(method->GetInstructions());
                    vm_->frameworkProgramMethods_.emplace_back(method);
                    begin += METHOD_SIZE;
                    if (begin >= end) {
                        others = others - i - 1;
                    }
                }
                break;
            }

            if (count_ == objectArraySize_) {
                ExtendObjectArray();
            }
            SlotBit slot(*reinterpret_cast<uint64_t *>(begin));
            if (slot.GetObjectType() == MASK_METHOD_SPACE_BEGIN) {
                begin += sizeof(uint64_t);
                for (size_t i = 0; i < slot.GetObjectSize(); i++) {
                    pandaMethod_.emplace_back(begin);
                    auto method = reinterpret_cast<JSMethod *>(begin);
                    method->SetPandaFile(pf);
                    method->SetBytecodeArray(method->GetInstructions());
                    vm_->frameworkProgramMethods_.emplace_back(method);
                    begin += METHOD_SIZE;
                    if (begin >= end) {
                        others = slot.GetObjectSize() - i - 1;
                        break;
                    }
                }
                break;
            }

            if (JSType(slot.GetObjectType()) == JSType::STRING) {
                stringTable->InsertStringIfNotExist(reinterpret_cast<EcmaString *>(begin));
            }

            SetAddressToSlot(count_, begin);
            begin = begin + AlignUp(slot.GetObjectSize(), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            count_++;
        }
    });

    auto constants = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    for (int i = 0; i < count_; i++) {
        SlotBit slot(*GetAddress<uint64_t *>(i));
        size_t objectSize = slot.GetObjectSize();
        uint8_t objectType = slot.GetObjectType();
        size_t index = slot.GetObjectInConstantsIndex();

        switch (JSType(objectType)) {
            case JSType::HCLASS:
                DynClassDeserialize(GetAddress<uint64_t *>(i));
                break;
            case JSType::STRING:
                DynStringDeserialize(GetAddress<uint64_t *>(i));
                break;
            case JSType::TAGGED_ARRAY:
            case JSType::TAGGED_DICTIONARY:
                DynArrayDeserialize(GetAddress<uint64_t *>(i));
                break;
            case JSType::JS_NATIVE_POINTER:
                NativePointerDeserialize(GetAddress<uint64_t *>(i));
                break;
            case JSType::GLOBAL_ENV:
                JSObjectDeserialize(GetAddress<uint64_t *>(i), objectSize);
                vm_->SetGlobalEnv(GetAddress<GlobalEnv *>(i));
                break;
            case JSType::MICRO_JOB_QUEUE:
                JSObjectDeserialize(GetAddress<uint64_t *>(i), objectSize);
                vm_->SetMicroJobQueue(GetAddress<job::MicroJobQueue *>(i));
                break;
            case JSType::PROGRAM:
                DynProgramDeserialize(GetAddress<uint64_t *>(i), objectSize);
                vm_->frameworkProgram_ = JSTaggedValue(GetAddress<Program *>(i));
                break;
            case JSType::JS_FUNCTION_BASE:
            case JSType::JS_FUNCTION:
            case JSType::JS_PROXY_REVOC_FUNCTION:
            case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            case JSType::JS_GENERATOR_FUNCTION:
            case JSType::JS_ASYNC_FUNCTION:
            case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            case JSType::JS_BOUND_FUNCTION:
                JSFunctionBaseDeserialize(GetAddress<uint64_t *>(i), objectSize);
                break;
            case JSType::JS_PROXY:
                JSProxyDeserialize(GetAddress<uint64_t *>(i), objectSize);
                break;
            default:
                JSObjectDeserialize(GetAddress<uint64_t *>(i), objectSize);
                break;
        }
        if (index != 0) {
            JSTaggedValue result(GetAddress<TaggedObject *>(i));
            constants->SetConstant(ConstantIndex(index - 1), result);
        }
    }
}

SlotBit SnapShotSerialize::HandleObjectHeader(TaggedObject *objectHeader, uint8_t objectType, size_t objectSize,
                                              CQueue<TaggedObject *> *queue,
                                              std::unordered_map<uint64_t, SlotBit> *data)
{
    auto *hclassClass = objectHeader->GetClass();
    SlotBit slot(0);

    ASSERT(hclassClass != nullptr);
    if (data->find(ToUintPtr(hclassClass)) == data->end()) {
        slot = SerializeHelper::AddObjectHeaderToData(hclassClass, queue, data);
    } else {
        slot = data->find(ToUintPtr(hclassClass))->second;
    }

    SlotBit objectSlotBit = data->find(ToUintPtr(objectHeader))->second;
    slot.SetObjectInConstantsIndex(objectSlotBit.GetObjectInConstantsIndex());
    slot.SetObjectSize(objectSize);
    slot.SetObjectType(objectType);
    return slot;
}

uint64_t SnapShotSerialize::HandleTaggedField(JSTaggedType *tagged, CQueue<TaggedObject *> *queue,
                                              std::unordered_map<uint64_t, SlotBit> *data)
{
    JSTaggedValue taggedValue(*tagged);
    if (taggedValue.IsWeak()) {
        return JSTaggedValue::Undefined().GetRawData();  // Undefind
    }

    if (taggedValue.IsSpecial()) {
        SlotBit special(taggedValue.GetRawData());
        special.SetObjectSpecial();
        return special.GetValue();  // special slot
    }

    if (!taggedValue.IsHeapObject()) {
        return taggedValue.GetRawData();  // not object
    }

    SlotBit slotBit(0);
    if (data->find(*tagged) == data->end()) {
        slotBit = SerializeHelper::AddObjectHeaderToData(taggedValue.GetTaggedObject(), queue, data);
    } else {
        slotBit = data->find(taggedValue.GetRawData())->second;
    }

    if (taggedValue.IsString()) {
        slotBit.SetReferenceToString(true);
    }
    return slotBit.GetValue();  // object
}

void SnapShotSerialize::DeserializeHandleTaggedField(uint64_t *value)
{
    SlotBit slot(*value);
    if (slot.IsReferenceSlot() && !slot.IsSpecial()) {
        uint32_t index = slot.GetObjectIndex();

        if (slot.IsReferenceToString()) {
            auto str = vm_->GetEcmaStringTable()->GetString(GetAddress<EcmaString *>(index));
            ASSERT(str != nullptr);
            *value = ToUintPtr(str);
        } else {
            *value = GetAddress<uintptr_t>(index);
        }
        return;
    }

    if (slot.IsSpecial()) {
        slot.ClearObjectSpecialFlag();
        *value = slot.GetValue();
    }
}

void SnapShotSerialize::DeserializeHandleClassWord(TaggedObject *object)
{
    SlotBit slot(*reinterpret_cast<uint64_t *>(object));
    ASSERT(slot.IsReferenceSlot());
    uint32_t index = slot.GetObjectIndex();
    *reinterpret_cast<uint64_t *>(object) = 0;

    object->SetClass(GetAddress<JSHClass *>(index));
}

void SnapShotSerialize::DynClassSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                                          CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, SlotBit> *data)
{
    size_t beginOffset = JSHClass::PROTOTYPE_OFFSET;
    int numOfFields = static_cast<int>((objectSize - beginOffset) / TAGGED_SIZE);
    uintptr_t startAddr = ToUintPtr(objectHeader) + beginOffset;
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(startAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, beginOffset + i * TAGGED_SIZE, HandleTaggedField(fieldAddr, queue, data));
    }
}

void SnapShotSerialize::DynClassDeserialize(uint64_t *objectHeader)
{
    auto dynClass = reinterpret_cast<JSHClass *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(dynClass);

    uintptr_t startAddr = ToUintPtr(dynClass) + JSHClass::PROTOTYPE_OFFSET;
    int numOfFields = static_cast<int>((JSHClass::SIZE - JSHClass::PROTOTYPE_OFFSET) / TAGGED_SIZE);
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<uint64_t *>(startAddr + i * TAGGED_SIZE);
        DeserializeHandleTaggedField(fieldAddr);
    }
}

void SnapShotSerialize::DynStringSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj)
{
    auto *str = EcmaString::Cast(objectHeader);
    SetObjectSlotFieldUint32(snapshotObj, OBJECT_HEADER_SIZE, str->GetLength() << 2U);
}

void SnapShotSerialize::DynStringDeserialize(uint64_t *objectHeader)
{
    auto object = reinterpret_cast<EcmaString *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(object);
}

void SnapShotSerialize::DynArraySerialize(TaggedObject *objectHeader, uintptr_t snapshotObj,
                                          CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, SlotBit> *data)
{
    auto arrayObject = reinterpret_cast<TaggedArray *>(objectHeader);
    size_t beginOffset = TaggedArray::DATA_OFFSET;
    auto arrayLength = arrayObject->GetLength();
    uintptr_t startAddr = ToUintPtr(objectHeader) + beginOffset;
    for (uint32_t i = 0; i < arrayLength; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(startAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, beginOffset + i * TAGGED_SIZE, HandleTaggedField(fieldAddr, queue, data));
    }
}

void SnapShotSerialize::DynArrayDeserialize(uint64_t *objectHeader)
{
    auto object = reinterpret_cast<TaggedArray *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(object);

    auto arrayLength = object->GetLength();
    size_t dataOffset = TaggedArray::DATA_OFFSET;
    uintptr_t startAddr = ToUintPtr(objectHeader) + dataOffset;
    for (uint32_t i = 0; i < arrayLength; i++) {
        auto fieldAddr = reinterpret_cast<uint64_t *>(startAddr + i * TAGGED_SIZE);
        DeserializeHandleTaggedField(fieldAddr);
    }
}

SlotBit SnapShotSerialize::NativePointerToSlotBit(void *nativePointer)
{
    SlotBit native(0);
    if (nativePointer != nullptr) {  // nativePointer
        uint32_t index = MAX_UINT_32;

        if (programSerialize_) {
            pandaMethod_.emplace_back(ToUintPtr(nativePointer));
            // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
            index = pandaMethod_.size() + PROGRAM_NATIVE_METHOD_BEGIN + NATIVE_METHOD_SIZE - 1;
        } else {
            for (size_t i = 0; i < PROGRAM_NATIVE_METHOD_BEGIN; i++) {
                if (nativePointer == reinterpret_cast<void *>(g_nativeTable[i + NATIVE_METHOD_SIZE])) {
                    index = i + NATIVE_METHOD_SIZE;
                    break;
                }
            }

            // not found
            if (index == MAX_UINT_32) {
                auto nativeMethod = reinterpret_cast<JSMethod *>(nativePointer)->GetNativePointer();
                for (size_t i = 0; i < NATIVE_METHOD_SIZE; i++) {
                    if (nativeMethod == GetAddress<void *>(i)) {
                        index = i;
                        break;
                    }
                }
            }
        }

        ASSERT(index != MAX_UINT_32);
        LOG_IF(index > MAX_C_POINTER_INDEX, FATAL, RUNTIME) << "MAX_C_POINTER_INDEX: " + ToCString(index);
        native.SetObjectIndex(index);
    }
    return native;
}

void *SnapShotSerialize::NativePointerSlotBitToAddr(SlotBit native)
{
    uint32_t index = native.GetObjectIndex();
    void *addr = nullptr;

    if (index < NATIVE_METHOD_SIZE) {
        addr = reinterpret_cast<void *>(vm_->nativeMethods_.at(index));
    } else if (index < NATIVE_METHOD_SIZE + PROGRAM_NATIVE_METHOD_BEGIN) {
        addr = reinterpret_cast<void *>(g_nativeTable[index]);
    } else {
        addr = ToVoidPtr(pandaMethod_.at(index - PROGRAM_NATIVE_METHOD_BEGIN - NATIVE_METHOD_SIZE));
    }
    return addr;
}

void SnapShotSerialize::NativePointerSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj)
{
    void *nativePointer = JSNativePointer::Cast(objectHeader)->GetExternalPointer();
    SetObjectSlotField(snapshotObj, OBJECT_HEADER_SIZE, NativePointerToSlotBit(nativePointer).GetValue());
}

void SnapShotSerialize::NativePointerDeserialize(uint64_t *objectHeader)
{
    auto object = reinterpret_cast<TaggedObject *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(object);

    size_t nativeAddr = ToUintPtr(object) + OBJECT_HEADER_SIZE;
    SlotBit native(*reinterpret_cast<uint64_t *>(nativeAddr));
    if (native.GetObjectIndex() == MAX_OBJECT_INDEX) {
        return;
    }
    JSNativePointer::Cast(object)->ResetExternalPointer(NativePointerSlotBitToAddr(native));
}

void SnapShotSerialize::JSObjectSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                                          CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, SlotBit> *data)
{
    int numOfFields = static_cast<int>((objectSize - OBJECT_HEADER_SIZE) / TAGGED_SIZE);
    uintptr_t startAddr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(startAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, OBJECT_HEADER_SIZE + i * TAGGED_SIZE,
                           HandleTaggedField(fieldAddr, queue, data));
    }
}

void SnapShotSerialize::JSFunctionBaseSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                                                CQueue<TaggedObject *> *queue,
                                                std::unordered_map<uint64_t, SlotBit> *data)
{
    // befour
    int befourFields = static_cast<int>((JSFunctionBase::METHOD_OFFSET - OBJECT_HEADER_SIZE) / TAGGED_SIZE);
    uintptr_t befourStartAddr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    for (int i = 0; i < befourFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(befourStartAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, OBJECT_HEADER_SIZE + i * TAGGED_SIZE,
                           HandleTaggedField(fieldAddr, queue, data));
    }

    // method
    auto functionBase = static_cast<JSFunctionBase *>(objectHeader);
    size_t methodOffset = JSFunctionBase::METHOD_OFFSET;
    auto nativePointer = reinterpret_cast<void *>(functionBase->GetMethod());
    SetObjectSlotField(snapshotObj, methodOffset, NativePointerToSlotBit(nativePointer).GetValue());

    // after
    size_t afterOffset = JSFunctionBase::METHOD_OFFSET + TAGGED_SIZE;
    int afterFields = static_cast<int>((objectSize - afterOffset) / TAGGED_SIZE);
    uintptr_t afterStartAddr = ToUintPtr(objectHeader) + JSFunctionBase::METHOD_OFFSET + TAGGED_SIZE;
    for (int i = 0; i < afterFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(afterStartAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, afterOffset + i * TAGGED_SIZE, HandleTaggedField(fieldAddr, queue, data));
    }
}

void SnapShotSerialize::JSProxySerialize(TaggedObject *objectHeader, uintptr_t snapshotObj, size_t objectSize,
                                         CQueue<TaggedObject *> *queue, std::unordered_map<uint64_t, SlotBit> *data)
{
    // befour
    int befourFields = static_cast<int>((JSProxy::METHOD_OFFSET - OBJECT_HEADER_SIZE) / TAGGED_SIZE);
    uintptr_t befourStartAddr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    for (int i = 0; i < befourFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(befourStartAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, OBJECT_HEADER_SIZE + i * TAGGED_SIZE,
                           HandleTaggedField(fieldAddr, queue, data));
    }

    // method
    auto jsproxy = static_cast<JSProxy *>(objectHeader);
    size_t methodOffset = JSProxy::METHOD_OFFSET;
    auto nativePointer = reinterpret_cast<void *>(jsproxy->GetMethod());
    SetObjectSlotField(snapshotObj, methodOffset, NativePointerToSlotBit(nativePointer).GetValue());

    // after
    size_t afterOffset = JSProxy::METHOD_OFFSET + TAGGED_SIZE;
    int afterFields = static_cast<int>((objectSize - afterOffset) / TAGGED_SIZE);
    uintptr_t afterStartAddr = ToUintPtr(objectHeader) + JSProxy::METHOD_OFFSET + TAGGED_SIZE;
    for (int i = 0; i < afterFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(afterStartAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, afterOffset + i * TAGGED_SIZE, HandleTaggedField(fieldAddr, queue, data));
    }
}

void SnapShotSerialize::DeserializeRangeTaggedField(size_t beginAddr, int numOfFields)
{
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<uint64_t *>(beginAddr + i * TAGGED_SIZE);
        DeserializeHandleTaggedField(fieldAddr);
    }
}

void SnapShotSerialize::JSObjectDeserialize(uint64_t *objectHeader, size_t objectSize)
{
    auto object = reinterpret_cast<TaggedObject *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(object);

    auto objBodySize = objectSize - OBJECT_HEADER_SIZE;
    ASSERT(objBodySize % TAGGED_SIZE == 0);
    int numOfFields = static_cast<int>(objBodySize / TAGGED_SIZE);
    size_t addr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    DeserializeRangeTaggedField(addr, numOfFields);
}

void SnapShotSerialize::JSFunctionBaseDeserialize(uint64_t *objectHeader, size_t objectSize)
{
    auto object = reinterpret_cast<JSFunctionBase *>(objectHeader);
    DeserializeHandleClassWord(object);

    // befour
    auto befourMethod = JSFunctionBase::METHOD_OFFSET - OBJECT_HEADER_SIZE;
    ASSERT(befourMethod % TAGGED_SIZE == 0);
    int befourMethodFields = static_cast<int>(befourMethod / TAGGED_SIZE);
    size_t befourAddr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    DeserializeRangeTaggedField(befourAddr, befourMethodFields);

    // method
    size_t nativeAddr = ToUintPtr(object) + JSFunctionBase::METHOD_OFFSET;
    SlotBit native(*reinterpret_cast<uint64_t *>(nativeAddr));
    if (native.GetObjectIndex() != MAX_OBJECT_INDEX) {
        object->SetMethod(reinterpret_cast<JSMethod *>(NativePointerSlotBitToAddr(native)));
    }

    // after
    auto afterMethod = objectSize - JSFunctionBase::METHOD_OFFSET - TAGGED_SIZE;
    ASSERT(afterMethod % TAGGED_SIZE == 0);
    int afterMethodFields = static_cast<int>(afterMethod / TAGGED_SIZE);
    size_t afterAddr = ToUintPtr(objectHeader) + JSFunctionBase::METHOD_OFFSET + TAGGED_SIZE;
    DeserializeRangeTaggedField(afterAddr, afterMethodFields);
}

void SnapShotSerialize::JSProxyDeserialize(uint64_t *objectHeader, size_t objectSize)
{
    auto object = reinterpret_cast<JSProxy *>(objectHeader);
    DeserializeHandleClassWord(object);

    // befour
    auto befourMethod = JSProxy::METHOD_OFFSET - OBJECT_HEADER_SIZE;
    ASSERT(befourMethod % TAGGED_SIZE == 0);
    int befourMethodFields = static_cast<int>(befourMethod / TAGGED_SIZE);
    size_t befourAddr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    DeserializeRangeTaggedField(befourAddr, befourMethodFields);

    // method
    size_t nativeAddr = ToUintPtr(object) + JSProxy::METHOD_OFFSET;
    SlotBit native(*reinterpret_cast<uint64_t *>(nativeAddr));
    if (native.GetObjectIndex() != MAX_OBJECT_INDEX) {
        object->SetMethod(reinterpret_cast<JSMethod *>(NativePointerSlotBitToAddr(native)));
    }

    // after
    auto afterMethod = objectSize - JSProxy::METHOD_OFFSET - TAGGED_SIZE;
    ASSERT(afterMethod % TAGGED_SIZE == 0);
    int afterMethodFields = static_cast<int>(afterMethod / TAGGED_SIZE);
    size_t afterAddr = ToUintPtr(objectHeader) + JSProxy::METHOD_OFFSET + TAGGED_SIZE;
    DeserializeRangeTaggedField(afterAddr, afterMethodFields);
}

void SnapShotSerialize::DynProgramSerialize(TaggedObject *objectHeader, uintptr_t snapshotObj,
                                            CQueue<TaggedObject *> *queue,
                                            std::unordered_map<uint64_t, ecmascript::SlotBit> *data)
{
    size_t beginOffset = OBJECT_HEADER_SIZE;
    auto objBodySize = Program::METHODS_DATA_OFFSET - OBJECT_HEADER_SIZE;
    int numOfFields = static_cast<int>((objBodySize) / TAGGED_SIZE);
    uintptr_t startAddr = ToUintPtr(objectHeader) + beginOffset;
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<JSTaggedType *>(startAddr + i * TAGGED_SIZE);
        SetObjectSlotField(snapshotObj, beginOffset + i * TAGGED_SIZE, HandleTaggedField(fieldAddr, queue, data));
    }
    SetObjectSlotField(snapshotObj, Program::METHODS_DATA_OFFSET, 0);    // methods
    SetObjectSlotField(snapshotObj, Program::NUMBER_METHODS_OFFSET, 0);  // method_number
}

void SnapShotSerialize::DynProgramDeserialize(uint64_t *objectHeader, [[maybe_unused]] size_t objectSize)
{
    auto object = reinterpret_cast<TaggedObject *>(objectHeader);
    // handle object_header
    DeserializeHandleClassWord(object);

    auto objBodySize = Program::METHODS_DATA_OFFSET - OBJECT_HEADER_SIZE;
    ASSERT(objBodySize % TAGGED_SIZE == 0);
    int numOfFields = static_cast<int>(objBodySize / TAGGED_SIZE);
    size_t addr = ToUintPtr(objectHeader) + OBJECT_HEADER_SIZE;
    for (int i = 0; i < numOfFields; i++) {
        auto fieldAddr = reinterpret_cast<uint64_t *>(addr + i * TAGGED_SIZE);
        DeserializeHandleTaggedField(fieldAddr);
    }

    auto program = reinterpret_cast<Program *>(objectHeader);
    program->SetMethodsData(nullptr);
    program->SetNumberMethods(0);
}

void SnapShotSerialize::SerializePandaFileMethod()
{
    SlotBit slot(0);
    slot.SetObjectType(MASK_METHOD_SPACE_BEGIN);
    slot.SetObjectSize(pandaMethod_.size());

    ObjectFactory *factory = vm_->GetFactory();
    // panda method space begin
    uintptr_t snapshotObj = factory->NewSpaceBySnapShotAllocator(sizeof(uint64_t));
    if (snapshotObj == 0) {
        LOG(ERROR, RUNTIME) << "SnapShotAllocator OOM";
        return;
    }
    SetObjectSlotField(snapshotObj, 0, slot.GetValue());  // methods

    // panda methods
    for (auto &it : pandaMethod_) {
        // write method
        size_t methodObjSize = METHOD_SIZE;
        uintptr_t methodObj = factory->NewSpaceBySnapShotAllocator(methodObjSize);
        if (methodObj == 0) {
            LOG(ERROR, RUNTIME) << "SnapShotAllocator OOM";
            return;
        }
        if (memcpy_s(ToVoidPtr(methodObj), methodObjSize, ToVoidPtr(it), METHOD_SIZE) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

void SnapShotSerialize::RegisterNativeMethod()  // NOLINT(readability-function-size)
{
    constexpr int size = sizeof(g_nativeTable) / sizeof(uintptr_t);
    ASSERT(size == NATIVE_METHOD_SIZE + PROGRAM_NATIVE_METHOD_BEGIN);
    for (int i = 0; i < size; i++) {
        SetAddressToSlot(i, g_nativeTable[i]);
    }
}

void SnapShotSerialize::GeneratedNativeMethod()  // NOLINT(readability-function-size)
{
    for (int i = 0; i < NATIVE_METHOD_SIZE; i++) {
        SetAddressToSlot(i, g_nativeTable[i]);
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(g_nativeTable[i]));
    }
}
}  // namespace panda::ecmascript
