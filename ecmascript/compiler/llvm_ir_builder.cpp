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
#include <tuple>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/compiler_macros.h"
#include "ecmascript/compiler/fast_stub.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_array.h"
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
        LLVMSetGC(function_, "statepoint-example");
    }
    if (compCfg_->IsArm32()) {
        LLVMTypeRef elementTypes[] = {
            LLVMInt64Type(), // frameType
            LLVMPointerType(LLVMInt64Type(), 0), // prev
            LLVMInt32Type(), // sp
            LLVMInt32Type(), // fp
            LLVMInt64Type(), // patchpointId
        };
        slotSize_ = panda::ecmascript::FrameCommonConstants::ARM32_SLOT_SIZE;
        slotType_ = LLVMInt32Type();
        optFrameType_ = LLVMStructType(elementTypes, sizeof(elementTypes) / sizeof(LLVMTypeRef), 0);
    } else {
        LLVMTypeRef elementTypes[] = {
            LLVMInt64Type(), // frameType
            LLVMPointerType(LLVMInt64Type(), 0), // prev
            LLVMInt64Type(), // sp
            LLVMInt64Type(), // fp
            LLVMInt64Type(), // patchpointId
        };
        slotSize_ = panda::ecmascript::FrameCommonConstants::AARCH64_SLOT_SIZE;
        slotType_ = LLVMInt64Type();
        optFrameType_ = LLVMStructType(elementTypes, sizeof(elementTypes) / sizeof(LLVMTypeRef), 0);
    }
    optFrameSize_ = compCfg_->GetGlueOffset(JSThread::GlueID::OPT_LEAVE_FRAME_SIZE);
    interpretedFrameSize_ = compCfg_->GetGlueOffset(JSThread::GlueID::FRAME_STATE_SIZE);
    if (compCfg_->IsArm32()) {
        // hard float instruction
        LLVMAddTargetDependentFunctionAttr(function_, "target-features",
            "+armv7-a");
    }
    optLeaveFramePrevOffset_ = compCfg_->GetGlueOffset(JSThread::GlueID::OPT_LEAVE_FRAME_PREV_OFFSET);
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
        {OpCode::IF_BRANCH, &LLVMIRBuilder::HandleBranch}, {OpCode::SWITCH_BRANCH, &LLVMIRBuilder::HandleSwitch},
        {OpCode::ORDINARY_BLOCK, &LLVMIRBuilder::HandleGoto}, {OpCode::IF_TRUE, &LLVMIRBuilder::HandleGoto},
        {OpCode::IF_FALSE, &LLVMIRBuilder::HandleGoto}, {OpCode::SWITCH_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::MERGE, &LLVMIRBuilder::HandleGoto}, {OpCode::DEFAULT_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::LOOP_BEGIN, &LLVMIRBuilder::HandleGoto}, {OpCode::LOOP_BACK, &LLVMIRBuilder::HandleGoto},
        {OpCode::VALUE_SELECTOR_INT1, &LLVMIRBuilder::HandlePhi},
        {OpCode::VALUE_SELECTOR_INT32, &LLVMIRBuilder::HandlePhi},
        {OpCode::VALUE_SELECTOR_INT64, &LLVMIRBuilder::HandlePhi},
        {OpCode::VALUE_SELECTOR_FLOAT64, &LLVMIRBuilder::HandlePhi},
        {OpCode::CALL, &LLVMIRBuilder::HandleCall}, {OpCode::INT1_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::BYTECODE_CALL, &LLVMIRBuilder::HandleCall}, {OpCode::INT1_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::INT32_CALL, &LLVMIRBuilder::HandleCall}, {OpCode::FLOAT64_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::INT64_CALL, &LLVMIRBuilder::HandleCall}, {OpCode::ALLOCA, &LLVMIRBuilder::HandleAlloca},
        {OpCode::ANYVALUE_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::INT1_ARG, &LLVMIRBuilder::HandleParameter}, {OpCode::INT32_ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::INT64_ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::INT32_CONSTANT, &LLVMIRBuilder::HandleInt32Constant},
        {OpCode::JS_CONSTANT, &LLVMIRBuilder::HandleInt64Constant},
        {OpCode::INT64_CONSTANT, &LLVMIRBuilder::HandleInt64Constant},
        {OpCode::FLOAT64_CONSTANT, &LLVMIRBuilder::HandleFloat64Constant},
        {OpCode::ZEXT_INT1_TO_INT32, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_INT8_TO_INT32, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_INT16_TO_INT32, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_INT32_TO_INT64, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::ZEXT_INT1_TO_INT64, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::SEXT_INT1_TO_INT32, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::SEXT_INT1_TO_INT64, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::SEXT_INT32_TO_INT64, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::TRUNC_INT64_TO_INT1, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::TRUNC_INT32_TO_INT1, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::TRUNC_INT64_TO_INT32, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::INT32_REV, &LLVMIRBuilder::HandleIntRev},
        {OpCode::INT64_REV, &LLVMIRBuilder::HandleIntRev},
        {OpCode::INT32_ADD, &LLVMIRBuilder::HandleIntAdd},
        {OpCode::INT64_ADD, &LLVMIRBuilder::HandleIntAdd},
        {OpCode::FLOAT64_ADD, &LLVMIRBuilder::HandleFloatAdd},
        {OpCode::FLOAT64_SUB, &LLVMIRBuilder::HandleFloatSub},
        {OpCode::FLOAT64_MUL, &LLVMIRBuilder::HandleFloatMul},
        {OpCode::FLOAT64_DIV, &LLVMIRBuilder::HandleFloatDiv},
        {OpCode::INT32_SUB, &LLVMIRBuilder::HandleIntSub},
        {OpCode::INT64_SUB, &LLVMIRBuilder::HandleIntSub},
        {OpCode::INT32_MUL, &LLVMIRBuilder::HandleIntMul},
        {OpCode::INT64_MUL, &LLVMIRBuilder::HandleIntMul},
        {OpCode::INT32_AND, &LLVMIRBuilder::HandleIntAnd},
        {OpCode::INT64_AND, &LLVMIRBuilder::HandleIntAnd},
        {OpCode::INT32_OR, &LLVMIRBuilder::HandleIntOr},
        {OpCode::INT64_OR, &LLVMIRBuilder::HandleIntOr},
        {OpCode::INT64_XOR, &LLVMIRBuilder::HandleIntXor},
        {OpCode::INT32_LSR, &LLVMIRBuilder::HandleIntLsr},
        {OpCode::INT64_LSR, &LLVMIRBuilder::HandleIntLsr},
        {OpCode::INT32_SLT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_SLT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT32_ULT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_ULT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT32_SLE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_SLE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT32_SGT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_SGT, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT32_SGE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_SGE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT32_EQ, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_EQ, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::FLOAT64_EQ, &LLVMIRBuilder::HandleFloatOrDoubleCmp},
        {OpCode::INT32_NE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT64_NE, &LLVMIRBuilder::HandleIntOrUintCmp},
        {OpCode::INT8_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT16_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT32_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT64_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT32_STORE, &LLVMIRBuilder::HandleStore},
        {OpCode::INT64_STORE, &LLVMIRBuilder::HandleStore},
        {OpCode::INT32_TO_FLOAT64, &LLVMIRBuilder::HandleChangeInt32ToDouble},
        {OpCode::FLOAT64_TO_INT32, &LLVMIRBuilder::HandleChangeDoubleToInt32},
        {OpCode::TAGGED_POINTER_TO_INT64, &LLVMIRBuilder::HandleChangeTaggedPointerToInt64},
        {OpCode::BITCAST_INT64_TO_FLOAT64, &LLVMIRBuilder::HandleCastInt64ToDouble},
        {OpCode::BITCAST_FLOAT64_TO_INT64, &LLVMIRBuilder::HandleCastDoubleToInt},
        {OpCode::INT32_LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::INT64_LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::FLOAT64_SMOD, &LLVMIRBuilder::HandleFloatMod},
        {OpCode::INT32_SMOD, &LLVMIRBuilder::HandleIntMod},
        {OpCode::TAGGED_POINTER_CALL, &LLVMIRBuilder::HandleCall},
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

