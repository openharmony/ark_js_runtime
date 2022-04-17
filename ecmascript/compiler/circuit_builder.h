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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#define DEFVAlUE(varname, cirBuilder, type, val) \
        Variable varname(cirBuilder, type, cirBuilder->NextVariableId(), val)

class Environment;
class Label;
class Variable;

#define BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                            \
    V(Int16Add, OpCode::ADD, MachineType::I16)                                    \
    V(Int32Add, OpCode::ADD, MachineType::I32)                                    \
    V(Int64Add, OpCode::ADD, MachineType::I64)                                    \
    V(DoubleAdd, OpCode::ADD, MachineType::F64)                                   \
    V(IntPtrAdd, OpCode::ADD, MachineType::ARCH)                                  \
    V(Int16Sub, OpCode::SUB, MachineType::I16)                                    \
    V(Int32Sub, OpCode::SUB, MachineType::I32)                                    \
    V(Int64Sub, OpCode::SUB, MachineType::I64)                                    \
    V(DoubleSub, OpCode::SUB, MachineType::F64)                                   \
    V(IntPtrSub, OpCode::SUB, MachineType::ARCH)                                  \
    V(Int32Mul, OpCode::MUL, MachineType::I32)                                    \
    V(Int64Mul, OpCode::MUL, MachineType::I64)                                    \
    V(DoubleMul, OpCode::MUL, MachineType::F64)                                   \
    V(ArchMul, OpCode::MUL, MachineType::ARCH)                                    \
    V(Int32Div, OpCode::SDIV, MachineType::I32)                                   \
    V(Int64Div, OpCode::SDIV, MachineType::I64)                                   \
    V(UInt32Div, OpCode::UDIV, MachineType::I32)                                  \
    V(UInt64Div, OpCode::UDIV, MachineType::I64)                                  \
    V(DoubleDiv, OpCode::FDIV, MachineType::F64)                                  \
    V(ArchDiv, OpCode::SDIV, MachineType::ARCH)                                   \
    V(Int32Mod, OpCode::SMOD, MachineType::I32)                                   \
    V(DoubleMod, OpCode::SMOD, MachineType::F64)                                  \
    V(BoolAnd, OpCode::AND, MachineType::I1)                                      \
    V(Int8And, OpCode::AND, MachineType::I8)                                      \
    V(Int32And, OpCode::AND, MachineType::I32)                                    \
    V(Int64And, OpCode::AND, MachineType::I64)                                    \
    V(ArchAnd, OpCode::AND, MachineType::ARCH)                                    \
    V(BoolOr, OpCode::OR, MachineType::I1)                                        \
    V(Int32Or, OpCode::OR, MachineType::I32)                                      \
    V(Int64Or, OpCode::OR, MachineType::I64)                                      \
    V(ArchOr, OpCode::OR, MachineType::ARCH)                                      \
    V(Int32Xor, OpCode::XOR, MachineType::I32)                                    \
    V(Int64Xor, OpCode::XOR, MachineType::I64)                                    \
    V(Int16LSL, OpCode::LSL, MachineType::I16)                                    \
    V(Int32LSL, OpCode::LSL, MachineType::I32)                                    \
    V(Int64LSL, OpCode::LSL, MachineType::I64)                                    \
    V(UInt64LSL, OpCode::LSL, MachineType::I64)                                   \
    V(ArchLSL, OpCode::LSL, MachineType::ARCH)                                    \
    V(Int8LSR, OpCode::LSR, MachineType::I8)                                      \
    V(UInt32LSR, OpCode::LSR, MachineType::I32)                                   \
    V(UInt64LSR, OpCode::LSR, MachineType::I64)                                   \
    V(ArchLSR, OpCode::LSR, MachineType::ARCH)                                    \
    V(Int32ASR, OpCode::ASR, MachineType::I32)

#define UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                             \
    V(BoolNot, OpCode::REV, MachineType::I1)                                      \
    V(Int32Not, OpCode::REV, MachineType::I32)                                    \
    V(Int64Not, OpCode::REV, MachineType::I64)                                    \
    V(CastDoubleToInt64, OpCode::BITCAST, MachineType::I64)                       \
    V(CastInt64ToFloat64, OpCode::BITCAST, MachineType::F64)

