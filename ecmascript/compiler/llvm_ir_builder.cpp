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
#include <set>
#include <string>
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/fast_stub.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_thread.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"
#include "securec.h"
#include "utils/logger.h"

namespace kungfu {
std::unordered_map<int, LLVMValueRef> g_values = {};

LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                             LLVMModuleRef module, LLVMValueRef function)
    : schedule_(schedule), circuit_(circuit), module_(module), function_(function)
{
    builder_ = LLVMCreateBuilder();
    context_ = LLVMGetGlobalContext();
    bbIdMapBb_.clear();
}

LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                             LLVMStubModule *module, LLVMValueRef function)
    : schedule_(schedule), circuit_(circuit), module_(module->GetModule()),
      function_(function), stubModule_(module)
{
    builder_ = LLVMCreateBuilder();
    context_ = LLVMGetGlobalContext();
    bbIdMapBb_.clear();
}

int LLVMIRBuilder::FindBasicBlock(AddrShift gate) const
{
    for (size_t bbIdx = 0; bbIdx < schedule_->size(); bbIdx++) {
        for (size_t instIdx = (*schedule_)[bbIdx].size(); instIdx > 0; instIdx--) {
            AddrShift tmp = (*schedule_)[bbIdx][instIdx - 1];
            if (tmp == gate) {
                return bbIdx;
            }
        }
    }
    return -1;
}

void LLVMIRBuilder::AssignHandleMap()
{
    opCodeHandleMap_ = {{OpCode::STATE_ENTRY, &LLVMIRBuilder::HandleGoto},
        {OpCode::RETURN, &LLVMIRBuilder::HandleReturn},
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
        {OpCode::INT32_CALL, &LLVMIRBuilder::HandleCall}, {OpCode::FLOAT64_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::INT64_CALL, &LLVMIRBuilder::HandleCall}, {OpCode::ALLOCA, &LLVMIRBuilder::HandleAlloca},
        {OpCode::INT1_ARG, &LLVMIRBuilder::HandleParameter}, {OpCode::INT32_ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::INT64_ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::INT32_CONSTANT, &LLVMIRBuilder::HandleInt32Constant},
        {OpCode::JS_CONSTANT, &LLVMIRBuilder::HandleInt64Constant},
        {OpCode::INT64_CONSTANT, &LLVMIRBuilder::HandleInt64Constant},
        {OpCode::FLOAT64_CONSTANT, &LLVMIRBuilder::HandleFloat64Constant},
        {OpCode::ZEXT_INT1_TO_INT32, &LLVMIRBuilder::HandleZExtInt},
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
        {OpCode::INT32_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT64_LOAD, &LLVMIRBuilder::HandleLoad},
        {OpCode::INT32_STORE, &LLVMIRBuilder::HandleStore},
        {OpCode::INT64_STORE, &LLVMIRBuilder::HandleStore},
        {OpCode::INT32_TO_FLOAT64, &LLVMIRBuilder::HandleCastInt32ToDouble},
        {OpCode::INT64_TO_FLOAT64, &LLVMIRBuilder::HandleCastInt64ToDouble},
        {OpCode::FLOAT64_TO_INT64, &LLVMIRBuilder::HandleCastDoubleToInt},
        {OpCode::INT32_LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::INT64_LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::FLOAT64_SMOD, &LLVMIRBuilder::HandleFloatMod},
        {OpCode::INT32_SMOD, &LLVMIRBuilder::HandleIntMod},

    };
    opCodeHandleIgnore= {OpCode::NOP, OpCode::CIRCUIT_ROOT, OpCode::DEPEND_ENTRY,
                        OpCode::FRAMESTATE_ENTRY, OpCode::RETURN_LIST, OpCode::THROW_LIST,
                        OpCode::CONSTANT_LIST, OpCode::ARG_LIST, OpCode::THROW,
                        OpCode::DEPEND_SELECTOR, OpCode::DEPEND_RELAY, OpCode::DEPEND_AND};
}

