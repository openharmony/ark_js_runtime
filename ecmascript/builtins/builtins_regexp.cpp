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
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/assert_scope.h"
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
            JSTaggedValue patternConstructor = FastRuntimeStub::FastGetPropertyByName(
                thread, pattern.GetTaggedValue(), constructorString.GetTaggedValue());
            // 4.b.ii ReturnIfAbrupt(patternConstructor).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // 4.b.iii If SameValue(newTarget, patternConstructor) is true, return pattern.
            if (JSTaggedValue::SameValue(newTarget.GetTaggedValue(), patternConstructor)) {
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
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
        }
        // 6. Else if patternIsRegExp is true
    } else if (patternIsRegExp) {
        JSHandle<JSTaggedValue> sourceString(factory->NewFromASCII("source"));
        JSHandle<JSTaggedValue> flagsString(factory->NewFromASCII("flags"));
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
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
        }
    } else {
        // 7.a Let P be pattern.
        patternTemp = pattern;
        // 7.b Let F be flags.
        if (flags->IsUndefined()) {
            flagsTemp = flags;
        } else {
            flagsTemp = JSHandle<JSTaggedValue>(thread, *JSTaggedValue::ToString(thread, flags));
        }
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

    bool useCache = true;
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (cacheTable->GetLargeStrCount() == 0 || cacheTable->GetConflictCount() == 0) {
        useCache = false;
    }

    // 6. Return RegExpBuiltinExec(R, S).
    JSTaggedValue result = RegExpBuiltinExec(thread, thisObj, string, useCache);
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
    JSTaggedValue matchResult = RegExpExec(thread, thisObj, string, false);
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
    JSHandle<JSTaggedValue> sourceString(factory->NewFromASCII("source"));
    JSHandle<JSTaggedValue> flagsString(factory->NewFromASCII("flags"));
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
    JSHandle<EcmaString> slashStr = factory->NewFromASCII("/");
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
    ASSERT(JSHandle<JSObject>::Cast(thisObj)->IsJSRegExp());
    uint8_t flagsBits = static_cast<uint8_t>(JSRegExp::Cast(thisObj->GetTaggedObject())->GetOriginalFlags().GetInt());
    return FlagsBitsToString(thread, flagsBits);
}

// 20.2.5.4
JSTaggedValue BuiltinsRegExp::GetGlobal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_GLOBAL);
    return GetTaggedBoolean(result);
}

// 20.2.5.5
JSTaggedValue BuiltinsRegExp::GetIgnoreCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_IGNORECASE);
    return GetTaggedBoolean(result);
}

// 20.2.5.7
JSTaggedValue BuiltinsRegExp::GetMultiline(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_MULTILINE);
    return GetTaggedBoolean(result);
}

JSTaggedValue BuiltinsRegExp::GetDotAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_DOTALL);
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
    uint8_t flagsBits = static_cast<uint8_t>(regexpObj->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> flags(thread, FlagsBitsToString(thread, flagsBits));
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
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_STICKY);
    return GetTaggedBoolean(result);
}

