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

#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/symbol_table.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::builtins {
// prototype
// 19.4.3.1
// constructor
JSTaggedValue BuiltinsSymbol::SymbolConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.If NewTarget is not undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (!newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "SymbolConstructor: NewTarget is not undefined",
                                    JSTaggedValue::Exception());
    }
    // 2.If description is undefined, let descString be undefined.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (key->IsUndefined()) {
        JSHandle<JSSymbol> jsSymbol = factory->NewJSSymbol();
        return jsSymbol.GetTaggedValue();
    }
    // 3.Else, let descString be ToString(description).
    JSHandle<JSTaggedValue> descHandle = JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, key));
    // 4.ReturnIfAbrupt(descString).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    // 5.Return a new unique Symbol value whose [[Description]] value is descString.
    JSHandle<JSSymbol> jsSymbol = factory->NewPublicSymbol(descHandle);
    return jsSymbol.GetTaggedValue();
}

// 19.4.3.2 Symbol.prototype.toString()
JSTaggedValue BuiltinsSymbol::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Let s be the this value.
    JSHandle<JSTaggedValue> valueHandle = GetThis(argv);
    // 2.If Type(s) is Symbol, let sym be s.
    JSTaggedValue sym = valueHandle.GetTaggedValue();
    // 3.Else
    if (!valueHandle->IsSymbol()) {
        if (valueHandle->IsObject()) {
            if (!valueHandle->IsJSPrimitiveRef()) {
                // If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
                THROW_TYPE_ERROR_AND_RETURN(thread, "ToString: no [[SymbolData]]", JSTaggedValue::Exception());
            }
            // Let sym be the value of s's [[SymbolData]] internal slot.
            JSTaggedValue primitive = JSPrimitiveRef::Cast(valueHandle->GetTaggedObject())->GetValue();
            ASSERT(primitive.IsSymbol());
            sym = primitive;
        } else {
            // If Type(s) is not Object, throw a TypeError exception.
            THROW_TYPE_ERROR_AND_RETURN(thread, "ToString: s is not Object", JSTaggedValue::Exception());
        }
    }
    // Return SymbolDescriptiveString(sym).
    return SymbolDescriptiveString(thread, sym);
}

JSTaggedValue BuiltinsSymbol::SymbolDescriptiveString(JSThread *thread, JSTaggedValue sym)
{
    BUILTINS_API_TRACE(thread, Symbol, SymbolDescriptiveString);
    // Assert: Type(sym) is Symbol.
    ASSERT(sym.IsSymbol());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // Let desc be sym’s [[Description]] value.
    auto symbolObject = reinterpret_cast<JSSymbol *>(sym.GetTaggedObject());
    JSHandle<JSTaggedValue> descHandle(thread, symbolObject->GetDescription());

    // If desc is undefined, let desc be the empty string.

    if (descHandle->IsUndefined()) {
        JSHandle<EcmaString> leftHandle(factory->NewFromCanBeCompressString("Symbol("));
        JSHandle<EcmaString> rightHandle(factory->NewFromCanBeCompressString(")"));
        JSHandle<EcmaString> str = factory->ConcatFromString(leftHandle, rightHandle);
        return str.GetTaggedValue();
    }
    // Assert: Type(desc) is String.
    ASSERT(descHandle->IsString());
    // Return the result of concatenating the strings "Symbol(", desc, and ")".
    JSHandle<EcmaString> leftHandle(factory->NewFromCanBeCompressString("Symbol("));
    JSHandle<EcmaString> rightHandle(factory->NewFromCanBeCompressString(")"));
    JSHandle<EcmaString> stringLeft =
        factory->ConcatFromString(leftHandle, JSTaggedValue::ToString(thread, descHandle));
    JSHandle<EcmaString> str = factory->ConcatFromString(stringLeft, rightHandle);
    return str.GetTaggedValue();
}

// 19.4.3.3
JSTaggedValue BuiltinsSymbol::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, ValueOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Let s be the this value.
    JSHandle<JSTaggedValue> valueHandle = GetThis(argv);
    // If Type(s) is Symbol, return s.
    if (valueHandle->IsSymbol()) {
        return valueHandle.GetTaggedValue();
    }
    // If Type(s) is not Object, throw a TypeError exception.
    if (!valueHandle->IsObject()) {
        // return TypeError
        THROW_TYPE_ERROR_AND_RETURN(thread, "ValueOf: s is not Object", JSTaggedValue::Exception());
    }
    // If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
    if (!valueHandle->IsJSPrimitiveRef()) {
        // If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "ValueOf: no [[SymbolData]]", JSTaggedValue::Exception());
    }
    JSTaggedValue primitive = JSPrimitiveRef::Cast(valueHandle->GetTaggedObject())->GetValue();
    ASSERT(primitive.IsSymbol());
    // Return the value of s's [[SymbolData]] internal slot.
    return primitive;
}

