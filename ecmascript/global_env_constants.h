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

#ifndef RUNTIME_ECMASCRIPT_ECMA_ROOTS_H
#define RUNTIME_ECMASCRIPT_ECMA_ROOTS_H

#include <cstdint>

#include "ecmascript/mem/object_xray.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
// Forward Declaration
class ObjectFactory;
template<typename T>
class JSHandle;
class JSHClass;
class JSThread;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_CONSTANT_CLASS(V)                                                                                  \
    /* GC Root */                                                                                                     \
    V(JSTaggedValue, HClassClass, HCLASS_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, StringClass, STRING_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, ArrayClass, ARRAY_CLASS_INDEX, ecma_roots_class)                                                 \
    V(JSTaggedValue, DictionaryClass, DICTIONARY_CLASS_INDEX, ecma_roots_class)                                       \
    V(JSTaggedValue, JSNativePointerClass, JS_NATIVE_POINTER_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, BigIntClass, BIGINT_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, EnvClass, ENV_CLASS_INDEX, ecma_roots_class)                                                     \
    V(JSTaggedValue, FreeObjectWithNoneFieldClass, FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX, ecma_roots_class)         \
    V(JSTaggedValue, FreeObjectWithOneFieldClass, FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX, ecma_roots_class)           \
    V(JSTaggedValue, FreeObjectWithTwoFieldClass, FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX, ecma_roots_class)           \
    V(JSTaggedValue, SymbolClass, SYMBOL_CLASS_INDEX, ecma_roots_class)                                               \
    V(JSTaggedValue, AccessorDataClass, ACCESSOR_DATA_CLASS_INDEX, ecma_roots_class)                                  \
    V(JSTaggedValue, InternalAccessorClass, INTERNAL_ACCESSOR_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, JSProxyOrdinaryClass, JS_PROXY_ORDINARY_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, CompletionRecordClass, COMPLETION_RECORD_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, GeneratorContextClass, GENERATOR_CONTEST_INDEX, ecma_roots_class)                                \
    V(JSTaggedValue, CapabilityRecordClass, CAPABILITY_RECORD_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, ReactionsRecordClass, REACTIONS_RECORD_CLASS_INDEX, ecma_roots_class)                            \
    V(JSTaggedValue, PromiseIteratorRecordClass, PROMISE_ITERATOR_RECORD_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, PromiseRecordClass, PROMISE_RECORD_CLASS_INDEX, ecma_roots_class)                                \
    V(JSTaggedValue, PromiseResolvingFunctionsRecordClass, PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX, ecma_roots_class) \
    V(JSTaggedValue, MicroJobQueueClass, MICRO_JOB_QUEUE_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, PendingJobClass, PENDING_JOB_CLASS_INDEX, ecma_roots_class)                                      \
    V(JSTaggedValue, ProtoChangeMarkerClass, PROTO_CHANGE_MARKER_CLASS_INDEX, ecma_roots_class)                       \
    V(JSTaggedValue, ProtoChangeDetailsClass, PROTO_CHANGE_DETAILS_CLASS_INDEX, ecma_roots_class)                     \
    V(JSTaggedValue, PrototypeHandlerClass, PROTOTYPE_HANDLER_CLASS_INDEX, ecma_roots_class)                          \
    V(JSTaggedValue, TransitionHandlerClass, TRANSITION_HANDLER_CLASS_INDEX, ecma_roots_class)                        \
    V(JSTaggedValue, PropertyBoxClass, PROPERTY_BOX_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ProgramClass, PROGRAM_CLASS_INDEX, ecma_roots_class)                                             \
    V(JSTaggedValue, JSProxyCallableClass, JS_PROXY_CALLABLE_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, JSProxyConstructClass, JS_PROXY_CONSTRUCT_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, JSRealmClass, JS_REALM_CLASS_INDEX, ecma_roots_class)                                            \
    V(JSTaggedValue, JSRegExpClass, JS_REGEXP_CLASS_INDEX, ecma_roots_class)                                          \
    V(JSTaggedValue, MachineCodeClass, MACHINE_CODE_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ClassInfoExtractorHClass, CLASS_INFO_EXTRACTOR_HCLASS_INDEX, ecma_roots_class)                   \
    V(JSTaggedValue, TSObjectTypeClass, TS_OBJECT_TYPE_CLASS_INDEX, ecma_roots_class)                                 \
    V(JSTaggedValue, TSClassTypeClass, TS_CLASS_TYPE_CLASS_INDEX, ecma_roots_class)                                   \
    V(JSTaggedValue, TSUnionTypeClass, TS_UNION_TYPE_CLASS_INDEX, ecma_roots_class)                                   \
    V(JSTaggedValue, TSInterfaceTypeClass, TS_INTERFACE_TYPE_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, TSClassInstanceTypeClass, TS_CLASS_INSTANCE_TYPE_CLASS_INDEX, ecma_roots_class)                  \
    V(JSTaggedValue, TSImportTypeClass, TS_IMPORT_TYPE_CLASS_INDEX, ecma_roots_class)                                 \
    V(JSTaggedValue, TSFunctionTypeClass, TS_FUNCTION_TYPE_CLASS_INDEX, ecma_roots_class)                             \
    V(JSTaggedValue, TSArrayTypeClass, TS_ARRAY_TYPE_CLASS_INDEX, ecma_roots_class)                                   \
    V(JSTaggedValue, ImportEntryClass, IMPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, ExportEntryClass, EXPORT_ENTRY_CLASS_INDEX, ecma_roots_class)                                    \
    V(JSTaggedValue, SourceTextModuleClass, SOURCE_TEXT_MODULE_CLASS_INDEX, ecma_roots_class)                         \
    V(JSTaggedValue, ResolvedBindingClass, RESOLVED_BINDING_CLASS_INDEX, ecma_roots_class)                            \
    V(JSTaggedValue, JSSetIteratorClass, JS_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, JSMapIteratorClass, JS_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)                               \
    V(JSTaggedValue, JSArrayIteratorClass, JS_ARRAY_ITERATOR_CLASS_INDEX, ecma_roots_class)                           \
    V(JSTaggedValue, JSAPIArrayListIteratorClass, JS_API_ARRAYLIST_ITERATOR_CLASS_INDEX, ecma_roots_class)            \
    V(JSTaggedValue, JSAPIDequeIteratorClass, JS_API_DEQUE_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPIQueueIteratorClass, JS_API_QUEUE_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPIStackIteratorClass, JS_API_STACK_ITERATOR_CLASS_INDEX, ecma_roots_class)                    \
    V(JSTaggedValue, JSAPITreeMapIteratorClass, JS_API_TREE_MAP_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, JSAPITreeSetIteratorClass, JS_API_TREE_SET_ITERATOR_CLASS_INDEX, ecma_roots_class)               \
    V(JSTaggedValue, JSAPIIteratorFuncDynClass, JS_API_ITERATOR_FUNC_DYN_CLASS_INDEX, ecma_roots_class)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_CONSTANT_SPECIAL(V)                                                      \
    V(JSTaggedValue, Undefined, UNDEFINED_INDEX, ecma_roots_special)                        \
    V(JSTaggedValue, Null, NULL_INDEX, ecma_roots_special)                                  \
    V(JSTaggedValue, EmptyString, EMPTY_STRING_OBJECT_INDEX, ecma_roots_special)            \
    V(JSTaggedValue, EmptyLayoutInfo, EMPTY_LAYOUT_INFO_OBJECT_INDEX, ecma_roots_special)   \
    V(JSTaggedValue, EmptyArray, EMPTY_ARRAY_OBJECT_INDEX, ecma_roots_special)              \
    V(JSTaggedValue, EmptyTaggedQueue, EMPTY_TAGGED_QUEUE_OBJECT_INDEX, ecma_roots_special) \