// 20.2.5.15
JSTaggedValue BuiltinsRegExp::GetUnicode(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj = GetThis(argv);
    bool result = GetFlagsInternal(thread, thisObj, RegExpParser::FLAG_UTF16);
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
    bool useCache = true;
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (cacheTable->GetLargeStrCount() == 0 || cacheTable->GetConflictCount() == 0) {
        useCache = false;
    }
    // 4. ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> string = JSHandle<JSTaggedValue>::Cast(stringHandle);
    if (!thisObj->IsECMAObject()) {
        // 2. If Type(rx) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not Object", JSTaggedValue::Exception());
    }
    // 5. Let global be ToBoolean(Get(rx, "global")).
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> global = globalConst->GetHandledGlobalString();
    JSTaggedValue globalValue =
        FastRuntimeStub::FastGetPropertyByName(thread, thisObj.GetTaggedValue(), global.GetTaggedValue());
    // 6. ReturnIfAbrupt(global).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSRegExp> regexpObj(thisObj);
    JSMutableHandle<JSTaggedValue> pattern(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> flags(thread, JSTaggedValue::Undefined());
    if (thisObj->IsJSRegExp()) {
        pattern.Update(regexpObj->GetOriginalSource());
        flags.Update(regexpObj->GetOriginalFlags());
    }
    bool isGlobal = globalValue.ToBoolean();
    // 7. If global is false, then
    if (!isGlobal) {
        // a. Return RegExpExec(rx, S).
        if (useCache) {
            JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flags, inputString,
                                                                     RegExpExecResultCache::EXEC_TYPE, thisObj);
            if (cacheResult != JSTaggedValue::Undefined()) {
                return cacheResult;
            }
        }
        JSTaggedValue result = RegExpExec(thread, thisObj, string, useCache);
        return JSTaggedValue(result);
    }

    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flags, inputString,
                                                                 RegExpExecResultCache::MATCH_TYPE, thisObj);
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    // 8. Else global is true
    // a. Let fullUnicode be ToBoolean(Get(rx, "unicode")).
    JSHandle<JSTaggedValue> unicode = globalConst->GetHandledUnicodeString();
    JSTaggedValue uincodeValue =
        FastRuntimeStub::FastGetProperty(thread, thisObj.GetTaggedValue(), unicode.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool fullUnicode = uincodeValue.ToBoolean();
    // b. ReturnIfAbrupt(fullUnicode)
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // c. Let setStatus be Set(rx, "lastIndex", 0, true).
    JSHandle<JSTaggedValue> lastIndexString(globalConst->GetHandledLastIndexString());
    FastRuntimeStub::FastSetProperty(thread, thisObj.GetTaggedValue(), lastIndexString.GetTaggedValue(),
                                     JSTaggedValue(0), true);
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
        result.Update(RegExpExec(thread, thisObj, string, useCache));

        // ii. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // iii. If result is null, then
        if (result->IsNull()) {
            // 1. If n=0, return null.
            if (resultNum == 0) {
                return JSTaggedValue::Null();
            }
            if (useCache) {
                RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flags, inputString,
                                                        JSHandle<JSTaggedValue>(array),
                                                        RegExpExecResultCache::MATCH_TYPE, 0);
            }
            // 2. Else, return A.
            return array.GetTaggedValue();
        }
        // iv. Else result is not null,
        // 1. Let matchStr be ToString(Get(result, "0")).
        JSHandle<JSTaggedValue> zeroString = globalConst->GetHandledZeroString();
        JSHandle<JSTaggedValue> matchStr(
            thread, FastRuntimeStub::FastGetProperty(thread, result.GetTaggedValue(), zeroString.GetTaggedValue()));
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, matchStr);
        // 2. ReturnIfAbrupt(matchStr).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> matchValue = JSHandle<JSTaggedValue>::Cast(matchString);
        // 3. Let status be CreateDataProperty(A, ToString(n), matchStr).
        JSObject::CreateDataProperty(thread, array, resultNum, matchValue);
        // 5. If matchStr is the empty String, then
        if (JSTaggedValue::ToString(thread, matchValue)->GetLength() == 0) {
            // a. Let thisIndex be ToLength(Get(rx, "lastIndex")).
            JSHandle<JSTaggedValue> lastIndexHandle(
                thread,
                FastRuntimeStub::FastGetProperty(thread, thisObj.GetTaggedValue(), lastIndexString.GetTaggedValue()));
            JSTaggedNumber thisIndex = JSTaggedValue::ToLength(thread, lastIndexHandle);
            // b. ReturnIfAbrupt(thisIndex).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
            // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
            JSTaggedValue nextIndex =
                JSTaggedValue(AdvanceStringIndex(string, thisIndex.GetNumber(), fullUnicode));
            FastRuntimeStub::FastSetProperty(thread, thisObj.GetTaggedValue(), lastIndexString.GetTaggedValue(),
                                             nextIndex, true);
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
    ASSERT(regexp->IsJSRegExp());
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
    bool useCache = false;
    if ((flags & (RegExpParser::FLAG_STICKY | RegExpParser::FLAG_GLOBAL)) == 0) {
        lastIndex = 0;
    } else {
        JSTaggedValue thisIndex =
            FastRuntimeStub::FastGetProperty(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue());
        if (thisIndex.IsInt()) {
            lastIndex = static_cast<uint32_t>(thisIndex.GetInt());
        } else {
            JSHandle<JSTaggedValue> thisIndexHandle(thread, thisIndex);
            lastIndex = JSTaggedValue::ToLength(thread, thisIndexHandle).GetNumber();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }

    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> tagInputString = JSHandle<JSTaggedValue>::Cast(inputString);
    JSHandle<JSTaggedValue> pattern(thread, regexpHandle->GetOriginalSource());
    JSHandle<JSTaggedValue> flagsBits(thread, regexpHandle->GetOriginalFlags());

    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    uint32_t length = inputString->GetLength();
    uint32_t largeStrCount = cacheTable->GetLargeStrCount();
    if (largeStrCount != 0) {
        if (length > MIN_REPLACE_STRING_LENGTH) {
            cacheTable->SetLargeStrCount(thread, --largeStrCount);
        }
    } else {
        cacheTable->SetStrLenThreshold(thread, MIN_REPLACE_STRING_LENGTH);
    }
    if (length > cacheTable->GetStrLenThreshold()) {
        useCache = true;
    }
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flagsBits, tagInputString,
                                                                 RegExpExecResultCache::REPLACE_TYPE, regexp,
                                                                 globalConst->GetEmptyString());
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    std::string resultString;
    uint32_t nextPosition = 0;

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
                FastRuntimeStub::FastSetProperty(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                                 JSTaggedValue(0), true);
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
            FastRuntimeStub::FastSetProperty(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                             JSTaggedValue(lastIndex), true);
            // b. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        }
        if (endIndex == startIndex) {
            bool unicode = inputString->IsUtf16() && (flags & RegExpParser::FLAG_UTF16);
            endIndex = AdvanceStringIndex(tagInputString, endIndex, unicode);
        }
        lastIndex = endIndex;
    }
    resultString += base::StringHelper::SubString(thread, inputString, nextPosition, inputLength - nextPosition);
    auto resultValue = factory->NewFromStdString(resultString);
    if (useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, tagInputString,
                                                JSHandle<JSTaggedValue>(resultValue),
                                                RegExpExecResultCache::REPLACE_TYPE, lastIndex,
                                                globalConst->GetEmptyString());
    }
    return resultValue.GetTaggedValue();
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
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 4. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> inputStr = JSHandle<JSTaggedValue>::Cast(srcString);
    // 5. Let lengthS be the number of code unit elements in S.
    uint32_t length = srcString->GetLength();
    // 6. Let functionalReplace be IsCallable(replaceValue).
    bool functionalReplace = inputReplaceValue->IsCallable();
    JSHandle<EcmaString> replaceValueHandle;
    if (!functionalReplace) {
        replaceValueHandle = JSTaggedValue::ToString(thread, inputReplaceValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSHandle<JSTaggedValue> lastIndex = globalConst->GetHandledLastIndexString();
    // 8. Let global be ToBoolean(Get(rx, "global")).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> global = globalConst->GetHandledGlobalString();
    JSTaggedValue globalValue =
        FastRuntimeStub::FastGetProperty(thread, thisObj.GetTaggedValue(), global.GetTaggedValue());
    // 9. ReturnIfAbrupt(global).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isGlobal = globalValue.ToBoolean();

    // 10. If global is true, then
    bool fullUnicode = false;
    if (isGlobal) {
        // a. Let fullUnicode be ToBoolean(Get(rx, "unicode")).
        JSHandle<JSTaggedValue> unicode = globalConst->GetHandledUnicodeString();
        JSTaggedValue fullUnicodeTag =
            FastRuntimeStub::FastGetProperty(thread, thisObj.GetTaggedValue(), unicode.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        fullUnicode = fullUnicodeTag.ToBoolean();
        // b. ReturnIfAbrupt(fullUnicode).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Let setStatus be Set(rx, "lastIndex", 0, true).
        FastRuntimeStub::FastSetProperty(thread, thisObj.GetTaggedValue(), lastIndex.GetTaggedValue(), JSTaggedValue(0),
                                         true);
        // d. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // Add cache for regexp replace
    bool useCache = false;
    JSMutableHandle<JSTaggedValue> pattern(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> flagsBits(thread, JSTaggedValue::Undefined());
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (isGlobal && !functionalReplace && thisObj->IsJSRegExp()) {
        JSHClass *hclass = JSHandle<JSObject>::Cast(thisObj)->GetJSHClass();
        JSHClass *originHClass = JSHClass::Cast(globalConst->GetJSRegExpClass().GetTaggedObject());
        if (hclass == originHClass) {
            if (replaceValueHandle->GetLength() == 0) {
                return RegExpReplaceFast(thread, thisObj, srcString, length);
            } else {
                JSHandle<JSRegExp> regexpHandle(thisObj);
                if (regexpHandle->IsJSRegExp()) {
                    pattern.Update(regexpHandle->GetOriginalSource());
                    flagsBits.Update(regexpHandle->GetOriginalFlags());
                }
                uint32_t strLength = replaceValueHandle->GetLength();
                uint32_t largeStrCount = cacheTable->GetLargeStrCount();
                if (largeStrCount != 0) {
                    if (strLength > MIN_REPLACE_STRING_LENGTH) {
                        cacheTable->SetLargeStrCount(thread, --largeStrCount);
                    }
                } else {
                    cacheTable->SetStrLenThreshold(thread, MIN_REPLACE_STRING_LENGTH);
                }
                if (strLength > cacheTable->GetStrLenThreshold()) {
                    useCache = true;
                    JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flagsBits, string,
                                                                             RegExpExecResultCache::REPLACE_TYPE,
                                                                             thisObj,
                                                                             inputReplaceValue.GetTaggedValue());
                    if (cacheResult != JSTaggedValue::Undefined()) {
                        return cacheResult;
                    }
                }
            }
        }
    }

    JSHandle<JSTaggedValue> matchedStr = globalConst->GetHandledZeroString();
    // 11. Let results be a new empty List.
    JSHandle<JSObject> resultsList(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    int resultsIndex = 0;
    // 12. Let done be false.
    // 13. Repeat, while done is false
    JSMutableHandle<JSTaggedValue> nextIndexHandle(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> execResult(thread, JSTaggedValue(0));
    for (;;) {
        // a. Let result be RegExpExec(rx, S).
        execResult.Update(RegExpExec(thread, thisObj, inputStr, useCache));
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
        JSHandle<JSTaggedValue> getMatch(
            thread, FastRuntimeStub::FastGetProperty(thread, execResult.GetTaggedValue(), matchedStr.GetTaggedValue()));
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatch);
        // 2. ReturnIfAbrupt(matchStr).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 3. If matchStr is the empty String, then
        if (matchString->GetLength() == 0) {
            // a. Let thisIndex be ToLength(Get(rx, "lastIndex")).
            JSHandle<JSTaggedValue> thisIndexHandle(
                thread, FastRuntimeStub::FastGetProperty(thread, thisObj.GetTaggedValue(), lastIndex.GetTaggedValue()));
            uint32_t thisIndex = 0;
            if (thisIndexHandle->IsInt()) {
                thisIndex = static_cast<uint32_t>(thisIndexHandle->GetInt());
            } else {
                thisIndex = JSTaggedValue::ToLength(thread, thisIndexHandle).GetNumber();
                // b. ReturnIfAbrupt(thisIndex).
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            // c. Let nextIndex be AdvanceStringIndex(S, thisIndex, fullUnicode).
            uint32_t nextIndex = AdvanceStringIndex(inputStr, thisIndex, fullUnicode);
            nextIndexHandle.Update(JSTaggedValue(nextIndex));
            // d. Let setStatus be Set(rx, "lastIndex", nextIndex, true).
            FastRuntimeStub::FastSetProperty(thread, thisObj.GetTaggedValue(), lastIndex.GetTaggedValue(),
                                             nextIndexHandle.GetTaggedValue(), true);
            // e. ReturnIfAbrupt(setStatus).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }
    // 14. Let accumulatedResult be the empty String value.
    std::string accumulatedResult;
    // 15. Let nextSourcePosition be 0.
    uint32_t nextSourcePosition = 0;
    JSHandle<JSTaggedValue> getMatchString;
    JSMutableHandle<JSTaggedValue> resultValues(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> ncapturesHandle(thread, JSTaggedValue(0));
    JSMutableHandle<JSTaggedValue> capN(thread, JSTaggedValue(0));
    // 16. Repeat, for each result in results,
    for (int i = 0; i < resultsIndex; i++) {
        resultValues.Update(FastRuntimeStub::FastGetPropertyByIndex(thread, resultsList.GetTaggedValue(), i));
        // a. Let nCaptures be ToLength(Get(result, "length")).
        JSHandle<JSTaggedValue> lengthHandle = globalConst->GetHandledLengthString();
        ncapturesHandle.Update(
            FastRuntimeStub::FastGetProperty(thread, resultValues.GetTaggedValue(), lengthHandle.GetTaggedValue()));
        uint32_t ncaptures = JSTaggedValue::ToUint32(thread, ncapturesHandle);
        // b. ReturnIfAbrupt(nCaptures).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. Let nCaptures be max(nCaptures − 1, 0).
        ncaptures = std::max<uint32_t>((ncaptures - 1), 0);
        // d. Let matched be ToString(Get(result, "0")).
        JSTaggedValue value = FastRuntimeStub::GetPropertyByIndex(thread, resultValues.GetTaggedValue(), 0);
        getMatchString = JSHandle<JSTaggedValue>(thread, value);
        JSHandle<EcmaString> matchString = JSTaggedValue::ToString(thread, getMatchString);
        // e. ReturnIfAbrupt(matched).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // f. Let matchLength be the number of code units in matched.
        uint32_t matchLength = matchString->GetLength();
        // g. Let position be ToInteger(Get(result, "index")).
        JSHandle<JSTaggedValue> resultIndex = globalConst->GetHandledIndexString();
        JSTaggedValue positionTag =
            FastRuntimeStub::FastGetPropertyByName(thread, resultValues.GetTaggedValue(), resultIndex.GetTaggedValue());
        JSHandle<JSTaggedValue> positionHandle(thread, positionTag);
        uint32_t position = 0;
        if (positionHandle->IsInt()) {
            position = static_cast<uint32_t>(positionHandle->GetInt());
        } else {
            position = JSTaggedValue::ToUint32(thread, positionHandle);
            // h. ReturnIfAbrupt(position).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        // i. Let position be max(min(position, lengthS), 0).
        position = std::max<uint32_t>(std::min<uint32_t>(position, length), 0);
        // j. Let n be 1.
        uint32_t index = 1;
        // k. Let captures be an empty List.
        JSHandle<TaggedArray> capturesList = factory->NewTaggedArray(ncaptures);
        // l. Repeat while n ≤ nCaptures
        while (index <= ncaptures) {
            // i. Let capN be Get(result, ToString(n)).
            capN.Update(FastRuntimeStub::FastGetPropertyByIndex(thread, resultValues.GetTaggedValue(), index));
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
            const size_t argsLength = replacerArgs->GetLength();
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, inputReplaceValue, undefined, undefined, argsLength);
            info.SetCallArg(argsLength, replacerArgs);
            JSTaggedValue replaceResult = JSFunction::Call(&info);
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
        JSHandle<EcmaString> resultValue = factory->NewFromStdString(accumulatedResult);
        if (useCache) {
            RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, string,
                                                    JSHandle<JSTaggedValue>(resultValue),
                                                    RegExpExecResultCache::REPLACE_TYPE, nextIndexHandle->GetInt(),
                                                    inputReplaceValue.GetTaggedValue());
        }
        return resultValue.GetTaggedValue();
    }
    // 18. Return the String formed by concatenating the code units of accumulatedResult with the substring of S
    // consisting of the code units from nextSourcePosition (inclusive) up through the final code unit of S(inclusive).
    accumulatedResult += base::StringHelper::SubString(thread, JSHandle<EcmaString>::Cast(inputStr), nextSourcePosition,
                                                       (length - nextSourcePosition));
    JSHandle<EcmaString> resultValue = factory->NewFromStdString(accumulatedResult);
    if (useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, string,
                                                JSHandle<JSTaggedValue>(resultValue),
                                                RegExpExecResultCache::REPLACE_TYPE, nextIndexHandle->GetInt(),
                                                inputReplaceValue.GetTaggedValue());
    }
    return resultValue.GetTaggedValue();
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
    JSHandle<JSTaggedValue> result(thread, RegExpExec(thread, thisObj, string, false));
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
    JSHandle<JSTaggedValue> index(factory->NewFromASCII("index"));
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
    bool useCache = false;
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
    JSHandle<JSTaggedValue> flagsString(factory->NewFromASCII("flags"));
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
    JSHandle<EcmaString> uStringHandle(factory->NewFromASCII("u"));
    bool unicodeMatching = base::StringHelper::Contains(*flags, *uStringHandle);
    // 11. If flags contains "y", let newFlags be flags.
    JSHandle<EcmaString> newFlagsHandle;
    JSHandle<EcmaString> yStringHandle(factory->NewFromASCII("y"));
    if (base::StringHelper::Contains(*flags, *yStringHandle)) {
        newFlagsHandle = flags;
    } else {
        // 12. Else, let newFlags be the string that is the concatenation of flags and "y".
        JSHandle<EcmaString> yStr = factory->NewFromASCII("y");
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

    if (lim == MAX_SPLIT_LIMIT) {
        useCache = true;
    }

    JSHandle<JSRegExp> regexpHandle(thisObj);
    JSMutableHandle<JSTaggedValue> pattern(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> flagsBits(thread, JSTaggedValue::Undefined());
    if (thisObj->IsJSRegExp()) {
        pattern.Update(regexpHandle->GetOriginalSource());
        flagsBits.Update(regexpHandle->GetOriginalFlags());
    }
    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (useCache) {
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flagsBits, inputString,
                                                                 RegExpExecResultCache::SPLIT_TYPE, thisObj);
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    // 13. Let splitter be Construct(C, «rx, newFlags»).
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 2); // 2: two args
    runtimeInfo.SetCallArg(thisObj.GetTaggedValue(), newFlagsHandle.GetTaggedValue());
    JSTaggedValue taggedSplitter = JSFunction::Construct(&runtimeInfo);
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
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString, useCache));
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
        JSHandle<JSTaggedValue> execResult(thread, RegExpExec(thread, splitter, jsString, useCache));
        // d. ReturnIfAbrupt(z).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // e. If z is null, let q be AdvanceStringIndex(S, q, unicodeMatching).
        if (execResult->IsNull()) {
            endIndex = AdvanceStringIndex(jsString, endIndex, unicodeMatching);
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
                endIndex = AdvanceStringIndex(jsString, endIndex, unicodeMatching);
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
                    if (useCache) {
                        RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, inputString,
                                                                JSHandle<JSTaggedValue>(array),
                                                                RegExpExecResultCache::SPLIT_TYPE, lastIndex);
                    }
                    return array.GetTaggedValue();
                }
                // 6. Let p be e.
                startIndex = lastIndex;
                // 7. Let numberOfCaptures be ToLength(Get(z, "length")).
                JSHandle<JSTaggedValue> lengthString(factory->NewFromASCII("length"));
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
                        if (useCache) {
                            RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, inputString,
                                                                    JSHandle<JSTaggedValue>(array),
                                                                    RegExpExecResultCache::SPLIT_TYPE, lastIndex);
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
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flagsBits, inputString,
                                                JSHandle<JSTaggedValue>(array), RegExpExecResultCache::SPLIT_TYPE,
                                                endIndex);
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
    Chunk chunk(thread->GetNativeAreaAllocator());
    RegExpExecutor executor(&chunk);
    if (lastIndex < 0) {
        lastIndex = 0;
    }
    bool ret = executor.Execute(buffer, lastIndex, length, bytecodeBuffer, isUtf16);
    RegExpExecutor::MatchResult result = executor.GetResult(thread, ret);
    return result;
}

uint32_t BuiltinsRegExp::AdvanceStringIndex(const JSHandle<JSTaggedValue> &inputStr, uint32_t index,
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

bool BuiltinsRegExp::GetFlagsInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const uint8_t mask)
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
    uint8_t flags = static_cast<uint8_t>(regexpObj->GetOriginalFlags().GetInt());
    return flags & mask;
}

// 21.2.5.2.2
JSTaggedValue BuiltinsRegExp::RegExpBuiltinExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                                const JSHandle<JSTaggedValue> &inputStr, bool useCache)
{
    ASSERT(JSObject::IsRegExp(thread, regexp));
    ASSERT(inputStr->IsString());

    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> lastIndexHandle = globalConst->GetHandledLastIndexString();
    JSTaggedValue result =
        FastRuntimeStub::FastGetProperty(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue());
    int32_t lastIndex = 0;
    if (result.IsInt()) {
        lastIndex = result.GetInt();
    } else {
        JSHandle<JSTaggedValue> lastIndexResult(thread, result);
        JSTaggedNumber lastIndexNumber = JSTaggedValue::ToLength(thread, lastIndexResult);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        lastIndex = lastIndexNumber.GetNumber();
    }

    JSHandle<JSTaggedValue> globalHandle = globalConst->GetHandledGlobalString();
    bool global =
        FastRuntimeStub::FastGetProperty(thread, regexp.GetTaggedValue(), globalHandle.GetTaggedValue()).ToBoolean();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> stickyHandle = globalConst->GetHandledStickyString();
    bool sticky =
        FastRuntimeStub::FastGetProperty(thread, regexp.GetTaggedValue(), stickyHandle.GetTaggedValue()).ToBoolean();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!global && !sticky) {
        lastIndex = 0;
    }

    JSHandle<JSRegExp> regexpObj(regexp);
    JSMutableHandle<JSTaggedValue> pattern(thread, regexpObj->GetOriginalSource());
    JSMutableHandle<JSTaggedValue> flags(thread, regexpObj->GetOriginalFlags());

    JSHandle<RegExpExecResultCache> cacheTable(thread->GetEcmaVM()->GetRegExpCache());
    if (lastIndex == 0 && useCache) {
        JSTaggedValue cacheResult =
            cacheTable->FindCachedResult(thread, pattern, flags, inputStr, RegExpExecResultCache::EXEC_TYPE, regexp);
        if (cacheResult != JSTaggedValue::Undefined()) {
            return cacheResult;
        }
    }

    int32_t length = static_cast<EcmaString *>(inputStr->GetTaggedObject())->GetLength();
    uint8_t flagsBits = static_cast<uint8_t>(regexpObj->GetOriginalFlags().GetInt());
    JSHandle<JSTaggedValue> flagsValue(thread, FlagsBitsToString(thread, flagsBits));
    JSHandle<EcmaString> flagsStr = JSTaggedValue::ToString(thread, flagsValue);
    JSHandle<EcmaString> uString(globalConst->GetHandledUString());
    [[maybe_unused]] bool fullUnicode = base::StringHelper::Contains(*flagsStr, *uString);
    if (lastIndex > static_cast<int32_t>(length)) {
        FastRuntimeStub::FastSetPropertyByValue(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                                JSTaggedValue(0));
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
            FastRuntimeStub::FastSetPropertyByValue(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                                    JSTaggedValue(0));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        return JSTaggedValue::Null();
    }
    uint32_t endIndex = matchResult.endIndex_;
    if (global || sticky) {
        // a. Let setStatus be Set(R, "lastIndex", e, true).
        FastRuntimeStub::FastSetPropertyByValue(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                                JSTaggedValue(endIndex));
        // b. ReturnIfAbrupt(setStatus).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    uint32_t capturesSize = matchResult.captures_.size();
    JSHandle<JSObject> results(JSArray::ArrayCreate(thread, JSTaggedNumber(capturesSize)));
    uint32_t matchIndex = matchResult.index_;
    // 24. Perform CreateDataProperty(A, "index", matchIndex).
    JSHandle<JSTaggedValue> indexKey = globalConst->GetHandledIndexString();
    JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(matchIndex));
    JSObject::CreateDataProperty(thread, results, indexKey, indexValue);
    // 25. Perform CreateDataProperty(A, "input", S).
    JSHandle<JSTaggedValue> inputKey = globalConst->GetHandledInputString();

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
    if (lastIndex == 0 && useCache) {
        RegExpExecResultCache::AddResultInCache(thread, cacheTable, pattern, flags, inputStr,
                                                JSHandle<JSTaggedValue>(results), RegExpExecResultCache::EXEC_TYPE,
                                                endIndex);
    }
    // 29. Return A.
    return results.GetTaggedValue();
}

// 21.2.5.2.1
JSTaggedValue BuiltinsRegExp::RegExpExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                         const JSHandle<JSTaggedValue> &inputString, bool useCache)
{
    // 1. Assert: Type(R) is Object.
    ASSERT(regexp->IsECMAObject());
    // 2. Assert: Type(S) is String.
    ASSERT(inputString->IsString());
    // 3. Let exec be Get(R, "exec").
    JSHandle<EcmaString> inputStr = JSTaggedValue::ToString(thread, inputString);

    JSHandle<JSTaggedValue> execHandle(thread->GlobalConstants()->GetHandledExecString());
    JSHandle<JSTaggedValue> exec(
        thread, FastRuntimeStub::FastGetProperty(thread, regexp.GetTaggedValue(), execHandle.GetTaggedValue()));
    // 4. ReturnIfAbrupt(exec).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If IsCallable(exec) is true, then
    if (exec->IsCallable()) {
        JSHClass *hclass = JSHandle<JSObject>::Cast(regexp)->GetJSHClass();
        JSHClass *originHClass = JSHClass::Cast(thread->GlobalConstants()->GetJSRegExpClass().GetTaggedObject());
        if (hclass == originHClass) {
            // 7. Return RegExpBuiltinExec(R, S).
            return RegExpBuiltinExec(thread, regexp, inputString, useCache);
        }
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, exec, regexp, undefined, 1);
        info.SetCallArg(inputStr.GetTaggedValue());
        JSTaggedValue result = JSFunction::Call(&info);
        // b. ReturnIfAbrupt(result).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!result.IsECMAObject() && !result.IsNull()) {
            // throw a TypeError exception.
            THROW_TYPE_ERROR_AND_RETURN(thread, "exec result is null or is not Object", JSTaggedValue::Exception());
        }
        return result;
    }
    // 6. If R does not have a [[RegExpMatcher]] internal slot, throw a TypeError exception.
    if (!regexp->IsJSRegExp()) {
        // throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "this does not have a [[RegExpMatcher]]", JSTaggedValue::Exception());
    }
    // 7. Return RegExpBuiltinExec(R, S).
    return RegExpBuiltinExec(thread, regexp, inputString, useCache);
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

