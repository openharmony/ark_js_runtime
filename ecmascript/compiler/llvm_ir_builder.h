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
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/js_method.h"
#include "llvm-c/Core.h"

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

class LLVMModule {
public:
    LLVMModule(const std::string &name, const std::string &triple);
    ~LLVMModule();
    void SetUpForCommonStubs();
    void SetUpForBytecodeHandlerStubs();
    LLVMValueRef AddFunc(const panda::ecmascript::JSMethod *method);
    LLVMModuleRef GetModule() const
    {
        return module_;
    }
    LLVMTypeRef GetFuncType(const CallSignature *stubDescriptor);
    LLVMValueRef GetFunction(size_t index)
    {
        return functions_[index];
    }

    const CallSignature *GetCSign(size_t index) const
    {
        return callSigns_[index];
    }

    const CompilationConfig *GetCompilationConfig() const
    {
        return &cfg_;
    }

    const std::vector<CallSignature*> &GetCSigns() const
    {
        return callSigns_;
    }

    size_t GetFuncCount() const
    {
        return functions_.size();
    }
private:
    void InitialLLVMFuncTypeAndFuncByModuleCSigns();
    LLVMValueRef AddAndGetFunc(CallSignature *stubDescriptor);
    LLVMTypeRef ConvertLLVMTypeFromVariableType(VariableType type);

    std::vector<LLVMValueRef> functions_;
    std::vector<CallSignature *> callSigns_;
    LLVMModuleRef module_;
    CompilationConfig cfg_;
};


