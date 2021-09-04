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

#include "builtins_generator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"

namespace panda::ecmascript::builtins {
// 26.2.1.1 GeneratorFunction(p1, p2, … , pn, body)
JSTaggedValue BuiltinsGenerator::GeneratorFunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Generator, Constructor);
    // not support
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "Not support eval. Forbidden using new GeneratorFunction().",
                                JSTaggedValue::Exception());
}

// 26.4.1.2 Generator.prototype.next(value)
JSTaggedValue BuiltinsGenerator::GeneratorPrototypeNext(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Generator, PrototypeNext);
    // 1.Let g be the this value.
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsGeneratorObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a generator object.", JSTaggedValue::Exception());
    }
    JSHandle<JSGeneratorObject> generator(thread, JSGeneratorObject::Cast(*JSTaggedValue::ToObject(thread, msg)));
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    // 2.Return ? GeneratorResume(g, value).
    JSHandle<JSObject> result = JSGeneratorObject::GeneratorResume(thread, generator, value.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 26.4.1.3 Generator.prototype.return(value)
JSTaggedValue BuiltinsGenerator::GeneratorPrototypeReturn(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Generator, PrototypeReturn);
    // 1.Let g be the this value.
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsGeneratorObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a generator object.", JSTaggedValue::Exception());
    }
    JSHandle<JSGeneratorObject> generator(thread, JSGeneratorObject::Cast(*JSTaggedValue::ToObject(thread, msg)));

    // 2.Let C be Completion { [[Type]]: return, [[Value]]: value, [[Target]]: empty }.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CompletionRecord> completionRecord =
        factory->NewCompletionRecord(CompletionRecord::RETURN, value);

    // 3.Return ? GeneratorResumeAbrupt(g, C).
    JSHandle<JSObject> result = JSGeneratorObject::GeneratorResumeAbrupt(thread, generator, completionRecord);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}

// 26.4.1.4 Generator.prototype.throw(exception)
JSTaggedValue BuiltinsGenerator::GeneratorPrototypeThrow(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Generator, PrototypeThrow);
    // 1.Let g be the this value.
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsGeneratorObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a generator object.", JSTaggedValue::Exception());
    }
    JSHandle<JSGeneratorObject> generator(thread, JSGeneratorObject::Cast(*JSTaggedValue::ToObject(thread, msg)));

    // 2.Let C be ThrowCompletion(exception).
    JSHandle<JSTaggedValue> exception = GetCallArg(argv, 0);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CompletionRecord> completionRecord =
        factory->NewCompletionRecord(CompletionRecord::THROW, exception);

    // 3.Return ? GeneratorResumeAbrupt(g, C).
    JSHandle<JSObject> result = JSGeneratorObject::GeneratorResumeAbrupt(thread, generator, completionRecord);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
