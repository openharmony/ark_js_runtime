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

#include "common_stubs.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table.h"
#include "stub-inl.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

#ifndef NDEBUG
void MulGCTestStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef x = Int64Argument(1);
    GateRef y = Int64Argument(2); // 2: 3rd argument

    DEFVARIABLE(intX, VariableType::INT64(), Int64(0));
    DEFVARIABLE(intY, VariableType::INT64(), Int64(0));
    DEFVARIABLE(valuePtr, VariableType::INT64(), Int64(0));
    DEFVARIABLE(doubleX, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleY, VariableType::FLOAT64(), Double(0));
    Label xIsNumber(env);
    Label xNotNumberOryNotNumber(env);
    Label xIsNumberAndyIsNumber(env);
    Label xIsDoubleAndyIsDouble(env);
    Branch(TaggedIsNumber(x), &xIsNumber, &xNotNumberOryNotNumber);
    Bind(&xIsNumber);
    {
        Label yIsNumber(env);
        // if right.IsNumber()
        Branch(TaggedIsNumber(y), &yIsNumber, &xNotNumberOryNotNumber);
        Bind(&yIsNumber);
        {
            Label xIsInt(env);
            Label xNotInt(env);
            Branch(TaggedIsInt(x), &xIsInt, &xNotInt);
            Bind(&xIsInt);
            {
                intX = TaggedCastToInt64(x);
                doubleX = CastInt64ToFloat64(*intX);
                Jump(&xIsNumberAndyIsNumber);
            }
            Bind(&xNotInt);
            {
                doubleX = TaggedCastToDouble(x);
                Jump(&xIsNumberAndyIsNumber);
            }
        }
    }
    Bind(&xNotNumberOryNotNumber);
    Return(Hole(VariableType::JS_ANY()));
    Label yIsInt(env);
    Label yNotInt(env);
    Bind(&xIsNumberAndyIsNumber);
    {
        Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
        Bind(&yIsInt);
        {
            intY = TaggedCastToInt64(y);
            doubleY = CastInt64ToFloat64(*intY);
            Jump(&xIsDoubleAndyIsDouble);
        }
        Bind(&yNotInt);
        {
            doubleY = TaggedCastToDouble(y);
            Jump(&xIsDoubleAndyIsDouble);
        }
    }
    Bind(&xIsDoubleAndyIsDouble);
    doubleX = DoubleMul(*doubleX, *doubleY);
    GateRef ptr1 = CallRuntime(glue, RTSTUB_ID(GetTaggedArrayPtrTest), {Int64(JSTaggedValue::VALUE_UNDEFINED)});
    GateRef ptr2 = CallRuntime(glue, RTSTUB_ID(GetTaggedArrayPtrTest), {ptr1});
    auto value1 = GetValueFromTaggedArray(VariableType::INT64(), ptr1, Int32(0));
    GateRef tmp = CastInt64ToFloat64(value1);
    doubleX = DoubleMul(*doubleX, tmp);
    auto value2 = GetValueFromTaggedArray(VariableType::INT64(), ptr2, Int32(1));
    tmp = CastInt64ToFloat64(value2);
    doubleX = DoubleMul(*doubleX, tmp);
    Return(DoubleBuildTaggedWithNoGC(*doubleX));
}
#endif

void AddStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    Return(FastAdd(x, y));
}

void SubStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    Return(FastSub(x, y));
}

void MulStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    Return(FastMul(x, y));
}

void DivStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    Return(FastDiv(x, y));
}

void ModStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    Return(FastMod(glue, x, y));
}

void TypeOfStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    Return(FastTypeOf(glue, obj));
}

void EqualStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    (void)glue;
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2);
    Return(FastEqual(x, y));
}

void GetPropertyByIndexStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    Return(GetPropertyByIndex(glue, receiver, index));
}

void SetPropertyByIndexStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    Return(SetPropertyByIndex(glue, receiver, index, value, false));
}

void SetPropertyByIndexWithOwnStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef index = Int32Argument(2); /* 2 : 3rd parameter is index */
    GateRef value = TaggedArgument(3); /* 3 : 4th parameter is value */
    Return(SetPropertyByIndex(glue, receiver, index, value, true));
}

void GetPropertyByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    Return(GetPropertyByName(glue, receiver, key));
}

void SetPropertyByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, false));
}

void SetPropertyByNameWithOwnStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value, true));
}

void GetPropertyByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    DEFVARIABLE(key, VariableType::JS_ANY(), TaggedArgument(2)); /* 2 : 3rd parameter is key */

    Label isNumberOrStringSymbol(env);
    Label notNumber(env);
    Label isStringOrSymbol(env);
    Label notStringOrSymbol(env);
    Label exit(env);

    Branch(TaggedIsNumber(*key), &isNumberOrStringSymbol, &notNumber);
    Bind(&notNumber);
    {
        Branch(TaggedIsStringOrSymbol(*key), &isNumberOrStringSymbol, &notStringOrSymbol);
        Bind(&notStringOrSymbol);
        {
            Return(Hole());
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        GateRef index = TryToElementsIndex(*key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            Return(GetPropertyByIndex(glue, receiver, index));
        }
        Bind(&notValidIndex);
        {
            Label notNumber1(env);
            Label getByName(env);
            Branch(TaggedIsNumber(*key), &exit, &notNumber1);
            Bind(&notNumber1);
            {
                Label isString(env);
                Label notString(env);
                Label isInternalString(env);
                Label notIntenalString(env);
                Branch(TaggedIsString(*key), &isString, &notString);
                Bind(&isString);
                {
                    Branch(IsInternalString(*key), &isInternalString, &notIntenalString);
                    Bind(&isInternalString);
                    Jump(&getByName);
                    Bind(&notIntenalString);
                    {
                        key = CallRuntime(glue, RTSTUB_ID(NewInternalString), { *key });
                        Jump(&getByName);
                    }
                }
                Bind(&notString);
                {
                    Jump(&getByName);
                }
            }
            Bind(&getByName);
            {
                Return(GetPropertyByName(glue, receiver, *key));
            }
        }
    }
    Bind(&exit);
    Return(Hole());
}

void SetPropertyByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    Return(SetPropertyByValue(glue, receiver, key, value, false));
}

void SetPropertyByValueWithOwnStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2);              /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */
    Return(SetPropertyByValue(glue, receiver, key, value, true));
}

void TryLoadICByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef firstValue = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef secondValue = TaggedArgument(3); /* 3 : 4th parameter is value */

    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label cachedHandlerNotHole(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
               &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(LoadICWithHandler(glue, receiver, receiver, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
            Bind(&cachedHandlerNotHole);
            {
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    {
        Return(Hole());
    }
}

void TryLoadICByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef firstValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef secondValue = TaggedArgument(4); /* 4 : 5th parameter is value */

    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
            &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(LoadElement(receiver, key));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                auto cachedHandler = CheckPolyHClass(secondValue, hclass);
                Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
                Bind(&cachedHandlerNotHole);
                Return(LoadICWithHandler(glue, receiver, receiver, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole());
}

void TryStoreICByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef firstValue = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef secondValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef value = TaggedArgument(4); /* 4 : 5th parameter is value */
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label cachedHandlerNotHole(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
               &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        {
            Return(StoreICWithHandler(glue, receiver, receiver, value, secondValue));
        }
        Bind(&hclassNotEqualFirstValue);
        {
            GateRef cachedHandler = CheckPolyHClass(firstValue, hclass);
            Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
            Bind(&cachedHandlerNotHole);
            {
                Return(StoreICWithHandler(glue, receiver, receiver, value, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole(VariableType::INT64()));
}

void TryStoreICByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef firstValue = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef secondValue = TaggedArgument(4); /* 4 : 5th parameter is value */
    GateRef value = TaggedArgument(5); /* 5 : 6th parameter is value */
    Label receiverIsHeapObject(env);
    Label receiverNotHeapObject(env);
    Label hclassEqualFirstValue(env);
    Label hclassNotEqualFirstValue(env);
    Label firstValueEqualKey(env);
    Label cachedHandlerNotHole(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &receiverNotHeapObject);
    Bind(&receiverIsHeapObject);
    {
        GateRef hclass = LoadHClass(receiver);
        Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
            &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(ICStoreElement(glue, receiver, key, value, secondValue));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Int64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
            Bind(&firstValueEqualKey);
            {
                GateRef cachedHandler = CheckPolyHClass(secondValue, hclass);
                Branch(TaggedIsHole(cachedHandler), &receiverNotHeapObject, &cachedHandlerNotHole);
                Bind(&cachedHandlerNotHole);
                Return(StoreICWithHandler(glue, receiver, receiver, value, cachedHandler));
            }
        }
    }
    Bind(&receiverNotHeapObject);
    Return(Hole(VariableType::INT64()));
}

void SetValueWithBarrierStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    GateRef offset = PtrArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    SetValueWithBarrier(glue, obj, offset, value);
    Return();
}

void TestAbsoluteAddressRelocationStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef a = Int64Argument(0);
    GateRef b = Int64Argument(1);
    Label start(env);
    Jump(&start);
    Bind(&start);
    GateRef globalValueC = RelocatableData(0xabc);
    GateRef globalValueD = RelocatableData(0xbcd);
    GateRef dummyValueC = Load(VariableType::INT64(), globalValueC);
    GateRef dummyValueD = Load(VariableType::INT64(), globalValueD);
    // Load from same relocatable data twice to see if it breaks constant fold opt. Result shows it doesn't.
    GateRef dummyValueC1 = Load(VariableType::INT64(), globalValueC);
    GateRef result = Int64Add(a, Int64Add(b, Int64Add(dummyValueC, Int64Add(dummyValueD, dummyValueC1))));
    Return(result);
}

void JsProxyCallInternalStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    Label exit(env);
    Label isNull(env);
    Label notNull(env);
    Label isUndefined(env);
    Label isNotUndefined(env);

    GateRef glue = PtrArgument(0);
    GateRef argc = Int32Argument(1);
    GateRef proxy = TaggedPointerArgument(2); // callTarget
    GateRef argv = PtrArgument(3);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef handler = GetHandlerFromJSProxy(proxy);
    Branch(TaggedIsNull(handler), &isNull, &notNull);
    Bind(&isNull);
    {
        ThrowTypeAndReturn(glue, GET_MESSAGE_STRING_ID(NonCallable), Exception());
    }
    Bind(&notNull);
    {
        GateRef target = GetTargetFromJSProxy(proxy);
        GateRef globalConstOffset = IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit()));
        GateRef keyOffset = PtrAdd(globalConstOffset,
            PtrMul(IntPtr(static_cast<int64_t>(ConstantIndex::APPLY_STRING_INDEX)),
            IntPtr(sizeof(JSTaggedValue))));
        GateRef key = Load(VariableType::JS_ANY(), glue, keyOffset);
        GateRef method = CallNGCRuntime(glue, RTSTUB_ID(JSObjectGetMethod), {glue, handler, key});
        ReturnExceptionIfAbruptCompletion(glue);

        Branch(TaggedIsUndefined(method), &isUndefined, &isNotUndefined);
        Bind(&isUndefined);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCallWithArgV), {glue, argc, target, argv});
            Return(*result);
        }
        Bind(&isNotUndefined);
        {
            GateRef arrHandle = CallRuntime(glue, RTSTUB_ID(CreateArrayFromList), argc, argv);
            GateRef thisArg = Load(VariableType::JS_POINTER(), argv, IntPtr(2*sizeof(JSTaggedValue)));
            GateRef numArgs = Int32(6);
            result = CallNGCRuntime(glue, RTSTUB_ID(JSCall),
                {glue,  numArgs, method, Undefined(), handler, target, thisArg, arrHandle});
            Jump(&exit);
        }
    }
    Bind(&exit);
    Return(*result);
}

CallSignature CommonStubCSigns::callSigns_[CommonStubCSigns::NUM_OF_STUBS];

void CommonStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name, counter)                           \
    name##CallSignature::Initialize(&callSigns_[name]);          \
    callSigns_[name].SetID(name);                                \
    callSigns_[name].SetConstructor(                             \
        [](void* ciruit) {                                       \
            return static_cast<void*>(                           \
                new name##Stub(static_cast<Circuit*>(ciruit)));  \
        });

    COMMON_STUB_ID_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
}

void CommonStubCSigns::GetCSigns(std::vector<CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
