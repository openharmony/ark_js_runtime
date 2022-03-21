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

#include "ecmascript/global_env_constants.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/builtins.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
void GlobalEnvConstants::InitRootsClass([[maybe_unused]] JSThread *thread, JSHClass *dynClassClass)
{
    // Global constants are readonly.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    SetConstant(ConstantIndex::HCLASS_CLASS_INDEX, JSTaggedValue(dynClassClass));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, FreeObject::NEXT_OFFSET, JSType::FREE_OBJECT_WITH_NONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, FreeObject::SIZE_OFFSET, JSType::FREE_OBJECT_WITH_ONE_FIELD));
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, FreeObject::SIZE, JSType::FREE_OBJECT_WITH_TWO_FIELD));
    SetConstant(ConstantIndex::STRING_CLASS_INDEX, factory->NewEcmaDynClass(dynClassClass, 0, JSType::STRING));
    SetConstant(ConstantIndex::ARRAY_CLASS_INDEX, factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_ARRAY));
    SetConstant(ConstantIndex::DICTIONARY_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_DICTIONARY));
    SetConstant(ConstantIndex::BIGINT_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, BigInt::SIZE, JSType::BIGINT));
    SetConstant(ConstantIndex::JS_NATIVE_POINTER_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSNativePointer::SIZE, JSType::JS_NATIVE_POINTER));
    SetConstant(ConstantIndex::ENV_CLASS_INDEX, factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_ARRAY));
    SetConstant(ConstantIndex::SYMBOL_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSSymbol::SIZE, JSType::SYMBOL));
    SetConstant(ConstantIndex::ACCESSOR_DATA_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, AccessorData::SIZE, JSType::ACCESSOR_DATA));
    SetConstant(ConstantIndex::INTERNAL_ACCESSOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, AccessorData::SIZE, JSType::INTERNAL_ACCESSOR));
    SetConstant(ConstantIndex::JS_PROXY_ORDINARY_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY));
    SetConstant(ConstantIndex::COMPLETION_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, CompletionRecord::SIZE, JSType::COMPLETION_RECORD));
    SetConstant(ConstantIndex::GENERATOR_CONTEST_INDEX,
                factory->NewEcmaDynClass(dynClassClass, GeneratorContext::SIZE, JSType::JS_GENERATOR_CONTEXT));
    SetConstant(ConstantIndex::CAPABILITY_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseCapability::SIZE, JSType::PROMISE_CAPABILITY));
    SetConstant(ConstantIndex::REACTIONS_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseReaction::SIZE, JSType::PROMISE_REACTIONS));
    SetConstant(ConstantIndex::PROMISE_ITERATOR_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseIteratorRecord::SIZE, JSType::PROMISE_ITERATOR_RECORD));
    SetConstant(ConstantIndex::PROMISE_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseRecord::SIZE, JSType::PROMISE_RECORD));
    SetConstant(ConstantIndex::PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, ResolvingFunctionsRecord::SIZE,
                                         JSType::RESOLVING_FUNCTIONS_RECORD));
    SetConstant(ConstantIndex::MICRO_JOB_QUEUE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, job::MicroJobQueue::SIZE, JSType::MICRO_JOB_QUEUE));
    SetConstant(ConstantIndex::PENDING_JOB_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, job::PendingJob::SIZE, JSType::PENDING_JOB));
    SetConstant(ConstantIndex::PROTO_CHANGE_MARKER_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, ProtoChangeMarker::SIZE, JSType::PROTO_CHANGE_MARKER));
    SetConstant(ConstantIndex::PROTO_CHANGE_DETAILS_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, ProtoChangeDetails::SIZE, JSType::PROTOTYPE_INFO));
    SetConstant(ConstantIndex::PROTOTYPE_HANDLER_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PrototypeHandler::SIZE, JSType::PROTOTYPE_HANDLER));
    SetConstant(ConstantIndex::TRANSITION_HANDLER_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TransitionHandler::SIZE, JSType::TRANSITION_HANDLER));
    SetConstant(ConstantIndex::PROPERTY_BOX_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PropertyBox::SIZE, JSType::PROPERTY_BOX));
    SetConstant(ConstantIndex::PROGRAM_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, Program::SIZE, JSType::PROGRAM));
    SetConstant(
        ConstantIndex::IMPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ImportEntry::SIZE, JSType::IMPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::EXPORT_ENTRY_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ExportEntry::SIZE, JSType::EXPORTENTRY_RECORD));
    SetConstant(
        ConstantIndex::SOURCE_TEXT_MODULE_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, SourceTextModule::SIZE, JSType::SOURCE_TEXT_MODULE_RECORD));
    SetConstant(
        ConstantIndex::RESOLVED_BINDING_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ResolvedBinding::SIZE, JSType::RESOLVEDBINDING_RECORD));

    JSHClass *jsProxyCallableClass = *factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyCallableClass->SetCallable(true);
    SetConstant(ConstantIndex::JS_PROXY_CALLABLE_CLASS_INDEX, JSTaggedValue(jsProxyCallableClass));

    JSHClass *jsProxyConstructClass = *factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyConstructClass->SetCallable(true);
    jsProxyConstructClass->SetConstructor(true);
    SetConstant(ConstantIndex::JS_PROXY_CONSTRUCT_CLASS_INDEX, JSTaggedValue(jsProxyConstructClass));

    SetConstant(ConstantIndex::JS_REALM_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSRealm::SIZE, JSType::JS_REALM));
    SetConstant(ConstantIndex::MACHINE_CODE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::MACHINE_CODE_OBJECT));
    SetConstant(ConstantIndex::CLASS_INFO_EXTRACTOR_HCLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, ClassInfoExtractor::SIZE, JSType::CLASS_INFO_EXTRACTOR));
    SetConstant(ConstantIndex::TS_OBJECT_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSObjectType::SIZE, JSType::TS_OBJECT_TYPE));
    SetConstant(ConstantIndex::TS_CLASS_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSClassType::SIZE, JSType::TS_CLASS_TYPE));
    SetConstant(ConstantIndex::TS_UNION_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSUnionType::SIZE, JSType::TS_UNION_TYPE));
    SetConstant(ConstantIndex::TS_INTERFACE_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSInterfaceType::SIZE, JSType::TS_INTERFACE_TYPE));
    SetConstant(ConstantIndex::TS_CLASS_INSTANCE_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSClassInstanceType::SIZE, JSType::TS_CLASS_INSTANCE_TYPE));
    SetConstant(ConstantIndex::TS_IMPORT_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSImportType::SIZE, JSType::TS_IMPORT_TYPE));
    SetConstant(ConstantIndex::TS_FUNCTION_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSFunctionType::SIZE, JSType::TS_FUNCTION_TYPE));
    SetConstant(ConstantIndex::TS_ARRAY_TYPE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, TSArrayType::SIZE, JSType::TS_ARRAY_TYPE));
}