#define UNARY_ARITHMETIC_METHOD_LIST_WITHOUT_BITWIDTH(V)                          \
    V(SExtInt32ToInt64, OpCode::SEXT_TO_INT64)                                    \
    V(SExtInt1ToInt64, OpCode::SEXT_TO_INT64)                                     \
    V(SExtInt1ToInt32, OpCode::SEXT_TO_INT32)                                     \
    V(ZExtInt8ToInt16, OpCode::ZEXT_TO_INT16)                                     \
    V(ZExtInt32ToInt64, OpCode::ZEXT_TO_INT64)                                    \
    V(ZExtInt1ToInt64, OpCode::ZEXT_TO_INT64)                                     \
    V(ZExtInt1ToInt32, OpCode::ZEXT_TO_INT32)                                     \
    V(ZExtInt8ToInt32, OpCode::ZEXT_TO_INT32)                                     \
    V(ZExtInt8ToInt64, OpCode::ZEXT_TO_INT64)                                     \
    V(ZExtInt8ToPtr, OpCode::ZEXT_TO_ARCH)                                        \
    V(ZExtInt16ToPtr, OpCode::ZEXT_TO_ARCH)                                       \
    V(ZExtInt32ToPtr, OpCode::ZEXT_TO_ARCH)                                       \
    V(SExtInt32ToPtr, OpCode::SEXT_TO_ARCH)                                       \
    V(ZExtInt16ToInt32, OpCode::ZEXT_TO_INT32)                                    \
    V(ZExtInt16ToInt64, OpCode::ZEXT_TO_INT64)                                    \
    V(ChangeInt64ToInt32, OpCode::TRUNC_TO_INT32)                                 \
    V(ChangeInt32ToIntPtr, OpCode::ZEXT_TO_ARCH)                                  \
    V(TruncInt64ToInt32, OpCode::TRUNC_TO_INT32)                                  \
    V(TruncPtrToInt32, OpCode::TRUNC_TO_INT32)                                    \
    V(TruncInt64ToInt1, OpCode::TRUNC_TO_INT1)                                    \
    V(TruncInt64ToInt16, OpCode::TRUNC_TO_INT16)                                  \
    V(TruncInt32ToInt1, OpCode::TRUNC_TO_INT1)

#define BINARY_LOGIC_METHOD_LIST_WITHOUT_BITWIDTH(V)                              \
    V(Equal, OpCode::EQ)                                                          \
    V(NotEqual, OpCode::NE)                                                       \
    V(DoubleLessThan, OpCode::SLT)                                                \
    V(DoubleLessThanOrEqual, OpCode::SLE)                                         \
    V(DoubleGreaterThan, OpCode::SGT)                                             \
    V(DoubleGreaterThanOrEqual, OpCode::SGE)                                      \
    V(Int32LessThan, OpCode::SLT)                                                 \
    V(Int32LessThanOrEqual, OpCode::SLE)                                          \
    V(Int32GreaterThan, OpCode::SGT)                                              \
    V(Int32GreaterThanOrEqual, OpCode::SGE)                                       \
    V(UInt32LessThan, OpCode::ULT)                                                \
    V(UInt32LessThanOrEqual, OpCode::ULE)                                         \
    V(UInt32GreaterThan, OpCode::UGT)                                             \
    V(UInt32GreaterThanOrEqual, OpCode::UGE)                                      \
    V(Int64LessThan, OpCode::SLT)                                                 \
    V(Int64LessThanOrEqual, OpCode::SLE)                                          \
    V(Int64GreaterThan, OpCode::SGT)                                              \
    V(Int64GreaterThanOrEqual, OpCode::SGE)                                       \
    V(UInt64LessThan, OpCode::ULT)                                                \
    V(UInt64LessThanOrEqual, OpCode::ULE)                                         \
    V(UInt6464GreaterThan, OpCode::UGT)                                           \
    V(UInt6464GreaterThanOrEqual, OpCode::UGE)

class CompilationConfig {
public:
    enum class Triple {
        TRIPLE_AMD64,
        TRIPLE_AARCH64,
        TRIPLE_ARM32,
    };
    // fake parameters for register r1 ~ r3
    static constexpr int FAKE_REGISTER_PARAMTERS_ARM32 = 3;

