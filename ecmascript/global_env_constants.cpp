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
#include "ecmascript/class_linker/program_object.h"
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_vm.h"
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
    SetConstant(ConstantIndex::MACHINE_CODE_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, 0, JSType::MACHINE_CODE_OBJECT).GetTaggedValue());
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
    SetConstant(ConstantIndex::CONSTRUCTOR_STRING_INDEX,
                factory->NewFromCanBeCompressString("constructor").GetTaggedValue());
    SetConstant(ConstantIndex::PROTOTYPE_STRING_INDEX,
                factory->NewFromCanBeCompressString("prototype").GetTaggedValue());
    SetConstant(ConstantIndex::LENGTH_STRING_INDEX, factory->NewFromCanBeCompressString("length").GetTaggedValue());
    SetConstant(ConstantIndex::VALUE_STRING_INDEX, factory->NewFromCanBeCompressString("value").GetTaggedValue());
    SetConstant(ConstantIndex::SET_STRING_INDEX, factory->NewFromCanBeCompressString("set").GetTaggedValue());
    SetConstant(ConstantIndex::GET_STRING_INDEX, factory->NewFromCanBeCompressString("get").GetTaggedValue());
    SetConstant(ConstantIndex::WRITABLE_STRING_INDEX, factory->NewFromCanBeCompressString("writable").GetTaggedValue());
    SetConstant(ConstantIndex::ENUMERABLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("enumerable").GetTaggedValue());
    SetConstant(ConstantIndex::CONFIGURABLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("configurable").GetTaggedValue());
    /* SymbolTable *RegisterSymbols */
    SetConstant(ConstantIndex::NAME_STRING_INDEX, factory->NewFromCanBeCompressString("name").GetTaggedValue());
    SetConstant(ConstantIndex::GETPROTOTYPEOF_STRING_INDEX,
                factory->NewFromCanBeCompressString("getPrototypeOf").GetTaggedValue());
    SetConstant(ConstantIndex::SETPROTOTYPEOF_STRING_INDEX,
                factory->NewFromCanBeCompressString("setPrototypeOf").GetTaggedValue());
    SetConstant(ConstantIndex::ISEXTENSIBLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("isExtensible").GetTaggedValue());
    SetConstant(ConstantIndex::PREVENTEXTENSIONS_STRING_INDEX,
                factory->NewFromCanBeCompressString("preventExtensions").GetTaggedValue());
    SetConstant(ConstantIndex::GETOWNPROPERTYDESCRIPTOR_STRING_INDEX,
                factory->NewFromCanBeCompressString("getOwnPropertyDescriptor").GetTaggedValue());
    SetConstant(ConstantIndex::DEFINEPROPERTY_STRING_INDEX,
                factory->NewFromCanBeCompressString("defineProperty").GetTaggedValue());
    SetConstant(ConstantIndex::HAS_STRING_INDEX, factory->NewFromCanBeCompressString("has").GetTaggedValue());
    SetConstant(ConstantIndex::DELETEPROPERTY_STRING_INDEX,
                factory->NewFromCanBeCompressString("deleteProperty").GetTaggedValue());
    SetConstant(ConstantIndex::ENUMERATE_STRING_INDEX,
                factory->NewFromCanBeCompressString("enumerate").GetTaggedValue());
    SetConstant(ConstantIndex::OWNKEYS_STRING_INDEX, factory->NewFromCanBeCompressString("ownKeys").GetTaggedValue());
    SetConstant(ConstantIndex::APPLY_STRING_INDEX, factory->NewFromCanBeCompressString("apply").GetTaggedValue());
    SetConstant(ConstantIndex::NEGATIVE_ZERO_STRING_INDEX, factory->NewFromCanBeCompressString("-0").GetTaggedValue());
    SetConstant(ConstantIndex::DONE_STRING_INDEX, factory->NewFromCanBeCompressString("done").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_STRING_INDEX, factory->NewFromCanBeCompressString("proxy").GetTaggedValue());
    SetConstant(ConstantIndex::REVOKE_STRING_INDEX, factory->NewFromCanBeCompressString("revoke").GetTaggedValue());
    SetConstant(ConstantIndex::NEXT_STRING_INDEX, factory->NewFromCanBeCompressString("next").GetTaggedValue());
    SetConstant(ConstantIndex::TO_STRING_STRING_INDEX,
                factory->NewFromCanBeCompressString("toString").GetTaggedValue());
    SetConstant(ConstantIndex::TO_LOCALE_STRING_STRING_INDEX,
                factory->NewFromCanBeCompressString("toLocaleString").GetTaggedValue());
    SetConstant(ConstantIndex::VALUE_OF_STRING_INDEX, factory->NewFromCanBeCompressString("valueOf").GetTaggedValue());
    SetConstant(ConstantIndex::UNDEFINED_STRING_INDEX,
                factory->NewFromCanBeCompressString("undefined").GetTaggedValue());
    SetConstant(ConstantIndex::NULL_STRING_INDEX, factory->NewFromCanBeCompressString("null").GetTaggedValue());
    SetConstant(ConstantIndex::BOOLEAN_STRING_INDEX, factory->NewFromCanBeCompressString("boolean").GetTaggedValue());
    SetConstant(ConstantIndex::NUMBER_STRING_INDEX, factory->NewFromCanBeCompressString("number").GetTaggedValue());
    SetConstant(ConstantIndex::FUNCTION_STRING_INDEX, factory->NewFromCanBeCompressString("function").GetTaggedValue());
    SetConstant(ConstantIndex::STRING_STRING_INDEX, factory->NewFromCanBeCompressString("string").GetTaggedValue());
    SetConstant(ConstantIndex::SYMBOL_STRING_INDEX, factory->NewFromCanBeCompressString("symbol").GetTaggedValue());
    SetConstant(ConstantIndex::OBJECT_STRING_INDEX, factory->NewFromCanBeCompressString("object").GetTaggedValue());
    SetConstant(ConstantIndex::TRUE_STRING_INDEX, factory->NewFromCanBeCompressString("true").GetTaggedValue());
    SetConstant(ConstantIndex::FALSE_STRING_INDEX, factory->NewFromCanBeCompressString("false").GetTaggedValue());
    SetConstant(ConstantIndex::RETURN_STRING_INDEX, factory->NewFromCanBeCompressString("return").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_CONSTRUCT_STRING_INDEX,
                factory->NewFromCanBeCompressString("construct").GetTaggedValue());
    SetConstant(ConstantIndex::PROXY_CALL_STRING_INDEX, factory->NewFromCanBeCompressString("call").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_THEN_STRING_INDEX, factory->NewFromCanBeCompressString("then").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_CATCH_STRING_INDEX,
                factory->NewFromCanBeCompressString("catch").GetTaggedValue());
    SetConstant(ConstantIndex::SCRIPT_JOB_STRING_INDEX,
                factory->NewFromCanBeCompressString("ScriptJobs").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_STRING_INDEX,
                factory->NewFromCanBeCompressString("PrimiseJobs").GetTaggedValue());
    SetConstant(ConstantIndex::THROWER_STRING_INDEX, factory->NewFromCanBeCompressString("Thrower").GetTaggedValue());
    SetConstant(ConstantIndex::IDENTITY_STRING_INDEX, factory->NewFromCanBeCompressString("Identity").GetTaggedValue());
    SetConstant(ConstantIndex::CALLER_STRING_INDEX, factory->NewFromCanBeCompressString("caller").GetTaggedValue());
    SetConstant(ConstantIndex::CALLEE_STRING_INDEX, factory->NewFromCanBeCompressString("callee").GetTaggedValue());
    SetConstant(ConstantIndex::INT8_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Int8Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT8_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Uint8Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT8_CLAMPED_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Uint8ClampedArray").GetTaggedValue());
    SetConstant(ConstantIndex::INT16_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Int16Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT16_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Uint16Array").GetTaggedValue());
    SetConstant(ConstantIndex::INT32_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Int32Array").GetTaggedValue());
    SetConstant(ConstantIndex::UINT32_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Uint32Array").GetTaggedValue());
    SetConstant(ConstantIndex::FLOAT32_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Float32Array").GetTaggedValue());
    SetConstant(ConstantIndex::FLOAT64_ARRAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("Float64Array").GetTaggedValue());
    SetConstant(ConstantIndex::ASYNC_FUNCTION_STRING_INDEX,
                factory->NewFromCanBeCompressString("AsyncFunction").GetTaggedValue());
    SetConstant(ConstantIndex::PROMISE_RESOLVE_STRING_INDEX,
                factory->NewFromCanBeCompressString("resolve").GetTaggedValue());
    SetConstant(ConstantIndex::ID_STRING_INDEX, factory->NewFromCanBeCompressString("id").GetTaggedValue());
    SetConstant(ConstantIndex::METHOD_STRING_INDEX, factory->NewFromCanBeCompressString("method").GetTaggedValue());
    SetConstant(ConstantIndex::PARAMS_STRING_INDEX, factory->NewFromCanBeCompressString("params").GetTaggedValue());
    SetConstant(ConstantIndex::RESULT_STRING_INDEX, factory->NewFromCanBeCompressString("result").GetTaggedValue());
    SetConstant(ConstantIndex::TO_JSON_STRING_INDEX, factory->NewFromCanBeCompressString("toJSON").GetTaggedValue());
    SetConstant(ConstantIndex::GLOBAL_STRING_INDEX, factory->NewFromCanBeCompressString("global").GetTaggedValue());
    SetConstant(ConstantIndex::MESSAGE_STRING_INDEX, factory->NewFromCanBeCompressString("message").GetTaggedValue());
    SetConstant(ConstantIndex::ERROR_STRING_INDEX, factory->NewFromCanBeCompressString("Error").GetTaggedValue());
    SetConstant(ConstantIndex::RANGE_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("RangeError").GetTaggedValue());
    SetConstant(ConstantIndex::REFERENCE_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("ReferenceError").GetTaggedValue());
    SetConstant(ConstantIndex::TYPE_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("TypeError").GetTaggedValue());
    SetConstant(ConstantIndex::URI_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("URIError").GetTaggedValue());
    SetConstant(ConstantIndex::SYNTAX_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("SyntaxError").GetTaggedValue());
    SetConstant(ConstantIndex::EVAL_ERROR_STRING_INDEX,
                factory->NewFromCanBeCompressString("EvalError").GetTaggedValue());
    SetConstant(ConstantIndex::STACK_STRING_INDEX, factory->NewFromCanBeCompressString("stack").GetTaggedValue());
    SetConstant(ConstantIndex::STACK_EMPTY_STRING_INDEX,
                factory->NewFromCanBeCompressString("stackisempty").GetTaggedValue());
    SetConstant(ConstantIndex::OBJ_NOT_COERCIBLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("objectnotcoercible").GetTaggedValue());
    /* for Intl. */
    SetConstant(ConstantIndex::LANGUAGE_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("language").GetTaggedValue());
    SetConstant(ConstantIndex::SCRIPT_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("script").GetTaggedValue());
    SetConstant(ConstantIndex::REGION_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("region").GetTaggedValue());
    SetConstant(ConstantIndex::BASE_NAME_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("baseName").GetTaggedValue());
    SetConstant(ConstantIndex::CALENDAR_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("calendar").GetTaggedValue());
    SetConstant(ConstantIndex::COLLATION_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("collation").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR_CYCLE_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("hourCycle").GetTaggedValue());
    SetConstant(ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("caseFirst").GetTaggedValue());
    SetConstant(ConstantIndex::NUMERIC_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("numeric").GetTaggedValue());
    SetConstant(ConstantIndex::NUMBERING_SYSTEM_STRING_CLASS_INDEX,
                factory->NewFromCanBeCompressString("numberingSystem").GetTaggedValue());
    SetConstant(ConstantIndex::TYPE_STRING_INDEX, factory->NewFromCanBeCompressString("type").GetTaggedValue());
    SetConstant(ConstantIndex::LOCALE_MATCHER_STRING_INDEX,
                factory->NewFromCanBeCompressString("localeMatcher").GetTaggedValue());
    SetConstant(ConstantIndex::FORMAT_MATCHER_STRING_INDEX,
                factory->NewFromCanBeCompressString("formatMatcher").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR12_STRING_INDEX, factory->NewFromCanBeCompressString("hour12").GetTaggedValue());
    SetConstant(ConstantIndex::H11_STRING_INDEX, factory->NewFromCanBeCompressString("h11").GetTaggedValue());
    SetConstant(ConstantIndex::H12_STRING_INDEX, factory->NewFromCanBeCompressString("h12").GetTaggedValue());
    SetConstant(ConstantIndex::H23_STRING_INDEX, factory->NewFromCanBeCompressString("h23").GetTaggedValue());
    SetConstant(ConstantIndex::H24_STRING_INDEX, factory->NewFromCanBeCompressString("h24").GetTaggedValue());
    SetConstant(ConstantIndex::WEEK_DAY_STRING_INDEX, factory->NewFromCanBeCompressString("weekday").GetTaggedValue());
    SetConstant(ConstantIndex::ERA_STRING_INDEX, factory->NewFromCanBeCompressString("era").GetTaggedValue());
    SetConstant(ConstantIndex::YEAR_STRING_INDEX, factory->NewFromCanBeCompressString("year").GetTaggedValue());
    SetConstant(ConstantIndex::QUARTER_STRING_INDEX, factory->NewFromCanBeCompressString("quarter").GetTaggedValue());
    SetConstant(ConstantIndex::MONTH_STRING_INDEX, factory->NewFromCanBeCompressString("month").GetTaggedValue());
    SetConstant(ConstantIndex::DAY_STRING_INDEX, factory->NewFromCanBeCompressString("day").GetTaggedValue());
    SetConstant(ConstantIndex::HOUR_STRING_INDEX, factory->NewFromCanBeCompressString("hour").GetTaggedValue());
    SetConstant(ConstantIndex::MINUTE_STRING_INDEX, factory->NewFromCanBeCompressString("minute").GetTaggedValue());
    SetConstant(ConstantIndex::SECOND_STRING_INDEX, factory->NewFromCanBeCompressString("second").GetTaggedValue());
    SetConstant(ConstantIndex::YEARS_STRING_INDEX, factory->NewFromCanBeCompressString("years").GetTaggedValue());
    SetConstant(ConstantIndex::QUARTERS_STRING_INDEX, factory->NewFromCanBeCompressString("quarters").GetTaggedValue());
    SetConstant(ConstantIndex::MONTHS_STRING_INDEX, factory->NewFromCanBeCompressString("months").GetTaggedValue());
    SetConstant(ConstantIndex::DAYS_STRING_INDEX, factory->NewFromCanBeCompressString("days").GetTaggedValue());
    SetConstant(ConstantIndex::HOURS_STRING_INDEX, factory->NewFromCanBeCompressString("hours").GetTaggedValue());
    SetConstant(ConstantIndex::MINUTES_STRING_INDEX, factory->NewFromCanBeCompressString("minutes").GetTaggedValue());
    SetConstant(ConstantIndex::SECONDS_STRING_INDEX, factory->NewFromCanBeCompressString("seconds").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_ZONE_NAME_STRING_INDEX,
                factory->NewFromCanBeCompressString("timeZoneName").GetTaggedValue());
    SetConstant(ConstantIndex::LOCALE_STRING_INDEX, factory->NewFromCanBeCompressString("locale").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_ZONE_STRING_INDEX,
                factory->NewFromCanBeCompressString("timeZone").GetTaggedValue());
    SetConstant(ConstantIndex::LITERAL_STRING_INDEX, factory->NewFromCanBeCompressString("literal").GetTaggedValue());
    SetConstant(ConstantIndex::YEAR_NAME_STRING_INDEX,
                factory->NewFromCanBeCompressString("yearName").GetTaggedValue());
    SetConstant(ConstantIndex::DAY_PERIOD_STRING_INDEX,
                factory->NewFromCanBeCompressString("dayPeriod").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_DIGITS_STRING_INDEX,
                factory->NewFromCanBeCompressString("fractionalSecondDigits").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_STRING_INDEX,
                factory->NewFromCanBeCompressString("fractionalSecond").GetTaggedValue());
    SetConstant(ConstantIndex::RELATED_YEAR_STRING_INDEX,
                factory->NewFromCanBeCompressString("relatedYear").GetTaggedValue());
    SetConstant(ConstantIndex::LOOK_UP_STRING_INDEX, factory->NewFromCanBeCompressString("lookup").GetTaggedValue());
    SetConstant(ConstantIndex::BEST_FIT_STRING_INDEX, factory->NewFromCanBeCompressString("bestfit").GetTaggedValue());
    SetConstant(ConstantIndex::DATE_STYLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("dateStyle").GetTaggedValue());
    SetConstant(ConstantIndex::TIME_STYLE_STRING_INDEX,
                factory->NewFromCanBeCompressString("timeStyle").GetTaggedValue());
    SetConstant(ConstantIndex::UTC_STRING_INDEX, factory->NewFromCanBeCompressString("UTC").GetTaggedValue());
    SetConstant(ConstantIndex::INITIALIZED_RELATIVE_INDEX,
                factory->NewFromCanBeCompressString("true").GetTaggedValue());
    SetConstant(ConstantIndex::WEEK_STRING_INDEX, factory->NewFromCanBeCompressString("week").GetTaggedValue());
    SetConstant(ConstantIndex::WEEKS_STRING_INDEX, factory->NewFromCanBeCompressString("weeks").GetTaggedValue());
    SetConstant(ConstantIndex::SOURCE_STRING_INDEX, factory->NewFromCanBeCompressString("source").GetTaggedValue());
    SetConstant(ConstantIndex::FORMAT_STRING_INDEX, factory->NewFromCanBeCompressString("format").GetTaggedValue());
    SetConstant(ConstantIndex::EN_US_STRING_INDEX, factory->NewFromCanBeCompressString("en-US").GetTaggedValue());
    SetConstant(ConstantIndex::UND_STRING_INDEX, factory->NewFromCanBeCompressString("und").GetTaggedValue());
    SetConstant(ConstantIndex::LATN_STRING_INDEX, factory->NewFromCanBeCompressString("latn").GetTaggedValue());
    SetConstant(ConstantIndex::STYLE_STRING_INDEX, factory->NewFromCanBeCompressString("style").GetTaggedValue());
    SetConstant(ConstantIndex::UNIT_STRING_INDEX, factory->NewFromCanBeCompressString("unit").GetTaggedValue());
    SetConstant(ConstantIndex::INTEGER_STRING_INDEX, factory->NewFromCanBeCompressString("integer").GetTaggedValue());
    SetConstant(ConstantIndex::NAN_STRING_INDEX, factory->NewFromCanBeCompressString("nan").GetTaggedValue());
    SetConstant(ConstantIndex::INFINITY_STRING_INDEX, factory->NewFromCanBeCompressString("infinity").GetTaggedValue());
    SetConstant(ConstantIndex::FRACTION_STRING_INDEX, factory->NewFromCanBeCompressString("fraction").GetTaggedValue());
    SetConstant(ConstantIndex::DECIMAL_STRING_INDEX, factory->NewFromCanBeCompressString("decimal").GetTaggedValue());
    SetConstant(ConstantIndex::GROUP_STRING_INDEX, factory->NewFromCanBeCompressString("group").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_STRING_INDEX, factory->NewFromCanBeCompressString("currency").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_SIGN_STRING_INDEX,
                factory->NewFromCanBeCompressString("currencySign").GetTaggedValue());
    SetConstant(ConstantIndex::CURRENCY_DISPLAY_STRING_INDEX,
                factory->NewFromCanBeCompressString("currencyDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::PERCENT_SIGN_STRING_INDEX,
                factory->NewFromCanBeCompressString("percentSign").GetTaggedValue());
    SetConstant(ConstantIndex::PERCENT_STRING_INDEX, factory->NewFromCanBeCompressString("percent").GetTaggedValue());
    SetConstant(ConstantIndex::MINUS_SIGN_STRING_INDEX,
                factory->NewFromCanBeCompressString("minusSign").GetTaggedValue());
    SetConstant(ConstantIndex::PLUS_SIGN_STRING_INDEX,
                factory->NewFromCanBeCompressString("plusSign").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_SEPARATOR_STRING_INDEX,
                factory->NewFromCanBeCompressString("exponentSeparator").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_MINUS_SIGN_INDEX,
                factory->NewFromCanBeCompressString("exponentMinusSign").GetTaggedValue());
    SetConstant(ConstantIndex::EXPONENT_INTEGER_STRING_INDEX,
                factory->NewFromCanBeCompressString("exponentInteger").GetTaggedValue());
    SetConstant(ConstantIndex::LONG_STRING_INDEX, factory->NewFromCanBeCompressString("long").GetTaggedValue());
    SetConstant(ConstantIndex::SHORT_STRING_INDEX, factory->NewFromCanBeCompressString("short").GetTaggedValue());
    SetConstant(ConstantIndex::FULL_STRING_INDEX, factory->NewFromCanBeCompressString("full").GetTaggedValue());
    SetConstant(ConstantIndex::MEDIUM_STRING_INDEX, factory->NewFromCanBeCompressString("medium").GetTaggedValue());
    SetConstant(ConstantIndex::NARROW_STRING_INDEX, factory->NewFromCanBeCompressString("narrow").GetTaggedValue());
    SetConstant(ConstantIndex::ALWAYS_STRING_INDEX, factory->NewFromCanBeCompressString("always").GetTaggedValue());
    SetConstant(ConstantIndex::AUTO_STRING_INDEX, factory->NewFromCanBeCompressString("auto").GetTaggedValue());
    SetConstant(ConstantIndex::UNIT_DISPLAY_INDEX, factory->NewFromCanBeCompressString("unitDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::NOTATION_INDEX, factory->NewFromCanBeCompressString("notation").GetTaggedValue());
    SetConstant(ConstantIndex::COMPACT_DISPALY_INDEX,
                factory->NewFromCanBeCompressString("compactDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::USER_GROUPING_INDEX,
                factory->NewFromCanBeCompressString("useGrouping").GetTaggedValue());
    SetConstant(ConstantIndex::SIGN_DISPLAY_INDEX, factory->NewFromCanBeCompressString("signDisplay").GetTaggedValue());
    SetConstant(ConstantIndex::CODE_INDEX, factory->NewFromCanBeCompressString("code").GetTaggedValue());
    SetConstant(ConstantIndex::NARROW_SYMBOL_INDEX,
                factory->NewFromCanBeCompressString("narrowSymbol").GetTaggedValue());
    SetConstant(ConstantIndex::STANDARD_INDEX, factory->NewFromCanBeCompressString("standard").GetTaggedValue());
    SetConstant(ConstantIndex::ACCOUNTING_INDEX, factory->NewFromCanBeCompressString("accounting").GetTaggedValue());
    SetConstant(ConstantIndex::SCIENTIFIC_INDEX, factory->NewFromCanBeCompressString("scientific").GetTaggedValue());
    SetConstant(ConstantIndex::ENGINEERING_INDEX, factory->NewFromCanBeCompressString("engineering").GetTaggedValue());
    SetConstant(ConstantIndex::COMPACT_STRING_INDEX, factory->NewFromCanBeCompressString("compact").GetTaggedValue());
    SetConstant(ConstantIndex::NEVER_INDEX, factory->NewFromCanBeCompressString("never").GetTaggedValue());
    SetConstant(ConstantIndex::EXPECT_ZERO_INDEX, factory->NewFromCanBeCompressString("exceptZero").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_INTEGER_DIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumIntegerDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumFractionDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MAXIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromCanBeCompressString("maximumFractionDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MINIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromCanBeCompressString("minimumSignificantDigits").GetTaggedValue());
    SetConstant(ConstantIndex::MAXIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromCanBeCompressString("maximumSignificantDigits").GetTaggedValue());
    SetConstant(ConstantIndex::INVALID_DATE_INDEX,
                factory->NewFromCanBeCompressString("Invalid Date").GetTaggedValue());
    SetConstant(ConstantIndex::USAGE_INDEX, factory->NewFromCanBeCompressString("usage").GetTaggedValue());
    SetConstant(ConstantIndex::COMPARE_INDEX, factory->NewFromCanBeCompressString("compare").GetTaggedValue());
    SetConstant(ConstantIndex::SENSITIVITY_INDEX, factory->NewFromCanBeCompressString("sensitivity").GetTaggedValue());
    SetConstant(ConstantIndex::IGNORE_PUNCTUATION_INDEX,
                factory->NewFromCanBeCompressString("ignorePunctuation").GetTaggedValue());
    SetConstant(ConstantIndex::CARDINAL_INDEX, factory->NewFromCanBeCompressString("cardinal").GetTaggedValue());
    SetConstant(ConstantIndex::ORDINAL_INDEX, factory->NewFromCanBeCompressString("ordinal").GetTaggedValue());
    SetConstant(ConstantIndex::EXEC_INDEX, factory->NewFromCanBeCompressString("exec").GetTaggedValue());
    SetConstant(ConstantIndex::LAST_INDEX_INDEX, factory->NewFromCanBeCompressString("lastIndex").GetTaggedValue());
    SetConstant(ConstantIndex::PLURAL_CATEGORIES_INDEX,
                factory->NewFromCanBeCompressString("pluralCategories").GetTaggedValue());
    SetConstant(ConstantIndex::SORT_INDEX, factory->NewFromCanBeCompressString("sort").GetTaggedValue());
    SetConstant(ConstantIndex::SEARCH_INDEX, factory->NewFromCanBeCompressString("search").GetTaggedValue());
    SetConstant(ConstantIndex::BASE_INDEX, factory->NewFromCanBeCompressString("base").GetTaggedValue());
    SetConstant(ConstantIndex::ACCENT_INDEX, factory->NewFromCanBeCompressString("accent").GetTaggedValue());
    SetConstant(ConstantIndex::CASE_INDEX, factory->NewFromCanBeCompressString("case").GetTaggedValue());
    SetConstant(ConstantIndex::VARIANT_INDEX, factory->NewFromCanBeCompressString("variant").GetTaggedValue());
    SetConstant(ConstantIndex::EN_US_POSIX_STRING_INDEX,
                factory->NewFromCanBeCompressString("en-US-POSIX").GetTaggedValue());
    SetConstant(ConstantIndex::UPPER_INDEX, factory->NewFromCanBeCompressString("upper").GetTaggedValue());
    SetConstant(ConstantIndex::LOWER_INDEX, factory->NewFromCanBeCompressString("lower").GetTaggedValue());
    SetConstant(ConstantIndex::DEFAULT_INDEX, factory->NewFromCanBeCompressString("default").GetTaggedValue());
    SetConstant(ConstantIndex::SHARED_INDEX, factory->NewFromCanBeCompressString("shared").GetTaggedValue());
    SetConstant(ConstantIndex::START_RANGE_INDEX, factory->NewFromCanBeCompressString("startRange").GetTaggedValue());
    SetConstant(ConstantIndex::END_RANGE_INDEX, factory->NewFromCanBeCompressString("endRange").GetTaggedValue());
    SetConstant(ConstantIndex::ISO8601_INDEX, factory->NewFromCanBeCompressString("iso8601").GetTaggedValue());
    SetConstant(ConstantIndex::GREGORY_INDEX, factory->NewFromCanBeCompressString("gregory").GetTaggedValue());
    SetConstant(ConstantIndex::ETHIOAA_INDEX, factory->NewFromCanBeCompressString("ethioaa").GetTaggedValue());

    auto accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSFunction::PrototypeSetter),
                                                 reinterpret_cast<void *>(JSFunction::PrototypeGetter));
    SetConstant(ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR, accessor.GetTaggedValue());
    accessor = factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(JSFunction::NameGetter));
    SetConstant(ConstantIndex::FUNCTION_NAME_ACCESSOR, accessor.GetTaggedValue());
    accessor = factory->NewInternalAccessor(reinterpret_cast<void *>(JSArray::LengthSetter),
                                            reinterpret_cast<void *>(JSArray::LengthGetter));
    SetConstant(ConstantIndex::ARRAY_LENGTH_ACCESSOR, accessor.GetTaggedValue());
}

void GlobalEnvConstants::InitGlobalUndefined()
{
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
}
}  // namespace panda::ecmascript