void LLVMIRBuilder::PrologueHandle(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* current frame for x86_64 system:
         0    pre rbp  <-- rbp
        -8    type
        -16   pre frame thread fp
     for 32 arm bit system:
        not support
     for arm64 system
         0    pre fp/ other regs  <-- fp
        -8    type
        -16   pre frame thread fp
        -24   current sp before call other function
    */
    if (compCfg_->IsArm32() || compCfg_->IsAArch64() || compCfg_->IsAmd64()) {
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
    (void)llvmFrameType;
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
    static constexpr int g_LLVMFrameOffset = 2;
    addr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(slotType_, g_LLVMFrameOffset * slotSize_, false), "");
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
            if (compCfg_->IsArm32()) {
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
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::CALL:
        case OpCode::ANYVALUE_CALL:
        case OpCode::INT1_CALL:
        case OpCode::INT32_CALL:
        case OpCode::FLOAT64_CALL:
        case OpCode::INT64_CALL: {
            VisitCall(gate, ins);
            break;
        }
        case OpCode::BYTECODE_CALL: {
            VisitBytecodeCall(gate, ins);
            break;
        }
        default: {
            break;
        }
    }
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
    if (compCfg_->IsAArch64()) {
        LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, false);
        LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder_, llvmFpAddr, slotType_, "cast_int_t");
        LLVMValueRef frameSpSlotAddr = LLVMBuildSub(builder_, frameAddr, LLVMConstInt(slotType_,
            3 * slotSize_, false), ""); // 3: type + threadsp + current sp
        LLVMValueRef addr = LLVMBuildIntToPtr(builder_, frameSpSlotAddr,
            LLVMPointerType(slotType_, 0), "frameType.Addr");
        LLVMMetadataRef meta = LLVMMDStringInContext2(context_, "sp", 3);   // 3 : 3 means len of "sp"
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
    LLVMValueRef frameTypeAddr = LLVMBuildIntToPtr(builder_, tmp, LLVMPointerType(LLVMInt64Type(), 1), "");
    LLVMValueRef frameType = LLVMBuildLoad(builder_, frameTypeAddr, "");
    return frameType;
}

