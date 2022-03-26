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
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_queue_iterator.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set_iterator.h"
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
void GlobalEnvConstants::Init(JSThread *thread, JSHClass *dynClassClass)
{
    InitRootsClass(thread, dynClassClass);
    InitGlobalConstant(thread);
}

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
    InitGlobalConstantSpecial(thread);
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
    SetConstant(ConstantIndex::JS_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSSetIterator::SIZE, JSType::JS_SET_ITERATOR));
    SetConstant(ConstantIndex::JS_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSMapIterator::SIZE, JSType::JS_MAP_ITERATOR));
    SetConstant(ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSArrayIterator::SIZE, JSType::JS_ARRAY_ITERATOR));
    SetConstant(
        ConstantIndex::JS_API_ARRAYLIST_ITERATOR_CLASS_INDEX,
        factory->NewEcmaDynClass(dynClassClass, JSAPIArrayListIterator::SIZE, JSType::JS_API_ARRAYLIST_ITERATOR));
    SetConstant(ConstantIndex::JS_API_DEQUE_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSAPIDequeIterator::SIZE, JSType::JS_API_DEQUE_ITERATOR));
    SetConstant(ConstantIndex::JS_API_QUEUE_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSAPIQueueIterator::SIZE, JSType::JS_API_QUEUE_ITERATOR));
    SetConstant(ConstantIndex::JS_API_STACK_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSAPIStackIterator::SIZE, JSType::JS_API_STACK_ITERATOR));
    SetConstant(ConstantIndex::JS_API_TREE_MAP_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSAPITreeMapIterator::SIZE, JSType::JS_API_TREEMAP_ITERATOR));
    SetConstant(ConstantIndex::JS_API_TREE_SET_ITERATOR_CLASS_INDEX,
                factory->NewEcmaDynClass(dynClassClass, JSAPITreeSetIterator::SIZE, JSType::JS_API_TREESET_ITERATOR));
}

void GlobalEnvConstants::InitGlobalConstantSpecial(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // SPECIAL INIT
    SetConstant(ConstantIndex::UNDEFINED_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::NULL_INDEX, JSTaggedValue::Null());
    auto vm = thread->GetEcmaVM();
    SetConstant(ConstantIndex::EMPTY_STRING_OBJECT_INDEX, JSTaggedValue(EcmaString::CreateEmptyString(vm)));
    SetConstant(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX, factory->NewEmptyArray());
    SetConstant(ConstantIndex::EMPTY_LAYOUT_INFO_OBJECT_INDEX, factory->CreateLayoutInfo(0));
    SetConstant(ConstantIndex::EMPTY_TAGGED_QUEUE_OBJECT_INDEX, factory->NewTaggedQueue(0));
}

