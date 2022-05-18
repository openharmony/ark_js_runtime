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

#ifndef ECMASCRIPT_RUNTIME_TRAMPOLINES_INL_H
#define ECMASCRIPT_RUNTIME_TRAMPOLINES_INL_H

#include "runtime_stubs.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_helper.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/template_string.h"
#include "ecmascript/jspandafile/scope_info_extractor.h"

namespace panda::ecmascript {
static constexpr size_t FIXED_NUM_ARGS = 3;

JSTaggedValue RuntimeStubs::RuntimeIncDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintAddOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(++number);
}

JSTaggedValue RuntimeStubs::RuntimeDecDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BigintSubOne(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    return JSTaggedValue(--number);
}

JSTaggedValue RuntimeStubs::RuntimeExpDyn(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent)
{
    JSHandle<JSTaggedValue> baseTag(thread, base);
    JSHandle<JSTaggedValue> exponentTag(thread, exponent);
    JSHandle<JSTaggedValue> valBase = JSTaggedValue::ToNumeric(thread, baseTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valExponent = JSTaggedValue::ToNumeric(thread, exponentTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valBase->IsBigInt() || valExponent->IsBigInt()) {
        if (valBase->IsBigInt() && valExponent->IsBigInt()) {
            JSHandle<BigInt> bigBaseVale(valBase);
            JSHandle<BigInt> bigExponentValue(valExponent);
            return BigInt::Exponentiate(thread, bigBaseVale, bigExponentValue).GetTaggedValue();
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot mix BigInt and other types, use explicit conversions",
                                    JSTaggedValue::Exception());
    }
    double doubleBase = valBase->GetNumber();
    double doubleExponent = valExponent->GetNumber();
    if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if (((doubleBase == 0) && ((bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
        std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
        base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF == (doubleExponent / 2)) {  // 2: half
        if (doubleExponent > 0) {
            return JSTaggedValue(-0.0);
        }
        if (doubleExponent < 0) {
            return JSTaggedValue(-base::POSITIVE_INFINITY);
        }
    }
    return JSTaggedValue(std::pow(doubleBase, doubleExponent));
}

JSTaggedValue RuntimeStubs::RuntimeIsInDyn(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                           const JSHandle<JSTaggedValue> &obj)
{
    if (!obj->IsECMAObject()) {
        return RuntimeThrowTypeError(thread, "Cannot use 'in' operator in Non-Object");
    }
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::HasProperty(thread, obj, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeInstanceofDyn(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                 const JSHandle<JSTaggedValue> &target)
{
    bool ret = JSObject::InstanceOf(thread, obj, target);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeCreateGeneratorObj(JSThread *thread, const JSHandle<JSTaggedValue> &genFunc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSGeneratorObject> obj = factory->NewJSGeneratorObject(genFunc);
    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, obj.GetTaggedValue());

    // change state to SUSPENDED_START
    obj->SetGeneratorState(JSGeneratorState::SUSPENDED_START);
    obj->SetGeneratorContext(thread, context);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetTemplateObject(JSThread *thread, const JSHandle<JSTaggedValue> &literal)
{
    JSHandle<JSTaggedValue> templateObj = TemplateString::GetTemplateObject(thread, literal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return templateObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetNextPropName(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    ASSERT(iter->IsForinIterator());
    std::pair<JSTaggedValue, bool> res =
        JSForInIterator::NextInternal(thread, JSHandle<JSForInIterator>::Cast(iter));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.first;
}

JSTaggedValue RuntimeStubs::RuntimeIterNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    JSHandle<JSObject> resultObj = JSIterator::IteratorNext(thread, iter);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return resultObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCloseIterator(JSThread *thread, const JSHandle<JSTaggedValue> &iter)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> record;
    if (thread->HasPendingException()) {
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(
            CompletionRecordType::THROW, JSHandle<JSTaggedValue>(thread, thread->GetException())));
    } else {
        JSHandle<JSTaggedValue> undefinedVal = globalConst->GetHandledUndefined();
        record = JSHandle<JSTaggedValue>(factory->NewCompletionRecord(CompletionRecordType::NORMAL, undefinedVal));
    }
    JSHandle<JSTaggedValue> result = JSIterator::IteratorClose(thread, iter, record);
    if (result->IsCompletionRecord()) {
        return CompletionRecord::Cast(result->GetTaggedObject())->GetValue();
    }
    return result.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &newTarget,
                                                   const JSHandle<JSTaggedValue> &array)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    ASSERT(superFunc->IsJSFunction());

    JSHandle<TaggedArray> argv(thread, RuntimeGetCallSpreadArgs(thread, array.GetTaggedValue()));
    const size_t argsLength = argv->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, argsLength);
    info.SetCallArg(argsLength, argv);
    JSTaggedValue result = JSFunction::Construct(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue RuntimeStubs::RuntimeDelObjProp(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                              const JSHandle<JSTaggedValue> &prop)
{
    JSHandle<JSTaggedValue> jsObj(JSTaggedValue::ToObject(thread, obj));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool ret = JSTaggedValue::DeletePropertyOrThrow(thread, jsObj, propKey);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(ret);
}

JSTaggedValue RuntimeStubs::RuntimeNewObjSpreadDyn(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                   const JSHandle<JSTaggedValue> &newTarget,
                                                   const JSHandle<JSTaggedValue> &array)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!array->IsJSArray()) {
        return RuntimeThrowTypeError(thread, "Cannot Newobjspread");
    }

    uint32_t length = JSHandle<JSArray>::Cast(array)->GetArrayLength();
    JSHandle<TaggedArray> argsArray = factory->NewTaggedArray(length);
    for (uint32_t i = 0; i < length; ++i) {
        auto prop = JSTaggedValue::GetProperty(thread, array, i).GetValue();
        argsArray->Set(thread, i, prop);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, newTarget, length);
    info.SetCallArg(length, argsArray);
    return SlowRuntimeHelper::NewObject(&info);
}

JSTaggedValue RuntimeStubs::RuntimeCreateIterResultObj(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                       JSTaggedValue flag)
{
    ASSERT(flag.IsBoolean());
    bool done = flag.IsTrue();
    JSHandle<JSObject> iter = JSIterator::CreateIterResultObject(thread, value, done);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionAwaitUncaught(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                              const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(asyncFuncObj);
    JSAsyncFunction::AsyncFunctionAwait(thread, asyncFuncObjHandle, value);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionResolveOrReject(JSThread *thread,
    const JSHandle<JSTaggedValue> &asyncFuncObj, const JSHandle<JSTaggedValue> &value, bool is_resolve)
{
    JSHandle<JSAsyncFuncObject> asyncFuncObjHandle(asyncFuncObj);
    JSHandle<JSPromise> promise(thread, asyncFuncObjHandle->GetPromise());

    // ActivePromise
    JSHandle<ResolvingFunctionsRecord> reactions = JSPromise::CreateResolvingFunctions(thread, promise);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisArg = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> activeFunc;
    if (is_resolve) {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetResolveFunction());
    } else {
        activeFunc = JSHandle<JSTaggedValue>(thread, reactions->GetRejectFunction());
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, activeFunc, thisArg, undefined, 1);
    info.SetCallArg(value.GetTaggedValue());
    [[maybe_unused]] JSTaggedValue res = JSFunction::Call(&info);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return promise.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCopyDataProperties(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                      const JSHandle<JSTaggedValue> &src)
{
    if (!src->IsNull() && !src->IsUndefined()) {
        JSHandle<TaggedArray> keys = JSTaggedValue::GetOwnPropertyKeys(thread, src);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        uint32_t keysLen = keys->GetLength();
        for (uint32_t i = 0; i < keysLen; i++) {
            PropertyDescriptor desc(thread);
            key.Update(keys->Get(i));
            bool success = JSTaggedValue::GetOwnProperty(thread, src, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

            if (success && desc.IsEnumerable()) {
                JSTaggedValue::DefineOwnProperty(thread, dst, key, desc);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return dst.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeStArraySpread(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                 JSTaggedValue index, const JSHandle<JSTaggedValue> &src)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ASSERT(dst->IsJSArray() && !src->IsNull() && !src->IsUndefined());
    if (src->IsString()) {
        JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, src);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        uint32_t dstLen = static_cast<uint32_t>(index.GetInt());
        uint32_t strLen = srcString->GetLength();
        for (uint32_t i = 0; i < strLen; i++) {
            uint16_t res = srcString->At<false>(i);
            JSHandle<JSTaggedValue> strValue(factory->NewFromUtf16Literal(&res, 1));
            JSTaggedValue::SetProperty(thread, dst, dstLen + i, strValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue(dstLen + strLen);
    }

    JSHandle<JSTaggedValue> iter;
    auto globalConst = thread->GlobalConstants();
    if (src->IsJSArrayIterator() || src->IsJSMapIterator() || src->IsJSSetIterator() ||
        src->IsIterator()) {
        iter = src;
    } else if (src->IsJSArray() || src->IsJSMap() || src->IsTypedArray() || src->IsJSSet()) {
        JSHandle<JSTaggedValue> valuesStr = globalConst->GetHandledValuesString();
        JSHandle<JSTaggedValue> valuesMethod = JSObject::GetMethod(thread, src, valuesStr);
        iter = JSIterator::GetIterator(thread, src, valuesMethod);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        iter = JSIterator::GetIterator(thread, src);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSMutableHandle<JSTaggedValue> indexHandle(thread, index);
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    PropertyDescriptor desc(thread);
    JSHandle<JSTaggedValue> iterResult;
    do {
        iterResult = JSIterator::IteratorStep(thread, iter);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (iterResult->IsFalse()) {
            break;
        }
        bool success = JSTaggedValue::GetOwnProperty(thread, iterResult, valueStr, desc);
        if (success && desc.IsEnumerable()) {
            JSTaggedValue::DefineOwnProperty(thread, dst, indexHandle, desc);
            int tmp = indexHandle->GetInt();
            indexHandle.Update(JSTaggedValue(tmp + 1));
        }
    } while (true);

    return indexHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetIteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                   const JSHandle<JSTaggedValue> &method)
{
    ASSERT(obj->IsCallable());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, obj, undefined, 0);
    JSTaggedValue ret = JSFunction::Call(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!ret.IsECMAObject()) {
        return RuntimeThrowTypeError(thread, "the Iterator is not an ecmaobject.");
    }
    return ret;
}

JSTaggedValue RuntimeStubs::RuntimeSetObjectWithProto(JSThread *thread, const JSHandle<JSTaggedValue> &proto,
                                                      const JSHandle<JSObject> &obj)
{
    if (!proto->IsECMAObject() && !proto->IsNull()) {
        return JSTaggedValue::False();
    }
    JSObject::SetPrototype(thread, obj, proto);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop, bool callGetter,
                                                JSTaggedValue receiver)
{
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), obj);
    } else {
        JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        res = JSTaggedValue::GetProperty(thread, obj, propKey).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop,
                                                const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, prop));

    // strict mode is true
    JSTaggedValue::SetProperty(thread, obj, propKey, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    if (obj->IsClassConstructor() &&
        JSTaggedValue::SameValue(key, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    JSMutableHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, propKey, desc);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "StOwnByValue failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &key, JSTaggedValue thisFunc)
{
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj->IsUndefined()) {
        return RuntimeThrowReferenceError(thread, obj, "this is uninitialized.");
    }

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue res = JSTaggedValue::GetProperty(thread, superBase, propKey, obj).GetValue().GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &key,
                                                  const JSHandle<JSTaggedValue> &value, JSTaggedValue thisFunc)
{
    ASSERT(thisFunc.IsJSFunction());
    // get Homeobject form function
    JSHandle<JSTaggedValue> homeObject(thread, JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject());

    if (obj->IsUndefined()) {
        return RuntimeThrowReferenceError(thread, obj, "this is uninitialized.");
    }

    JSHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    JSHandle<JSTaggedValue> superBase(thread, JSTaggedValue::GetSuperBase(thread, homeObject));
    JSTaggedValue::RequireObjectCoercible(thread, superBase);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // check may_throw is false?
    JSTaggedValue::SetProperty(thread, superBase, propKey, value, obj, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeLdObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                uint32_t idx, bool callGetter, JSTaggedValue receiver)
{
    JSTaggedValue res;
    if (callGetter) {
        res = JSObject::CallGetter(thread, AccessorData::Cast(receiver.GetTaggedObject()), obj);
    } else {
        res = JSTaggedValue::GetProperty(thread, obj, idx).GetValue().GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

JSTaggedValue RuntimeStubs::RuntimeStObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                uint32_t idx, const JSHandle<JSTaggedValue> &value)
{
    JSTaggedValue::SetProperty(thread, obj, idx, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &idx,
                                                const JSHandle<JSTaggedValue> &value)
{
    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, idx, desc);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "SetOwnByIndex failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStGlobalRecord(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                  const JSHandle<JSTaggedValue> &value, bool isConst)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());

    // cross files global record name binding judgment
    int entry = dict->FindEntry(prop.GetTaggedValue());
    if (entry != -1) {
        return RuntimeThrowSyntaxError(thread, "Duplicate identifier");
    }

    PropertyAttributes attributes;
    if (isConst) {
        attributes.SetIsConstProps(true);
    }
    JSHandle<GlobalDictionary> dictHandle(thread, dict);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PropertyBox> box = factory->NewPropertyBox(value);
    PropertyBoxType boxType = value->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
    attributes.SetBoxType(boxType);

    dict = *GlobalDictionary::PutIfAbsent(thread, dictHandle, prop, JSHandle<JSTaggedValue>(box), attributes);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    env->SetGlobalRecord(thread, JSTaggedValue(dict));
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeNegDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::UnaryMinus(thread, bigValue).GetTaggedValue();
    }
    JSTaggedNumber number(inputVal.GetTaggedValue());
    if (number.IsInt()) {
        int32_t intValue = number.GetInt();
        if (intValue == 0) {
            return JSTaggedValue(-0.0);
        }
        return JSTaggedValue(-intValue);
    }
    if (number.IsDouble()) {
        return JSTaggedValue(-number.GetDouble());
    }
    UNREACHABLE();
}

JSTaggedValue RuntimeStubs::RuntimeNotDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> inputVal = JSTaggedValue::ToNumeric(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (inputVal->IsBigInt()) {
        JSHandle<BigInt> bigValue(inputVal);
        return BigInt::BitwiseNOT(thread, bigValue).GetTaggedValue();
    }
    int32_t number = JSTaggedValue::ToInt32(thread, inputVal);
    return JSTaggedValue(~number); // NOLINT(hicpp-signed-bitwise)
}

JSTaggedValue RuntimeStubs::RuntimeResolveClass(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                const JSHandle<TaggedArray> &literal,
                                                const JSHandle<JSTaggedValue> &base,
                                                const JSHandle<JSTaggedValue> &lexenv,
                                                const JSHandle<ConstantPool> &constpool)
{
    ASSERT(ctor.GetTaggedValue().IsClassConstructor());

    FrameHandler frameHandler(thread);
    JSTaggedValue currentFunc = frameHandler.GetFunction();
    JSHandle<JSTaggedValue> ecmaModule(thread, JSFunction::Cast(currentFunc.GetTaggedObject())->GetModule());

    RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(ctor), base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t literalBufferLength = literal->GetLength();

    // only traverse the value of key-value pair
    for (uint32_t index = 1; index < literalBufferLength - 1; index += 2) {  // 2: key-value pair
        JSTaggedValue value = literal->Get(index);
        if (LIKELY(value.IsJSFunction())) {
            JSFunction::Cast(value.GetTaggedObject())->SetLexicalEnv(thread, lexenv.GetTaggedValue());
            JSFunction::Cast(value.GetTaggedObject())->SetConstantPool(thread, constpool.GetTaggedValue());
            JSFunction::Cast(value.GetTaggedObject())->SetModule(thread, ecmaModule);
        }
    }

    ctor->SetResolved(true);
    return ctor.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCloneClassFromTemplate(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                          const JSHandle<JSTaggedValue> &base,
                                                          const JSHandle<JSTaggedValue> &lexenv,
                                                          const JSHandle<JSTaggedValue> &constpool)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(ctor.GetTaggedValue().IsClassConstructor());
    JSHandle<JSObject> clsPrototype(thread, ctor->GetFunctionPrototype());

    bool canShareHClass = false;
    if (ctor->GetClass()->GetProto() == base.GetTaggedValue()) {
        canShareHClass = true;
    }

    JSHandle<JSFunction> cloneClass = factory->CloneClassCtor(ctor, lexenv, canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> cloneClassPrototype = factory->CloneObjectLiteral(JSHandle<JSObject>(clsPrototype), lexenv,
                                                                         constpool, canShareHClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // After clone both, reset "constructor" and "prototype" properties.
    cloneClass->SetFunctionPrototype(thread, cloneClassPrototype.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    PropertyDescriptor ctorDesc(thread, JSHandle<JSTaggedValue>(cloneClass), true, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(cloneClassPrototype),
                                         globalConst->GetHandledConstructorString(), ctorDesc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    cloneClass->SetHomeObject(thread, cloneClassPrototype);

    if (!canShareHClass) {
        RuntimeSetClassInheritanceRelationship(thread, JSHandle<JSTaggedValue>(ctor), base);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    return cloneClass.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                   const JSHandle<JSTaggedValue> &ctor,
                                                                   const JSHandle<JSTaggedValue> &base)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    ASSERT(ctor->IsJSFunction());
    JSMutableHandle<JSTaggedValue> parent(base);

    /*
     *         class A / class A extends null                             class A extends B
     *                                       a                                                 a
     *                                       |                                                 |
     *                                       |  __proto__                                      |  __proto__
     *                                       |                                                 |
     *       A            ---->         A.prototype                  A             ---->    A.prototype
     *       |                               |                       |                         |
     *       |  __proto__                    |  __proto__            |  __proto__              |  __proto__
     *       |                               |                       |                         |
     *   Function.prototype       Object.prototype / null            B             ---->    B.prototype
     */

    JSHandle<JSTaggedValue> parentPrototype;
    // hole means parent is not present
    if (parent->IsHole()) {
        JSHandle<JSFunction>::Cast(ctor)->SetFunctionKind(FunctionKind::CLASS_CONSTRUCTOR);
        parentPrototype = env->GetObjectFunctionPrototype();
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (parent->IsNull()) {
        JSHandle<JSFunction>::Cast(ctor)->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSHandle<JSTaggedValue>(thread, JSTaggedValue::Null());
        parent.Update(env->GetFunctionPrototype().GetTaggedValue());
    } else if (!parent->IsConstructor()) {
        return RuntimeThrowTypeError(thread, "parent class is not constructor");
    } else {
        JSHandle<JSFunction>::Cast(ctor)->SetFunctionKind(FunctionKind::DERIVED_CONSTRUCTOR);
        parentPrototype = JSTaggedValue::GetProperty(thread, parent,
            globalConst->GetHandledPrototypeString()).GetValue();
        if (!parentPrototype->IsECMAObject() && !parentPrototype->IsNull()) {
            return RuntimeThrowTypeError(thread, "parent class have no valid prototype");
        }
    }

    ctor->GetTaggedObject()->GetClass()->SetPrototype(thread, parent);

    JSHandle<JSObject> clsPrototype(thread, JSHandle<JSFunction>(ctor)->GetFunctionPrototype());
    clsPrototype->GetClass()->SetPrototype(thread, parentPrototype);

    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeSetClassConstructorLength(JSThread *thread, JSTaggedValue ctor,
                                                             JSTaggedValue length)
{
    ASSERT(ctor.IsClassConstructor());

    JSFunction* cls = JSFunction::Cast(ctor.GetTaggedObject());
    if (LIKELY(!cls->GetClass()->IsDictionaryMode())) {
        cls->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    } else {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        cls->UpdatePropertyInDictionary(thread, globalConst->GetLengthString(), length);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<JSFunction> &func,
                                                     JSMethod *method)
{
    uint32_t icSlotSize = method->GetSlotSize();
    if (icSlotSize > 0 && icSlotSize < ProfileTypeInfo::INVALID_SLOT_INDEX) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<ProfileTypeInfo> profileTypeInfo = factory->NewProfileTypeInfo(icSlotSize);
        func->SetProfileTypeInfo(thread, profileTypeInfo.GetTaggedValue());
        return profileTypeInfo.GetTaggedValue();
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByValueWithNameSet(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                           const JSHandle<JSTaggedValue> &key,
                                                           const JSHandle<JSTaggedValue> &value)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    if (obj->IsClassConstructor() &&
        JSTaggedValue::SameValue(key, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(thread, "In a class, static property named 'prototype' throw a TypeError");
    }

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    JSMutableHandle<JSTaggedValue> propKey(JSTaggedValue::ToPropertyKey(thread, key));
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, propKey, desc);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "StOwnByValueWithNameSet failed");
    }
    if (value->IsJSFunction()) {
        if (propKey->IsNumber()) {
            propKey.Update(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()).GetTaggedValue());
        }
        JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(value), propKey,
                                        JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeStOwnByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &value)
{
    ASSERT(prop->IsStringOrSymbol());

    // property in class is non-enumerable
    bool enumerable = !(obj->IsClassPrototype() || obj->IsClassConstructor());

    PropertyDescriptor desc(thread, value, true, enumerable, true);
    bool ret = JSTaggedValue::DefineOwnProperty(thread, obj, prop, desc);
    if (!ret) {
        return RuntimeThrowTypeError(thread, "SetOwnByName failed");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                    const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSGeneratorObject> generatorObjectHandle(genObj);
    JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());
    // save stack, should copy cur_frame, function execute over will free cur_frame
    SlowRuntimeHelper::SaveFrameToContext(thread, genContextHandle);

    // change state to SuspendedYield
    if (generatorObjectHandle->IsExecuting()) {
        generatorObjectHandle->SetGeneratorState(JSGeneratorState::SUSPENDED_YIELD);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return value.GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return generatorObjectHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName)
{
    return thread->GetEcmaVM()->GetModuleManager()->GetModuleNamespace(localName);
}

void RuntimeStubs::RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    thread->GetEcmaVM()->GetModuleManager()->StoreModuleValue(key, value);
}

JSTaggedValue RuntimeStubs::RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner)
{
    if (inner) {
        JSTaggedValue moduleValue = thread->GetEcmaVM()->GetModuleManager()->GetModuleValueInner(key);
        return moduleValue;
    }

    return thread->GetEcmaVM()->GetModuleManager()->GetModuleValueOutter(key);
}

JSTaggedValue RuntimeStubs::RuntimeGetPropIterator(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSForInIterator> iteratorHandle = JSObject::EnumerateObjectProperties(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iteratorHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeAsyncFunctionEnter(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. create promise
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = globalEnv->GetPromiseFunction();

    JSHandle<JSPromise> promiseObject =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    promiseObject->SetPromiseState(PromiseState::PENDING);
    // 2. create asyncfuncobj
    JSHandle<JSAsyncFuncObject> asyncFuncObj = factory->NewJSAsyncFuncObject();
    asyncFuncObj->SetPromise(thread, promiseObject);

    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();
    context->SetGeneratorObject(thread, asyncFuncObj);

    // change state to EXECUTING
    asyncFuncObj->SetGeneratorState(JSGeneratorState::EXECUTING);
    asyncFuncObj->SetGeneratorContext(thread, context);

    // 3. return asyncfuncobj
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return asyncFuncObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> valuesFunc =
        JSTaggedValue::GetProperty(thread, obj, env->GetIteratorSymbol()).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!valuesFunc->IsCallable()) {
        return valuesFunc.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, valuesFunc, obj, undefined, 0);
    return EcmaInterpreter::Execute(&info);
}

void RuntimeStubs::RuntimeThrowDyn(JSThread *thread, JSTaggedValue value)
{
    thread->SetException(value);
}

void RuntimeStubs::RuntimeThrowThrowNotExists(JSThread *thread)
{
    THROW_TYPE_ERROR(thread, "Throw method is not defined");
}

void RuntimeStubs::RuntimeThrowPatternNonCoercible(JSThread *thread)
{
    JSHandle<EcmaString> msg(thread->GlobalConstants()->GetHandledObjNotCoercibleString());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowDeleteSuperProperty(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII("Can not delete super property");
    JSHandle<JSObject> errorObj = factory->NewJSError(base::ErrorType::REFERENCE_ERROR, info);
    THROW_NEW_ERROR_AND_RETURN(thread, errorObj.GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowUndefinedIfHole(JSThread *thread, const JSHandle<EcmaString> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> info = factory->NewFromASCII(" is not initialized");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, obj);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue());
}

void RuntimeStubs::RuntimeThrowIfNotObject(JSThread *thread)
{
    THROW_TYPE_ERROR(thread, "Inner return result is not object");
}

void RuntimeStubs::RuntimeThrowConstAssignment(JSThread *thread, const JSHandle<EcmaString> &value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> info = factory->NewFromASCII("Assignment to const variable ");

    JSHandle<EcmaString> msg = factory->ConcatFromString(info, value);
    THROW_NEW_ERROR_AND_RETURN(thread, factory->NewJSError(base::ErrorType::TYPE_ERROR, msg).GetTaggedValue());
}

JSTaggedValue RuntimeStubs::RuntimeLdGlobalRecord(JSThread *thread, JSTaggedValue key)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return JSTaggedValue(dict->GetBox(entry));
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue RuntimeStubs::RuntimeTryLdGlobalByName(JSThread *thread, JSTaggedValue global,
                                                     const JSHandle<JSTaggedValue> &prop)
{
    JSHandle<JSTaggedValue> obj(thread, global);
    OperationResult res = JSTaggedValue::GetProperty(thread, obj, prop);
    if (!res.GetPropertyMetaData().IsFound()) {
        return RuntimeThrowReferenceError(thread, prop, " is not defined");
    }
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeTryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop,
                                                         JSTaggedValue value)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    GlobalDictionary *dict = GlobalDictionary::Cast(env->GetGlobalRecord()->GetTaggedObject());
    int entry = dict->FindEntry(prop);
    ASSERT(entry != -1);

    if (dict->GetAttributes(entry).IsConstProps()) {
        return RuntimeThrowSyntaxError(thread, "const variable can not be modified");
    }

    PropertyBox *box = dict->GetBox(entry);
    box->SetValue(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeThrowReferenceError(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                       const char *desc)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> propName = JSTaggedValue::ToString(thread, prop);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> info = factory->NewFromUtf8(desc);
    JSHandle<EcmaString> msg = factory->ConcatFromString(propName, info);
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread,
                                     factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg).GetTaggedValue(),
                                     JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeLdGlobalVar(JSThread *thread, JSTaggedValue global,
                                               const JSHandle<JSTaggedValue> &prop)
{
    JSHandle<JSTaggedValue> objHandle(thread, global.GetTaggedObject()->GetClass()->GetPrototype());
    OperationResult res = JSTaggedValue::GetProperty(thread, objHandle, prop);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.GetValue().GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeStGlobalVar(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSTaggedValue> global(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());

    JSObject::GlobalSetProperty(thread, prop, value, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    return JSTaggedValue::ToNumeric(thread, value).GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                         const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Equal(thread, left, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeNotEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                            const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Equal(thread, left, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::False() : JSTaggedValue::True());
}

JSTaggedValue RuntimeStubs::RuntimeLessDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) == ComparisonResult::LESS;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeLessEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                             const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) <= ComparisonResult::EQUAL;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeGreaterDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                              const JSHandle<JSTaggedValue> &right)
{
    bool ret = JSTaggedValue::Compare(thread, left, right) == ComparisonResult::GREAT;
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeGreaterEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                const JSHandle<JSTaggedValue> &right)
{
    ComparisonResult comparison = JSTaggedValue::Compare(thread, left, right);
    bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return (ret ? JSTaggedValue::True() : JSTaggedValue::False());
}

JSTaggedValue RuntimeStubs::RuntimeAdd2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    if (left->IsString() && right->IsString()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(JSHandle<EcmaString>(left), JSHandle<EcmaString>(right));
        return newString.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> primitiveA0(thread, JSTaggedValue::ToPrimitive(thread, left));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> primitiveA1(thread, JSTaggedValue::ToPrimitive(thread, right));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // contain string
    if (primitiveA0->IsString() || primitiveA1->IsString()) {
        JSHandle<EcmaString> stringA0 = JSTaggedValue::ToString(thread, primitiveA0);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<EcmaString> stringA1 = JSTaggedValue::ToString(thread, primitiveA1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> newString = factory->ConcatFromString(stringA0, stringA1);
        return newString.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, primitiveA0);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, primitiveA1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Add(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double doubleA0 = valLeft->GetNumber();
    double doubleA1 = valRight->GetNumber();
    return JSTaggedValue(doubleA0 + doubleA1);
}

JSTaggedValue RuntimeStubs::RuntimeSub2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Subtract(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 - number1;
}

JSTaggedValue RuntimeStubs::RuntimeMul2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9. ReturnIfAbrupt(rnum).
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Multiply(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    // 12.6.3.1 Applying the * Operator
    JSTaggedNumber number0(valLeft.GetTaggedValue());
    JSTaggedNumber number1(valRight.GetTaggedValue());
    return number0 * number1;
}

JSTaggedValue RuntimeStubs::RuntimeDiv2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> bigLeft(valLeft);
            JSHandle<BigInt> bigRight(valRight);
            return BigInt::Divide(thread, bigLeft, bigRight).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    if (dRight == 0) {
        if (dLeft == 0 || std::isnan(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        bool positive = (((bit_cast<uint64_t>(dRight)) & base::DOUBLE_SIGN_MASK) ==
                         ((bit_cast<uint64_t>(dLeft)) & base::DOUBLE_SIGN_MASK));
        return JSTaggedValue(positive ? base::POSITIVE_INFINITY : -base::POSITIVE_INFINITY);
    }
    return JSTaggedValue(dLeft / dRight);
}

JSTaggedValue RuntimeStubs::RuntimeMod2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                           const JSHandle<JSTaggedValue> &right)
{
    JSHandle<JSTaggedValue> valLeft = JSTaggedValue::ToNumeric(thread, left);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> valRight = JSTaggedValue::ToNumeric(thread, right);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12.6.3.3 Applying the % Operator
    if (valLeft->IsBigInt() || valRight->IsBigInt()) {
        if (valLeft->IsBigInt() && valRight->IsBigInt()) {
            JSHandle<BigInt> leftBigint(valLeft);
            JSHandle<BigInt> rightBigint(valRight);
            return BigInt::Remainder(thread, leftBigint, rightBigint).GetTaggedValue();
        }
        return RuntimeThrowTypeError(thread, "Cannot mix BigInt and other types, use explicit conversions");
    }
    double dLeft = valLeft->GetNumber();
    double dRight = valRight->GetNumber();
    // 12.6.3.3 Applying the % Operator
    if ((dRight == 0.0) || std::isnan(dRight) || std::isnan(dLeft) || std::isinf(dLeft)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    if ((dLeft == 0.0) || std::isinf(dRight)) {
        return JSTaggedValue(dLeft);
    }
    return JSTaggedValue(std::fmod(dLeft, dRight));
}

JSTaggedValue RuntimeStubs::RuntimeCreateEmptyObject([[maybe_unused]] JSThread *thread, ObjectFactory *factory,
                                                     JSHandle<GlobalEnv> globalEnv)
{
    JSHandle<JSFunction> builtinObj(globalEnv->GetObjectFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateEmptyArray([[maybe_unused]] JSThread *thread, ObjectFactory *factory,
                                                    JSHandle<GlobalEnv> globalEnv)
{
    JSHandle<JSFunction> builtinObj(globalEnv->GetArrayFunction());
    JSHandle<JSObject> arr = factory->NewJSObjectByConstructor(builtinObj, JSHandle<JSTaggedValue>(builtinObj));
    return arr.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                                  uint32_t startIdx)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs);
    for (uint32_t i = 0; i < actualNumArgs; ++i) {
        argumentsList->Set(thread, i,
                           JSTaggedValue(sp[startIdx + i]));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    // 1. Let len be the number of elements in argumentsList
    uint32_t len = argumentsList->GetLength();
    // 2. Let obj be ObjectCreate(%ObjectPrototype%, «[[ParameterMap]]»).
    // 3. Set obj’s [[ParameterMap]] internal slot to undefined.
    JSHandle<JSArguments> obj = factory->NewJSArguments();
    // 4. Perform DefinePropertyOrThrow(obj, "length", PropertyDescriptor{[[Value]]: len, [[Writable]]: true,
    // [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(len));
    // 5. Let index be 0.
    // 6. Repeat while index < len,
    //    a. Let val be argumentsList[index].
    //    b. Perform CreateDataProperty(obj, ToString(index), val).
    //    c. Let index be index + 1
    obj->SetElements(thread, argumentsList.GetTaggedValue());
    // 7. Perform DefinePropertyOrThrow(obj, @@iterator, PropertyDescriptor
    // {[[Value]]:%ArrayProto_values%,
    // [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 globalEnv->GetArrayProtoValuesFunction().GetTaggedValue());
    // 8. Perform DefinePropertyOrThrow(obj, "caller", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> throwFunction = globalEnv->GetThrowTypeError();
    JSHandle<AccessorData> accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLER_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    // 9. Perform DefinePropertyOrThrow(obj, "callee", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLEE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return obj
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs,
                                                uint32_t startIdx)
{
    JSHandle<JSTaggedValue> restArray = JSArray::ArrayCreate(thread, JSTaggedNumber(restNumArgs));

    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < restNumArgs; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        element.Update(JSTaggedValue(sp[startIdx + i]));
        JSObject::SetProperty(thread, restArray, i, element, true);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return restArray.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                         const JSHandle<JSTaggedValue> &literal)
{
    JSHandle<JSArray> array(literal);
    JSHandle<JSArray> arrLiteral = factory->CloneArrayLiteral(array);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return arrLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                          const JSHandle<JSObject> &literal)
{
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(literal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewLexicalEnvDyn(JSThread *thread, uint16_t numVars)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    newEnv->SetScopeInfo(thread, JSTaggedValue::Hole());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewObjDynRange(JSThread *thread, const JSHandle<JSTaggedValue> &func,
    const JSHandle<JSTaggedValue> &newTarget, uint16_t firstArgIdx, uint16_t length)
{
    JSHandle<JSTaggedValue> preArgs(thread, JSTaggedValue::Undefined());
    auto tagged = SlowRuntimeHelper::Construct(thread, func, newTarget, preArgs, length, firstArgIdx);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return tagged;
}

JSTaggedValue RuntimeStubs::RuntimeDefinefuncDyn(JSThread *thread, JSFunction *func)
{
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::BASE_CONSTRUCTOR);
    jsFunc->SetCodeEntry(func->GetCodeEntry());
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateRegExpWithLiteral(JSThread *thread,
                                                           const JSHandle<JSTaggedValue> &pattern, uint8_t flags)
{
    JSHandle<JSTaggedValue> flagsHandle(thread, JSTaggedValue(flags));
    return builtins::BuiltinsRegExp::RegExpCreate(thread, pattern, flagsHandle);
}

JSTaggedValue RuntimeStubs::RuntimeThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index,
                                                              JSTaggedValue thisValue)
{
    if (index == 0 && (thisValue.IsUndefined() || thisValue.IsHole())) {
        return RuntimeThrowReferenceError(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                       "sub-class must call super before use 'this'");
    }
    if (index == 1 && !thisValue.IsUndefined() && !thisValue.IsHole()) {
        return RuntimeThrowReferenceError(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                       "super() forbidden re-bind 'this'");
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory,
                                                            const JSHandle<JSObject> &literal,
                                                            const JSHandle<JSTaggedValue> &env,
                                                            const JSHandle<JSTaggedValue> &constpool)
{
    JSHandle<JSObject> objLiteral = factory->CloneObjectLiteral(literal, env, constpool);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return objLiteral.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                const JSHandle<JSTaggedValue> &objVal,
                                                                uint16_t firstArgRegIdx)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    ASSERT(objVal->IsJSObject());
    JSHandle<JSObject> obj(objVal);
    uint32_t numExcludedKeys = 0;
    JSHandle<TaggedArray> excludedKeys = factory->NewTaggedArray(numKeys + 1);
    FrameHandler frameHandler(thread);
    JSTaggedValue excludedKey = frameHandler.GetVRegValue(firstArgRegIdx);
    if (!excludedKey.IsUndefined()) {
        numExcludedKeys = numKeys + 1;
        excludedKeys->Set(thread, 0, excludedKey);
        for (uint32_t i = 1; i < numExcludedKeys; i++) {
            excludedKey = frameHandler.GetVRegValue(firstArgRegIdx + i);
            excludedKeys->Set(thread, i, excludedKey);
        }
    }

    uint32_t numAllKeys = obj->GetNumberOfKeys();
    JSHandle<TaggedArray> allKeys = factory->NewTaggedArray(numAllKeys);
    JSObject::GetAllKeys(thread, obj, 0, allKeys);

    JSHandle<JSObject> restObj = factory->NewEmptyJSObject();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < numAllKeys; i++) {
        key.Update(allKeys->Get(i));
        bool isExcludedKey = false;
        for (uint32_t j = 0; j < numExcludedKeys; j++) {
            if (JSTaggedValue::Equal(thread, key, JSHandle<JSTaggedValue>(thread, excludedKeys->Get(j)))) {
                isExcludedKey = true;
                break;
            }
        }
        if (!isExcludedKey) {
            PropertyDescriptor desc(thread);
            bool success = JSObject::GetOwnProperty(thread, obj, key, desc);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (success && desc.IsEnumerable()) {
                JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, obj, key).GetValue();
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSObject::SetProperty(thread, restObj, key, value, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
        }
    }
    return restObj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDefineNCFuncDyn(JSThread *thread, JSFunction *func)
{
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::ARROW_FUNCTION);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDefineGeneratorFunc(JSThread *thread, JSFunction *func)
{
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> jsFunc = factory->NewJSGeneratorFunction(method);
    ASSERT_NO_ABRUPT_COMPLETION(thread);

    // 26.3.4.3 prototype
    // Whenever a GeneratorFunction instance is created another ordinary object is also created and
    // is the initial value of the generator function's "prototype" property.
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> initialGeneratorFuncPrototype =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    jsFunc->SetProtoOrDynClass(thread, initialGeneratorFuncPrototype);

    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDefineGeneratorFuncWithMethodId(JSThread *thread, JSTaggedValue methodId)
{
    auto aotCodeInfo  = thread->GetEcmaVM()->GetAotCodeInfo();
    auto codeEntry = aotCodeInfo->GetAOTFuncEntry(methodId.GetInt());
    JSMethod *method = thread->GetEcmaVM()->GetMethodForNativeFunction(reinterpret_cast<void *>(codeEntry));
    method->SetAotCodeBit(true);
    method->SetNativeBit(false);
    method->SetNumArgsWithCallField(1);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> jsFunc = factory->NewJSGeneratorFunction(method);
    ASSERT_NO_ABRUPT_COMPLETION(thread);

    // 26.3.4.3 prototype
    // Whenever a GeneratorFunction instance is created another ordinary object is also created and
    // is the initial value of the generator function's "prototype" property.
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> initialGeneratorFuncPrototype =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetPrototype(thread, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    jsFunc->SetProtoOrDynClass(thread, initialGeneratorFuncPrototype);
    jsFunc->SetCodeEntry(codeEntry);

    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDefineAsyncFunc(JSThread *thread, JSFunction *func)
{
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::ASYNC_FUNCTION);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeDefineMethod(JSThread *thread, JSFunction *func,
                                                const JSHandle<JSTaggedValue> &homeObject)
{
    ASSERT(homeObject->IsECMAObject());
    auto method = func->GetCallTarget();

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> jsFunc = factory->NewJSFunctionByDynClass(method, dynclass, FunctionKind::NORMAL_FUNCTION);
    jsFunc->SetHomeObject(thread, homeObject);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    return jsFunc.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCallSpreadDyn(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                 const JSHandle<JSTaggedValue> &obj,
                                                 const JSHandle<JSTaggedValue> &array)
{
    if ((!obj->IsUndefined() && !obj->IsECMAObject()) || !func->IsJSFunction() || !array->IsJSArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "cannot Callspread", JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> coretypesArray(thread, RuntimeGetCallSpreadArgs(thread, array.GetTaggedValue()));
    uint32_t length = coretypesArray->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, obj, undefined, length);
    info.SetCallArg(length, coretypesArray);
    return EcmaInterpreter::Execute(&info);
}

JSTaggedValue RuntimeStubs::RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                             const JSHandle<JSTaggedValue> &prop,
                                                             const JSHandle<JSTaggedValue> &getter,
                                                             const JSHandle<JSTaggedValue> &setter, bool flag)
{
    JSHandle<JSTaggedValue> propKey = JSTaggedValue::ToPropertyKey(thread, prop);

    auto globalConst = thread->GlobalConstants();
    if (obj.GetTaggedValue().IsClassConstructor() &&
        JSTaggedValue::SameValue(propKey, globalConst->GetHandledPrototypeString())) {
        return RuntimeThrowTypeError(
            thread,
            "In a class, computed property names for static getter that are named 'prototype' throw a TypeError");
    }

    if (flag) {
        if (!getter->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(getter), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetGetString()));
        }

        if (!setter->IsUndefined()) {
            if (propKey->IsNumber()) {
                propKey =
                    JSHandle<JSTaggedValue>::Cast(base::NumberHelper::NumberToString(thread, propKey.GetTaggedValue()));
            }
            JSFunctionBase::SetFunctionName(thread, JSHandle<JSFunctionBase>::Cast(setter), propKey,
                                            JSHandle<JSTaggedValue>(thread, globalConst->GetSetString()));
        }
    }

    // set accessor
    bool enumerable =
        !(obj.GetTaggedValue().IsClassPrototype() || obj.GetTaggedValue().IsClassConstructor());
    PropertyDescriptor desc(thread, true, enumerable, true);
    if (!getter->IsUndefined()) {
        JSHandle<JSFunction>::Cast(getter)->SetFunctionKind(FunctionKind::GETTER_FUNCTION);
        desc.SetGetter(getter);
    }
    if (!setter->IsUndefined()) {
        JSHandle<JSFunction>::Cast(setter)->SetFunctionKind(FunctionKind::SETTER_FUNCTION);
        desc.SetSetter(setter);
    }
    JSObject::DefineOwnProperty(thread, obj, propKey, desc);

    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                             const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                             uint16_t length)
{
    JSHandle<JSTaggedValue> superFunc(thread, JSTaggedValue::GetPrototype(thread, func));
    ASSERT(superFunc->IsJSFunction());

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argv = factory->NewTaggedArray(length);
    FrameHandler frameHandler(thread);
    for (size_t i = 0; i < length; ++i) {
        argv->Set(thread, i, frameHandler.GetVRegValue(firstVRegIdx + i));
    }

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, superFunc, undefined, newTarget, length);
    info.SetCallArg(length, argv);
    JSTaggedValue result = JSFunction::Construct(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return result;
}

JSTaggedValue RuntimeStubs::RuntimeThrowTypeError(JSThread *thread, const char *message)
{
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeGetCallSpreadArgs(JSThread *thread, JSTaggedValue array)
{
    JSHandle<JSTaggedValue> jsArray(thread, array);
    uint32_t argvMayMaxLength = JSHandle<JSArray>::Cast(jsArray)->GetArrayLength();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> argv = factory->NewTaggedArray(argvMayMaxLength);
    JSHandle<JSTaggedValue> itor = JSIterator::GetIterator(thread, jsArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSMutableHandle<JSTaggedValue> next(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> nextArg(thread, JSTaggedValue::Undefined());
    size_t argvIndex = 0;
    while (true) {
        next.Update(JSIterator::IteratorStep(thread, itor).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValue(next.GetTaggedValue(), JSTaggedValue::False())) {
            break;
        }
        nextArg.Update(JSIterator::IteratorValue(thread, next).GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argv->Set(thread, argvIndex++, nextArg);
    }

    argv = factory->CopyArray(argv, argvMayMaxLength, argvIndex);
    return argv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeThrowSyntaxError(JSThread *thread, const char *message)
{
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    THROW_SYNTAX_ERROR_AND_RETURN(thread, message, JSTaggedValue::Exception());
}

JSTaggedValue RuntimeStubs::RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt)
{
    return JSTaggedValue::ToBigInt(thread, numberBigInt);
}

JSTaggedValue RuntimeStubs::RuntimeNewLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    JSTaggedValue scopeInfo = ScopeInfoExtractor::GenerateScopeInfo(thread, scopeId);
    newEnv->SetScopeInfo(thread, scopeInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeGetAotUnmapedArgs(JSThread *thread, uint32_t actualNumArgs, uintptr_t argv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<TaggedArray> argumentsList = factory->NewTaggedArray(actualNumArgs - FIXED_NUM_ARGS);
    for (uint32_t i = 0; i < actualNumArgs - FIXED_NUM_ARGS; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i + 1]; // skip actualNumArgs
        argumentsList->Set(thread, i, JSTaggedValue(arg));
    }
    // 1. Let len be the number of elements in argumentsList
    uint32_t len = argumentsList->GetLength();
    // 2. Let obj be ObjectCreate(%ObjectPrototype%, «[[ParameterMap]]»).
    // 3. Set obj’s [[ParameterMap]] internal slot to undefined.
    JSHandle<JSArguments> obj = factory->NewJSArguments();
    // 4. Perform DefinePropertyOrThrow(obj, "length", PropertyDescriptor{[[Value]]: len, [[Writable]]: true,
    // [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(len));
    // 5. Let index be 0.
    // 6. Repeat while index < len,
    //    a. Let val be argumentsList[index].
    //    b. Perform CreateDataProperty(obj, ToString(index), val).
    //    c. Let index be index + 1
    obj->SetElements(thread, argumentsList.GetTaggedValue());
    // 7. Perform DefinePropertyOrThrow(obj, @@iterator, PropertyDescriptor
    // {[[Value]]:%ArrayProto_values%,
    // [[Writable]]: true, [[Enumerable]]: false, [[Configurable]]: true}).
    obj->SetPropertyInlinedProps(thread, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 globalEnv->GetArrayProtoValuesFunction().GetTaggedValue());
    // 8. Perform DefinePropertyOrThrow(obj, "caller", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    JSHandle<JSTaggedValue> throwFunction = globalEnv->GetThrowTypeError();
    JSHandle<AccessorData> accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLER_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    // 9. Perform DefinePropertyOrThrow(obj, "callee", PropertyDescriptor {[[Get]]: %ThrowTypeError%,
    // [[Set]]: %ThrowTypeError%, [[Enumerable]]: false, [[Configurable]]: false}).
    accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, throwFunction);
    accessor->SetSetter(thread, throwFunction);
    obj->SetPropertyInlinedProps(thread, JSArguments::CALLEE_INLINE_PROPERTY_INDEX, accessor.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return obj
    return obj.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewAotLexicalEnvDyn(JSThread *thread, uint16_t numVars,
                                                       JSHandle<JSTaggedValue> &currentLexEnv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);
    newEnv->SetParentEnv(thread, currentLexEnv.GetTaggedValue());
    newEnv->SetScopeInfo(thread, JSTaggedValue::Hole());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewAotLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                               JSHandle<JSTaggedValue> &currentLexEnv)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LexicalEnv> newEnv = factory->NewLexicalEnv(numVars);

    newEnv->SetParentEnv(thread, currentLexEnv.GetTaggedValue());
    JSTaggedValue scopeInfo = ScopeInfoExtractor::GenerateScopeInfo(thread, scopeId);
    newEnv->SetScopeInfo(thread, scopeInfo);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newEnv.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeCopyAotRestArgs(JSThread *thread, uint32_t restNumArgs, uintptr_t argv)
{
    uint32_t actualRestNum = restNumArgs - FIXED_NUM_ARGS;
    JSHandle<JSTaggedValue> restArray = JSArray::ArrayCreate(thread, JSTaggedNumber(actualRestNum));

    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < actualRestNum; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i + 1]; // skip restNumArgs
        element.Update(JSTaggedValue(arg));
        JSObject::SetProperty(thread, restArray, i, element, true);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return restArray.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeSuspendAotGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                       const JSHandle<JSTaggedValue> &value)
{
    JSHandle<JSGeneratorObject> generatorObjectHandle(genObj);
    JSHandle<GeneratorContext> genContextHandle(thread, generatorObjectHandle->GetGeneratorContext());

    // change state to SuspendedYield
    if (generatorObjectHandle->IsExecuting()) {
        generatorObjectHandle->SetGeneratorState(JSGeneratorState::SUSPENDED_YIELD);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return value.GetTaggedValue();
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return generatorObjectHandle.GetTaggedValue();
}

JSTaggedValue RuntimeStubs::RuntimeNewAotObjDynRange(JSThread *thread, uintptr_t argv, uint32_t argc)
{
    JSTaggedType *args = reinterpret_cast<JSTaggedType *>(argv);
    JSHandle<JSTaggedValue> ctor = GetHArg<JSTaggedValue>(argv, argc, 0);
    JSHandle<JSTaggedValue> newTgt = GetHArg<JSTaggedValue>(argv, argc, 1);
    JSHandle<JSTaggedValue> thisObj = thread->GlobalConstants()->GetHandledUndefined();
    const size_t numCtorAndNewTgt = 2;
    STACK_ASSERT_SCOPE(thread);
    EcmaRuntimeCallInfo info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, ctor, thisObj, newTgt, argc - numCtorAndNewTgt);
    for (size_t i = 0; i < argc - numCtorAndNewTgt; ++i) {
        info.SetCallArg(i, JSTaggedValue(args[i + numCtorAndNewTgt]));
    }

    JSTaggedValue object = JSFunction::Construct(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return object;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_RUNTIME_TRAMPOLINES_INL_H
