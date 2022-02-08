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

#include "ecmascript/generator_helper.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_iterator.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
JSHandle<JSObject> GeneratorHelper::Next(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                         JSTaggedValue value)
{
    JSHandle<JSGeneratorObject> genObject(thread, genContext->GetGeneratorObject());
    genObject->SetResumeResult(thread, value);
    genObject->SetGeneratorState(JSGeneratorState::EXECUTING);
    genObject->SetResumeMode(GeneratorResumeMode::NEXT);

    JSTaggedValue next = EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    JSHandle<JSTaggedValue> nextValue(thread, next);

    if (genObject->IsSuspendYield()) {
        return JSHandle<JSObject>::Cast(nextValue);
    }
    genObject->SetGeneratorState(JSGeneratorState::COMPLETED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    return JSIterator::CreateIterResultObject(thread, nextValue, true);
}

JSHandle<JSObject> GeneratorHelper::Return(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                           JSTaggedValue value)
{
    JSHandle<JSGeneratorObject> genObject(thread, genContext->GetGeneratorObject());
    genObject->SetResumeMode(GeneratorResumeMode::RETURN);
    genObject->SetResumeResult(thread, value);

    JSTaggedValue res = EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    JSHandle<JSTaggedValue> returnValue(thread, res);
    // change state to completed
    if (genObject->IsExecuting()) {
        genObject->SetGeneratorState(JSGeneratorState::COMPLETED);
    }
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    return JSIterator::CreateIterResultObject(thread, returnValue, true);
}

JSHandle<JSObject> GeneratorHelper::Throw(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                          JSTaggedValue value)
{
    JSHandle<JSGeneratorObject> genObject(thread, genContext->GetGeneratorObject());
    genObject->SetResumeMode(GeneratorResumeMode::THROW);
    genObject->SetResumeResult(thread, value);

    JSTaggedValue res = EcmaInterpreter::GeneratorReEnterInterpreter(thread, genContext);
    JSHandle<JSTaggedValue> throwValue(thread, res);

    if (genObject->IsSuspendYield()) {
        return JSHandle<JSObject>::Cast(throwValue);
    }

    // change state to completed
    genObject->SetGeneratorState(JSGeneratorState::COMPLETED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    return JSIterator::CreateIterResultObject(thread, throwValue, true);
}

// main->foo
void GeneratorHelper::ChangeGenContext(JSThread *thread, const JSHandle<GeneratorContext> &genContext)
{
    JSThread *jsThread = thread;
    EcmaInterpreter::ChangeGenContext(jsThread, genContext);
}

// foo->main
void GeneratorHelper::ResumeContext(JSThread *thread)
{
    EcmaInterpreter::ResumeContext(thread);
}
}  // namespace panda::ecmascript