#define OPCODES(V) \
    V(Call, (GateRef gate, const std::vector<GateRef> &inList, OpCode op))                \
    V(RuntimeCall, (GateRef gate, const std::vector<GateRef> &inList))                    \
    V(RuntimeCallWithArgv, (GateRef gate, const std::vector<GateRef> &inList))            \
    V(NoGcRuntimeCall, (GateRef gate, const std::vector<GateRef> &inList))                \
    V(BytecodeCall, (GateRef gate, const std::vector<GateRef> &inList))                   \
    V(Alloca, (GateRef gate))                                                             \
    V(Block, (int id, const OperandsVector &predecessors))                                \
    V(Goto, (int block, int bbout))                                                       \
    V(Parameter, (GateRef gate))                                                          \
    V(Constant, (GateRef gate, std::bitset<64> value))                                    \
    V(RelocatableData, (GateRef gate, uint64_t value))                                    \
    V(ZExtInt, (GateRef gate, GateRef e1))                                                \
    V(SExtInt, (GateRef gate, GateRef e1))                                                \
    V(Load, (GateRef gate, GateRef base))                                                 \
    V(Store, (GateRef gate, GateRef base, GateRef value))                                 \
    V(IntRev, (GateRef gate, GateRef e1))                                                 \
    V(Add, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Sub, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Mul, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(FloatDiv, (GateRef gate, GateRef e1, GateRef e2))                                   \
    V(IntDiv, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(UDiv, (GateRef gate, GateRef e1, GateRef e2))                                       \
    V(IntOr, (GateRef gate, GateRef e1, GateRef e2))                                      \
    V(IntAnd, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntXor, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntLsr, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(IntAsr, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(Int32LessThanOrEqual, (GateRef gate, GateRef e1, GateRef e2))                       \
    V(Cmp, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(Branch, (GateRef gate, GateRef cmp, GateRef btrue, GateRef bfalse))                 \
    V(Switch, (GateRef gate, GateRef input, const std::vector<GateRef> &outList))         \
    V(SwitchCase, (GateRef gate, GateRef switchBranch, GateRef out))                      \
    V(Phi, (GateRef gate, const std::vector<GateRef> &srcGates))                          \
    V(Return, (GateRef gate, GateRef popCount, const std::vector<GateRef> &operands))     \
    V(ReturnVoid, (GateRef gate))                                                         \
    V(CastIntXToIntY, (GateRef gate, GateRef e1))                                         \
    V(ChangeInt32ToDouble, (GateRef gate, GateRef e1))                                    \
    V(ChangeUInt32ToDouble, (GateRef gate, GateRef e1))                                   \
    V(ChangeDoubleToInt32, (GateRef gate, GateRef e1))                                    \
    V(BitCast, (GateRef gate, GateRef e1))                                                \
    V(IntLsl, (GateRef gate, GateRef e1, GateRef e2))                                     \
    V(Mod, (GateRef gate, GateRef e1, GateRef e2))                                        \
    V(ChangeTaggedPointerToInt64, (GateRef gate, GateRef e1))                             \
    V(ChangeInt64ToTagged, (GateRef gate, GateRef e1))

// runtime/common stub ID, opcodeOffset for bc stub
using StubIdType = std::variant<RuntimeStubCSigns::ID, CommonStubCSigns::ID, LLVMValueRef>;
class LLVMIRBuilder {
public:
    explicit LLVMIRBuilder(const std::vector<std::vector<GateRef>> *schedule, const Circuit *circuit,
                           LLVMModule *module, LLVMValueRef function, const CompilationConfig *cfg,
                           CallSignature::CallConv callConv, bool enableLog = false);
    ~LLVMIRBuilder();
    void Build();

private:
    #define DECLAREVISITOPCODE(name, signature) void Visit##name signature;
        OPCODES(DECLAREVISITOPCODE)
    #undef DECLAREVISITOPCODE
    #define DECLAREHANDLEOPCODE(name, ignore) void Handle##name(GateRef gate);
        OPCODES(DECLAREHANDLEOPCODE)
    #undef DECLAREHANDLEOPCODE

    BasicBlock *EnsureBasicBlock(int id);
    LLVMValueRef CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder, bool isCaller);
    void SaveCurrentSP();
    LLVMValueRef GetCurrentSP();
    LLVMValueRef ReadRegister(LLVMModuleRef &module, LLVMBuilderRef &builder,
        LLVMMetadataRef meta);
    void GenPrologue(LLVMModuleRef &module, LLVMBuilderRef &builder);
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

    LLVMTypeRef GetIntPtr() const
    {
        if (compCfg_->Is32Bit()) {
            return LLVMInt32Type();
        }
        return LLVMInt64Type();
    }
    LLVMTypeRef ConvertLLVMTypeFromGate(GateRef gate) const;
    int64_t GetBitWidthFromMachineType(MachineType machineType) const;
    LLVMValueRef PointerAdd(LLVMValueRef baseAddr, LLVMValueRef offset, LLVMTypeRef rep);
    LLVMValueRef VectorAdd(LLVMValueRef e1Value, LLVMValueRef e2Value, LLVMTypeRef rep);
    LLVMValueRef CanonicalizeToInt(LLVMValueRef value);
    LLVMValueRef CanonicalizeToPtr(LLVMValueRef value);
    LLVMValueRef GetCurrentSPFrame();
    LLVMValueRef GetCurrentSPFrameAddr();
    void SetCurrentSPFrame(LLVMValueRef sp);
    LLVMValueRef GetCurrentFrameType(LLVMValueRef currentSpFrameAddr);
    bool IsGCRelated(GateType typeCode) const;
    void SetFunctionCallConv();

    bool IsLogEnabled() const
    {
        return enableLog_;
    }
    LLVMValueRef GetFunction(LLVMValueRef glue, StubIdType id);
    bool IsInterpreted();
    bool IsOptimized();
    void SetTailCallAttr(LLVMValueRef call);
    void SetCallConvAttr(const CallSignature *calleeDescriptor, LLVMValueRef call);

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
    LLVMModule *llvmModule_ {nullptr};
    std::unordered_map<GateRef, LLVMValueRef> gateToLLVMMaps_;
    std::unordered_map<OpCode::Op, HandleType> opCodeHandleMap_;
    std::set<OpCode::Op> opCodeHandleIgnore;
    int slotSize_;
    LLVMTypeRef slotType_;
    CallSignature::CallConv callConv_ = CallSignature::CallConv::CCallConv;
    bool enableLog_ {false};

    enum class CallInputs : size_t {
        DEPEND = 0,
        TARGET,
        GLUE,
        FIRST_PARAMETER,
    };
};
}  // namespace panda::ecmascript::kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_LLVM_IR_BUILDER_H