// NOLINTNEXTLINE(readability-function-size)
void GlobalEnvConstants::InitGlobalConstant(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // SPECIAL INIT
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::NULL_INDEX, JSTaggedValue::Null());
    auto vm = thread->GetEcmaVM();
    SetConstant(ConstantIndex::EMPTY_STRING_OBJECT_INDEX, JSTaggedValue(EcmaString::CreateEmptyString(vm)));
    [[maybe_unused]] auto test = EcmaString::Cast(GetHandledEmptyString().GetObject<EcmaString>());
    SetConstant(ConstantIndex::CONSTRUCTOR_STRING_INDEX, factory->NewFromCanBeCompressString("constructor"));
    SetConstant(ConstantIndex::PROTOTYPE_STRING_INDEX, factory->NewFromCanBeCompressString("prototype"));
    SetConstant(ConstantIndex::LENGTH_STRING_INDEX, factory->NewFromCanBeCompressString("length"));
    SetConstant(ConstantIndex::VALUE_STRING_INDEX, factory->NewFromCanBeCompressString("value"));
    SetConstant(ConstantIndex::SET_STRING_INDEX, factory->NewFromCanBeCompressString("set"));
    SetConstant(ConstantIndex::GET_STRING_INDEX, factory->NewFromCanBeCompressString("get"));
    SetConstant(ConstantIndex::WRITABLE_STRING_INDEX, factory->NewFromCanBeCompressString("writable"));
    SetConstant(ConstantIndex::ENUMERABLE_STRING_INDEX, factory->NewFromCanBeCompressString("enumerable"));
    SetConstant(ConstantIndex::CONFIGURABLE_STRING_INDEX, factory->NewFromCanBeCompressString("configurable"));
    /* SymbolTable *RegisterSymbols */
    SetConstant(ConstantIndex::NAME_STRING_INDEX, factory->NewFromCanBeCompressString("name"));
    SetConstant(ConstantIndex::GETPROTOTYPEOF_STRING_INDEX, factory->NewFromCanBeCompressString("getPrototypeOf"));
    SetConstant(ConstantIndex::SETPROTOTYPEOF_STRING_INDEX, factory->NewFromCanBeCompressString("setPrototypeOf"));
    SetConstant(ConstantIndex::ISEXTENSIBLE_STRING_INDEX, factory->NewFromCanBeCompressString("isExtensible"));
    SetConstant(ConstantIndex::PREVENTEXTENSIONS_STRING_INDEX,
                factory->NewFromCanBeCompressString("preventExtensions"));
    SetConstant(ConstantIndex::GETOWNPROPERTYDESCRIPTOR_STRING_INDEX,
                factory->NewFromCanBeCompressString("getOwnPropertyDescriptor"));
    SetConstant(ConstantIndex::DEFINEPROPERTY_STRING_INDEX, factory->NewFromCanBeCompressString("defineProperty"));
    SetConstant(ConstantIndex::HAS_STRING_INDEX, factory->NewFromCanBeCompressString("has"));
    SetConstant(ConstantIndex::DELETEPROPERTY_STRING_INDEX, factory->NewFromCanBeCompressString("deleteProperty"));
    SetConstant(ConstantIndex::ENUMERATE_STRING_INDEX, factory->NewFromCanBeCompressString("enumerate"));
    SetConstant(ConstantIndex::OWNKEYS_STRING_INDEX, factory->NewFromCanBeCompressString("ownKeys"));
    SetConstant(ConstantIndex::APPLY_STRING_INDEX, factory->NewFromCanBeCompressString("apply"));
    SetConstant(ConstantIndex::NEGATIVE_ZERO_STRING_INDEX, factory->NewFromCanBeCompressString("-0"));
    SetConstant(ConstantIndex::DONE_STRING_INDEX, factory->NewFromCanBeCompressString("done"));
    SetConstant(ConstantIndex::PROXY_STRING_INDEX, factory->NewFromCanBeCompressString("proxy"));
    SetConstant(ConstantIndex::REVOKE_STRING_INDEX, factory->NewFromCanBeCompressString("revoke"));
    SetConstant(ConstantIndex::NEXT_STRING_INDEX, factory->NewFromCanBeCompressString("next"));
    SetConstant(ConstantIndex::TO_STRING_STRING_INDEX, factory->NewFromCanBeCompressString("toString"));
    SetConstant(ConstantIndex::TO_LOCALE_STRING_STRING_INDEX, factory->NewFromCanBeCompressString("toLocaleString"));
    SetConstant(ConstantIndex::VALUE_OF_STRING_INDEX, factory->NewFromCanBeCompressString("valueOf"));
    SetConstant(ConstantIndex::UNDEFINED_STRING_INDEX, factory->NewFromCanBeCompressString("undefined"));
    SetConstant(ConstantIndex::NULL_STRING_INDEX, factory->NewFromCanBeCompressString("null"));
    SetConstant(ConstantIndex::BOOLEAN_STRING_INDEX, factory->NewFromCanBeCompressString("boolean"));
    SetConstant(ConstantIndex::NUMBER_STRING_INDEX, factory->NewFromCanBeCompressString("number"));
    SetConstant(ConstantIndex::BIGINT_STRING_INDEX, factory->NewFromCanBeCompressString("bigint"));
    SetConstant(ConstantIndex::FUNCTION_STRING_INDEX, factory->NewFromCanBeCompressString("function"));
    SetConstant(ConstantIndex::STRING_STRING_INDEX, factory->NewFromCanBeCompressString("string"));
    SetConstant(ConstantIndex::SYMBOL_STRING_INDEX, factory->NewFromCanBeCompressString("symbol"));
    SetConstant(ConstantIndex::OBJECT_STRING_INDEX, factory->NewFromCanBeCompressString("object"));
    SetConstant(ConstantIndex::TRUE_STRING_INDEX, factory->NewFromCanBeCompressString("true"));
    SetConstant(ConstantIndex::FALSE_STRING_INDEX, factory->NewFromCanBeCompressString("false"));
    SetConstant(ConstantIndex::RETURN_STRING_INDEX, factory->NewFromCanBeCompressString("return"));
    SetConstant(ConstantIndex::PROXY_CONSTRUCT_STRING_INDEX, factory->NewFromCanBeCompressString("construct"));
    SetConstant(ConstantIndex::PROXY_CALL_STRING_INDEX, factory->NewFromCanBeCompressString("call"));
    SetConstant(ConstantIndex::PROMISE_THEN_STRING_INDEX, factory->NewFromCanBeCompressString("then"));
    SetConstant(ConstantIndex::PROMISE_CATCH_STRING_INDEX, factory->NewFromCanBeCompressString("catch"));
    SetConstant(ConstantIndex::SCRIPT_JOB_STRING_INDEX, factory->NewFromCanBeCompressString("ScriptJobs"));
    SetConstant(ConstantIndex::PROMISE_STRING_INDEX, factory->NewFromCanBeCompressString("PrimiseJobs"));
    SetConstant(ConstantIndex::THROWER_STRING_INDEX, factory->NewFromCanBeCompressString("Thrower"));
    SetConstant(ConstantIndex::IDENTITY_STRING_INDEX, factory->NewFromCanBeCompressString("Identity"));
    SetConstant(ConstantIndex::CALLER_STRING_INDEX, factory->NewFromCanBeCompressString("caller"));
    SetConstant(ConstantIndex::CALLEE_STRING_INDEX, factory->NewFromCanBeCompressString("callee"));
    SetConstant(ConstantIndex::INT8_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Int8Array"));
    SetConstant(ConstantIndex::UINT8_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Uint8Array"));
    SetConstant(ConstantIndex::UINT8_CLAMPED_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Uint8ClampedArray"));
    SetConstant(ConstantIndex::INT16_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Int16Array"));
    SetConstant(ConstantIndex::UINT16_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Uint16Array"));
    SetConstant(ConstantIndex::INT32_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Int32Array"));
    SetConstant(ConstantIndex::UINT32_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Uint32Array"));
    SetConstant(ConstantIndex::FLOAT32_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Float32Array"));
    SetConstant(ConstantIndex::FLOAT64_ARRAY_STRING_INDEX, factory->NewFromCanBeCompressString("Float64Array"));
    SetConstant(ConstantIndex::ASYNC_FUNCTION_STRING_INDEX, factory->NewFromCanBeCompressString("AsyncFunction"));
    SetConstant(ConstantIndex::PROMISE_RESOLVE_STRING_INDEX, factory->NewFromCanBeCompressString("resolve"));
    SetConstant(ConstantIndex::ID_STRING_INDEX, factory->NewFromCanBeCompressString("id"));
    SetConstant(ConstantIndex::METHOD_STRING_INDEX, factory->NewFromCanBeCompressString("method"));
    SetConstant(ConstantIndex::PARAMS_STRING_INDEX, factory->NewFromCanBeCompressString("params"));
    SetConstant(ConstantIndex::RESULT_STRING_INDEX, factory->NewFromCanBeCompressString("result"));
    SetConstant(ConstantIndex::TO_JSON_STRING_INDEX, factory->NewFromCanBeCompressString("toJSON"));
    SetConstant(ConstantIndex::GLOBAL_STRING_INDEX, factory->NewFromCanBeCompressString("global"));
    SetConstant(ConstantIndex::MESSAGE_STRING_INDEX, factory->NewFromCanBeCompressString("message"));
    SetConstant(ConstantIndex::ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("Error"));
    SetConstant(ConstantIndex::RANGE_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("RangeError"));
    SetConstant(ConstantIndex::REFERENCE_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("ReferenceError"));
    SetConstant(ConstantIndex::TYPE_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("TypeError"));
    SetConstant(ConstantIndex::URI_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("URIError"));
    SetConstant(ConstantIndex::SYNTAX_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("SyntaxError"));
    SetConstant(ConstantIndex::EVAL_ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("EvalError"));
    SetConstant(ConstantIndex::STACK_STRING_INDEX, factory->NewFromCanBeCompressString("stack"));
    SetConstant(ConstantIndex::STACK_EMPTY_STRING_INDEX, factory->NewFromCanBeCompressString("stackisempty"));
    SetConstant(ConstantIndex::OBJ_NOT_COERCIBLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("objectnotcoercible"));
    /* for Intl. */
    SetConstant(ConstantIndex::LANGUAGE_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("language"));
    SetConstant(ConstantIndex::SCRIPT_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("script"));
    SetConstant(ConstantIndex::REGION_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("region"));
    SetConstant(ConstantIndex::BASE_NAME_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("baseName"));
    SetConstant(ConstantIndex::CALENDAR_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("calendar"));
    SetConstant(ConstantIndex::COLLATION_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("collation"));
    SetConstant(ConstantIndex::HOUR_CYCLE_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("hourCycle"));
    SetConstant(ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("caseFirst"));
    SetConstant(ConstantIndex::NUMERIC_STRING_CLASS_INDEX, factory->NewFromCanBeCompressString("numeric"));
    SetConstant(ConstantIndex::NUMBERING_SYSTEM_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("numberingSystem"));
    SetConstant(ConstantIndex::TYPE_STRING_INDEX, factory->NewFromCanBeCompressString("type"));
    SetConstant(ConstantIndex::LOCALE_MATCHER_STRING_INDEX, factory->NewFromCanBeCompressString("localeMatcher"));
    SetConstant(ConstantIndex::FORMAT_MATCHER_STRING_INDEX, factory->NewFromCanBeCompressString("formatMatcher"));
    SetConstant(ConstantIndex::HOUR12_STRING_INDEX, factory->NewFromCanBeCompressString("hour12"));
    SetConstant(ConstantIndex::H11_STRING_INDEX, factory->NewFromCanBeCompressString("h11"));
    SetConstant(ConstantIndex::H12_STRING_INDEX, factory->NewFromCanBeCompressString("h12"));
    SetConstant(ConstantIndex::H23_STRING_INDEX, factory->NewFromCanBeCompressString("h23"));
    SetConstant(ConstantIndex::H24_STRING_INDEX, factory->NewFromCanBeCompressString("h24"));
    SetConstant(ConstantIndex::WEEK_DAY_STRING_INDEX, factory->NewFromCanBeCompressString("weekday"));
    SetConstant(ConstantIndex::ERA_STRING_INDEX, factory->NewFromCanBeCompressString("era"));
    SetConstant(ConstantIndex::YEAR_STRING_INDEX, factory->NewFromCanBeCompressString("year"));
    SetConstant(ConstantIndex::QUARTER_STRING_INDEX, factory->NewFromCanBeCompressString("quarter"));
    SetConstant(ConstantIndex::MONTH_STRING_INDEX, factory->NewFromCanBeCompressString("month"));
    SetConstant(ConstantIndex::DAY_STRING_INDEX, factory->NewFromCanBeCompressString("day"));
    SetConstant(ConstantIndex::HOUR_STRING_INDEX, factory->NewFromCanBeCompressString("hour"));
    SetConstant(ConstantIndex::MINUTE_STRING_INDEX, factory->NewFromCanBeCompressString("minute"));
    SetConstant(ConstantIndex::SECOND_STRING_INDEX, factory->NewFromCanBeCompressString("second"));
    SetConstant(ConstantIndex::YEARS_STRING_INDEX, factory->NewFromCanBeCompressString("years"));
    SetConstant(ConstantIndex::QUARTERS_STRING_INDEX, factory->NewFromCanBeCompressString("quarters"));
    SetConstant(ConstantIndex::MONTHS_STRING_INDEX, factory->NewFromCanBeCompressString("months"));
    SetConstant(ConstantIndex::DAYS_STRING_INDEX, factory->NewFromCanBeCompressString("days"));
    SetConstant(ConstantIndex::HOURS_STRING_INDEX, factory->NewFromCanBeCompressString("hours"));
    SetConstant(ConstantIndex::MINUTES_STRING_INDEX, factory->NewFromCanBeCompressString("minutes"));
    SetConstant(ConstantIndex::SECONDS_STRING_INDEX, factory->NewFromCanBeCompressString("seconds"));
    SetConstant(ConstantIndex::TIME_ZONE_NAME_STRING_INDEX, factory->NewFromCanBeCompressString("timeZoneName"));
    SetConstant(ConstantIndex::LOCALE_STRING_INDEX, factory->NewFromCanBeCompressString("locale"));
    SetConstant(ConstantIndex::TIME_ZONE_STRING_INDEX, factory->NewFromCanBeCompressString("timeZone"));
    SetConstant(ConstantIndex::LITERAL_STRING_INDEX, factory->NewFromCanBeCompressString("literal"));
    SetConstant(ConstantIndex::YEAR_NAME_STRING_INDEX, factory->NewFromCanBeCompressString("yearName"));
    SetConstant(ConstantIndex::DAY_PERIOD_STRING_INDEX, factory->NewFromCanBeCompressString("dayPeriod"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_DIGITS_STRING_INDEX,
                factory->NewFromCanBeCompressString("fractionalSecondDigits"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_STRING_INDEX, factory->NewFromCanBeCompressString("fractionalSecond"));
    SetConstant(ConstantIndex::RELATED_YEAR_STRING_INDEX, factory->NewFromCanBeCompressString("relatedYear"));
    SetConstant(ConstantIndex::LOOK_UP_STRING_INDEX, factory->NewFromCanBeCompressString("lookup"));
    SetConstant(ConstantIndex::BEST_FIT_STRING_INDEX, factory->NewFromCanBeCompressString("bestfit"));
    SetConstant(ConstantIndex::DATE_STYLE_STRING_INDEX, factory->NewFromCanBeCompressString("dateStyle"));
    SetConstant(ConstantIndex::TIME_STYLE_STRING_INDEX, factory->NewFromCanBeCompressString("timeStyle"));
    SetConstant(ConstantIndex::UTC_STRING_INDEX, factory->NewFromCanBeCompressString("UTC"));
    SetConstant(ConstantIndex::INITIALIZED_RELATIVE_INDEX, factory->NewFromCanBeCompressString("true"));
    SetConstant(ConstantIndex::WEEK_STRING_INDEX, factory->NewFromCanBeCompressString("week"));
    SetConstant(ConstantIndex::WEEKS_STRING_INDEX, factory->NewFromCanBeCompressString("weeks"));
    SetConstant(ConstantIndex::SOURCE_STRING_INDEX, factory->NewFromCanBeCompressString("source"));
    SetConstant(ConstantIndex::FORMAT_STRING_INDEX, factory->NewFromCanBeCompressString("format"));
    SetConstant(ConstantIndex::EN_US_STRING_INDEX, factory->NewFromCanBeCompressString("en-US"));
    SetConstant(ConstantIndex::UND_STRING_INDEX, factory->NewFromCanBeCompressString("und"));
    SetConstant(ConstantIndex::LATN_STRING_INDEX, factory->NewFromCanBeCompressString("latn"));
    SetConstant(ConstantIndex::STYLE_STRING_INDEX, factory->NewFromCanBeCompressString("style"));
    SetConstant(ConstantIndex::UNIT_STRING_INDEX, factory->NewFromCanBeCompressString("unit"));
    SetConstant(ConstantIndex::INTEGER_STRING_INDEX, factory->NewFromCanBeCompressString("integer"));
    SetConstant(ConstantIndex::NAN_STRING_INDEX, factory->NewFromCanBeCompressString("nan"));
    SetConstant(ConstantIndex::INFINITY_STRING_INDEX, factory->NewFromCanBeCompressString("infinity"));
    SetConstant(ConstantIndex::FRACTION_STRING_INDEX, factory->NewFromCanBeCompressString("fraction"));
    SetConstant(ConstantIndex::DECIMAL_STRING_INDEX, factory->NewFromCanBeCompressString("decimal"));
    SetConstant(ConstantIndex::GROUP_STRING_INDEX, factory->NewFromCanBeCompressString("group"));
    SetConstant(ConstantIndex::CURRENCY_STRING_INDEX, factory->NewFromCanBeCompressString("currency"));
    SetConstant(ConstantIndex::CURRENCY_SIGN_STRING_INDEX, factory->NewFromCanBeCompressString("currencySign"));
    SetConstant(ConstantIndex::CURRENCY_DISPLAY_STRING_INDEX, factory->NewFromCanBeCompressString("currencyDisplay"));
    SetConstant(ConstantIndex::PERCENT_SIGN_STRING_INDEX, factory->NewFromCanBeCompressString("percentSign"));
    SetConstant(ConstantIndex::PERCENT_STRING_INDEX, factory->NewFromCanBeCompressString("percent"));
    SetConstant(ConstantIndex::MINUS_SIGN_STRING_INDEX, factory->NewFromCanBeCompressString("minusSign"));
    SetConstant(ConstantIndex::PLUS_SIGN_STRING_INDEX, factory->NewFromCanBeCompressString("plusSign"));
    SetConstant(ConstantIndex::EXPONENT_SEPARATOR_STRING_INDEX,
                factory->NewFromCanBeCompressString("exponentSeparator"));
    SetConstant(ConstantIndex::EXPONENT_MINUS_SIGN_INDEX, factory->NewFromCanBeCompressString("exponentMinusSign"));
    SetConstant(ConstantIndex::EXPONENT_INTEGER_STRING_INDEX, factory->NewFromCanBeCompressString("exponentInteger"));
    SetConstant(ConstantIndex::LONG_STRING_INDEX, factory->NewFromCanBeCompressString("long"));
    SetConstant(ConstantIndex::SHORT_STRING_INDEX, factory->NewFromCanBeCompressString("short"));
    SetConstant(ConstantIndex::FULL_STRING_INDEX, factory->NewFromCanBeCompressString("full"));
    SetConstant(ConstantIndex::MEDIUM_STRING_INDEX, factory->NewFromCanBeCompressString("medium"));
    SetConstant(ConstantIndex::NARROW_STRING_INDEX, factory->NewFromCanBeCompressString("narrow"));
    SetConstant(ConstantIndex::ALWAYS_STRING_INDEX, factory->NewFromCanBeCompressString("always"));
    SetConstant(ConstantIndex::AUTO_STRING_INDEX, factory->NewFromCanBeCompressString("auto"));
    SetConstant(ConstantIndex::UNIT_DISPLAY_INDEX, factory->NewFromCanBeCompressString("unitDisplay"));
    SetConstant(ConstantIndex::NOTATION_INDEX, factory->NewFromCanBeCompressString("notation"));
    SetConstant(ConstantIndex::COMPACT_DISPALY_INDEX, factory->NewFromCanBeCompressString("compactDisplay"));
    SetConstant(ConstantIndex::USER_GROUPING_INDEX, factory->NewFromCanBeCompressString("useGrouping"));
    SetConstant(ConstantIndex::SIGN_DISPLAY_INDEX, factory->NewFromCanBeCompressString("signDisplay"));
    SetConstant(ConstantIndex::CODE_INDEX, factory->NewFromCanBeCompressString("code"));
    SetConstant(ConstantIndex::NARROW_SYMBOL_INDEX, factory->NewFromCanBeCompressString("narrowSymbol"));
    SetConstant(ConstantIndex::STANDARD_INDEX, factory->NewFromCanBeCompressString("standard"));
    SetConstant(ConstantIndex::ACCOUNTING_INDEX, factory->NewFromCanBeCompressString("accounting"));
    SetConstant(ConstantIndex::SCIENTIFIC_INDEX, factory->NewFromCanBeCompressString("scientific"));
    SetConstant(ConstantIndex::ENGINEERING_INDEX, factory->NewFromCanBeCompressString("engineering"));
    SetConstant(ConstantIndex::COMPACT_STRING_INDEX, factory->NewFromCanBeCompressString("compact"));
    SetConstant(ConstantIndex::NEVER_INDEX, factory->NewFromCanBeCompressString("never"));
    SetConstant(ConstantIndex::EXPECT_ZERO_INDEX, factory->NewFromCanBeCompressString("exceptZero"));
    SetConstant(ConstantIndex::MINIMUM_INTEGER_DIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumIntegerDigits"));
    SetConstant(ConstantIndex::MINIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumFractionDigits"));
    SetConstant(ConstantIndex::MAXIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromCanBeCompressString("maximumFractionDigits"));
    SetConstant(ConstantIndex::MINIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumSignificantDigits"));
    SetConstant(ConstantIndex::MAXIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromCanBeCompressString("maximumSignificantDigits"));
    SetConstant(ConstantIndex::INVALID_DATE_INDEX, factory->NewFromCanBeCompressString("Invalid Date"));
    SetConstant(ConstantIndex::USAGE_INDEX, factory->NewFromCanBeCompressString("usage"));
    SetConstant(ConstantIndex::COMPARE_INDEX, factory->NewFromCanBeCompressString("compare"));
    SetConstant(ConstantIndex::SENSITIVITY_INDEX, factory->NewFromCanBeCompressString("sensitivity"));
    SetConstant(ConstantIndex::IGNORE_PUNCTUATION_INDEX, factory->NewFromCanBeCompressString("ignorePunctuation"));
    SetConstant(ConstantIndex::CARDINAL_INDEX, factory->NewFromCanBeCompressString("cardinal"));
    SetConstant(ConstantIndex::ORDINAL_INDEX, factory->NewFromCanBeCompressString("ordinal"));
    SetConstant(ConstantIndex::EXEC_INDEX, factory->NewFromCanBeCompressString("exec"));
    SetConstant(ConstantIndex::LAST_INDEX_INDEX, factory->NewFromCanBeCompressString("lastIndex"));
    SetConstant(ConstantIndex::PLURAL_CATEGORIES_INDEX, factory->NewFromCanBeCompressString("pluralCategories"));
    SetConstant(ConstantIndex::SORT_INDEX, factory->NewFromCanBeCompressString("sort"));
    SetConstant(ConstantIndex::SEARCH_INDEX, factory->NewFromCanBeCompressString("search"));
    SetConstant(ConstantIndex::BASE_INDEX, factory->NewFromCanBeCompressString("base"));
    SetConstant(ConstantIndex::ACCENT_INDEX, factory->NewFromCanBeCompressString("accent"));
    SetConstant(ConstantIndex::CASE_INDEX, factory->NewFromCanBeCompressString("case"));
    SetConstant(ConstantIndex::VARIANT_INDEX, factory->NewFromCanBeCompressString("variant"));
    SetConstant(ConstantIndex::EN_US_POSIX_STRING_INDEX, factory->NewFromCanBeCompressString("en-US-POSIX"));
    SetConstant(ConstantIndex::UPPER_INDEX, factory->NewFromCanBeCompressString("upper"));
    SetConstant(ConstantIndex::LOWER_INDEX, factory->NewFromCanBeCompressString("lower"));
    SetConstant(ConstantIndex::DEFAULT_INDEX, factory->NewFromCanBeCompressString("default"));
    SetConstant(ConstantIndex::SHARED_INDEX, factory->NewFromCanBeCompressString("shared"));
    SetConstant(ConstantIndex::START_RANGE_INDEX, factory->NewFromCanBeCompressString("startRange"));
    SetConstant(ConstantIndex::END_RANGE_INDEX, factory->NewFromCanBeCompressString("endRange"));
    SetConstant(ConstantIndex::ISO8601_INDEX, factory->NewFromCanBeCompressString("iso8601"));
    SetConstant(ConstantIndex::GREGORY_INDEX, factory->NewFromCanBeCompressString("gregory"));
    SetConstant(ConstantIndex::ETHIOAA_INDEX, factory->NewFromCanBeCompressString("ethioaa"));
    SetConstant(ConstantIndex::STICKY_INDEX, factory->NewFromCanBeCompressString("sticky"));
    SetConstant(ConstantIndex::U_INDEX, factory->NewFromCanBeCompressString("u"));
    SetConstant(ConstantIndex::INDEX_INDEX, factory->NewFromCanBeCompressString("index"));
    SetConstant(ConstantIndex::INPUT_INDEX, factory->NewFromCanBeCompressString("input"));
    SetConstant(ConstantIndex::UNICODE_INDEX, factory->NewFromCanBeCompressString("unicode"));
    SetConstant(ConstantIndex::ZERO_INDEX, factory->NewFromCanBeCompressString("0"));
    SetConstant(ConstantIndex::VALUES_INDEX, factory->NewFromCanBeCompressString("values"));
    SetConstant(ConstantIndex::AMBIGUOUS_INDEX, factory->NewFromCanBeCompressString("ambiguous"));
    SetConstant(ConstantIndex::MODULE_INDEX, factory->NewFromCanBeCompressString("module"));
    SetConstant(ConstantIndex::STAR_INDEX, factory->NewFromCanBeCompressString("*"));

    auto accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSFunction::PrototypeSetter),
                                                 reinterpret_cast<void *>(JSFunction::PrototypeGetter));
    SetConstant(ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR, accessor);
    accessor = factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::NameGetter));
    SetConstant(ConstantIndex::FUNCTION_NAME_ACCESSOR, accessor);
    accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSArray::LengthSetter),
                                            reinterpret_cast<void *>(JSArray::LengthGetter));
    SetConstant(ConstantIndex::ARRAY_LENGTH_ACCESSOR, accessor);
}

void GlobalEnvConstants::InitGlobalUndefined()
{
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
}
}  // namespace panda::ecmascript
