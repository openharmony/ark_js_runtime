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

#include "ecmascript/builtins/builtins_regexp.h"
#include <cmath>
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/assert_scope-inl.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
// 21.2.3.1
JSTaggedValue BuiltinsRegExp::RegExpConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTargetTemp = GetNewTarget(argv);
    JSHandle<JSTaggedValue> pattern = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> flags = GetCallArg(argv, 1);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let patternIsRegExp be IsRegExp(pattern).
    bool patternIsRegExp = JSObject::IsRegExp(thread, pattern);
    // 2. ReturnIfAbrupt(patternIsRegExp).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If NewTarget is not undefined, let newTarget be NewTarget.
    JSHandle<JSTaggedValue> newTarget;
    if (!newTargetTemp->IsUndefined()) {
        newTarget = newTargetTemp;
    } else {
        auto ecmaVm = thread->GetEcmaVM();
        JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        // disable gc
        [[maybe_unused]] DisallowGarbageCollection no_gc;
        // 4.a Let newTarget be the active function object.
        newTarget = env->GetRegExpFunction();
        JSHandle<JSTaggedValue> constructorString = globalConst->GetHandledConstructorString();
        // 4.b If patternIsRegExp is true and flags is undefined
        if (patternIsRegExp && flags->IsUndefined()) {
            // 4.b.i Let patternConstructor be Get(pattern, "constructor").
            JSHandle<JSTaggedValue> patternConstructor =
                JSObject::GetProperty(thread, pattern, constructorString).GetValue();
            // 4.b.ii ReturnIfAbrupt(patternConstructor).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // 4.b.iii If SameValue(newTarget, patternConstructor) is true, return pattern.
            if (JSTaggedValue::SameValue(newTarget.GetTaggedValue(), patternConstructor.GetTaggedValue())) {
                return pattern.GetTaggedValue();
            }
        }
    }
    // 5. If Type(pattern) is Object and pattern has a [[RegExpMatcher]] internal slot
    bool isJsReg = false;
    if (pattern->IsECMAObject()) {
        JSHandle<JSObject> patternObj = JSHandle<JSObject>::Cast(pattern);
        isJsReg = patternObj->IsJSRegExp();
    }
    JSHandle<JSTaggedValue> patternTemp;
    JSHandle<JSTaggedValue> flagsTemp;
    if (isJsReg) {
        JSHandle<JSRegExp> patternReg(thread, JSRegExp::Cast(pattern->GetTaggedObject()));
        // 5.a Let P be the value of pattern’s [[OriginalSource]] internal slot.
        patternTemp = JSHandle<JSTaggedValue>(thread, patternReg->GetOriginalSource());
        if (flags->IsUndefined()) {
            // 5.b If flags is undefined, let F be the value of pattern’s [[OriginalFlags]] internal slot.
            flagsTemp = JSHandle<JSTaggedValue>(thread, patternReg->GetOriginalFlags());
        } else {
            // 5.c Else, let F be flags.
            flagsTemp = flags;
        }
        // 6. Else if patternIsRegExp is true
    } else if (patternIsRegExp) {
        JSHandle<JSTaggedValue> sourceString(factory->NewFromString("source"));
        JSHandle<JSTaggedValue> flagsString(factory->NewFromString("flags"));
        // disable gc
        [[maybe_unused]] DisallowGarbageCollection noGc;
        // 6.a Let P be Get(pattern, "source").
        patternTemp = JSObject::GetProperty(thread, pattern, sourceString).GetValue();
        // 6.b ReturnIfAbrupt(P).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 6.c If flags is undefined
        if (flags->IsUndefined()) {
            // 6.c.i Let F be Get(pattern, "flags").
            flagsTemp = JSObject::GetProperty(thread, pattern, flagsString).GetValue();
            // 6.c.ii ReturnIfAbrupt(F).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            // 6.d Else, let F be flags.
            flagsTemp = flags;
        }
    } else {
        // 7.a Let P be pattern.
        patternTemp = pattern;
        // 7.b Let F be flags.
        flagsTemp = flags;
    }
    // 8. Let O be RegExpAlloc(newTarget).
    JSHandle<JSTaggedValue> object(thread, RegExpAlloc(thread, newTarget));
    // 9. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. Return RegExpInitialize(O, P, F).
    JSTaggedValue result = RegExpInitialize(thread, object, patternTemp, flagsTemp);
    return JSTaggedValue(result);
}

// prototype
// 20.2.5.2
JSTaggedValue BuiltinsRegExp::Exec(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Exec);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 4. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);
    // 5. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. If R does not have a [[RegExpMatcher]] internal slot, throw a TypeError exception.
    if (!thisObj->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[RegExpMatcher]]", JSTaggedValue::Exception());
    }
    // 6. Return RegExpBuiltinExec(R, S).
    JSTaggedValue result = RegExpBuiltinExec(thread, thisObj, string);
    return JSTaggedValue(result);
}

// 20.2.5.13
JSTaggedValue BuiltinsRegExp::Test(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Test);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    // 3. Let string be ToString(S).
    // 4. ReturnIfAbrupt(string).
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }

    // 5. Let match be RegExpExec(R, string).
    JSTaggedValue matchResult = RegExpExec(thread, thisObj, string);
    // 6. ReturnIfAbrupt(match).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. If match is not null, return true; else return false.
    return GetTaggedBoolean(!matchResult.IsNull());
}

// 20.2.5.14
JSTaggedValue BuiltinsRegExp::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    auto ecmaVm = thread->GetEcmaVM();
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> sourceString(factory->NewFromString("source"));
    JSHandle<JSTaggedValue> flagsString(factory->NewFromString("flags"));
    // 3. Let pattern be ToString(Get(R, "source")).
    JSHandle<JSTaggedValue> getSource(JSObject::GetProperty(thread, thisObj, sourceString).GetValue());
    JSHandle<JSTaggedValue> getFlags(JSObject::GetProperty(thread, thisObj, flagsString).GetValue());
    JSHandle<EcmaString> sourceStrHandle = JSTaggedValue::ToString(thread, getSource);
    // 4. ReturnIfAbrupt(pattern).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Let flags be ToString(Get(R, "flags")).
    JSHandle<EcmaString> flagsStrHandle = JSTaggedValue::ToString(thread, getFlags);
    // 4. ReturnIfAbrupt(flags).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> slashStr = factory->NewFromString("/");
    // 7. Let result be the String value formed by concatenating "/", pattern, and "/", and flags.
    JSHandle<EcmaString> tempStr = factory->ConcatFromString(slashStr, sourceStrHandle);
    JSHandle<EcmaString> resultTemp = factory->ConcatFromString(tempStr, slashStr);
    return factory->ConcatFromString(resultTemp, flagsStrHandle).GetTaggedValue();
}

// 20.2.5.3
JSTaggedValue BuiltinsRegExp::GetFlags(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, GetFlags);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. Let result be the empty String.
    // 4. ~ 19.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> result(factory->GetEmptyString());
    result = ConcatFlags(thread, thisObj, result, "global");
    result = ConcatFlags(thread, thisObj, result, "ignoreCase");
    result = ConcatFlags(thread, thisObj, result, "multiline");
    result = ConcatFlags(thread, thisObj, result, "dotAll");
    result = ConcatFlags(thread, thisObj, result, "unicode");
    result = ConcatFlags(thread, thisObj, result, "sticky");
    return JSTaggedValue(static_cast<EcmaString *>(result->GetTaggedObject()));
}

// 20.2.5.4
JSTaggedValue BuiltinsRegExp::GetGlobal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);

    JSHandle<EcmaString> gString = thread->GetEcmaVM()->GetFactory()->NewFromString("g");
    bool result = GetFlagsInternal(thread, thisObj, gString);
    return GetTaggedBoolean(result);
}

// 20.2.5.5
JSTaggedValue BuiltinsRegExp::GetIgnoreCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<EcmaString> iString = thread->GetEcmaVM()->GetFactory()->NewFromString("i");
    bool result = GetFlagsInternal(thread, thisObj, iString);
    return GetTaggedBoolean(result);
}

// 20.2.5.7
JSTaggedValue BuiltinsRegExp::GetMultiline(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<EcmaString> mString = thread->GetEcmaVM()->GetFactory()->NewFromString("m");
    bool result = GetFlagsInternal(thread, thisObj, mString);
    return GetTaggedBoolean(result);
}

