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

#ifndef ECMASCRIPT_MEM_OBJECT_XRAY_H
#define ECMASCRIPT_MEM_OBJECT_XRAY_H

#include <cstdint>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_api_arraylist_iterator.h"
#include "ecmascript/js_api_deque.h"
#include "ecmascript/js_api_deque_iterator.h"
#include "ecmascript/js_api_lightweightmap.h"
#include "ecmascript/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api_lightweightset.h"
#include "ecmascript/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api_linked_list.h"
#include "ecmascript/js_api_linked_list_iterator.h"
#include "ecmascript/js_api_list.h"
#include "ecmascript/js_api_list_iterator.h"
#include "ecmascript/js_api_plain_array.h"
#include "ecmascript/js_api_plain_array_iterator.h"
#include "ecmascript/js_api_queue.h"
#include "ecmascript/js_api_queue_iterator.h"
#include "ecmascript/js_api_stack.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/js_api_tree_map.h"
#include "ecmascript/js_api_tree_map_iterator.h"
#include "ecmascript/js_api_tree_set.h"
#include "ecmascript/js_api_tree_set_iterator.h"
#include "ecmascript/js_api_vector.h"
#include "ecmascript/js_api_vector_iterator.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/ts_types/ts_type.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"

namespace panda::ecmascript {
class ObjectXRay {
public:
    explicit ObjectXRay(EcmaVM *ecmaVm) : ecmaVm_(ecmaVm) {}
    ~ObjectXRay() = default;

