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

#include "ecmascript/compiler/llvm_ir_builder.h"

#include <iostream>
#include <set>
#include <string>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/compiler_macros.h"
#include "ecmascript/compiler/fast_stub.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"
#include "securec.h"
#include "utils/logger.h"

namespace panda::ecmascript::kungfu {
LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<GateRef>> *schedule, const Circuit *circuit,
                             LLVMStubModule *module, LLVMValueRef function, const CompilationConfig *cfg)
    : compCfg_(cfg), schedule_(schedule), circuit_(circuit), module_(module->GetModule()),
      function_(function), stubModule_(module)
{
    builder_ = LLVMCreateBuilder();
    context_ = LLVMGetGlobalContext();
    bbIdMapBb_.clear();
    if (circuit_->GetFrameType() == FrameType::INTERPRETER_FRAME) {
        LLVMSetFunctionCallConv(function_, LLVMGHCCallConv);
    } else if (circuit_->GetFrameType() == FrameType::OPTIMIZED_FRAME) {
        if (!compCfg_->Is32Bit()) {  // Arm32 not support webkit jscc calling convention
            LLVMSetFunctionCallConv(function_, LLVMWebKitJSCallConv);
        }
    }
    LLVMSetGC(function_, "statepoint-example");
    if (compCfg_->Is32Bit()) {
        slotSize_ = panda::ecmascript::FrameConstants::ARM32_SLOT_SIZE;
        slotType_ = LLVMInt32Type();
    } else {
        slotSize_ = panda::ecmascript::FrameConstants::AARCH64_SLOT_SIZE;
        slotType_ = LLVMInt64Type();
    }
    if (compCfg_->Is32Bit()) {
        // hard float instruction
        LLVMAddTargetDependentFunctionAttr(function_, "target-features", "+armv8-a");
    }
}

LLVMIRBuilder::~LLVMIRBuilder()
{
    if (builder_ != nullptr) {
        LLVMDisposeBuilder(builder_);
    }
}

int LLVMIRBuilder::FindBasicBlock(GateRef gate) const
{
    for (size_t bbIdx = 0; bbIdx < schedule_->size(); bbIdx++) {
        for (size_t instIdx = (*schedule_)[bbIdx].size(); instIdx > 0; instIdx--) {
            GateRef tmp = (*schedule_)[bbIdx][instIdx - 1];
            if (tmp == gate) {
                return bbIdx;
            }
        }
    }
    return -1;
}

void LLVMIRBuilder::AssignHandleMap()
{
    opCodeHandleMap_ = {
        {OpCode::STATE_ENTRY, &LLVMIRBuilder::HandleGoto},
        {OpCode::RETURN, &LLVMIRBuilder::HandleReturn},
        {OpCode::RETURN_VOID, &LLVMIRBuilder::HandleReturnVoid},
        {OpCode::IF_BRANCH, &LLVMIRBuilder::HandleBranch},
        {OpCode::SWITCH_BRANCH, &LLVMIRBuilder::HandleSwitch},
        {OpCode::ORDINARY_BLOCK, &LLVMIRBuilder::HandleGoto},
        {OpCode::IF_TRUE, &LLVMIRBuilder::HandleGoto},
        {OpCode::IF_FALSE, &LLVMIRBuilder::HandleGoto},
        {OpCode::SWITCH_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::MERGE, &LLVMIRBuilder::HandleGoto},
        {OpCode::DEFAULT_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::LOOP_BEGIN, &LLVMIRBuilder::HandleGoto},
        {OpCode::LOOP_BACK, &LLVMIRBuilder::HandleGoto},
        {OpCode::VALUE_SELECTOR, &LLVMIRBuilder::HandlePhi},
        {OpCode::RUNTIME_CALL, &LLVMIRBuilder::HandleRuntimeCall},
        {OpCode::CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::BYTECODE_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::ALLOCA, &LLVMIRBuilder::HandleAlloca},
        {OpCode::ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::CONSTANT, &LLVMIRBuilder::HandleConstant},
        {OpCode::RELOCATABLE_DATA, &LLVMIRBuilder::HandleRelocatableData},
        {OpCode::ZEXT_TO_INT16, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_TO_INT32, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_TO_INT64, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::SEXT_TO_INT32, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::SEXT_TO_INT64, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::TRUNC_TO_INT1, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::TRUNC_TO_INT32, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::REV, &LLVMIRBuilder::HandleIntRev},
        {OpCode::ADD, &LLVMIRBuilder::HandleAdd},
        {OpCode::SUB, &LLVMIRBuilder::HandleSub},
        {OpCode::MUL, &LLVMIRBuilder::HandleMul},
        {OpCode::FDIV, &LLVMIRBuilder::HandleFloatDiv},
        {OpCode::SDIV, &LLVMIRBuilder::HandleIntDiv},
        {OpCode::UDIV, &LLVMIRBuilder::HandleUDiv},
        {OpCode::AND, &LLVMIRBuilder::HandleIntAnd},
        {OpCode::OR, &LLVMIRBuilder::HandleIntOr},
        {OpCode::XOR, &LLVMIRBuilder::HandleIntXor},
        {OpCode::LSR, &LLVMIRBuilder::HandleIntLsr},
        {OpCode::SLT, &LLVMIRBuilder::HandleCmp},
        {OpCode::ULT, &LLVMIRBuilder::HandleCmp},
        {OpCode::SLE, &LLVMIRBuilder::HandleCmp},
        {OpCode::SGT, &LLVMIRBuilder::HandleCmp},
        {OpCode::SGE, &LLVMIRBuilder::HandleCmp},
        {OpCode::NE, &LLVMIRBuilder::HandleCmp},
        {OpCode::EQ, &LLVMIRBuilder::HandleCmp},
        {OpCode::LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::STORE, &LLVMIRBuilder::HandleStore},
        {OpCode::SIGNED_INT_TO_FLOAT, &LLVMIRBuilder::HandleChangeInt32ToDouble},
        {OpCode::FLOAT_TO_SIGNED_INT, &LLVMIRBuilder::HandleChangeDoubleToInt32},
        {OpCode::TAGGED_TO_INT64, &LLVMIRBuilder::HandleChangeTaggedPointerToInt64},
        {OpCode::INT64_TO_TAGGED, &LLVMIRBuilder::HandleChangeInt64ToTagged},
        {OpCode::BITCAST, &LLVMIRBuilder::HandleBitCast},
        {OpCode::LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::SMOD, &LLVMIRBuilder::HandleMod},
        {OpCode::FMOD, &LLVMIRBuilder::HandleMod},
    };
    opCodeHandleIgnore = {
        OpCode::NOP, OpCode::CIRCUIT_ROOT, OpCode::DEPEND_ENTRY,
        OpCode::FRAMESTATE_ENTRY, OpCode::RETURN_LIST, OpCode::THROW_LIST,
        OpCode::CONSTANT_LIST, OpCode::ARG_LIST, OpCode::THROW,
        OpCode::DEPEND_SELECTOR, OpCode::DEPEND_RELAY, OpCode::DEPEND_AND
    };
}

std::string LLVMIRBuilder::LLVMValueToString(LLVMValueRef val) const
{
    char* msg = LLVMPrintValueToString(val);
    std::string str(msg);
    LLVMDisposeMessage(msg);
    return str;
}

