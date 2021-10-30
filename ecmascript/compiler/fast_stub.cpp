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

#include "fast_stub.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"
#include "llvm_ir_builder.h"

namespace kungfu {
using namespace panda::ecmascript;
void FastArrayLoadElementStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift aVal = Int64Argument(0);
    AddrShift indexVal = Int32Argument(1);

    // load a.length
    AddrShift lengthOffset = GetInt32Constant(JSArray::GetArrayLengthOffset());
    if (PtrValueCode() == ValueCode::INT64) {
        lengthOffset = SExtInt32ToInt64(lengthOffset);
    } else if (PtrValueCode() == ValueCode::INT32) {
        aVal = TruncInt64ToInt32(aVal);
    }
    AddrShift taggedLength = Load(MachineType::TAGGED_TYPE, aVal, lengthOffset);
    AddrShift intLength = TaggedCastToInt32(taggedLength);
    // if index < length
    Label ifTrue(env);
    Label ifFalse(env);
    Branch(Int32LessThan(indexVal, intLength), &ifTrue, &ifFalse);
    Bind(&ifTrue);
    Return(LoadFromObject(MachineType::TAGGED_TYPE, aVal, indexVal));
    Bind(&ifFalse);
    Return(GetUndefinedConstant());
}

void FastAddStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift x = Int64Argument(0);
    AddrShift y = Int64Argument(1);
    DEFVARIABLE(intX, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(intY, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
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
                intX = TaggedCastToInt32(x);
                doubleX = ChangeInt32ToFloat64(*intX);
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
    Return(GetHoleConstant());
    Label yIsInt(env);
    Label yNotInt(env);
    Bind(&xIsNumberAndyIsNumber);
    {
        Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
        Bind(&yIsInt);
        {
            intY = TaggedCastToInt32(y);
            doubleY = ChangeInt32ToFloat64(*intY);
            Jump(&xIsDoubleAndyIsDouble);
        }
        Bind(&yNotInt);
        {
            doubleY = TaggedCastToDouble(y);
            Jump(&xIsDoubleAndyIsDouble);
        }
    }
    Bind(&xIsDoubleAndyIsDouble);
    doubleX = DoubleAdd(*doubleX, *doubleY);
    Return(DoubleBuildTagged(*doubleX));
}

void FastSubStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift x = Int64Argument(0);
    AddrShift y = Int64Argument(1);
    DEFVARIABLE(intX, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(intY, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
    Label xIsNumber(env);
    Label xNotNumberOryNotNumber(env);
    Label xNotIntOryNotInt(env);
    Label xIsIntAndyIsInt(env);
    // if x is number
    Branch(TaggedIsNumber(x), &xIsNumber, &xNotNumberOryNotNumber);
    Bind(&xIsNumber);
    {
        Label yIsNumber(env);
        // if y is number
        Branch(TaggedIsNumber(y), &yIsNumber, &xNotNumberOryNotNumber);
        {
            Bind(&yIsNumber);
            {
                Label xIsInt(env);
                Label xNotInt(env);
                Branch(TaggedIsInt(x), &xIsInt, &xNotInt);
                Bind(&xIsInt);
                {
                    intX = TaggedCastToInt32(x);
                    Label yIsInt(env);
                    Label yNotInt(env);
                    Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
                    Bind(&yIsInt);
                    {
                        intY = TaggedCastToInt32(y);
                        intX = Int32Sub(*intX, *intY);
                        Jump(&xIsIntAndyIsInt);
                    }
                    Bind(&yNotInt);
                    {
                        doubleY = TaggedCastToDouble(y);
                        doubleX = ChangeInt32ToFloat64(*intX);
                        Jump(&xNotIntOryNotInt);
                    }
                }
                Bind(&xNotInt);
                {
                    Label yIsInt(env);
                    Label yNotInt(env);
                    doubleX = TaggedCastToDouble(x);
                    Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
                    Bind(&yIsInt);
                    {
                        intY = TaggedCastToInt32(y);
                        doubleY = ChangeInt32ToFloat64(*intY);
                        Jump(&xNotIntOryNotInt);
                    }
                    Bind(&yNotInt);
                    {
                        doubleY = TaggedCastToDouble(y);
                        Jump(&xNotIntOryNotInt);
                    }
                }
            }
        }
    }
    Bind(&xNotNumberOryNotNumber);
    Return(GetHoleConstant());
    Bind(&xNotIntOryNotInt);
    doubleX = DoubleSub(*doubleX, *doubleY);
    Return(DoubleBuildTagged(*doubleX));
    Bind(&xIsIntAndyIsInt);
    Return(IntBuildTagged(*intX));
}

void FastMulStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift x = Int64Argument(0);
    AddrShift y = Int64Argument(1);
    DEFVARIABLE(intX, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(intY, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
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
                intX = TaggedCastToInt32(x);
                doubleX = ChangeInt32ToFloat64(*intX);
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
    Return(GetHoleConstant());
    Label yIsInt(env);
    Label yNotInt(env);
    Bind(&xIsNumberAndyIsNumber);
    {
        Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
        Bind(&yIsInt);
        {
            intY = TaggedCastToInt32(y);
            doubleY = ChangeInt32ToFloat64(*intY);
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
    Return(DoubleBuildTagged(*doubleX));
}

void FastDivStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift x = Int64Argument(0);
    AddrShift y = Int64Argument(1);
    DEFVARIABLE(intX, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(intY, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
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
                intX = TaggedCastToInt32(x);
                doubleX = ChangeInt32ToFloat64(*intX);
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
    Return(GetHoleConstant());
    Label yIsInt(env);
    Label yNotInt(env);
    Bind(&xIsNumberAndyIsNumber);
    Branch(TaggedIsInt(y), &yIsInt, &yNotInt);
    Bind(&yIsInt);
    {
        intY = TaggedCastToInt32(y);
        doubleY = ChangeInt32ToFloat64(*intY);
        Jump(&xIsDoubleAndyIsDouble);
    }
    Bind(&yNotInt);
    {
        doubleY = TaggedCastToDouble(y);
        Jump(&xIsDoubleAndyIsDouble);
    }
    Bind(&xIsDoubleAndyIsDouble);
    {
        Label divisorIsZero(env);
        Label divisorNotZero(env);
        Branch(DoubleEqual(*doubleY, GetDoubleConstant(0.0)), &divisorIsZero, &divisorNotZero);
        Bind(&divisorIsZero);
        {
            Label xIsZeroOrNan(env);
            Label xNeiZeroOrNan(env);
            Label xIsZero(env);
            Label xNotZero(env);
            // dLeft == 0.0 || std::isnan(dLeft)
            Branch(DoubleEqual(*doubleX, GetDoubleConstant(0.0)), &xIsZero, &xNotZero);
            Bind(&xIsZero);
            Jump(&xIsZeroOrNan);
            Bind(&xNotZero);
            {
                Label xIsNan(env);
                Label xNotNan(env);
                Branch(DoubleIsNAN(*doubleX), &xIsNan, &xNotNan);
                Bind(&xIsNan);
                Jump(&xIsZeroOrNan);
                Bind(&xNotNan);
                Jump(&xNeiZeroOrNan);
            }
            Bind(&xIsZeroOrNan);
            Return(DoubleBuildTagged(GetDoubleConstant(base::NAN_VALUE)));
            Bind(&xNeiZeroOrNan);
            {
                AddrShift intXTmp = CastDoubleToInt64(*doubleX);
                AddrShift intYtmp = CastDoubleToInt64(*doubleY);
                intXTmp = Word64And(Word64Xor(intXTmp, intYtmp), GetWord64Constant(base::DOUBLE_SIGN_MASK));
                intXTmp = Word64Xor(intXTmp, CastDoubleToInt64(GetDoubleConstant(base::POSITIVE_INFINITY)));
                doubleX = CastInt64ToFloat64(intXTmp);
                Return(DoubleBuildTagged(*doubleX));
            }
        }
        Bind(&divisorNotZero);
        {
            doubleX = DoubleDiv(*doubleX, *doubleY);
            Return(DoubleBuildTagged(*doubleX));
        }
    }
}

void FindOwnElementStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift obj = PtrArgument(1);
    AddrShift index = Int32Argument(2);  // 2: 3rd parameter - index
    Label notDict(env);
    Label isDict(env);
    Label invalidValue(env);
    Label end(env);
    AddrShift elements = Load(MachineType::POINTER_TYPE, obj, GetPtrConstant(JSObject::ELEMENTS_OFFSET));
    Branch(IsDictionaryMode(elements), &isDict, &notDict);
    Bind(&notDict);
    {
        Label outOfArray(env);
        Label notOutOfArray(env);
        AddrShift arrayLength = Load(MachineType::UINT32_TYPE, elements,
                                     GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
        Branch(Int32LessThanOrEqual(arrayLength, index), &outOfArray, &notOutOfArray);
        Bind(&outOfArray);
        Jump(&invalidValue);
        Bind(&notOutOfArray);
        {
            AddrShift offset = PtrMul(ChangeInt32ToPointer(index), GetPtrConstant(JSTaggedValue::TaggedTypeSize()));
            AddrShift dataIndex = PtrAdd(offset, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
            AddrShift value = Load(MachineType::TAGGED_TYPE, elements, dataIndex);
            Label isHole1(env);
            Label notHole1(env);
            Branch(TaggedIsHole(value), &isHole1, &notHole1);
            Bind(&isHole1);
            Jump(&invalidValue);
            Bind(&notHole1);
            Return(value);
        }
        Bind(&invalidValue);
        Return(GetHoleConstant());
    }
    // IsDictionary
    Bind(&isDict);
    {
        AddrShift taggedIndex = IntBuildTagged(index);
        AddrShift entry = FindElementFromNumberDictionary(thread, elements, taggedIndex);
        Label notNegtiveOne(env);
        Label negtiveOne(env);
        Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
        Bind(&notNegtiveOne);
        {
            Return(GetValueFromDictionary(elements, entry));
        }
        Bind(&negtiveOne);
        Jump(&end);
    }
    Bind(&end);
    Return(GetHoleConstant());
}

void GetElementStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = Int64Argument(1);
    AddrShift index = Int64Argument(2);  // 2: 3rd parameter - index
    Label isHole(env);
    Label notHole(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label notHeapObj(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    AddrShift objPtr = ChangeInt64ToPointer(receiver);
    auto findOwnElementDescriptor = GET_STUBDESCRIPTOR(FindOwnElement);
    AddrShift callFindOwnElementVal =
        CallStub(findOwnElementDescriptor, thread, GetWord64Constant(FAST_STUB_ID(FindOwnElement)),
                 {thread, objPtr, index});
    Branch(TaggedIsHole(callFindOwnElementVal), &isHole, &notHole);
    Bind(&notHole);
    Return(callFindOwnElementVal);
    Bind(&isHole);
    receiver = Load(MachineType::TAGGED_TYPE, LoadHClass(objPtr), GetPtrConstant(JSHClass::PROTOTYPE_OFFSET));
    Branch(TaggedIsHeapObject(receiver), &loopEnd, &notHeapObj);
    Bind(&notHeapObj);
    Return(GetUndefinedConstant());
    Bind(&loopEnd);
    LoopEnd(&loopHead);
}

void FindOwnElement2Stub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift elements = PtrArgument(1);
    AddrShift index = Int32Argument(2);       // 2 : 3rd parameter
    AddrShift isDict = Int32Argument(3);      // 3 : 4th parameter
    AddrShift attr = PtrArgument(4);          // 4 : 5th parameter
    AddrShift indexOrEntry = PtrArgument(5);  // 5 : 6rd parameter
    isDict = ZExtInt1ToInt32(isDict);
    Label notDictionary(env);
    Label isDictionary(env);
    Label end(env);
    Branch(Word32Equal(isDict, GetInt32Constant(0)), &notDictionary, &isDictionary);
    Bind(&notDictionary);
    {
        AddrShift elementsLength =
            Load(MachineType::UINT32_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
        Label outOfElements(env);
        Label notOutOfElements(env);
        Branch(Int32LessThanOrEqual(elementsLength, index), &outOfElements, &notOutOfElements);
        Bind(&outOfElements);
        {
            Return(GetHoleConstant());
        }
        Bind(&notOutOfElements);
        {
            AddrShift value = GetValueFromTaggedArray(elements, index);
            Label isHole(env);
            Label notHole(env);
            Branch(TaggedIsHole(value), &isHole, &notHole);
            Bind(&isHole);
            Jump(&end);
            Bind(&notHole);
            {
                Store(MachineType::UINT32_TYPE, attr, GetPtrConstant(0),
                      GetInt32Constant(PropertyAttributes::GetDefaultAttributes()));
                Store(MachineType::UINT32_TYPE, indexOrEntry, GetPtrConstant(0), index);
                Return(value);
            }
        }
    }
    Bind(&isDictionary);
    {
        AddrShift entry = FindElementFromNumberDictionary(thread, elements, IntBuildTagged(index));
        Label notNegtiveOne(env);
        Label negtiveOne(env);
        Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
        Bind(&notNegtiveOne);
        {
            Store(MachineType::UINT32_TYPE, attr, GetPtrConstant(0), GetAttributesFromDictionary(elements, entry));
            Store(MachineType::UINT32_TYPE, indexOrEntry, GetPtrConstant(0), entry);
            Return(GetValueFromDictionary(elements, entry));
        }
        Bind(&negtiveOne);
        Jump(&end);
    }
    Bind(&end);
    Return(GetHoleConstant());
}

void SetElementStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = PtrArgument(1);
    DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, receiver);
    AddrShift index = Int32Argument(2);     // 2 : 3rd argument
    AddrShift value = Int64Argument(3);     // 3 : 4th argument
    AddrShift mayThrow = Int32Argument(4);  // 4 : 5th argument
    DEFVARIABLE(onPrototype, MachineType::BOOL_TYPE, FalseConstant());

    AddrShift pattr = Alloca(static_cast<int>(MachineRep::K_WORD32));
    AddrShift pindexOrEntry = Alloca(static_cast<int>(MachineRep::K_WORD32));
    Label loopHead(env);
    Label loopEnd(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        AddrShift elements = GetElements(*holder);
        AddrShift isDictionary = IsDictionaryMode(elements);
        StubDescriptor *findOwnElemnt2 = GET_STUBDESCRIPTOR(FindOwnElement2);
        AddrShift val = CallStub(findOwnElemnt2, thread, GetWord64Constant(FAST_STUB_ID(FindOwnElement2)),
                                 {thread, elements, index, isDictionary, pattr, pindexOrEntry});
        Label notHole(env);
        Label isHole(env);
        Branch(TaggedIsNotHole(val), &notHole, &isHole);
        Bind(&notHole);
        {
            Label isOnProtoType(env);
            Label notOnProtoType(env);
            Label afterOnProtoType(env);
            Branch(*onPrototype, &isOnProtoType, &notOnProtoType);
            Bind(&notOnProtoType);
            Jump(&afterOnProtoType);
            Bind(&isOnProtoType);
            {
                Label isExtensible(env);
                Label notExtensible(env);
                Label nextExtensible(env);
                Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
                Bind(&isExtensible);
                Jump(&nextExtensible);
                Bind(&notExtensible);
                {
                    Label isThrow(env);
                    Label notThrow(env);
                    Branch(Word32NotEqual(mayThrow, GetInt32Constant(0)), &isThrow, &notThrow);
                    Bind(&isThrow);
                    ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible), FalseConstant());
                    Bind(&notThrow);
                    Return(FalseConstant());
                }
                Bind(&nextExtensible);
                StubDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
                Return(CallRuntime(addElementInternal, thread, GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
                                   {thread, receiver, index, value,
                                    GetInt32Constant(PropertyAttributes::GetDefaultAttributes())}));
            }
            Bind(&afterOnProtoType);
            {
                AddrShift attr = Load(MachineType::INT32_TYPE, pattr);
                Label isAccessor(env);
                Label notAccessor(env);
                Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&notAccessor);
                {
                    Label isWritable(env);
                    Label notWritable(env);
                    Branch(IsWritable(attr), &isWritable, &notWritable);
                    Bind(&isWritable);
                    {
                        AddrShift elements = GetElements(receiver);
                        Label isDict(env);
                        Label notDict(env);
                        AddrShift indexOrEntry = Load(MachineType::INT32_TYPE, pindexOrEntry);
                        Branch(isDictionary, &isDict, &notDict);
                        Bind(&notDict);
                        {
                            StoreElement(elements, indexOrEntry, value);
                            UpdateRepresention(LoadHClass(receiver), value);
                            Return(TrueConstant());
                        }
                        Bind(&isDict);
                        {
                            UpdateValueAndAttributes(elements, indexOrEntry, value, attr);
                            Return(TrueConstant());
                        }
                    }
                    Bind(&notWritable);
                    {
                        Label isThrow(env);
                        Label notThrow(env);
                        Branch(Word32NotEqual(mayThrow, GetInt32Constant(0)), &isThrow, &notThrow);
                        Bind(&isThrow);
                        ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetReadOnlyProperty), FalseConstant());
                        Bind(&notThrow);
                        Return(FalseConstant());
                    }
                }
                Bind(&isAccessor);
                {
                    StubDescriptor *callsetter = GET_STUBDESCRIPTOR(CallSetter);
                    AddrShift setter = GetSetterFromAccessor(val);
                    Return(CallRuntime(callsetter, thread, GetWord64Constant(FAST_STUB_ID(CallSetter)),
                                       {thread, setter, receiver, value, TruncInt32ToInt1(mayThrow)}));
                }
            }
        }
        Bind(&isHole);
        {
            // holder equals to
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Label isHeapObj(env);
            Label notHeapObj(env);
            Branch(TaggedIsObject(*holder), &isHeapObj, &notHeapObj);
            Bind(&notHeapObj);
            {
                Label isExtensible(env);
                Label notExtensible(env);
                Label nextExtensible(env);
                Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
                Bind(&isExtensible);
                Jump(&nextExtensible);
                Bind(&notExtensible);
                {
                    Label isThrow(env);
                    Label notThrow(env);
                    Branch(Word32NotEqual(mayThrow, GetInt32Constant(0)), &isThrow, &notThrow);
                    Bind(&isThrow);
                    ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible), FalseConstant());
                    Bind(&notThrow);
                    Return(FalseConstant());
                }
                Bind(&nextExtensible);
                {
                    StubDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
                    Return(CallRuntime(addElementInternal, thread, GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
                                       {thread, receiver, index, value,
                                        GetInt32Constant(PropertyAttributes::GetDefaultAttributes())}));
                }
            }
            Bind(&isHeapObj);
            {
                Label isJsProxy(env);
                Label notJsProxy(env);
                Branch(IsJsProxy(*holder), &isJsProxy, &notJsProxy);
                Bind(&isJsProxy);
                {
                    StubDescriptor *setProperty = GET_STUBDESCRIPTOR(JSProxySetProperty);
                    Return(CallRuntime(
                        setProperty, thread, GetWord64Constant(FAST_STUB_ID(JSProxySetProperty)),
                        {thread, *holder, IntBuildTagged(index), value, receiver, TruncInt32ToInt1(mayThrow)}));
                }
                Bind(&notJsProxy);
                onPrototype = TrueConstant();
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
}

void GetPropertyByIndexStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = PtrArgument(1);
    AddrShift index = Int32Argument(2); /* 2 : 3rd parameter is index */

    DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, receiver);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        AddrShift hclass = LoadHClass(*holder);
        AddrShift jsType = GetObjectType(hclass);
        Label isSpecialIndexed(env);
        Label notSpecialIndexed(env);
        Label loopEnd(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndexed, &notSpecialIndexed);
        Bind(&isSpecialIndexed);
        {
            Return(GetHoleConstant());
        }
        Bind(&notSpecialIndexed);
        {
            AddrShift elements = GetElements(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                Branch(Word32LessThan(index, GetLengthofElements(elements)), &lessThanLength, &notLessThanLength);
                Bind(&lessThanLength);
                {
                    Label notHole(env);
                    Label isHole(env);
                    AddrShift value = GetValueFromTaggedArray(elements, index);
                    Branch(TaggedIsNotHole(value), &notHole, &isHole);
                    Bind(&notHole);
                    {
                        Return(value);
                    }
                    Bind(&isHole);
                    {
                        Jump(&loopExit);
                    }
                }
                Bind(&notLessThanLength);
                {
                    Return(GetHoleConstant());
                }
            }
            Bind(&isDictionaryElement);
            {
                AddrShift entry =
                    FindElementFromNumberDictionary(thread, elements, IntBuildTagged(index));
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    AddrShift attr = GetAttributesFromDictionary(elements, entry);
                    AddrShift value = GetValueFromDictionary(elements, entry);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        Label isInternal(env);
                        Label notInternal(env);
                        Branch(IsAccessorInternal(value), &isInternal, &notInternal);
                        Bind(&isInternal);
                        {
                            StubDescriptor *callAccessorGetter = GET_STUBDESCRIPTOR(AccessorGetter);
                            Return(CallRuntime(callAccessorGetter, thread,
                                               GetWord64Constant(FAST_STUB_ID(AccessorGetter)),
                                               {thread, value, *holder}));
                        }
                        Bind(&notInternal);
                        {
                            StubDescriptor *callGetter = GET_STUBDESCRIPTOR(CallGetter);
                            Return(CallRuntime(callGetter, thread, GetWord64Constant(FAST_STUB_ID(CallGetter)),
                                               {thread, value, receiver}));
                        }
                    }
                    Bind(&notAccessor);
                    {
                        Return(value);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            Return(GetUndefinedConstant());
        }
    }
}

void SetPropertyByIndexStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = PtrArgument(1);
    AddrShift index = Int32Argument(2); /* 2 : 3rd parameter is index */
    AddrShift value = Int64Argument(3); /* 3 : 4th parameter is value */

    DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, receiver);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        AddrShift hclass = LoadHClass(*holder);
        AddrShift jsType = GetObjectType(hclass);
        Label isSpecialIndex(env);
        Label notSpecialIndex(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
        Bind(&isSpecialIndex);
        Return(GetHoleConstant());
        Bind(&notSpecialIndex);
        {
            AddrShift elements = GetElements(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label isReceiver(env);
                Label notReceiver(env);
                Branch(Word64Equal(*holder, receiver), &isReceiver, &notReceiver);
                Bind(&isReceiver);
                {
                    AddrShift length = GetLengthofElements(elements);
                    Label inRange(env);
                    Branch(Word64LessThan(index, length), &inRange, &loopExit);
                    Bind(&inRange);
                    {
                        AddrShift value1 = GetValueFromTaggedArray(elements, index);
                        Label notHole(env);
                        Branch(Word64NotEqual(value1, GetHoleConstant()), &notHole, &loopExit);
                        Bind(&notHole);
                        {
                            StoreElement(elements, index, value);
                            Return(GetUndefinedConstant());
                        }
                    }
                }
                Bind(&notReceiver);
                Jump(&afterLoop);
            }
            Bind(&isDictionaryElement);
            Return(GetHoleConstant());
        }
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    {
        Label isExtensible(env);
        Label notExtensible(env);
        Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
        Bind(&isExtensible);
        {
            StubDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
            AddrShift result = CallRuntime(addElementInternal, thread,
                                           GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
                                           {thread, receiver, index, value,
                                           GetInt32Constant(PropertyAttributes::GetDefaultAttributes())});
            Label success(env);
            Label failed(env);
            Branch(result, &success, &failed);
            Bind(&success);
            Return(GetUndefinedConstant());
            Bind(&failed);
            Return(GetExceptionConstant());
        }
        Bind(&notExtensible);
        {
            ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible), GetExceptionConstant());
        }
    }
}

void GetPropertyByNameStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = Int64Argument(1);
    AddrShift key = Int64Argument(2); // 2 : 3rd para
    DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, receiver);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    // a do-while loop
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        // auto *hclass = holder.GetTaggedObject()->GetClass()
        // JSType jsType = hclass->GetObjectType()
        AddrShift hClass = LoadHClass(*holder);
        AddrShift jsType = GetObjectType(hClass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        // if branch condition : IsSpecialIndexedObj(jsType)
        Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            Return(GetHoleConstant());
        }
        Bind(&notSIndexObj);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            // if branch condition : LIKELY(!hclass->IsDictionaryMode())
            Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
                AddrShift layOutInfo = GetAttributesFromHclass(hClass);
                // int propsNumber = hclass->GetPropertiesNumber()
                AddrShift propsNum = ChangeInt64ToInt32(GetPropertiesNumberFromHClass(hClass));
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                StubDescriptor *findElemWithCache = GET_STUBDESCRIPTOR(FindElementWithCache);
                AddrShift entry = CallRuntime(findElemWithCache, thread,
                    GetWord64Constant(FAST_STUB_ID(FindElementWithCache)),
                    {thread, hClass, key, propsNum});
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    AddrShift propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                    AddrShift attr = TaggedCastToInt32(propAttr);
                    // auto value = JSObject::Cast(holder)->GetProperty(hclass, attr)
                    AddrShift value = JSObjectGetProperty(*holder, hClass, attr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        Label isInternal(env);
                        Label notInternal(env);
                        Branch(IsAccessorInternal(value), &isInternal, &notInternal);
                        Bind(&isInternal);
                        {
                            StubDescriptor *callAccessorGetter = GET_STUBDESCRIPTOR(AccessorGetter);
                            Return(CallRuntime(callAccessorGetter, thread,
                                GetWord64Constant(FAST_STUB_ID(AccessorGetter)),
                                {thread, value, *holder}));
                        }
                        Bind(&notInternal);
                        {
                            StubDescriptor *callGetter = GET_STUBDESCRIPTOR(CallGetter);
                            Return(CallRuntime(callGetter, thread, GetWord64Constant(FAST_STUB_ID(CallGetter)),
                                {thread, value, receiver}));
                        }
                    }
                    Bind(&notAccessor);
                    {
                        Return(value);
                    }
                }
                Bind(&noEntry);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDicMode);
            {
                // TaggedArray *array = TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetTaggedObject())
                AddrShift array = GetProperties(*holder);
                // int entry = dict->FindEntry(key)
                AddrShift entry = FindEntryFromNameDictionary(thread, array, key);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto value = dict->GetValue(entry)
                    AddrShift attr = GetAttributesFromDictionary(array, entry);
                    // auto attr = dict->GetAttributes(entry)
                    AddrShift value = GetValueFromDictionary(array, entry);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    // if branch condition : UNLIKELY(attr.IsAccessor())
                    Branch(IsAccessor(attr), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        Label isInternal1(env);
                        Label notInternal1(env);
                        Branch(IsAccessorInternal(value), &isInternal1, &notInternal1);
                        Bind(&isInternal1);
                        {
                            StubDescriptor *callAccessorGetter1 = GET_STUBDESCRIPTOR(AccessorGetter);
                            Return(CallRuntime(callAccessorGetter1, thread,
                                GetWord64Constant(FAST_STUB_ID(AccessorGetter)),
                                {thread, value, *holder}));
                        }
                        Bind(&notInternal1);
                        {
                            StubDescriptor *callGetter1 = GET_STUBDESCRIPTOR(CallGetter);
                            Return(CallRuntime(callGetter1, thread, GetWord64Constant(FAST_STUB_ID(CallGetter)),
                                {thread, value, receiver}));
                        }
                    }
                    Bind(&notAccessor1);
                    {
                        Return(value);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                // holder = hclass->GetPrototype()
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                // loop condition for a do-while loop
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            Return(GetUndefinedConstant());
        }
    }
}

void FastModStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift x = Int64Argument(1);
    AddrShift y = Int64Argument(2);
    DEFVARIABLE(intX, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(intY, MachineType::INT32_TYPE, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
    Label xIsInt(env);
    Label xNotIntOryNotInt(env);
    Branch(TaggedIsInt(x), &xIsInt, &xNotIntOryNotInt);
    Bind(&xIsInt);
    {
        Label yIsInt(env);
        Label xIsIntAndyIsInt(env);
        // if right.IsInt()
        Branch(TaggedIsInt(y), &yIsInt, &xNotIntOryNotInt);
        Bind(&yIsInt);
        {
            intX = TaggedCastToInt32(x);
            intY = TaggedCastToInt32(y);
            Jump(&xIsIntAndyIsInt);
        }
        Bind(&xIsIntAndyIsInt);
        {
            Label xGtZero(env);
            Label xGtZeroAndyGtZero(env);
            Branch(Int32GreaterThan(*intX, GetInt32Constant(0)), &xGtZero, &xNotIntOryNotInt);
            Bind(&xGtZero);
            {
                Branch(Int32GreaterThan(*intY, GetInt32Constant(0)), &xGtZeroAndyGtZero, &xNotIntOryNotInt);
                Bind(&xGtZeroAndyGtZero);
                {
                    intX = Int32Mod(*intX, *intY);
                    Return(IntBuildTagged(*intX));
                }
            }
        }
    }
    Bind(&xNotIntOryNotInt);
    {
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
                Label xIfInt(env);
                Label xIfNotInt(env);
                Branch(TaggedIsInt(x), &xIfInt, &xIfNotInt);
                Bind(&xIfInt);
                {
                    intX = TaggedCastToInt32(x);
                    doubleX = ChangeInt32ToFloat64(*intX);
                    Jump(&xIsNumberAndyIsNumber);
                }
                Bind(&xIfNotInt);
                {
                    doubleX = TaggedCastToDouble(x);
                    Jump(&xIsNumberAndyIsNumber);
                }
            }
        }
        Bind(&xNotNumberOryNotNumber);
        Return(GetHoleConstant());
        Label yIfInt(env);
        Label yIfNotInt(env);
        Bind(&xIsNumberAndyIsNumber);
        Branch(TaggedIsInt(y), &yIfInt, &yIfNotInt);
        Bind(&yIfInt);
        {
            intY = TaggedCastToInt32(y);
            doubleY = ChangeInt32ToFloat64(*intY);
            Jump(&xIsDoubleAndyIsDouble);
        }
        Bind(&yIfNotInt);
        {
            doubleY = TaggedCastToDouble(y);
            Jump(&xIsDoubleAndyIsDouble);
        }
        Bind(&xIsDoubleAndyIsDouble);
        {
            Label yIsZero(env);
            Label yNotZero(env);
            Label yIsZeroOrNanOrxIsNanOrInf(env);
            Label yNeiZeroOrNanAndxNeiNanOrInf(env);
            // dRight == 0.0 or std::isnan(dRight) or std::isnan(dLeft) or std::isinf(dLeft)
            Branch(DoubleEqual(*doubleY, GetDoubleConstant(0.0)), &yIsZero, &yNotZero);
            Bind(&yIsZero);
            Jump(&yIsZeroOrNanOrxIsNanOrInf);
            Bind(&yNotZero);
            {
                Label yIsNan(env);
                Label yNotNan(env);
                Branch(DoubleIsNAN(*doubleY), &yIsNan, &yNotNan);
                Bind(&yIsNan);
                Jump(&yIsZeroOrNanOrxIsNanOrInf);
                Bind(&yNotNan);
                {
                    Label xIsNan(env);
                    Label xNotNan(env);
                    Branch(DoubleIsNAN(*doubleX), &xIsNan, &xNotNan);
                    Bind(&xIsNan);
                    Jump(&yIsZeroOrNanOrxIsNanOrInf);
                    Bind(&xNotNan);
                    {
                        Label xIsInf(env);
                        Label xNotInf(env);
                        Branch(DoubleIsINF(*doubleX), &xIsInf, &xNotInf);
                        Bind(&xIsInf);
                        Jump(&yIsZeroOrNanOrxIsNanOrInf);
                        Bind(&xNotInf);
                        Jump(&yNeiZeroOrNanAndxNeiNanOrInf);
                    }
                }
            }
            Bind(&yIsZeroOrNanOrxIsNanOrInf);
            Return(DoubleBuildTagged(GetDoubleConstant(base::NAN_VALUE)));
            Bind(&yNeiZeroOrNanAndxNeiNanOrInf);
            {
                Label xIsFloatZero(env);
                Label xIsZeroOryIsInf(env);
                Label xNotZeroAndyNotInf(env);
                Branch(DoubleEqual(*doubleX, GetDoubleConstant(0.0)), &xIsFloatZero, &xNotZeroAndyNotInf);
                Bind(&xIsFloatZero);
                Jump(&xIsZeroOryIsInf);
                Label yIsInf(env);
                Label yNotInf(env);
                Bind(&xNotZeroAndyNotInf);
                Branch(DoubleIsINF(*doubleY), &yIsInf, &yNotInf);
                Bind(&yIsInf);
                Jump(&xIsZeroOryIsInf);
                Bind(&yNotInf);
                {
                    StubDescriptor *floatMod = GET_STUBDESCRIPTOR(FloatMod);
                    doubleX =CallRuntime(floatMod, thread, GetWord64Constant(FAST_STUB_ID(FloatMod)),
                        {*doubleX, *doubleY});
                    Return(DoubleBuildTagged(*doubleX));
                }
                Bind(&xIsZeroOryIsInf);
                Return(DoubleBuildTagged(*doubleX));
            }
        }
    }
}

void FastTypeOfStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift obj = PtrArgument(1);
    DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, obj);
    AddrShift gConstOffset = PtrAdd(thread, GetPtrConstant(panda::ecmascript::JSThread::GetGlobalConstantOffset()));
    AddrShift booleanIndex = GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX);
    AddrShift gConstUndefindStr = Load(MachineType::TAGGED_TYPE, gConstOffset, booleanIndex);
    DEFVARIABLE(resultRep, MachineType::TAGGED_TYPE, gConstUndefindStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label exit(env);
    Label defaultLabel(env);
    AddrShift gConstBooleanStr = Load(
        MachineType::TAGGED_TYPE, gConstOffset, GetGlobalConstantString(ConstantIndex::BOOLEAN_STRING_INDEX));
    Branch(Word64Equal(obj, GetWord64Constant(JSTaggedValue::VALUE_TRUE)), &objIsTrue, &objNotTrue);
    Bind(&objIsTrue);
    {
        resultRep = gConstBooleanStr;
        Jump(&exit);
    }
    Bind(&objNotTrue);
    {
        Label objIsFalse(env);
        Label objNotFalse(env);
        Branch(Word64Equal(obj, GetWord64Constant(JSTaggedValue::VALUE_FALSE)), &objIsFalse, &objNotFalse);
        Bind(&objIsFalse);
        {
            resultRep = gConstBooleanStr;
            Jump(&exit);
        }
        Bind(&objNotFalse);
        {
            Label objIsNull(env);
            Label objNotNull(env);
            Branch(Word64Equal(obj, GetWord64Constant(JSTaggedValue::VALUE_NULL)), &objIsNull, &objNotNull);
            Bind(&objIsNull);
            {
                resultRep = Load(
                    MachineType::TAGGED_TYPE, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNull);
            {
                Label objIsUndefined(env);
                Label objNotUndefined(env);
                Branch(Word64Equal(obj, GetWord64Constant(JSTaggedValue::VALUE_UNDEFINED)), &objIsUndefined,
                    &objNotUndefined);
                Bind(&objIsUndefined);
                {
                    resultRep = Load(MachineType::TAGGED_TYPE, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotUndefined);
                Jump(&defaultLabel);
            }
        }
    }
    Bind(&defaultLabel);
    {
        Label objIsHeapObject(env);
        Label objNotHeapObject(env);
        Branch(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotHeapObject);
        Bind(&objIsHeapObject);
        {
            Label objIsString(env);
            Label objNotString(env);
            Branch(IsString(obj), &objIsString, &objNotString);
            Bind(&objIsString);
            {
                resultRep = Load(
                    MachineType::TAGGED_TYPE, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::STRING_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotString);
            {
                Label objIsSymbol(env);
                Label objNotSymbol(env);
                Branch(IsSymbol(obj), &objIsSymbol, &objNotSymbol);
                Bind(&objIsSymbol);
                {
                    resultRep = Load(MachineType::TAGGED_TYPE, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::SYMBOL_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotSymbol);
                {
                    Label objIsCallable(env);
                    Label objNotCallable(env);
                    Branch(IsCallable(obj), &objIsCallable, &objNotCallable);
                    Bind(&objIsCallable);
                    {
                        resultRep = Load(
                            MachineType::TAGGED_TYPE, gConstOffset,
                                GetGlobalConstantString(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&exit);
                    }
                    Bind(&objNotCallable);
                    {
                        resultRep = Load(
                            MachineType::TAGGED_TYPE, gConstOffset,
                                GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&objNotHeapObject);
        {
            Label objIsNum(env);
            Label objNotNum(env);
            Branch(TaggedIsNumber(obj), &objIsNum, &objNotNum);
            Bind(&objIsNum);
            {
                resultRep = Load(
                    MachineType::TAGGED_TYPE, gConstOffset,
                        GetGlobalConstantString(ConstantIndex::NUMBER_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNum);
            Jump(&exit);
        }
    }
    Bind(&exit);
    Return(*resultRep);
}

void FunctionCallInternalStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift func = PtrArgument(1);
    AddrShift thisArg = Int64Argument(2); /* 2 : 3rd parameter is value */
    AddrShift argc = Int32Argument(3); /* 3 : 4th parameter is value */
    AddrShift argv = PtrArgument(4); /* 4 : 5th parameter is ptr */
    Label funcNotBuiltinsConstructor(env);
    Label funcIsBuiltinsConstructorOrFuncNotClassConstructor(env);
    Label funcIsClassConstructor(env);
    Branch(NotBuiltinsConstructor(func), &funcNotBuiltinsConstructor,
           &funcIsBuiltinsConstructorOrFuncNotClassConstructor);
    Bind(&funcNotBuiltinsConstructor);
    {
        Branch(IsClassConstructor(func), &funcIsClassConstructor, &funcIsBuiltinsConstructorOrFuncNotClassConstructor);
        Bind(&funcIsClassConstructor);
        ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(FunctionCallNotConstructor), GetExceptionConstant());
    }
    Bind(&funcIsBuiltinsConstructorOrFuncNotClassConstructor);
    StubDescriptor *execute = GET_STUBDESCRIPTOR(Execute);
    Return(CallRuntime(execute, thread, GetWord64Constant(FAST_STUB_ID(Execute)),
                       {thread, func, thisArg, argc, argv}));
}

void GetPropertyByValueStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = PtrArgument(1);
    DEFVARIABLE(key, MachineType::TAGGED_TYPE, PtrArgument(2)); /* 2 : 3rd parameter is key */

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
            Return(GetHoleConstant());
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        AddrShift index = TryToElementsIndex(*key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, GetInt32Constant(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            auto getPropertyByIndex = GET_STUBDESCRIPTOR(GetPropertyByIndex);
            Return(CallStub(getPropertyByIndex, thread, GetWord64Constant(FAST_STUB_ID(GetPropertyByIndex)),
                            {thread, receiver, index}));
        }
        Bind(&notValidIndex);
        {
            Label notNumber(env);
            Label getByName(env);
            Branch(TaggedIsNumber(*key), &exit, &notNumber);
            Bind(&notNumber);
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
                        StubDescriptor *newInternalString = GET_STUBDESCRIPTOR(NewInternalString);
                        key = CallRuntime(newInternalString, thread,
                                          GetWord64Constant(FAST_STUB_ID(NewInternalString)),
                                          {thread, *key});
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
                auto getPropertyByName = GET_STUBDESCRIPTOR(GetPropertyByName);
                Return(CallStub(getPropertyByName, thread, GetWord64Constant(FAST_STUB_ID(GetPropertyByName)),
                                {thread, receiver, *key}));
            }
        }
    }
    Bind(&exit);
    Return(GetHoleConstant());
}

void SetPropertyByValueStub::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift thread = PtrArgument(0);
    AddrShift receiver = PtrArgument(1);
    DEFVARIABLE(key, MachineType::TAGGED_TYPE, PtrArgument(2)); /* 2 : 3rd parameter is key */
    AddrShift value = Int64Argument(3);            /* 3 : 4th parameter is value */

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
            Return(GetHoleConstant());
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        AddrShift index = TryToElementsIndex(*key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, GetInt32Constant(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            auto setPropertyByIndex = GET_STUBDESCRIPTOR(SetPropertyByIndex);
            Return(CallStub(setPropertyByIndex, thread, GetWord64Constant(FAST_STUB_ID(SetPropertyByIndex)),
                            {thread, receiver, index, value}));
        }
        Bind(&notValidIndex);
        {
            Label notNumber(env);
            Label getByName(env);
            Branch(TaggedIsNumber(*key), &exit, &notNumber);
            Bind(&notNumber);
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
                        StubDescriptor *newInternalString = GET_STUBDESCRIPTOR(NewInternalString);
                        key = CallRuntime(newInternalString, thread,
                                          GetWord64Constant(FAST_STUB_ID(NewInternalString)),
                                          {thread, *key});
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
                auto setPropertyByName = GET_STUBDESCRIPTOR(SetPropertyByName);
                Return(CallStub(setPropertyByName, thread, GetWord64Constant(FAST_STUB_ID(SetPropertyByName)),
                                {thread, receiver, *key, value}));
            }
        }
    }
    Bind(&exit);
    Return(GetHoleConstant());
}
}  // namespace kungfu
