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

#ifndef ECMASCRIPT_GLOBAL_ENV_H
#define ECMASCRIPT_GLOBAL_ENV_H

#include "ecmascript/js_global_object.h"
#include "ecmascript/js_function.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/global_env_constants-inl.h"

namespace panda::ecmascript {
class JSThread;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_FIELDS(V)                                                                        \
    /* Function */                                                                                  \
    V(JSTaggedValue, ObjectFunction, OBJECT_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, ObjectFunctionPrototype, OBJECT_FUNCTION_PROTOTYPE_INDEX)                      \
    V(JSTaggedValue, ObjectFunctionPrototypeClass, OBJECT_FUNCTION_PROTOTYPE_CLASS_INDEX)           \
    V(JSTaggedValue, FunctionFunction, FUNCTION_FUNCTION_INDEX)                                     \
    V(JSTaggedValue, FunctionPrototype, FUNCTION_PROTOTYPE_INDEX)                                   \
    V(JSTaggedValue, NumberFunction, NUMBER_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, BigIntFunction, BIGINT_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, DateFunction, DATE_FUNCTION_INDEX)                                             \
    V(JSTaggedValue, BooleanFunction, BOOLEAN_FUNCTION_INDEX)                                       \
    V(JSTaggedValue, ErrorFunction, ERROR_FUNCTION_INDEX)                                           \
    V(JSTaggedValue, ArrayFunction, ARRAY_FUNCTION_INDEX)                                           \
    V(JSTaggedValue, ArrayPrototype, ARRAY_PROTOTYPE_INDEX)                                         \
    V(JSTaggedValue, TypedArrayFunction, TYPED_ARRAY_FUNCTION_INDEX)                                \
    V(JSTaggedValue, TypedArrayPrototype, TYPED_ARRAY_PROTOTYPE_INDEX)                              \
    V(JSTaggedValue, Int8ArrayFunction, INT8_ARRAY_FUNCTION_INDEX)                                  \
    V(JSTaggedValue, Uint8ArrayFunction, UINT8_ARRAY_FUNCTION_INDEX)                                \
    V(JSTaggedValue, Uint8ClampedArrayFunction, UINT8_CLAMPED_ARRAY_FUNCTION_INDEX)                 \
    V(JSTaggedValue, Int16ArrayFunction, INT16_ARRAY_FUNCTION_INDEX)                                \
    V(JSTaggedValue, Uint16ArrayFunction, UINT16_ARRAY_FUNCTION_INDEX)                              \
    V(JSTaggedValue, Int32ArrayFunction, INT32_ARRAY_FUNCTION_INDEX)                                \
    V(JSTaggedValue, Uint32ArrayFunction, UINT32_ARRAY_FUNCTION_INDEX)                              \
    V(JSTaggedValue, Float32ArrayFunction, FLOAT32_ARRAY_FUNCTION_INDEX)                            \
    V(JSTaggedValue, Float64ArrayFunction, FLOAT64_ARRAY_FUNCTION_INDEX)                            \
    V(JSTaggedValue, BigInt64ArrayFunction, BIGINT64_ARRAY_FUNCTION_INDEX)                          \
    V(JSTaggedValue, BigUint64ArrayFunction, BIGUINT64_ARRAY_FUNCTION_INDEX)                        \
    V(JSTaggedValue, ArrayBufferFunction, ARRAY_BUFFER_FUNCTION_INDEX)                              \
    V(JSTaggedValue, SharedArrayBufferFunction, SHAREDARRAY_BUFFER_FUNCTION_INDEX)                  \
    V(JSTaggedValue, ArrayProtoValuesFunction, ARRAY_PROTO_VALUES_FUNCTION_INDEX)                   \
    V(JSTaggedValue, DataViewFunction, DATA_VIEW_FUNCTION_INDEX)                                    \
    V(JSTaggedValue, SymbolFunction, SYMBOL_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, RangeErrorFunction, RANGE_ERROR_FUNCTION_INDEX)                                \
    V(JSTaggedValue, ReferenceErrorFunction, REFERENCE_ERROR_FUNCTION_INDEX)                        \
    V(JSTaggedValue, TypeErrorFunction, TYPE_ERROR_FUNCTION_INDEX)                                  \
    V(JSTaggedValue, URIErrorFunction, URI_ERROR_FUNCTION_INDEX)                                    \
    V(JSTaggedValue, SyntaxErrorFunction, SYNTAX_ERROR_FUNCTION_INDEX)                              \
    V(JSTaggedValue, EvalErrorFunction, EVAL_ERROR_FUNCTION_INDEX)                                  \
    V(JSTaggedValue, IntlFunction, INTL_FUNCTION_INDEX)                                             \
    V(JSTaggedValue, LocaleFunction, LOCALE_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, DateTimeFormatFunction, DATE_TIME_FORMAT_FUNCTION_INDEX)                       \
    V(JSTaggedValue, RelativeTimeFormatFunction, RELATIVE_TIME_FORMAT_FUNCTION_INDEX)               \
    V(JSTaggedValue, NumberFormatFunction, NUMBER_FORMAT_FUNCTION_INDEX)                            \
    V(JSTaggedValue, CollatorFunction, COLLATOR_FUNCTION_INDEX)                                     \
    V(JSTaggedValue, PluralRulesFunction, PLURAL_RULES_FUNCTION_INDEX)                              \
    V(JSTaggedValue, DisplayNamesFunction, DISPLAY_NAMES_FUNCTION_INDEX)                            \
    V(JSTaggedValue, ListFormatFunction, LIST_FORMAT_FUNCTION_INDEX)                                \
    V(JSTaggedValue, RegExpFunction, REGEXP_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, BuiltinsSetFunction, BUILTINS_SET_FUNCTION_INDEX)                              \
    V(JSTaggedValue, SetPrototype, SET_PROTOTYPE_INDEX)                                             \
    V(JSTaggedValue, BuiltinsMapFunction, BUILTINS_MAP_FUNCTION_INDEX)                              \
    V(JSTaggedValue, BuiltinsWeakMapFunction, BUILTINS_WEAK_MAP_FUNCTION_INDEX)                     \
    V(JSTaggedValue, BuiltinsWeakSetFunction, BUILTINS_WEAK_SET_FUNCTION_INDEX)                     \
    V(JSTaggedValue, BuiltinsWeakRefFunction, BUILTINS_WEAK_REF_FUNCTION_INDEX)                     \
    V(JSTaggedValue, BuiltinsFinalizationRegistryFunction, BUILTINS_FINALIZATION_REGISTRY_FUNCTION_INDEX) \
    V(JSTaggedValue, MapPrototype, MAP_PROTOTYPE_INDEX)                                             \
    V(JSTaggedValue, MathFunction, MATH_FUNCTION_INDEX)                                             \
    V(JSTaggedValue, AtomicsFunction, ATOMICS_FUNCTION_INDEX)                                       \
    V(JSTaggedValue, JsonFunction, JSON_FUNCTION_INDEX)                                             \
    V(JSTaggedValue, StringFunction, STRING_FUNCTION_INDEX)                                         \
    V(JSTaggedValue, ProxyFunction, PROXY_FUNCTION_INDEX)                                           \
    V(JSTaggedValue, GeneratorFunctionFunction, GENERATOR_FUNCTION_OFFSET)                          \
    V(JSTaggedValue, GeneratorFunctionPrototype, GENERATOR_FUNCTION_PROTOTYPE_OFFSET)               \
    V(JSTaggedValue, InitialGenerator, INITIAL_GENERATOR_OFFSET)                                    \
    V(JSTaggedValue, GeneratorPrototype, GENERATOR_PROTOTYPE_OFFSET)                                \
    V(JSTaggedValue, ReflectFunction, REFLECT_FUNCTION_INDEX)                                       \
    V(JSTaggedValue, AsyncFunction, ASYNC_FUNCTION_INDEX)                                           \
    V(JSTaggedValue, AsyncFunctionPrototype, ASYNC_FUNCTION_PROTOTYPE_INDEX)                        \
    V(JSTaggedValue, JSGlobalObject, JS_GLOBAL_OBJECT_INDEX)                                        \
    V(JSTaggedValue, HasInstanceSymbol, HASINSTANCE_SYMBOL_INDEX)                                   \
    V(JSTaggedValue, IsConcatSpreadableSymbol, ISCONCAT_SYMBOL_INDEX)                               \
    V(JSTaggedValue, ToStringTagSymbol, TOSTRINGTAG_SYMBOL_INDEX)                                   \
    V(JSTaggedValue, IteratorSymbol, ITERATOR_SYMBOL_INDEX)                                         \
    V(JSTaggedValue, MatchSymbol, MATCH_SYMBOL_INDEX)                                               \
    V(JSTaggedValue, MatchAllSymbol, MATCH_All_SYMBOL_INDEX)                                        \
    V(JSTaggedValue, ReplaceSymbol, REPLACE_SYMBOL_INDEX)                                           \
    V(JSTaggedValue, SearchSymbol, SEARCH_SYMBOL_INDEX)                                             \
    V(JSTaggedValue, SpeciesSymbol, SPECIES_SYMBOL_INDEX)                                           \
    V(JSTaggedValue, SplitSymbol, SPLIT_SYMBOL_INDEX)                                               \
    V(JSTaggedValue, ToPrimitiveSymbol, TOPRIMITIVE_SYMBOL_INDEX)                                   \
    V(JSTaggedValue, UnscopablesSymbol, UNSCOPABLES_SYMBOL_INDEX)                                   \
    V(JSTaggedValue, HoleySymbol, HOLEY_SYMBOL_OFFSET)                                              \
    V(JSTaggedValue, ElementICSymbol, ELEMENT_IC_SYMBOL_OFFSET)                                     \
    V(JSTaggedValue, IteratorPrototype, ITERATOR_PROTOTYPE_INDEX)                                   \
    V(JSTaggedValue, ForinIteratorPrototype, FORIN_ITERATOR_PROTOTYPE_INDEX)                        \
    V(JSTaggedValue, ForinIteratorClass, FOR_IN_ITERATOR_CLASS_INDEX)                               \
    V(JSTaggedValue, StringIterator, STRING_ITERATOR_INDEX)                                         \
    V(JSTaggedValue, MapIteratorPrototype, MAP_ITERATOR_PROTOTYPE_INDEX)                            \
    V(JSTaggedValue, SetIteratorPrototype, SET_ITERATOR_PROTOTYPE_INDEX)                            \
    V(JSTaggedValue, RegExpIteratorPrototype, REGEXP_ITERATOR_PROTOTYPE_INDEX)                      \
    V(JSTaggedValue, ArrayIteratorPrototype, ARRAY_ITERATOR_PROTOTYPE_INDEX)                        \
    V(JSTaggedValue, StringIteratorPrototype, STRING_ITERATOR_PROTOTYPE_INDEX)                      \
    /* SymbolTable *RegisterSymbols */                                                              \
    V(JSTaggedValue, RegisterSymbols, SYMBOLS_INDEX)                                                \
    V(JSTaggedValue, ThrowTypeError, THROW_TYPE_ERROR_INDEX)                                        \
    V(JSTaggedValue, PromiseFunction, PROMISE_FUNCTION_INDEX)                                       \
    V(JSTaggedValue, PromiseReactionJob, PROMISE_REACTION_JOB_INDEX)                                \
    V(JSTaggedValue, PromiseResolveThenableJob, PROMISE_REACTION_THENABLE_JOB_INDEX)                \
    V(JSTaggedValue, TemplateMap, TEMPLATE_MAP_INDEX)                                               \
    V(JSTaggedValue, FunctionClassWithProto, FUNCTION_CLASS_WITH_PROTO)                             \
    V(JSTaggedValue, FunctionClassWithoutProto, FUNCTION_CLASS_WITHOUT_PROTO)                       \
    V(JSTaggedValue, FunctionClassWithoutName, FUNCTION_CLASS_WITHOUT_NAME)                         \
    V(JSTaggedValue, ArgumentsClass, ARGUMENTS_CLASS)                                               \
    V(JSTaggedValue, ArgumentsCallerAccessor, ARGUMENTSCALLERACCESSOR)                              \
    V(JSTaggedValue, ArgumentsCalleeAccessor, ARGUMENTSCALLEEACCESSOR)                              \
    V(JSTaggedValue, AsyncFunctionClass, ASYNC_FUNCTION_CLASS)                                      \
    V(JSTaggedValue, AsyncAwaitStatusFunctionClass, ASYNC_AWAIT_STATUS_FUNCTION_CLASS)              \
    V(JSTaggedValue, PromiseReactionFunctionClass, PROMISE_REACTION_FUNCTION_CLASS)                 \
    V(JSTaggedValue, PromiseExecutorFunctionClass, PROMISE_EXECUTOR_FUNCTION_CLASS)                 \
    V(JSTaggedValue, GeneratorFunctionClass, GENERATOR_FUNCTION_CLASS)                              \
    V(JSTaggedValue, PromiseAllResolveElementFunctionClass, PROMISE_ALL_RESOLVE_ELEMENT_FUNC_CLASS) \
    V(JSTaggedValue, ProxyRevocFunctionClass, PROXY_REVOC_FUNCTION_CLASS)                           \
    V(JSTaggedValue, NativeErrorFunctionClass, NATIVE_ERROR_FUNCTION_CLASS)                         \
    V(JSTaggedValue, SpecificTypedArrayFunctionClass, SPERCIFIC_TYPED_ARRAY_FUNCTION_CLASS)         \
    V(JSTaggedValue, ConstructorFunctionClass, CONSTRUCTOR_FUNCTION_CLASS)                          \
    V(JSTaggedValue, NormalFunctionClass, NORMAL_FUNCTION_CLASS)                                    \
    V(JSTaggedValue, JSIntlBoundFunctionClass, JS_INTL_BOUND_FUNCTION_CLASS)                        \
    V(JSTaggedValue, NumberFormatLocales, NUMBER_FORMAT_LOCALES_INDEX)                              \
    V(JSTaggedValue, DateTimeFormatLocales, DATE_TIMEFORMAT_LOCALES_INDEX)                          \
    V(JSTaggedValue, ListFormatLocales, LIST_FORMAT_LOCALES_INDEX)                                  \
    V(JSTaggedValue, GlobalRecord, GLOBAL_RECORD)                                                   \
    V(JSTaggedValue, ModuleNamespaceClass, MODULENAMESPACE_CLASS)                                   \
    V(JSTaggedValue, ObjectLiteralHClassCache, OBJECT_LITERAL_HCLASS_CACHE)                         \
    V(JSTaggedValue, WeakRefKeepObjects, WEAK_REF_KEEP_OBJECTS)                                     \
    V(JSTaggedValue, FinRegLists, FIN_REG_LISTS)                                                    \
    V(JSTaggedValue, AttachSymbol, ATTACH_SYMBOL_INDEX)                                             \
    V(JSTaggedValue, DetachSymbol, DETACH_SYMBOL_INDEX)                                             \
    V(JSTaggedValue, CjsModuleFunction, CJS_MODULE_FUNCTION_INDEX)                                  \
    V(JSTaggedValue, CjsExportsFunction, CJS_EXPORTS_FUNCTION_INDEX)                                \
    V(JSTaggedValue, CjsRequireFunction, CJS_REQUIRE_FUNCTION_INDEX)

class GlobalEnv : public TaggedObject {
public:
    JSTaggedValue GetGlobalObject() const
    {
        return GetJSGlobalObject().GetTaggedValue();
    }