void LLVMIRBuilder::Build()
{
    COMPILER_LOG(INFO) << "LLVM IR Builder Create Id Map of Blocks...";
    for (size_t bbIdx = 0; bbIdx < schedule_->size(); bbIdx++) {
        for (size_t instIdx = (*schedule_)[bbIdx].size(); instIdx > 0; instIdx--) {
            GateId gateId = circuit_->GetId((*schedule_)[bbIdx][instIdx - 1]);
            instIdMapBbId_[gateId] = bbIdx;
        }
    }
    COMPILER_LOG(INFO) << "LLVM IR Builder Visit Gate...";

    AssignHandleMap();
    for (size_t bbIdx = 0; bbIdx < (*schedule_).size(); bbIdx++) {
        OperandsVector predecessors;
        for (auto in : circuit_->GetInVector((*schedule_)[bbIdx][0])) {
            if (!circuit_->GetOpCode(in).IsState()) {
                continue;
            }
            predecessors.insert(instIdMapBbId_[circuit_->GetId(in)]);
        }
        VisitBlock(bbIdx, predecessors);

        for (size_t instIdx = (*schedule_)[bbIdx].size(); instIdx > 0; instIdx--) {
            GateRef gate = (*schedule_)[bbIdx][instIdx - 1];
            std::vector<GateRef> ins = circuit_->GetInVector(gate);
            std::vector<GateRef> outs = circuit_->GetOutVector(gate);
#if ECMASCRIPT_ENABLE_COMPILER_LOG
            circuit_->Print(gate);
#endif
            auto found = opCodeHandleMap_.find(circuit_->GetOpCode(gate));
            if (found != opCodeHandleMap_.end()) {
                (this->*(found->second))(gate);
                continue;
            }
            if (opCodeHandleIgnore.find(circuit_->GetOpCode(gate)) == opCodeHandleIgnore.end()) {
                LOG_ECMA(ERROR) << "The gate below need to be translated ";
                circuit_->Print(gate);
                UNREACHABLE();
            }
        }
    }
    End();
}

BasicBlock *LLVMIRBuilder::EnsureBasicBlock(int id)
{
    BasicBlock *bb = nullptr;
    if (bbIdMapBb_.count(id) == 0) {
        auto newBB = std::make_unique<BasicBlock>(id);
        bb = newBB.get();
        bbIdMapBb_[id] = std::move(newBB);
    } else {
        bb = bbIdMapBb_[id].get();
    }
    return bb;
}

void LLVMIRBuilder::StartBuilder(BasicBlock *bb) const
{
    EnsureBasicBlock(bb);
    LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    if ((impl == nullptr) || (impl->llvm_bb_ == nullptr)) {
        COMPILER_LOG(ERROR) << "StartBuilder failed ";
        return;
    }
    impl->started = true;
    bb->SetImpl(impl);
    COMPILER_LOG(DEBUG) << "Basicblock id :" << bb->GetId() << "impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    LLVMPositionBuilderAtEnd(builder_, impl->llvm_bb_);
}

void LLVMIRBuilder::ProcessPhiWorkList()
{
    for (BasicBlock *bb : phiRebuildWorklist_) {
        auto impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
        for (auto &e : impl->not_merged_phis) {
            BasicBlock *pred = e.pred;
            if (impl->started == 0) {
                COMPILER_LOG(ERROR) << " ProcessPhiWorkList error hav't start ";
                return;
            }
            LLVMValueRef value = gateToLLVMMaps_[e.operand];
            if (LLVMTypeOf(value) != LLVMTypeOf(e.phi)) {
                COMPILER_LOG(ERROR) << " ProcessPhiWorkList LLVMTypeOf don't match error ";
            }
            LLVMBasicBlockRef llvmBB = EnsureBasicBlock(pred);
            LLVMAddIncoming(e.phi, &value, &llvmBB, 1);
        }
        impl->not_merged_phis.clear();
    }
    phiRebuildWorklist_.clear();
}

void LLVMIRBuilder::EndCurrentBlock() const
{
    LLVMTFBuilderBasicBlockImpl *impl = currentBb_->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->ended = true;
}

void LLVMIRBuilder::End()
{
    ASSERT(!!currentBb_);
    EndCurrentBlock();
    ProcessPhiWorkList();
    for (auto &it : bbIdMapBb_) {
        it.second->ResetImpl<LLVMTFBuilderBasicBlockImpl>();
    }
}

LLVMTFBuilderBasicBlockImpl *LLVMIRBuilder::EnsureBasicBlockImpl(BasicBlock *bb) const
{
    if (bb->GetImpl<LLVMTFBuilderBasicBlockImpl>()) {
        return bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    }
    auto impl = std::make_unique<LLVMTFBuilderBasicBlockImpl>();
    bb->SetImpl(impl.release());
    return bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
}

void LLVMIRBuilder::GenPrologue(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* current frame for x86_64 system:
    for optimized entry frame
         0    pre   <-- rbp register
        -8    type
        -16   current sp before call other function
        -24   pre frame thread fp
    for optimized frame
         0    pre rbp  <-- rbp
        -8    type
        -16   current sp before call other function

     for 32 arm bit system:
        not support
     for arm64 system
     for optimized entry frame
         0    pre rbp  <-- x29 register
        -8    type
        -16   current sp before call other function
        -24   pre frame thread fp
    for optimized frame
        0    pre rbp  <-- rbp
        -8    type
        -16   current sp before call other function
    */
    if (compCfg_->Is32Bit()) {
        return;
    }
    auto frameType = circuit_->GetFrameType();
    if (frameType != FrameType::OPTIMIZED_FRAME) {
        return;
    }
    LLVMAddTargetDependentFunctionAttr(function_, "frame-pointer", "all");

    LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, false);

    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, llvmFpAddr, slotType_, "cast_int_t");
    LLVMValueRef frameTypeSlotAddr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(slotType_,
        slotSize_, false), "");
    LLVMValueRef addr = LLVMBuildIntToPtr(builder, frameTypeSlotAddr,
        LLVMPointerType(slotType_, 0), "frameType.Addr");

    if (frameType == panda::ecmascript::FrameType::OPTIMIZED_FRAME) {
        LLVMAddTargetDependentFunctionAttr(function_, "js-stub-call", "0");
    } else if (frameType == panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME) {
        LLVMAddTargetDependentFunctionAttr(function_, "js-stub-call", "1");
    } else {
        LOG_ECMA(FATAL) << "frameType interpret type error !";
        ASSERT_PRINT(static_cast<uintptr_t>(frameType), "is not support !");
    }

    LLVMValueRef llvmFrameType = LLVMConstInt(slotType_, static_cast<uintptr_t>(frameType), 0);
    LLVMValueRef value = LLVMBuildStore(builder_, llvmFrameType, addr);

    if (frameType != panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME) {
        return;
    }

    LLVMValueRef glue = LLVMGetParam(function_, 0);
    LLVMTypeRef glue_type = LLVMTypeOf(glue);
    LLVMValueRef rtoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::CURRENT_FRAME), 0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(slotType_, 0), "");
    LLVMValueRef threadFpValue = LLVMBuildLoad(builder_, rtbaseAddr, "");
    addr = LLVMBuildAdd(builder, frameAddr, LLVMConstInt(slotType_,
        FrameConstants::INTERPER_FRAME_FP_TO_FP_DELTA * slotSize_, true), "");
    value = LLVMBuildStore(builder_, threadFpValue,
        LLVMBuildIntToPtr(builder_, addr, LLVMPointerType(slotType_, 0), "cast"));
    LOG_ECMA(DEBUG) << "store value:" << value << " "
                << "value type" << LLVMTypeOf(value);
}

LLVMValueRef LLVMIRBuilder::CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder, bool isCaller)
{
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, isCaller)};
    auto fn = LLVMGetNamedFunction(module, "llvm.frameaddress.p0i8");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = {
            LLVMInt32Type(),
        };
        auto fnTy = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), paramTys1, 1, 0);
        fn = LLVMAddFunction(module, "llvm.frameaddress.p0i8", fnTy);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    return fAddrRet;
}

LLVMValueRef LLVMIRBuilder::ReadRegister(LLVMModuleRef &module, LLVMBuilderRef &builder,
    LLVMMetadataRef meta)
{
    std::vector<LLVMValueRef> args = {LLVMMetadataAsValue(context_, meta)};
    auto fn = LLVMGetNamedFunction(module, "llvm.read_register.i64");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = {
            GetMachineRepType(MachineRep::K_META),
        };
        auto fnTy = LLVMFunctionType(LLVMInt64Type(), paramTys1, 1, 0);
        fn = LLVMAddFunction(module, "llvm.read_register.i64", fnTy);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder_, fn, args.data(), 1, "");
    return fAddrRet;
}