void LLVMIRBuilder::PushFrameContext(LLVMValueRef newSp, LLVMValueRef oldSp)
{
    LLVMValueRef optFramePtr = LLVMBuildIntToPtr(builder_, newSp, LLVMPointerType(optFrameType_, 0), "");
    // set frameType
    LLVMValueRef typeDices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 0, 0)};
    LLVMValueRef frameTypeAddr = LLVMBuildGEP2(builder_, optFrameType_,
        optFramePtr, typeDices, sizeof(typeDices) / sizeof(LLVMValueRef), "getFrameTypeAddr");
    LLVMBuildStore(builder_, LLVMConstInt(LLVMInt64Type(),
        static_cast<long long>(panda::ecmascript::FrameType::OPTIMIZED_LEAVE_FRAME), 1),
        frameTypeAddr);
    // set prev
    LLVMValueRef prevDices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 1, 0)};
    LLVMValueRef prevTypeAddr = LLVMBuildGEP2(builder_, optFrameType_,
        optFramePtr, prevDices, sizeof(prevDices) / sizeof(LLVMValueRef), "getFramePrevAddr");
    LLVMValueRef convertOldSp = LLVMBuildIntToPtr(builder_, oldSp, LLVMPointerType(LLVMInt64Type(), 0), "");
    LLVMBuildStore(builder_, convertOldSp, prevTypeAddr);
    // sp
    LLVMValueRef spDices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 2, 0)};
    LLVMValueRef spAddr = LLVMBuildGEP2(builder_, optFrameType_,
        optFramePtr, spDices, sizeof(spDices) / sizeof(LLVMValueRef), "getFrameSpAddr");
    LLVMValueRef sp = GetCurrentSP();
    LLVMValueRef convertSpAddr = LLVMBuildIntCast2(builder_, sp, slotType_, 1, "");
    LLVMBuildStore(builder_, convertSpAddr, spAddr);
    // fp
    LLVMValueRef fpDices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 3, 0)};
    LLVMValueRef fpAddr = LLVMBuildGEP2(builder_, optFrameType_,
        optFramePtr, fpDices, sizeof(fpDices) / sizeof(LLVMValueRef), "getFrameFpAddr");
    LLVMValueRef fpValue = CallingFp(module_, builder_, false);
    LLVMValueRef convertFpValue = LLVMBuildPtrToInt(builder_, fpValue, slotType_, "");
    LLVMBuildStore(builder_, convertFpValue, fpAddr);
    // patchpointid
    LLVMValueRef patchpointIdDices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), 4, 0)};
    LLVMValueRef patchpointIdAddr = LLVMBuildGEP2(builder_, optFrameType_,
        optFramePtr, patchpointIdDices, sizeof(patchpointIdDices) / sizeof(LLVMValueRef), "getFramePatchPointIdAddr");
    LLVMValueRef patchPointIdVlue = LLVMConstInt(LLVMInt64Type(), 2882400000, 0); // 2882400000: default statepoint ID
    LLVMBuildStore(builder_, patchPointIdVlue, patchpointIdAddr);
}

