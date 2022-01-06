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
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

namespace panda::ecmascript::kungfu {
using OperandsVector = std::set<int>;
class BasicBlock;
using BasicBlockMap = std::map<int, std::unique_ptr<BasicBlock>>;
class LLVMIRBuilder;
using HandleType = void(LLVMIRBuilder::*)(GateRef gate);

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
    K_SIMD128,
    K_PTR_1, // Tagged Pointer
    K_META,
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
    GateRef operand;
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
    LLVMStubModule(const std::string &name, const std::string &triple);
    ~LLVMStubModule();

    void Initialize(const std::vector<int> &stubIndices);

    LLVMModuleRef GetModule() const
    {
        return module_;
    }
    LLVMTypeRef GetStubFunctionType(uint32_t index) const
    {
        ASSERT(index < CALL_STUB_MAXCOUNT);
        return stubFunctionType_[index];
    }

    LLVMValueRef GetStubFunction(uint32_t index)
    {
        ASSERT(index < ALL_STUB_MAXCOUNT);
        return stubFunctions_[index];
    }

    LLVMValueRef GetTestFunction(uint32_t index)
    {
#ifndef NDEBUG
            ASSERT(index < TEST_FUNC_MAXCOUNT);
            return testFunctions_[index];
#else
            return nullptr;
#endif
    }

    const CompilationConfig *GetCompilationConfig() const
    {
        return &compCfg_;
    }

private:
    LLVMValueRef GetLLVMFunctionByStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef GetLLVMFunctionTypeStubDescriptor(StubDescriptor *stubDescriptor);
    LLVMTypeRef ConvertLLVMTypeFromMachineType(MachineType type);
    std::array<LLVMValueRef, ALL_STUB_MAXCOUNT> stubFunctions_ {nullptr};
    std::array<LLVMTypeRef, CALL_STUB_MAXCOUNT> stubFunctionType_ {nullptr};
#ifndef NDEBUG
    std::array<LLVMValueRef, TEST_FUNC_MAXCOUNT> testFunctions_ {nullptr};
#endif
    LLVMModuleRef module_;
    CompilationConfig compCfg_;
};

#define OPCODES(V) \
    V(Call, (GateRef gate, const std::vector<GateRef> &inList)) \
    V(Alloca, (GateRef gate)) \
    V(Block, (int id, const OperandsVector &predecessors)) \
    V(Goto, (int block, int bbout)) \
    V(Parameter, (GateRef gate)) \
    V(Int32Constant, (GateRef gate, int32_t value)) \
    V(Int64Constant, (GateRef gate, int64_t value)) \
    V(Float64Constant, (GateRef gate, double value)) \
    V(ZExtInt, (GateRef gate, GateRef e1)) \
    V(SExtInt, (GateRef gate, GateRef e1)) \
    V(Load, (GateRef gate, GateRef base)) \
    V(Store, (GateRef gate, GateRef base, GateRef value)) \
    V(IntRev, (GateRef gate, GateRef e1)) \
    V(IntAdd, (GateRef gate, GateRef e1, GateRef e2)) \
    V(FloatAdd, (GateRef gate, GateRef e1, GateRef e2)) \
    V(FloatSub, (GateRef gate, GateRef e1, GateRef e2)) \
    V(FloatMul, (GateRef gate, GateRef e1, GateRef e2)) \
    V(FloatDiv, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntSub, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntMul, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntOr, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntAnd, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntXor, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntLsr, (GateRef gate, GateRef e1, GateRef e2)) \
    V(Int32LessThanOrEqual, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntOrUintCmp, (GateRef gate, GateRef e1, GateRef e2, LLVMIntPredicate opcode)) \
    V(FloatOrDoubleCmp, (GateRef gate, GateRef e1, GateRef e2, LLVMRealPredicate opcode)) \
    V(Branch, (GateRef gate, GateRef cmp, GateRef btrue, GateRef bfalse)) \
    V(Switch, (GateRef gate, GateRef input, const std::vector<GateRef> &outList)) \
    V(SwitchCase, (GateRef gate, GateRef switchBranch, GateRef out)) \
    V(Phi, (GateRef gate, const std::vector<GateRef> &srcGates)) \
    V(Return, (GateRef gate, GateRef popCount, const std::vector<GateRef> &operands)) \
    V(ReturnVoid, (GateRef gate)) \
    V(CastIntXToIntY, (GateRef gate, GateRef e1)) \
    V(ChangeInt32ToDouble, (GateRef gate, GateRef e1)) \
    V(ChangeDoubleToInt32, (GateRef gate, GateRef e1)) \
    V(CastInt64ToDouble, (GateRef gate, GateRef e1)) \
    V(CastDoubleToInt, (GateRef gate, GateRef e1)) \
    V(CastInt64ToPointer, (GateRef gate, GateRef e1)) \
    V(IntLsl, (GateRef gate, GateRef e1, GateRef e2)) \
    V(IntMod, (GateRef gate, GateRef e1, GateRef e2)) \
    V(FloatMod, (GateRef gate, GateRef e1, GateRef e2)) \
    V(ChangeTaggedPointerToInt64, (GateRef gate, GateRef e1))