JSTaggedValue BuiltinsRegExp::FlagsBitsToString(JSThread *thread, uint8_t flags)
{
    ASSERT((flags & 0xC0) == 0);  // 0xC0: first 2 bits of flags must be 0

    uint8_t *flagsStr = new uint8_t[7];  // 7: maximum 6 flags + '\0'
    size_t flagsLen = 0;
    if (flags & RegExpParser::FLAG_GLOBAL) {
        flagsStr[flagsLen] = 'g';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_IGNORECASE) {
        flagsStr[flagsLen] = 'i';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_MULTILINE) {
        flagsStr[flagsLen] = 'm';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_DOTALL) {
        flagsStr[flagsLen] = 's';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_UTF16) {
        flagsStr[flagsLen] = 'u';
        flagsLen++;
    }
    if (flags & RegExpParser::FLAG_STICKY) {
        flagsStr[flagsLen] = 'y';
        flagsLen++;
    }
    flagsStr[flagsLen] = '\0';
    JSHandle<EcmaString> flagsString = thread->GetEcmaVM()->GetFactory()->NewFromUtf8(flagsStr, flagsLen);
    delete[] flagsStr;

    return flagsString.GetTaggedValue();
}

// 21.2.3.2.2
JSTaggedValue BuiltinsRegExp::RegExpInitialize(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &pattern,
                                               const JSHandle<JSTaggedValue> &flags)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> patternStrHandle;
    uint8_t flagsBits = 0;
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
        flagsBits = 0;
    } else if (flags->IsInt()) {
        flagsBits = static_cast<uint8_t>(flags->GetInt());
    } else {
        // 5. Else, let F be ToString(flags).
        JSHandle<EcmaString> flagsStrHandle = JSTaggedValue::ToString(thread, flags);
        // 6. ReturnIfAbrupt(F).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        /**
         * 7. If F contains any code unit other than "g", "i", "m", "u", or "y" or if it contains the same code
         * unit more than once, throw a SyntaxError exception.
         **/
        CString checkStr = ConvertToString(*flagsStrHandle, StringConvertedUsage::LOGICOPERATION);
        flagsBits = static_cast<uint8_t>(UpdateExpressionFlags(thread, checkStr));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // String -> CString
    CString patternStdStr = ConvertToString(*patternStrHandle, StringConvertedUsage::LOGICOPERATION);
    // 9. 10.
    Chunk chunk(thread->GetNativeAreaAllocator());
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
    regexp->SetOriginalFlags(thread, JSTaggedValue(flagsBits));
    // 13. Set obj’s [[RegExpMatcher]] internal slot.
    if (getCache.first == JSTaggedValue::Hole()) {
        auto bufferSize = parser.GetOriginBufferSize();
        auto buffer = parser.GetOriginBuffer();
        factory->NewJSRegExpByteCodeData(regexp, buffer, bufferSize);
        regExpParserCache->SetCache(*patternStrHandle, flagsBits, regexp->GetByteCodeBuffer(), bufferSize);
    } else {
        regexp->SetByteCodeBuffer(thread, getCache.first);
        regexp->SetLength(static_cast<uint32_t>(getCache.second));
    }
    // 14. Let setStatus be Set(obj, "lastIndex", 0, true).
    JSHandle<JSTaggedValue> lastIndexString = thread->GlobalConstants()->GetHandledLastIndexString();
    FastRuntimeStub::FastSetProperty(thread, obj.GetTaggedValue(), lastIndexString.GetTaggedValue(), JSTaggedValue(0),
                                     true);
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
                                                const JSHandle<JSTaggedValue> &flags)
{
    // String -> CString
    JSHandle<EcmaString> srcStr(thread, static_cast<EcmaString *>(src->GetTaggedObject()));
    JSHandle<EcmaString> flagsStr(thread, static_cast<EcmaString *>(flags->GetTaggedObject()));
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

    return *factory->NewFromUtf8(srcStdStr);
}