void LLVMIRBuilder::ConstructFrame()
{
    if (circuit_->GetFrameType() != FrameType::OPTIMIZED_FRAME) {
        return;
    }
    LLVMValueRef currentSp = GetCurrentSPFrame();
    LLVMValueRef newSp = currentSp;
    // set newsp:currentSp sub interpretedFrame and optFrame
    newSp = LLVMBuildSub(builder_, newSp, LLVMConstInt(slotType_,
        optFrameSize_, 1), "");
    newSp = LLVMBuildSub(builder_, newSp, LLVMConstInt(slotType_,
        interpretedFrameSize_, 1), "");
    PushFrameContext(newSp, currentSp);
    LLVMValueRef preAddr = LLVMBuildAdd(builder_, newSp, LLVMConstInt(slotType_,
        optLeaveFramePrevOffset_, 1), ""); // newSp sub type size get presize
    SetCurrentSPFrame(preAddr);
}

void LLVMIRBuilder::DestoryFrame()
{
    if (circuit_->GetFrameType() != FrameType::OPTIMIZED_FRAME) {
        return;
    }
    LLVMValueRef currentSp = GetCurrentSPFrame();
    LLVMValueRef frameType = GetCurrentFrameType(currentSp);
    (void)frameType;
    LLVMValueRef preAddr = LLVMBuildSub(builder_, currentSp, LLVMConstInt(slotType_,
        optLeaveFramePrevOffset_, 1), "");
    preAddr = LLVMBuildAdd(builder_, preAddr, LLVMConstInt(slotType_,
        optFrameSize_, 1), "");
    preAddr = LLVMBuildAdd(builder_, preAddr,
        LLVMConstInt(slotType_, interpretedFrameSize_, 1), "");
    SetCurrentSPFrame(preAddr);
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
    if (calleeDescriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB) {
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
    // 16 : params limit
    LLVMValueRef params[16];
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params[paraIdx - paraStartIndex] = gateToLLVMMaps_[gateTmp];
    }
    if (callee == nullptr) {
        COMPILER_LOG(ERROR) << "callee nullptr";
        return;
    }
    if (compCfg_->IsArm32() || compCfg_->IsAArch64() || compCfg_->IsAmd64()) {
        // callerid | idx
        if (calleeDescriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB) {
            ConstructFrame();
        }
    } else {
        SaveCurrentSP();
    }

    gateToLLVMMaps_[gate] = LLVMBuildCall(builder_, callee, params, inList.size() - paraStartIndex, "");
    if (calleeDescriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB) {
        DestoryFrame();
    }
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
    LLVMValueRef bytecodeoffset = LLVMConstInt(glue_type, compCfg_->GetGlueOffset(JSThread::GlueID::BYTECODE_HANDLERS), 0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, LLVMBuildAdd(builder_, bytecodeoffset, opcodeOffset, ""), "");
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(glue_type, 0), "");
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, "");
    callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, "cast");
    // 16 : params limit
    LLVMValueRef params[16];
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params[paraIdx - paraStartIndex] = gateToLLVMMaps_[gateTmp];
    }
    gateToLLVMMaps_[gate] = LLVMBuildCall(builder_, callee, params, inList.size() - paraStartIndex, "");
    LLVMSetTailCall(gateToLLVMMaps_[gate], true);
    LLVMSetInstructionCallConv(gateToLLVMMaps_[gate], LLVMGHCCallConv);
    return;

}

void LLVMIRBuilder::HandleAlloca(GateRef gate)
{
    return VisitAlloca(gate);
}

