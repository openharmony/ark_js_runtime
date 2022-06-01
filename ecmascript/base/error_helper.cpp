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

#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/error_type.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tooling/backend/js_pt_extractor.h"

namespace panda::ecmascript::base {
JSTaggedValue ErrorHelper::ErrorCommonToString(EcmaRuntimeCallInfo *argv, const ErrorType &errorType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be the this value.
    // 2. If Type(O) is not Object, throw a TypeError exception
    JSHandle<JSTaggedValue> thisValue = BuiltinsBase::GetThis(argv);
    if (!thisValue->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "ErrorToString:not an object", JSTaggedValue::Exception());
    }
    // 3. Let name be Get(O, "name").
    // 4. ReturnIfAbrupt(name).
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> handleName = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> name = JSObject::GetProperty(thread, thisValue, handleName).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If name is undefined, let name be "Error"; otherwise let name be ToString(name).
    // 6. ReturnIfAbrupt(name).
    name = ErrorHelper::GetErrorName(thread, name, errorType);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Let msg be Get(O, "message").
    // 8. ReturnIfAbrupt(msg).
    JSHandle<JSTaggedValue> handleMsg = globalConst->GetHandledMessageString();
    JSHandle<JSTaggedValue> msg = JSObject::GetProperty(thread, thisValue, handleMsg).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 9. If msg is undefined, let msg be the empty String; otherwise let msg be ToString(msg).
    // 10. ReturnIfAbrupt(msg).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (msg->IsUndefined()) {
        msg = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    } else {
        msg = JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, msg));
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 11. If name is the empty String, return msg.
    // 12. If msg is the empty String, return name.
    if (JSHandle<EcmaString>::Cast(name)->GetLength() == 0) {
        return msg.GetTaggedValue();
    }

    if (JSHandle<EcmaString>::Cast(msg)->GetLength() == 0) {
        return name.GetTaggedValue();
    }

    // 13. Return the result of concatenating name, the code unit 0x003A (COLON), the code unit 0x0020 (SPACE), and msg.
    JSHandle<EcmaString> space = factory->NewFromASCII(": ");
    JSHandle<EcmaString> jsHandleName = JSHandle<EcmaString>::Cast(name);
    JSHandle<EcmaString> jsHandleMsg = JSHandle<EcmaString>::Cast(msg);
    JSHandle<EcmaString> handleNameSpace = factory->ConcatFromString(jsHandleName, space);
    JSHandle<EcmaString> result = factory->ConcatFromString(handleNameSpace, jsHandleMsg);
    return result.GetTaggedValue();
}

JSHandle<JSTaggedValue> ErrorHelper::GetErrorName(JSThread *thread, const JSHandle<JSTaggedValue> &name,
                                                  const ErrorType &errorType)
{
    auto globalConst = thread->GlobalConstants();
    if (name->IsUndefined()) {
        TaggedObject *errorKey = nullptr;
        switch (errorType) {
            case ErrorType::RANGE_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledRangeErrorString());
                break;
            case ErrorType::EVAL_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledEvalErrorString());
                break;
            case ErrorType::REFERENCE_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledReferenceErrorString());
                break;
            case ErrorType::TYPE_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledTypeErrorString());
                break;
            case ErrorType::URI_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledURIErrorString());
                break;
            case ErrorType::SYNTAX_ERROR:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledSyntaxErrorString());
                break;
            default:
                errorKey = reinterpret_cast<TaggedObject *>(*globalConst->GetHandledErrorString());
                break;
        }
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue(errorKey));
    }
    return JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, name));
}

