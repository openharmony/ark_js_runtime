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
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue SlowRuntimeHelper::CallBoundFunction(JSThread *thread, JSHandle<JSBoundFunction> boundFunc,
                                                   JSHandle<JSTaggedValue> obj, JSHandle<TaggedArray> args)
{
    uint32_t numArgsReal = args->GetLength();

    JSHandle<TaggedArray> argsBound(thread, boundFunc->GetBoundArguments());
    uint32_t numArgsBound = argsBound->GetLength();

    JSHandle<JSFunction> targetFunc(thread, boundFunc->GetBoundTarget());
    if (targetFunc->IsClassConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot called without 'new'",
                                    JSTaggedValue::Exception());
    }

    uint32_t arraySize = numArgsBound + numArgsReal;
    JSHandle<TaggedArray> argsNew = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(arraySize);

    uint32_t idx = 0;
    for (uint32_t i = 0; i < numArgsBound; i++) {
        argsNew->Set(thread, idx++, argsBound->Get(i));
    }
    for (uint32_t i = 0; i < numArgsReal; i++) {
        argsNew->Set(thread, idx++, args->Get(i));
    }

    JSHandle<JSTaggedValue> newTarget(thread, JSTaggedValue::Undefined());
    return InvokeJsFunction(thread, targetFunc, obj, newTarget, argsNew);
}

JSTaggedValue SlowRuntimeHelper::NewObject(JSThread *thread, JSHandle<JSTaggedValue> func,
                                           JSHandle<JSTaggedValue> newTarget, JSHandle<TaggedArray> args)
{
    if (!func->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "function is nullptr", JSTaggedValue::Exception());
    }

    if (!func->IsJSFunction()) {
        if (func->IsBoundFunction()) {
            JSTaggedValue result =
                JSBoundFunction::ConstructInternal(thread, JSHandle<JSBoundFunction>::Cast(func), args, newTarget);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result;
        }

        if (func->IsJSProxy()) {
            JSTaggedValue jsObj = JSProxy::ConstructInternal(thread, JSHandle<JSProxy>(func), args, newTarget);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return jsObj;
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructed NonConstructable", JSTaggedValue::Exception());
    }

    JSHandle<JSFunction> jsFunc = JSHandle<JSFunction>::Cast(func);
    ASSERT(jsFunc->GetCallTarget() != nullptr);
    ASSERT(JSFunction::Cast(newTarget->GetTaggedObject())->GetCallTarget() != nullptr);

    if (jsFunc->GetCallTarget()->IsNative()) {
        if (jsFunc->IsBuiltinsConstructor()) {
            return InvokeJsFunction(thread, jsFunc, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                    newTarget, args);
        }
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructed NonConstructable", JSTaggedValue::Exception());
    }

    JSTaggedValue result = JSFunction::ConstructInternal(thread, jsFunc, args, newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

void SlowRuntimeHelper::SaveFrameToContext(JSThread *thread, JSHandle<GeneratorContext> context)
{
    EcmaFrameHandler frameHandler(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t nregs = frameHandler.GetSize();
    JSHandle<TaggedArray> regsArray = factory->NewTaggedArray(nregs);
    for (uint32_t i = 0; i < nregs; i++) {
        JSTaggedValue value = frameHandler.GetVRegValue(i);
        regsArray->Set(thread, i, value);
    }
    context->SetRegsArray(thread, regsArray.GetTaggedValue());
    context->SetMethod(thread, frameHandler.GetFunction());

    context->SetAcc(thread, frameHandler.GetAcc());
    context->SetNRegs(thread, JSTaggedValue(nregs));
    context->SetBCOffset(thread, JSTaggedValue(frameHandler.GetBytecodeOffset()));
    context->SetLexicalEnv(thread, thread->GetCurrentLexenv());
}

JSTaggedValue ConstructGeneric(JSThread *thread, JSHandle<JSFunction> ctor, JSHandle<JSTaggedValue> newTgt,
                               JSHandle<JSTaggedValue> preArgs, uint32_t argsCount, uint32_t baseArgLocation)
{
    if (!ctor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> obj(thread, JSTaggedValue::Undefined());
    if (!ctor->IsBuiltinsConstructor() && ctor->IsBase()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        obj = JSHandle<JSTaggedValue>(factory->NewJSObjectByConstructor(ctor, newTgt));
    }
    uint32_t preArgsSize = preArgs->IsUndefined() ? 0 : JSHandle<TaggedArray>::Cast(preArgs)->GetLength();
    const array_size_t size = preArgsSize + argsCount;
    CVector<JSTaggedType> values;
    values.reserve(size);

    JSMethod *method = ctor->GetCallTarget();
    if (method == nullptr) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Undefined target", JSTaggedValue::Exception());
    }

    // Add the input parameter
    EcmaFrameHandler frameHandler(thread);
    CallParams params;
    params.callTarget = ECMAObject::Cast(*ctor);
    params.newTarget = newTgt.GetTaggedType();
    params.thisArg = obj.GetTaggedType();
    params.argc = size;
    // add preArgs when boundfunction is encountered
    if (preArgsSize > 0) {
        JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
        for (array_size_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            values.emplace_back(value.GetRawData());
        }
        for (array_size_t i = 0; i < argsCount; ++i) {
            JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
            values.emplace_back(value.GetRawData());
        }
        params.argv = values.data();
    } else {
        for (array_size_t i = 0; i < argsCount; ++i) {
            JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
            values.emplace_back(value.GetRawData());
        }
        params.argv = values.data();
    }

    JSTaggedValue resultValue = EcmaInterpreter::Execute(thread, params);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9.3.2 [[Construct]] (argumentsList, newTarget)
    if (ctor->IsBuiltinsConstructor() || resultValue.IsECMAObject()) {
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
    const array_size_t size = preArgsSize + argsCount;
    JSHandle<TaggedArray> args = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(size);
    JSHandle<TaggedArray> tgaPreArgs = JSHandle<TaggedArray>::Cast(preArgs);
    if (preArgsSize > 0) {
        for (array_size_t i = 0; i < preArgsSize; ++i) {
            JSTaggedValue value = tgaPreArgs->Get(i);
            args->Set(thread, i, value);
        }
    }
    EcmaFrameHandler frameHandler(thread);
    for (array_size_t i = 0; i < argsCount; ++i) {
        JSTaggedValue value = frameHandler.GetVRegValue(baseArgLocation + i);
        args->Set(thread, i + preArgsSize, value);
    }

    // step 8 ~ 9 Call(trap, handler, «target, argArray, newTarget »).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> nextArgv(factory->NewTaggedArray(3));  // 3: «target, argArray, newTarget »
    nextArgv->Set(thread, 0, target.GetTaggedValue());
    nextArgv->Set(thread, 1, args.GetTaggedValue());
    nextArgv->Set(thread, 2, newTgt.GetTaggedValue());  // 2: the third arg is new target
    JSTaggedValue newObjValue = JSFunction::Call(thread, method, handler, nextArgv);
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