/* GlobalConstant */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_CONSTANT_CONSTANT(V)                                                                               \
    V(JSTaggedValue, ConstructorString, CONSTRUCTOR_STRING_INDEX, constructor)                                        \
    V(JSTaggedValue, PrototypeString, PROTOTYPE_STRING_INDEX, prototype)                                              \
    V(JSTaggedValue, LengthString, LENGTH_STRING_INDEX, length)                                                       \
    V(JSTaggedValue, ValueString, VALUE_STRING_INDEX, value)                                                          \
    V(JSTaggedValue, GetString, GET_STRING_INDEX, set)                                                                \
    V(JSTaggedValue, SetString, SET_STRING_INDEX, get)                                                                \
    V(JSTaggedValue, WritableString, WRITABLE_STRING_INDEX, writable)                                                 \
    V(JSTaggedValue, EnumerableString, ENUMERABLE_STRING_INDEX, enumerable)                                           \
    V(JSTaggedValue, ConfigurableString, CONFIGURABLE_STRING_INDEX, configurable)                                     \
    /* non ECMA standard jsapi containers iterators */                                                                \
    V(JSTaggedValue, ArrayListFunction, ARRAYLIST_FUNCTION_INDEX, ArrayListFunction)                                  \
    V(JSTaggedValue, ArrayListIteratorPrototype, ARRAYLIST_ITERATOR_PROTOTYPE_INDEX, ArrayListIterator)               \
    V(JSTaggedValue, TreeMapIteratorPrototype, TREEMAP_ITERATOR_PROTOTYPE_INDEX, TreeMapIterator)                     \
    V(JSTaggedValue, TreeSetIteratorPrototype, TREESET_ITERATOR_PROTOTYPE_INDEX, TreeSetIterator)                     \
    V(JSTaggedValue, QueueIteratorPrototype, QUEUE_ITERATOR_PROTOTYPE_INDEX, QueueIterator)                           \
    V(JSTaggedValue, DequeIteratorPrototype, DEQUE_ITERATOR_PROTOTYPE_INDEX, DequeIterator)                           \
    V(JSTaggedValue, StackIteratorPrototype, STACK_ITERATOR_PROTOTYPE_INDEX, StackIterator)                           \
    /* SymbolTable*RegisterSymbols */                                                                                 \
    V(JSTaggedValue, NameString, NAME_STRING_INDEX, name)                                                             \
    V(JSTaggedValue, GetPrototypeOfString, GETPROTOTYPEOF_STRING_INDEX, getPrototypeOf)                               \
    V(JSTaggedValue, SetPrototypeOfString, SETPROTOTYPEOF_STRING_INDEX, setPrototypeOf)                               \
    V(JSTaggedValue, IsExtensibleString, ISEXTENSIBLE_STRING_INDEX, isExtensible)                                     \
    V(JSTaggedValue, PreventExtensionsString, PREVENTEXTENSIONS_STRING_INDEX, preventExtensions)                      \
    V(JSTaggedValue, GetOwnPropertyDescriptorString, GETOWNPROPERTYDESCRIPTOR_STRING_INDEX, getOwnPropertyDescriptor) \
    V(JSTaggedValue, DefinePropertyString, DEFINEPROPERTY_STRING_INDEX, defineProperty)                               \
    V(JSTaggedValue, HasString, HAS_STRING_INDEX, has)                                                                \
    V(JSTaggedValue, DeletePropertyString, DELETEPROPERTY_STRING_INDEX, deleteProperty)                               \
    V(JSTaggedValue, EnumerateString, ENUMERATE_STRING_INDEX, enumerate)                                              \
    V(JSTaggedValue, OwnKeysString, OWNKEYS_STRING_INDEX, ownKeys)                                                    \
    V(JSTaggedValue, ApplyString, APPLY_STRING_INDEX, apply)                                                          \
    V(JSTaggedValue, NegativeZeroString, NEGATIVE_ZERO_STRING_INDEX, -0)                                              \
    V(JSTaggedValue, DoneString, DONE_STRING_INDEX, done)                                                             \
    V(JSTaggedValue, ProxyString, PROXY_STRING_INDEX, proxy)                                                          \
    V(JSTaggedValue, RevokeString, REVOKE_STRING_INDEX, revoke)                                                       \
    V(JSTaggedValue, NextString, NEXT_STRING_INDEX, next)                                                             \
    V(JSTaggedValue, ToStringString, TO_STRING_STRING_INDEX, toString)                                                \
    V(JSTaggedValue, ToLocaleStringString, TO_LOCALE_STRING_STRING_INDEX, toLocaleString)                             \
    V(JSTaggedValue, ValueOfString, VALUE_OF_STRING_INDEX, valueOf)                                                   \
    V(JSTaggedValue, UndefinedString, UNDEFINED_STRING_INDEX, undefined)                                              \
    V(JSTaggedValue, NullString, NULL_STRING_INDEX, null)                                                             \
    V(JSTaggedValue, BooleanString, BOOLEAN_STRING_INDEX, boolean)                                                    \
    V(JSTaggedValue, NumberString, NUMBER_STRING_INDEX, number)                                                       \
    V(JSTaggedValue, BigIntString, BIGINT_STRING_INDEX, bigint)                                                       \
    V(JSTaggedValue, FunctionString, FUNCTION_STRING_INDEX, function)                                                 \
    V(JSTaggedValue, StringString, STRING_STRING_INDEX, string)                                                       \
    V(JSTaggedValue, SymbolString, SYMBOL_STRING_INDEX, symbol)                                                       \
    V(JSTaggedValue, ObjectString, OBJECT_STRING_INDEX, object)                                                       \
    V(JSTaggedValue, TrueString, TRUE_STRING_INDEX, true)                                                             \
    V(JSTaggedValue, FalseString, FALSE_STRING_INDEX, false)                                                          \
    V(JSTaggedValue, ReturnString, RETURN_STRING_INDEX, return )                                                      \
    V(JSTaggedValue, ProxyConstructString, PROXY_CONSTRUCT_STRING_INDEX, construct)                                   \
    V(JSTaggedValue, ProxyCallString, PROXY_CALL_STRING_INDEX, call)                                                  \
    V(JSTaggedValue, PromiseThenString, PROMISE_THEN_STRING_INDEX, then)                                              \
    V(JSTaggedValue, PromiseCatchString, PROMISE_CATCH_STRING_INDEX, catch)                                           \
    V(JSTaggedValue, ScriptJobString, SCRIPT_JOB_STRING_INDEX, ScriptJobs)                                            \
    V(JSTaggedValue, PromiseString, PROMISE_STRING_INDEX, PrimiseJobs)                                                \
    V(JSTaggedValue, ThrowerString, THROWER_STRING_INDEX, Thrower)                                                    \
    V(JSTaggedValue, IdentityString, IDENTITY_STRING_INDEX, Identity)                                                 \
    V(JSTaggedValue, CallerString, CALLER_STRING_INDEX, caller)                                                       \
    V(JSTaggedValue, CalleeString, CALLEE_STRING_INDEX, callee)                                                       \
    V(JSTaggedValue, Int8ArrayString, INT8_ARRAY_STRING_INDEX, Int8Array)                                             \
    V(JSTaggedValue, Uint8ArrayString, UINT8_ARRAY_STRING_INDEX, Uint8Array)                                          \
    V(JSTaggedValue, Uint8ClampedArrayString, UINT8_CLAMPED_ARRAY_STRING_INDEX, Uint8ClampedArray)                    \
    V(JSTaggedValue, Int16ArrayString, INT16_ARRAY_STRING_INDEX, Int16Array)                                          \
    V(JSTaggedValue, Uint16ArrayString, UINT16_ARRAY_STRING_INDEX, Uint16Array)                                       \
    V(JSTaggedValue, Int32ArrayString, INT32_ARRAY_STRING_INDEX, Int32Array)                                          \
    V(JSTaggedValue, Uint32ArrayString, UINT32_ARRAY_STRING_INDEX, Uint32Array)                                       \
    V(JSTaggedValue, Float32ArrayString, FLOAT32_ARRAY_STRING_INDEX, Float32Array)                                    \
    V(JSTaggedValue, Float64ArrayString, FLOAT64_ARRAY_STRING_INDEX, Float64Array)                                    \
    V(JSTaggedValue, BigInt64ArrayString, BIGINT64_ARRAY_STRING_INDEX, BigInt64Array)                                 \
    V(JSTaggedValue, BigUint64ArrayString, BIGUINT64_ARRAY_STRING_INDEX, BigUint64Array)                              \
    V(JSTaggedValue, AsyncFunctionString, ASYNC_FUNCTION_STRING_INDEX, AsyncFunction)                                 \
    V(JSTaggedValue, PromiseResolveString, PROMISE_RESOLVE_STRING_INDEX, resolve)                                     \
    V(JSTaggedValue, IdString, ID_STRING_INDEX, id)                                                                   \
    V(JSTaggedValue, MethodString, METHOD_STRING_INDEX, method)                                                       \
    V(JSTaggedValue, ParamsString, PARAMS_STRING_INDEX, params)                                                       \
    V(JSTaggedValue, ResultString, RESULT_STRING_INDEX, result)                                                       \
    V(JSTaggedValue, ToJsonString, TO_JSON_STRING_INDEX, toJSON)                                                      \
    V(JSTaggedValue, GlobalString, GLOBAL_STRING_INDEX, global)                                                       \
    V(JSTaggedValue, MessageString, MESSAGE_STRING_INDEX, message)                                                    \
    V(JSTaggedValue, ErrorString, ERROR_STRING_INDEX, Error)                                                          \
    V(JSTaggedValue, RangeErrorString, RANGE_ERROR_STRING_INDEX, RangeError)                                          \
    V(JSTaggedValue, ReferenceErrorString, REFERENCE_ERROR_STRING_INDEX, ReferenceError)                              \
    V(JSTaggedValue, TypeErrorString, TYPE_ERROR_STRING_INDEX, TypeError)                                             \
    V(JSTaggedValue, URIErrorString, URI_ERROR_STRING_INDEX, URIError)                                                \
    V(JSTaggedValue, SyntaxErrorString, SYNTAX_ERROR_STRING_INDEX, SyntaxError)                                       \
    V(JSTaggedValue, EvalErrorString, EVAL_ERROR_STRING_INDEX, EvalError)                                             \
    V(JSTaggedValue, StackString, STACK_STRING_INDEX, stack)                                                          \
    V(JSTaggedValue, StackEmptyString, STACK_EMPTY_STRING_INDEX, stackisempty)                                        \
    V(JSTaggedValue, ObjNotCoercibleString, OBJ_NOT_COERCIBLE_STRING_INDEX, objectnotcoercible)                       \
    /* forIntl. */                                                                                                    \
    V(JSTaggedValue, LanguageString, LANGUAGE_STRING_CLASS_INDEX, language)                                           \
    V(JSTaggedValue, ScriptString, SCRIPT_STRING_CLASS_INDEX, script)                                                 \
    V(JSTaggedValue, RegionString, REGION_STRING_CLASS_INDEX, region)                                                 \
    V(JSTaggedValue, BaseNameString, BASE_NAME_STRING_CLASS_INDEX, baseName)                                          \
    V(JSTaggedValue, CalendarString, CALENDAR_STRING_CLASS_INDEX, calendar)                                           \
    V(JSTaggedValue, CollationString, COLLATION_STRING_CLASS_INDEX, collation)                                        \
    V(JSTaggedValue, HourCycleString, HOUR_CYCLE_STRING_CLASS_INDEX, hourCycle)                                       \
    V(JSTaggedValue, CaseFirstString, CASE_FIRST_STRING_CLASS_INDEX, caseFirst)                                       \
    V(JSTaggedValue, NumericString, NUMERIC_STRING_CLASS_INDEX, numeric)                                              \
    V(JSTaggedValue, NumberingSystemString, NUMBERING_SYSTEM_STRING_CLASS_INDEX, numberingSystem)                     \
    V(JSTaggedValue, TypeString, TYPE_STRING_INDEX, type)                                                             \
    V(JSTaggedValue, LocaleMatcherString, LOCALE_MATCHER_STRING_INDEX, localeMatcher)                                 \
    V(JSTaggedValue, FormatMatcherString, FORMAT_MATCHER_STRING_INDEX, formatMatcher)                                 \
    V(JSTaggedValue, Hour12String, HOUR12_STRING_INDEX, hour12)                                                       \
    V(JSTaggedValue, H11String, H11_STRING_INDEX, h11)                                                                \
    V(JSTaggedValue, H12String, H12_STRING_INDEX, h12)                                                                \
    V(JSTaggedValue, H23String, H23_STRING_INDEX, h23)                                                                \
    V(JSTaggedValue, H24String, H24_STRING_INDEX, h24)                                                                \
    V(JSTaggedValue, WeekdayString, WEEK_DAY_STRING_INDEX, weekday)                                                   \
    V(JSTaggedValue, EraString, ERA_STRING_INDEX, era)                                                                \
    V(JSTaggedValue, YearString, YEAR_STRING_INDEX, year)                                                             \
    V(JSTaggedValue, QuarterString, QUARTER_STRING_INDEX, quarter)                                                    \
    V(JSTaggedValue, MonthString, MONTH_STRING_INDEX, month)                                                          \
    V(JSTaggedValue, DayString, DAY_STRING_INDEX, day)                                                                \
    V(JSTaggedValue, HourString, HOUR_STRING_INDEX, hour)                                                             \
    V(JSTaggedValue, MinuteString, MINUTE_STRING_INDEX, minute)                                                       \
    V(JSTaggedValue, SecondString, SECOND_STRING_INDEX, second)                                                       \
    V(JSTaggedValue, YearsString, YEARS_STRING_INDEX, years)                                                          \
    V(JSTaggedValue, QuartersString, QUARTERS_STRING_INDEX, quarters)                                                 \
    V(JSTaggedValue, MonthsString, MONTHS_STRING_INDEX, months)                                                       \
    V(JSTaggedValue, DaysString, DAYS_STRING_INDEX, days)                                                             \
    V(JSTaggedValue, HoursString, HOURS_STRING_INDEX, hours)                                                          \
    V(JSTaggedValue, MinutesString, MINUTES_STRING_INDEX, minutes)                                                    \
    V(JSTaggedValue, SecondsString, SECONDS_STRING_INDEX, seconds)                                                    \
    V(JSTaggedValue, TimeZoneNameString, TIME_ZONE_NAME_STRING_INDEX, timeZoneName)                                   \
    V(JSTaggedValue, LocaleString, LOCALE_STRING_INDEX, locale)                                                       \
    V(JSTaggedValue, TimeZoneString, TIME_ZONE_STRING_INDEX, timeZone)                                                \
    V(JSTaggedValue, LiteralString, LITERAL_STRING_INDEX, literal)                                                    \
    V(JSTaggedValue, YearNameString, YEAR_NAME_STRING_INDEX, yearName)                                                \
    V(JSTaggedValue, DayPeriodString, DAY_PERIOD_STRING_INDEX, dayPeriod)                                             \
    V(JSTaggedValue, FractionalSecondDigitsString, FRACTIONAL_SECOND_DIGITS_STRING_INDEX, fractionalSecondDigits)     \
    V(JSTaggedValue, FractionalSecondString, FRACTIONAL_SECOND_STRING_INDEX, fractionalSecond)                        \
    V(JSTaggedValue, RelatedYearString, RELATED_YEAR_STRING_INDEX, relatedYear)                                       \
    V(JSTaggedValue, LookUpString, LOOK_UP_STRING_INDEX, lookup)                                                      \
    V(JSTaggedValue, BestFitString, BEST_FIT_STRING_INDEX, bestfit)                                                   \
    V(JSTaggedValue, DateStyleString, DATE_STYLE_STRING_INDEX, dateStyle)                                             \
    V(JSTaggedValue, TimeStyleString, TIME_STYLE_STRING_INDEX, timeStyle)                                             \
    V(JSTaggedValue, UTCString, UTC_STRING_INDEX, UTC)                                                                \
    V(JSTaggedValue, InitializedRelativeTimeFormatString, INITIALIZED_RELATIVE_INDEX, true)                           \
    V(JSTaggedValue, WeekString, WEEK_STRING_INDEX, week)                                                             \
    V(JSTaggedValue, WeeksString, WEEKS_STRING_INDEX, weeks)                                                          \
    V(JSTaggedValue, SourceString, SOURCE_STRING_INDEX, source)                                                       \
    V(JSTaggedValue, FormatString, FORMAT_STRING_INDEX, format)                                                       \
    V(JSTaggedValue, EnUsString, EN_US_STRING_INDEX, en - US)                                                         \
    V(JSTaggedValue, UndString, UND_STRING_INDEX, und)                                                                \
    V(JSTaggedValue, LatnString, LATN_STRING_INDEX, latn)                                                             \
    V(JSTaggedValue, StyleString, STYLE_STRING_INDEX, style)                                                          \
    V(JSTaggedValue, UnitString, UNIT_STRING_INDEX, unit)                                                             \
    V(JSTaggedValue, IntegerString, INTEGER_STRING_INDEX, integer)                                                    \
    V(JSTaggedValue, NanString, NAN_STRING_INDEX, nan)                                                                \
    V(JSTaggedValue, InfinityString, INFINITY_STRING_INDEX, infinity)                                                 \
    V(JSTaggedValue, FractionString, FRACTION_STRING_INDEX, fraction)                                                 \
    V(JSTaggedValue, DecimalString, DECIMAL_STRING_INDEX, decimal)                                                    \
    V(JSTaggedValue, GroupString, GROUP_STRING_INDEX, group)                                                          \
    V(JSTaggedValue, CurrencyString, CURRENCY_STRING_INDEX, currency)                                                 \
    V(JSTaggedValue, CurrencySignString, CURRENCY_SIGN_STRING_INDEX, currencySign)                                    \
    V(JSTaggedValue, CurrencyDisplayString, CURRENCY_DISPLAY_STRING_INDEX, currencyDisplay)                           \
    V(JSTaggedValue, PercentSignString, PERCENT_SIGN_STRING_INDEX, percentSign)                                       \
    V(JSTaggedValue, PercentString, PERCENT_STRING_INDEX, percent)                                                    \
    V(JSTaggedValue, MinusSignString, MINUS_SIGN_STRING_INDEX, minusSign)                                             \
    V(JSTaggedValue, PlusSignString, PLUS_SIGN_STRING_INDEX, plusSign)                                                \
    V(JSTaggedValue, ExponentSeparatorString, EXPONENT_SEPARATOR_STRING_INDEX, exponentSeparator)                     \
    V(JSTaggedValue, ExponentMinusSignString, EXPONENT_MINUS_SIGN_INDEX, exponentMinusSign)                           \
    V(JSTaggedValue, ExponentIntegerString, EXPONENT_INTEGER_STRING_INDEX, exponentInteger)                           \
    V(JSTaggedValue, LongString, LONG_STRING_INDEX, long)                                                             \
    V(JSTaggedValue, ShortString, SHORT_STRING_INDEX, short)                                                          \
    V(JSTaggedValue, FullString, FULL_STRING_INDEX, full)                                                             \
    V(JSTaggedValue, MediumString, MEDIUM_STRING_INDEX, medium)                                                       \
    V(JSTaggedValue, NarrowString, NARROW_STRING_INDEX, narrow)                                                       \
    V(JSTaggedValue, AlwaysString, ALWAYS_STRING_INDEX, always)                                                       \
    V(JSTaggedValue, AutoString, AUTO_STRING_INDEX, auto)                                                             \
    V(JSTaggedValue, UnitDisplayString, UNIT_DISPLAY_INDEX, unitDisplay)                                              \
    V(JSTaggedValue, NotationString, NOTATION_INDEX, notation)                                                        \
    V(JSTaggedValue, CompactDisplayString, COMPACT_DISPALY_INDEX, compactDisplay)                                     \
    V(JSTaggedValue, UserGroupingString, USER_GROUPING_INDEX, useGrouping)                                            \
    V(JSTaggedValue, SignDisplayString, SIGN_DISPLAY_INDEX, signDisplay)                                              \
    V(JSTaggedValue, CodeString, CODE_INDEX, code)                                                                    \
    V(JSTaggedValue, NarrowSymbolString, NARROW_SYMBOL_INDEX, narrowSymbol)                                           \
    V(JSTaggedValue, StandardString, STANDARD_INDEX, standard)                                                        \
    V(JSTaggedValue, AccountingString, ACCOUNTING_INDEX, accounting)                                                  \
    V(JSTaggedValue, ScientificString, SCIENTIFIC_INDEX, scientific)                                                  \
    V(JSTaggedValue, EngineeringString, ENGINEERING_INDEX, engineering)                                               \
    V(JSTaggedValue, CompactString, COMPACT_STRING_INDEX, compact)                                                    \
    V(JSTaggedValue, NeverString, NEVER_INDEX, never)                                                                 \
    V(JSTaggedValue, ExceptZeroString, EXPECT_ZERO_INDEX, exceptZero)                                                 \
    V(JSTaggedValue, MinimumIntegerDigitsString, MINIMUM_INTEGER_DIGITS_INDEX, minimumIntegerDigits)                  \
    V(JSTaggedValue, MinimumFractionDigitsString, MINIMUM_FRACTIONDIGITS_INDEX, minimumFractionDigits)                \
    V(JSTaggedValue, MaximumFractionDigitsString, MAXIMUM_FRACTIONDIGITS_INDEX, maximumFractionDigits)                \
    V(JSTaggedValue, MinimumSignificantDigitsString, MINIMUM_SIGNIFICANTDIGITS_INDEX, minimumSignificantDigits)       \
    V(JSTaggedValue, MaximumSignificantDigitsString, MAXIMUM_SIGNIFICANTDIGITS_INDEX, maximumSignificantDigits)       \
    V(JSTaggedValue, InvalidDateString, INVALID_DATE_INDEX, InvalidDate)                                              \
    V(JSTaggedValue, UsageString, USAGE_INDEX, usage)                                                                 \
    V(JSTaggedValue, CompareString, COMPARE_INDEX, compare)                                                           \
    V(JSTaggedValue, SensitivityString, SENSITIVITY_INDEX, sensitivity)                                               \
    V(JSTaggedValue, IgnorePunctuationString, IGNORE_PUNCTUATION_INDEX, ignorePunctuation)                            \
    V(JSTaggedValue, CardinalString, CARDINAL_INDEX, cardinal)                                                        \
    V(JSTaggedValue, OrdinalString, ORDINAL_INDEX, ordinal)                                                           \
    V(JSTaggedValue, PluralCategoriesString, PLURAL_CATEGORIES_INDEX, pluralCategories)                               \
    V(JSTaggedValue, SortString, SORT_INDEX, sort)                                                                    \
    V(JSTaggedValue, SearchString, SEARCH_INDEX, search)                                                              \
    V(JSTaggedValue, BaseString, BASE_INDEX, base)                                                                    \
    V(JSTaggedValue, AccentString, ACCENT_INDEX, accent)                                                              \
    V(JSTaggedValue, CaseString, CASE_INDEX, Case)                                                                    \
    V(JSTaggedValue, VariantString, VARIANT_INDEX, variant)                                                           \
    V(JSTaggedValue, EnUsPosixString, EN_US_POSIX_STRING_INDEX, en - US - POSIX)                                      \
    V(JSTaggedValue, UpperString, UPPER_INDEX, upper)                                                                 \
    V(JSTaggedValue, LowerString, LOWER_INDEX, lower)                                                                 \
    V(JSTaggedValue, DefaultString, DEFAULT_INDEX, Default)                                                           \
    V(JSTaggedValue, SharedString, SHARED_INDEX, shared)                                                              \
    V(JSTaggedValue, StartRangeString, START_RANGE_INDEX, startRange)                                                 \
    V(JSTaggedValue, EndRangeString, END_RANGE_INDEX, endRange)                                                       \
    V(JSTaggedValue, Iso8601String, ISO8601_INDEX, iso8601)                                                           \
    V(JSTaggedValue, GregoryString, GREGORY_INDEX, gregory)                                                           \
    V(JSTaggedValue, EthioaaString, ETHIOAA_INDEX, ethioaa)                                                           \
    V(JSTaggedValue, ValuesString, VALUES_INDEX, values)                                                              \
    V(JSTaggedValue, FallbackString, FALLBACK_INDEX, fallback)                                                        \
    V(JSTaggedValue, DateTimeFieldString, DATETIMEFIELD_INDEX, datetimefield)                                         \
    V(JSTaggedValue, NoneString, NONE_INDEX, none)                                                                    \
    /* for regexp. */                                                                                                 \
    V(JSTaggedValue, ExecString, EXEC_INDEX, exec)                                                                    \
    V(JSTaggedValue, LastIndexString, LAST_INDEX_INDEX, lastIndex)                                                    \
    V(JSTaggedValue, StickyString, STICKY_INDEX, sticky)                                                              \
    V(JSTaggedValue, UString, U_INDEX, u)                                                                             \
    V(JSTaggedValue, IndexString, INDEX_INDEX, index)                                                                 \
    V(JSTaggedValue, InputString, INPUT_INDEX, input)                                                                 \
    V(JSTaggedValue, UnicodeString, UNICODE_INDEX, unicode)                                                           \
    V(JSTaggedValue, ZeroString, ZERO_INDEX, zero)                                                                    \
    /* for module. */                                                                                                 \
    V(JSTaggedValue, AmbiguousString, AMBIGUOUS_INDEX, ambiguous)                                                     \
    V(JSTaggedValue, ModuleString, MODULE_INDEX, module)                                                              \
    V(JSTaggedValue, StarString, STAR_INDEX, star)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_CONSTANT_ACCESSOR(V)                                                           \
    V(JSTaggedValue, FunctionPrototypeAccessor, FUNCTION_PROTOTYPE_ACCESSOR, ecma_roots_accessor) \
    V(JSTaggedValue, FunctionNameAccessor, FUNCTION_NAME_ACCESSOR, ecma_roots_accessor)           \
    V(JSTaggedValue, ArrayLengthAccessor, ARRAY_LENGTH_ACCESSOR, ecma_roots_accessor)