JSTaggedValue BuiltinsRegExp::GetDotAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<EcmaString> sString = thread->GetEcmaVM()->GetFactory()->NewFromString("s");
    bool result = GetFlagsInternal(thread, thisObj, sString);
    return GetTaggedBoolean(result);
}

// 20.2.5.10
JSTaggedValue BuiltinsRegExp::GetSource(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let R be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 2. If Type(R) is not Object, throw a TypeError exception.
    // 3. If R does not have an [[OriginalSource]] internal slot, throw a TypeError exception.
    // 4. If R does not have an [[OriginalFlags]] internal slot, throw a TypeError exception.
    if (!thisObj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    if (!thisObj->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[OriginalSource]]", JSTaggedValue::Exception());
    }
    // 5. Let src be the value of R’s [[OriginalSource]] internal slot.
    JSHandle<JSRegExp> regexpObj(thread, JSRegExp::Cast(thisObj->GetTaggedObject()));
    JSHandle<JSTaggedValue> source(thread, regexpObj->GetOriginalSource());
    // 6. Let flags be the value of R’s [[OriginalFlags]] internal slot.
    JSHandle<JSTaggedValue> flags(thread, regexpObj->GetOriginalFlags());
    // 7. Return EscapeRegExpPattern(src, flags).
    return JSTaggedValue(EscapeRegExpPattern(thread, source, flags));
}

// 20.2.5.12
JSTaggedValue BuiltinsRegExp::GetSticky(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<EcmaString> yString = thread->GetEcmaVM()->GetFactory()->NewFromString("y");
    bool result = GetFlagsInternal(thread, thisObj, yString);
    return GetTaggedBoolean(result);
}

// 20.2.5.15
JSTaggedValue BuiltinsRegExp::GetUnicode(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    JSHandle<EcmaString> uString = thread->GetEcmaVM()->GetFactory()->NewFromString("u");
    bool result = GetFlagsInternal(thread, thisObj, uString);
    return GetTaggedBoolean(result);
}

// 21.2.4.2
JSTaggedValue BuiltinsRegExp::GetSpecies(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    return GetThis(argv).GetTaggedValue();
}

// 21.2.5.6
JSTaggedValue BuiltinsRegExp::Match(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Match);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 3. Let S be ToString(string)
    JSHandle<JSTaggedValue> inputString = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputString);
    // 4. ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 5. Let global be ToBoolean(Get(rx, "global")).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> global(factory->NewFromString("global"));
    auto globalValue = JSObject::GetProperty(thread, thisObj, global).GetValue();
    // 6. ReturnIfAbrupt(global).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isGlobal = globalValue->ToBoolean();
    // 7. If global is false, then
    if (!isGlobal) {
        // a. Return RegExpExec(rx, S).
        JSTaggedValue result = RegExpExec(thread, thisObj, string);
        return JSTaggedValue(result);
    }
    // 8. Else global is true
    // a. Let fullUnicode be ToBoolean(Get(rx, "unicode")).
    JSHandle<JSTaggedValue> unicode(factory->NewFromString("unicode"));
    JSHandle<JSTaggedValue> unicodeHandle = JSObject::GetProperty(thread, thisObj, unicode).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool fullUnicode = unicodeHandle->ToBoolean();
    // b. ReturnIfAbrupt(fullUnicode)
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // c. Let setStatus be Set(rx, "lastIndex", 0, true).
    JSHandle<JSTaggedValue> lastIndexString(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    JSObject::SetProperty(thread, thisObj, lastIndexString, value, true);
    // d. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // e. Let A be ArrayCreate(0).
    JSHandle<JSObject> array(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    // f. Let n be 0.
    int resultNum = 0;
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue(0));
    // g. Repeat,
    while (true) {
        // i. Let result be RegExpExec(rx, S).
        result.Update(RegExpExec(thread, thisObj, string));
        // ii. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // iii. If result is null, then
        if (result->IsNull()) {
            // 1. If n=0, return null.
            if (resultNum == 0) {
                return JSTaggedValue::Null();
            }
            // 2. Else, return A.
            return array.GetTaggedValue();
        }
        // iv. Else result is not null,
        // 1. Let matchStr be ToString(Get(result, "0")).
        JSHandle<JSTaggedValue> zoreString(factory->NewFromString("0"));
        JSHandle<JSTaggedValue> matchStr(JSObject::GetProperty(thread, result, zoreString).GetValue());
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, matchStr);
        // 2. ReturnIfAbrupt(matchStr).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> matchValue = JSHandle<JSTaggedValue>::Cast(matchString);
        // 3. Let status be CreateDataProperty(A, ToString(n), matchStr).
        JSObject::CreateDataProperty(thread, array, resultNum, matchValue);
        // 5. If matchStr is the empty String, then
        if (JSTaggedValue::ToString(thread, matchValue)->GetLength() == 0) {
            // a. Let thisIndex be ToLength(Get(rx, "lastIndex")).
            JSHandle<JSTaggedValue> lastIndexHandle =
                JSObject::GetProperty(thread, thisObj, lastIndexString).GetValue();
            JSTaggedNumber thisIndex = JSTaggedValue::ToLength(thread, lastIndexHandle);
            // b. ReturnIfAbrupt(thisIndex).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
            // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
            JSHandle<JSTaggedValue> nextIndex(
                thread, JSTaggedValue(AdvanceStringIndex(thread, string, thisIndex.GetNumber(), fullUnicode)));
            JSObject::SetProperty(thread, thisObj, lastIndexString, nextIndex, true);
            // e. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        // 6. Increment n.
        resultNum++;
    }
}