JSTaggedValue RegExpExecResultCache::CreateCacheTable(JSThread *thread)
{
    int length = CACHE_TABLE_HEADER_SIZE + INITIAL_CACHE_NUMBER * ENTRY_SIZE;

    auto table = static_cast<RegExpExecResultCache *>(
        *thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, JSTaggedValue::Undefined()));
    table->SetLargeStrCount(thread, DEFAULT_LARGE_STRING_COUNT);
    table->SetConflictCount(thread, DEFAULT_CONFLICT_COUNT);
    table->SetStrLenThreshold(thread, 0);
    table->SetHitCount(thread, 0);
    table->SetCacheCount(thread, 0);
    table->SetCacheLength(thread, INITIAL_CACHE_NUMBER);
    return JSTaggedValue(table);
}

JSTaggedValue RegExpExecResultCache::FindCachedResult(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                                      const JSHandle<JSTaggedValue> &flags,
                                                      const JSHandle<JSTaggedValue> &input, CacheType type,
                                                      const JSHandle<JSTaggedValue> &regexp, JSTaggedValue extend)
{
    JSTaggedValue patternValue = pattern.GetTaggedValue();
    JSTaggedValue flagsValue = flags.GetTaggedValue();
    JSTaggedValue inputValue = input.GetTaggedValue();

    if (!pattern->IsString() || !flags->IsInt() || !input->IsString()) {
        return JSTaggedValue::Undefined();
    }

    uint32_t hash = pattern->GetKeyHashCode() + flags->GetInt() + input->GetKeyHashCode();
    uint32_t entry = hash & (GetCacheLength() - 1);
    if (!Match(entry, patternValue, flagsValue, inputValue, extend)) {
        uint32_t entry2 = (entry + 1) & (GetCacheLength() - 1);
        if (!Match(entry2, patternValue, flagsValue, inputValue, extend)) {
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
        case MATCH_TYPE:
            result = Get(index + RESULT_MATCH_INDEX);
            break;
        case EXEC_TYPE:
            result = Get(index + RESULT_EXEC_INDEX);
            break;
        default:
            UNREACHABLE();
            break;
    }
    SetHitCount(thread, GetHitCount() + 1);
    JSHandle<JSTaggedValue> lastIndexHandle = thread->GlobalConstants()->GetHandledLastIndexString();
    FastRuntimeStub::FastSetPropertyByValue(thread, regexp.GetTaggedValue(), lastIndexHandle.GetTaggedValue(),
                                            Get(index + LAST_INDEX_INDEX));
    return result;
}

void RegExpExecResultCache::AddResultInCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache,
                                             const JSHandle<JSTaggedValue> &pattern,
                                             const JSHandle<JSTaggedValue> &flags, const JSHandle<JSTaggedValue> &input,
                                             const JSHandle<JSTaggedValue> &resultArray, CacheType type,
                                             uint32_t lastIndex, JSTaggedValue extend)
{
    if (!pattern->IsString() || !flags->IsInt() || !input->IsString()) {
        return;
    }

    JSTaggedValue patternValue = pattern.GetTaggedValue();
    JSTaggedValue flagsValue = flags.GetTaggedValue();
    JSTaggedValue inputValue = input.GetTaggedValue();
    JSTaggedValue lastIndexValue(lastIndex);

    uint32_t hash = patternValue.GetKeyHashCode() + flagsValue.GetInt() + inputValue.GetKeyHashCode();
    uint32_t entry = hash & (cache->GetCacheLength() - 1);
    uint32_t index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    if (cache->Get(index) == JSTaggedValue::Undefined()) {
        cache->SetCacheCount(thread, cache->GetCacheCount() + 1);
        cache->SetEntry(thread, entry, patternValue, flagsValue, inputValue, lastIndexValue, extend);
        cache->UpdateResultArray(thread, entry, resultArray.GetTaggedValue(), type);
    } else if (cache->Match(entry, patternValue, flagsValue, inputValue, extend)) {
        cache->UpdateResultArray(thread, entry, resultArray.GetTaggedValue(), type);
    } else {
        uint32_t entry2 = (entry + 1) & static_cast<uint32_t>(cache->GetCacheLength() - 1);
        uint32_t index2 = CACHE_TABLE_HEADER_SIZE + entry2 * ENTRY_SIZE;
        if (cache->GetCacheLength() < DEFAULT_CACHE_NUMBER) {
            GrowRegexpCache(thread, cache);
            // update value after gc.
            patternValue = pattern.GetTaggedValue();
            flagsValue = flags.GetTaggedValue();
            inputValue = input.GetTaggedValue();

            cache->SetCacheLength(thread, DEFAULT_CACHE_NUMBER);
            entry2 = hash & static_cast<uint32_t>(cache->GetCacheLength() - 1);
            index2 = CACHE_TABLE_HEADER_SIZE + entry2 * ENTRY_SIZE;
        }
        if (cache->Get(index2) == JSTaggedValue::Undefined()) {
            cache->SetCacheCount(thread, cache->GetCacheCount() + 1);
            cache->SetEntry(thread, entry2, patternValue, flagsValue, inputValue, lastIndexValue, extend);
            cache->UpdateResultArray(thread, entry2, resultArray.GetTaggedValue(), type);
        } else if (cache->Match(entry2, patternValue, flagsValue, inputValue, extend)) {
            cache->UpdateResultArray(thread, entry2, resultArray.GetTaggedValue(), type);
        } else {
            cache->SetConflictCount(thread, cache->GetConflictCount() > 1 ? (cache->GetConflictCount() - 1) : 0);
            cache->SetCacheCount(thread, cache->GetCacheCount() - 1);
            cache->ClearEntry(thread, entry2);
            cache->SetEntry(thread, entry, patternValue, flagsValue, inputValue, lastIndexValue, extend);
            cache->UpdateResultArray(thread, entry, resultArray.GetTaggedValue(), type);
        }
    }
}

