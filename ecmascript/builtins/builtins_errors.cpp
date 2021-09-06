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

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::builtins {
using ErrorHelper = base::ErrorHelper;
using ErrorType = base::ErrorType;
// Error
JSTaggedValue BuiltinsError::ErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::ERROR);
}

JSTaggedValue BuiltinsError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::ERROR);
}

// RangeError
JSTaggedValue BuiltinsRangeError::RangeErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::RANGE_ERROR);
}

JSTaggedValue BuiltinsRangeError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::RANGE_ERROR);
}

// ReferenceError
JSTaggedValue BuiltinsReferenceError::ReferenceErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::REFERENCE_ERROR);
}

JSTaggedValue BuiltinsReferenceError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::REFERENCE_ERROR);
}

// TypeError
JSTaggedValue BuiltinsTypeError::TypeErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::TYPE_ERROR);
}

JSTaggedValue BuiltinsTypeError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::TYPE_ERROR);
}

JSTaggedValue BuiltinsTypeError::ThrowTypeError(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handle_scope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "type error", JSTaggedValue::Exception());
}

// URIError
JSTaggedValue BuiltinsURIError::URIErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::URI_ERROR);
}

JSTaggedValue BuiltinsURIError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::URI_ERROR);
}

// SyntaxError
JSTaggedValue BuiltinsSyntaxError::SyntaxErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::SYNTAX_ERROR);
}

JSTaggedValue BuiltinsSyntaxError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::SYNTAX_ERROR);
}

// EvalError
JSTaggedValue BuiltinsEvalError::EvalErrorConstructor(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonConstructor(argv, ErrorType::EVAL_ERROR);
}

JSTaggedValue BuiltinsEvalError::ToString(EcmaRuntimeCallInfo *argv)
{
    return ErrorHelper::ErrorCommonToString(argv, ErrorType::EVAL_ERROR);
}
}  // namespace panda::ecmascript::builtins