class LLVMIRBuilder {
public:
    explicit LLVMIRBuilder(const std::vector<std::vector<GateRef>> *schedule, const Circuit *circuit,
                           LLVMStubModule *module, LLVMValueRef function, const CompilationConfig *cfg);
    ~LLVMIRBuilder();
    void Build();

private:
    #define DECLAREVISITOPCODE(name, signature) void Visit##name signature;
        OPCODES(DECLAREVISITOPCODE)
    #undef DECLAREVISITOPCODE
    #define DECLAREHANDLEOPCODE(name, ignore) void Handle##name(GateRef gate);
        OPCODES(DECLAREHANDLEOPCODE)
    #undef DECLAREHANDLEOPCODE

    void VisitBytecodeCall(GateRef gate, const std::vector<GateRef> &inList);
    BasicBlock *EnsureBasicBlock(int id);
    LLVMValueRef CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder, bool isCaller);
    void SaveCurrentSP();
    LLVMValueRef GetCurrentSP();
    LLVMValueRef ReadRegister(LLVMModuleRef &module, LLVMBuilderRef &builder,
        LLVMMetadataRef meta);
    void PrologueHandle(LLVMModuleRef &module, LLVMBuilderRef &builder);
    LLVMBasicBlockRef EnsureBasicBlock(BasicBlock *bb) const;
    LLVMTFBuilderBasicBlockImpl *EnsureBasicBlockImpl(BasicBlock *bb) const;
    void StartBuilder(BasicBlock *bb) const;

    LLVMTypeRef GetMachineRepType(MachineRep rep) const;
    int FindBasicBlock(GateRef gate) const;
    void EndCurrentBlock() const;
    void End();

    void ProcessPhiWorkList();
    void AssignHandleMap();
    std::string LLVMValueToString(LLVMValueRef val) const;
    LLVMTypeRef ConvertLLVMTypeFromTypeCode(TypeCode type) const;

    LLVMTypeRef GetArchRelate() const
    {
        if (compCfg_->IsArm32()) {
            return LLVMInt32Type();
        }
        return LLVMInt64Type();
    }
    LLVMTypeRef ConvertLLVMTypeFromGate(GateRef gate) const;
    LLVMValueRef PointerAdd(LLVMValueRef baseAddr, LLVMValueRef offset, LLVMTypeRef rep);
    LLVMValueRef VectorAdd(LLVMValueRef e1Value, LLVMValueRef e2Value, LLVMTypeRef rep);
    LLVMValueRef CanonicalizeToInt(LLVMValueRef value);
    LLVMValueRef CanonicalizeToPtr(LLVMValueRef value);
    LLVMValueRef GetCurrentSPFrame();
    LLVMValueRef GetCurrentSPFrameAddr();
    void SetCurrentSPFrame(LLVMValueRef sp);
    LLVMValueRef GetCurrentFrameType(LLVMValueRef currentSpFrameAddr);
    void ConstructFrame();
    void PushFrameContext(LLVMValueRef newSp, LLVMValueRef oldSp);
    void DestoryFrame();
private:
    const CompilationConfig *compCfg_ {nullptr};
    const std::vector<std::vector<GateRef>> *schedule_ {nullptr};
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
    std::unordered_map<GateRef, LLVMValueRef> gateToLLVMMaps_;
    std::unordered_map<OpCode::Op, HandleType> opCodeHandleMap_;
    std::set<OpCode::Op> opCodeHandleIgnore;
    LLVMTypeRef optFrameType_;
    int optFrameSize_;
    int slotSize_;
    int interpretedFrameSize_;
    LLVMTypeRef slotType_;
    int optLeaveFramePrevOffset_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H
