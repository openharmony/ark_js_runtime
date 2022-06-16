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

#include "slow_runtime_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue SlowRuntimeHelper::CallBoundFunction(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    JSHandle<JSBoundFunction> boundFunc(info->GetFunction());
    JSHandle<JSFunction> targetFunc(thread, boundFunc->GetBoundTarget());
    if (targetFunc->IsClassConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot called without 'new'",
                                    JSTaggedValue::Exception());
    }

    JSHandle<TaggedArray> boundArgs(thread, boundFunc->GetBoundArguments());
    const size_t boundLength = boundArgs->GetLength();
    const size_t argsLength = info->GetArgsNumber() + boundLength;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo runtimeInfo = EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(targetFunc),
        info->GetThis(), undefined, argsLength);
    if (boundLength == 0) {
        runtimeInfo.SetCallArg(argsLength, 0, info, 0);
    } else {
        // 0 ~ boundLength is boundArgs; boundLength ~ argsLength is args of EcmaRuntimeCallInfo.
        runtimeInfo.SetCallArg(boundLength, boundArgs);
        runtimeInfo.SetCallArg(argsLength, boundLength, info, 0);
    }
    return EcmaInterpreter::Execute(&runtimeInfo);
}

JSTaggedValue SlowRuntimeHelper::NewObject(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    JSHandle<JSTaggedValue> func(info->GetFunction());
    if (!func->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is nullptr", JSTaggedValue::Exception());
    }

    if (!func->IsJSFunction()) {
        if (func->IsBoundFunction()) {
            JSTaggedValue result = JSBoundFunction::ConstructInternal(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result;
        }

        if (func->IsJSProxy()) {
            JSTaggedValue jsObj = JSProxy::ConstructInternal(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return jsObj;
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructed NonConstructable", JSTaggedValue::Exception());
    }

    JSTaggedValue result = JSFunction::ConstructInternal(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

void SlowRuntimeHelper::SaveFrameToContext(JSThread *thread, JSHandle<GeneratorContext> context)
{
    FrameHandler frameHandler(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t nregs = frameHandler.GetNumberArgs();
    JSHandle<TaggedArray> regsArray = factory->NewTaggedArray(nregs);
    for (uint32_t i = 0; i < nregs; i++) {
        JSTaggedValue value = frameHandler.GetVRegValue(i);
        regsArray->Set(thread, i, value);
    }
    context->SetRegsArray(thread, regsArray.GetTaggedValue());
    context->SetMethod(thread, frameHandler.GetFunction());

    context->SetAcc(thread, frameHandler.GetAcc());
    context->SetLexicalEnv(thread, thread->GetCurrentLexenv());
    context->SetNRegs(nregs);
    context->SetBCOffset(frameHandler.GetBytecodeOffset());
}

JSTaggedValue ConstructGeneric(JSThread *thread, JSHandle<JSFunction> ctor, JSHandle<JSTaggedValue> newTgt,
                               JSHandle<JSTaggedValue> preArgs, uint32_t argsCount, uint32_t baseArgLocation)
{
    if (!ctor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> obj(thread, JSTaggedValue::Undefined());
    if (ctor->IsBase()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        obj = JSHandle<JSTaggedValue>(factory->NewJSObjectByConstructor(ctor, newTgt));
    }
    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const uint32_t size = preArgsSize + argsCount;
    CVector<JSTaggedType> values;
    values.reserve(size);

    JSMethod *method = ctor->GetCallTarget();
    if (method == nullptr) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Undefined target", JSTaggedValue::Exception());
    }

    // Add the input parameter
    FrameHandler frameHandler(thread);
    // add preArgs when boundfunction is encountered
    if (preArgsSize > 0) {
        JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
        for (uint32_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            values.emplace_back(value.GetRawData());
        }
        for (uint32_t i = 0; i < argsCount; ++i) {
            JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
            values.emplace_back(value.GetRawData());
        }
    } else {
        for (uint32_t i = 0; i < argsCount; ++i) {
            JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
            values.emplace_back(value.GetRawData());
        }
    }
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, JSHandle<JSTaggedValue>(ctor), obj, newTgt, size);
    info.SetCallArg(size, values.data());
    JSTaggedValue resultValue = EcmaInterpreter::Execute(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9.3.2 [[Construct]] (argumentsList, newTarget)
    if (resultValue.IsECMAObject()) {
        return resultValue;
    }

    if (ctor->IsBase()) {
        return obj.GetTaggedValue();
    }
    if (!resultValue.IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is non-constructor", JSTaggedValue::Exception());
    }
    return obj.GetTaggedValue();
}

JSTaggedValue ConstructBoundFunction(JSThread *thread, JSHandle<JSBoundFunction> ctor, JSHandle<JSTaggedValue> newTgt,
                                     JSHandle<JSTaggedValue> preArgs, uint32_t argsCount, uint32_t baseArgLocation)
{
    JSHandle<JSTaggedValue> target(thread, ctor->GetBoundTarget());
    ASSERT(target->IsConstructor());

    JSHandle<TaggedArray> boundArgs(thread, ctor->GetBoundArguments());
    JSMutableHandle<JSTaggedValue> newPreArgs(thread, preArgs.GetTaggedValue());
    if (newPreArgs->IsUndefined()) {
        newPreArgs.Update(boundArgs.GetTaggedValue());
    } else {
        newPreArgs.Update(
            TaggedArray::Append(thread, boundArgs, JSHandle<TaggedArray>::Cast(preArgs)).GetTaggedValue());
    }
    JSMutableHandle<JSTaggedValue> newTargetMutable(thread, newTgt.GetTaggedValue());
    if (JSTaggedValue::SameValue(ctor.GetTaggedValue(), newTgt.GetTaggedValue())) {
        newTargetMutable.Update(target.GetTaggedValue());
    }
    return SlowRuntimeHelper::Construct(thread, target, newTargetMutable, newPreArgs, argsCount, baseArgLocation);
}

JSTaggedValue ConstructProxy(JSThread *thread, JSHandle<JSProxy> ctor, JSHandle<JSTaggedValue> newTgt,
                             JSHandle<JSTaggedValue> preArgs, uint32_t argsCount, uint32_t baseArgLocation)
{
    // step 1 ~ 4 get ProxyHandler and ProxyTarget
    JSHandle<JSTaggedValue> handler(thread, ctor->GetHandler());
    if (handler->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor: handler is null", JSTaggedValue::Exception());
    }
    ASSERT(handler->IsJSObject());
    JSHandle<JSTaggedValue> target(thread, ctor->GetTarget());

    // 5.Let trap be GetMethod(handler, "construct").
    JSHandle<JSTaggedValue> key(thread->GlobalConstants()->GetHandledProxyConstructString());
    JSHandle<JSTaggedValue> method = JSObject::GetMethod(thread, handler, key);

    // 6.ReturnIfAbrupt(trap).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7.If trap is undefined, then
    //   a.Assert: target has a [[Construct]] internal method.
    //   b.Return Construct(target, argumentsList, newTarget).
    if (method->IsUndefined()) {
        ASSERT(target->IsConstructor());
        return SlowRuntimeHelper::Construct(thread, target, newTgt, preArgs, argsCount, baseArgLocation);
    }

    // 8.Let argArray be CreateArrayFromList(argumentsList).
    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const uint32_t size = preArgsSize + argsCount;
    JSHandle<TaggedArray> args = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(size);
    JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
    if (preArgsSize > 0) {
        for (uint32_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            args->Set(thread, i, value);
        }
    }
    FrameHandler frameHandler(thread);
    for (uint32_t i = 0; i < argsCount; ++i) {
        JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
        args->Set(thread, i + preArgsSize, value);
    }

    // step 8 ~ 9 Call(trap, handler, «target, argArray, newTarget »).
    const size_t argsLength = 3;  // 3: «target, argArray, newTarget »
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, method, handler, undefined, argsLength);
    info.SetCallArg(target.GetTaggedValue(), args.GetTaggedValue(), newTgt.GetTaggedValue());
    JSTaggedValue newObjValue = JSFunction::Call(&info);
    // 10.ReturnIfAbrupt(newObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11.If Type(newObj) is not Object, throw a TypeError exception.
    if (!newObjValue.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new object is not object", JSTaggedValue::Exception());
    }
    // 12.Return newObj.
    return newObjValue;
}

JSTaggedValue SlowRuntimeHelper::Construct(JSThread *thread, JSHandle<JSTaggedValue> ctor,
                                           JSHandle<JSTaggedValue> newTarget, JSHandle<JSTaggedValue> preArgs,
                                           uint32_t argsCount, uint32_t baseArgLocation)
{
    if (newTarget->IsUndefined()) {
        newTarget = ctor;
    }

    if (!(newTarget->IsConstructor() && ctor->IsConstructor())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }
    if (ctor->IsJSFunction()) {
        return ConstructGeneric(thread, JSHandle<JSFunction>::Cast(ctor), newTarget, preArgs, argsCount,
                                baseArgLocation);
    }
    if (ctor->IsBoundFunction()) {
        return ConstructBoundFunction(thread, JSHandle<JSBoundFunction>::Cast(ctor), newTarget, preArgs, argsCount,
                                      baseArgLocation);
    }
    if (ctor->IsJSProxy()) {
        return ConstructProxy(thread, JSHandle<JSProxy>::Cast(ctor), newTarget, preArgs, argsCount, baseArgLocation);
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor NonConstructor", JSTaggedValue::Exception());
}
}  // namespace panda::ecmascript
