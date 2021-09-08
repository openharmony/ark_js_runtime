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

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tagged_array.h"
#include "js_object-inl.h"
#include "object_factory.h"

namespace panda::ecmascript {
JSHandle<EcmaString> GetTypeString(JSThread *thread, PreferredPrimitiveType type)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (type == NO_PREFERENCE) {
        return factory->NewFromCanBeCompressString("default");
    }
    if (type == PREFER_NUMBER) {
        return factory->NewFromCanBeCompressString("number");
    }
    return factory->NewFromCanBeCompressString("string");
}

JSHandle<JSTaggedValue> JSTaggedValue::ToPropertyKey(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsStringOrSymbol() || tagged->IsNumber()) {
        return tagged;
    }
    JSHandle<JSTaggedValue> key(thread, ToPrimitive(thread, tagged, PREFER_STRING));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (key->IsSymbol()) {
        return key;
    }
    JSHandle<EcmaString> string = ToString(thread, key);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>::Cast(string);
}

bool JSTaggedValue::IsInteger() const
{
    if (!IsNumber()) {
        return false;
    }

    if (IsInt()) {
        return true;
    }

    double thisValue = GetDouble();
    // If argument is NaN, +∞, or -∞, return false.
    if (!std::isfinite(thisValue)) {
        return false;
    }

    // If floor(abs(argument)) ≠ abs(argument), return false.
    if (std::floor(std::abs(thisValue)) != std::abs(thisValue)) {
        return false;
    }

    return true;
}

bool JSTaggedValue::WithinInt32() const
{
    if (!IsNumber()) {
        return false;
    }

    double doubleValue = GetNumber();
    if (bit_cast<int64_t>(doubleValue) == bit_cast<int64_t>(-0.0)) {
        return false;
    }

    int32_t intvalue = base::NumberHelper::DoubleToInt(doubleValue, base::INT32_BITS);
    return doubleValue == static_cast<double>(intvalue);
}

bool JSTaggedValue::IsZero() const
{
    if (GetRawData() == VALUE_ZERO) {
        return true;
    }
    if (IsDouble()) {
        const double limit = 1e-8;
        return (std::abs(GetDouble() - 0.0) <= limit);
    }
    return false;
}

bool JSTaggedValue::Equal(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y)
{
    if (x->IsNumber()) {
        if (y->IsNumber()) {
            return StrictNumberEquals(x->ExtractNumber(), y->ExtractNumber());
        }
        if (y->IsString()) {
            JSTaggedNumber yNumber = ToNumber(thread, y);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return StrictNumberEquals(x->ExtractNumber(), yNumber.GetNumber());
        }
        if (y->IsBoolean()) {
            JSTaggedNumber yNumber = ToNumber(thread, y);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return StrictNumberEquals(x->ExtractNumber(), yNumber.GetNumber());
        }
        if (y->IsHeapObject() && !y->IsSymbol()) {
            JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return Equal(thread, x, yPrimitive);
        }
        return false;
    }

    if (x->IsString()) {
        if (y->IsString()) {
            return EcmaString::StringsAreEqual(static_cast<EcmaString *>(x->GetTaggedObject()),
                                               static_cast<EcmaString *>(y->GetTaggedObject()));
        }
        if (y->IsNumber()) {
            JSTaggedNumber xNumber = ToNumber(thread, x);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return StrictNumberEquals(xNumber.GetNumber(), y->ExtractNumber());
        }
        if (y->IsBoolean()) {
            JSTaggedNumber xNumber = ToNumber(thread, x);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            JSTaggedNumber yNumber = ToNumber(thread, y);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return StrictNumberEquals(xNumber.GetNumber(), yNumber.GetNumber());
        }
        if (y->IsHeapObject() && !y->IsSymbol()) {
            JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return Equal(thread, x, yPrimitive);
        }
        return false;
    }

    if (x->IsBoolean()) {
        JSTaggedNumber xNumber = ToNumber(thread, x);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, JSHandle<JSTaggedValue>(thread, xNumber), y);
    }

    if (x->IsSymbol()) {
        if (y->IsSymbol()) {
            return x.GetTaggedValue() == y.GetTaggedValue();
        }
        if (y->IsHeapObject()) {
            JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return Equal(thread, x, yPrimitive);
        }
        return false;
    }

    if (x->IsHeapObject()) {
        if (y->IsHeapObject()) {
            // if same type, must call Type::StrictEqual()
            JSType xType = x.GetTaggedValue().GetTaggedObject()->GetClass()->GetObjectType();
            JSType yType = y.GetTaggedValue().GetTaggedObject()->GetClass()->GetObjectType();
            if (xType == yType) {
                return StrictEqual(thread, x, y);
            }
        }
        if (y->IsNumber() || y->IsStringOrSymbol() || y->IsBoolean()) {
            JSHandle<JSTaggedValue> x_primitive(thread, ToPrimitive(thread, x));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return Equal(thread, x_primitive, y);
        }
        return false;
    }

    if (x->IsNull() && y->IsNull()) {
        return true;
    }

    if (x->IsUndefined() && y->IsUndefined()) {
        return true;
    }

    if (x->IsNull() && y->IsUndefined()) {
        return true;
    }

    if (x->IsUndefined() && y->IsNull()) {
        return true;
    }

    return false;
}

