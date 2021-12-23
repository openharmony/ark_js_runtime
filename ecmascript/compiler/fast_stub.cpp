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
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

void FastAddStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef x = TaggedArgument(0);
    GateRef y = TaggedArgument(1);
    DEFVARIABLE(intX, MachineType::INT32, 0);
    DEFVARIABLE(intY, MachineType::INT32, 0);
    DEFVARIABLE(doubleX, MachineType::FLOAT64, 0);
    DEFVARIABLE(doubleY, MachineType::FLOAT64, 0);
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
    Return(GetHoleConstant(MachineType::UINT64));
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
    Return(DoubleBuildTaggedWithNoGC(*doubleX));
}

#ifndef NDEBUG
void FastMulGCTestStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    env->GetCircuit()->SetFrameType(FrameType::OPTIMIZED_ENTRY_FRAME);
    GateRef glue = PtrArgument(0);
    GateRef x = Int64Argument(1);
    GateRef y = Int64Argument(2); // 2: 3rd argument

    DEFVARIABLE(intX, MachineType::INT64, GetWord64Constant(0));
    DEFVARIABLE(intY, MachineType::INT64, GetWord64Constant(0));
    DEFVARIABLE(valuePtr, MachineType::INT64, GetWord64Constant(0));
    DEFVARIABLE(doubleX, MachineType::FLOAT64, GetDoubleConstant(0));
    DEFVARIABLE(doubleY, MachineType::FLOAT64, GetDoubleConstant(0));
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
    Return(GetHoleConstant(MachineType::UINT64));
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
    StubDescriptor *getTaggedArrayPtr = GET_STUBDESCRIPTOR(GetTaggedArrayPtrTest);
    GateRef ptr1 = CallRuntime(getTaggedArrayPtr, glue, GetWord64Constant(FAST_STUB_ID(GetTaggedArrayPtrTest)), {
            glue
        });
    GateRef ptr2 = CallRuntime(getTaggedArrayPtr, glue, GetWord64Constant(FAST_STUB_ID(GetTaggedArrayPtrTest)), {
            glue
        });
    (void)ptr2;
    auto value = Load(MachineType::UINT64, ptr1);
    GateRef value2 = CastInt64ToFloat64(value);
    doubleX = DoubleMul(*doubleX, value2);
    Return(DoubleBuildTaggedWithNoGC(*doubleX));
}
#endif

void FastSubStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef x = TaggedArgument(0);
    GateRef y = TaggedArgument(1);
    DEFVARIABLE(intX, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(intY, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(doubleX, MachineType::FLOAT64, GetDoubleConstant(0));
    DEFVARIABLE(doubleY, MachineType::FLOAT64, GetDoubleConstant(0));
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
    Return(GetHoleConstant(MachineType::UINT64));
    Bind(&xNotIntOryNotInt);
    doubleX = DoubleSub(*doubleX, *doubleY);
    Return(DoubleBuildTaggedWithNoGC(*doubleX));
    Bind(&xIsIntAndyIsInt);
    Return(IntBuildTaggedWithNoGC(*intX));
}

void FastMulStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef x = TaggedArgument(0);
    GateRef y = TaggedArgument(1);
    DEFVARIABLE(intX, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(intY, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(doubleX, MachineType::FLOAT64, GetDoubleConstant(0));
    DEFVARIABLE(doubleY, MachineType::FLOAT64, GetDoubleConstant(0));
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
    Return(GetHoleConstant(MachineType::UINT64));
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
    Return(DoubleBuildTaggedWithNoGC(*doubleX));
}

void FastDivStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef x = TaggedArgument(0);
    GateRef y = TaggedArgument(1);
    DEFVARIABLE(intX, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(intY, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(doubleX, MachineType::FLOAT64, GetDoubleConstant(0));
    DEFVARIABLE(doubleY, MachineType::FLOAT64, GetDoubleConstant(0));
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
    Return(GetHoleConstant(MachineType::UINT64));
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
            Return(DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE)));
            Bind(&xNeiZeroOrNan);
            {
                GateRef intXTmp = CastDoubleToInt64(*doubleX);
                GateRef intYtmp = CastDoubleToInt64(*doubleY);
                intXTmp = Word64And(Word64Xor(intXTmp, intYtmp), GetWord64Constant(base::DOUBLE_SIGN_MASK));
                intXTmp = Word64Xor(intXTmp, CastDoubleToInt64(GetDoubleConstant(base::POSITIVE_INFINITY)));
                doubleX = CastInt64ToFloat64(intXTmp);
                Return(DoubleBuildTaggedWithNoGC(*doubleX));
            }
        }
        Bind(&divisorNotZero);
        {
            doubleX = DoubleDiv(*doubleX, *doubleY);
            Return(DoubleBuildTaggedWithNoGC(*doubleX));
        }
    }
}

void FastModStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef x = TaggedArgument(1);
    GateRef y = TaggedArgument(2); // 2: 3rd argument
    DEFVARIABLE(intX, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(intY, MachineType::INT32, GetInt32Constant(0));
    DEFVARIABLE(doubleX, MachineType::FLOAT64, GetDoubleConstant(0));
    DEFVARIABLE(doubleY, MachineType::FLOAT64, GetDoubleConstant(0));
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
                    Return(IntBuildTaggedWithNoGC(*intX));
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
        Return(GetHoleConstant(MachineType::UINT64));
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
            Return(DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE)));
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
                    doubleX = CallRuntime(floatMod, glue, GetWord64Constant(FAST_STUB_ID(FloatMod)), {
                            *doubleX, *doubleY
                        });
                    Return(DoubleBuildTaggedWithNoGC(*doubleX));
                }
                Bind(&xIsZeroOryIsInf);
                Return(DoubleBuildTaggedWithNoGC(*doubleX));
            }
        }
    }
}

void FastTypeOfStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef obj = TaggedArgument(1);
    DEFVARIABLE(holder, MachineType::TAGGED, obj);
    GateRef gConstOffset = PtrAdd(glue, GetArchRelateConstant(env->GetGlueOffset(JSThread::GlueID::GLOBAL_CONST)));
    GateRef booleanIndex = GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefindStr = Load(MachineType::TAGGED_POINTER, gConstOffset, booleanIndex);
    DEFVARIABLE(resultRep, MachineType::TAGGED_POINTER, gConstUndefindStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label exit(env);
    Label defaultLabel(env);
    GateRef gConstBooleanStr = Load(
        MachineType::TAGGED_POINTER, gConstOffset, GetGlobalConstantString(ConstantIndex::BOOLEAN_STRING_INDEX));
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
                    MachineType::TAGGED_POINTER, gConstOffset,
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
                    resultRep = Load(MachineType::TAGGED_POINTER, gConstOffset,
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
                    MachineType::TAGGED_POINTER, gConstOffset,
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
                    resultRep = Load(MachineType::TAGGED_POINTER, gConstOffset,
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
                            MachineType::TAGGED_POINTER, gConstOffset,
                            GetGlobalConstantString(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&exit);
                    }
                    Bind(&objNotCallable);
                    {
                        resultRep = Load(
                            MachineType::TAGGED_POINTER, gConstOffset,
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
                    MachineType::TAGGED_POINTER, gConstOffset,
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

void FastEqualStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef x = TaggedArgument(0);
    GateRef y = TaggedArgument(1);
    Label xIsEqualy(env);
    Label xIsNotEqualy(env);
    Branch(Word64Equal(x, y), &xIsEqualy, &xIsNotEqualy);
    Bind(&xIsEqualy);
    {
        Label xIsDouble(env);
        Label xNotDoubleOrxNotNan(env);
        Branch(TaggedIsDouble(x), &xIsDouble, &xNotDoubleOrxNotNan);
        Bind(&xIsDouble);
        {
            GateRef doubleX = TaggedCastToDouble(x);
            Label xIsNan(env);
            Branch(DoubleIsNAN(doubleX), &xIsNan, &xNotDoubleOrxNotNan);
            Bind(&xIsNan);
            Return(TaggedFalse());
        }
        Bind(&xNotDoubleOrxNotNan);
        Return(TaggedTrue());
    }
    Bind(&xIsNotEqualy);
    {
        Label xIsNumber(env);
        Label xNotNumberAndxNotIntAndyNotInt(env);
        Branch(TaggedIsNumber(x), &xIsNumber, &xNotNumberAndxNotIntAndyNotInt);
        Bind(&xIsNumber);
        {
            Label xIsInt(env);
            Branch(TaggedIsInt(x), &xIsInt, &xNotNumberAndxNotIntAndyNotInt);
            Bind(&xIsInt);
            {
                Label yIsInt(env);
                Branch(TaggedIsInt(y), &yIsInt, &xNotNumberAndxNotIntAndyNotInt);
                Bind(&yIsInt);
                Return(TaggedFalse());
            }
        }
        Bind(&xNotNumberAndxNotIntAndyNotInt);
        {
            Label yIsUndefinedOrNull(env);
            Label xyNotUndefinedAndNull(env);
            Branch(TaggedIsUndefinedOrNull(y), &yIsUndefinedOrNull, &xyNotUndefinedAndNull);
            Bind(&yIsUndefinedOrNull);
            {
                Label xIsHeapObject(env);
                Label xNotHeapObject(env);
                Branch(TaggedIsHeapObject(x), &xIsHeapObject, &xNotHeapObject);
                Bind(&xIsHeapObject);
                Return(TaggedFalse());
                Bind(&xNotHeapObject);
                {
                    Label xIsUndefinedOrNull(env);
                    Branch(TaggedIsUndefinedOrNull(x), &xIsUndefinedOrNull, &xyNotUndefinedAndNull);
                    Bind(&xIsUndefinedOrNull);
                    Return(TaggedTrue());
                }
            }
            Bind(&xyNotUndefinedAndNull);
            {
                Label xIsBoolean(env);
                Label xNotBooleanAndyNotSpecial(env);
                Branch(TaggedIsBoolean(x), &xIsBoolean, &xNotBooleanAndyNotSpecial);
                Bind(&xIsBoolean);
                {
                    Label yIsSpecial(env);
                    Branch(TaggedIsSpecial(y), &yIsSpecial, &xNotBooleanAndyNotSpecial);
                    Bind(&yIsSpecial);
                    Return(TaggedFalse());
                }
                Bind(&xNotBooleanAndyNotSpecial);
                {
                    Return(GetHoleConstant(MachineType::UINT64));
                }
            }
        }
    }
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
    Return(SetPropertyByIndex(glue, receiver, index, value));
}

void GetPropertyByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    Return(GetPropertyByName(glue, receiver, key));
}

void FindOwnElement2Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef elements = TaggedArgument(1);
    GateRef index = Int32Argument(2);       // 2 : 3rd parameter
    GateRef isDict = Int32Argument(3);      // 3 : 4th parameter
    GateRef attr = PtrArgument(4);          // 4 : 5th parameter
    GateRef indexOrEntry = PtrArgument(5);  // 5 : 6rd parameter
    Return(FindOwnElement2(glue, elements, index, isDict, attr, indexOrEntry));
}

void SetPropertyByNameStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByName(glue, receiver, key, value));
}

void SetPropertyByNameWithOwnStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    GateRef key = TaggedArgument(2); // 2 : 3rd para
    GateRef value = TaggedArgument(3); // 3 : 4th para
    Return(SetPropertyByNameWithOwn(glue, receiver, key, value));
}

void FunctionCallInternalStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef func = PtrArgument(1, TypeCode::TAGGED_POINTER);
    GateRef thisArg = TaggedArgument(2); /* 2 : 3rd parameter is value */
    GateRef argc = Int32Argument(3); /* 3 : 4th parameter is value */
    GateRef argv = PtrArgument(4); /* 4 : 5th parameter is ptr */
    Label funcNotBuiltinsConstructor(env);
    Label funcIsBuiltinsConstructorOrFuncNotClassConstructor(env);
    Label funcIsClassConstructor(env);
    Branch(NotBuiltinsConstructor(func), &funcNotBuiltinsConstructor,
           &funcIsBuiltinsConstructorOrFuncNotClassConstructor);
    Bind(&funcNotBuiltinsConstructor);
    {
        Branch(IsClassConstructor(func), &funcIsClassConstructor, &funcIsBuiltinsConstructorOrFuncNotClassConstructor);
        Bind(&funcIsClassConstructor);
        ThrowTypeAndReturn(glue, GET_MESSAGE_STRING_ID(FunctionCallNotConstructor),
                           GetExceptionConstant(MachineType::UINT64));
    }
    Bind(&funcIsBuiltinsConstructorOrFuncNotClassConstructor);
    StubDescriptor *execute = GET_STUBDESCRIPTOR(Execute);
    Return(CallRuntime(execute, glue, GetWord64Constant(FAST_STUB_ID(Execute)), {
            glue, func, thisArg, argc, argv
        }));
}

void GetPropertyByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    DEFVARIABLE(key, MachineType::TAGGED, TaggedArgument(2)); /* 2 : 3rd parameter is key */

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
        GateRef index = TryToElementsIndex(*key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, GetInt32Constant(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            Return(GetPropertyByIndex(glue, receiver, index));
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
                        key = CallRuntime(newInternalString, glue,
                            GetWord64Constant(FAST_STUB_ID(NewInternalString)), { glue, *key });
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
    Return(GetHoleConstant());
}

void SetPropertyByValueStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef receiver = TaggedArgument(1);
    DEFVARIABLE(key, MachineType::TAGGED, TaggedArgument(2)); /* 2 : 3rd parameter is key */
    GateRef value = TaggedArgument(3);            /* 3 : 4th parameter is value */

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
            Return(GetHoleConstant(MachineType::UINT64));
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        GateRef index = TryToElementsIndex(*key);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, GetInt32Constant(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            Return(SetPropertyByIndex(glue, receiver, index, value));
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
                        key = CallRuntime(newInternalString, glue,
                            GetWord64Constant(FAST_STUB_ID(NewInternalString)), { glue, *key });
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
                Return(SetPropertyByName(glue, receiver, *key, value));
            }
        }
    }
    Bind(&exit);
    Return(GetHoleConstant(MachineType::UINT64));
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
        Branch(Word64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
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
        Return(GetHoleConstant());
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
        Branch(Word64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
            &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        Return(LoadElement(receiver, key));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Word64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
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
    Return(GetHoleConstant());
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
        Branch(Word64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
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
    Return(GetHoleConstant(MachineType::UINT64));
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
        Branch(Word64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
            &hclassEqualFirstValue, &hclassNotEqualFirstValue);
        Bind(&hclassEqualFirstValue);
        GateRef handlerInfo = TaggedCastToInt32(secondValue);
        Return(ICStoreElement(glue, receiver, key, value, handlerInfo));
        Bind(&hclassNotEqualFirstValue);
        {
            Branch(Word64Equal(firstValue, key), &firstValueEqualKey, &receiverNotHeapObject);
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
    Return(GetHoleConstant(MachineType::UINT64));
}
}  // namespace kungfu