    explicit CompilationConfig(const std::string &triple)
        : triple_(GetTripleFromString(triple))
    {
    }
    ~CompilationConfig() = default;

    inline bool Is32Bit() const
    {
        return triple_ == Triple::TRIPLE_ARM32;
    }

    inline bool IsAArch64() const
    {
        return triple_ == Triple::TRIPLE_AARCH64;
    }

    inline bool IsAmd64() const
    {
        return triple_ == Triple::TRIPLE_AMD64;
    }

    inline bool Is64Bit() const
    {
        return IsAArch64() || IsAmd64();
    }

    Triple GetTriple() const
    {
        return triple_;
    }

private:
    inline Triple GetTripleFromString(const std::string &triple)
    {
        if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AMD64;
        }

        if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AARCH64;
        }

        if (triple.compare("arm-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_ARM32;
        }
        UNREACHABLE();
    }
    Triple triple_;
};

class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    explicit CircuitBuilder(Circuit *circuit, CompilationConfig *cmpCfg) : circuit_(circuit), cmpCfg_(cmpCfg) {}
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);
    // low level interface
    GateRef Arguments(size_t index);
    GateRef Merge(GateRef *in, size_t controlCount);
    GateRef Selector(OpCode opcode, MachineType machineType, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Selector(OpCode opcode, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Int8(int8_t val);
    GateRef Int16(int16_t val);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef IntPtr(int64_t val);
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef UndefineConstant(GateType type = GateType::TAGGED_VALUE);
    GateRef HoleConstant(GateType type = GateType::TAGGED_VALUE);
    GateRef NullConstant(GateType type = GateType::TAGGED_VALUE);
    GateRef ExceptionConstant(GateType type = GateType::TAGGED_VALUE);
    GateRef RelocatableData(uint64_t val);
    GateRef Alloca(int size);
    GateRef Branch(GateRef state, GateRef condition);
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef IfTrue(GateRef ifBranch);
    GateRef IfFalse(GateRef ifBranch);
    GateRef SwitchCase(GateRef switchBranch, int64_t value);
    GateRef DefaultCase(GateRef switchBranch);
    GateRef DependRelay(GateRef state, GateRef depend);
    GateRef DependAnd(std::initializer_list<GateRef> args);
    GateRef TaggedNumber(OpCode opcode, GateRef value);
    GateRef BinaryArithmetic(OpCode opcode, MachineType machineType, GateRef left, GateRef right);
    GateRef UnaryArithmetic(OpCode opcode, MachineType machineType, GateRef value);
    GateRef UnaryArithmetic(OpCode opcode, GateRef value);
    GateRef BinaryLogic(OpCode opcode, GateRef left, GateRef right);

    // call related
    GateRef Call(const CallSignature *signature, GateRef glue, GateRef target, const std::vector<GateRef> &args,
        GateRef depend = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY)));
    GateRef NoGcRuntimeCall(const CallSignature *signature, GateRef glue, GateRef target,
                            GateRef depend, const std::vector<GateRef> &args);
    GateRef VariadicRuntimeCall(GateRef glue, GateRef target, GateRef depend, const std::vector<GateRef> &args);
    GateRef BytecodeCall(const CallSignature *signature, GateRef glue, GateRef target,
                         GateRef depend, const std::vector<GateRef> &args);
    static MachineType GetMachineTypeFromVariableType(VariableType type);
    Circuit *GetCircuit() const
    {
        return circuit_;
    }
    // constant
    inline GateRef True();
    inline GateRef False();
    inline GateRef Undefined(VariableType type = VariableType::JS_ANY());

    // call operation
    GateRef CallRuntimeWithDepend(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args);
    GateRef CallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel = false);
    GateRef CallNGCRuntime(GateRef glue, size_t index, const std::vector<GateRef> &args);
    GateRef CallStub(GateRef glue, size_t index, const std::vector<GateRef> &args);
    // memory
    inline GateRef Load(VariableType type, GateRef base, GateRef offset);
    void Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value);

#define ARITHMETIC_BINARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                 \
    inline GateRef NAME(GateRef x, GateRef y)                                             \
    {                                                                                     \
        return BinaryArithmetic(OpCode(OPCODEID), MACHINETYPEID, x, y);                   \
    }
    BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_BINARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_BINARY_OP_WITH_BITWIDTH

