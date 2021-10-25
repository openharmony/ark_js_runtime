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

#ifndef ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H
#define ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/compiler/gate.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

namespace kungfu {
using OperandsVector = std::set<int>;
class BasicBlock;
using BasicBlockMap = std::map<int, std::unique_ptr<BasicBlock>>;
class LLVMIRBuilder;
typedef void (LLVMIRBuilder::*HandleType)(AddrShift gate);

enum class MachineRep {
    K_NONE,
    K_BIT,
    K_WORD8,
    K_WORD16,
    K_WORD32,
    K_WORD64,
    // FP representations must be last, and in order of increasing size.
    K_FLOAT32,
    K_FLOAT64,
    K_SIMD128
};

class BasicBlock {
public:
    explicit BasicBlock(int id) : id_(id)
    {
        predecessors_ = {};
        successors_ = {};
        impl_ = nullptr;
    }

    int GetId() const
    {
        return id_;
    }

    template<class T>
    inline T *GetImpl() const
    {
        return static_cast<T *>(impl_);
    }

    inline void SetImpl(void *impl)
    {
        impl_ = impl;
    }

    template<class T>
    inline void ResetImpl()
    {
        if (impl_) {
            delete GetImpl<T>();
            impl_ = nullptr;
        }
    }
    ~BasicBlock() = default;

private:
    std::vector<BasicBlock *> predecessors_ {};
    std::vector<BasicBlock *> successors_ {};
    int id_ {-1};
    void *impl_ {nullptr};
};

struct NotMergedPhiDesc {
    BasicBlock *pred;
    AddrShift operand;
    LLVMValueRef phi;
};

struct LLVMTFBuilderBasicBlockImpl {
    LLVMBasicBlockRef llvm_bb_ = nullptr;
    LLVMBasicBlockRef continuation = nullptr;
    bool started = false;
    bool ended = false;
    std::vector<NotMergedPhiDesc> not_merged_phis;
};

class LLVMStubModule {
public:
    explicit LLVMStubModule(const char *name, const char *triple);
    ~LLVMStubModule() = default;

    void Initialize();

    LLVMModuleRef GetModule() const
    {
        return module_;
    }

    LLVMTypeRef GetStubFunctionType(uint32_t index) const
    {
        ASSERT(index < MAX_STUB_FUNCTION_COUNT);
        return stubFunctionType_[index];
    }

    LLVMValueRef GetStubFunction(uint32_t index)
    {
        ASSERT(index < FAST_STUB_MAXCOUNT);
        return stubFunctions_[index];
    }