// NOLINTNEXTLINE(readability-function-size)
void GlobalEnvConstants::InitGlobalConstant(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] auto test = EcmaString::Cast(GetHandledEmptyString().GetObject<EcmaString>());
    SetConstant(ConstantIndex::CONSTRUCTOR_STRING_INDEX, factory->NewFromASCII("constructor"));
    SetConstant(ConstantIndex::PROTOTYPE_STRING_INDEX, factory->NewFromASCII("prototype"));
    SetConstant(ConstantIndex::LENGTH_STRING_INDEX, factory->NewFromASCII("length"));
    SetConstant(ConstantIndex::VALUE_STRING_INDEX, factory->NewFromASCII("value"));
    SetConstant(ConstantIndex::SET_STRING_INDEX, factory->NewFromASCII("set"));
    SetConstant(ConstantIndex::GET_STRING_INDEX, factory->NewFromASCII("get"));
    SetConstant(ConstantIndex::WRITABLE_STRING_INDEX, factory->NewFromASCII("writable"));
    SetConstant(ConstantIndex::ENUMERABLE_STRING_INDEX, factory->NewFromASCII("enumerable"));
    SetConstant(ConstantIndex::CONFIGURABLE_STRING_INDEX, factory->NewFromASCII("configurable"));
    /* non ECMA standard jsapi containers iterators, init to Undefined first */
    SetConstant(ConstantIndex::ARRAYLIST_FUNCTION_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::ARRAYLIST_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::TREEMAP_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::TREESET_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::QUEUE_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::DEQUE_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    SetConstant(ConstantIndex::STACK_ITERATOR_PROTOTYPE_INDEX, JSTaggedValue::Undefined());
    /* SymbolTable *RegisterSymbols */
    SetConstant(ConstantIndex::NAME_STRING_INDEX, factory->NewFromASCII("name"));
    SetConstant(ConstantIndex::GETPROTOTYPEOF_STRING_INDEX, factory->NewFromASCII("getPrototypeOf"));
    SetConstant(ConstantIndex::SETPROTOTYPEOF_STRING_INDEX, factory->NewFromASCII("setPrototypeOf"));
    SetConstant(ConstantIndex::ISEXTENSIBLE_STRING_INDEX, factory->NewFromASCII("isExtensible"));
    SetConstant(ConstantIndex::PREVENTEXTENSIONS_STRING_INDEX,
                factory->NewFromASCII("preventExtensions"));
    SetConstant(ConstantIndex::GETOWNPROPERTYDESCRIPTOR_STRING_INDEX,
                factory->NewFromASCII("getOwnPropertyDescriptor"));
    SetConstant(ConstantIndex::DEFINEPROPERTY_STRING_INDEX, factory->NewFromASCII("defineProperty"));
    SetConstant(ConstantIndex::HAS_STRING_INDEX, factory->NewFromASCII("has"));
    SetConstant(ConstantIndex::DELETEPROPERTY_STRING_INDEX, factory->NewFromASCII("deleteProperty"));
    SetConstant(ConstantIndex::ENUMERATE_STRING_INDEX, factory->NewFromASCII("enumerate"));
    SetConstant(ConstantIndex::OWNKEYS_STRING_INDEX, factory->NewFromASCII("ownKeys"));
    SetConstant(ConstantIndex::APPLY_STRING_INDEX, factory->NewFromASCII("apply"));
    SetConstant(ConstantIndex::NEGATIVE_ZERO_STRING_INDEX, factory->NewFromASCII("-0"));
    SetConstant(ConstantIndex::DONE_STRING_INDEX, factory->NewFromASCII("done"));
    SetConstant(ConstantIndex::PROXY_STRING_INDEX, factory->NewFromASCII("proxy"));
    SetConstant(ConstantIndex::REVOKE_STRING_INDEX, factory->NewFromASCII("revoke"));
    SetConstant(ConstantIndex::NEXT_STRING_INDEX, factory->NewFromASCII("next"));
    SetConstant(ConstantIndex::TO_STRING_STRING_INDEX, factory->NewFromASCII("toString"));
    SetConstant(ConstantIndex::TO_LOCALE_STRING_STRING_INDEX, factory->NewFromASCII("toLocaleString"));
    SetConstant(ConstantIndex::VALUE_OF_STRING_INDEX, factory->NewFromASCII("valueOf"));
    SetConstant(ConstantIndex::UNDEFINED_STRING_INDEX, factory->NewFromASCII("undefined"));
    SetConstant(ConstantIndex::NULL_STRING_INDEX, factory->NewFromASCII("null"));
    SetConstant(ConstantIndex::BOOLEAN_STRING_INDEX, factory->NewFromASCII("boolean"));
    SetConstant(ConstantIndex::NUMBER_STRING_INDEX, factory->NewFromASCII("number"));
    SetConstant(ConstantIndex::BIGINT_STRING_INDEX, factory->NewFromASCII("bigint"));
    SetConstant(ConstantIndex::FUNCTION_STRING_INDEX, factory->NewFromASCII("function"));
    SetConstant(ConstantIndex::STRING_STRING_INDEX, factory->NewFromASCII("string"));
    SetConstant(ConstantIndex::SYMBOL_STRING_INDEX, factory->NewFromASCII("symbol"));
    SetConstant(ConstantIndex::OBJECT_STRING_INDEX, factory->NewFromASCII("object"));
    SetConstant(ConstantIndex::TRUE_STRING_INDEX, factory->NewFromASCII("true"));
    SetConstant(ConstantIndex::FALSE_STRING_INDEX, factory->NewFromASCII("false"));
    SetConstant(ConstantIndex::RETURN_STRING_INDEX, factory->NewFromASCII("return"));
    SetConstant(ConstantIndex::PROXY_CONSTRUCT_STRING_INDEX, factory->NewFromASCII("construct"));
    SetConstant(ConstantIndex::PROXY_CALL_STRING_INDEX, factory->NewFromASCII("call"));
    SetConstant(ConstantIndex::PROMISE_THEN_STRING_INDEX, factory->NewFromASCII("then"));
    SetConstant(ConstantIndex::PROMISE_CATCH_STRING_INDEX, factory->NewFromASCII("catch"));
    SetConstant(ConstantIndex::SCRIPT_JOB_STRING_INDEX, factory->NewFromASCII("ScriptJobs"));
    SetConstant(ConstantIndex::PROMISE_STRING_INDEX, factory->NewFromASCII("PrimiseJobs"));
    SetConstant(ConstantIndex::THROWER_STRING_INDEX, factory->NewFromASCII("Thrower"));
    SetConstant(ConstantIndex::IDENTITY_STRING_INDEX, factory->NewFromASCII("Identity"));
    SetConstant(ConstantIndex::CALLER_STRING_INDEX, factory->NewFromASCII("caller"));
    SetConstant(ConstantIndex::CALLEE_STRING_INDEX, factory->NewFromASCII("callee"));
    SetConstant(ConstantIndex::INT8_ARRAY_STRING_INDEX, factory->NewFromASCII("Int8Array"));
    SetConstant(ConstantIndex::UINT8_ARRAY_STRING_INDEX, factory->NewFromASCII("Uint8Array"));
    SetConstant(ConstantIndex::UINT8_CLAMPED_ARRAY_STRING_INDEX,
                factory->NewFromASCII("Uint8ClampedArray"));
    SetConstant(ConstantIndex::INT16_ARRAY_STRING_INDEX, factory->NewFromASCII("Int16Array"));
    SetConstant(ConstantIndex::UINT16_ARRAY_STRING_INDEX, factory->NewFromASCII("Uint16Array"));
    SetConstant(ConstantIndex::INT32_ARRAY_STRING_INDEX, factory->NewFromASCII("Int32Array"));
    SetConstant(ConstantIndex::UINT32_ARRAY_STRING_INDEX, factory->NewFromASCII("Uint32Array"));
    SetConstant(ConstantIndex::FLOAT32_ARRAY_STRING_INDEX, factory->NewFromASCII("Float32Array"));
    SetConstant(ConstantIndex::FLOAT64_ARRAY_STRING_INDEX, factory->NewFromASCII("Float64Array"));
    SetConstant(ConstantIndex::BIGINT64_ARRAY_STRING_INDEX, factory->NewFromASCII("BigInt64Array"));
    SetConstant(ConstantIndex::BIGUINT64_ARRAY_STRING_INDEX, factory->NewFromASCII("BigUint64Array"));
    SetConstant(ConstantIndex::ASYNC_FUNCTION_STRING_INDEX, factory->NewFromASCII("AsyncFunction"));
    SetConstant(ConstantIndex::PROMISE_RESOLVE_STRING_INDEX, factory->NewFromASCII("resolve"));
    SetConstant(ConstantIndex::ID_STRING_INDEX, factory->NewFromASCII("id"));
    SetConstant(ConstantIndex::METHOD_STRING_INDEX, factory->NewFromASCII("method"));
    SetConstant(ConstantIndex::PARAMS_STRING_INDEX, factory->NewFromASCII("params"));
    SetConstant(ConstantIndex::RESULT_STRING_INDEX, factory->NewFromASCII("result"));
    SetConstant(ConstantIndex::TO_JSON_STRING_INDEX, factory->NewFromASCII("toJSON"));
    SetConstant(ConstantIndex::GLOBAL_STRING_INDEX, factory->NewFromASCII("global"));
    SetConstant(ConstantIndex::MESSAGE_STRING_INDEX, factory->NewFromASCII("message"));
    SetConstant(ConstantIndex::ERROR_STRING_INDEX, factory->NewFromASCII("Error"));
    SetConstant(ConstantIndex::RANGE_ERROR_STRING_INDEX, factory->NewFromASCII("RangeError"));
    SetConstant(ConstantIndex::REFERENCE_ERROR_STRING_INDEX, factory->NewFromASCII("ReferenceError"));
    SetConstant(ConstantIndex::TYPE_ERROR_STRING_INDEX, factory->NewFromASCII("TypeError"));
    SetConstant(ConstantIndex::URI_ERROR_STRING_INDEX, factory->NewFromASCII("URIError"));
    SetConstant(ConstantIndex::SYNTAX_ERROR_STRING_INDEX, factory->NewFromASCII("SyntaxError"));
    SetConstant(ConstantIndex::EVAL_ERROR_STRING_INDEX, factory->NewFromASCII("EvalError"));
    SetConstant(ConstantIndex::STACK_STRING_INDEX, factory->NewFromASCII("stack"));
    SetConstant(ConstantIndex::STACK_EMPTY_STRING_INDEX, factory->NewFromASCII("stackisempty"));
    SetConstant(ConstantIndex::OBJ_NOT_COERCIBLE_STRING_INDEX,
                factory->NewFromASCII("objectnotcoercible"));
    /* for Intl. */
    SetConstant(ConstantIndex::LANGUAGE_STRING_CLASS_INDEX, factory->NewFromASCII("language"));
    SetConstant(ConstantIndex::SCRIPT_STRING_CLASS_INDEX, factory->NewFromASCII("script"));
    SetConstant(ConstantIndex::REGION_STRING_CLASS_INDEX, factory->NewFromASCII("region"));
    SetConstant(ConstantIndex::BASE_NAME_STRING_CLASS_INDEX, factory->NewFromASCII("baseName"));
    SetConstant(ConstantIndex::CALENDAR_STRING_CLASS_INDEX, factory->NewFromASCII("calendar"));
    SetConstant(ConstantIndex::COLLATION_STRING_CLASS_INDEX, factory->NewFromASCII("collation"));
    SetConstant(ConstantIndex::HOUR_CYCLE_STRING_CLASS_INDEX, factory->NewFromASCII("hourCycle"));
    SetConstant(ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX, factory->NewFromASCII("caseFirst"));
    SetConstant(ConstantIndex::NUMERIC_STRING_CLASS_INDEX, factory->NewFromASCII("numeric"));
    SetConstant(ConstantIndex::NUMBERING_SYSTEM_STRING_CLASS_INDEX,
                factory->NewFromASCII("numberingSystem"));
    SetConstant(ConstantIndex::TYPE_STRING_INDEX, factory->NewFromASCII("type"));
    SetConstant(ConstantIndex::LOCALE_MATCHER_STRING_INDEX, factory->NewFromASCII("localeMatcher"));
    SetConstant(ConstantIndex::FORMAT_MATCHER_STRING_INDEX, factory->NewFromASCII("formatMatcher"));
    SetConstant(ConstantIndex::HOUR12_STRING_INDEX, factory->NewFromASCII("hour12"));
    SetConstant(ConstantIndex::H11_STRING_INDEX, factory->NewFromASCII("h11"));
    SetConstant(ConstantIndex::H12_STRING_INDEX, factory->NewFromASCII("h12"));
    SetConstant(ConstantIndex::H23_STRING_INDEX, factory->NewFromASCII("h23"));
    SetConstant(ConstantIndex::H24_STRING_INDEX, factory->NewFromASCII("h24"));
    SetConstant(ConstantIndex::WEEK_DAY_STRING_INDEX, factory->NewFromASCII("weekday"));
    SetConstant(ConstantIndex::ERA_STRING_INDEX, factory->NewFromASCII("era"));
    SetConstant(ConstantIndex::YEAR_STRING_INDEX, factory->NewFromASCII("year"));
    SetConstant(ConstantIndex::QUARTER_STRING_INDEX, factory->NewFromASCII("quarter"));
    SetConstant(ConstantIndex::MONTH_STRING_INDEX, factory->NewFromASCII("month"));
    SetConstant(ConstantIndex::DAY_STRING_INDEX, factory->NewFromASCII("day"));
    SetConstant(ConstantIndex::HOUR_STRING_INDEX, factory->NewFromASCII("hour"));
    SetConstant(ConstantIndex::MINUTE_STRING_INDEX, factory->NewFromASCII("minute"));
    SetConstant(ConstantIndex::SECOND_STRING_INDEX, factory->NewFromASCII("second"));
    SetConstant(ConstantIndex::YEARS_STRING_INDEX, factory->NewFromASCII("years"));
    SetConstant(ConstantIndex::QUARTERS_STRING_INDEX, factory->NewFromASCII("quarters"));
    SetConstant(ConstantIndex::MONTHS_STRING_INDEX, factory->NewFromASCII("months"));
    SetConstant(ConstantIndex::DAYS_STRING_INDEX, factory->NewFromASCII("days"));
    SetConstant(ConstantIndex::HOURS_STRING_INDEX, factory->NewFromASCII("hours"));
    SetConstant(ConstantIndex::MINUTES_STRING_INDEX, factory->NewFromASCII("minutes"));
    SetConstant(ConstantIndex::SECONDS_STRING_INDEX, factory->NewFromASCII("seconds"));
    SetConstant(ConstantIndex::TIME_ZONE_NAME_STRING_INDEX, factory->NewFromASCII("timeZoneName"));
    SetConstant(ConstantIndex::LOCALE_STRING_INDEX, factory->NewFromASCII("locale"));
    SetConstant(ConstantIndex::TIME_ZONE_STRING_INDEX, factory->NewFromASCII("timeZone"));
    SetConstant(ConstantIndex::LITERAL_STRING_INDEX, factory->NewFromASCII("literal"));
    SetConstant(ConstantIndex::YEAR_NAME_STRING_INDEX, factory->NewFromASCII("yearName"));
    SetConstant(ConstantIndex::DAY_PERIOD_STRING_INDEX, factory->NewFromASCII("dayPeriod"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_DIGITS_STRING_INDEX,
                factory->NewFromASCII("fractionalSecondDigits"));
    SetConstant(ConstantIndex::FRACTIONAL_SECOND_STRING_INDEX, factory->NewFromASCII("fractionalSecond"));
    SetConstant(ConstantIndex::RELATED_YEAR_STRING_INDEX, factory->NewFromASCII("relatedYear"));
    SetConstant(ConstantIndex::LOOK_UP_STRING_INDEX, factory->NewFromASCII("lookup"));
    SetConstant(ConstantIndex::BEST_FIT_STRING_INDEX, factory->NewFromASCII("bestfit"));
    SetConstant(ConstantIndex::DATE_STYLE_STRING_INDEX, factory->NewFromASCII("dateStyle"));
    SetConstant(ConstantIndex::TIME_STYLE_STRING_INDEX, factory->NewFromASCII("timeStyle"));
    SetConstant(ConstantIndex::UTC_STRING_INDEX, factory->NewFromASCII("UTC"));
    SetConstant(ConstantIndex::INITIALIZED_RELATIVE_INDEX, factory->NewFromASCII("true"));
    SetConstant(ConstantIndex::WEEK_STRING_INDEX, factory->NewFromASCII("week"));
    SetConstant(ConstantIndex::WEEKS_STRING_INDEX, factory->NewFromASCII("weeks"));
    SetConstant(ConstantIndex::SOURCE_STRING_INDEX, factory->NewFromASCII("source"));
    SetConstant(ConstantIndex::FORMAT_STRING_INDEX, factory->NewFromASCII("format"));
    SetConstant(ConstantIndex::EN_US_STRING_INDEX, factory->NewFromASCII("en-US"));
    SetConstant(ConstantIndex::UND_STRING_INDEX, factory->NewFromASCII("und"));
    SetConstant(ConstantIndex::LATN_STRING_INDEX, factory->NewFromASCII("latn"));
    SetConstant(ConstantIndex::STYLE_STRING_INDEX, factory->NewFromASCII("style"));
    SetConstant(ConstantIndex::UNIT_STRING_INDEX, factory->NewFromASCII("unit"));
    SetConstant(ConstantIndex::INTEGER_STRING_INDEX, factory->NewFromASCII("integer"));
    SetConstant(ConstantIndex::NAN_STRING_INDEX, factory->NewFromASCII("nan"));
    SetConstant(ConstantIndex::INFINITY_STRING_INDEX, factory->NewFromASCII("infinity"));
    SetConstant(ConstantIndex::FRACTION_STRING_INDEX, factory->NewFromASCII("fraction"));
    SetConstant(ConstantIndex::DECIMAL_STRING_INDEX, factory->NewFromASCII("decimal"));
    SetConstant(ConstantIndex::GROUP_STRING_INDEX, factory->NewFromASCII("group"));
    SetConstant(ConstantIndex::CURRENCY_STRING_INDEX, factory->NewFromASCII("currency"));
    SetConstant(ConstantIndex::CURRENCY_SIGN_STRING_INDEX, factory->NewFromASCII("currencySign"));
    SetConstant(ConstantIndex::CURRENCY_DISPLAY_STRING_INDEX, factory->NewFromASCII("currencyDisplay"));
    SetConstant(ConstantIndex::PERCENT_SIGN_STRING_INDEX, factory->NewFromASCII("percentSign"));
    SetConstant(ConstantIndex::PERCENT_STRING_INDEX, factory->NewFromASCII("percent"));
    SetConstant(ConstantIndex::MINUS_SIGN_STRING_INDEX, factory->NewFromASCII("minusSign"));
    SetConstant(ConstantIndex::PLUS_SIGN_STRING_INDEX, factory->NewFromASCII("plusSign"));
    SetConstant(ConstantIndex::EXPONENT_SEPARATOR_STRING_INDEX,
                factory->NewFromASCII("exponentSeparator"));
    SetConstant(ConstantIndex::EXPONENT_MINUS_SIGN_INDEX, factory->NewFromASCII("exponentMinusSign"));
    SetConstant(ConstantIndex::EXPONENT_INTEGER_STRING_INDEX, factory->NewFromASCII("exponentInteger"));
    SetConstant(ConstantIndex::LONG_STRING_INDEX, factory->NewFromASCII("long"));
    SetConstant(ConstantIndex::SHORT_STRING_INDEX, factory->NewFromASCII("short"));
    SetConstant(ConstantIndex::FULL_STRING_INDEX, factory->NewFromASCII("full"));
    SetConstant(ConstantIndex::MEDIUM_STRING_INDEX, factory->NewFromASCII("medium"));
    SetConstant(ConstantIndex::NARROW_STRING_INDEX, factory->NewFromASCII("narrow"));
    SetConstant(ConstantIndex::ALWAYS_STRING_INDEX, factory->NewFromASCII("always"));
    SetConstant(ConstantIndex::AUTO_STRING_INDEX, factory->NewFromASCII("auto"));
    SetConstant(ConstantIndex::UNIT_DISPLAY_INDEX, factory->NewFromASCII("unitDisplay"));
    SetConstant(ConstantIndex::NOTATION_INDEX, factory->NewFromASCII("notation"));
    SetConstant(ConstantIndex::COMPACT_DISPALY_INDEX, factory->NewFromASCII("compactDisplay"));
    SetConstant(ConstantIndex::USER_GROUPING_INDEX, factory->NewFromASCII("useGrouping"));
    SetConstant(ConstantIndex::SIGN_DISPLAY_INDEX, factory->NewFromASCII("signDisplay"));
    SetConstant(ConstantIndex::CODE_INDEX, factory->NewFromASCII("code"));
    SetConstant(ConstantIndex::NARROW_SYMBOL_INDEX, factory->NewFromASCII("narrowSymbol"));
    SetConstant(ConstantIndex::STANDARD_INDEX, factory->NewFromASCII("standard"));
    SetConstant(ConstantIndex::ACCOUNTING_INDEX, factory->NewFromASCII("accounting"));
    SetConstant(ConstantIndex::SCIENTIFIC_INDEX, factory->NewFromASCII("scientific"));
    SetConstant(ConstantIndex::ENGINEERING_INDEX, factory->NewFromASCII("engineering"));
    SetConstant(ConstantIndex::COMPACT_STRING_INDEX, factory->NewFromASCII("compact"));
    SetConstant(ConstantIndex::NEVER_INDEX, factory->NewFromASCII("never"));
    SetConstant(ConstantIndex::EXPECT_ZERO_INDEX, factory->NewFromASCII("exceptZero"));
    SetConstant(ConstantIndex::MINIMUM_INTEGER_DIGITS_INDEX,
                factory->NewFromASCII("minimumIntegerDigits"));
    SetConstant(ConstantIndex::MINIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromASCII("minimumFractionDigits"));
    SetConstant(ConstantIndex::MAXIMUM_FRACTIONDIGITS_INDEX,
                factory->NewFromASCII("maximumFractionDigits"));
    SetConstant(ConstantIndex::MINIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromASCII("minimumSignificantDigits"));
    SetConstant(ConstantIndex::MAXIMUM_SIGNIFICANTDIGITS_INDEX,
                factory->NewFromASCII("maximumSignificantDigits"));
    SetConstant(ConstantIndex::INVALID_DATE_INDEX, factory->NewFromASCII("Invalid Date"));
    SetConstant(ConstantIndex::USAGE_INDEX, factory->NewFromASCII("usage"));
    SetConstant(ConstantIndex::COMPARE_INDEX, factory->NewFromASCII("compare"));
    SetConstant(ConstantIndex::SENSITIVITY_INDEX, factory->NewFromASCII("sensitivity"));
    SetConstant(ConstantIndex::IGNORE_PUNCTUATION_INDEX, factory->NewFromASCII("ignorePunctuation"));
    SetConstant(ConstantIndex::CARDINAL_INDEX, factory->NewFromASCII("cardinal"));
    SetConstant(ConstantIndex::ORDINAL_INDEX, factory->NewFromASCII("ordinal"));
    SetConstant(ConstantIndex::EXEC_INDEX, factory->NewFromASCII("exec"));
    SetConstant(ConstantIndex::LAST_INDEX_INDEX, factory->NewFromASCII("lastIndex"));
    SetConstant(ConstantIndex::PLURAL_CATEGORIES_INDEX, factory->NewFromASCII("pluralCategories"));
    SetConstant(ConstantIndex::SORT_INDEX, factory->NewFromASCII("sort"));
    SetConstant(ConstantIndex::SEARCH_INDEX, factory->NewFromASCII("search"));
    SetConstant(ConstantIndex::BASE_INDEX, factory->NewFromASCII("base"));
    SetConstant(ConstantIndex::ACCENT_INDEX, factory->NewFromASCII("accent"));
    SetConstant(ConstantIndex::CASE_INDEX, factory->NewFromASCII("case"));
    SetConstant(ConstantIndex::VARIANT_INDEX, factory->NewFromASCII("variant"));
    SetConstant(ConstantIndex::EN_US_POSIX_STRING_INDEX, factory->NewFromASCII("en-US-POSIX"));
    SetConstant(ConstantIndex::UPPER_INDEX, factory->NewFromASCII("upper"));
    SetConstant(ConstantIndex::LOWER_INDEX, factory->NewFromASCII("lower"));
    SetConstant(ConstantIndex::DEFAULT_INDEX, factory->NewFromASCII("default"));
    SetConstant(ConstantIndex::SHARED_INDEX, factory->NewFromASCII("shared"));
    SetConstant(ConstantIndex::START_RANGE_INDEX, factory->NewFromASCII("startRange"));
    SetConstant(ConstantIndex::END_RANGE_INDEX, factory->NewFromASCII("endRange"));
    SetConstant(ConstantIndex::ISO8601_INDEX, factory->NewFromASCII("iso8601"));
    SetConstant(ConstantIndex::GREGORY_INDEX, factory->NewFromASCII("gregory"));
    SetConstant(ConstantIndex::ETHIOAA_INDEX, factory->NewFromASCII("ethioaa"));
    SetConstant(ConstantIndex::STICKY_INDEX, factory->NewFromASCII("sticky"));
    SetConstant(ConstantIndex::U_INDEX, factory->NewFromASCII("u"));
    SetConstant(ConstantIndex::INDEX_INDEX, factory->NewFromASCII("index"));
    SetConstant(ConstantIndex::INPUT_INDEX, factory->NewFromASCII("input"));
    SetConstant(ConstantIndex::UNICODE_INDEX, factory->NewFromASCII("unicode"));
    SetConstant(ConstantIndex::ZERO_INDEX, factory->NewFromASCII("0"));
    SetConstant(ConstantIndex::VALUES_INDEX, factory->NewFromASCII("values"));
    SetConstant(ConstantIndex::AMBIGUOUS_INDEX, factory->NewFromASCII("ambiguous"));
    SetConstant(ConstantIndex::MODULE_INDEX, factory->NewFromASCII("module"));
    SetConstant(ConstantIndex::STAR_INDEX, factory->NewFromASCII("*"));
    SetConstant(ConstantIndex::DATETIMEFIELD_INDEX, factory->NewFromASCII("datetimefield"));
    SetConstant(ConstantIndex::NONE_INDEX, factory->NewFromASCII("none"));
    SetConstant(ConstantIndex::FALLBACK_INDEX, factory->NewFromASCII("fallback"));

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