LLVMBasicBlockRef LLVMIRBuilder::EnsureBasicBlock(BasicBlock *bb) const
{
    LLVMTFBuilderBasicBlockImpl *impl = EnsureBasicBlockImpl(bb);
    if (impl->llvm_bb_) {
        return impl->llvm_bb_;
    }

    std::string buf = "B" + std::to_string(bb->GetId());
    LLVMBasicBlockRef llvmBB = LLVMAppendBasicBlock(function_, buf.c_str());
    impl->llvm_bb_ = llvmBB;
    impl->continuation = llvmBB;
    bb->SetImpl(impl);
    COMPILER_LOG(DEBUG) << "create LLVMBB = " << buf << " impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    return llvmBB;
}

LLVMTypeRef LLVMIRBuilder::GetMachineRepType(MachineRep rep) const
{
    LLVMTypeRef dstType;
    switch (rep) {
        case MachineRep::K_BIT:
            dstType = LLVMInt1TypeInContext(context_);
            break;
        case MachineRep::K_WORD8:
            dstType = LLVMInt8TypeInContext(context_);
            break;
        case MachineRep::K_WORD16:
            dstType = LLVMInt16TypeInContext(context_);
            break;
        case MachineRep::K_WORD32:
            dstType = LLVMInt32TypeInContext(context_);
            break;
        case MachineRep::K_FLOAT64:
            dstType = LLVMDoubleTypeInContext(context_);
            break;
        case MachineRep::K_WORD64:
            dstType = LLVMInt64TypeInContext(context_);
            break;
        case MachineRep::K_PTR_1:
            if (compCfg_->Is32Bit()) {
                dstType = LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2); // 2: packed vector type
            } else {
                dstType = LLVMPointerType(LLVMInt64TypeInContext(context_), 1);
            }
            break;
        case MachineRep::K_META:
            dstType = LLVMMetadataTypeInContext(context_);
            break;
        default:
            UNREACHABLE();
            break;
    }
    return dstType;
}

void LLVMIRBuilder::HandleCall(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    if (circuit_->GetOpCode(gate) == OpCode::CALL) {
        VisitCall(gate, ins);
    } else {
        VisitBytecodeCall(gate, ins);
    }
}

void LLVMIRBuilder::HandleRuntimeCall(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitRuntimeCall(gate, ins);
}

void LLVMIRBuilder::VisitRuntimeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    int paraStartIndex = 3;
    ASSERT(stubModule_ != nullptr);
    LLVMValueRef callee;
    LLVMValueRef rtoffset;
    LLVMTypeRef rtfuncType = stubModule_->GetStubFunctionType(FAST_STUB_ID(RuntimeCallTrampoline));
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
    LLVMValueRef glue = gateToLLVMMaps_[inList[2]];  // 2 : 2 means skip two input gates (target glue)
    LLVMTypeRef glue_type = LLVMTypeOf(glue);
    rtoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::RUNTIME_FUNCTIONS) +
                                (FAST_STUB_ID(RuntimeCallTrampoline) - FAST_STUB_MAXCOUNT) * slotSize_, 0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(glue_type, 0), "");
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, "");
    callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, "cast");
    // 16 : params limit
    LLVMValueRef params[16];
    params[0] = glue;
    int index = circuit_->GetBitField(inList[1]);
    params[1] = LLVMConstInt(LLVMInt64Type(), index - FAST_STUB_MAXCOUNT, 0);
    params[2] = LLVMConstInt(LLVMInt64Type(), 2882400000, 0); // 2882400000: default statepoint ID
    params[3] = LLVMConstInt(LLVMInt64Type(), inList.size() - paraStartIndex, 0); // 3 : 3 means fourth parameter
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params[paraIdx + 1] = gateToLLVMMaps_[gateTmp];
    }
    if (callee == nullptr) {
        COMPILER_LOG(ERROR) << "callee nullptr";
        return;
    }
    LLVMValueRef runtimeCall = LLVMBuildCall(builder_, callee, params, inList.size() + 1, "runtime_call");
    if (!compCfg_->Is32Bit()) {  // Arm32 not support webkit jscc calling convention
        LLVMSetInstructionCallConv(runtimeCall, LLVMWebKitJSCallConv);
    }
    gateToLLVMMaps_[gate] = runtimeCall;
}

LLVMValueRef LLVMIRBuilder::GetCurrentSP()
{
    LLVMMetadataRef meta;
    if (compCfg_->IsAmd64()) {
        meta = LLVMMDStringInContext2(context_, "rsp", 4);   // 4 : 4 means len of "rsp"
    } else {
        meta = LLVMMDStringInContext2(context_, "sp", 3);   // 3 : 3 means len of "sp"
    }
    LLVMMetadataRef metadataNode = LLVMMDNodeInContext2(context_, &meta, 1);
    LLVMValueRef spValue = ReadRegister(module_, builder_, metadataNode);
    return spValue;
}

void LLVMIRBuilder::SaveCurrentSP()
{
    if (circuit_->GetFrameType() != FrameType::OPTIMIZED_FRAME) {
        return;
    }
    if (compCfg_->Is64Bit()) {
        LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, false);
        LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder_, llvmFpAddr, slotType_, "cast_int_t");
        LLVMValueRef frameSpSlotAddr = LLVMBuildSub(builder_, frameAddr, LLVMConstInt(slotType_,
            3 * slotSize_, false), ""); // 3: type + threadsp + current sp
        LLVMValueRef addr = LLVMBuildIntToPtr(builder_, frameSpSlotAddr,
                                              LLVMPointerType(slotType_, 0), "frameCallSiteSP.Addr");
        LLVMMetadataRef meta;
        if (compCfg_->IsAmd64()) {
            meta = LLVMMDStringInContext2(context_, "rsp", 4);   // 4 : 4 means len of "rsp"
        } else {
            meta = LLVMMDStringInContext2(context_, "sp", 3);   // 3 : 3 means len of "sp"
        }
        LLVMMetadataRef metadataNode = LLVMMDNodeInContext2(context_, &meta, 1);
        LLVMValueRef spValue = ReadRegister(module_, builder_, metadataNode);
        LLVMBuildStore(builder_, spValue, addr);
    }
}

LLVMValueRef LLVMIRBuilder::GetCurrentSPFrameAddr()
{
    LLVMValueRef glue = LLVMGetParam(function_, 0);
    LLVMTypeRef glue_type = LLVMTypeOf(glue);
    LLVMValueRef rtoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::CURRENT_FRAME), 0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
    return rtbaseoffset;
}

LLVMValueRef LLVMIRBuilder::GetCurrentSPFrame()
{
    LLVMValueRef addr = GetCurrentSPFrameAddr();
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, addr, LLVMPointerType(slotType_, 0), "");
    LLVMValueRef currentSpFrame = LLVMBuildLoad(builder_, rtbaseAddr, "");
    return currentSpFrame;
}

void LLVMIRBuilder::SetCurrentSPFrame(LLVMValueRef sp)
{
    LLVMValueRef addr = GetCurrentSPFrameAddr();
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, addr, LLVMPointerType(slotType_, 0), "");
    LLVMBuildStore(builder_, sp, rtbaseAddr);
}

LLVMValueRef LLVMIRBuilder::GetCurrentFrameType(LLVMValueRef currentSpFrameAddr)
{
    LLVMValueRef tmp = LLVMBuildSub(builder_, currentSpFrameAddr, LLVMConstInt(slotType_, slotSize_, 1), "");
    LLVMValueRef frameTypeAddr = LLVMBuildIntToPtr(builder_, tmp, LLVMPointerType(LLVMInt64Type(), 0), "");
    LLVMValueRef frameType = LLVMBuildLoad(builder_, frameTypeAddr, "");
    return frameType;
}

