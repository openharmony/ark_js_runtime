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

#include "ecma_vm.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/builtins.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
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
#include "ecmascript/object_factory.h"


namespace panda::ecmascript {
void GlobalEnvConstants::InitRootsClass([[maybe_unused]] JSThread *thread, JSHClass *dynClassClass)
{
    // Global constants are readonly.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    SetConstant(ConstantIndex::HCLASS_CLASS_INDEX, JSTaggedValue(dynClassClass));
    SetConstant(ConstantIndex::STRING_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::STRING).GetTaggedValue());
    SetConstant(ConstantIndex::ARRAY_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_ARRAY).GetTaggedValue());
    SetConstant(ConstantIndex::DICTIONARY_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_DICTIONARY).GetTaggedValue());
    SetConstant(
        ConstantIndex::JS_NATIVE_POINTER_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, sizeof(JSNativePointer), JSType::JS_NATIVE_POINTER).GetTaggedValue());
    SetConstant(ConstantIndex::ENV_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::TAGGED_ARRAY).GetTaggedValue());
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_NONE_FIELD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, FreeObject::NEXT_OFFSET, JSType::FREE_OBJECT_WITH_NONE_FIELD)
                    .GetTaggedValue());
    SetConstant(ConstantIndex::FREE_OBJECT_WITH_ONE_FIELD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, FreeObject::SIZE_OFFSET, JSType::FREE_OBJECT_WITH_ONE_FIELD)
                    .GetTaggedValue());
    SetConstant(
        ConstantIndex::FREE_OBJECT_WITH_TWO_FIELD_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, FreeObject::SIZE, JSType::FREE_OBJECT_WITH_TWO_FIELD).GetTaggedValue());
    SetConstant(ConstantIndex::SYMBOL_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSSymbol::SIZE, JSType::SYMBOL).GetTaggedValue());
    SetConstant(ConstantIndex::ACCESSOE_DATA_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, AccessorData::SIZE, JSType::ACCESSOR_DATA).GetTaggedValue());
    SetConstant(
        ConstantIndex::INTERNAL_ACCESSOR_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, AccessorData::SIZE, JSType::INTERNAL_ACCESSOR).GetTaggedValue());
    SetConstant(ConstantIndex::JS_PROXY_ORDINARY_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY).GetTaggedValue());
    SetConstant(ConstantIndex::OBJECT_WRAPPER_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, ObjectWrapper::SIZE, JSType::OBJECT_WRAPPER).GetTaggedValue());
    SetConstant(
        ConstantIndex::COMPLETION_RECORD_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, CompletionRecord::SIZE, JSType::COMPLETION_RECORD).GetTaggedValue());
    SetConstant(
        ConstantIndex::GENERATOR_CONTEST_INDEX,
        factory->NewEcmaDynClass(dynClassClass, GeneratorContext::SIZE, JSType::JS_GENERATOR_CONTEXT).GetTaggedValue());
    SetConstant(
        ConstantIndex::CAPABILITY_RECORD_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, PromiseCapability::SIZE, JSType::PROMISE_CAPABILITY).GetTaggedValue());
    SetConstant(
        ConstantIndex::REACTIONS_RECORD_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, PromiseReaction::SIZE, JSType::PROMISE_REACTIONS).GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_ITERATOR_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseIteratorRecord::SIZE, JSType::PROMISE_ITERATOR_RECORD)
                    .GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_RECORD_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PromiseRecord::SIZE, JSType::PROMISE_RECORD).GetTaggedValue());
    SetConstant(
        ConstantIndex::PROMISE_RESOLVING_FUNCTIONS_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ResolvingFunctionsRecord::SIZE, JSType::RESOLVING_FUNCTIONS_RECORD)
            .GetTaggedValue());
    SetConstant(
        ConstantIndex::MICRO_JOB_QUEUE_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, job::MicroJobQueue::SIZE, JSType::MICRO_JOB_QUEUE).GetTaggedValue());
    SetConstant(ConstantIndex::PENDING_JOB_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, job::PendingJob::SIZE, JSType::PENDING_JOB).GetTaggedValue());
    SetConstant(
        ConstantIndex::PROTO_CHANGE_MARKER_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ProtoChangeMarker::SIZE, JSType::PROTO_CHANGE_MARKER).GetTaggedValue());
    SetConstant(
        ConstantIndex::PROTO_CHANGE_DETAILS_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, ProtoChangeDetails::SIZE, JSType::PROTOTYPE_INFO).GetTaggedValue());
    SetConstant(
        ConstantIndex::PROTOTYPE_HANDLER_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, PrototypeHandler::SIZE, JSType::PROTOTYPE_HANDLER).GetTaggedValue());
    SetConstant(
        ConstantIndex::TRANSITION_HANDLER_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, TransitionHandler::SIZE, JSType::TRANSITION_HANDLER).GetTaggedValue());
    SetConstant(ConstantIndex::PROPERTY_BOX_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, PropertyBox::SIZE, JSType::PROPERTY_BOX).GetTaggedValue());
    SetConstant(ConstantIndex::FUNCTION_EXTRA_INFO_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSFunctionExtraInfo::SIZE, JSType::FUNCTION_EXTRA_INFO)
                    .GetTaggedValue());
    SetConstant(ConstantIndex::PROGRAM_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, Program::SIZE, JSType::PROGRAM).GetTaggedValue());
    SetConstant(ConstantIndex::ECMA_MODULE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, EcmaModule::SIZE, JSType::ECMA_MODULE).GetTaggedValue());

    JSHClass *jsProxyCallableClass = *factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyCallableClass->SetCallable(true);
    SetConstant(ConstantIndex::JS_PROXY_CALLABLE_CLASS_INDEX, JSTaggedValue(jsProxyCallableClass));

    JSHClass *jsProxyConstructClass = *factory->NewEcmaDynClass(dynClassClass, JSProxy::SIZE, JSType::JS_PROXY);

    jsProxyConstructClass->SetCallable(true);
    jsProxyConstructClass->SetConstructor(true);
    SetConstant(ConstantIndex::JS_PROXY_CONSTRUCT_CLASS_INDEX, JSTaggedValue(jsProxyConstructClass));

    SetConstant(ConstantIndex::JS_REALM_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSRealm::SIZE, JSType::JS_REALM).GetTaggedValue());
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
    SetConstant(ConstantIndex::CONSTRUCTOR_STRING_INDEX, factory->NewFromString("constructor").GetTaggedValue());
    SetConstant(ConstantIndex::PROTOTYPE_STRING_INDEX, factory->NewFromString("prototype").GetTaggedValue());
    SetConstant(ConstantIndex::LENGTH_STRING_INDEX, factory->NewFromString("length").GetTaggedValue());
    SetConstant(ConstantIndex::VALUE_STRING_INDEX, factory->NewFromString("value").GetTaggedValue());
    SetConstant(ConstantIndex::SET_STRING_INDEX, factory->NewFromString("set").GetTaggedValue());
    SetConstant(ConstantIndex::GET_STRING_INDEX, factory->NewFromString("get").GetTaggedValue());
    SetConstant(ConstantIndex::WRITABLE_STRING_INDEX, factory->NewFromString("writable").GetTaggedValue());
    SetConstant(ConstantIndex::ENUMERABLE_STRING_INDEX, factory->NewFromString("enumerable").GetTaggedValue());
    SetConstant(ConstantIndex::CONFIGURABLE_STRING_INDEX, factory->NewFromString("configurable").GetTaggedValue());
    /* SymbolTable *RegisterSymbols */
    SetConstant(ConstantIndex::NAME_STRING_INDEX, factory->NewFromString("name").GetTaggedValue());
    SetConstant(ConstantIndex::GETPROTOTYPEOF_STRING_INDEX, factory->NewFromString("getPrototypeOf").GetTaggedValue());
    SetConstant(ConstantIndex::SETPROTOTYPEOF_STRING_INDEX, factory->NewFromString("setPrototypeOf").GetTaggedValue());
    SetConstant(ConstantIndex::ISEXTENSIBLE_STRING_INDEX, factory->NewFromString("isExtensible").GetTaggedValue());
    SetConstant(ConstantIndex::PREVENTEXTENSIONS_STRING_INDEX,
                factory->NewFromString("preventExtensions").GetTaggedValue());
    SetConstant(ConstantIndex::GETOWNPROPERTYDESCRIPTOR_STRING_INDEX,
                factory->NewFromString("getOwnPropertyDescriptor").GetTaggedValue());
    SetConstant(ConstantIndex::DEFINEPROPERTY_STRING_INDEX, factory->NewFromString("defineProperty").GetTaggedValue());
    SetConstant(ConstantIndex::HAS_STRING_INDEX, factory->NewFromString("has").GetTaggedValue());
    SetConstant(ConstantIndex::DELETEPROPERTY_STRING_INDEX, factory->NewFromString("deleteProperty").GetTaggedValue());
    SetConstant(ConstantIndex::ENUMERATE_STRING_INDEX, factory->NewFromString("enumerate").GetTaggedValue());
    SetConstant(ConstantIndex::OWNKEYS_STRING_INDEX, factory->NewFromString("ownKeys").GetTaggedValue());
    SetConstant(ConstantIndex::APPLY_STRING_INDEX, factory->NewFromString("apply").GetTaggedValue());
    SetConstant(ConstantIndex::NEGATIVE_ZERO_STRING_INDEX, factory->NewFromString("-0").GetTaggedValue());
    SetConstant(ConstantIndex::DONE_STRING_INDEX, factory->NewFromString("done").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_STRING_INDEX, factory->NewFromString("proxy").GetTaggedValue());
    SetConstant(ConstantIndex::REVOKE_STRING_INDEX, factory->NewFromString("revoke").GetTaggedValue());
    SetConstant(ConstantIndex::NEXT_STRING_INDEX, factory->NewFromString("next").GetTaggedValue());
    SetConstant(ConstantIndex::TO_STRING_STRING_INDEX, factory->NewFromString("toString").GetTaggedValue());
    SetConstant(ConstantIndex::TO_LOCALE_STRING_STRING_INDEX,
                factory->NewFromString("toLocaleString").GetTaggedValue());
    SetConstant(ConstantIndex::VALUE_OF_STRING_INDEX, factory->NewFromString("valueOf").GetTaggedValue());
    SetConstant(ConstantIndex::UNDEFINED_STRING_INDEX, factory->NewFromString("undefined").GetTaggedValue());
    SetConstant(ConstantIndex::NULL_STRING_INDEX, factory->NewFromString("null").GetTaggedValue());
    SetConstant(ConstantIndex::BOOLEAN_STRING_INDEX, factory->NewFromString("boolean").GetTaggedValue());
    SetConstant(ConstantIndex::NUMBER_STRING_INDEX, factory->NewFromString("number").GetTaggedValue());
    SetConstant(ConstantIndex::FUNCTION_STRING_INDEX, factory->NewFromString("function").GetTaggedValue());
    SetConstant(ConstantIndex::STRING_STRING_INDEX, factory->NewFromString("string").GetTaggedValue());
    SetConstant(ConstantIndex::SYMBOL_STRING_INDEX, factory->NewFromString("symbol").GetTaggedValue());
    SetConstant(ConstantIndex::OBJECT_STRING_INDEX, factory->NewFromString("object").GetTaggedValue());
    SetConstant(ConstantIndex::TRUE_STRING_INDEX, factory->NewFromString("true").GetTaggedValue());
    SetConstant(ConstantIndex::FALSE_STRING_INDEX, factory->NewFromString("false").GetTaggedValue());
    SetConstant(ConstantIndex::RETURN_STRING_INDEX, factory->NewFromString("return").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_CONSTRUCT_STRING_INDEX, factory->NewFromString("construct").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_CALL_STRING_INDEX, factory->NewFromString("call").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_THEN_STRING_INDEX, factory->NewFromString("then").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_CATCH_STRING_INDEX, factory->NewFromString("catch").GetTaggedValue());
    SetConstant(ConstantIndex::SCRIPT_JOB_STRING_INDEX, factory->NewFromString("ScriptJobs").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_STRING_INDEX, factory->NewFromString("PrimiseJobs").GetTaggedValue());
    SetConstant(ConstantIndex::THROWER_STRING_INDEX, factory->NewFromString("Thrower").GetTaggedValue());
    SetConstant(ConstantIndex::IDENTITY_STRING_INDEX, factory->NewFromString("Identity").GetTaggedValue());
    SetConstant(ConstantIndex::CALLER_STRING_INDEX, factory->NewFromString("caller").GetTaggedValue());
    SetConstant(ConstantIndex::CALLEE_STRING_INDEX, factory->NewFromString("callee").GetTaggedValue());
    SetConstant(ConstantIndex::INT8_ARRAY_STRING_INDEX, factory->NewFromString("Int8Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT8_ARRAY_STRING_INDEX, factory->NewFromString("Uint8Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT8_CLAMPED_ARRAY_STRING_INDEX,
                factory->NewFromString("Uint8ClampedArray").GetTaggedValue());
    SetConstant(ConstantIndex::INT16_ARRAY_STRING_INDEX, factory->NewFromString("Int16Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT16_ARRAY_STRING_INDEX, factory->NewFromString("Uint16Array").GetTaggedValue());
    SetConstant(ConstantIndex::INT32_ARRAY_STRING_INDEX, factory->NewFromString("Int32Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT32_ARRAY_STRING_INDEX, factory->NewFromString("Uint32Array").GetTaggedValue());
    SetConstant(ConstantIndex::FLOAT32_ARRAY_STRING_INDEX, factory->NewFromString("Float32Array").GetTaggedValue());
    SetConstant(ConstantIndex::FLOAT64_ARRAY_STRING_INDEX, factory->NewFromString("Float64Array").GetTaggedValue());
    SetConstant(ConstantIndex::ASYNC_FUNCTION_STRING_INDEX, factory->NewFromString("AsyncFunction").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_RESOLVE_STRING_INDEX, factory->NewFromString("resolve").GetTaggedValue());
    SetConstant(ConstantIndex::ID_STRING_INDEX, factory->NewFromString("id").GetTaggedValue());
    SetConstant(ConstantIndex::METHOD_STRING_INDEX, factory->NewFromString("method").GetTaggedValue());
    SetConstant(ConstantIndex::PARAMS_STRING_INDEX, factory->NewFromString("params").GetTaggedValue());
    SetConstant(ConstantIndex::RESULT_STRING_INDEX, factory->NewFromString("result").GetTaggedValue());
    SetConstant(ConstantIndex::TO_JSON_STRING_INDEX, factory->NewFromString("toJSON").GetTaggedValue());
    SetConstant(ConstantIndex::GLOBAL_STRING_INDEX, factory->NewFromString("global").GetTaggedValue());
    SetConstant(ConstantIndex::MESSAGE_STRING_INDEX, factory->NewFromString("message").GetTaggedValue());
    SetConstant(ConstantIndex::ERROR_STRING_INDEX, factory->NewFromString("Error").GetTaggedValue());
    SetConstant(ConstantIndex::RANGE_ERROR_STRING_INDEX, factory->NewFromString("RangeError").GetTaggedValue());
    SetConstant(ConstantIndex::REFERENCE_ERROR_STRING_INDEX, factory->NewFromString("ReferenceError").GetTaggedValue());
    SetConstant(ConstantIndex::TYPE_ERROR_STRING_INDEX, factory->NewFromString("TypeError").GetTaggedValue());
    SetConstant(ConstantIndex::URI_ERROR_STRING_INDEX, factory->NewFromString("URIError").GetTaggedValue());
    SetConstant(ConstantIndex::SYNTAX_ERROR_STRING_INDEX, factory->NewFromString("SyntaxError").GetTaggedValue());
    SetConstant(ConstantIndex::EVAL_ERROR_STRING_INDEX, factory->NewFromString("EvalError").GetTaggedValue());
    SetConstant(ConstantIndex::STACK_STRING_INDEX, factory->NewFromString("stack").GetTaggedValue());
    SetConstant(ConstantIndex::STACK_EMPTY_STRING_INDEX, factory->NewFromString("stackisempty").GetTaggedValue());
    SetConstant(ConstantIndex::OBJ_NOT_COERCIBLE_STRING_INDEX,
                factory->NewFromString("objectnotcoercible").GetTaggedValue());
    /* for Intl. */
    SetConstant(ConstantIndex::LANGUAGE_STRING_CLASS_INDEX, factory->NewFromString("language").GetTaggedValue());
    SetConstant(ConstantIndex::SCRIPT_STRING_CLASS_INDEX, factory->NewFromString("script").GetTaggedValue());
    SetConstant(ConstantIndex::REGION_STRING_CLASS_INDEX, factory->NewFromString("region").GetTaggedValue());
    SetConstant(ConstantIndex::BASE_NAME_STRING_CLASS_INDEX, factory->NewFromString("baseName").GetTaggedValue());
    SetConstant(ConstantIndex::CALENDAR_STRING_CLASS_INDEX, factory->NewFromString("calendar").GetTaggedValue());
    SetConstant(ConstantIndex::COLLATION_STRING_CLASS_INDEX, factory->NewFromString("collation").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR_CYCLE_STRING_CLASS_INDEX, factory->NewFromString("hourCycle").GetTaggedValue());
    SetConstant(ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX, factory->NewFromString("caseFirst").GetTaggedValue());
    SetConstant(ConstantIndex::NUMERIC_STRING_CLASS_INDEX, factory->NewFromString("numeric").GetTaggedValue());
    SetConstant(ConstantIndex::NUMBERING_SYSTEM_STRING_CLASS_INDEX,
                factory->NewFromString("numberingSystem").GetTaggedValue());
    SetConstant(ConstantIndex::TYPE_STRING_INDEX, factory->NewFromString("type").GetTaggedValue());
    SetConstant(ConstantIndex::LOCALE_MATCHER_STRING_INDEX, factory->NewFromString("localeMatcher").GetTaggedValue());
    SetConstant(ConstantIndex::FORMAT_MATCHER_STRING_INDEX, factory->NewFromString("formatMatcher").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR12_STRING_INDEX, factory->NewFromString("hour12").GetTaggedValue());
    SetConstant(ConstantIndex::H11_STRING_INDEX, factory->NewFromString("h11").GetTaggedValue());
    SetConstant(ConstantIndex::H12_STRING_INDEX, factory->NewFromString("h12").GetTaggedValue());
    SetConstant(ConstantIndex::H23_STRING_INDEX, factory->NewFromString("h23").GetTaggedValue());
    SetConstant(ConstantIndex::H24_STRING_INDEX, factory->NewFromString("h24").GetTaggedValue());
    SetConstant(ConstantIndex::WEEK_DAY_STRING_INDEX, factory->NewFromString("weekday").GetTaggedValue());
    SetConstant(ConstantIndex::ERA_STRING_INDEX, factory->NewFromString("era").GetTaggedValue());
    SetConstant(ConstantIndex::YEAR_STRING_INDEX, factory->NewFromString("year").GetTaggedValue());
    SetConstant(ConstantIndex::QUARTER_STRING_INDEX, factory->NewFromString("quarter").GetTaggedValue());
    SetConstant(ConstantIndex::MONTH_STRING_INDEX, factory->NewFromString("month").GetTaggedValue());
    SetConstant(ConstantIndex::DAY_STRING_INDEX, factory->NewFromString("day").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR_STRING_INDEX, factory->NewFromString("hour").GetTaggedValue());
    SetConstant(ConstantIndex::MINUTE_STRING_INDEX, factory->NewFromString("minute").GetTaggedValue());
    SetConstant(ConstantIndex::SECOND_STRING_INDEX, factory->NewFromString("second").GetTaggedValue());
    SetConstant(ConstantIndex::YEARS_STRING_INDEX, factory->NewFromString("years").GetTaggedValue());
    SetConstant(ConstantIndex::QUARTERS_STRING_INDEX, factory->NewFromString("quarters").GetTaggedValue());
    SetConstant(ConstantIndex::MONTHS_STRING_INDEX, factory->NewFromString("months").GetTaggedValue());
    SetConstant(ConstantIndex::DAYS_STRING_INDEX, factory->NewFromString("days").GetTaggedValue());
    SetConstant(ConstantIndex::HOURS_STRING_INDEX, factory->NewFromString("hours").GetTaggedValue());
    SetConstant(ConstantIndex::MINUTES_STRING_INDEX, factory->NewFromString("minutes").GetTaggedValue());
    SetConstant(ConstantIndex::SECONDS_STRING_INDEX, factory->NewFromString("seconds").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_ZONE_NAME_STRING_INDEX, factory->NewFromString("timeZoneName").GetTaggedValue());
    SetConstant(ConstantIndex::LOCALE_STRING_INDEX, factory->NewFromString("locale").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_ZONE_STRING_INDEX, factory->NewFromString("timeZone").GetTaggedValue());
    SetConstant(ConstantIndex::LITERAL_STRING_INDEX, factory->NewFromString("literal").GetTaggedValue());
    SetConstant(ConstantIndex::YEAR_NAME_STRING_INDEX, factory->NewFromString("yearName").GetTaggedValue());
    SetConstant(ConstantIndex::DAY_PERIOD_STRING_INDEX, factory->NewFromString("dayPeriod").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_DIGITS_STRING_INDEX,
                factory->NewFromString("fractionalSecondDigits").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_STRING_INDEX,
                factory->NewFromString("fractionalSecond").GetTaggedValue());
    SetConstant(ConstantIndex::RELATED_YEAR_STRING_INDEX, factory->NewFromString("relatedYear").GetTaggedValue());
    SetConstant(ConstantIndex::LOOK_UP_STRING_INDEX, factory->NewFromString("lookup").GetTaggedValue());
    SetConstant(ConstantIndex::BEST_FIT_STRING_INDEX, factory->NewFromString("bestfit").GetTaggedValue());
    SetConstant(ConstantIndex::DATE_STYLE_STRING_INDEX, factory->NewFromString("dateStyle").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_STYLE_STRING_INDEX, factory->NewFromString("timeStyle").GetTaggedValue());
    SetConstant(ConstantIndex::UTC_STRING_INDEX, factory->NewFromString("UTC").GetTaggedValue());
    SetConstant(ConstantIndex::INITIALIZED_RELATIVE_INDEX, factory->NewFromString("true").GetTaggedValue());
    SetConstant(ConstantIndex::WEEK_STRING_INDEX, factory->NewFromString("week").GetTaggedValue());
    SetConstant(ConstantIndex::WEEKS_STRING_INDEX, factory->NewFromString("weeks").GetTaggedValue());
    SetConstant(ConstantIndex::SOURCE_STRING_INDEX, factory->NewFromString("source").GetTaggedValue());
    SetConstant(ConstantIndex::FORMAT_STRING_INDEX, factory->NewFromString("format").GetTaggedValue());
    SetConstant(ConstantIndex::EN_US_STRING_INDEX, factory->NewFromString("en-US").GetTaggedValue());
    SetConstant(ConstantIndex::UND_STRING_INDEX, factory->NewFromString("und").GetTaggedValue());
    SetConstant(ConstantIndex::LATN_STRING_INDEX, factory->NewFromString("latn").GetTaggedValue());
    SetConstant(ConstantIndex::STYLE_STRING_INDEX, factory->NewFromString("style").GetTaggedValue());
    SetConstant(ConstantIndex::UNIT_STRING_INDEX, factory->NewFromString("unit").GetTaggedValue());
    SetConstant(ConstantIndex::INTEGER_STRING_INDEX, factory->NewFromString("integer").GetTaggedValue());
    SetConstant(ConstantIndex::NAN_STRING_INDEX, factory->NewFromString("nan").GetTaggedValue());
    SetConstant(ConstantIndex::INFINITY_STRING_INDEX, factory->NewFromString("infinity").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTION_STRING_INDEX, factory->NewFromString("fraction").GetTaggedValue());
    SetConstant(ConstantIndex::DECIMAL_STRING_INDEX, factory->NewFromString("decimal").GetTaggedValue());
    SetConstant(ConstantIndex::GROUP_STRING_INDEX, factory->NewFromString("group").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_STRING_INDEX, factory->NewFromString("currency").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_SIGN_STRING_INDEX, factory->NewFromString("currencySign").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_DISPLAY_STRING_INDEX,
                factory->NewFromString("currencyDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::PERCENT_SIGN_STRING_INDEX, factory->NewFromString("percentSign").GetTaggedValue());
    SetConstant(ConstantIndex::PERCENT_STRING_INDEX, factory->NewFromString("percent").GetTaggedValue());
    SetConstant(ConstantIndex::MINUS_SIGN_STRING_INDEX, factory->NewFromString("minusSign").GetTaggedValue());
    SetConstant(ConstantIndex::PLUS_SIGN_STRING_INDEX, factory->NewFromString("plusSign").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_SEPARATOR_STRING_INDEX,
                factory->NewFromString("exponentSeparator").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_MINUS_SIGN_INDEX, factory->NewFromString("exponentMinusSign").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_INTEGER_STRING_INDEX,
                factory->NewFromString("exponentInteger").GetTaggedValue());
    SetConstant(ConstantIndex::LONG_STRING_INDEX, factory->NewFromString("long").GetTaggedValue());
    SetConstant(ConstantIndex::SHORT_STRING_INDEX, factory->NewFromString("short").GetTaggedValue());
    SetConstant(ConstantIndex::FULL_STRING_INDEX, factory->NewFromString("full").GetTaggedValue());
    SetConstant(ConstantIndex::MEDIUM_STRING_INDEX, factory->NewFromString("medium").GetTaggedValue());
    SetConstant(ConstantIndex::NARROW_STRING_INDEX, factory->NewFromString("narrow").GetTaggedValue());
    SetConstant(ConstantIndex::ALWAYS_STRING_INDEX, factory->NewFromString("always").GetTaggedValue());
    SetConstant(ConstantIndex::AUTO_STRING_INDEX, factory->NewFromString("auto").GetTaggedValue());
    SetConstant(ConstantIndex::UNIT_DISPLAY_INDEX, factory->NewFromString("unitDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::NOTATION_INDEX, factory->NewFromString("notation").GetTaggedValue());
    SetConstant(ConstantIndex::COMPACT_DISPALY_INDEX, factory->NewFromString("compactDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::USER_GROUPING_INDEX, factory->NewFromString("useGrouping").GetTaggedValue());
    SetConstant(ConstantIndex::SIGN_DISPLAY_INDEX, factory->NewFromString("signDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::CODE_INDEX, factory->NewFromString("code").GetTaggedValue());
    SetConstant(ConstantIndex::NARROW_SYMBOL_INDEX, factory->NewFromString("narrowSymbol").GetTaggedValue());
    SetConstant(ConstantIndex::STANDARD_INDEX, factory->NewFromString("standard").GetTaggedValue());
    SetConstant(ConstantIndex::ACCOUNTING_INDEX, factory->NewFromString("accounting").GetTaggedValue());
    SetConstant(ConstantIndex::SCIENTIFIC_INDEX, factory->NewFromString("scientific").GetTaggedValue());
    SetConstant(ConstantIndex::ENGINEERING_INDEX, factory->NewFromString("engineering").GetTaggedValue());
    SetConstant(ConstantIndex::COMPACT_STRING_INDEX, factory->NewFromString("compact").GetTaggedValue());
    SetConstant(ConstantIndex::NEVER_INDEX, factory->NewFromString("never").GetTaggedValue());
    SetConstant(ConstantIndex::EXPECT_ZERO_INDEX, factory->NewFromString("exceptZero").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_INTEGER_DIGITS_INDEX,
                factory->NewFromString("minimumIntegerDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromString("minimumFractionDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MAXIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromString("maximumFractionDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromString("minimumSignificantDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MAXIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromString("maximumSignificantDigits").GetTaggedValue());
    SetConstant(ConstantIndex::INVALID_DATE_INDEX, factory->NewFromString("InvalidDate").GetTaggedValue());
    SetConstant(ConstantIndex::USAGE_INDEX, factory->NewFromString("usage").GetTaggedValue());
    SetConstant(ConstantIndex::COMPARE_INDEX, factory->NewFromString("compare").GetTaggedValue());
    SetConstant(ConstantIndex::SENSITIVITY_INDEX, factory->NewFromString("sensitivity").GetTaggedValue());
    SetConstant(ConstantIndex::IGNORE_PUNCTUATION_INDEX, factory->NewFromString("ignorePunctuation").GetTaggedValue());
    SetConstant(ConstantIndex::CARDINAL_INDEX, factory->NewFromString("cardinal").GetTaggedValue());
    SetConstant(ConstantIndex::ORDINAL_INDEX, factory->NewFromString("ordinal").GetTaggedValue());
    SetConstant(ConstantIndex::EXEC_INDEX, factory->NewFromString("exec").GetTaggedValue());
    SetConstant(ConstantIndex::LAST_INDEX_INDEX, factory->NewFromString("lastIndex").GetTaggedValue());

    auto accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSFunction::PrototypeSetter),
                                                 reinterpret_cast<void *>(JSFunction::PrototypeGetter));
    SetConstant(ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR, accessor.GetTaggedValue());
    accessor = factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::NameGetter));
    SetConstant(ConstantIndex::FUNCTION_NAME_ACCESSOR, accessor.GetTaggedValue());
    accessor = factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::LengthGetter));
    SetConstant(ConstantIndex::FUNCTION_LENGTH_ACCESSOR, accessor.GetTaggedValue());
    accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSArray::LengthSetter),
                                            reinterpret_cast<void *>(JSArray::LengthGetter));
    SetConstant(ConstantIndex::ARRAY_LENGTH_ACCESSOR, accessor.GetTaggedValue());
}

void GlobalEnvConstants::InitGlobalUndefined()
{
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
}
}  // namespace panda::ecmascript