ComparisonResult JSTaggedValue::Compare(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                        const JSHandle<JSTaggedValue> &y)
{
    JSHandle<JSTaggedValue> primX(thread, ToPrimitive(thread, x));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    JSHandle<JSTaggedValue> primY(thread, ToPrimitive(thread, y));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);

    if (primX->IsString() && primY->IsString()) {
        auto xString = static_cast<EcmaString *>(primX->GetTaggedObject());
        auto yString = static_cast<EcmaString *>(primY->GetTaggedObject());
        int result = xString->Compare(yString);
        if (result < 0) {
            return ComparisonResult::LESS;
        }
        if (result == 0) {
            return ComparisonResult::EQUAL;
        }
        return ComparisonResult::GREAT;
    }

    JSTaggedNumber xNumber = ToNumber(thread, x);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    JSTaggedNumber yNumber = ToNumber(thread, y);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    return StrictNumberCompare(xNumber.GetNumber(), yNumber.GetNumber());
}

bool JSTaggedValue::IsSameTypeOrHClass(JSTaggedValue x, JSTaggedValue y)
{
    if (x.IsNumber() && y.IsNumber()) {
        return true;
    }
    if (x.IsBoolean() && y.IsBoolean()) {
        return true;
    }
    if (x.IsString() && y.IsString()) {
        return true;
    }
    if (x.IsHeapObject() && y.IsHeapObject()) {
        return x.GetTaggedObject()->GetClass() == y.GetTaggedObject()->GetClass();
    }

    return false;
}

JSTaggedValue JSTaggedValue::ToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                         PreferredPrimitiveType type)
{
    if (tagged->IsECMAObject()) {
        JSHandle<JSObject> object(tagged);
        EcmaVM *vm = thread->GetEcmaVM();
        JSHandle<JSTaggedValue> keyString = vm->GetGlobalEnv()->GetToPrimitiveSymbol();

        JSHandle<JSTaggedValue> exoticToprim = JSObject::GetProperty(thread, object, keyString).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        if (!exoticToprim->IsUndefined()) {
            JSTaggedValue value = GetTypeString(thread, type).GetTaggedValue();
            InternalCallParams *arguments = thread->GetInternalCallParams();
            arguments->MakeArgv(value);
            JSTaggedValue valueResult = JSFunction::Call(thread, exoticToprim, tagged, 1, arguments->GetArgv());
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (!valueResult.IsECMAObject()) {
                return valueResult;
            }
            THROW_TYPE_ERROR_AND_RETURN(thread, "", JSTaggedValue::Exception());
        } else {
            type = (type == NO_PREFERENCE) ? PREFER_NUMBER : type;
            return OrdinaryToPrimitive(thread, tagged, type);
        }
    }
    return tagged.GetTaggedValue();
}