void LLVMIRBuilder::VisitCall(GateRef gate, const std::vector<GateRef> &inList)
{
    int paraStartIndex = 3;
    int index = circuit_->GetBitField(inList[1]);
    ASSERT(stubModule_ != nullptr);
    LLVMValueRef callee;
    LLVMValueRef rtoffset;
    StubDescriptor *calleeDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(index);
    LLVMTypeRef rtfuncType = stubModule_->GetStubFunctionType(index);
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
    LLVMValueRef glue = gateToLLVMMaps_[inList[2]];  // 2 : 2 means skip two input gates (target glue)
    LLVMTypeRef glue_type = LLVMTypeOf(glue);
    // runtime case
    if (calleeDescriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB ||
        calleeDescriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB_NO_GC) {
        rtoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::RUNTIME_FUNCTIONS) +
                                (index - FAST_STUB_MAXCOUNT) * slotSize_, 0);
    } else {
        rtoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::FAST_STUB_ENTRIES) +
                                index * slotSize_, 0);
    }
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(glue_type, 0), "");
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, "");
    callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, "cast");
    // 16 : function params max limit
    LLVMValueRef params[16];
    int extraParameterCnt = 0;

    size_t dstParaIndex = 0;
    // first argument is glue
    GateRef glueGate = inList[paraStartIndex];
    params[dstParaIndex++] = gateToLLVMMaps_[glueGate];
    // for arm32, r0-r4 must be occupied by fake parameters, then the actual paramters will be in stack.
    if (compCfg_->Is32Bit() && calleeDescriptor->GetStubKind() != StubDescriptor::CallStubKind::RUNTIME_STUB) {
        for (int i = 0; i < reservedSlotForArm32; i++) {
            params[dstParaIndex++] = gateToLLVMMaps_[glueGate];
        }
        extraParameterCnt += reservedSlotForArm32;
    }
    // then push the actual parameter for js function call
    for (size_t paraIdx = paraStartIndex + 1; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params[dstParaIndex++] = gateToLLVMMaps_[gateTmp];
    }
    if (callee == nullptr) {
        COMPILER_LOG(ERROR) << "callee nullptr";
        return;
    }
    if (compCfg_->Is32Bit() || compCfg_->Is64Bit()) {
        SaveCurrentSP();
    }

    gateToLLVMMaps_[gate] = LLVMBuildCall(builder_, callee, params,
        inList.size() - paraStartIndex + extraParameterCnt, "");
    return;
}

void LLVMIRBuilder::VisitBytecodeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    int paraStartIndex = 3;
    LLVMValueRef opcodeOffset = gateToLLVMMaps_[inList[1]];
    ASSERT(stubModule_ != nullptr);
    LLVMValueRef callee;
    LLVMTypeRef rtfuncType = stubModule_->GetStubFunctionType(CallStubId::NAME_BytecodeHandler);
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
    LLVMValueRef glue = gateToLLVMMaps_[inList[2]];  // 2 : 2 means skip two input gates (target glue)
    LLVMTypeRef glue_type = LLVMTypeOf(glue);
    LLVMValueRef bytecodeoffset = LLVMConstInt(
        glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::BYTECODE_HANDLERS), 0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(
        builder_, glue, LLVMBuildAdd(builder_, bytecodeoffset, opcodeOffset, ""), "");
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(glue_type, 0), "");
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, "");
    callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, "cast");
    // 16 : params limit
    LLVMValueRef params[16];
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params[paraIdx - paraStartIndex] = gateToLLVMMaps_[gateTmp];
    }
    LLVMValueRef call = LLVMBuildCall(builder_, callee, params, inList.size() - paraStartIndex, "");
    LLVMSetTailCall(call, true);
    const char *attrName = "gc-leaf-function";
    const char *attrValue = "true";
    LLVMAttributeRef llvmAttr = LLVMCreateStringAttribute(context_, attrName, strlen(attrName), attrValue,
                                                          strlen(attrValue));
    LLVMAddCallSiteAttribute(call, LLVMAttributeFunctionIndex, llvmAttr);
    LLVMSetInstructionCallConv(call, LLVMGHCCallConv);
    gateToLLVMMaps_[gate] = call;
}

void LLVMIRBuilder::HandleAlloca(GateRef gate)
{
    return VisitAlloca(gate);
}

void LLVMIRBuilder::VisitAlloca(GateRef gate)
{
    uint64_t machineRep = circuit_->GetBitField(gate);
    LLVMTypeRef dataType = GetMachineRepType(static_cast<MachineRep>(machineRep));
    COMPILER_LOG(DEBUG) << LLVMPrintTypeToString(dataType);
    COMPILER_LOG(DEBUG) << LLVMPrintTypeToString(ConvertLLVMTypeFromGate(gate));
    gateToLLVMMaps_[gate] = LLVMBuildPtrToInt(builder_, LLVMBuildAlloca(builder_, dataType, ""),
                                              ConvertLLVMTypeFromGate(gate), "");
}

void LLVMIRBuilder::HandlePhi(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitPhi(gate, ins);
}

void LLVMIRBuilder::VisitPhi(GateRef gate, const std::vector<GateRef> &srcGates)
{
    LLVMTypeRef type = ConvertLLVMTypeFromGate(gate);
    LLVMValueRef phi = LLVMBuildPhi(builder_, type, "");
    std::vector<GateRef> relMergeIns = circuit_->GetInVector(srcGates[0]);
    bool addToPhiRebuildList = false;
    for (int i = 1; i < static_cast<int>(srcGates.size()); i++) {
        GateId gateId = circuit_->GetId(relMergeIns[i - 1]);
        int bbIdx = instIdMapBbId_[gateId];
        COMPILER_LOG(DEBUG) << "srcGate: " << srcGates[i] << " dominated gateId:" << gateId << "dominated bbIdx: " <<
            bbIdx;
        int cnt = bbIdMapBb_.count(bbIdx);
        // if cnt = 0 means bb with current bbIdx hasn't been created
        if (cnt > 0) {
            BasicBlock *bb = bbIdMapBb_[bbIdx].get();
            COMPILER_LOG(DEBUG) << "bb : " << bb;
            if (bb == nullptr) {
                COMPILER_LOG(ERROR) << "VisitPhi failed BasicBlock nullptr";
                return;
            }
            LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
            if (impl == nullptr) {
                COMPILER_LOG(ERROR) << "VisitPhi failed impl nullptr";
                return;
            }
            LLVMBasicBlockRef llvmBB = EnsureBasicBlock(bb);  // The llvm bb
            LLVMValueRef value = gateToLLVMMaps_[srcGates[i]];

            if (impl->started) {
                LLVMAddIncoming(phi, &value, &llvmBB, 1);
            } else {
                addToPhiRebuildList = true;
                impl = currentBb_->GetImpl<LLVMTFBuilderBasicBlockImpl>();
                impl->not_merged_phis.emplace_back();
                auto &not_merged_phi = impl->not_merged_phis.back();
                not_merged_phi.phi = phi;
                not_merged_phi.pred = bb;
                not_merged_phi.operand = srcGates[i];
            }
        } else {
            addToPhiRebuildList = true;
        }
        if (addToPhiRebuildList == true) {
            phiRebuildWorklist_.push_back(currentBb_);
        }
        gateToLLVMMaps_[gate] = phi;
    }
}

void LLVMIRBuilder::VisitReturn(GateRef gate, GateRef popCount, const std::vector<GateRef> &operands)
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    GateRef operand = operands[2];  // 2: skip 2 in gate that are not data gate
    COMPILER_LOG(DEBUG) << " gate: " << gate << " popCount: " << popCount;
    COMPILER_LOG(DEBUG) << " return: " << operand << " gateId: " << circuit_->GetId(operand);
    LLVMValueRef returnValue = gateToLLVMMaps_[operand];
    COMPILER_LOG(DEBUG) << LLVMValueToString(returnValue);
    LLVMBuildRet(builder_, returnValue);
}

void LLVMIRBuilder::HandleReturn(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitReturn(gate, 1, ins);
}

void LLVMIRBuilder::VisitReturnVoid(GateRef gate)
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    COMPILER_LOG(DEBUG) << " gate: " << gate;
    LLVMBuildRetVoid(builder_);
}

void LLVMIRBuilder::HandleReturnVoid(GateRef gate)
{
    VisitReturnVoid(gate);
}