/* RealmConstant */

// ConstantIndex used for explicit visit each constant.
enum class ConstantIndex : size_t {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INDEX_FILTER(Type, Name, Index, Desc) Index,
    GLOBAL_ENV_CONSTANT_CLASS(INDEX_FILTER) GLOBAL_ENV_CONSTANT_SPECIAL(INDEX_FILTER)
        GLOBAL_ENV_CONSTANT_CONSTANT(INDEX_FILTER) GLOBAL_ENV_CONSTANT_ACCESSOR(INDEX_FILTER)

#undef INDEX_FILTER
            CONSTATNT_COUNT,

    CONSTATNT_BEGIN = 0,
    CONSTATNT_END = CONSTATNT_COUNT,

    READ_ONLY_CONSTATNT_BEGIN = CONSTATNT_BEGIN,
    READ_ONLY_CONSTATNT_END = CONSTATNT_END,
    // ...
};
// clang-format on

class GlobalEnvConstants {
public:
    explicit GlobalEnvConstants() = default;

    DEFAULT_MOVE_SEMANTIC(GlobalEnvConstants);
    DEFAULT_COPY_SEMANTIC(GlobalEnvConstants);

    ~GlobalEnvConstants() = default;

    const JSTaggedValue *BeginSlot() const;

    const JSTaggedValue *EndSlot() const;

