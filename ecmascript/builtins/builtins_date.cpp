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

#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::builtins {
// constructor
JSTaggedValue BuiltinsDate::DateConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        double now = JSDate::Now().GetDouble();
        CString str = JSDate::ToDateString(now);
        return GetTaggedString(thread, str.c_str());
    }

    JSTaggedValue timeValue(0.0);
    uint32_t length = static_cast<uint32_t>(argv->GetArgsNumber());
    if (length == 0) {  // no value
        timeValue = JSDate::Now();
    } else if (length == 1) {  // one value
        JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
        if (value->IsDate()) {  // The value is a date object.
            JSHandle<JSDate> jsDate(thread, JSDate::Cast(value->GetTaggedObject()));
            timeValue = jsDate->GetTimeValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            JSHandle<JSTaggedValue> objValue(thread, JSTaggedValue::ToPrimitive(thread, value));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (objValue->IsString()) {  // The value is a string object.
                timeValue = JSDate::Parse(argv);
            } else {  // The value is a number.
                JSTaggedNumber val = JSTaggedValue::ToNumber(thread, objValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                timeValue = JSTaggedValue(val.GetNumber());
            }
            timeValue = JSTaggedValue(JSDate::TimeClip(timeValue.GetDouble()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    } else {  // two or more values
        std::array<int64_t, DATE_LENGTH> fields = {0, 0, 1, 0, 0, 0, 0, 0, 0};
        if (length > CONSTRUCTOR_MAX_LENGTH) {  // The max length is 7.
            length = CONSTRUCTOR_MAX_LENGTH;
        }
        uint32_t i = 0;
        for (; i < length; ++i) {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, i);
            JSTaggedNumber res = JSTaggedValue::ToNumber(thread, value);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            double temp = res.GetNumber();
            if (std::isnan(temp) || !std::isfinite(temp)) {  // Check the double value is finite.
                break;
            }
            fields[i] = static_cast<int64_t>(temp);
            if (i == 0 && fields[0] >= 0 && fields[0] < JSDate::HUNDRED) {
                fields[0] += JSDate::NINETEEN_HUNDRED_YEAR;
            }
        }
        timeValue = JSTaggedValue((i == length) ? JSDate::SetDateValues(&fields, true) : base::NAN_VALUE);
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    dateObject->SetTimeValue(thread, timeValue);
    return JSTaggedValue(JSObject::Cast(static_cast<TaggedObject *>(*dateObject)));
}

// 20.4.3.1
JSTaggedValue BuiltinsDate::Now([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, Now);
    return JSDate::Now();
}

// 20.4.3.2
JSTaggedValue BuiltinsDate::Parse(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, Parse);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    return JSDate::Parse(argv);
}

// 20.4.3.4
JSTaggedValue BuiltinsDate::UTC(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, UTC);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    return JSDate::UTC(argv);
}

// 20.4.4.10
JSTaggedValue BuiltinsDate::GetTime(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, GetTime);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    return JSDate::Cast(msg->GetTaggedObject())->GetTime();
}

JSTaggedValue BuiltinsDate::SetTime(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, SetTime);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSHandle<JSDate> js_data(thread, JSDate::Cast(msg->GetTaggedObject()));
    JSTaggedNumber res = JSTaggedValue::ToNumber(thread, GetCallArg(argv, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    double number = res.GetNumber();
    double value = JSDate::TimeClip(number);
    js_data->SetTimeValue(thread, JSTaggedValue(value));
    return GetTaggedDouble(value);
}

// 20.4.4.37
JSTaggedValue BuiltinsDate::ToJSON(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ToJSON);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let tv be ToPrimitive(hint Number)
    JSHandle<JSTaggedValue> objectHandle = JSHandle<JSTaggedValue>::Cast(object);
    JSHandle<JSTaggedValue> tv(thread,
                               JSTaggedValue::ToPrimitive(thread, objectHandle, PreferredPrimitiveType::PREFER_NUMBER));

    // 3. If Type(tv) is Number and tv is not finite, return null
    if (tv->IsNumber()) {
        if (tv->IsDouble() && !std::isfinite(tv->GetDouble())) {
            return JSTaggedValue::Null();
        }
    }
    JSHandle<JSTaggedValue> calleeKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII("toISOString"));
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, objectHandle, undefined, 0);
    return JSFunction::Invoke(info, calleeKey);
}

// 20.4.4.44
JSTaggedValue BuiltinsDate::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ValueOf);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    return JSDate::Cast(msg->GetTaggedObject())->ValueOf();
}

// 20.4.4.45
JSTaggedValue BuiltinsDate::ToPrimitive(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ToPrimitive);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> object = GetThis(argv);
    if (!object->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a JSObject", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> hint = GetCallArg(argv, 0);
    PreferredPrimitiveType tryFirst = PREFER_STRING;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (hint->IsString()) {
        JSHandle<EcmaString> numberStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledNumberString());
        if (EcmaString::StringsAreEqual(hint.GetObject<EcmaString>(), *numberStrHandle)) {
            tryFirst = PREFER_NUMBER;
        } else {
            JSHandle<EcmaString> stringStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledStringString());
            JSHandle<EcmaString> defaultStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledDefaultString());
            if (EcmaString::StringsAreEqual(hint.GetObject<EcmaString>(), *stringStrHandle) ||
                EcmaString::StringsAreEqual(hint.GetObject<EcmaString>(), *defaultStrHandle)) {
                tryFirst = PREFER_STRING;
            } else {
                THROW_TYPE_ERROR_AND_RETURN(thread, "This is not a primitiveType.", JSTaggedValue::Exception());
            }
        }
    } else {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This is not an primitiveType.", JSTaggedValue::Exception());
    }
    return JSTaggedValue::OrdinaryToPrimitive(thread, object, tryFirst);
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::ANY, DefaultsOption::ALL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSTaggedValue> ctor = env->GetDateTimeFormatFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(
        thread, JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    return result.GetTaggedValue();
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleDateString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::DATE, DefaultsOption::DATE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSTaggedValue> ctor = env->GetDateTimeFormatFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(
        thread, JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    return result.GetTaggedValue();
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleTimeString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::TIME, DefaultsOption::TIME);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSTaggedValue> ctor = env->GetDateTimeFormatFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor);
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(
        thread, JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