// 19.4.2.1 Symbol.for (key)
JSTaggedValue BuiltinsSymbol::For(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, For);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Let stringKey be ToString(key).
    JSHandle<JSTaggedValue> key = BuiltinsSymbol::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> stringHandle = JSHandle<JSTaggedValue>::Cast(JSTaggedValue::ToString(thread, key));
    // 2.ReturnIfAbrupt
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    // 3.For each element e of the GlobalSymbolRegistry List,
    // If SameValue(e.[[key]], stringKey) is true, return e.[[symbol]].
    // 4.Assert: GlobalSymbolRegistry does not currently contain an entry for stringKey.
    // 5.Let newSymbol be a new unique Symbol value whose [[Description]] value is stringKey.
    // 6.Append the record { [[key]]: stringKey, [[symbol]]: newSymbol } to the GlobalSymbolRegistry List.
    // 7.Return newSymbol.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> symbol = factory->NewSymbolWithTable(stringHandle);
    return symbol.GetTaggedValue();
}

// 19.4.2.5 Symbol.keyFor (sym)
JSTaggedValue BuiltinsSymbol::KeyFor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, KeyFor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.If Type(sym) is not Symbol, throw a TypeError exception.
    JSHandle<JSTaggedValue> sym = BuiltinsSymbol::GetCallArg(argv, 0);
    if (!sym->IsSymbol()) {
        // return typeError
        THROW_TYPE_ERROR_AND_RETURN(thread, "KeyFor: sym is not Symbol", JSTaggedValue::Exception());
    }
    // 2.For each element e of the GlobalSymbolRegistry List,
    // If SameValue(e.[[symbol]], sym) is true, return e.[[key]].
    // 3.Assert: GlobalSymbolRegistry does not currently contain an entry for sym.
    // 4.Return undefined.
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto *table = env->GetRegisterSymbols().GetObject<SymbolTable>();
    JSTaggedValue key = table->FindSymbol(sym.GetTaggedValue());
    if (key.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }
    return JSTaggedValue::ToString(thread, JSHandle<JSTaggedValue>(thread, key)).GetTaggedValue();
}

// 19.4.3.4 Symbol.prototype [ @@toPrimitive ] ( hint )
JSTaggedValue BuiltinsSymbol::ToPrimitive(EcmaRuntimeCallInfo *argv)
{
    // The allowed values for hint are "default", "number", and "string".
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, ToPrimitive);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1.Let s be the this value.
    JSHandle<JSTaggedValue> sym = GetThis(argv);
    // 2.If Type(s) is Symbol, return s.
    if (sym->IsSymbol()) {
        return sym.GetTaggedValue();
    }
    // 3.If Type(s) is not Object, throw a TypeError exception.
    if (!sym->IsObject()) {
        // return TypeError
        THROW_TYPE_ERROR_AND_RETURN(thread, "ToPrimitive: s is not Object", JSTaggedValue::Exception());
    }
    ASSERT(sym->IsObject());
    // 4.If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
    // 5.Return the value of s's [[SymbolData]] internal slot.
    if (!sym->IsJSPrimitiveRef()) {
        // If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
        THROW_TYPE_ERROR_AND_RETURN(thread, "ToPrimitive: no [[SymbolData]]", JSTaggedValue::Exception());
    }
    // Let sym be the value of s's [[SymbolData]] internal slot.
    JSTaggedValue primitive = JSPrimitiveRef::Cast(sym->GetTaggedObject())->GetValue();
    ASSERT(primitive.IsSymbol());
    return primitive;
}

JSTaggedValue BuiltinsSymbol::DescriptionGetter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Symbol, DescriptionGetter);
    // 1.Let s be the this value.
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> s = GetThis(argv);
    // 2.Let sym be ? thisSymbolValue(s).
    // 3.Return sym.[[Description]].
    return ThisSymbolValue(thread, s);
}

JSTaggedValue BuiltinsSymbol::ThisSymbolValue(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    BUILTINS_API_TRACE(thread, Symbol, ThisSymbolValue);
    if (value->IsSymbol()) {
        JSTaggedValue desValue = JSSymbol::Cast(value->GetTaggedObject())->GetDescription();
        return desValue;
    }

    // If s does not have a [[SymbolData]] internal slot, throw a TypeError exception.
    if (!value->IsJSPrimitive()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "ThisSymbolValue: no SymbolData", JSTaggedValue::Exception());
    }
    JSTaggedValue primitive = JSPrimitiveRef::Cast(value->GetTaggedObject())->GetValue();
    ASSERT(primitive.IsSymbol());
    // Return the value of s's [[SymbolData]] internal slot.
    JSTaggedValue primitiveDesValue = JSSymbol::Cast(primitive.GetTaggedObject())->GetDescription();
    return primitiveDesValue;
}
}  // namespace panda::ecmascript::builtins