JSTaggedValue ErrorHelper::ErrorCommonConstructor(EcmaRuntimeCallInfo *argv,
                                                  [[maybe_unused]] const ErrorType &errorType)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = BuiltinsBase::GetConstructor(argv);
    JSMutableHandle<JSTaggedValue> newTarget(BuiltinsBase::GetNewTarget(argv));
    if (newTarget->IsUndefined()) {
        newTarget.Update(ctor.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> message = BuiltinsBase::GetCallArg(argv, 0);

    // 2. Let O be OrdinaryCreateFromConstructor(newTarget, "%ErrorPrototype%", «[[ErrorData]]»).
    JSHandle<JSObject> nativeInstanceObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), newTarget);

    // 3. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. If message is not undefined, then
    //    a. Let msg be ToString(message).
    //    b. ReturnIfAbrupt(msg).
    //    c. Let msgDesc be the PropertyDescriptor{[[Value]]: msg, [[Writable]]: true, [[Enumerable]]: false,
    //       [[Configurable]]: true}.
    //    d. Let status be DefinePropertyOrThrow(O, "message", msgDesc).
    //    e. Assert: status is not an abrupt completion
    auto globalConst = thread->GlobalConstants();
    if (!message->IsUndefined()) {
        JSHandle<EcmaString> handleStr = JSTaggedValue::ToString(thread, message);
        LOG(DEBUG, ECMASCRIPT) << "Ark throw error: " << utf::Mutf8AsCString(handleStr->GetDataUtf8());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> msgKey = globalConst->GetHandledMessageString();
        PropertyDescriptor msgDesc(thread, JSHandle<JSTaggedValue>::Cast(handleStr), true, false, true);
        [[maybe_unused]] bool status = JSObject::DefineOwnProperty(thread, nativeInstanceObj, msgKey, msgDesc);
        ASSERT_PRINT(status == true, "return result exception!");
    }

    JSHandle<EcmaString> handleStack = BuildEcmaStackTrace(thread);
    JSHandle<JSTaggedValue> stackkey = globalConst->GetHandledStackString();
    PropertyDescriptor stackDesc(thread, JSHandle<JSTaggedValue>::Cast(handleStack), true, false, true);
    [[maybe_unused]] bool status = JSObject::DefineOwnProperty(thread, nativeInstanceObj, stackkey, stackDesc);
    ASSERT_PRINT(status == true, "return result exception!");

    // 5. Return O.
    return nativeInstanceObj.GetTaggedValue();
}

CString ErrorHelper::DecodeFunctionName(const CString &name)
{
    if (name.empty()) {
        return "anonymous";
    }
    return name;
}

JSHandle<EcmaString> ErrorHelper::BuildEcmaStackTrace(JSThread *thread)
{
    CString data = BuildNativeAndJsStackTrace(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    LOG(DEBUG, ECMASCRIPT) << data;
    return factory->NewFromUtf8(data);
}

CString ErrorHelper::BuildNativeEcmaStackTrace(JSThread *thread)
{
    CString data;
    CString fristLineSrcCode;
    bool isFirstLine = true;
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
        auto method = frameHandler.GetMethod();
        if (!method->IsNativeWithCallField()) {
            data.append("    at ");
            data += DecodeFunctionName(method->ParseFunctionName());
            data.append(" (");
            // source file
            tooling::JSPtExtractor *debugExtractor =
                JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
            const CString &sourceFile = debugExtractor->GetSourceFile(method->GetMethodId());
            if (sourceFile.empty()) {
                data.push_back('?');
            } else {
                data += sourceFile;
            }
            data.push_back(':');
            // line number and column number
            int lineNumber = 0;
            auto callbackLineFunc = [&data, &lineNumber](int32_t line) -> bool {
                lineNumber = line + 1;
                data += ToCString(lineNumber);
                data.push_back(':');
                return true;
            };
            auto callbackColumnFunc = [&data](int32_t column) -> bool {
                data += ToCString(column + 1);
                return true;
            };
            panda_file::File::EntityId methodId = method->GetMethodId();
            uint32_t offset = frameHandler.GetBytecodeOffset();
            if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, offset) ||
                !debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, offset)) {
                data.push_back('?');
            }
            data.push_back(')');
            data.push_back('\n');
            if (isFirstLine) {
                const CString &sourceCode = debugExtractor->GetSourceCode(
                    panda_file::File::EntityId(method->GetJSPandaFile()->GetMainMethodIndex()));
                fristLineSrcCode = StringHelper::GetSpecifiedLine(sourceCode, lineNumber);
                isFirstLine = false;
            }
        }
    }
    if (!fristLineSrcCode.empty()) {
        uint32_t codeLen = fristLineSrcCode.length();
        if (fristLineSrcCode[codeLen - 1] == '\r') {
            fristLineSrcCode = fristLineSrcCode.substr(0, codeLen - 1);
        }
        fristLineSrcCode = "SourceCode (" + fristLineSrcCode;
        fristLineSrcCode.push_back(')');
        fristLineSrcCode.push_back('\n');
        data = fristLineSrcCode + data;
    }
    return data;
}

CString ErrorHelper::BuildNativeAndJsStackTrace(JSThread *thread)
{
    CString data = BuildNativeEcmaStackTrace(thread);
    return data;
}
}  // namespace panda::ecmascript::base
