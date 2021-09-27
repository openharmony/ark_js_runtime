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

#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
// 24.5.1
JSTaggedValue BuiltinsJson::Parse(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Json, Parse);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    array_size_t argc = argv->GetArgsNumber();
    if (argc == 0) {
        JSHandle<JSObject> syntaxError = factory->GetJSError(base::ErrorType::SYNTAX_ERROR, "arg is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> msg = GetCallArg(argv, 0);
    JSHandle<EcmaString> parseString = JSTaggedValue::ToString(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> result;
    if (parseString->IsUtf8()) {
        panda::ecmascript::base::JsonParser<uint8_t> parser(thread);
        result = parser.ParseUtf8(*parseString);
    } else {
        panda::ecmascript::base::JsonParser<uint16_t> parser(thread);
        result = parser.ParseUtf16(*parseString);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue reviver = JSTaggedValue::Undefined();
    if (argc == 2) {  // 2: 2 args
        reviver = GetCallArg(argv, 1).GetTaggedValue();
        if (reviver.IsCallable()) {
            JSHandle<JSTaggedValue> callbackfnHandle(thread, reviver);
            // Let root be ! OrdinaryObjectCreate(%Object.prototype%).
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSTaggedValue> constructor = env->GetObjectFunction();
            JSHandle<JSObject> root = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
            // Let rootName be the empty String.
            JSHandle<JSTaggedValue> rootName(factory->GetEmptyString());
            // Perform ! CreateDataPropertyOrThrow(root, rootName, unfiltered).
            bool success = JSObject::CreateDataProperty(thread, root, rootName, result);
            if (success) {
                result = base::Internalize::InternalizeJsonProperty(thread, root, rootName, callbackfnHandle);
            }
        }
    }
    return result.GetTaggedValue();
}

// 24.5.2
JSTaggedValue BuiltinsJson::Stringify(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Json, Parse);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    array_size_t argc = argv->GetArgsNumber();
    JSTaggedValue value = GetCallArg(argv, 0).GetTaggedValue();
    JSTaggedValue replacer = JSTaggedValue::Undefined();
    JSTaggedValue gap = JSTaggedValue::Undefined();

    if (argc == 2) {  // 2: 2 args
        replacer = GetCallArg(argv, 1).GetTaggedValue();
    } else if (argc == 3) {  // 3: 3 args
        replacer = GetCallArg(argv, 1).GetTaggedValue();
        gap = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD).GetTaggedValue();
    }

    JSHandle<JSTaggedValue> handleValue(thread, value);
    JSHandle<JSTaggedValue> handleReplacer(thread, replacer);
    JSHandle<JSTaggedValue> handleGap(thread, gap);
    panda::ecmascript::base::JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> result = stringifier.Stringify(handleValue, handleReplacer, handleGap);

    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