#define ARITHMETIC_UNARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                  \
    inline GateRef NAME(GateRef x)                                                        \
    {                                                                                     \
        return UnaryArithmetic(OpCode(OPCODEID), MACHINETYPEID, x);                       \
    }
    UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_UNARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_UNARY_OP_WITH_BITWIDTH

#define ARITHMETIC_UNARY_OP_WITHOUT_BITWIDTH(NAME, OPCODEID)                              \
    inline GateRef NAME(GateRef x)                                                        \
    {                                                                                     \
        return UnaryArithmetic(OpCode(OPCODEID), x);                                      \
    }
    UNARY_ARITHMETIC_METHOD_LIST_WITHOUT_BITWIDTH(ARITHMETIC_UNARY_OP_WITHOUT_BITWIDTH)
#undef ARITHMETIC_UNARY_OP_WITHOUT_BITWIDTH

#define LOGIC_BINARY_OP_WITHOUT_BITWIDTH(NAME, OPCODEID)                                  \
    inline GateRef NAME(GateRef x, GateRef y)                                             \
    {                                                                                     \
        return BinaryLogic(OpCode(OPCODEID), x, y);                                       \
    }
    BINARY_LOGIC_METHOD_LIST_WITHOUT_BITWIDTH(LOGIC_BINARY_OP_WITHOUT_BITWIDTH)
#undef LOGIC_BINARY_OP_WITHOUT_BITWIDTH

    // js world
    // cast operation
    inline GateRef TaggedCastToInt64(GateRef x);
    inline GateRef TaggedCastToInt32(GateRef x);
    inline GateRef TaggedCastToIntPtr(GateRef x);
    inline GateRef TaggedCastToDouble(GateRef x);
    inline GateRef ChangeTaggedPointerToInt64(GateRef x);
    inline GateRef ChangeInt64ToTagged(GateRef x);
    // bit operation
    inline GateRef TaggedSpecialValueChecker(GateRef x, JSTaggedType type);
    inline GateRef TaggedIsInt(GateRef x);
    inline GateRef TaggedIsDouble(GateRef x);
    inline GateRef TaggedIsObject(GateRef x);
    inline GateRef TaggedIsNumber(GateRef x);
    inline GateRef TaggedIsNotHole(GateRef x);
    inline GateRef TaggedIsHole(GateRef x);
    inline GateRef TaggedIsUndefined(GateRef x);
    inline GateRef TaggedIsException(GateRef x);
    inline GateRef TaggedIsSpecial(GateRef x);
    inline GateRef TaggedIsHeapObject(GateRef x);
    inline GateRef TaggedIsGeneratorObject(GateRef x);
    inline GateRef TaggedIsPropertyBox(GateRef x);
    inline GateRef TaggedIsWeak(GateRef x);
    inline GateRef TaggedIsPrototypeHandler(GateRef x);
    inline GateRef TaggedIsTransitionHandler(GateRef x);
    inline GateRef TaggedIsUndefinedOrNull(GateRef x);
    inline GateRef TaggedIsTrue(GateRef x);
    inline GateRef TaggedIsFalse(GateRef x);
    inline GateRef TaggedIsNull(GateRef x);
    inline GateRef TaggedIsBoolean(GateRef x);
    inline GateRef TaggedGetInt(GateRef x);
    inline GateRef TaggedTypeNGC(GateRef x);
    inline GateRef TaggedNGC(GateRef x);
    inline GateRef DoubleToTaggedNGC(GateRef x);
    inline GateRef DoubleToTaggedTypeNGC(GateRef x);
    inline GateRef Tagged(GateRef x);
    inline GateRef DoubleToTagged(GateRef x);
    inline GateRef TaggedTrue();
    inline GateRef TaggedFalse();
    inline GateRef GetValueFromTaggedArray(VariableType returnType, GateRef array, GateRef index);
    inline void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    inline GateRef GetGlobalConstantString(ConstantIndex index);
    // object operation
    inline GateRef LoadHClass(GateRef object);
    inline GateRef IsJsType(GateRef object, JSType type);
    inline GateRef GetObjectType(GateRef hClass);
    inline GateRef IsDictionaryModeByHClass(GateRef hClass);
    inline GateRef IsDictionaryElement(GateRef hClass);
    inline GateRef IsClassConstructor(GateRef object);
    inline GateRef IsClassPrototype(GateRef object);
    inline GateRef IsExtensible(GateRef object);
    inline GateRef IsEcmaObject(GateRef obj);
    inline GateRef IsJsObject(GateRef obj);
    inline GateRef BothAreString(GateRef x, GateRef y);
    inline GateRef IsCallable(GateRef obj);
    GateRef GetFunctionBitFieldFromJSFunction(GateRef function);
    GateRef GetModuleFromFunction(GateRef function);
    GateRef FunctionIsResolved(GateRef function);
    void SetResolvedToFunction(GateRef glue, GateRef function, GateRef value);
    void SetConstPoolToFunction(GateRef glue, GateRef function, GateRef value);
    void SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value);
    void SetModuleToFunction(GateRef glue, GateRef function, GateRef value);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
        GateRef value, GateRef attrOffset, VariableType type);
    void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    void SetEnvironment(Environment *env)
    {
        env_ = env;
    }
    Environment *GetCurrentEnvironment() const
    {
        return env_;
    }
    void SetCompilationConfig(CompilationConfig *cmpCfg)
    {
        cmpCfg_ = cmpCfg;
    }
    CompilationConfig *GetCompilationConfig()
    {
        return cmpCfg_;
    }
    // label related
    void NewEnvironment(GateRef hir);
    void DeleteCurrentEnvironment();
    inline int NextVariableId();
    inline void HandleException(GateRef result, Label *success, Label *exception, Label *exit, VariableType type);
    inline void HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal);
    inline void SubCfgEntry(Label *entry);
    inline void SubCfgExit();
    inline GateRef Return(GateRef value);
    inline GateRef Return();
    inline void Bind(Label *label);
    inline void Bind(Label *label, bool justSlowPath);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    inline Label *GetCurrentLabel() const;
    inline GateRef GetState() const;
    inline GateRef GetDepend() const;