void LLVMIRBuilder::VisitAlloca(GateRef gate)
{
    uint64_t sizeEnum = circuit_->GetBitField(gate);
    LLVMTypeRef sizeType = GetMachineRepType(static_cast<MachineRep>(sizeEnum));
    COMPILER_LOG(DEBUG) << LLVMGetTypeKind(sizeType);
    gateToLLVMMaps_[gate] = LLVMBuildPtrToInt(builder_, LLVMBuildAlloca(builder_, sizeType, ""),
        ConvertLLVMTypeFromGate(gate), "");  // NOTE: pointer val is currently viewed as 64bits
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
    for (int j = 1; j < static_cast<int>(srcGates.size()); j++) {
        circuit_->Print(srcGates[j]);
    }
    circuit_->Print(gate);
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
        PrologueHandle(module_, builder_);
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

void LLVMIRBuilder::HandleInt32Constant(GateRef gate)
{
    int32_t value = circuit_->GetBitField(gate);
    VisitInt32Constant(gate, value);
}

void LLVMIRBuilder::HandleInt64Constant(GateRef gate)
{
    int64_t value = circuit_->GetBitField(gate);
    VisitInt64Constant(gate, value);
}

void LLVMIRBuilder::HandleFloat64Constant(GateRef gate)
{
    int64_t value = circuit_->GetBitField(gate);
    double doubleValue = bit_cast<double>(value); // actual double value
    VisitFloat64Constant(gate, doubleValue);
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

void LLVMIRBuilder::VisitInt32Constant(GateRef gate, int32_t value)
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt32Type(), value, 0);
    gateToLLVMMaps_[gate] = llvmValue;
    COMPILER_LOG(DEBUG) << "VisitInt32Constant set gate:" << gate << "  value:" << value;
    COMPILER_LOG(DEBUG) << "VisitInt32Constant " << LLVMValueToString(llvmValue);
}

void LLVMIRBuilder::VisitInt64Constant(GateRef gate, int64_t value)
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt64Type(), value, 0);
    LLVMTypeRef type = ConvertLLVMTypeFromGate(gate);
    if (LLVMGetTypeKind(type) == LLVMPointerTypeKind) {
        llvmValue = LLVMBuildIntToPtr(builder_, llvmValue, type, "");
    } else if (LLVMGetTypeKind(type) == LLVMVectorTypeKind) {
        LLVMValueRef tmp1Value =
            LLVMBuildLShr(builder_, llvmValue, LLVMConstInt(LLVMInt32Type(), 32, 0), ""); // 32: offset
        LLVMValueRef tmp2Value = LLVMBuildIntCast(builder_, llvmValue, LLVMInt32Type(), ""); // low
        LLVMValueRef emptyValue = LLVMGetUndef(type);
        tmp1Value = LLVMBuildIntToPtr(builder_, tmp1Value, LLVMPointerType(LLVMInt8Type(), 1), "");
        tmp2Value = LLVMBuildIntToPtr(builder_, tmp2Value, LLVMPointerType(LLVMInt8Type(), 1), "");
        llvmValue = LLVMBuildInsertElement(builder_, emptyValue, tmp2Value, LLVMConstInt(LLVMInt32Type(), 0, 0), "");
        llvmValue = LLVMBuildInsertElement(builder_, llvmValue, tmp1Value, LLVMConstInt(LLVMInt32Type(), 1, 0), "");
    } else if (LLVMGetTypeKind(type) == LLVMIntegerTypeKind) {
        // do nothing
    } else {
        abort();
    }
    gateToLLVMMaps_[gate] = llvmValue;
    COMPILER_LOG(DEBUG) << "VisitInt64Constant set gate:" << gate << "  value:" << value;
    COMPILER_LOG(DEBUG) << "VisitInt64Constant " << LLVMValueToString(llvmValue);
}

void LLVMIRBuilder::VisitFloat64Constant(GateRef gate, double value)
{
    LLVMValueRef llvmValue = LLVMConstReal(LLVMDoubleType(), value);
    gateToLLVMMaps_[gate] = llvmValue;
    COMPILER_LOG(DEBUG) << "VisitFloat64Constant set gate:" << gate << "  value:" << value;
    COMPILER_LOG(DEBUG) << "VisitFloat64Constant " << LLVMValueToString(llvmValue);
}