    inline void VisitVMRoots(const RootVisitor &visitor, const RootRangeVisitor &range_visitor) const
    {
        ecmaVm_->Iterate(visitor);
        ecmaVm_->GetJSThread()->Iterate(visitor, range_visitor);
    }
    template<VisitType visitType>
    inline void VisitObjectBody(TaggedObject *object, JSHClass *klass, const EcmaObjectRangeVisitor &visitor)
    {
        // handle body
        JSType type = klass->GetObjectType();
        switch (type) {
            case JSType::JS_OBJECT:
            case JSType::JS_ERROR:
            case JSType::JS_EVAL_ERROR:
            case JSType::JS_RANGE_ERROR:
            case JSType::JS_REFERENCE_ERROR:
            case JSType::JS_TYPE_ERROR:
            case JSType::JS_AGGREGATE_ERROR:
            case JSType::JS_URI_ERROR:
            case JSType::JS_SYNTAX_ERROR:
            case JSType::JS_ITERATOR:
                JSObject::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_GLOBAL_OBJECT:
                JSGlobalObject::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_FUNCTION_BASE: {
                auto jsFunctionBase = JSFunctionBase::Cast(object);
                jsFunctionBase->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsFunctionBase->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_FUNCTION: {
                auto jsFunction = JSFunction::Cast(object);
                jsFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_GENERATOR_FUNCTION: {
                auto jsGeneratorFunction = JSGeneratorFunction::Cast(object);
                jsGeneratorFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsGeneratorFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROXY_REVOC_FUNCTION: {
                auto jsProxyRevocFunction = JSProxyRevocFunction::Cast(object);
                jsProxyRevocFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsProxyRevocFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_REACTIONS_FUNCTION: {
                auto jsPromiseReactionsFunction = JSPromiseReactionsFunction::Cast(object);
                jsPromiseReactionsFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseReactionsFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_EXECUTOR_FUNCTION: {
                auto jsPromiseExecutorFunction = JSPromiseExecutorFunction::Cast(object);
                jsPromiseExecutorFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseExecutorFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION: {
                auto jsPromiseAllResolveElementFunction = JSPromiseAllResolveElementFunction::Cast(object);
                jsPromiseAllResolveElementFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseAllResolveElementFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION: {
                auto jsPromiseAnyRejectElementFunction = JSPromiseAnyRejectElementFunction::Cast(object);
                jsPromiseAnyRejectElementFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseAnyRejectElementFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION: {
                auto jsPromiseAllSettledElementFunction = JSPromiseAllSettledElementFunction::Cast(object);
                jsPromiseAllSettledElementFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseAllSettledElementFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_FINALLY_FUNCTION: {
                auto jsPromiseFinallyFunction = JSPromiseFinallyFunction::Cast(object);
                jsPromiseFinallyFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseFinallyFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION: {
                auto jsPromiseValueThunkOrThrowerFunction = JSPromiseValueThunkOrThrowerFunction::Cast(object);
                jsPromiseValueThunkOrThrowerFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsPromiseValueThunkOrThrowerFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_ASYNC_FUNCTION: {
                auto jsAsyncFunction = JSAsyncFunction::Cast(object);
                jsAsyncFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsAsyncFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION: {
                auto jsAsyncAwaitStatusFunction = JSAsyncAwaitStatusFunction::Cast(object);
                jsAsyncAwaitStatusFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsAsyncAwaitStatusFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_REG_EXP:
                JSRegExp::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_SET:
                JSSet::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_MAP:
                JSMap::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_WEAK_MAP:
                JSWeakMap::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_WEAK_SET:
                JSWeakSet::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_WEAK_REF:
                JSWeakRef::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_FINALIZATION_REGISTRY:
                JSFinalizationRegistry::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::CELL_RECORD:
                CellRecord::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_DATE:
                JSDate::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_FORIN_ITERATOR:
                JSForInIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_MAP_ITERATOR:
                JSMapIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_SET_ITERATOR:
                JSSetIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_REG_EXP_ITERATOR:
                JSRegExpIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_ARRAY_ITERATOR:
                JSArrayIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_STRING_ITERATOR:
                JSStringIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_ARRAY_BUFFER:
                JSArrayBuffer::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_SHARED_ARRAY_BUFFER:
                JSArrayBuffer::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_PROMISE:
                JSPromise::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_DATA_VIEW:
                JSDataView::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_BOUND_FUNCTION: {
                auto jsBoundFunction = JSBoundFunction::Cast(object);
                jsBoundFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsBoundFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_ARGUMENTS:
                JSArguments::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_GENERATOR_OBJECT:
                JSGeneratorObject::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_ASYNC_FUNC_OBJECT:
                JSAsyncFuncObject::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_ARRAY:
                JSArray::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_TYPED_ARRAY:
            case JSType::JS_INT8_ARRAY:
            case JSType::JS_UINT8_ARRAY:
            case JSType::JS_UINT8_CLAMPED_ARRAY:
            case JSType::JS_INT16_ARRAY:
            case JSType::JS_UINT16_ARRAY:
            case JSType::JS_INT32_ARRAY:
            case JSType::JS_UINT32_ARRAY:
            case JSType::JS_FLOAT32_ARRAY:
            case JSType::JS_FLOAT64_ARRAY:
            case JSType::JS_BIGINT64_ARRAY:
            case JSType::JS_BIGUINT64_ARRAY:
                JSTypedArray::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_PRIMITIVE_REF:
                JSPrimitiveRef::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_PROXY: {
                auto jsProxy = JSProxy::Cast(object);
                jsProxy->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsProxy->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::HCLASS:
                // semi gc is not needed to visit dyn class
                if (visitType != VisitType::SEMI_GC_VISIT) {
                    JSHClass::Cast(object)->VisitRangeSlot(visitor);
                }
                break;
            case JSType::STRING:
                break;
            case JSType::JS_NATIVE_POINTER:
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    JSNativePointer::Cast(object)->VisitRangeSlotForNative(visitor);
                }
                break;
            case JSType::TAGGED_ARRAY:
            case JSType::TAGGED_DICTIONARY:
            case JSType::TEMPLATE_MAP:
                TaggedArray::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::GLOBAL_ENV:
                GlobalEnv::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::ACCESSOR_DATA:
            case JSType::INTERNAL_ACCESSOR:
                AccessorData::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::SYMBOL:
                JSSymbol::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_GENERATOR_CONTEXT:
                GeneratorContext::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROTOTYPE_HANDLER:
                PrototypeHandler::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TRANSITION_HANDLER:
                TransitionHandler::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROPERTY_BOX:
                PropertyBox::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROTO_CHANGE_MARKER:
                break;
            case JSType::PROTOTYPE_INFO:
                ProtoChangeDetails::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROMISE_CAPABILITY:
                PromiseCapability::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROMISE_RECORD:
                PromiseRecord::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::RESOLVING_FUNCTIONS_RECORD:
                ResolvingFunctionsRecord::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROMISE_REACTIONS:
                PromiseReaction::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROMISE_ITERATOR_RECORD:
                PromiseIteratorRecord::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::MICRO_JOB_QUEUE:
                job::MicroJobQueue::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PENDING_JOB:
                job::PendingJob::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::COMPLETION_RECORD:
                CompletionRecord::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::PROGRAM:
                Program::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_INTL:
                JSIntl::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_NUMBER_FORMAT:
                JSNumberFormat::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_LOCALE:
                JSLocale::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_DATE_TIME_FORMAT:
                JSDateTimeFormat::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_RELATIVE_TIME_FORMAT:
                JSRelativeTimeFormat::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_INTL_BOUND_FUNCTION: {
                auto jsIntlBoundFunction = JSIntlBoundFunction::Cast(object);
                jsIntlBoundFunction->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    jsIntlBoundFunction->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_REALM:
                JSRealm::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_COLLATOR:
                JSCollator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_PLURAL_RULES:
                JSPluralRules::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_DISPLAYNAMES:
                JSDisplayNames::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_LIST_FORMAT:
                JSListFormat::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::MACHINE_CODE_OBJECT:
                MachineCode::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::CLASS_INFO_EXTRACTOR: {
                auto classInfoExtractor = ClassInfoExtractor::Cast(object);
                classInfoExtractor->VisitRangeSlot(visitor);
                if (visitType == VisitType::SNAPSHOT_VISIT) {
                    classInfoExtractor->VisitRangeSlotForNative(visitor);
                }
                break;
            }
            case JSType::JS_API_QUEUE:
                JSAPIQueue::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_QUEUE_ITERATOR:
                JSAPIQueueIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_ARRAY_LIST:
                JSAPIArrayList::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_ARRAYLIST_ITERATOR:
                JSAPIArrayListIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_MAP:
                JSAPILightWeightMap::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
                JSAPILightWeightMapIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_SET:
                JSAPILightWeightSet::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
                JSAPILightWeightSetIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_OBJECT_TYPE:
                TSObjectType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_CLASS_TYPE:
                TSClassType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_UNION_TYPE:
                TSUnionType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_INTERFACE_TYPE:
                TSInterfaceType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_IMPORT_TYPE:
                TSImportType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_CLASS_INSTANCE_TYPE:
                break;
            case JSType::TS_FUNCTION_TYPE:
                TSFunctionType::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::TS_ARRAY_TYPE:
                break;
            case JSType::JS_API_TREE_MAP:
                JSAPITreeMap::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_TREE_SET:
                JSAPITreeSet::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_TREEMAP_ITERATOR:
                JSAPITreeMapIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_TREESET_ITERATOR:
                JSAPITreeSetIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_PLAIN_ARRAY:
                JSAPIPlainArray::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
                JSAPIPlainArrayIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_DEQUE:
                JSAPIDeque::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_DEQUE_ITERATOR:
                JSAPIDequeIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_STACK:
                JSAPIStack::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_STACK_ITERATOR:
                JSAPIStackIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_VECTOR:
                JSAPIVector::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_VECTOR_ITERATOR:
                JSAPIVectorIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIST:
                JSAPIList::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LINKED_LIST:
                JSAPILinkedList::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LIST_ITERATOR:
                JSAPIListIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_API_LINKED_LIST_ITERATOR:
                JSAPILinkedListIterator::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::BIGINT:
                break;
            case JSType::SOURCE_TEXT_MODULE_RECORD:
                SourceTextModule::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::IMPORTENTRY_RECORD:
                ImportEntry::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::EXPORTENTRY_RECORD:
                ExportEntry::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::RESOLVEDBINDING_RECORD:
                ResolvedBinding::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_MODULE_NAMESPACE:
                ModuleNamespace::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_CJS_EXPORTS:
                CjsExports::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_CJS_MODULE:
                CjsModule::Cast(object)->VisitRangeSlot(visitor);
                break;
            case JSType::JS_CJS_REQUIRE:
                CjsRequire::Cast(object)->VisitRangeSlot(visitor);
                break;
            default:
                UNREACHABLE();
        }
    }

private:
    EcmaVM *ecmaVm_ {nullptr};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_OBJECT_XRAY_H