private:
    Circuit *circuit_ {nullptr};
    Environment *env_ {nullptr};
    CompilationConfig *cmpCfg_ {nullptr};
};

class Label {
public:
    explicit Label() = default;
    explicit Label(Environment *env);
    explicit Label(CircuitBuilder *cirBuilder);
    ~Label() = default;
    Label(Label const &label) = default;
    Label &operator=(Label const &label) = default;
    Label(Label &&label) = default;
    Label &operator=(Label &&label) = default;
    inline void Seal();
    inline void WriteVariable(Variable *var, GateRef value);
    inline GateRef ReadVariable(Variable *var);
    inline void Bind();
    inline void MergeAllControl();
    inline void MergeAllDepend();
    inline void AppendPredecessor(const Label *predecessor);
    inline std::vector<Label> GetPredecessors() const;
    inline void SetControl(GateRef control);
    inline void SetPreControl(GateRef control);
    inline void MergeControl(GateRef control);
    inline GateRef GetControl() const;
    inline GateRef GetDepend() const;
    inline void SetDepend(GateRef depend);
private:
    class LabelImpl {
    public:
        LabelImpl(Environment *env, GateRef control)
            : env_(env), control_(control), predeControl_(-1), isSealed_(false)
        {
        }
        ~LabelImpl() = default;
        NO_MOVE_SEMANTIC(LabelImpl);
        NO_COPY_SEMANTIC(LabelImpl);
        void Seal();
        void WriteVariable(Variable *var, GateRef value);
        GateRef ReadVariable(Variable *var);
        void Bind();
        void MergeAllControl();
        void MergeAllDepend();
        void AppendPredecessor(LabelImpl *predecessor);
        std::vector<LabelImpl *> GetPredecessors() const
        {
            return predecessors_;
        }
        void SetControl(GateRef control)
        {
            control_ = control;
        }
        void SetPreControl(GateRef control)
        {
            predeControl_ = control;
        }
        void MergeControl(GateRef control)
        {
            if (predeControl_ == -1) {
                predeControl_ = control;
                control_ = predeControl_;
            } else {
                otherPredeControls_.push_back(control);
            }
        }
        GateRef GetControl() const
        {
            return control_;
        }
        void SetDepend(GateRef depend)
        {
            depend_ = depend;
        }
        GateRef GetDepend() const
        {
            return depend_;
        }
    private:
        bool IsNeedSeal() const;
        bool IsSealed() const
        {
            return isSealed_;
        }
        bool IsLoopHead() const;
        bool IsControlCase() const;
        GateRef ReadVariableRecursive(Variable *var);
        Environment *env_;
        GateRef control_;
        GateRef predeControl_ {-1};
        GateRef dependRelay_ {-1};
        GateRef depend_ {-1};
        GateRef loopDepend_ {-1};
        std::vector<GateRef> otherPredeControls_;
        bool isSealed_ {false};
        std::map<Variable *, GateRef> valueMap_;
        std::vector<GateRef> phi;
        std::vector<LabelImpl *> predecessors_;
        std::map<Variable *, GateRef> incompletePhis_;
    };
    explicit Label(LabelImpl *impl) : impl_(impl) {}
    friend class Environment;
    LabelImpl *GetRawLabel() const
    {
        return impl_;
    }
    LabelImpl *impl_ {nullptr};
};