JSTaggedValue JSTaggedValue::OrdinaryToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                                 PreferredPrimitiveType type)
{
    static_assert(PREFER_NUMBER == 0 && PREFER_STRING == 1);
    ASSERT(tagged->IsECMAObject());
    auto globalConst = thread->GlobalConstants();
    for (uint8_t i = 0; i < 2; i++) {  // 2: 2 means value has 2 target types, string or value.
        JSHandle<JSTaggedValue> keyString;
        if ((type ^ i) != 0) {
            keyString = globalConst->GetHandledToStringString();
        } else {
            keyString = globalConst->GetHandledValueOfString();
        }
        JSHandle<JSTaggedValue> entryfunc = JSObject::GetProperty(thread, tagged, keyString).GetValue();
        if (entryfunc->IsCallable()) {
            JSTaggedValue valueResult = JSFunction::Call(thread, entryfunc, tagged, 0, nullptr);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (!valueResult.IsECMAObject()) {
                return valueResult;
            }
        }
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a illegal value to a Primitive", JSTaggedValue::Undefined());
}

JSHandle<EcmaString> JSTaggedValue::ToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsString()) {
        return JSHandle<EcmaString>(tagged);
    }
    auto globalConst = thread->GlobalConstants();
    if (tagged->IsSpecial()) {
        switch (tagged->GetRawData()) {
            case VALUE_UNDEFINED: {
                return JSHandle<EcmaString>(globalConst->GetHandledUndefinedString());
            }
            case VALUE_NULL: {
                return JSHandle<EcmaString>(globalConst->GetHandledNullString());
            }
            case VALUE_TRUE: {
                return JSHandle<EcmaString>(globalConst->GetHandledTrueString());
            }
            case VALUE_FALSE: {
                return JSHandle<EcmaString>(globalConst->GetHandledFalseString());
            }
            case VALUE_HOLE: {
                return JSHandle<EcmaString>(globalConst->GetHandledEmptyString());
            }
            default:
                break;
        }
    }

    if (tagged->IsNumber()) {
        return base::NumberHelper::NumberToString(thread, tagged.GetTaggedValue());
    }

    auto emptyStr = globalConst->GetHandledEmptyString();
    if (tagged->IsECMAObject()) {
        JSHandle<JSTaggedValue> primValue(thread, ToPrimitive(thread, tagged, PREFER_STRING));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<EcmaString>(emptyStr));
        return ToString(thread, primValue);
    }
    // Already Include Symbol
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a illegal value to a String", JSHandle<EcmaString>(emptyStr));
}

JSTaggedValue JSTaggedValue::CanonicalNumericIndexString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("-0");
    if (tagged->IsString()) {
        if (EcmaString::StringsAreEqual(static_cast<EcmaString *>(tagged->GetTaggedObject()), *str)) {
            return JSTaggedValue(-0.0);
        }
        JSHandle<JSTaggedValue> tmp(thread, ToNumber(thread, tagged));
        if (SameValue(ToString(thread, tmp).GetTaggedValue(), tagged.GetTaggedValue())) {
            return tmp.GetTaggedValue();
        }
    }
    return JSTaggedValue::Undefined();
}

JSHandle<JSObject> JSTaggedValue::ToObject(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (tagged->IsInt() || tagged->IsDouble()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, tagged));
    }

    switch (tagged->GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a UNDEFINED value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_HOLE: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a HOLE value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_NULL: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a NULL value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_TRUE:
        case JSTaggedValue::VALUE_FALSE: {
            return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, tagged));
        }
        default: {
            break;
        }
    }

    if (tagged->IsECMAObject()) {
        return JSHandle<JSObject>::Cast(tagged);
    }
    if (tagged->IsSymbol()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, tagged));
    }
    if (tagged->IsString()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_STRING, tagged));
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Unknown object value to a JSObject",
                                JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
}

// 7.3.1 Get ( O, P )
OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a valid object",
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    if (obj->IsJSProxy()) {
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), key);
    }
    if (obj->IsTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, JSTypedArray::ToPropKey(thread, key));
    }

    return JSObject::GetProperty(thread, obj, key);
}

OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a valid object",
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), keyHandle);
    }

    if (obj->IsTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, key);
    }

    return JSObject::GetProperty(thread, obj, key);
}

OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a valid object",
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    if (obj->IsJSProxy()) {
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), key, receiver);
    }
    if (obj->IsTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, JSTypedArray::ToPropKey(thread, key), receiver);
    }

    return JSObject::GetProperty(thread, obj, key, receiver);
}

// 7.3.3 Set (O, P, V, Throw)
bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), key, value, mayThrow);
    } else if (obj->IsTypedArray()) {
        success = JSTypedArray::SetProperty(thread, obj, JSTypedArray::ToPropKey(thread, key), value, mayThrow);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, mayThrow);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key,
                                const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), keyHandle, value, mayThrow);
    } else if (obj->IsTypedArray()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        success = JSTypedArray::SetProperty(
            thread, obj, JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, keyHandle)), value, mayThrow);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, mayThrow);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                const JSHandle<JSTaggedValue> &receiver, bool mayThrow)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), key, value, receiver, mayThrow);
    } else if (obj->IsTypedArray()) {
        success =
            JSTypedArray::SetProperty(thread, obj, JSTypedArray::ToPropKey(thread, key), value, receiver, mayThrow);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, receiver, mayThrow);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key)
{
    if (obj->IsJSProxy()) {
        return JSProxy::DeleteProperty(thread, JSHandle<JSProxy>(obj), key);
    }

    return JSObject::DeleteProperty(thread, JSHandle<JSObject>(obj), key);
}