    LLVMValueRef GetTestFunction(uint32_t index)
    {
        ASSERT(index - TEST_FUNCTION_OFFSET < MAX_TEST_FUNCTION_COUNT);
        return testFunctions_[index - TEST_FUNCTION_OFFSET];
    }

private:
    LLVMValueRef GetLLVMFunctionByStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef GetLLVMFunctionTypeStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef ConvertLLVMTypeFromMachineType(MachineType type);
    static constexpr uint32_t MAX_STUB_FUNCTION_COUNT = kungfu::EXTERN_RUNTIME_STUB_MAXCOUNT;
    static constexpr uint32_t MAX_TEST_FUNCTION_COUNT = kungfu::TEST_FUNC_MAXCOUNT - kungfu::TEST_FUNC_BEGIN - 1;
    static constexpr uint32_t TEST_FUNCTION_OFFSET = kungfu::TEST_FUNC_BEGIN + 1;
    std::array<LLVMValueRef, FAST_STUB_MAXCOUNT> stubFunctions_ {nullptr};
    std::array<LLVMTypeRef, MAX_STUB_FUNCTION_COUNT> stubFunctionType_ {nullptr};
    std::array<LLVMValueRef, MAX_TEST_FUNCTION_COUNT> testFunctions_ {nullptr};
    LLVMModuleRef module_;
};

#define OPCODES(V) \
    V(Call, (AddrShift gate, const std::vector<AddrShift> &inList)) \
    V(Alloca, (AddrShift gate)) \
    V(Block, (int id, const OperandsVector &predecessors)) \
    V(Goto, (int block, int bbout)) \
    V(Parameter, (AddrShift gate) const ) \
    V(Int32Constant, (AddrShift gate, int32_t value) const ) \
    V(Int64Constant, (AddrShift gate, int64_t value) const ) \
    V(Float64Constant, (AddrShift gate, double value) const ) \
    V(ZExtInt, (AddrShift gate, AddrShift e1, MachineRep rep) const ) \
    V(SExtInt, (AddrShift gate, AddrShift e1, MachineRep rep) const ) \
    V(Load, (AddrShift gate, MachineRep rep, AddrShift base) const ) \
    V(Store, (AddrShift gate, MachineRep rep, AddrShift base, AddrShift value) const ) \
    V(IntRev, (AddrShift gate, AddrShift e1) const ) \
    V(IntAdd, (AddrShift gate, AddrShift e1, AddrShift e2, MachineRep rep) const ) \
    V(FloatAdd, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(FloatSub, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(FloatMul, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(FloatDiv, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntSub, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntMul, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntOr, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntAnd, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntXor, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntLsr, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(Int32LessThanOrEqual, (AddrShift gate, AddrShift e1, AddrShift e2) const ) \
    V(IntOrUintCmp, (AddrShift gate, AddrShift e1, AddrShift e2, LLVMIntPredicate opcode) const ) \
    V(FloatOrDoubleCmp, (AddrShift gate, AddrShift e1, AddrShift e2, LLVMRealPredicate opcode) const ) \
    V(Branch, (AddrShift gate, AddrShift cmp, AddrShift btrue, AddrShift bfalse)) \
    V(Switch, (AddrShift gate, AddrShift input, const std::vector<AddrShift> &outList)) \
    V(SwitchCase, (AddrShift gate, AddrShift switchBranch, AddrShift out)) \
    V(Phi, (AddrShift gate, const std::vector<AddrShift> &srcGates, MachineRep rep)) \
    V(Return, (AddrShift gate, AddrShift popCount, const std::vector<AddrShift> &operands) const ) \
    V(CastIntXToIntY, (AddrShift gate, AddrShift e1, MachineRep rep) const ) \
    V(ChangeInt32ToDouble, (AddrShift gate, AddrShift e1) const ) \
    V(ChangeDoubleToInt32, (AddrShift gate, AddrShift e1) const ) \
    V(CastInt64ToDouble, (AddrShift gate, AddrShift e1) const ) \
    V(CastDoubleToInt, (AddrShift gate, AddrShift e1) const ) \
    V(CastInt64ToPointer, (AddrShift gate, AddrShift e1) const ) \
    V(IntLsl, (AddrShift gate, AddrShift e1, AddrShift e2) const) \
    V(IntMod, (AddrShift gate, AddrShift e1, AddrShift e2) const) \
    V(FloatMod, (AddrShift gate, AddrShift e1, AddrShift e2) const) \


class LLVMIRBuilder {
public:
    explicit LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                           LLVMModuleRef module, LLVMValueRef function);
    explicit LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                           LLVMStubModule *module, LLVMValueRef function);
    ~LLVMIRBuilder();
    void Build();

private:
    #define DECLAREVISITOPCODE(name, signature) void Visit##name signature;
        OPCODES(DECLAREVISITOPCODE)
    #undef DECLAREVISITOPCODE
    #define DECLAREHANDLEOPCODE(name, ignore) void Handle##name(AddrShift gate);
        OPCODES(DECLAREHANDLEOPCODE)
    #undef DECLAREHANDLEOPCODE

    BasicBlock *EnsurBasicBlock(int id);
    LLVMValueRef LLVMCallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder);
    void PrologueHandle(LLVMModuleRef &module, LLVMBuilderRef &builder);
    LLVMBasicBlockRef EnsureLLVMBB(BasicBlock *bb) const;
    LLVMTFBuilderBasicBlockImpl *EnsureLLVMBBImpl(BasicBlock *bb) const;
    void StartLLVMBuilder(BasicBlock *bb) const;

    LLVMTypeRef GetMachineRepType(MachineRep rep) const;
    int FindBasicBlock(AddrShift gate) const;
    void EndCurrentBlock() const;
    void End();

    void ProcessPhiWorkList();
    void AssignHandleMap();
    std::string LLVMValueToString(LLVMValueRef val) const;

private:
    const std::vector<std::vector<AddrShift>> *schedule_ {nullptr};
    const Circuit *circuit_ {nullptr};
    BasicBlock *currentBb_ {nullptr};
    int lineNumber_ {0};

    LLVMModuleRef module_ {nullptr};
    LLVMContextRef context_;
    LLVMValueRef function_ {nullptr};
    LLVMBuilderRef builder_ {nullptr};
    std::map<GateId, int> instIdMapBbId_;
    BasicBlockMap bbIdMapBb_;

    std::vector<BasicBlock *> phiRebuildWorklist_;
    LLVMStubModule *stubModule_ {nullptr};
    std::unordered_map<OpCode::Op, HandleType> opCodeHandleMap_;
    std::set<OpCode::Op> opCodeHandleIgnore;
};
}  // namespace kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H