JSTaggedValue BuiltinsRegExp::RegExpReplaceFast(JSThread *thread, JSHandle<JSTaggedValue> &regexp,
                                                JSHandle<EcmaString> inputString, uint32_t inputLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // get bytecode
    JSTaggedValue bufferData = JSRegExp::Cast(regexp->GetTaggedObject())->GetByteCodeBuffer();
    void *dynBuf = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
    // get flags
    auto bytecodeBuffer = reinterpret_cast<uint8_t *>(dynBuf);
    uint32_t flags = *reinterpret_cast<uint32_t *>(bytecodeBuffer + RegExpParser::FLAGS_OFFSET);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GlobalConstants()->GetHandledLastIndexString());
    uint32_t lastIndex;
    JSHandle<JSRegExp> regexpHandle(regexp);

    if ((flags & (RegExpParser::FLAG_STICKY | RegExpParser::FLAG_GLOBAL)) == 0) {
        lastIndex = 0;
    } else {
        JSHandle<JSTaggedValue> thisIndexHandle = JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue();
        lastIndex = JSTaggedValue::ToLength(thread, thisIndexHandle).GetNumber();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSHandle<JSTaggedValue> tagInputString = JSHandle<JSTaggedValue>::Cast(inputString);
    JSHandle<JSTaggedValue> pattern(thread, regexpHandle->GetOriginalSource());
    JSHandle<JSTaggedValue> flag(thread, regexpHandle->GetOriginalFlags());
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (lastIndex == 0 && inputLength > MIN_REPLACE_STRING_LENGTH) {
        JSTaggedValue cacheResult =
            cacheTable->FindCachedResult(thread, pattern, flag, tagInputString, RegExpExecResultCache::REPLACE_TYPE);
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    std::string resultString;
    uint32_t nextPosition = 0;
    JSMutableHandle<JSTaggedValue> lastIndexValue(thread, JSTaggedValue(lastIndex));

    // 12. Let done be false.
    // 13. Repeat, while done is false
    for (;;) {
        if (lastIndex > inputLength) {
            break;
        }

        bool isUtf16 = inputString->IsUtf16();
        const uint8_t *strBuffer;
        CVector<uint8_t> u8Buffer;
        CVector<uint16_t> u16Buffer;
        if (isUtf16) {
            u16Buffer = CVector<uint16_t>(inputLength);
            inputString->CopyDataUtf16(u16Buffer.data(), inputLength);
            strBuffer = reinterpret_cast<uint8_t *>(u16Buffer.data());
        } else {
            u8Buffer = CVector<uint8_t>(inputLength + 1);
            inputString->CopyDataUtf8(u8Buffer.data(), inputLength + 1);
            strBuffer = u8Buffer.data();
        }

        RegExpExecutor::MatchResult matchResult = Matcher(thread, regexp, strBuffer, inputLength, lastIndex, isUtf16);
        if (!matchResult.isSuccess_) {
            if (flags & (RegExpParser::FLAG_STICKY | RegExpParser::FLAG_GLOBAL)) {
                lastIndex = 0;
                lastIndexValue.Update(JSTaggedValue(lastIndex));
                JSObject::SetProperty(thread, regexp, lastIndexHandle, lastIndexValue, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            break;
        }
        uint32_t startIndex = matchResult.index_;
        uint32_t endIndex = matchResult.endIndex_;
        lastIndex = endIndex;
        if (nextPosition < startIndex) {
            resultString += base::StringHelper::SubString(thread, inputString, nextPosition, startIndex - nextPosition);
        }
        nextPosition = endIndex;
        if (!(flags & RegExpParser::FLAG_GLOBAL)) {
            // a. Let setStatus be Set(R, "lastIndex", e, true).
            lastIndexValue.Update(JSTaggedValue(lastIndex));
            JSObject::SetProperty(thread, regexp, lastIndexHandle, lastIndexValue, true);
            // b. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        }
        if (endIndex == startIndex) {
            bool unicode = inputString->IsUtf16() && (flags & RegExpParser::FLAG_UTF16);
            endIndex = AdvanceStringIndex(thread, tagInputString, endIndex, unicode);
        }
        lastIndex = endIndex;
    }
    resultString += base::StringHelper::SubString(thread, inputString, nextPosition, inputLength - nextPosition);
    JSTaggedValue resultValue = factory->NewFromStdString(resultString).GetTaggedValue();
    if (lastIndex == 0 && inputLength > MIN_REPLACE_STRING_LENGTH) {
        cacheTable->AddResultInCache(thread, pattern, flag, tagInputString, resultValue,
                                     RegExpExecResultCache::REPLACE_TYPE);
    }
    return resultValue;
}

// 21.2.5.8
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsRegExp::Replace(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Replace);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> string = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> inputReplaceValue = GetCallArg(argv, 1);
    JSHandle<EcmaString> srcString = JSTaggedValue::ToString(thread, string);

    // 4. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> inputStr = JSHandle<JSTaggedValue>::Cast(srcString);
    // 5. Let lengthS be the number of code unit elements in S.
    uint32_t length = static_cast<EcmaString *>(inputStr->GetTaggedObject())->GetLength();
    // 6. Let functionalReplace be IsCallable(replaceValue).
    bool functionalReplace = inputReplaceValue->IsCallable();
    JSHandle<EcmaString> replaceValueHandle;
    if (!functionalReplace) {
        replaceValueHandle = JSTaggedValue::ToString(thread, inputReplaceValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> lastIndex(thread->GlobalConstants()->GetHandledLastIndexString());
    // 8. Let global be ToBoolean(Get(rx, "global")).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> global(factory->NewFromString("global"));
    auto globalValue = JSObject::GetProperty(thread, thisObj, global).GetValue();
    // 9. ReturnIfAbrupt(global).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isGlobal = globalValue->ToBoolean();
    // 10. If global is true, then
    bool fullUnicode = false;
    if (isGlobal) {
        // a. Let fullUnicode be ToBoolean(Get(rx, "unicode")).
        JSHandle<JSTaggedValue> unicode(factory->NewFromString("unicode"));
        JSHandle<JSTaggedValue> fullUnicodeHandle = JSObject::GetProperty(thread, thisObj, unicode).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        fullUnicode = fullUnicodeHandle->ToBoolean();
        // b. ReturnIfAbrupt(fullUnicode).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Let setStatus be Set(rx, "lastIndex", 0, true).
        JSHandle<JSTaggedValue> lastIndexValue(thread, JSTaggedValue(0));
        JSObject::SetProperty(thread, thisObj, lastIndex, lastIndexValue, true);
        // d. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    if (isGlobal && !functionalReplace && (replaceValueHandle->GetLength() == 0) && thisObj->IsJSRegExp()) {
        JSHClass *hclass = JSHandle<JSObject>::Cast(thisObj)->GetJSHClass();
        JSHClass *originHClass = JSHClass::Cast(thread->GlobalConstants()->GetJSRegExpClass().GetTaggedObject());
        if (hclass == originHClass) {
            return RegExpReplaceFast(thread, thisObj, srcString, length);
        }
    }

    JSHandle<JSTaggedValue> matchedStr(factory->NewFromString("0"));
    // 11. Let results be a new empty List.
    JSHandle<JSObject> resultsList(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    int resultsIndex = 0;
    // 12. Let done be false.
    // 13. Repeat, while done is false
    JSMutableHandle<JSTaggedValue> nextIndexHandle(thread, JSTaggedValue(0));
    for (;;) {
        // a. Let result be RegExpExec(rx, S).
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, thisObj, inputStr));
        // b. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If result is null, set done to true.
        if (execResult->IsNull()) {
            break;
        }
        // d. Else result is not null, i. Append result to the end of results.
        JSObject::CreateDataProperty(thread, resultsList, resultsIndex, execResult);
        resultsIndex++;
        // ii. If global is false, set done to true.
        if (!isGlobal) {
            break;
        }
        // iii. Else, 1. Let matchStr be ToString(Get(result, "0")).
        JSHandle<JSTaggedValue> getMatch = JSObject::GetProperty(thread, execResult, matchedStr).GetValue();
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatch);
        // 2. ReturnIfAbrupt(matchStr).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 3. If matchStr is the empty String, then
        if (matchString->GetLength() == 0) {
            // a. Let thisIndex be ToLength(Get(rx, "lastIndex")).
            JSHandle<JSTaggedValue> thisIndexHandle = JSObject::GetProperty(thread, thisObj, lastIndex).GetValue();
            uint32_t thisIndex = JSTaggedValue::ToLength(thread, thisIndexHandle).GetNumber();
            // b. ReturnIfAbrupt(thisIndex).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
            uint32_t nextIndex = AdvanceStringIndex(thread, inputStr, thisIndex, fullUnicode);
            nextIndexHandle.Update(JSTaggedValue(nextIndex));
            // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
            JSObject::SetProperty(thread, thisObj, lastIndex, nextIndexHandle, true);
            // e. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }
    // 14. Let accumulatedResult be the empty String value.
    std::string accumulatedResult;
    // 15. Let nextSourcePosition be 0.
    uint32_t nextSourcePosition = 0;
    JSHandle<JSTaggedValue> getMatchString;
    // 16. Repeat, for each result in results,
    for (int i = 0; i < resultsIndex; i++) {
        JSHandle<JSTaggedValue> resultValues =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(resultsList), i).GetValue();
        // a. Let nCaptures be ToLength(Get(result, "length")).
        JSHandle<JSTaggedValue> lengthHandle = thread->GlobalConstants()->GetHandledLengthString();
        JSHandle<JSTaggedValue> ncapturesHandle = JSObject::GetProperty(thread, resultValues, lengthHandle).GetValue();
        uint32_t ncaptures = JSTaggedValue::ToUint32(thread, ncapturesHandle);
        // b. ReturnIfAbrupt(nCaptures).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Let nCaptures be max(nCaptures − 1, 0).
        ncaptures = std::max<uint32_t>((ncaptures - 1), 0);
        // d. Let matched be ToString(Get(result, "0")).
        getMatchString = JSObject::GetProperty(thread, resultValues, matchedStr).GetValue();
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatchString);
        // e. ReturnIfAbrupt(matched).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // f. Let matchLength be the number of code units in matched.
        uint32_t matchLength = matchString->GetLength();
        // g. Let position be ToInteger(Get(result, "index")).
        JSHandle<JSTaggedValue> resultIndex(factory->NewFromString("index"));
        JSHandle<JSTaggedValue> positionHandle = JSObject::GetProperty(thread, resultValues, resultIndex).GetValue();
        uint32_t position = JSTaggedValue::ToUint32(thread, positionHandle);
        // h. ReturnIfAbrupt(position).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // i. Let position be max(min(position, lengthS), 0).
        position = std::max<uint32_t>(std::min<uint32_t>(position, length), 0);
        // j. Let n be 1.
        uint32_t index = 1;
        // k. Let captures be an empty List.
        JSHandle<TaggedArray> capturesList = factory->NewTaggedArray(ncaptures);
        // l. Repeat while n ≤ nCaptures
        while (index <= ncaptures) {
            // i. Let capN be Get(result, ToString(n)).
            JSHandle<JSTaggedValue> capN = JSObject::GetProperty(thread, resultValues, index).GetValue();
            // ii. ReturnIfAbrupt(capN).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. If capN is not undefined, then
            if (!capN->IsUndefined()) {
                // 1. Let capN be ToString(capN).
                JSHandle<EcmaString> capNStr = JSTaggedValue::ToString(thread, capN);
                // 2. ReturnIfAbrupt(capN).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSHandle<JSTaggedValue> capnStr = JSHandle<JSTaggedValue>::Cast(capNStr);
                capturesList->Set(thread, index - 1, capnStr);
            } else {
                // iv. Append capN as the last element of captures.
                capturesList->Set(thread, index - 1, capN);
            }
            // v. Let n be n+1
            ++index;
        }
        // m. If functionalReplace is true, then
        CString replacement;
        JSHandle<TaggedArray> replacerArgs =
            factory->NewTaggedArray(3 + capturesList->GetLength());  // 3: «matched, pos, and string»
        if (functionalReplace) {
            // i. Let replacerArgs be «matched».
            replacerArgs->Set(thread, 0, getMatchString.GetTaggedValue());
            // ii. Append in list order the elements of captures to the end of the List replacerArgs.
            // iii. Append position and S as the last two elements of replacerArgs.
            index = 0;
            while (index < capturesList->GetLength()) {
                replacerArgs->Set(thread, index + 1, capturesList->Get(index));
                ++index;
            }
            replacerArgs->Set(thread, index + 1, JSTaggedValue(position));
            replacerArgs->Set(thread, index + 2, inputStr.GetTaggedValue());  // 2: position of string
            // iv. Let replValue be Call(replaceValue, undefined, replacerArgs).
            JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
            JSTaggedValue replaceResult = JSFunction::Call(thread, inputReplaceValue, undefined, replacerArgs);
            JSHandle<JSTaggedValue> replValue(thread, replaceResult);
            // v. Let replacement be ToString(replValue).
            JSHandle<EcmaString> replacementString = JSTaggedValue::ToString(thread, replValue);
            // o. ReturnIfAbrupt(replacement).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            replacement = ConvertToString(*replacementString, StringConvertedUsage::LOGICOPERATION);
        } else {
            // n. Else,
            JSHandle<JSTaggedValue> replacementHandle(
                thread, BuiltinsString::GetSubstitution(thread, matchString, srcString, position, capturesList,
                                                        replaceValueHandle));
            replacement = ConvertToString(EcmaString::Cast(replacementHandle->GetTaggedObject()),
                                          StringConvertedUsage::LOGICOPERATION);
        }
        // p. If position ≥ nextSourcePosition, then
        if (position >= nextSourcePosition) {
            // ii. Let accumulatedResult be the String formed by concatenating the code units of the current value
            // of accumulatedResult with the substring of S consisting of the code units from nextSourcePosition
            // (inclusive) up to position (exclusive) and with the code units of replacement.
            accumulatedResult += base::StringHelper::SubString(thread, JSHandle<EcmaString>::Cast(inputStr),
                                                               nextSourcePosition, (position - nextSourcePosition));
            accumulatedResult += replacement;
            // iii. Let nextSourcePosition be position + matchLength.
            nextSourcePosition = position + matchLength;
        }
    }
    // 17. If nextSourcePosition ≥ lengthS, return accumulatedResult.
    if (nextSourcePosition >= length) {
        return factory->NewFromStdString(accumulatedResult).GetTaggedValue();
    }
    // 18. Return the String formed by concatenating the code units of accumulatedResult with the substring of S
    // consisting of the code units from nextSourcePosition (inclusive) up through the final code unit of S(inclusive).
    accumulatedResult += base::StringHelper::SubString(thread, JSHandle<EcmaString>::Cast(inputStr), nextSourcePosition,
                                                       (length - nextSourcePosition));
    return factory->NewFromStdString(accumulatedResult).GetTaggedValue();
}

// 21.2.5.9
JSTaggedValue BuiltinsRegExp::Search(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Search);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputStr = GetCallArg(argv, 0);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputStr);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 4. Let previousLastIndex be ? Get(rx, "lastIndex").
    JSHandle<JSTaggedValue> lastIndexString(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> previousLastIndex = JSObject::GetProperty(thread, thisObj, lastIndexString).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If SameValue(previousLastIndex, 0) is false, then
    // Perform ? Set(rx, "lastIndex", 0, true).
    if (!JSTaggedValue::SameValue(previousLastIndex.GetTaggedValue(), JSTaggedValue(0))) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSObject::SetProperty(thread, thisObj, lastIndexString, value, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 6. Let result be ? RegExpExec(rx, S).
    JSHandle<JSTaggedValue> result(thread, RegExpExec(thread, thisObj, string));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let currentLastIndex be ? Get(rx, "lastIndex").
    JSHandle<JSTaggedValue> currentLastIndex = JSObject::GetProperty(thread, thisObj, lastIndexString).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 8. If SameValue(currentLastIndex, previousLastIndex) is false, then
    // Perform ? Set(rx, "lastIndex", previousLastIndex, true).
    if (!JSTaggedValue::SameValue(previousLastIndex.GetTaggedValue(), currentLastIndex.GetTaggedValue())) {
        JSObject::SetProperty(thread, thisObj, lastIndexString, previousLastIndex, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 9. If result is null, return -1.
    if (result->IsNull()) {
        return JSTaggedValue(-1);
    }
    // 10. Return ? Get(result, "index").
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> index(factory->NewFromString("index"));
    return JSObject::GetProperty(thread, result, index).GetValue().GetTaggedValue();
}

// 21.2.5.11
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsRegExp::Split(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), RegExp, Split);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let rx be the this value.
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    auto ecmaVm = thread->GetEcmaVM();
    // 3. Let S be ToString(string).
    JSHandle<JSTaggedValue> inputString = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> limit = GetCallArg(argv, 1);
    JSHandle<EcmaString> stringHandle = JSTaggedValue::ToString(thread, inputString);

    // 4. ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> jsString = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 5. Let C be SpeciesConstructor(rx, %RegExp%).
    JSHandle<JSTaggedValue> defaultConstructor = ecmaVm->GetGlobalEnv()->GetRegExpFunction();
    JSHandle<JSObject> objHandle(thisObj);
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
    // 6. ReturnIfAbrupt(C).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let flags be ToString(Get(rx, "flags")).
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> flagsString(factory->NewFromString("flags"));
    JSHandle<JSTaggedValue> taggedFlags = JSObject::GetProperty(thread, thisObj, flagsString).GetValue();
    JSHandle<EcmaString> flags;

    if (taggedFlags->IsUndefined()) {
        flags = factory->GetEmptyString();
    } else {
        flags = JSTaggedValue::ToString(thread, taggedFlags);
    }
    //  8. ReturnIfAbrupt(flags).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 9. If flags contains "u", let unicodeMatching be true.
    // 10. Else, let unicodeMatching be false.
    JSHandle<EcmaString> uStringHandle(factory->NewFromString("u"));
    bool unicodeMatching = base::StringHelper::Contains(*flags, *uStringHandle);
    // 11. If flags contains "y", let newFlags be flags.
    JSHandle<EcmaString> newFlagsHandle;
    JSHandle<EcmaString> yStringHandle(factory->NewFromString("y"));
    if (base::StringHelper::Contains(*flags, *yStringHandle)) {
        newFlagsHandle = flags;
    } else {
        // 12. Else, let newFlags be the string that is the concatenation of flags and "y".
        JSHandle<EcmaString> yStr = factory->NewFromString("y");
        newFlagsHandle = factory->ConcatFromString(flags, yStr);
    }

    // 17. If limit is undefined, let lim be 2^32–1; else let lim be ToUint32(limit).
    uint32_t lim;
    if (limit->IsUndefined()) {
        lim = MAX_SPLIT_LIMIT;
    } else {
        lim = JSTaggedValue::ToUint32(thread, limit);
        // 18. ReturnIfAbrupt(lim).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSHandle<JSRegExp> regexpHandle(thisObj);
    JSHandle<JSTaggedValue> pattern(thread, regexpHandle->GetOriginalSource());
    JSHandle<JSTaggedValue> flag(thread, regexpHandle->GetOriginalFlags());
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (lim == MAX_SPLIT_LIMIT) {
        JSTaggedValue cacheResult =
            cacheTable->FindCachedResult(thread, pattern, flag, inputString, RegExpExecResultCache::SPLIT_TYPE);
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    // 13. Let splitter be Construct(C, «rx, newFlags»).
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(2);  // 2: «rx, newFlags»
    arguments->Set(thread, 0, thisObj.GetTaggedValue());
    arguments->Set(thread, 1, newFlagsHandle.GetTaggedValue());
    JSTaggedValue taggedSplitter = JSFunction::Construct(thread, constructor, arguments, undefined);
    // 14. ReturnIfAbrupt(splitter).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> splitter(thread, taggedSplitter);
    // 15. Let A be ArrayCreate(0).
    JSHandle<JSObject> array(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    // 16. Let lengthA be 0.
    uint32_t aLength = 0;

    // 19. Let size be the number of elements in S.
    uint32_t size = static_cast<EcmaString *>(jsString->GetTaggedObject())->GetLength();
    // 20. Let p be 0.
    uint32_t startIndex = 0;
    // 21. If lim = 0, return A.
    if (lim == 0) {
        return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
    }
    // 22. If size = 0, then
    if (size == 0) {
        // a. Let z be RegExpExec(splitter, S).
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString));
        // b. ReturnIfAbrupt(z).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If z is not null, return A.
        if (!execResult->IsNull()) {
            return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
        }
        // d. Assert: The following call will never result in an abrupt completion.
        // e. Perform CreateDataProperty(A, "0", S).
        JSObject::CreateDataProperty(thread, array, 0, jsString);
        // f. Return A.
        return JSTaggedValue(static_cast<JSArray *>(array.GetTaggedValue().GetTaggedObject()));
    }
    // 23. Let q be p.
    uint32_t endIndex = startIndex;
    JSMutableHandle<JSTaggedValue> lastIndexvalue(thread, JSTaggedValue(endIndex));
    // 24. Repeat, while q < size
    JSHandle<JSTaggedValue> lastIndexString(thread->GlobalConstants()->GetHandledLastIndexString());
    while (endIndex < size) {
        // a. Let setStatus be Set(splitter, "lastIndex", q, true).
        lastIndexvalue.Update(JSTaggedValue(endIndex));
        JSObject::SetProperty(thread, splitter, lastIndexString, lastIndexvalue, true);
        // b. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString));
        // d. ReturnIfAbrupt(z).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // e. If z is null, let q be AdvanceStringIndex(S, q, unicodeMatching).
        if (execResult->IsNull()) {
            endIndex = AdvanceStringIndex(thread, jsString, endIndex, unicodeMatching);
        } else {
            // f. Else z is not null,
            // i. Let e be ToLength(Get(splitter, "lastIndex")).
            JSHandle<JSTaggedValue> lastIndexHandle =
                JSObject::GetProperty(thread, splitter, lastIndexString).GetValue();
            JSTaggedNumber lastIndexNumber = JSTaggedValue::ToLength(thread, lastIndexHandle);
            // ii. ReturnIfAbrupt(e).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            uint32_t lastIndex = lastIndexNumber.GetNumber();
            // iii. If e = p, let q be AdvanceStringIndex(S, q, unicodeMatching).
            if (lastIndex == startIndex) {
                endIndex = AdvanceStringIndex(thread, jsString, endIndex, unicodeMatching);
            } else {
                // iv. Else e != p,
                // 1. Let T be a String value equal to the substring of S consisting of the elements at indices p
                // (inclusive) through q (exclusive).
                std::string stdStrT = base::StringHelper::SubString(thread, JSHandle<EcmaString>::Cast(jsString),
                                                                    startIndex, (endIndex - startIndex));
                // 2. Assert: The following call will never result in an abrupt completion.
                // 3. Perform CreateDataProperty(A, ToString(lengthA), T).
                JSHandle<JSTaggedValue> tValue(factory->NewFromStdString(stdStrT));
                JSObject::CreateDataProperty(thread, array, aLength, tValue);
                // 4. Let lengthA be lengthA +1.
                ++aLength;
                // 5. If lengthA = lim, return A.
                if (aLength == lim) {
                    if (lim == MAX_SPLIT_LIMIT) {
                        cacheTable->AddResultInCache(thread, pattern, flag, inputString, array.GetTaggedValue(),
                                                     RegExpExecResultCache::SPLIT_TYPE);
                    }
                    return array.GetTaggedValue();
                }
                // 6. Let p be e.
                startIndex = lastIndex;
                // 7. Let numberOfCaptures be ToLength(Get(z, "length")).
                JSHandle<JSTaggedValue> lengthString(factory->NewFromString("length"));
                JSHandle<JSTaggedValue> capturesHandle =
                    JSObject::GetProperty(thread, execResult, lengthString).GetValue();
                JSTaggedNumber numberOfCapturesNumber = JSTaggedValue::ToLength(thread, capturesHandle);
                // 8. ReturnIfAbrupt(numberOfCaptures).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                uint32_t numberOfCaptures = numberOfCapturesNumber.GetNumber();
                // 9. Let numberOfCaptures be max(numberOfCaptures-1, 0).
                numberOfCaptures = (numberOfCaptures == 0) ? 0 : numberOfCaptures - 1;
                // 10. Let i be 1.
                uint32_t i = 1;
                // 11. Repeat, while i ≤ numberOfCaptures.
                while (i <= numberOfCaptures) {
                    // a. Let nextCapture be Get(z, ToString(i)).
                    JSHandle<JSTaggedValue> nextCapture = JSObject::GetProperty(thread, execResult, i).GetValue();
                    // b. ReturnIfAbrupt(nextCapture).
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    // c. Perform CreateDataProperty(A, ToString(lengthA), nextCapture).
                    JSObject::CreateDataProperty(thread, array, aLength, nextCapture);
                    // d. Let i be i + 1.
                    ++i;
                    // e. Let lengthA be lengthA +1.
                    ++aLength;
                    // f. If lengthA = lim, return A.
                    if (aLength == lim) {
                        if (lim == MAX_SPLIT_LIMIT) {
                            cacheTable->AddResultInCache(thread, pattern, flag, inputString, array.GetTaggedValue(),
                                                         RegExpExecResultCache::SPLIT_TYPE);
                        }
                        return array.GetTaggedValue();
                    }
                }
                // 12. Let q be p.
                endIndex = startIndex;
            }
        }
    }
    // 25. Let T be a String value equal to the substring of S consisting of the elements at indices p (inclusive)
    // through size (exclusive).
    std::string stdStrT =
        base::StringHelper::SubString(thread, JSHandle<EcmaString>::Cast(jsString), startIndex, (size - startIndex));
    // 26. Assert: The following call will never result in an abrupt completion.
    // 27. Perform CreateDataProperty(A, ToString(lengthA), t).
    JSHandle<JSTaggedValue> tValue(factory->NewFromStdString(stdStrT));
    JSObject::CreateDataProperty(thread, array, aLength, tValue);
    if (lim == MAX_SPLIT_LIMIT) {
        cacheTable->AddResultInCache(thread, pattern, flag, inputString, array.GetTaggedValue(),
                                     RegExpExecResultCache::SPLIT_TYPE);
    }
    // 28. Return A.
    return array.GetTaggedValue();
}

// NOLINTNEXTLINE(readability-non-const-parameter)
RegExpExecutor::MatchResult BuiltinsRegExp::Matcher(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                                    const uint8_t *buffer, size_t length, int32_t lastIndex,
                                                    bool isUtf16)
{
    // get bytecode
    JSTaggedValue bufferData = JSRegExp::Cast(regexp->GetTaggedObject())->GetByteCodeBuffer();
    void *dynBuf = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
    auto bytecodeBuffer = reinterpret_cast<uint8_t *>(dynBuf);
    // execute
    Chunk chunk(thread->GetRegionFactory());
    RegExpExecutor executor(&chunk);
    if (lastIndex < 0) {
        lastIndex = 0;
    }
    bool ret = executor.Execute(buffer, lastIndex, length, bytecodeBuffer, isUtf16);
    RegExpExecutor::MatchResult result = executor.GetResult(thread, ret);
    return result;
}

uint32_t BuiltinsRegExp::AdvanceStringIndex(JSThread *thread, const JSHandle<JSTaggedValue> &inputStr, uint32_t index,
                                            bool unicode)
{
    // 1. Assert: Type(S) is String.
    ASSERT(inputStr->IsString());
    // 2. Assert: index is an integer such that 0≤index≤2^53 - 1
    ASSERT(index <= pow(2, 53) - 1);
    // 3. Assert: Type(unicode) is Boolean.
    // 4. If unicode is false, return index+1.
    if (!unicode) {
        return index + 1;
    }
    // 5. Let length be the number of code units in S.
    uint32_t length = static_cast<EcmaString *>(inputStr->GetTaggedObject())->GetLength();
    // 6. If index+1 ≥ length, return index+1.
    if (index + 1 >= length) {
        return index + 1;
    }
    // 7. Let first be the code unit value at index index in S.
    uint16_t first = static_cast<EcmaString *>(inputStr->GetTaggedObject())->At(index);
    // 8. If first < 0xD800 or first > 0xDFFF, return index+1.
    if (first < 0xD800 || first > 0xDFFF) {  // NOLINT(readability-magic-numbers)
        return index + 1;
    }
    // 9. Let second be the code unit value at index index+1 in S.
    uint16_t second = static_cast<EcmaString *>(inputStr->GetTaggedObject())->At(index + 1);
    // 10. If second < 0xDC00 or second > 0xDFFF, return index+1.
    if (second < 0xDC00 || second > 0xDFFF) {  // NOLINT(readability-magic-numbers)
        return index + 1;
    }
    // 11. Return index + 2.
    return index + 2;
}

JSHandle<JSTaggedValue> BuiltinsRegExp::ConcatFlags(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &string, const char *name)
{
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> nameString(factory->NewFromString(name));
    bool exist = JSObject::GetProperty(thread, obj, nameString).GetValue()->ToBoolean();
    // ReturnIfAbrupt
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (exist) {
        JSHandle<EcmaString> temp = factory->GetEmptyString();
        if (CString("global") == name) {
            temp = factory->NewFromString("g");
        } else if (CString("ignoreCase") == name) {
            temp = factory->NewFromString("i");
        } else if (CString("multiline") == name) {
            temp = factory->NewFromString("m");
        } else if (CString("dotAll") == name) {
            temp = factory->NewFromString("s");
        } else if (CString("unicode") == name) {
            temp = factory->NewFromString("u");
        } else if (CString("sticky") == name) {
            temp = factory->NewFromString("y");
        }
        JSHandle<EcmaString> thisString(string);
        return JSHandle<JSTaggedValue>(factory->ConcatFromString(thisString, temp));
    }
    return JSHandle<JSTaggedValue>(string);
}

bool BuiltinsRegExp::GetFlagsInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<EcmaString> &flag)
{
    // 1. Let R be the this value.
    // 2. If Type(R) is not Object, throw a TypeError exception.
    if (!obj->IsECMAObject()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", false);
    }
    // 3. If R does not have an [[OriginalFlags]] internal slot, throw a TypeError exception.
    JSHandle<JSObject> patternObj = JSHandle<JSObject>::Cast(obj);
    if (!patternObj->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have [[OriginalFlags]]", false);
    }
    // 4. Let flags be the value of R’s [[OriginalFlags]] internal slot.
    JSHandle<JSRegExp> regexpObj(thread, JSRegExp::Cast(obj->GetTaggedObject()));
    // 5. If flags contains the code unit "[flag]", return true.
    // 6. Return false.
    auto flagsStr = static_cast<EcmaString *>(regexpObj->GetOriginalFlags().GetTaggedObject());
    return base::StringHelper::Contains(flagsStr, *flag);
}

// 21.2.5.2.2
JSTaggedValue BuiltinsRegExp::RegExpBuiltinExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                                const JSHandle<JSTaggedValue> &inputStr)
{
    ASSERT(JSObject::IsRegExp(thread, regexp));
    ASSERT(inputStr->IsString());
    int32_t length = static_cast<EcmaString *>(inputStr->GetTaggedObject())->GetLength();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> lastIndexResult = JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue();
    JSTaggedNumber lastIndexNumber = JSTaggedValue::ToLength(thread, lastIndexResult);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t lastIndex = lastIndexNumber.GetNumber();
    JSHandle<JSTaggedValue> globalHandle(factory->NewFromString("global"));
    bool global = JSObject::GetProperty(thread, regexp, globalHandle).GetValue()->ToBoolean();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> stickyHandle(factory->NewFromString("sticky"));
    bool sticky = JSObject::GetProperty(thread, regexp, stickyHandle).GetValue()->ToBoolean();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!global && !sticky) {
        lastIndex = 0;
    }
    JSHandle<JSRegExp> regexpObj(thread, JSRegExp::Cast(regexp->GetTaggedObject()));
    auto flagsStr = static_cast<EcmaString *>(regexpObj->GetOriginalFlags().GetTaggedObject());
    JSHandle<EcmaString> uString = factory->NewFromString("u");
    [[maybe_unused]] bool fullUnicode = base::StringHelper::Contains(flagsStr, *uString);
    if (lastIndex > length) {
        JSHandle<JSTaggedValue> lastIndexValue(thread, JSTaggedValue(0));
        JSObject::SetProperty(thread, regexp, lastIndexHandle, lastIndexValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue::Null();
    }
    JSHandle<EcmaString> inputString = JSTaggedValue::ToString(thread, inputStr);
    bool isUtf16 = inputString->IsUtf16();
    const uint8_t *strBuffer;
    size_t stringLength = inputString->GetLength();
    CVector<uint8_t> u8Buffer;
    CVector<uint16_t> u16Buffer;
    if (isUtf16) {
        u16Buffer = CVector<uint16_t>(stringLength);
        inputString->CopyDataUtf16(u16Buffer.data(), stringLength);
        strBuffer = reinterpret_cast<uint8_t *>(u16Buffer.data());
    } else {
        u8Buffer = CVector<uint8_t>(stringLength + 1);
        inputString->CopyDataUtf8(u8Buffer.data(), stringLength + 1);
        strBuffer = u8Buffer.data();
    }
    RegExpExecutor::MatchResult matchResult = Matcher(thread, regexp, strBuffer, stringLength, lastIndex, isUtf16);
    if (!matchResult.isSuccess_) {
        if (global || sticky) {
            JSHandle<JSTaggedValue> lastIndexValue(thread, JSTaggedValue(0));
            JSObject::SetProperty(thread, regexp, lastIndexHandle, lastIndexValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue::Null();
    }
    uint32_t endIndex = matchResult.endIndex_;
    if (global || sticky) {
        // a. Let setStatus be Set(R, "lastIndex", e, true).
        JSHandle<JSTaggedValue> lastIndexValue(thread, JSTaggedValue(endIndex));
        JSObject::SetProperty(thread, regexp, lastIndexHandle, lastIndexValue, true);
        // b. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    uint32_t capturesSize = matchResult.captures_.size();
    JSHandle<JSObject> results(JSArray::ArrayCreate(thread, JSTaggedNumber(capturesSize)));
    uint32_t matchIndex = matchResult.index_;
    // 24. Perform CreateDataProperty(A, "index", matchIndex).
    JSHandle<JSTaggedValue> indexKey(factory->NewFromString("index"));
    JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(matchIndex));
    JSObject::CreateDataProperty(thread, results, indexKey, indexValue);
    // 25. Perform CreateDataProperty(A, "input", S).
    JSHandle<JSTaggedValue> inputKey(factory->NewFromString("input"));

    JSHandle<JSTaggedValue> inputValue(thread, static_cast<EcmaString *>(inputStr->GetTaggedObject()));
    JSObject::CreateDataProperty(thread, results, inputKey, inputValue);
    // 27. Perform CreateDataProperty(A, "0", matched_substr).
    JSHandle<JSTaggedValue> zeroValue(matchResult.captures_[0].second);
    JSObject::CreateDataProperty(thread, results, 0, zeroValue);
    // 28. For each integer i such that i > 0 and i <= n
    for (uint32_t i = 1; i < capturesSize; i++) {
        // a. Let capture_i be ith element of r's captures List
        JSTaggedValue capturedValue;
        if (matchResult.captures_[i].first) {
            capturedValue = JSTaggedValue::Undefined();
        } else {
            capturedValue = matchResult.captures_[i].second.GetTaggedValue();
        }
        JSHandle<JSTaggedValue> iValue(thread, capturedValue);
        JSObject::CreateDataProperty(thread, results, i, iValue);
    }
    // 29. Return A.
    return results.GetTaggedValue();
}

// 21.2.5.2.1
JSTaggedValue BuiltinsRegExp::RegExpExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                         const JSHandle<JSTaggedValue> &inputString)
{
    // 1. Assert: Type(R) is Object.
    ASSERT(regexp->IsECMAObject());
    // 2. Assert: Type(S) is String.
    ASSERT(inputString->IsString());
    // 3. Let exec be Get(R, "exec").
    JSHandle<JSObject> thisObj(thread, regexp->GetTaggedObject());
    JSHandle<EcmaString> inputStr = JSTaggedValue::ToString(thread, inputString);

    JSHandle<JSTaggedValue> execHandle(thread->GlobalConstants()->GetHandledExecString());
    JSHandle<JSTaggedValue> exec =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(thisObj), execHandle).GetValue();
    // 4. ReturnIfAbrupt(exec).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If IsCallable(exec) is true, then
    if (exec->IsCallable()) {
        JSHClass *hclass = JSHandle<JSObject>::Cast(regexp)->GetJSHClass();
        JSHClass *originHClass = JSHClass::Cast(thread->GlobalConstants()->GetJSRegExpClass().GetTaggedObject());
        if (hclass == originHClass) {
            // 7. Return RegExpBuiltinExec(R, S).
            return RegExpBuiltinExec(thread, regexp, inputString);
        }
        JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>::Cast(thisObj);
        JSHandle<TaggedArray> arguments = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1);
        arguments->Set(thread, 0, inputStr.GetTaggedValue());
        JSTaggedValue result = JSFunction::Call(thread, exec, obj, arguments);
        // b. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!result.IsECMAObject() && !result.IsNull()) {
            // throw a TypeError exception.
            THROW_TYPE_ERROR_AND_RETURN(thread, "exec result is null or is not Object", JSTaggedValue::Exception());
        }
        return result;
    }
    // 6. If R does not have a [[RegExpMatcher]] internal slot, throw a TypeError exception.
    if (!thisObj->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have a [[RegExpMatcher]]", JSTaggedValue::Exception());
    }
    // 7. Return RegExpBuiltinExec(R, S).
    return RegExpBuiltinExec(thread, regexp, inputString);
}