void LLVMIRBuilder::VisitBlock(int gate, const OperandsVector &predecessors)  // NOLINTNEXTLINE(misc-unused-parameters)
{
    COMPILER_LOG(DEBUG) << " BBIdx:" << gate;
    BasicBlock *bb = EnsureBasicBlock(gate);
    if (bb == nullptr) {
        COMPILER_LOG(ERROR) << " block create failed ";
        return;
    }
    currentBb_ = bb;
    LLVMBasicBlockRef llvmbb = EnsureBasicBlock(bb);
    StartBuilder(bb);
    COMPILER_LOG(DEBUG) << "predecessors :";
    for (int predecessor : predecessors) {
        BasicBlock *pre = EnsureBasicBlock(predecessor);
        if (pre == nullptr) {
            COMPILER_LOG(ERROR) << " block setup failed, predecessor:%d nullptr" << predecessor;
            return;
        }
        LLVMBasicBlockRef llvmpre = EnsureBasicBlock(pre);
        COMPILER_LOG(DEBUG) << "  " << predecessor;
        LLVMMoveBasicBlockBefore(llvmpre, llvmbb);
    }
    if (gate == 0) { // insert prologue
        GenPrologue(module_, builder_);
    }
}

void LLVMIRBuilder::HandleGoto(GateRef gate)
{
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    int block = instIdMapBbId_[circuit_->GetId(gate)];
    int bbOut = instIdMapBbId_[circuit_->GetId(outs[0])];
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::MERGE:
        case OpCode::LOOP_BEGIN: {
            for (int i = 0; i < static_cast<int>(outs.size()); i++) {
                bbOut = instIdMapBbId_[circuit_->GetId(outs[i])];
                VisitGoto(block, bbOut);
            }
            break;
        }
        default: {
            VisitGoto(block, bbOut);
            break;
        }
    }
}

void LLVMIRBuilder::VisitGoto(int block, int bbOut)
{
    if (block == bbOut) {
        return;
    }
    BasicBlock *bb = EnsureBasicBlock(bbOut);
    if (bb == nullptr) {
        COMPILER_LOG(ERROR) << " block is nullptr ";
        return;
    }
    llvm::BasicBlock *self = llvm::unwrap(EnsureBasicBlock(bbIdMapBb_[block].get()));
    llvm::BasicBlock *out = llvm::unwrap(EnsureBasicBlock(bbIdMapBb_[bbOut].get()));
    llvm::BranchInst::Create(out, self);
    EndCurrentBlock();
}

void LLVMIRBuilder::HandleConstant(GateRef gate)
{
    std::bitset<64> value = circuit_->GetBitField(gate); // 64: bit width
    VisitConstant(gate, value);
}