void LLVMIRBuilder::Build()
{
    LOG_ECMA(INFO) << "LLVM IR Builder Create Id Map of Blocks...";
    for (size_t bbIdx = 0; bbIdx < schedule_->size(); bbIdx++) {
        for (size_t instIdx = (*schedule_)[bbIdx].size(); instIdx > 0; instIdx--) {
            GateId gateId = circuit_->GetId((*schedule_)[bbIdx][instIdx - 1]);
            instIdMapBbId_[gateId] = bbIdx;
        }
    }
    LOG_ECMA(INFO) << "LLVM IR Builder Visit Gate...";
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
            AddrShift gate = (*schedule_)[bbIdx][instIdx - 1];
            std::vector<AddrShift> ins = circuit_->GetInVector(gate);
            std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
            std::cout << "instIdx :" << instIdx << std::endl;
            circuit_->Print(gate);
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

BasicBlock *LLVMIRBuilder::EnsurBasicBlock(int id)
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

void LLVMIRBuilder::StartLLVMBuilder(BasicBlock *bb) const
{
    EnsureLLVMBB(bb);
    LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    if ((impl == nullptr) || (impl->llvm_bb_ == nullptr)) {
        LOG_ECMA(ERROR) << "StartLLVMBuilder failed ";
        return;
    }
    impl->started = true;
    bb->SetImpl(impl);
    LOG_ECMA(DEBUG) << "Basicblock id :" << bb->GetId() << "impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    LLVMPositionBuilderAtEnd(builder_, impl->llvm_bb_);
}

void LLVMIRBuilder::ProcessPhiWorkList()
{
    for (BasicBlock *bb : phiRebuildWorklist_) {
        auto impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
        for (auto &e : impl->not_merged_phis) {
            BasicBlock *pred = e.pred;
            if (impl->started == 0) {
                LOG_ECMA(ERROR) << " ProcessPhiWorkList error hav't start ";
                return;
            }
            LLVMValueRef value = g_values[e.operand];
            if (LLVMTypeOf(value) != LLVMTypeOf(e.phi)) {
                LOG_ECMA(ERROR) << " ProcessPhiWorkList LLVMTypeOf don't match error ";
            }
            LLVMBasicBlockRef llvmBB = EnsureLLVMBB(pred);
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

LLVMTFBuilderBasicBlockImpl *LLVMIRBuilder::EnsureLLVMBBImpl(BasicBlock *bb) const
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
    auto frameType = circuit_->GetFrameType();
    LLVMAddTargetDependentFunctionAttr(function_, "frame-pointer", "all");

    if (frameType == panda::ecmascript::FrameType::OPTIMIZED_FRAME) {
        LLVMAddTargetDependentFunctionAttr(function_, "js-stub-call", "0");
    } else if (frameType == panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME) {
        LLVMAddTargetDependentFunctionAttr(function_, "js-stub-call", "1");
    } else {
        LOG_ECMA(DEBUG) << "frameType interpret type error !";
    }

    if (frameType != panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME) {
        return;
    }

    LLVMValueRef llvmFpAddr = LLVMCallingFp(module_, builder_);
    /* current frame
        0     pre rbp  <-- rbp
        -8    type
        -16    pre frame thread fp
    */
    LLVMValueRef thread = LLVMGetParam(function_, 0);
    LLVMValueRef rtoffset = LLVMConstInt(LLVMInt64Type(),
                                         panda::ecmascript::JSThread::GetCurrentFrameOffset(),
                                         0);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, thread, rtoffset, "");

    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(LLVMInt64Type(), 0), "");
    LLVMValueRef threadFpValue = LLVMBuildLoad(builder_, rtbaseAddr, "");
    LLVMValueRef value = LLVMBuildStore(builder_, threadFpValue,
        LLVMBuildIntToPtr(builder_, llvmFpAddr, LLVMPointerType(LLVMInt64Type(), 0), "cast"));
    LOG_ECMA(INFO) << "store value:" << value << " "
                << "value type" << LLVMTypeOf(value);
}

LLVMValueRef LLVMIRBuilder::LLVMCallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, false)};
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
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, fAddrRet, LLVMInt64Type(), "cast_int64_t");
    LLVMValueRef addr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(LLVMInt64Type(),
        16, false), ""); // 16:thread fp offset
    LLVMValueRef preFpAddr = LLVMBuildIntToPtr(builder, addr, LLVMPointerType(LLVMInt64Type(), 0), "thread.fp.Addr");
    return preFpAddr;
}