    void Init(JSThread *thread, JSHClass *dynClassClass);

    void InitRootsClass(JSThread *thread, JSHClass *dynClassClass);
    void InitGlobalConstantSpecial(JSThread *thread);

    void InitGlobalConstant(JSThread *thread);

    void InitGlobalUndefined();

    void SetConstant(ConstantIndex index, JSTaggedValue value);

    template<typename T>
    void SetConstant(ConstantIndex index, JSHandle<T> value);

    uintptr_t GetGlobalConstantAddr(ConstantIndex index) const;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_GET(Type, Name, Index, Desc) \
    const Type Get##Name() const;         \
    const JSHandle<Type> GetHandled##Name() const;
    GLOBAL_ENV_CONSTANT_CLASS(DECL_GET)
    GLOBAL_ENV_CONSTANT_SPECIAL(DECL_GET)
    GLOBAL_ENV_CONSTANT_CONSTANT(DECL_GET)
    GLOBAL_ENV_CONSTANT_ACCESSOR(DECL_GET)
#undef DECL_GET

    void VisitRangeSlot(const RootRangeVisitor &visitor)
    {
        visitor(ecmascript::Root::ROOT_VM, ObjectSlot(ToUintPtr(BeginSlot())), ObjectSlot(ToUintPtr(EndSlot())));
    }

    static constexpr size_t SizeArch32 =
        JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(ConstantIndex::CONSTATNT_COUNT);
    static constexpr size_t SizeArch64 =
        JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(ConstantIndex::CONSTATNT_COUNT);

private:
    JSTaggedValue constants_[static_cast<int>(ConstantIndex::CONSTATNT_COUNT)];  // NOLINT(modernize-avoid-c-arrays)
};
STATIC_ASSERT_EQ_ARCH(sizeof(GlobalEnvConstants), GlobalEnvConstants::SizeArch32, GlobalEnvConstants::SizeArch64);
}  // namespace panda::ecmascript
#endif  // RUNTIME_ECMASCRIPT_ECMA_ROOTS_H