    uintptr_t ComputeObjectAddress(size_t index) const
    {
        return reinterpret_cast<uintptr_t>(this) + HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();
    }

    JSHandle<JSTaggedValue> GetGlobalEnvObjectByIndex(size_t index) const
    {
        ASSERT(index < FINAL_INDEX);
        uintptr_t address = ComputeObjectAddress(index);
        JSHandle<JSTaggedValue> result(address);
        return result;
    }

    size_t GetGlobalEnvFieldSize() const
    {
        return FINAL_INDEX;
    }

    void Init(JSThread *thread);
    void InitGlobalObject();

    static GlobalEnv *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsJSGlobalEnv());
        return reinterpret_cast<GlobalEnv *>(object);
    }

    JSHandle<JSTaggedValue> GetSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &string);
    JSHandle<JSTaggedValue> GetStringFunctionByName(JSThread *thread, const char *name);
    JSHandle<JSTaggedValue> GetStringPrototypeFunctionByName(JSThread *thread, const char *name);

    enum Field {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_SLOT(type, name, index) index,
        GLOBAL_ENV_FIELDS(GLOBAL_ENV_SLOT)
#undef GLOBAL_ENV_SLOT
            FINAL_INDEX
    };

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GLOBAL_ENV_FIELD_ACCESSORS(type, name, index)                                                   \
    inline JSHandle<type> Get##name() const                                                             \
    {                                                                                                   \
        const uintptr_t address =                                                                       \
            reinterpret_cast<uintptr_t>(this) + HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();  \
        JSHandle<type> result(address);                                                                 \
        return result;                                                                                  \
    }                                                                                                   \
    template<typename T>                                                                                \
    inline void Set##name(const JSThread *thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)  \
    {                                                                                                   \
        uint32_t offset = HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();                        \
        if (mode == WRITE_BARRIER && value.GetTaggedValue().IsHeapObject()) {                           \
            Barriers::SetDynObject<true>(thread, this, offset, value.GetTaggedValue().GetRawData());    \
        } else {                                                                                        \
            Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData()); \
        }                                                                                               \
    }                                                                                                   \
    inline void Set##name(const JSThread *thread, type value, BarrierMode mode = WRITE_BARRIER)         \
    {                                                                                                   \
        uint32_t offset = HEADER_SIZE + index * JSTaggedValue::TaggedTypeSize();                        \
        if (mode == WRITE_BARRIER && value.IsHeapObject()) {                                            \
            Barriers::SetDynObject<true>(thread, this, offset, value.GetRawData());                     \
        } else {                                                                                        \
            Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetRawData());                  \
        }                                                                                               \
    }
    GLOBAL_ENV_FIELDS(GLOBAL_ENV_FIELD_ACCESSORS)
#undef GLOBAL_ENV_FIELD_ACCESSORS

    static constexpr size_t HEADER_SIZE = TaggedObjectSize();
    static constexpr size_t SIZE = HEADER_SIZE + FINAL_INDEX * JSTaggedValue::TaggedTypeSize();

    DECL_VISIT_OBJECT(HEADER_SIZE, SIZE);

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GLOBAL_ENV_H