class Environment {
public:
    using LabelImpl = Label::LabelImpl;
    Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder);
    Environment(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist,
        Circuit *circuit, CircuitBuilder *builder);
    ~Environment();
    Label *GetCurrentLabel() const
    {
        return currentLabel_;
    }
    void SetCurrentLabel(Label *label)
    {
        currentLabel_ = label;
    }
    CircuitBuilder *GetCircuitBuilder()
    {
        return circuitBuilder_;
    }
    Circuit *GetCircuit()
    {
        return circuit_;
    }
    int NextVariableId()
    {
        return nextVariableId_++;
    }
    inline GateType GetGateType(GateRef gate) const;
    inline Label GetLabelFromSelector(GateRef sel);
    inline void AddSelectorToLabel(GateRef sel, Label label);
    inline LabelImpl *NewLabel(Environment *env, GateRef control = -1);
    inline void SubCfgEntry(Label *entry);
    inline void SubCfgExit();
    inline GateRef GetInput(size_t index) const;
private:
    Label *currentLabel_ {nullptr};
    Circuit *circuit_ {nullptr};
    CircuitBuilder *circuitBuilder_ {nullptr};
    std::unordered_map<GateRef, LabelImpl *> phiToLabels_;
    std::vector<GateRef> inputList_;
    Label entry_;
    std::vector<LabelImpl *> rawLabels_;
    std::stack<Label *> stack_;
    int nextVariableId_ {0};
};

class Variable {
public:
    Variable(Environment *env, VariableType type, uint32_t id, GateRef value) : id_(id), type_(type), env_(env)
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    Variable(CircuitBuilder *cirbuilder, VariableType type, uint32_t id, GateRef value)
        : id_(id), type_(type), env_(cirbuilder->GetCurrentEnvironment())
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    ~Variable() = default;
    NO_MOVE_SEMANTIC(Variable);
    NO_COPY_SEMANTIC(Variable);
    void Bind(GateRef value)
    {
        currentValue_ = value;
    }
    GateRef Value() const
    {
        return currentValue_;
    }
    VariableType Type() const
    {
        return type_;
    }
    bool IsBound() const
    {
        return currentValue_ != 0;
    }
    Variable &operator=(const GateRef value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
        return *this;
    }
    GateRef operator*()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }
    GateRef AddPhiOperand(GateRef val);
    GateRef AddOperandToSelector(GateRef val, size_t idx, GateRef in);
    GateRef TryRemoveTrivialPhi(GateRef phi);
    void RerouteOuts(const std::vector<Out *> &outs, Gate *newGate);
    bool IsSelector(GateRef gate) const
    {
        return env_->GetCircuit()->IsSelector(gate);
    }
    bool IsSelector(const Gate *gate) const
    {
        return gate->GetOpCode() == OpCode::VALUE_SELECTOR;
    }
    uint32_t GetId() const
    {
        return id_;
    }
private:
    uint32_t id_;
    VariableType type_;
    GateRef currentValue_ {0};
    Environment *env_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