void RegExpExecResultCache::GrowRegexpCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache)
{
    int length = CACHE_TABLE_HEADER_SIZE + DEFAULT_CACHE_NUMBER * ENTRY_SIZE;
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto newCache = factory->ExtendArray(JSHandle<TaggedArray>(cache), length, JSTaggedValue::Undefined());
    thread->GetEcmaVM()->SetRegExpCache(newCache.GetTaggedValue());
}

void RegExpExecResultCache::SetEntry(JSThread *thread, int entry, JSTaggedValue &pattern, JSTaggedValue &flags,
                                     JSTaggedValue &input, JSTaggedValue &lastIndexValue, JSTaggedValue &extendValue)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    Set(thread, index + PATTERN_INDEX, pattern);
    Set(thread, index + FLAG_INDEX, flags);
    Set(thread, index + INPUT_STRING_INDEX, input);
    Set(thread, index + LAST_INDEX_INDEX, lastIndexValue);
    Set(thread, index + EXTEND_INDEX, extendValue);
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
        case MATCH_TYPE:
            Set(thread, index + RESULT_MATCH_INDEX, resultArray);
            break;
        case EXEC_TYPE:
            Set(thread, index + RESULT_EXEC_INDEX, resultArray);
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
bool RegExpExecResultCache::Match(int entry, JSTaggedValue &pattern, JSTaggedValue &flags, JSTaggedValue &input,
                                  JSTaggedValue &extend)
{
    int index = CACHE_TABLE_HEADER_SIZE + entry * ENTRY_SIZE;
    JSTaggedValue keyPattern = Get(index + PATTERN_INDEX);
    JSTaggedValue keyFlags = Get(index + FLAG_INDEX);
    JSTaggedValue keyInput = Get(index + INPUT_STRING_INDEX);
    JSTaggedValue keyExtend = Get(index + EXTEND_INDEX);

    if (keyPattern == JSTaggedValue::Undefined()) {
        return false;
    }

    EcmaString *patternStr = EcmaString::Cast(pattern.GetTaggedObject());
    uint8_t flagsBits = static_cast<uint8_t>(flags.GetInt());
    EcmaString *inputStr = EcmaString::Cast(input.GetTaggedObject());
    EcmaString *keyPatternStr = EcmaString::Cast(keyPattern.GetTaggedObject());
    uint8_t keyFlagsBits = static_cast<uint8_t>(keyFlags.GetInt());
    EcmaString *keyInputStr = EcmaString::Cast(keyInput.GetTaggedObject());
    bool extendEqual = false;
    if (extend.IsString() && keyExtend.IsString()) {
        EcmaString *extendStr = EcmaString::Cast(extend.GetTaggedObject());
        EcmaString *keyExtendStr = EcmaString::Cast(keyExtend.GetTaggedObject());
        extendEqual = EcmaString::StringsAreEqual(extendStr, keyExtendStr);
    } else if (extend.IsUndefined() && keyExtend.IsUndefined()) {
        extendEqual = true;
    } else {
        return false;
    }
    return EcmaString::StringsAreEqual(patternStr, keyPatternStr) && flagsBits == keyFlagsBits &&
           EcmaString::StringsAreEqual(inputStr, keyInputStr) && extendEqual;
}
}  // namespace panda::ecmascript::builtins