void LLVMIRBuilder::HandleParameter(GateRef gate)
{
    return VisitParameter(gate);
}

void LLVMIRBuilder::VisitParameter(GateRef gate)
{
    int argth = circuit_->LoadGatePtrConst(gate)->GetBitField();
    COMPILER_LOG(DEBUG) << " Parameter value" << argth;
    LLVMValueRef value = LLVMGetParam(function_, argth);

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

void LLVMIRBuilder::HandleIntMod(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitIntMod(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitIntMod(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int mod gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildSRem(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleFloatMod(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitFloatMod(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitFloatMod(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "float mod gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFRem(builder_, e1Value, e2Value, "");
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
    for (int i = 0; i < static_cast<int>(caseNum - 1); i++) {
        if (circuit_->GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        curOutBB = EnsureBasicBlock(instIdMapBbId_[circuit_->GetId(outList[i])]);
        llvmCurOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        LLVMAddCase(result, LLVMConstInt(LLVMInt64Type(), circuit_->GetBitField(outList[i]), 0), llvmCurOutBB);
    }
    EndCurrentBlock();
    gateToLLVMMaps_[gate] = result;
}

void LLVMIRBuilder::VisitLoad(GateRef gate, GateRef base)
{
    COMPILER_LOG(DEBUG) << "Load base gate:" << base;
    LLVMValueRef baseAddr = gateToLLVMMaps_[base];
    LLVMTypeRef pointerType = ConvertLLVMTypeFromGate(gate);
    LLVMDumpType(pointerType);
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
    LLVMDumpValue(baseAddr);
    std::cout << std::endl;
    baseAddr = CanonicalizeToPtr(baseAddr);
    LLVMValueRef data = gateToLLVMMaps_[dataToStore];
    baseAddr = LLVMBuildPointerCast(builder_, baseAddr,
        LLVMPointerType(ConvertLLVMTypeFromGate(dataToStore), LLVMGetPointerAddressSpace(LLVMTypeOf(baseAddr))), "");
    LLVMValueRef value = LLVMBuildStore(builder_, data, baseAddr);
    gateToLLVMMaps_[gate] = value;
    COMPILER_LOG(DEBUG) << "store value:" << value << " " << "value type" << LLVMTypeOf(value);
}

void LLVMIRBuilder::VisitIntOrUintCmp(GateRef gate, GateRef e1, GateRef e2, LLVMIntPredicate opcode)
{
    COMPILER_LOG(DEBUG) << "cmp gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    e1Value = CanonicalizeToInt(e1Value);
    e2Value = CanonicalizeToInt(e2Value);
    LLVMValueRef result = LLVMBuildICmp(builder_, opcode, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
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

void LLVMIRBuilder::VisitFloatOrDoubleCmp(GateRef gate, GateRef e1, GateRef e2, LLVMRealPredicate opcode)
{
    COMPILER_LOG(DEBUG) << "cmp gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFCmp(builder_, opcode, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
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
    LLVMValueRef result = LLVMBuildNot(builder_, e1Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleIntAdd(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntAdd(gate, ins[0], ins[1]);
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

void LLVMIRBuilder::VisitIntAdd(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int add gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMTypeRef e1Type = ConvertLLVMTypeFromGate(e1);
    LLVMValueRef result;
    LLVMValueRef offset = e2Value;
    /* pointer                          int
       vector{i8 * x 2}          int
    */
    LLVMTypeRef returnType = ConvertLLVMTypeFromGate(gate);
    if (LLVMGetTypeKind(e1Type) == LLVMPointerTypeKind) {  // for scenario: pointer + offset
        result = PointerAdd(e1Value, offset, returnType);
    } else if (LLVMGetTypeKind(e1Type) == LLVMVectorTypeKind) {
        result = VectorAdd(e1Value, offset, returnType);
    } else {
        LLVMValueRef tmp1Value = LLVMBuildIntCast2(builder_, e1Value, returnType, 0, "");
        LLVMValueRef tmp2Value = LLVMBuildIntCast2(builder_, e2Value, returnType, 0, "");
        result = LLVMBuildAdd(builder_, tmp1Value, tmp2Value, "");
        if (LLVMTypeOf(tmp1Value) != LLVMTypeOf(tmp2Value)) {
            ASSERT(LLVMTypeOf(tmp1Value) == LLVMTypeOf(tmp2Value));
        }
    }
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

LLVMTypeRef LLVMIRBuilder::ConvertLLVMTypeFromGate(GateRef gate) const
{
    if (circuit_->GetTypeCode(gate) >= TypeCode::JS_TYPE_OBJECT_START) {
        if (compCfg_->IsArm32()) {
            return LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2);
        } else {
            return LLVMPointerType(LLVMInt64Type(), 1);
        }
    }
    switch (circuit_->GetOpCode(gate).GetValueCode()) {
        case ValueCode::NOVALUE:
            return LLVMVoidType();
        case ValueCode::ANYVALUE:
        {
            if (compCfg_->IsArm32()) {
                return LLVMInt32Type();
            } else {
                return LLVMInt64Type();
            }
        }
        case ValueCode::INT1:
            return LLVMInt1Type();
        case ValueCode::INT8:
            return LLVMInt8Type();
        case ValueCode::INT16:
            return LLVMInt16Type();
        case ValueCode::INT32:
            return LLVMInt32Type();
        case ValueCode::INT64:
            return LLVMInt64Type();
        case ValueCode::FLOAT32:
            return LLVMFloatType();
        case ValueCode::FLOAT64:
            return LLVMDoubleType();
        default:
            abort();
    }
}

void LLVMIRBuilder::HandleFloatAdd(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitFloatAdd(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitFloatAdd(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "float add gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFAdd(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleFloatSub(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitFloatSub(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleFloatMul(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitFloatMul(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleFloatDiv(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitFloatDiv(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntSub(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntSub(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntMul(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntMul(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntOr(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntOr(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntXor(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntXor(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntLsr(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitIntLsr(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntOrUintCmp(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::INT32_SLT:  // no break, fall through
        case OpCode::INT64_SLT: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntSLT);
            break;
        }
        case OpCode::INT32_ULT:  // no break, fall through
        case OpCode::INT64_ULT: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntULT);
            break;
        }
        case OpCode::INT32_SLE:  // no break, fall through
        case OpCode::INT64_SLE: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntSLE);
            break;
        }
        case OpCode::INT32_SGT:  // no break, fall through
        case OpCode::INT64_SGT: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntSGT);
            break;
        }
        case OpCode::INT32_SGE:  // no break, fall through
        case OpCode::INT64_SGE: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntSGE);
            break;
        }
        case OpCode::INT32_EQ:  // no break, fall through
        case OpCode::INT64_EQ: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntEQ);
            break;
        }
        case OpCode::INT32_NE:  // no break, fall through
        case OpCode::INT64_NE: {
            VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntNE);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::HandleFloatOrDoubleCmp(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    std::vector<GateRef> outs = circuit_->GetOutVector(gate);
    VisitFloatOrDoubleCmp(gate, ins[0], ins[1], LLVMRealOEQ);
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

void LLVMIRBuilder::VisitFloatSub(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "float sub gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFSub(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitFloatMul(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "float mul gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFMul(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
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

void LLVMIRBuilder::VisitIntSub(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int sub gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildSub(builder_, e1Value, e2Value, "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::VisitIntMul(GateRef gate, GateRef e1, GateRef e2)
{
    COMPILER_LOG(DEBUG) << "int mul gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef e2Value = gateToLLVMMaps_[e2];
    COMPILER_LOG(DEBUG) << "operand 1: " << LLVMValueToString(e2Value);
    LLVMValueRef result = LLVMBuildMul(builder_, e1Value, e2Value, "");
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
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitIntAnd(gate, ins[0], ins[1]);
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
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitIntLsl(gate, ins[0], ins[1]);
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
    COMPILER_LOG(DEBUG) << "double cast2 int32 gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = CanonicalizeToInt(e1Value);
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleCastInt64ToDouble(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitCastInt64ToDouble(gate, ins[0]);
}

void LLVMIRBuilder::VisitCastInt64ToDouble(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "int cast2 double gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, LLVMDoubleType(), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

void LLVMIRBuilder::HandleCastDoubleToInt(GateRef gate)
{
    std::vector<GateRef> ins = circuit_->GetInVector(gate);
    VisitCastDoubleToInt(gate, ins[0]);
}

void LLVMIRBuilder::VisitCastDoubleToInt(GateRef gate, GateRef e1)
{
    COMPILER_LOG(DEBUG) << "double cast2 int gate:" << gate;
    LLVMValueRef e1Value = gateToLLVMMaps_[e1];
    COMPILER_LOG(DEBUG) << "operand 0: " << LLVMValueToString(e1Value);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, LLVMInt64Type(), "");
    gateToLLVMMaps_[gate] = result;
    COMPILER_LOG(DEBUG) << "result: " << LLVMValueToString(result);
}

LLVMTypeRef LLVMIRBuilder::ConvertLLVMTypeFromTypeCode(TypeCode type) const
{
    if (UNLIKELY(type == TypeCode::NOTYPE)) {
        UNREACHABLE();
    }
    if (type <= TypeCode::JS_TYPE_SPECIAL_STOP) {
        return LLVMInt64Type();
    }
    // type >= TypeCode::JS_TYPE_OBJECT_START
    return LLVMPointerType(LLVMInt64Type(), 1);
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
    for (i = 0; i < ALL_STUB_MAXCOUNT; i++) {
        uint32_t index = stubIndices[i];
        auto stubDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(index);
        if (!stubDescriptor->GetName().empty()) {
            stubFunctions_[i] = GetLLVMFunctionByStubDescriptor(stubDescriptor);
        }
    }
    for (i = 0; i < CALL_STUB_MAXCOUNT; i++) {
        auto stubDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(i);
        if (!stubDescriptor->GetName().empty()) {
            stubFunctionType_[i] = GetLLVMFunctionTypeStubDescriptor(stubDescriptor);
        }
    }
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
    LLVMTypeRef returnType = ConvertLLVMTypeFromMachineType(stubDescriptor->GetReturnType());
    std::vector<LLVMTypeRef> paramTys;
    auto paramCount = stubDescriptor->GetParametersCount();
    for (int i = 0; i < paramCount; i++) {
        auto paramsType = stubDescriptor->GetParametersType();
        paramTys.push_back(ConvertLLVMTypeFromMachineType(paramsType[i]));
    }
    auto functype = LLVMFunctionType(returnType, paramTys.data(), paramCount, stubDescriptor->GetVariableArgs());
    return functype;
}

LLVMTypeRef LLVMStubModule::ConvertLLVMTypeFromMachineType(MachineType type)
{
    static std::map<MachineType, LLVMTypeRef> machineTypeMap = {
        {MachineType::NONE,           LLVMVoidType()},
        {MachineType::BOOL,           LLVMInt1Type()},
        {MachineType::INT8,           LLVMInt8Type()},
        {MachineType::INT16,          LLVMInt16Type()},
        {MachineType::INT32,          LLVMInt32Type()},
        {MachineType::INT64,          LLVMInt64Type()},
        {MachineType::UINT8,          LLVMInt8Type()},
        {MachineType::UINT16,         LLVMInt16Type()},
        {MachineType::UINT32,         LLVMInt32Type()},
        {MachineType::UINT64,         LLVMInt64Type()},
        {MachineType::FLOAT32,        LLVMFloatType()},
        {MachineType::FLOAT64,        LLVMDoubleType()},
        {MachineType::NATIVE_POINTER, LLVMInt64Type()},
        {MachineType::TAGGED_POINTER, LLVMPointerType(LLVMInt64Type(), 1)},
        {MachineType::TAGGED,         LLVMPointerType(LLVMInt64Type(), 1)},
    };
    if (compCfg_.IsArm32()) {
        machineTypeMap[MachineType::NATIVE_POINTER] = LLVMInt32Type();
        LLVMTypeRef vectorType = LLVMVectorType(LLVMPointerType(LLVMInt8Type(), 1), 2);  // 2: packed vector type
        machineTypeMap[MachineType::TAGGED_POINTER] = vectorType;
        machineTypeMap[MachineType::TAGGED] = vectorType;
    }
    return machineTypeMap[type];
}
}  // namespace panda::ecmascript::kungfu