// 7.3.8 DeletePropertyOrThrow (O, P)
bool JSTaggedValue::DeletePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                          const JSHandle<JSTaggedValue> &key)
{
    if (!obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a valid object", false);
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 3. Let success be O.[[Delete]](P).
    bool success = DeleteProperty(thread, obj, key);

    // 4. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 5. If success is false, throw a TypeError exception
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot delete property", false);
    }
    return success;
}

// 7.3.7 DefinePropertyOrThrow (O, P, desc)
bool JSTaggedValue::DefinePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                          const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc)
{
    // 1. Assert: Type(O) is Object.
    // 2. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 3. Let success be ? O.[[DefineOwnProperty]](P, desc).
    bool success = DefineOwnProperty(thread, obj, key, desc);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 4. If success is false, throw a TypeError exception.
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "", false);
    }
    return success;
}

bool JSTaggedValue::DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc)
{
    if (obj->IsJSArray()) {
        return JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }

    if (obj->IsJSProxy()) {
        return JSProxy::DefineOwnProperty(thread, JSHandle<JSProxy>(obj), key, desc);
    }

    if (obj->IsTypedArray()) {
        return JSTypedArray::DefineOwnProperty(thread, obj, JSTypedArray::ToPropKey(thread, key), desc);
    }

    return JSObject::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
}

bool JSTaggedValue::GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    if (obj->IsJSProxy()) {
        return JSProxy::GetOwnProperty(thread, JSHandle<JSProxy>(obj), key, desc);
    }
    if (obj->IsTypedArray()) {
        return JSTypedArray::GetOwnProperty(thread, obj, JSTypedArray::ToPropKey(thread, key), desc);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
}

bool JSTaggedValue::SetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                 const JSHandle<JSTaggedValue> &proto)
{
    if (obj->IsJSProxy()) {
        return JSProxy::SetPrototype(thread, JSHandle<JSProxy>(obj), proto);
    }

    return JSObject::SetPrototype(thread, JSHandle<JSObject>(obj), proto);
}

bool JSTaggedValue::PreventExtensions(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsJSProxy()) {
        return JSProxy::PreventExtensions(thread, JSHandle<JSProxy>(obj));
    }
    return JSObject::PreventExtensions(thread, JSHandle<JSObject>(obj));
}

JSHandle<TaggedArray> JSTaggedValue::GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsJSProxy()) {
        return JSProxy::OwnPropertyKeys(thread, JSHandle<JSProxy>(obj));
    }
    if (obj->IsTypedArray()) {
        return JSTypedArray::OwnPropertyKeys(thread, obj);
    }
    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
}

// 7.3.10 HasProperty (O, P)
bool JSTaggedValue::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key)
{
    if (obj->IsJSProxy()) {
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>(obj), key);
    }
    if (obj->IsTypedArray()) {
        return JSTypedArray::HasProperty(thread, obj, JSTypedArray::ToPropKey(thread, key));
    }
    return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
}

bool JSTaggedValue::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key)
{
    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>(obj), keyHandle);
    }
    if (obj->IsTypedArray()) {
        JSHandle<JSTaggedValue> key_handle(thread, JSTaggedValue(key));
        return JSTypedArray::HasProperty(thread, obj, JSHandle<JSTaggedValue>(ToString(thread, key_handle)));
    }
    return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
}

// 7.3.11 HasOwnProperty (O, P)
bool JSTaggedValue::HasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread);
    return JSTaggedValue::GetOwnProperty(thread, obj, key, desc);
}

bool JSTaggedValue::GlobalHasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread);
    return JSObject::GlobalGetOwnProperty(thread, key, desc);
}

JSTaggedNumber JSTaggedValue::ToIndex(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsUndefined()) {
        return JSTaggedNumber(0);
    }
    JSTaggedNumber integerIndex = ToInteger(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
    if (integerIndex.GetNumber() < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "integerIndex < 0", JSTaggedNumber::Exception());
    }
    JSTaggedNumber index = ToLength(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
    if (!SameValue(integerIndex, index)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "integerIndex != index", JSTaggedNumber::Exception());
    }
    return index;
}

JSHandle<JSTaggedValue> JSTaggedValue::ToPrototypeOrObj(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    if (obj->IsNumber()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetNumberFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsBoolean()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetBooleanFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsString()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetStringFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsSymbol()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetSymbolFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }

    return obj;
}

JSTaggedValue JSTaggedValue::GetSuperBase(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsUndefined()) {
        return JSTaggedValue::Undefined();
    }

    ASSERT(obj->IsECMAObject());
    return JSObject::Cast(obj.GetTaggedValue())->GetPrototype(thread);
}
}  // namespace panda::ecmascript
