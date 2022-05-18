/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_regexp_iterator.h"

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"


namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using BuiltinsRegExp = builtins::BuiltinsRegExp;
JSTaggedValue JSRegExpIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    // 2. If Type(O) is not Object, throw a TypeError exception.
    // 3. If O does not have all of the internal slots of a RegExp String Iterator Object Instance
    // (see 21.2.7.2), throw a TypeError exception.
    if (!input->IsJSRegExpIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a regExp iterator", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> undefinedHandle(thread->GlobalConstants()->GetHandledUndefined());
    JSHandle<JSRegExpIterator> jsIterator = JSHandle<JSRegExpIterator>::Cast(input);
    // 4. If O.[[Done]] is true, then
    //     a. Return ! CreateIterResultObject(undefined, true).
    if (jsIterator->GetDone()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }

    // 5. Let R be O.[[IteratingRegExp]].
    // 6. Let S be O.[[IteratedString]].
    // 7. Let global be O.[[Global]].
    // 8. Let fullUnicode be O.[[Unicode]].
    JSHandle<JSTaggedValue> regexHandle(thread, jsIterator->GetIteratingRegExp());
    JSHandle<JSTaggedValue> inputStr(thread, jsIterator->GetIteratedString());
    bool global = jsIterator->GetGlobal();
    bool fullUnicode = jsIterator->GetUnicode();

    // 9. Let match be ? RegExpExec(R, S).
    JSTaggedValue match = BuiltinsRegExp::RegExpExec(thread, regexHandle, inputStr, false);
    JSHandle<JSTaggedValue> matchHandle(thread, match);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 10. If match is null, then
    //     a. Set O.[[Done]] to true.
    //     b. Return ! CreateIterResultObject(undefined, true).
    // Else,
    if (matchHandle->IsNull()) {
        jsIterator->SetDone(true);
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    } else {
        // a. If global is true, then
        //    i. Let matchStr be ? ToString(? Get(match, "0")).
        if (global) {
            const GlobalEnvConstants *globalConstants = thread->GlobalConstants();
            JSHandle<JSTaggedValue> zeroString(globalConstants->GetHandledZeroString());
            JSHandle<JSTaggedValue> getZero(JSObject::GetProperty(thread, matchHandle, zeroString).GetValue());
            JSHandle<EcmaString> matchStr = JSTaggedValue::ToString(thread, getZero);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // ii. If matchStr is the empty String, then
            //     1. Let thisIndex be ? ToLength(? Get(R, "lastIndex")).
            //     2. Let nextIndex be ! AdvanceStringIndex(S, thisIndex, fullUnicode).
            //     3. Perform ? Set(R, "lastIndex", 𝔽(nextIndex), true).
            if (matchStr->GetLength() == 0) {
                JSHandle<JSTaggedValue> lastIndexString(globalConstants->GetHandledLastIndexString());
                JSHandle<JSTaggedValue> getLastIndex(JSObject::GetProperty(thread, regexHandle,
                                                                           lastIndexString).GetValue());
                JSTaggedNumber thisIndex = JSTaggedValue::ToLength(thread, getLastIndex);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                uint32_t nextIndex = BuiltinsRegExp::AdvanceStringIndex(inputStr, thisIndex.ToUint32(),
                                                                        fullUnicode);
                FastRuntimeStub::FastSetProperty(thread, regexHandle.GetTaggedValue(),
                                                 lastIndexString.GetTaggedValue(),
                                                 JSTaggedValue(nextIndex), true);
            }
            // iii. Return ! CreateIterResultObject(match, false).
            return JSIterator::CreateIterResultObject(thread, matchHandle, false).GetTaggedValue();
        } else {
            // b. Else,
            //    i. Set O.[[Done]] to true.
            //   ii. Return ! CreateIterResultObject(match, false).
            jsIterator->SetDone(true);
            return JSIterator::CreateIterResultObject(thread, matchHandle, false).GetTaggedValue();
        }
    }
}

JSHandle<JSTaggedValue> JSRegExpIterator::CreateRegExpStringIterator(JSThread *thread,
                                                                     const JSHandle<JSTaggedValue> &matcher,
                                                                     const JSHandle<EcmaString> &inputStr,
                                                                     bool global, bool fullUnicode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!matcher->IsJSRegExp()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread->GlobalConstants()->GetHandledUndefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "matcher is not JSRegExp", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSRegExpIterator(matcher, inputStr, global, fullUnicode));
    return iter;
}
}  // namespace panda::ecmascript
