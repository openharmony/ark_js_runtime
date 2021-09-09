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
    std::unordered_map<int, LLVMValueRef> values_ = {};
    bool started = false;
    bool ended = false;
    std::vector<NotMergedPhiDesc> not_merged_phis;
};

class LLVMStubModule {
public:
    explicit LLVMStubModule(const char *name);
    ~LLVMStubModule() = default;

    void Initialize();

    LLVMModuleRef GetModule() const
    {
        return module_;
    }

    LLVMTypeRef GetExternalFunctionType(int index) const
    {
        ASSERT(index < MAX_EXTERNAL_FUNCTION_COUNT);
        return externalFuctionType_[index - EXTERNAL_FUNCTION_OFFSET];
    }

    LLVMValueRef GetStubFunction(int index)
    {
        ASSERT(index < FAST_STUB_MAXCOUNT);
        return stubFunctions_[index];
    }

private:
    LLVMValueRef GetLLVMFunctionByStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef GetLLVMFunctionTypeStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef ConvertLLVMTypeFromMachineType(MachineType type);
    static constexpr uint32_t MAX_EXTERNAL_FUNCTION_COUNT =
        kungfu::EXTERN_RUNTIME_STUB_MAXCOUNT - kungfu::EXTERNAL_RUNTIME_STUB_BEGIN - 1;
    static constexpr uint32_t EXTERNAL_FUNCTION_OFFSET = kungfu::EXTERNAL_RUNTIME_STUB_BEGIN + 1;
    std::array<LLVMValueRef, FAST_STUB_MAXCOUNT> stubFunctions_ {nullptr};
    std::array<LLVMTypeRef, MAX_EXTERNAL_FUNCTION_COUNT> externalFuctionType_ {nullptr};
    LLVMModuleRef module_;
};

class LLVMIRBuilder {
public:
    explicit LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit);
    explicit LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                           LLVMModuleRef module, LLVMValueRef function);
    explicit LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                           LLVMStubModule *module, LLVMValueRef function);
    void Build();
    ~LLVMIRBuilder() = default;

private:
    void VisitCall(AddrShift gate, const std::vector<AddrShift> &inList);
    void VisitAlloca(AddrShift gate);
    void VisitBlock(int id, const OperandsVector &predecessors);
    void VisitGoto(int block, int bbout);
    void VisitParameter(AddrShift gate) const;
    void VisitInt32Constant(AddrShift gate, int32_t value) const;
    void VisitInt64Constant(AddrShift gate, int64_t value) const;
    void VisitFloat64Constant(AddrShift gate, int64_t value) const;
    void VisitZExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const;
    void VisitSExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const;
    void VisitLoad(AddrShift gate, MachineRep rep, AddrShift base) const;
    void VisitStore(AddrShift gate, MachineRep rep, AddrShift base, AddrShift value) const;
    void VisitIntRev(AddrShift gate, AddrShift e1) const;
    void VisitIntAdd(AddrShift gate, AddrShift e1, AddrShift e2, MachineRep rep) const;
    void VisitFloatAdd(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitFloatSub(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitFloatMul(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitFloatDiv(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntSub(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntMul(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntOr(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntAnd(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntXor(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntLsr(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitInt32LessThanOrEqual(AddrShift gate, AddrShift e1, AddrShift e2) const;
    void VisitIntOrUintCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMIntPredicate opcode) const;
    void VisitFloatOrDoubleCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMRealPredicate opcode) const;
    void VisitBranch(AddrShift gate, AddrShift cmp, AddrShift btrue, AddrShift bfalse);
    void VisitSwitch(AddrShift gate, AddrShift input, const std::vector<AddrShift> &outList);
    void VisitSwitchCase(AddrShift gate, AddrShift switchBranch, AddrShift out);
    void VisitPhi(AddrShift gate, const std::vector<AddrShift> &srcGates, MachineRep rep);
    void VisitReturn(AddrShift gate, AddrShift popCount, const std::vector<AddrShift> &operands) const;
    void VisitCastIntXToIntY(AddrShift gate, AddrShift e1, MachineRep rep) const;
    void VisitCastInt32ToDouble(AddrShift gate, AddrShift e1) const;
    void VisitCastInt64ToDouble(AddrShift gate, AddrShift e1) const;
    void VisitCastDoubleToInt(AddrShift gate, AddrShift e1) const;
    void VisitCastInt64ToPointer(AddrShift gate, AddrShift e1) const;

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
};
}  // namespace kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H