void LLVMIRBuilder::VisitConstant(GateRef gate, std::bitset<64> value) // 64: bit width
{
    LLVMValueRef llvmValue = nullptr;
    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    if (machineType == MachineType::I32) {
        llvmValue = LLVMConstInt(LLVMInt32Type(), value.to_ulong(), 0);
    } else if (machineType == MachineType::I64) {
        llvmValue = LLVMConstInt(LLVMInt64Type(), value.to_ullong(), 0);
        LLVMTypeRef type = ConvertLLVMTypeFromGate(gate);
        if (LLVMGetTypeKind(type) == LLVMPointerTypeKind) {
            llvmValue = LLVMBuildIntToPtr(builder_, llvmValue, type, "");
        } else if (LLVMGetTypeKind(type) == LLVMVectorTypeKind) {
            LLVMValueRef tmp1Value =
                    LLVMBuildLShr(builder_, llvmValue, LLVMConstInt(LLVMInt64Type(), 32, 0), ""); // 32: offset
            LLVMValueRef tmp2Value = LLVMBuildIntCast(builder_, llvmValue, LLVMInt32Type(), ""); // low
            LLVMValueRef emptyValue = LLVMGetUndef(type);
            tmp1Value = LLVMBuildIntToPtr(builder_, tmp1Value, LLVMPointerType(LLVMInt8Type(), 1), "");
            tmp2Value = LLVMBuildIntToPtr(builder_, tmp2Value, LLVMPointerType(LLVMInt8Type(), 1), "");
            llvmValue = LLVMBuildInsertElement(
                builder_, emptyValue, tmp2Value, LLVMConstInt(LLVMInt32Type(), 0, 0), "");
            llvmValue = LLVMBuildInsertElement(builder_, llvmValue, tmp1Value, LLVMConstInt(LLVMInt32Type(), 1, 0), "");
        } else if (LLVMGetTypeKind(type) == LLVMIntegerTypeKind) {
            // do nothing
        } else {
            abort();
        }
    } else if (machineType == MachineType::F64) {
        auto doubleValue = bit_cast<double>(value.to_ullong()); // actual double value
        llvmValue = LLVMConstReal(LLVMDoubleType(), doubleValue);
    } else if (machineType == MachineType::I8) {
        llvmValue = LLVMConstInt(LLVMInt8Type(), value.to_ulong(), 0);
    } else if (machineType == MachineType::I16) {
        llvmValue = LLVMConstInt(LLVMInt16Type(), value.to_ulong(), 0);
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = llvmValue;
    COMPILER_LOG(DEBUG) << "VisitConstant set gate:" << gate << "  value:" << value;
    COMPILER_LOG(DEBUG) << "VisitConstant " << LLVMValueToString(llvmValue);
}

void LLVMIRBuilder::HandleRelocatableData(GateRef gate)
{
    uint64_t value = circuit_->GetBitField(gate);
    VisitRelocatableData(gate, value);
}

void LLVMIRBuilder::VisitRelocatableData(GateRef gate, uint64_t value)
{
    LLVMValueRef globalValue = LLVMAddGlobal(module_, LLVMInt64Type(), "G");
    LLVMSetInitializer(globalValue, LLVMConstInt(LLVMInt64Type(), value, 0));
    gateToLLVMMaps_[gate] = globalValue;
    COMPILER_LOG(DEBUG) << "VisitRelocatableData set gate:" << gate << "  value:" << value;
    COMPILER_LOG(DEBUG) << "VisitRelocatableData " << LLVMValueToString(globalValue);
}

void LLVMIRBuilder::HandleZExtInt(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitZExtInt(gate, ins[0]);
}

void LLVMIRBuilder::HandleSExtInt(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitSExtInt(gate, ins[0]);
}

void LLVMIRBuilder::HandleParameter(GateRef gate)
{
    return VisitParameter(gate);
}

void LLVMIRBuilder::VisitParameter(GateRef gate)
{
    int argth = circuit_->LoadGatePtrConst(gate)->GetBitField();
    COMPILER_LOG(DEBUG) << " Parameter value" << argth;
    if (compCfg_->Is32Bit() && argth > 0) {
        argth += reservedSlotForArm32;
    }
    LLVMValueRef value = LLVMGetParam(function_, argth);

    ASSERT(LLVMTypeOf(value) == ConvertLLVMTypeFromGate(gate));
    gateToLLVMMaps_[gate] = value;
    COMPILER_LOG(DEBUG) << "VisitParameter set gate:" << gate << "  value:" << value;
    // NOTE: caller put args, otherwise crash
    if (value == nullptr) {
        COMPILER_LOG(FATAL) << "generate LLVM IR for para: " << argth << "fail";
        return;
    }
    COMPILER_LOG(DEBUG) << "para arg:" << argth << "value IR:" << LLVMValueToString(value);
}

void LLVMIRBuilder::HandleBranch(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    GateRef bTrue = (circuit_->GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
    GateRef bFalse = (circuit_->GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
    int bbTrue = instIdMapBbId_[circuit_->GetId(bTrue)];
    int bbFalse = instIdMapBbId_[circuit_->GetId(bFalse)];
    VisitBranch(gate, ins[1], bbTrue, bbFalse);
}

void LLVMIRBuilder::HandleMod(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitMod(gate, g0, g1);
}

void LLVMIRBuilder::VisitMod(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "mod gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = nullptr;
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e1));
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e2));
    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    if (machineType == MachineType::I32) {
        result = LLVMBuildSRem(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFRem(builder_, e1Value, e2Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitBranch(GateRef gate, GateRef cmp, int btrue, int bfalse)
{
    COMPILER_LOG(DEBUG) << "cmp gate:" << cmp;
    if (gateToLLVMMaps_.count(cmp) == 0) {
        COMPILER_LOG(ERROR) << "Branch condition gate is nullptr!";
        return;
    }
    LLVMValueRef cond = gateToLLVMMaps_[cmp];

    BasicBlock *trueBB = EnsureBasicBlock(btrue);
    BasicBlock *falseBB = EnsureBasicBlock(bfalse);
    EnsureBasicBlock(trueBB);
    EnsureBasicBlock(falseBB);

    LLVMBasicBlockRef llvmTrueBB = trueBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMBasicBlockRef llvmFalseBB = falseBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMValueRef result = LLVMBuildCondBr(builder_, cond, llvmTrueBB, llvmFalseBB);
    EndCurrentBlock();
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::HandleSwitch(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitSwitch(gate, ins[1], outs);
}

void LLVMIRBuilder::VisitSwitch(GateRef gate, GateRef input, const std::vector<GateRef> &outList)
{
    LLVMValueRef cond = gateToLLVMMaps_[input];
    unsigned caseNum = outList.size();
    BasicBlock *curOutBB = nullptr;
    LLVMBasicBlockRef llvmDefaultOutBB = nullptr;
    for (int i = 0; i < static_cast<int>(caseNum); i++) {
        curOutBB = EnsureBasicBlock(instIdMapBbId_[circuit_->GetId(outList[i])]);
        EnsureBasicBlock(curOutBB);
        if (circuit_->GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            llvmDefaultOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        }
    }
    LLVMValueRef result = LLVMBuildSwitch(builder_, cond, llvmDefaultOutBB, caseNum - 1);
    LLVMBasicBlockRef llvmCurOutBB = nullptr;
    for (int i = 0; i < static_cast<int>(caseNum); i++) {
        if (circuit_->GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        curOutBB = EnsureBasicBlock(instIdMapBbId_[circuit_->GetId(outList[i])]);
        llvmCurOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        LLVMAddCase(result, LLVMConstInt(ConvertLLVMTypeFromGate(input), circuit_->GetBitField(outList[i]), 0),
                    llvmCurOutBB);
    }
    EndCurrentBlock();
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::VisitLoad(GateRef gate, GateRef base)
{
    COMPILER_LOG(DEBUG) << "Load base gate:" << base;
    LLVMValueRef baseAddr = gateToLLVMMaps_[base];
    LLVMTypeRef returnType;
    baseAddr = CanonicalizeToPtr(baseAddr);
    returnType = ConvertLLVMTypeFromGate(gate);
    baseAddr = LLVMBuildPointerCast(builder_, baseAddr,
        LLVMPointerType(returnType, LLVMGetPointerAddressSpace(LLVMTypeOf(baseAddr))), "");
    LLVMValueRef result = LLVMBuildLoad(builder_, baseAddr, "");
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::VisitStore(GateRef gate, GateRef base, GateRef dataToStore)
{
    COMPILER_LOG(DEBUG) << "store base gate:" << base;
    LLVMValueRef baseAddr = gateToLLVMMaps_[base];
    baseAddr = CanonicalizeToPtr(baseAddr);
    LLVMValueRef data = gateToLLVMMaps_[dataToStore];
    baseAddr = LLVMBuildPointerCast(builder_, baseAddr,
        LLVMPointerType(ConvertLLVMTypeFromGate(dataToStore), LLVMGetPointerAddressSpace(LLVMTypeOf(baseAddr))), "");
    LLVMValueRef value = LLVMBuildStore(builder_, data, baseAddr);
    gateToLLVMMaps_[gate] = value;
    COMPILER_LOG(DEBUG) << "store value:" << value << " " << "value type" << LLVMTypeOf(value);
}

LLVMValueRef LLVMIRBuilder::CanonicalizeToInt(LLVMValueRef value)
{
    if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMVectorTypeKind) {
        LLVMValueRef e1Value0 = LLVMBuildExtractElement(builder_, value, LLVMConstInt(LLVMInt32Type(), 0, 1), "");
        LLVMValueRef e1Value1 = LLVMBuildExtractElement(builder_, value, LLVMConstInt(LLVMInt32Type(), 1, 1), "");
        LLVMValueRef tmp1 = LLVMBuildPtrToInt(builder_, e1Value1, LLVMInt64Type(), "");
        LLVMValueRef constValue = LLVMConstInt(LLVMInt64Type(), 32, 0); // 32: offset
        LLVMValueRef tmp1Value = LLVMBuildShl(builder_, tmp1, constValue, "");
        LLVMValueRef tmp2Value = LLVMBuildPtrToInt(builder_, e1Value0, LLVMInt64Type(), "");
        LLVMValueRef resultValue = LLVMBuildAdd(builder_, tmp1Value, tmp2Value, "");
        return resultValue;
    } else if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind) {
        return LLVMBuildPtrToInt(builder_, value, LLVMInt64Type(), "");
    } else if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMIntegerTypeKind) {
        return value;
    } else {
        COMPILER_LOG(DEBUG) << "can't Canonicalize to Int64: ";
        abort();
    }
}

LLVMValueRef LLVMIRBuilder::CanonicalizeToPtr(LLVMValueRef value)
{
    if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMVectorTypeKind) {
        LLVMValueRef tmp = LLVMBuildExtractElement(builder_, value, LLVMConstInt(LLVMInt32Type(), 0, 1), "");
        return LLVMBuildPointerCast(builder_, tmp, LLVMPointerType(LLVMInt8Type(), 1), "");
    } else if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind) {
        return LLVMBuildPointerCast(builder_, value,
            LLVMPointerType(LLVMInt8Type(), LLVMGetPointerAddressSpace(LLVMTypeOf(value))), "");
    } else if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMIntegerTypeKind) {
        LLVMValueRef tmp = LLVMBuildIntToPtr(builder_, value, LLVMPointerType(LLVMInt64Type(), 0), "");
        return LLVMBuildPointerCast(builder_, tmp, LLVMPointerType(LLVMInt8Type(), 0), "");
    } else {
        COMPILER_LOG(DEBUG) << "can't Canonicalize to Ptr: ";
        abort();
    }
}

void LLVMIRBuilder::HandleIntRev(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitIntRev(gate, ins[0]);
}

void LLVMIRBuilder::VisitIntRev(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int sign invert gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e1));
    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    LLVMValueRef result = nullptr;
    if (machineType <= MachineType::I64 && machineType >= MachineType::I1) {
        result = LLVMBuildNot(builder_, e1Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

LLVMValueRef LLVMIRBuilder::PointerAdd(LLVMValueRef baseAddr, LLVMValueRef offset, LLVMTypeRef rep)
{
    LLVMValueRef ptr = CanonicalizeToPtr(baseAddr);
    LLVMValueRef dstRef8 = LLVMBuildGEP(builder_, ptr, &offset, 1, "");
    LLVMValueRef result = LLVMBuildPointerCast(builder_, dstRef8, rep, "");
    return result;
}

LLVMValueRef LLVMIRBuilder::VectorAdd(LLVMValueRef baseAddr, LLVMValueRef offset, LLVMTypeRef rep)
{
    LLVMValueRef ptr = CanonicalizeToPtr(baseAddr);
    LLVMValueRef dstRef8 = LLVMBuildGEP(builder_, ptr, &offset, 1, "");
    LLVMValueRef result = LLVMBuildInsertElement(builder_, baseAddr, dstRef8, LLVMConstInt(LLVMInt32Type(), 0, 0), "");
    return result;
}

bool LLVMIRBuilder::IsGCRelated(GateType typeCode) const
{
    if ((typeCode & (~GC_MASK)) == 0) {
        return true;
    }
    return false;
}

LLVMTypeRef LLVMIRBuilder::ConvertLLVMTypeFromGate(GateRef gate) const
{
    if (IsGCRelated(circuit_->GetGateType(gate))) {
        if (compCfg_->Is32Bit()) {
            return LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2);
        } else {
            return LLVMPointerType(LLVMInt64Type(), 1);
        }
    }
    switch (circuit_->LoadGatePtrConst(gate)->GetMachineType()) {
        case MachineType::NOVALUE:
            return LLVMVoidType();
        case MachineType::I1:
            return LLVMInt1Type();
        case MachineType::I8:
            return LLVMInt8Type();
        case MachineType::I16:
            return LLVMInt16Type();
        case MachineType::I32:
            return LLVMInt32Type();
        case MachineType::I64:
            return LLVMInt64Type();
        case MachineType::F32:
            return LLVMFloatType();
        case MachineType::F64:
            return LLVMDoubleType();
        case MachineType::ARCH: {
            if (compCfg_->Is32Bit()) {
                return LLVMInt32Type();
            } else {
                return LLVMInt64Type();
            }
        }
        default:
            abort();
    }
}

int64_t LLVMIRBuilder::GetBitWidthFromMachineType(MachineType machineType) const
{
    switch (machineType) {
        case NOVALUE:
            return 0;
        case ARCH:
            return 48;  // 48: Pointer representation in different architectures
        case I1:
            return 1;
        case I8:
            return 8; // 8: bit width
        case I16:
            return 16; // 16: bit width
        case I32:
            return 32; // 32: bit width
        case I64:
            return 64; // 64: bit width
        case F32:
            return 32; // 32: bit width
        case F64:
            return 64; // 64: bit width
        case FLEX:
        case ANYVALUE:
            abort();
            break;
    }
}

void LLVMIRBuilder::HandleAdd(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitAdd(gate, g0, g1);
}

void LLVMIRBuilder::VisitAdd(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "add gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = nullptr;
    /* pointer                          int
      vector{i8 * x 2}          int
    */
    LLVMTypeRef returnType = ConvertLLVMTypeFromGate(gate);

    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    if (machineType == MachineType::I32 || machineType == MachineType::I64 ||
        machineType == MachineType::I8 || machineType == MachineType::I16) {
        auto e1Type = LLVMGetTypeKind(ConvertLLVMTypeFromGate(e1));
        if (e1Type == LLVMVectorTypeKind) {
            result = VectorAdd(e1Value, e2Value, returnType);
        } else if (e1Type == LLVMPointerTypeKind) {
            result = PointerAdd(e1Value, e2Value, returnType);
        } else {
            LLVMValueRef tmp1Value = LLVMBuildIntCast2(builder_, e1Value, returnType, 0, "");
            LLVMValueRef tmp2Value = LLVMBuildIntCast2(builder_, e2Value, returnType, 0, "");
            result = LLVMBuildAdd(builder_, tmp1Value, tmp2Value, "");
            if (LLVMTypeOf(tmp1Value) != LLVMTypeOf(tmp2Value)) {
                ASSERT(LLVMTypeOf(tmp1Value) == LLVMTypeOf(tmp2Value));
            }
        }
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFAdd(builder_, e1Value, e2Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleSub(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitSub(gate, g0, g1);
}

void LLVMIRBuilder::VisitSub(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "sub gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = nullptr;
    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    if (machineType == MachineType::I16 || machineType == MachineType::I32 || machineType == MachineType::I64) {
        result = LLVMBuildSub(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFSub(builder_, e1Value, e2Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleMul(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitMul(gate, g0, g1);
}

void LLVMIRBuilder::VisitMul(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "mul gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = nullptr;
    auto machineType = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    if (machineType == MachineType::I32 || machineType == MachineType::I64) {
        result = LLVMBuildMul(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFMul(builder_, e1Value, e2Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleFloatDiv(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitFloatDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntDiv(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleUDiv(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitUDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntOr(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntOr(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntXor(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntXor(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntLsr(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntLsr(gate, g0, g1);
}

void LLVMIRBuilder::HandleCmp(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitCmp(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitCmp(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "cmp gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = nullptr;
    auto e1ValCode = circuit_->LoadGatePtrConst(e1)->GetMachineType();
    [[maybe_unused]]auto e2ValCode = circuit_->LoadGatePtrConst(e2)->GetMachineType();
    ASSERT((e1ValCode == e2ValCode) ||
        (compCfg_->Is32Bit() && (e1ValCode == MachineType::ARCH) && (e2ValCode == MachineType::I32)) ||
        (compCfg_->Is64Bit() && (e1ValCode == MachineType::ARCH) && (e2ValCode == MachineType::I64)) ||
        (compCfg_->Is32Bit() && (e2ValCode == MachineType::ARCH) && (e1ValCode == MachineType::I32)) ||
        (compCfg_->Is64Bit() && (e2ValCode == MachineType::ARCH) && (e1ValCode == MachineType::I64)));
    LLVMIntPredicate intOpcode = LLVMIntEQ;
    LLVMRealPredicate realOpcode = LLVMRealPredicateFalse;
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::SLT: {
            intOpcode = LLVMIntSLT;
            realOpcode = LLVMRealOLT;
            break;
        }
        case OpCode::ULT: {
            intOpcode = LLVMIntULT;
            realOpcode = LLVMRealOLT;
            break;
        }
        case OpCode::SLE: {
            intOpcode = LLVMIntSLE;
            realOpcode = LLVMRealOLE;
            break;
        }
        case OpCode::SGT: {
            intOpcode = LLVMIntSGT;
            realOpcode = LLVMRealOGT;
            break;
        }
        case OpCode::SGE: {
            intOpcode = LLVMIntSGE;
            realOpcode = LLVMRealOGE;
            break;
        }
        case OpCode::NE: {
            intOpcode = LLVMIntNE;
            realOpcode = LLVMRealONE;
            break;
        }
        case OpCode::EQ: {
            intOpcode = LLVMIntEQ;
            realOpcode = LLVMRealOEQ;
            break;
        }
        default: {
            abort();
            break;
        }
    }
    if (e1ValCode == MachineType::I32 || e1ValCode == MachineType::I64 || e1ValCode == MachineType::ARCH) {
        e1Value = CanonicalizeToInt(e1Value);
        e2Value = CanonicalizeToInt(e2Value);
        result = LLVMBuildICmp(builder_, intOpcode, e1Value, e2Value, "");
    } else if (e1ValCode == MachineType::F64) {
        result = LLVMBuildFCmp(builder_, realOpcode, e1Value, e2Value, "");
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleLoad(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    GateRef base = ins[1];
    VisitLoad(gate, base);
}

void LLVMIRBuilder::HandleStore(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitStore(gate, ins[2], ins[1]);  // 2:baseAddr gate, 1:data gate
}

void LLVMIRBuilder::HandleChangeInt32ToDouble(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitChangeInt32ToDouble(gate, ins[0]);
}

void LLVMIRBuilder::HandleChangeDoubleToInt32(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitChangeDoubleToInt32(gate, ins[0]);
}

void LLVMIRBuilder::HandleChangeTaggedPointerToInt64(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitChangeTaggedPointerToInt64(gate, ins[0]);
}

void LLVMIRBuilder::HandleChangeInt64ToTagged(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitChangeInt64ToTagged(gate, ins[0]);
}

void LLVMIRBuilder::VisitIntDiv(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    LLVMValueRef result = LLVMBuildSDiv(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::VisitUDiv(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    LLVMValueRef result = LLVMBuildUDiv(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::VisitFloatDiv(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "float div gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);

    LLVMValueRef result = LLVMBuildFDiv(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitIntOr(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int or gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);

    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildOr(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleIntAnd(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntAnd(gate, g0, g1);
}

void LLVMIRBuilder::VisitIntAnd(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int and gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildAnd(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitIntXor(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int xor gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildXor(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitIntLsr(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int lsr gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildLShr(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleIntLsl(GateRef gate)
{
    auto g0 = circuit_->GetIn(gate, 0);
    auto g1 = circuit_->GetIn(gate, 1);
    VisitIntLsl(gate, g0, g1);
}

void LLVMIRBuilder::VisitIntLsl(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int lsl gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildShl(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitZExtInt(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int zero extension gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    ASSERT(GetBitWidthFromMachineType(circuit_->LoadGatePtrConst(e1)->GetMachineType()) <=
        GetBitWidthFromMachineType(circuit_->LoadGatePtrConst(gate)->GetMachineType()));
    LLVMValueRef result = LLVMBuildZExt(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitSExtInt(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int sign extension gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildSExt(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleCastIntXToIntY(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitCastIntXToIntY(gate, ins[0]);
}

void LLVMIRBuilder::VisitCastIntXToIntY(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int cast2 int gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildIntCast2(builder_, e1Value, ConvertLLVMTypeFromGate(gate), 1, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitChangeInt32ToDouble(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int cast2 double gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildSIToFP(builder_, e1Value, LLVMDoubleType(), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitChangeDoubleToInt32(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "double cast2 int32 gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildFPToSI(builder_, e1Value, LLVMInt32Type(), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitChangeTaggedPointerToInt64(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "tagged pointer cast2 int64 gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = CanonicalizeToInt(e1Value);
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitChangeInt64ToTagged(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int64 cast2 tagged gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    ASSERT(LLVMGetTypeKind(LLVMTypeOf(e1Value)) == LLVMIntegerTypeKind);
    LLVMValueRef result;
    if (compCfg_->Is32Bit()) {
        LLVMValueRef tmp1Value =
            LLVMBuildLShr(builder_, e1Value, LLVMConstInt(LLVMInt64Type(), 32, 0), ""); // 32: offset
        LLVMValueRef tmp2Value = LLVMBuildIntCast(builder_, e1Value, LLVMInt32Type(), ""); // low
        LLVMTypeRef vectorType = LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2);  // 2: packed vector type
        LLVMValueRef emptyValue = LLVMGetUndef(vectorType);
        tmp1Value = LLVMBuildIntToPtr(builder_, tmp1Value, LLVMPointerType(LLVMInt8Type(), 1), "");
        tmp2Value = LLVMBuildIntToPtr(builder_, tmp2Value, LLVMPointerType(LLVMInt8Type(), 1), "");
        result = LLVMBuildInsertElement(builder_, emptyValue, tmp2Value, LLVMConstInt(LLVMInt32Type(), 0, 0), "");
        result = LLVMBuildInsertElement(builder_, result, tmp1Value, LLVMConstInt(LLVMInt32Type(), 1, 0), "");
    } else {
        result = LLVMBuildIntToPtr(builder_, e1Value, LLVMPointerType(LLVMInt64Type(), 1), "");
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleBitCast(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitBitCast(gate, ins[0]);
}

void LLVMIRBuilder::VisitBitCast(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "bitcast gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    [[maybe_unused]] auto gateValCode = circuit_->LoadGatePtrConst(gate)->GetMachineType();
    [[maybe_unused]] auto e1ValCode = circuit_->LoadGatePtrConst(e1)->GetMachineType();
    ASSERT(GetBitWidthFromMachineType(gateValCode) == GetBitWidthFromMachineType(e1ValCode));
    auto returnType = ConvertLLVMTypeFromGate(gate);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, returnType, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

LLVMStubModule::LLVMStubModule(const std::string &name, const std::string &triple)
    : compCfg_(triple)
{
    module_ = LLVMModuleCreateWithName(name.c_str());
    LLVMSetTarget(module_, triple.c_str());
}

LLVMStubModule::~LLVMStubModule()
{
    if (module_ != nullptr) {
        LLVMDisposeModule(module_);
        module_ = nullptr;
    }
}

void LLVMStubModule::Initialize(const std::vector<int> &stubIndices)
{
    uint32_t i;
    for (i = 0; i < CALL_STUB_MAXCOUNT; i++) {
        auto stubDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(i);
        if (!stubDescriptor->GetName().empty()) {
            stubFunctionType_[i] = GetLLVMFunctionTypeStubDescriptor(stubDescriptor);
        }
    }
    for (i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        uint32_t index = stubIndices[i];
        auto stubDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(index);
        if (!stubDescriptor->GetName().empty()) {
            stubFunctions_[i] = GetLLVMFunctionByStubDescriptor(stubDescriptor);
        }
    }
#define DEF_STUB_FUNCTION(name, argc)                                                       \
    {                                                                                       \
        auto funcType = stubFunctionType_[CallStubId::NAME_BytecodeHandler];                \
        stubFunctions_[StubId::STUB_##name] = LLVMAddFunction(module_, #name, funcType);    \
    }
#if ECMASCRIPT_COMPILE_INTERPRETER_ASM
    INTERPRETER_STUB_LIST(DEF_STUB_FUNCTION)
#endif
#undef DEF_STUB_FUNCTION

#ifndef NDEBUG
    for (i = 0; i < TEST_FUNC_MAXCOUNT; i++) {
        auto testFuncDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(i + TEST_FUNC_OFFSET);
        if (!testFuncDescriptor->GetName().empty()) {
            testFunctions_[i] = GetLLVMFunctionByStubDescriptor(testFuncDescriptor);
        }
    }
#endif
}

LLVMValueRef LLVMStubModule::GetLLVMFunctionByStubDescriptor(StubDescriptor *stubDescriptor)
{
    auto funcType = GetLLVMFunctionTypeStubDescriptor(stubDescriptor);
    return LLVMAddFunction(module_, stubDescriptor->GetName().c_str(), funcType);
}

LLVMTypeRef LLVMStubModule::GetLLVMFunctionTypeStubDescriptor(StubDescriptor *stubDescriptor)
{
    LLVMTypeRef returnType = ConvertLLVMTypeFromVariableType(stubDescriptor->GetReturnType());
    std::vector<LLVMTypeRef> paramTys;
    auto paramCount = stubDescriptor->GetParametersCount();
    int extraParameterCnt = 0;

    auto paramsType = stubDescriptor->GetParametersType();
    ASSERT(paramCount > 0);
    // first argument is glue
    LLVMTypeRef glueType = ConvertLLVMTypeFromVariableType(paramsType[0]);
    paramTys.push_back(glueType);
    if (compCfg_.Is32Bit() && stubDescriptor->GetStubKind() != StubDescriptor::CallStubKind::RUNTIME_STUB) {
        for (int i = 0; i < reservedSlotForArm32; i++) {
            paramTys.push_back(glueType);  // fake paramter's type is same with glue type
        }
        extraParameterCnt += reservedSlotForArm32;
    }

    for (int i = 1; i < paramCount; i++) {
        auto paramsType = stubDescriptor->GetParametersType();
        paramTys.push_back(ConvertLLVMTypeFromVariableType(paramsType[i]));
    }
    auto functype = LLVMFunctionType(returnType, paramTys.data(), paramCount + extraParameterCnt,
        stubDescriptor->GetVariableArgs());
    return functype;
}

LLVMTypeRef LLVMStubModule::ConvertLLVMTypeFromVariableType(VariableType type)
{
    static std::map<VariableType, LLVMTypeRef> machineTypeMap = {
        {VariableType::VOID(), LLVMVoidType()},
        {VariableType::BOOL(), LLVMInt1Type()},
        {VariableType::INT8(), LLVMInt8Type()},
        {VariableType::INT16(), LLVMInt16Type()},
        {VariableType::INT32(), LLVMInt32Type()},
        {VariableType::INT64(), LLVMInt64Type()},
        {VariableType::INT8(), LLVMInt8Type()},
        {VariableType::INT16(), LLVMInt16Type()},
        {VariableType::INT32(), LLVMInt32Type()},
        {VariableType::INT64(), LLVMInt64Type()},
        {VariableType::FLOAT32(), LLVMFloatType()},
        {VariableType::FLOAT64(), LLVMDoubleType()},
        {VariableType::POINTER(), LLVMInt64Type()},
        {VariableType::JS_POINTER(), LLVMPointerType(LLVMInt64Type(), 1)},
        {VariableType::JS_ANY(), LLVMPointerType(LLVMInt64Type(), 1)},
    };
    if (compCfg_.Is32Bit()) {
        machineTypeMap[VariableType::POINTER()] = LLVMInt32Type();
        LLVMTypeRef vectorType = LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2);  // 2: packed vector type
        machineTypeMap[VariableType::JS_POINTER()] = vectorType;
        machineTypeMap[VariableType::JS_ANY()] = vectorType;
    }
    return machineTypeMap[type];
}
}  // namespace panda::ecmascript::kungfu