LLVMBasicBlockRef LLVMIRBuilder::EnsureLLVMBB(BasicBlock *bb) const
{
    LLVMTFBuilderBasicBlockImpl *impl = EnsureLLVMBBImpl(bb);
    if (impl->llvm_bb_) {
        return impl->llvm_bb_;
    }
    std::string buf = "B" + std::to_string(bb->GetId());
    LLVMBasicBlockRef llvmBB = LLVMAppendBasicBlock(function_, buf.c_str());
    impl->llvm_bb_ = llvmBB;
    impl->continuation = llvmBB;
    bb->SetImpl(impl);
    LOG_ECMA(DEBUG) << "create LLVMBB = " << buf << " impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
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
        case MachineRep::K_WORD32:
            dstType = LLVMInt32TypeInContext(context_);
            break;
        case MachineRep::K_FLOAT64:
            dstType = LLVMDoubleTypeInContext(context_);
            break;
        default:  // 64bit int goes to default scenario
            dstType = LLVMInt64TypeInContext(context_);
            break;
    }
    return dstType;
}

void LLVMIRBuilder::HandleCall(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::CALL:
        case OpCode::INT1_CALL:
        case OpCode::INT32_CALL:
        case OpCode::FLOAT64_CALL:
        case OpCode::INT64_CALL: {
            VisitCall(gate, ins);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::VisitCall(AddrShift gate, const std::vector<AddrShift> &inList)
{
    int paraStartIndex = 2;
    int index = circuit_->GetBitField(inList[1]);
    ASSERT(stubModule_ != nullptr);
    LLVMValueRef callee;
    StubDescriptor *callee_descriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(index);
    // runtime case
    if (callee_descriptor->GetStubKind() == StubDescriptor::CallStubKind::RUNTIME_STUB) {
        LLVMTypeRef rtfuncType = stubModule_->GetExternalFunctionType(index);
        LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
        LLVMValueRef thread = g_values[inList[2]];  // 2 : 2 means skip two input gates (target thread )
        LLVMValueRef rtoffset = LLVMConstInt(LLVMInt64Type(),
                                             panda::ecmascript::JSThread::GetRuntimeFunctionsOffset() +
                                                 (index - FAST_STUB_MAXCOUNT) * sizeof(uintptr_t),
                                             0);
        LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, thread, rtoffset, "");
        LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(LLVMInt64Type(), 0), "");
        LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, "");
        callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, "cast");
        paraStartIndex += 1;
    } else {
        callee = stubModule_->GetStubFunction(index);
    }
    // 16 : params limit
    LLVMValueRef params[16];
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        AddrShift gateTmp = inList[paraIdx];
        params[paraIdx - paraStartIndex] = g_values[gateTmp];
        circuit_->Print(gateTmp);
        LOG_ECMA(INFO) << "arg" << paraIdx - paraStartIndex << ": "
                       << LLVMPrintValueToString(params[paraIdx - paraStartIndex]);
    }
    if (callee == nullptr) {
        LOG_ECMA(ERROR) << "callee nullptr";
        return;
    }
    g_values[gate] = LLVMBuildCall(builder_, callee, params, inList.size() - paraStartIndex, "");
    return;
}

void LLVMIRBuilder::HandleAlloca(AddrShift gate)
{
    return VisitAlloca(gate);
}

void LLVMIRBuilder::VisitAlloca(AddrShift gate)
{
    uint64_t sizeEnum = circuit_->GetBitField(gate);
    LLVMTypeRef sizeType = GetMachineRepType(static_cast<MachineRep>(sizeEnum));
    LOG_ECMA(INFO) << LLVMGetTypeKind(sizeType);
    g_values[gate] = LLVMBuildPtrToInt(builder_, LLVMBuildAlloca(builder_, sizeType, ""), LLVMInt64Type(),
                                       "");  // NOTE: pointer val is currently viewed as 64bits
    return;
}