// 21.2.3.2.1
JSTaggedValue BuiltinsRegExp::RegExpAlloc(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget)
{
    /**
     * 1. Let obj be OrdinaryCreateFromConstructor(newTarget, "%RegExpPrototype%",
     * «[[RegExpMatcher]],[[OriginalSource]], [[OriginalFlags]]»).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> func = env->GetRegExpFunction();
    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(func), newTarget));
    // 2. ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Return obj.
    return obj.GetTaggedValue();
}

uint32_t BuiltinsRegExp::UpdateExpressionFlags(JSThread *thread, const CString &checkStr)
{
    uint32_t flagsBits = 0;
    uint32_t flagsBitsTemp = 0;
    for (char i : checkStr) {
        switch (i) {
            case 'g':
                flagsBitsTemp = RegExpParser::FLAG_GLOBAL;
                break;
            case 'i':
                flagsBitsTemp = RegExpParser::FLAG_IGNORECASE;
                break;
            case 'm':
                flagsBitsTemp = RegExpParser::FLAG_MULTILINE;
                break;
            case 's':
                flagsBitsTemp = RegExpParser::FLAG_DOTALL;
                break;
            case 'u':
                flagsBitsTemp = RegExpParser::FLAG_UTF16;
                break;
            case 'y':
                flagsBitsTemp = RegExpParser::FLAG_STICKY;
                break;
            default: {
                ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
                JSHandle<JSObject> syntaxError =
                    factory->GetJSError(base::ErrorType::SYNTAX_ERROR, "invalid regular expression flags");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), 0);
            }
        }
        if ((flagsBits & flagsBitsTemp) != 0) {
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSObject> syntaxError =
                factory->GetJSError(base::ErrorType::SYNTAX_ERROR, "invalid regular expression flags");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), 0);
        }
        flagsBits |= flagsBitsTemp;
    }
    return flagsBits;
}

// 21.2.3.2.2
JSTaggedValue BuiltinsRegExp::RegExpInitialize(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &pattern,
                                               const JSHandle<JSTaggedValue> &flags)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> patternStrHandle;
    JSHandle<EcmaString> flagsStrHandle;
    // 1. If pattern is undefined, let P be the empty String.
    if (pattern->IsUndefined()) {
        patternStrHandle = factory->GetEmptyString();
    } else {
        // 2. Else, let P be ToString(pattern).
        patternStrHandle = JSTaggedValue::ToString(thread, pattern);
        // 3. ReturnIfAbrupt(P).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 4. If flags is undefined, let F be the empty String.
    if (flags->IsUndefined()) {
        flagsStrHandle = factory->GetEmptyString();
    } else {
        // 5. Else, let F be ToString(flags).
        flagsStrHandle = JSTaggedValue::ToString(thread, flags);
        // 6. ReturnIfAbrupt(F).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    /**
     * 7. If F contains any code unit other than "g", "i", "m", "u", or "y" or if it contains the same code
     * unit more than once, throw a SyntaxError exception.
     **/
    CString checkStr = ConvertToString(*flagsStrHandle, StringConvertedUsage::LOGICOPERATION);
    uint32_t flagsBits = UpdateExpressionFlags(thread, checkStr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // String -> CString
    CString patternStdStr = ConvertToString(*patternStrHandle, StringConvertedUsage::LOGICOPERATION);
    // 9. 10.
    Chunk chunk(thread->GetRegionFactory());
    RegExpParser parser = RegExpParser(&chunk);
    RegExpParserCache *regExpParserCache = thread->GetEcmaVM()->GetRegExpParserCache();
    auto getCache = regExpParserCache->GetCache(*patternStrHandle, flagsBits);
    if (getCache.first == JSTaggedValue::Hole()) {
        parser.Init(const_cast<char *>(reinterpret_cast<const char *>(patternStdStr.c_str())), patternStdStr.size(),
                    flagsBits);
        parser.Parse();
        if (parser.IsError()) {
            JSHandle<JSObject> syntaxError =
                factory->GetJSError(base::ErrorType::SYNTAX_ERROR, parser.GetErrorMsg().c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, syntaxError.GetTaggedValue(), JSTaggedValue::Exception());
        }
    }
    JSHandle<JSRegExp> regexp(thread, JSRegExp::Cast(obj->GetTaggedObject()));
    // 11. Set the value of obj’s [[OriginalSource]] internal slot to P.
    regexp->SetOriginalSource(thread, patternStrHandle.GetTaggedValue());
    // 12. Set the value of obj’s [[OriginalFlags]] internal slot to F.
    regexp->SetOriginalFlags(thread, flagsStrHandle.GetTaggedValue());
    // 13. Set obj’s [[RegExpMatcher]] internal slot.
    if (getCache.first == JSTaggedValue::Hole()) {
        auto bufferSize = parser.GetOriginBufferSize();
        auto buffer = parser.GetOriginBuffer();
        factory->NewJSRegExpByteCodeData(regexp, buffer, bufferSize);
        regExpParserCache->SetCache(*patternStrHandle, flagsBits, regexp->GetByteCodeBuffer(), bufferSize);
    } else {
        regexp->SetByteCodeBuffer(thread, getCache.first);
        regexp->SetLength(thread, JSTaggedValue(static_cast<uint32_t>(getCache.second)));
    }
    // 14. Let setStatus be Set(obj, "lastIndex", 0, true).
    JSHandle<JSTaggedValue> lastIndexString(thread->GlobalConstants()->GetHandledLastIndexString());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    JSObject::SetProperty(thread, obj, lastIndexString, value, true);
    // 15. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 16. Return obj.
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsRegExp::RegExpCreate(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                           const JSHandle<JSTaggedValue> &flags)
{
    BUILTINS_API_TRACE(thread, RegExp, Create);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> newTarget = env->GetRegExpFunction();
    // 1. Let obj be RegExpAlloc(%RegExp%).
    JSHandle<JSTaggedValue> object(thread, RegExpAlloc(thread, newTarget));
    // 2. ReturnIfAbrupt(obj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return RegExpInitialize(obj, P, F).
    return RegExpInitialize(thread, object, pattern, flags);
}

// 21.2.3.2.4
EcmaString *BuiltinsRegExp::EscapeRegExpPattern(JSThread *thread, const JSHandle<JSTaggedValue> &src,
                                                const JSHandle<JSTaggedValue> &flag)
{
    // String -> CString
    JSHandle<EcmaString> srcStr(thread, static_cast<EcmaString *>(src->GetTaggedObject()));
    JSHandle<EcmaString> flagStr(thread, static_cast<EcmaString *>(flag->GetTaggedObject()));
    CString srcStdStr = ConvertToString(*srcStr, StringConvertedUsage::LOGICOPERATION);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // "" -> (?:)
    if (srcStdStr.empty()) {
        srcStdStr = "(?:)";
    }
    // "/" -> "\/"
    srcStdStr = base::StringHelper::RepalceAll(srcStdStr, "/", "\\/");
    // "\\" -> "\"
    srcStdStr = base::StringHelper::RepalceAll(srcStdStr, "\\", "\\");

    return *factory->NewFromString(srcStdStr);
}

JSTaggedValue RegExpExecResultCache::CreateCacheTable(JSThread *thread)
{
    int length = CACHE_TABLE_HEADER_SIZE + DEFAULT_CACHE_NUMBER * ENTRY_SIZE;

    auto table = static_cast<RegExpExecResultCache *>(*thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(length));
    table->SetHitCount(thread, 0);
    table->SetCacheCount(thread, 0);

    return JSTaggedValue(table);
}

JSTaggedValue RegExpExecResultCache::FindCachedResult(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                                      const JSHandle<JSTaggedValue> &flag,
                                                      const JSHandle<JSTaggedValue> &input, CacheType type)
{
    JSHandle<EcmaString> patternStr(pattern);
    JSHandle<EcmaString> flagStr(flag);
    JSHandle<EcmaString> inputStr(input);
    JSTaggedValue patternValue = pattern.GetTaggedValue();
    JSTaggedValue flagValue = flag.GetTaggedValue();
    JSTaggedValue inputValue = input.GetTaggedValue();

    if (!pattern->IsString() || !flag->IsString() || !input->IsString()) {
        return JSTaggedValue::Undefined();
    }

    uint32_t hash = pattern->GetKeyHashCode() + flag->GetKeyHashCode() + input->GetKeyHashCode();
    uint32_t entry = hash & (RegExpExecResultCache::DEFAULT_CACHE_NUMBER - 1);
    if (!Match(entry, patternValue, flagValue, inputValue)) {
        uint32_t entry2 = (entry + 1) & (DEFAULT_CACHE_NUMBER - 1);
        if (!Match(entry2, patternValue, flagValue, inputValue)) {
            return JSTaggedValue::Undefined();
        }
        entry = entry2;
    }
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    JSTaggedValue result;
    switch (type) {
        case REPLACE_TYPE:
            result = Get(index + RESULT_REPLACE_INDEX);
            break;
        case SPLIT_TYPE:
            result = Get(index + RESULT_SPLIT_INDEX);
            break;
        default:
            UNREACHABLE();
            break;
    }
    SetHitCount(thread, GetHitCount() + 1);
    return result;
}

void RegExpExecResultCache::AddResultInCache(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                             const JSHandle<JSTaggedValue> &flag, const JSHandle<JSTaggedValue> &input,
                                             JSTaggedValue resultArray, CacheType type)
{
    JSHandle<EcmaString> patternStr(pattern);
    JSHandle<EcmaString> flagStr(flag);
    JSHandle<EcmaString> inputStr(input);

    if (!pattern->IsString() || !flag->IsString() || !input->IsString()) {
        return;
    }

    JSTaggedValue patternValue = pattern.GetTaggedValue();
    JSTaggedValue flagValue = flag.GetTaggedValue();
    JSTaggedValue inputValue = input.GetTaggedValue();

    uint32_t hash = pattern->GetKeyHashCode() + flag->GetKeyHashCode() + input->GetKeyHashCode();
    uint32_t entry = hash & (DEFAULT_CACHE_NUMBER - 1);
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    if (Get(index) == JSTaggedValue::Undefined()) {
        SetCacheCount(thread, GetCacheCount() + 1);
        SetEntry(thread, entry, patternValue, flagValue, inputValue);
        UpdateResultArray(thread, entry, resultArray, type);
    } else if (Match(entry, patternValue, flagValue, inputValue)) {
        UpdateResultArray(thread, entry, resultArray, type);
    } else {
        uint32_t entry2 = (entry + 1) & (DEFAULT_CACHE_NUMBER - 1);
        uint32_t index2 = CACHE_TABLE_HEADER_SIZE + entry2 * ENTRY_SIZE;
        if (Get(index2) == JSTaggedValue::Undefined()) {
            SetCacheCount(thread, GetCacheCount() + 1);
            SetEntry(thread, entry2, patternValue, flagValue, inputValue);
            UpdateResultArray(thread, entry2, resultArray, type);
        } else if (Match(entry2, patternValue, flagValue, inputValue)) {
            UpdateResultArray(thread, entry2, resultArray, type);
        } else {
            SetCacheCount(thread, GetCacheCount() - 1);
            ClearEntry(thread, entry2);
            SetEntry(thread, entry, patternValue, flagValue, inputValue);
            UpdateResultArray(thread, entry, resultArray, type);
        }
    }
}

void RegExpExecResultCache::SetEntry(JSThread *thread, int entry, JSTaggedValue &pattern, JSTaggedValue &flag,
                                     JSTaggedValue &input)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    Set(thread, index + PATTERN_INDEX, pattern);
    Set(thread, index + FLAG_INDEX, flag);
    Set(thread, index + INPUT_STRING_INDEX, input);
}

void RegExpExecResultCache::UpdateResultArray(JSThread *thread, int entry, JSTaggedValue resultArray, CacheType type)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    switch (type) {
        break;
        case REPLACE_TYPE:
            Set(thread, index + RESULT_REPLACE_INDEX, resultArray);
            break;
        case SPLIT_TYPE:
            Set(thread, index + RESULT_SPLIT_INDEX, resultArray);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void RegExpExecResultCache::ClearEntry(JSThread *thread, int entry)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    JSTaggedValue undefined = JSTaggedValue::Undefined();
    for (int i = 0; i < ENTRY_SIZE; i++) {
        Set(thread, index + i, undefined);
    }
}
bool RegExpExecResultCache::Match(int entry, JSTaggedValue &pattern, JSTaggedValue &flag, JSTaggedValue &input)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    JSTaggedValue keyPattern = Get(index + PATTERN_INDEX);
    JSTaggedValue keyFlag = Get(index + FLAG_INDEX);
    JSTaggedValue keyInput = Get(index + INPUT_STRING_INDEX);

    if (keyPattern == JSTaggedValue::Undefined()) {
        return false;
    }

    EcmaString *patternStr = EcmaString::Cast(pattern.GetTaggedObject());
    EcmaString *flagStr = EcmaString::Cast(flag.GetTaggedObject());
    EcmaString *inputStr = EcmaString::Cast(input.GetTaggedObject());
    EcmaString *keyPatternStr = EcmaString::Cast(keyPattern.GetTaggedObject());
    EcmaString *keyFlagStr = EcmaString::Cast(keyFlag.GetTaggedObject());
    EcmaString *keyInputStr = EcmaString::Cast(keyInput.GetTaggedObject());
    return EcmaString::StringsAreEqual(patternStr, keyPatternStr) && EcmaString::StringsAreEqual(flagStr, keyFlagStr) &&
           EcmaString::StringsAreEqual(inputStr, keyInputStr);
}
}  // namespace panda::ecmascript::builtins