void LLVMIRBuilder::HandlePhi(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::VALUE_SELECTOR_INT1: {
            VisitPhi(gate, ins, MachineRep::K_BIT);
            break;
        }
        case OpCode::VALUE_SELECTOR_INT32: {
            VisitPhi(gate, ins, MachineRep::K_WORD32);
            break;
        }
        case OpCode::VALUE_SELECTOR_INT64: {
            VisitPhi(gate, ins, MachineRep::K_WORD64);
            break;
        }
        case OpCode::VALUE_SELECTOR_FLOAT64: {
            VisitPhi(gate, ins, MachineRep::K_FLOAT64);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::VisitPhi(AddrShift gate, const std::vector<AddrShift> &srcGates, MachineRep rep)
{
    LLVMTypeRef type = GetMachineRepType(rep);
    LLVMValueRef phi = LLVMBuildPhi(builder_, type, "");
    std::vector<AddrShift> relMergeIns = circuit_->GetInVector(srcGates[0]);
    bool addToPhiRebuildList = false;
    for (int i = 1; i < static_cast<int>(srcGates.size()); i++) {
        GateId gateId = circuit_->GetId(relMergeIns[i - 1]);
        int bbIdx = instIdMapBbId_[gateId];
        LOG_ECMA(INFO) << "srcGate: " << srcGates[i] << " dominated gateId:" << gateId << "dominated bbIdx: " << bbIdx;
        int cnt = bbIdMapBb_.count(bbIdx);
        // if cnt = 0 means bb with current bbIdx hasn't been created
        if (cnt > 0) {
            BasicBlock *bb = bbIdMapBb_[bbIdx].get();
            LOG_ECMA(INFO) << "bb : " << bb;
            if (bb == nullptr) {
                LOG_ECMA(ERROR) << "VisitPhi failed BasicBlock nullptr";
                return;
            }
            LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
            if (impl == nullptr) {
                LOG_ECMA(ERROR) << "VisitPhi failed impl nullptr";
                return;
            }
            LLVMBasicBlockRef llvmBB = EnsureLLVMBB(bb);  // The llvm bb
            LLVMValueRef value = g_values[srcGates[i]];
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
        g_values[gate] = phi;
    }
}

void LLVMIRBuilder::VisitReturn(AddrShift gate, AddrShift popCount, const std::vector<AddrShift> &operands) const
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    AddrShift operand = operands[2];  // 2: skip 2 in gate that are not data gate
    LOG_ECMA(INFO) << " gate: " << gate << " popCount: " << popCount;
    LOG_ECMA(INFO) << " return: " << operand << " gateId: " << circuit_->GetId(operand);
    LLVMValueRef returnValue = g_values[operand];
    LOG_ECMA(INFO) << LLVMPrintValueToString(returnValue);
    LLVMBuildRet(builder_, returnValue);
}

void LLVMIRBuilder::HandleReturn(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitReturn(gate, 1, ins);
}

void LLVMIRBuilder::VisitBlock(int gate, const OperandsVector &predecessors)  // NOLINTNEXTLINE(misc-unused-parameters)
{
    LOG_ECMA(INFO) << " BBIdx:" << gate;
    BasicBlock *bb = EnsurBasicBlock(gate);
    if (bb == nullptr) {
        LOG_ECMA(ERROR) << " block create failed ";
        return;
    }
    currentBb_ = bb;
    LLVMBasicBlockRef llvmbb = EnsureLLVMBB(bb);
    StartLLVMBuilder(bb);
    LOG_ECMA(INFO) << "predecessors :";
    for (int predecessor : predecessors) {
        BasicBlock *pre = EnsurBasicBlock(predecessor);
        if (pre == nullptr) {
            LOG_ECMA(ERROR) << " block setup failed, predecessor:%d nullptr" << predecessor;
            return;
        }
        LLVMBasicBlockRef llvmpre = EnsureLLVMBB(pre);
        LOG_ECMA(INFO) << "  " << predecessor;
        LLVMMoveBasicBlockBefore(llvmpre, llvmbb);
    }
    if (gate == 0) { // insert prologue
        PrologueHandle(module_, builder_);
    }
}

void LLVMIRBuilder::HandleGoto(AddrShift gate)
{
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
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
    BasicBlock *bb = EnsurBasicBlock(bbOut);
    if (bb == nullptr) {
        LOG_ECMA(ERROR) << " block is nullptr ";
        return;
    }
    llvm::BasicBlock *self = llvm::unwrap(EnsureLLVMBB(bbIdMapBb_[block].get()));
    llvm::BasicBlock *out = llvm::unwrap(EnsureLLVMBB(bbIdMapBb_[bbOut].get()));
    llvm::BranchInst::Create(out, self);
    EndCurrentBlock();
}

void LLVMIRBuilder::HandleInt32Constant(AddrShift gate)
{
    int32_t value = circuit_->GetBitField(gate);
    VisitInt32Constant(gate, value);
}

void LLVMIRBuilder::HandleInt64Constant(AddrShift gate)
{
    int64_t value = circuit_->GetBitField(gate);
    VisitInt64Constant(gate, value);
}

void LLVMIRBuilder::HandleFloat64Constant(AddrShift gate)
{
    int64_t value = circuit_->GetBitField(gate);
    double doubleValue = bit_cast<double>(value); // actual double value
    VisitFloat64Constant(gate, doubleValue);
}

void LLVMIRBuilder::HandleZExtInt(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::ZEXT_INT1_TO_INT32: {
            VisitZExtInt(gate, ins[0], MachineRep::K_WORD32);
            break;
        }
        case OpCode::ZEXT_INT32_TO_INT64:  // no break, fall through
        case OpCode::ZEXT_INT1_TO_INT64: {
            VisitZExtInt(gate, ins[0], MachineRep::K_WORD64);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::HandleSExtInt(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::SEXT_INT1_TO_INT32: {
            VisitSExtInt(gate, ins[0], MachineRep::K_WORD32);
            break;
        }
        case OpCode::SEXT_INT1_TO_INT64:  // no break, fall through
        case OpCode::SEXT_INT32_TO_INT64: {
            VisitSExtInt(gate, ins[0], MachineRep::K_WORD64);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::VisitInt32Constant(AddrShift gate, int32_t value) const
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt32Type(), value, 0);
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    LOG_ECMA(INFO) << "VisitInt32Constant set gate:" << gate << "  value:" << value;
    LOG_ECMA(INFO) << "VisitInt32Constant " << str;
}

void LLVMIRBuilder::VisitInt64Constant(AddrShift gate, int64_t value) const
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt64Type(), value, 0);
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    LOG_ECMA(INFO) << "VisitInt64Constant set gate:" << gate << "  value:" << value;
    LOG_ECMA(INFO) << "VisitInt64Constant " << str;
}

void LLVMIRBuilder::VisitFloat64Constant(AddrShift gate, double value) const
{
    LLVMValueRef llvmValue = LLVMConstReal(LLVMDoubleType(), value);
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    LOG_ECMA(INFO) << "VisitFloat64Constant set gate:" << gate << "  value:" << value;
    LOG_ECMA(INFO) << "VisitFloat64Constant " << str;
}

void LLVMIRBuilder::HandleParameter(AddrShift gate)
{
    return VisitParameter(gate);
}

void LLVMIRBuilder::VisitParameter(AddrShift gate) const
{
    int argth = circuit_->LoadGatePtrConst(gate)->GetBitField();
    LOG_ECMA(INFO) << " Parameter value" << argth;
    LLVMValueRef value = LLVMGetParam(function_, argth);
    g_values[gate] = value;
    LOG_ECMA(INFO) << "VisitParameter set gate:" << gate << "  value:" << value;
    // NOTE: caller put args, otherwise crash
    if (value == nullptr) {
        LOG_ECMA(ERROR) << "generate LLVM IR for para: " << argth << "fail";
        return;
    }
    char *str = LLVMPrintValueToString(value);
    LOG_ECMA(INFO) << "para arg:" << argth << "value IR:" << str;
}

void LLVMIRBuilder::HandleBranch(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    AddrShift bTrue = (circuit_->GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
    AddrShift bFalse = (circuit_->GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
    int bbTrue = instIdMapBbId_[circuit_->GetId(bTrue)];
    int bbFalse = instIdMapBbId_[circuit_->GetId(bFalse)];
    VisitBranch(gate, ins[1], bbTrue, bbFalse);
}

void LLVMIRBuilder::HandleIntMod(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitIntMod(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitIntMod(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int mod gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildSRem(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleFloatMod(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitFloatMod(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitFloatMod(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "float mod gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFRem(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitBranch(AddrShift gate, AddrShift cmp, int btrue, int bfalse)
{
    LOG_ECMA(INFO) << "cmp gate:" << cmp;
    if (g_values.count(cmp) == 0) {
        LOG_ECMA(ERROR) << "Branch condition gate is nullptr!";
        return;
    }
    LLVMValueRef cond = g_values[cmp];

    BasicBlock *trueBB = EnsurBasicBlock(btrue);
    BasicBlock *falseBB = EnsurBasicBlock(bfalse);
    EnsureLLVMBB(trueBB);
    EnsureLLVMBB(falseBB);

    LLVMBasicBlockRef llvmTrueBB = trueBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMBasicBlockRef llvmFalseBB = falseBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMValueRef result = LLVMBuildCondBr(builder_, cond, llvmTrueBB, llvmFalseBB);
    EndCurrentBlock();
    g_values[gate] = result;
}

void LLVMIRBuilder::HandleSwitch(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitSwitch(gate, ins[1], outs);
}

void LLVMIRBuilder::VisitSwitch(AddrShift gate, AddrShift input, const std::vector<AddrShift> &outList)
{
    LLVMValueRef cond = g_values[input];
    unsigned caseNum = outList.size();
    BasicBlock *curOutBB = nullptr;
    LLVMBasicBlockRef llvmDefaultOutBB = nullptr;
    for (int i = 0; i < static_cast<int>(caseNum); i++) {
        curOutBB = EnsurBasicBlock(instIdMapBbId_[circuit_->GetId(outList[i])]);
        EnsureLLVMBB(curOutBB);
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
        curOutBB = EnsurBasicBlock(instIdMapBbId_[circuit_->GetId(outList[i])]);
        llvmCurOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        LLVMAddCase(result, LLVMConstInt(LLVMInt64Type(), circuit_->GetBitField(outList[i]), 0), llvmCurOutBB);
    }
    EndCurrentBlock();
    g_values[gate] = result;
}

void LLVMIRBuilder::VisitLoad(AddrShift gate, MachineRep rep, AddrShift base) const
{
    LOG_ECMA(INFO) << "Load base gate:" << base;
    LLVMValueRef baseAddr = g_values[base];
    if (LLVMGetTypeKind(LLVMTypeOf(baseAddr)) == LLVMIntegerTypeKind) {
        baseAddr = LLVMBuildIntToPtr(builder_, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    }
    baseAddr = LLVMBuildPointerCast(builder_, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    LLVMValueRef value = LLVMBuildLoad(builder_, baseAddr, "");
    g_values[gate] = value;
    LOG_ECMA(INFO) << "Load value:" << value << " "
                   << "value type" << LLVMTypeOf(value);
}

void LLVMIRBuilder::VisitStore(AddrShift gate, MachineRep rep, AddrShift base, AddrShift dataToStore) const
{
    LOG_ECMA(INFO) << "store base gate:" << base;
    LLVMValueRef baseAddr = g_values[base];
    if (LLVMGetTypeKind(LLVMTypeOf(baseAddr)) == LLVMIntegerTypeKind) {
        baseAddr = LLVMBuildIntToPtr(builder_, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    }
    baseAddr = LLVMBuildPointerCast(builder_, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    LLVMValueRef value = LLVMBuildStore(builder_, g_values[dataToStore], baseAddr);
    g_values[gate] = value;
    LOG_ECMA(INFO) << "store value:" << value << " "
                   << "value type" << LLVMTypeOf(value);
}

void LLVMIRBuilder::VisitIntOrUintCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMIntPredicate opcode) const
{
    LOG_ECMA(INFO) << "cmp gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildICmp(builder_, opcode, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitFloatOrDoubleCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMRealPredicate opcode) const
{
    LOG_ECMA(INFO) << "cmp gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFCmp(builder_, opcode, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleIntRev(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitIntRev(gate, ins[0]);
}

void LLVMIRBuilder::VisitIntRev(AddrShift gate, AddrShift e1) const
{
    LOG_ECMA(INFO) << "int sign invert gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildNeg(builder_, e1Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleIntAdd(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::INT32_ADD: {
            VisitIntAdd(gate, ins[0], ins[1], MachineRep::K_WORD32);
            break;
        }
        case OpCode::INT64_ADD: {
            VisitIntAdd(gate, ins[0], ins[1], MachineRep::K_WORD64);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::VisitIntAdd(AddrShift gate, AddrShift e1, AddrShift e2, MachineRep rep) const
{
    LOG_ECMA(INFO) << "int add gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    if (LLVMGetTypeKind(LLVMTypeOf(e1Value)) == LLVMPointerTypeKind) {  // for scenario: pointer + offset
        e1Value = LLVMBuildPtrToInt(builder_, e1Value, GetMachineRepType(rep), "");
    }
    LLVMValueRef result = LLVMBuildAdd(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleFloatAdd(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitFloatAdd(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitFloatAdd(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "float add gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFAdd(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleFloatSub(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitFloatSub(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleFloatMul(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitFloatMul(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleFloatDiv(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitFloatDiv(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntSub(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitIntSub(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntMul(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitIntMul(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntOr(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitIntOr(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntXor(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitIntXor(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntLsr(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitIntLsr(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::HandleIntOrUintCmp(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
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

void LLVMIRBuilder::HandleFloatOrDoubleCmp(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    VisitFloatOrDoubleCmp(gate, ins[0], ins[1], LLVMRealOEQ);
}

void LLVMIRBuilder::HandleLoad(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::INT32_LOAD: {
            AddrShift base = ins[1];
            VisitLoad(gate, MachineRep::K_WORD32, base);
            break;
        }
        case OpCode::INT64_LOAD: {
            AddrShift base = ins[1];
            VisitLoad(gate, MachineRep::K_WORD64, base);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::HandleStore(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    std::vector<AddrShift> outs = circuit_->GetOutVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::INT32_STORE: {
            VisitStore(gate, MachineRep::K_WORD32, ins[2], ins[1]);  // 2:baseAddr gate, 1:data gate
            break;
        }
        case OpCode::INT64_STORE: {
            VisitStore(gate, MachineRep::K_WORD64, ins[2], ins[1]);  // 2:baseAddr gate, 1:data gate
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::HandleCastInt32ToDouble(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitCastInt32ToDouble(gate, ins[0]);
}

void LLVMIRBuilder::VisitFloatSub(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "float sub gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFSub(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitFloatMul(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "float mul gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFMul(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitFloatDiv(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "float div gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildFDiv(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitIntSub(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int sub gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildSub(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitIntMul(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int mul gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildMul(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitIntOr(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int or gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildOr(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleIntAnd(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitIntAnd(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitIntAnd(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int and gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildAnd(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitIntXor(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int xor gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildXor(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitIntLsr(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int lsr gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildLShr(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleIntLsl(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitIntLsl(gate, ins[0], ins[1]);
}

void LLVMIRBuilder::VisitIntLsl(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    LOG_ECMA(INFO) << "int lsl gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef e2Value = g_values[e2];
    LOG_ECMA(INFO) << "operand 1: " << LLVMPrintValueToString(e2Value);
    LLVMValueRef result = LLVMBuildShl(builder_, e1Value, e2Value, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitZExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    LOG_ECMA(INFO) << "int zero extension gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildZExt(builder_, e1Value, GetMachineRepType(rep), "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitSExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    LOG_ECMA(INFO) << "int sign extension gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildSExt(builder_, e1Value, GetMachineRepType(rep), "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleCastIntXToIntY(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    switch (circuit_->GetOpCode(gate)) {
        case OpCode::TRUNC_INT64_TO_INT1:
        case OpCode::TRUNC_INT32_TO_INT1: {
            VisitCastIntXToIntY(gate, ins[0], MachineRep::K_BIT);
            break;
        }
        case OpCode::TRUNC_INT64_TO_INT32: {
            VisitCastIntXToIntY(gate, ins[0], MachineRep::K_WORD32);
            break;
        }
        default: {
            break;
        }
    }
}

void LLVMIRBuilder::VisitCastIntXToIntY(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    LOG_ECMA(INFO) << "int cast2 int gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildIntCast2(builder_, e1Value, GetMachineRepType(rep), 1, "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::VisitCastInt32ToDouble(AddrShift gate, AddrShift e1) const
{
    LOG_ECMA(INFO) << "int cast2 double gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildSIToFP(builder_, e1Value, LLVMDoubleType(), "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleCastInt64ToDouble(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitCastInt64ToDouble(gate, ins[0]);
}

void LLVMIRBuilder::VisitCastInt64ToDouble(AddrShift gate, AddrShift e1) const
{
    LOG_ECMA(INFO) << "int cast2 double gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, LLVMDoubleType(), "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

void LLVMIRBuilder::HandleCastDoubleToInt(AddrShift gate)
{
    std::vector<AddrShift> ins = circuit_->GetInVector(gate);
    VisitCastDoubleToInt(gate, ins[0]);
}

void LLVMIRBuilder::VisitCastDoubleToInt(AddrShift gate, AddrShift e1) const
{
    LOG_ECMA(INFO) << "double cast2 int gate:" << gate;
    LLVMValueRef e1Value = g_values[e1];
    LOG_ECMA(INFO) << "operand 0: " << LLVMPrintValueToString(e1Value);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, LLVMInt64Type(), "");
    g_values[gate] = result;
    LOG_ECMA(INFO) << "result: " << LLVMPrintValueToString(result);
}

LLVMStubModule::LLVMStubModule(const char *name, const char *triple)
{
    module_ = LLVMModuleCreateWithName(name);
    LLVMSetTarget(module_, triple);
}

void LLVMStubModule::Initialize()
{
    uint32_t i;
    for (i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        auto stubDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(i);
        if (!stubDescriptor->GetName().empty()) {
            stubFunctions_[i] = GetLLVMFunctionByStubDescriptor(stubDescriptor);
        }
    }
    for (i = 0; i < MAX_EXTERNAL_FUNCTION_COUNT; i++) {
        auto externalDescriptor = FastStubDescriptors::GetInstance().GetStubDescriptor(i + EXTERNAL_FUNCTION_OFFSET);
        if (!externalDescriptor->GetName().empty()) {
            externalFuctionType_[i] = GetLLVMFunctionTypeStubDescriptor(externalDescriptor);
        }
    }
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
    auto functype = LLVMFunctionType(returnType, paramTys.data(), paramCount, 0);
    return functype;
}

LLVMTypeRef LLVMStubModule::ConvertLLVMTypeFromMachineType(MachineType type)
{
    static std::map<MachineType, LLVMTypeRef> machineTypeMap = {
        {MachineType::NONE_TYPE,        LLVMVoidType()},
        {MachineType::BOOL_TYPE,        LLVMInt1Type()},
        {MachineType::INT8_TYPE,        LLVMInt8Type()},
        {MachineType::INT16_TYPE,       LLVMInt16Type()},
        {MachineType::INT32_TYPE,       LLVMInt32Type()},
        {MachineType::INT64_TYPE,       LLVMInt64Type()},
        {MachineType::UINT8_TYPE,       LLVMInt8Type()},
        {MachineType::UINT16_TYPE,      LLVMInt16Type()},
        {MachineType::UINT32_TYPE,      LLVMInt32Type()},
        {MachineType::UINT64_TYPE,      LLVMInt64Type()},
        {MachineType::FLOAT32_TYPE,     LLVMFloatType()},
        {MachineType::FLOAT64_TYPE,     LLVMDoubleType()},
    };
    return machineTypeMap[type];
}
}  // namespace kungfu
