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

#include <string>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/fastpath_optimizer.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_interface.h"
#include "ecmascript/js_array.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"
#include "llvm_mcjit_compiler.h"
#include "securec.h"

namespace kungfu {
std::unordered_map<int, LLVMValueRef> g_values = {};

LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit)
    : m_schedule(schedule), m_circuit(circuit)
{
    m_module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(m_module, "x86_64-unknown-linux-gnu");
    m_builder = LLVMCreateBuilder();
    m_context = LLVMGetGlobalContext();
    LLVMTypeRef paramTys[] = {
        LLVMInt32Type(),
    };
    m_function = LLVMAddFunction(m_module, "foo", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
    m_bbIdMapBb.clear();
}

LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<AddrShift>> *schedule, const Circuit *circuit,
                             LLVMModuleRef module, LLVMValueRef function)
    : m_schedule(schedule), m_circuit(circuit), m_module(module), m_function(function)
{
    LLVMSetTarget(m_module, "x86_64-unknown-linux-gnu");
    m_builder = LLVMCreateBuilder();
    m_context = LLVMGetGlobalContext();
    m_bbIdMapBb.clear();
}

int LLVMIRBuilder::FindBasicBlock(AddrShift gate) const
{
    for (size_t bbIdx = 0; bbIdx < m_schedule->size(); bbIdx++) {
        for (size_t instIdx = (*m_schedule)[bbIdx].size(); instIdx > 0; instIdx--) {
            AddrShift tmp = (*m_schedule)[bbIdx][instIdx - 1];
            if (tmp == gate) {
                return bbIdx;
            }
        }
    }
    return -1;
}

void LLVMIRBuilder::Build()
{
    std::cout << "LLVM IR Builder Create Id Map of Blocks..." << std::endl;
    for (size_t bbIdx = 0; bbIdx < m_schedule->size(); bbIdx++) {
        for (size_t instIdx = (*m_schedule)[bbIdx].size(); instIdx > 0; instIdx--) {
            GateId gateId = m_circuit->GetId((*m_schedule)[bbIdx][instIdx - 1]);
            m_instIdMapBbId[gateId] = bbIdx;
        }
    }

    std::cout << "LLVM IR Builder Visit Gate..." << std::endl;
    for (size_t bbIdx = 0; bbIdx < (*m_schedule).size(); bbIdx++) {
        OperandsVector predecessors;
        for (auto in : m_circuit->GetInVector((*m_schedule)[bbIdx][0])) {
            if (!m_circuit->GetOpCode(in).IsState()) {
                continue;
            }
            predecessors.insert(m_instIdMapBbId[m_circuit->GetId(in)]);
        }
        VisitBlock(bbIdx, predecessors);

        for (size_t instIdx = (*m_schedule)[bbIdx].size(); instIdx > 0; instIdx--) {
            AddrShift gate = (*m_schedule)[bbIdx][instIdx - 1];
            std::vector<AddrShift> ins = m_circuit->GetInVector(gate);
            std::vector<AddrShift> outs = m_circuit->GetOutVector(gate);
            switch (m_circuit->GetOpCode(gate)) {
                case OpCode::NOP:
                    break;
                case OpCode::CIRCUIT_ROOT:
                    break;
                case OpCode::STATE_ENTRY: {
                    int block = m_instIdMapBbId[m_circuit->GetId(gate)];
                    int bbOut = m_instIdMapBbId[m_circuit->GetId(outs[0])];
                    VisitGoto(block, bbOut);
                    break;
                }
                case OpCode::DEPEND_ENTRY:
                    break;
                case OpCode::FRAMESTATE_ENTRY:
                    break;
                case OpCode::RETURN_LIST:
                    break;
                case OpCode::THROW_LIST:
                    break;
                case OpCode::CONSTANT_LIST:
                    break;
                case OpCode::ARG_LIST:
                    break;
                case OpCode::RETURN: {
                    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
                    VisitReturn(gate, 1, ins);
                    break;
                }
                case OpCode::THROW:
                    break;
                case OpCode::IF_BRANCH: {
                    AddrShift bTrue = (m_circuit->GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
                    AddrShift bFalse = (m_circuit->GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
                    int bbTrue = m_instIdMapBbId[m_circuit->GetId(bTrue)];
                    int bbFalse = m_instIdMapBbId[m_circuit->GetId(bFalse)];
                    VisitBranch(gate, ins[1], bbTrue, bbFalse);
                    break;
                }
                case OpCode::SWITCH_BRANCH: {
                    VisitSwitch(gate, ins[1], outs);
                    break;
                }
                case OpCode::ORDINARY_BLOCK:
                case OpCode::IF_TRUE:
                case OpCode::IF_FALSE:
                case OpCode::SWITCH_CASE:
                case OpCode::DEFAULT_CASE: {
                    int block = m_instIdMapBbId[m_circuit->GetId(gate)];
                    int bbOut = m_instIdMapBbId[m_circuit->GetId(outs[0])];
                    VisitGoto(block, bbOut);
                    break;
                }
                case OpCode::MERGE: {
                    int block = m_instIdMapBbId[m_circuit->GetId(gate)];
                    int bbOut;
                    for (int i = 0; i < static_cast<int>(outs.size()); i++) {
                        bbOut = m_instIdMapBbId[m_circuit->GetId(outs[i])];
                        VisitGoto(block, bbOut);
                    }
                    break;
                }
                case OpCode::LOOP_BEGIN: {
                    int block = m_instIdMapBbId[m_circuit->GetId(gate)];
                    int bbOut;
                    for (int i = 0; i < static_cast<int>(outs.size()); i++) {
                        bbOut = m_instIdMapBbId[m_circuit->GetId(outs[i])];
                        VisitGoto(block, bbOut);
                    }
                    break;
                }
                case OpCode::LOOP_BACK: {
                    int block = m_instIdMapBbId[m_circuit->GetId(gate)];
                    int bbOut = m_instIdMapBbId[m_circuit->GetId(outs[0])];
                    VisitGoto(block, bbOut);
                    break;
                }
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
                case OpCode::DEPEND_SELECTOR:
                    break;
                case OpCode::DEPEND_RELAY:
                    break;
                case OpCode::CALL:
                case OpCode::INT1_CALL:
                case OpCode::INT32_CALL:
                case OpCode::INT64_CALL: {
                    VisitCall(gate, ins);
                    break;
                }
                case OpCode::ALLOCA: {
                    VisitAlloca(gate);
                    break;
                }
                case OpCode::INT1_ARG: // no break, fall through
                case OpCode::INT32_ARG:  // no break, fall through
                case OpCode::INT64_ARG: {
                    VisitParameter(gate);
                    break;
                }
                case OpCode::INT32_CONSTANT: {
                    int32_t value = m_circuit->GetBitField(gate);
                    VisitInt32Constant(gate, value);
                    break;
                }
                case OpCode::JS_CONSTANT: // no break, fall through
                case OpCode::INT64_CONSTANT: {
                    int64_t value = m_circuit->GetBitField(gate);
                    VisitInt64Constant(gate, value);
                    break;
                }
                case OpCode::FLOAT64_CONSTANT: {
                    int64_t value = m_circuit->GetBitField(gate);
                    VisitFloat64Constant(gate, value);
                    break;
                }
                case OpCode::ZEXT_INT1_TO_INT32: {
                    VisitZExtInt(gate, ins[0], MachineRep::K_WORD32);
                    break;
                }
                case OpCode::ZEXT_INT32_TO_INT64:  // no break, fall through
                case OpCode::ZEXT_INT1_TO_INT64: {
                    VisitZExtInt(gate, ins[0], MachineRep::K_WORD64);
                    break;
                }
                case OpCode::SEXT_INT1_TO_INT32: {
                    VisitSExtInt(gate, ins[0], MachineRep::K_WORD32);
                    break;
                }
                case OpCode::SEXT_INT1_TO_INT64:  // no break, fall through
                case OpCode::SEXT_INT32_TO_INT64: {
                    VisitSExtInt(gate, ins[0], MachineRep::K_WORD64);
                    break;
                }
                case OpCode::TRUNC_INT64_TO_INT1:
                case OpCode::TRUNC_INT32_TO_INT1: {
                    VisitCastIntXToIntY(gate, ins[0], MachineRep::K_BIT);
                    break;
                }
                case OpCode::TRUNC_INT64_TO_INT32: {
                    VisitCastIntXToIntY(gate, ins[0], MachineRep::K_WORD32);
                    break;
                }
                case OpCode::INT32_REV:  // no break, fall through
                case OpCode::INT64_REV: {
                    VisitIntRev(gate, ins[0]);
                    break;
                }
                case OpCode::INT32_ADD: {
                    VisitIntAdd(gate, ins[0], ins[1], MachineRep::K_WORD32);
                    break;
                }
                case OpCode::INT64_ADD: {
                    VisitIntAdd(gate, ins[0], ins[1], MachineRep::K_WORD64);
                    break;
                }
                case OpCode::FLOAT64_ADD: {
                    VisitFloatAdd(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::FLOAT64_SUB: {
                    VisitFloatSub(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::FLOAT64_MUL: {
                    VisitFloatMul(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::FLOAT64_DIV: {
                    VisitFloatDiv(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_SUB:  // no break, fall through
                case OpCode::INT64_SUB: {
                    VisitIntSub(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_MUL:  // no break, fall through
                case OpCode::INT64_MUL: {
                    VisitIntMul(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_AND:  // no break, fall through
                case OpCode::INT64_AND: {
                    VisitIntAnd(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_OR:  // no break, fall through
                case OpCode::INT64_OR: {
                    VisitIntOr(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_LSR: // no break, fall through
                case OpCode::INT64_LSR: {
                    VisitIntLsr(gate, ins[0], ins[1]);
                    break;
                }
                case OpCode::INT32_SLT:  // no break, fall through
                case OpCode::INT64_SLT: {
                    VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntSLT);
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
                case OpCode::FLOAT64_EQ: {
                    VisitFloatOrDoubleCmp(gate, ins[0], ins[1], LLVMRealUEQ);
                    break;
                }
                case OpCode::INT32_NE:  // no break, fall through
                case OpCode::INT64_NE: {
                    VisitIntOrUintCmp(gate, ins[0], ins[1], LLVMIntNE);
                    break;
                }
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
                case OpCode::INT32_STORE: {
                    VisitStore(gate, MachineRep::K_WORD32, ins[2], ins[1]); // 2:baseAddr gate, 1:data gate
                    break;
                }
                case OpCode::INT64_STORE: {
                    VisitStore(gate, MachineRep::K_WORD64, ins[2], ins[1]); // 2:baseAddr gate, 1:data gate
                    break;
                }
                case OpCode::INT32_TO_FLOAT64: // no break, fall through
                case OpCode::INT64_TO_FLOAT64: {
                    VisitCastIntToDouble(gate, ins[0]);
                    break;
                }
                case OpCode::FLOAT64_TO_INT64: {
                    VisitCastDoubleToInt(gate, ins[0]);
                    break;
                }
                default: {
                    std::cout << "The gate below need to be translated " << std::endl;
                    m_circuit->Print(gate);
                    UNREACHABLE();
                }
            }
        }
    }
    End();
}

BasicBlock *LLVMIRBuilder::EnsurBasicBlock(int id)
{
    BasicBlock *bb = nullptr;
    if (m_bbIdMapBb.count(id) == 0) {
        auto newBB = std::make_unique<BasicBlock>(id);
        bb = newBB.get();
        m_bbIdMapBb[id] = std::move(newBB);
    } else {
        bb = m_bbIdMapBb[id].get();
    }
    return bb;
}

void LLVMIRBuilder::StartLLVMBuilder(BasicBlock *bb) const
{
    EnsureLLVMBB(bb);
    LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    if ((impl == nullptr) || (impl->llvm_bb_ == nullptr)) {
        std::cerr << "StartLLVMBuilder failed " << std::endl;
        return;
    }
    impl->started = true;
    bb->SetImpl(impl);
    std::cout << "Basicblock id :" << bb->GetId() <<
        "impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>() << std::endl;
    LLVMPositionBuilderAtEnd(m_builder, impl->llvm_bb_);
}

void LLVMIRBuilder::ProcessPhiWorkList()
{
    for (BasicBlock *bb : m_phiRebuildWorklist) {
        auto impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
        for (auto &e : impl->not_merged_phis) {
            BasicBlock *pred = e.pred;
            if (impl->started == 0) {
                std::cerr << " ProcessPhiWorkList error hav't start " << std::endl;
                return;
            }
            LLVMValueRef value = g_values[e.operand];
            if (LLVMTypeOf(value) != LLVMTypeOf(e.phi)) {
                std::cerr << " ProcessPhiWorkList LLVMTypeOf don't match error " << std::endl;
            }
            LLVMBasicBlockRef llvmBB = EnsureLLVMBB(pred);
            LLVMAddIncoming(e.phi, &value, &llvmBB, 1);
        }
        impl->not_merged_phis.clear();
    }
    m_phiRebuildWorklist.clear();
}

void LLVMIRBuilder::EndCurrentBlock() const
{
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->ended = true;
}

void LLVMIRBuilder::End()
{
    ASSERT(!!m_currentBb);
    EndCurrentBlock();
    ProcessPhiWorkList();
    for (auto &it: m_bbIdMapBb) {
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

LLVMBasicBlockRef LLVMIRBuilder::EnsureLLVMBB(BasicBlock *bb) const
{
    LLVMTFBuilderBasicBlockImpl *impl = EnsureLLVMBBImpl(bb);
    if (impl->llvm_bb_) {
        return impl->llvm_bb_;
    }
    std::string buf = "B" + std::to_string(bb->GetId());
    LLVMBasicBlockRef llvmBB = LLVMAppendBasicBlock(m_function, buf.c_str());
    impl->llvm_bb_ = llvmBB;
    impl->continuation = llvmBB;
    bb->SetImpl(impl);
    std::cout << std::endl
              << "create LLVMBB = " << buf << " impl:" << bb->GetImpl<LLVMTFBuilderBasicBlockImpl>() << std::endl;
    return llvmBB;
}

LLVMTypeRef LLVMIRBuilder::GetMachineRepType(MachineRep rep) const
{
    LLVMTypeRef dstType;
    switch (rep) {
        case MachineRep::K_BIT:
            dstType = LLVMInt1TypeInContext(m_context);
            break;
        case MachineRep::K_WORD8:
            dstType = LLVMInt8TypeInContext(m_context);
            break;
        case MachineRep::K_WORD32:
            dstType = LLVMInt32TypeInContext(m_context);
            break;
        case MachineRep::K_FLOAT64:
            dstType = LLVMDoubleTypeInContext(m_context);
            break;
        default: // 64bit int goes to default scenario
            dstType = LLVMInt64TypeInContext(m_context);
            break;
    }
    return dstType;
}

void LLVMIRBuilder::VisitCall(AddrShift gate, const std::vector<AddrShift> &inList)
{
    constexpr int paraStartIndex = 2;
    int index = m_circuit->GetBitField(inList[1]);
    LLVMValueRef callee = reinterpret_cast<LLVMValueRef>(FastStubs::GetInstance().GetFastStub(index));
    LLVMValueRef params[16]; // 16: number of param
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        AddrShift gateTmp = inList[paraIdx];
        params[paraIdx - paraStartIndex] = g_values[gateTmp];
        m_circuit->Print(gateTmp);
        std::cout << "arg" << paraIdx - paraStartIndex << " th " << std::endl;
        LLVMDumpValue(params[paraIdx - paraStartIndex]);
        std::cout << std::endl;
    }
    if (callee == nullptr) {
        std::cout << "callee nullptr" << std::endl;
        return;
    }
    g_values[gate] = LLVMBuildCall(m_builder, callee, params, inList.size() - paraStartIndex, "");
    return;
}

void LLVMIRBuilder::VisitAlloca(AddrShift gate)
{
    std::cout << " VisitAlloca " << std::endl;
    uint64_t sizeEnum = m_circuit->GetBitField(gate);
    LLVMTypeRef sizeType = GetMachineRepType(static_cast<MachineRep>(sizeEnum));
    std::cout << LLVMGetTypeKind(sizeType) << std::endl;
    g_values[gate] = LLVMBuildPtrToInt(m_builder, LLVMBuildAlloca(m_builder, sizeType, ""),
        LLVMInt64Type(), ""); // NOTE: pointer val is currently viewed as 64bits
    return;
}

void LLVMIRBuilder::VisitPhi(AddrShift gate, const std::vector<AddrShift> &srcGates, MachineRep rep)
{
    std::cout << "--------------- VisitPhi ------------------------" << std::endl;
    LLVMTypeRef type = GetMachineRepType(rep);
    LLVMValueRef phi = LLVMBuildPhi(m_builder, type, "");
    std::vector<AddrShift> relMergeIns = m_circuit->GetInVector(srcGates[0]);
    bool addToPhiRebuildList = false;
    for (int i = 1; i < static_cast<int>(srcGates.size()); i++) {
        GateId gateId = m_circuit->GetId(relMergeIns[i - 1]);
        int bbIdx = m_instIdMapBbId[gateId];
        std::cout << "srcGate: " << srcGates[i] << " dominated gateId:" << gateId << "dominated bbIdx: " << bbIdx
                  << std::endl;
        int cnt = m_bbIdMapBb.count(bbIdx);
        // if cnt = 0 means bb with current bbIdx hasn't been created
        if (cnt > 0) {
            BasicBlock *bb = m_bbIdMapBb[bbIdx].get();
            std::cout << "bb : " << bb << std::endl;
            if (bb == nullptr) {
                std::cerr << "VisitPhi failed BasicBlock nullptr" << std::endl;
                return;
            }
            LLVMTFBuilderBasicBlockImpl *impl = bb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
            if (impl == nullptr) {
                std::cerr << "VisitPhi failed impl nullptr" << std::endl;
                return;
            }
            LLVMBasicBlockRef llvmBB = EnsureLLVMBB(bb);  // The llvm bb
            LLVMValueRef value = g_values[srcGates[i]];
            if (impl->started) {
                LLVMAddIncoming(phi, &value, &llvmBB, 1);
            } else {
                addToPhiRebuildList = true;
                impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
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
            m_phiRebuildWorklist.push_back(m_currentBb);
        }
        g_values[gate] = phi;
    }
    std::cout << "+++++++++++++++ VisitPhi ++++++++++++++++++++++++" << std::endl;
}

void LLVMIRBuilder::VisitReturn(AddrShift gate, AddrShift popCount, const std::vector<AddrShift> &operands) const
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    std::cout << "VisitReturn  -" << std::endl;
    AddrShift operand = operands[2]; // 2: skip 2 in gate that are not data gate
    std::cout << "VisitReturn gate: " << gate << " popCount: " << popCount << std::endl;
    std::cout << "VisitReturn return: " << operand << " gateId: " << m_circuit->GetId(operand) << std::endl;
    LLVMValueRef returnValue = g_values[operand];
    LLVMDumpValue(returnValue);
    std::cout << std::endl;
    LLVMBuildRet(m_builder, returnValue);
    std::cout << "VisitReturn  +" << std::endl;
}

void LLVMIRBuilder::VisitBlock(int gate, const OperandsVector &predecessors)  // NOLINTNEXTLINE(misc-unused-parameters)
{
    std::cout << "VisitBlock BBIdx:" << gate << "  -" << std::endl;
    BasicBlock *bb = EnsurBasicBlock(gate);
    if (bb == nullptr) {
        std::cerr << "StartLLVMBuilder failed " << std::endl;
        return;
    }
    m_currentBb = bb;
    LLVMBasicBlockRef llvmbb = EnsureLLVMBB(bb);
    StartLLVMBuilder(bb);
    std::cout << "predecessors :";
    for (int predecessor : predecessors) {
        BasicBlock *pre = EnsurBasicBlock(predecessor);
        if (pre == nullptr) {
            std::cerr << "StartLLVMBuilder failed  predecessor:%d nullptr" << predecessor << std::endl;
            return;
        }
        LLVMBasicBlockRef llvmpre = EnsureLLVMBB(pre);
        std::cout << "  " << predecessor;
        LLVMMoveBasicBlockBefore(llvmpre, llvmbb);
    }
    std::cout << "VisitBlock BBIdx:" << gate << "  +" << std::endl;
}

void LLVMIRBuilder::VisitGoto(int block, int bbOut)
{
    if (block == bbOut) {
        return;
    }
    BasicBlock *bb = EnsurBasicBlock(bbOut);
    if (bb == nullptr) {
        std::cerr << "StartLLVMBuilder failed " << std::endl;
        return;
    }
    llvm::BasicBlock *self = llvm::unwrap(EnsureLLVMBB(m_bbIdMapBb[block].get()));
    llvm::BasicBlock *out = llvm::unwrap(EnsureLLVMBB(m_bbIdMapBb[bbOut].get()));
    llvm::BranchInst::Create(out, self);
    EndCurrentBlock();
}

void LLVMIRBuilder::VisitInt32Constant(AddrShift gate, int32_t value) const
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt32Type(), value, 0);
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->values_[gate] = llvmValue;
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    std::cout << "VisitInt32Constant set gate:" << gate << "  value:" << value << " impl:" << impl << std::endl;
    std::cout << "VisitInt32Constant " << str << std::endl;
}

void LLVMIRBuilder::VisitInt64Constant(AddrShift gate, int64_t value) const
{
    LLVMValueRef llvmValue = LLVMConstInt(LLVMInt64Type(), value, 0);
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->values_[gate] = llvmValue;
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    std::cout << "VisitInt64Constant set gate:" << gate << "  value:" << value << " impl:" << impl << std::endl;
    std::cout << "VisitInt64Constant " << str << std::endl;
}

void LLVMIRBuilder::VisitFloat64Constant(AddrShift gate, int64_t value) const
{
    LLVMValueRef llvmValue = LLVMConstReal(LLVMDoubleType(), value);
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->values_[gate] = llvmValue;
    g_values[gate] = llvmValue;
    char *str = LLVMPrintValueToString(llvmValue);
    std::cout << "VisitFloat64Constant set gate:" << gate << "  value:" << value << " impl:" << impl << std::endl;
    std::cout << "VisitFloat64Constant " << str << std::endl;
}

void LLVMIRBuilder::VisitParameter(AddrShift gate) const
{
    int argth = m_circuit->LoadGatePtrConst(gate)->GetBitField();
    std::cout << "VisitParameter " << argth << "th parameter -" << std::endl;
    LLVMValueRef value = LLVMGetParam(m_function, argth);
    LLVMDumpValue(value);
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    impl->values_[gate] = value;
    g_values[gate] = value;
    std::cout << "VisitParameter set gate:" << gate << "  value:" << value << " impl:" << impl << std::endl;
    // NOTE: caller put args, otherwise crash
    if (value == nullptr) {
        std::cerr << "VisitParameter arg" << argth << "th nullptr" << std::endl;
        return;
    }
    LLVMDumpValue(value);
    char *str = LLVMPrintValueToString(value);
    if (str != nullptr) {
        std::cout << "VisitParameter arg" << argth << "th value = " << str << std::endl;
    }
    std::cout << "VisitParameter " << argth << "th parameter +" << std::endl;
}

void LLVMIRBuilder::VisitBranch(AddrShift gate, AddrShift cmp, int btrue, int bfalse)
{
    std::cout << "VisitBranch  -  cmp:" << cmp << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = EnsureLLVMBBImpl(m_currentBb);
    if ((impl->values_.count(cmp) == 0) && (g_values.count(cmp) == 0)) {
        std::cerr << "VisitBranch cmp empty !" << std::endl;
        return;
    }
    LLVMValueRef cond = g_values[cmp];

    BasicBlock *trueBB = EnsurBasicBlock(btrue);
    BasicBlock *falseBB = EnsurBasicBlock(bfalse);
    EnsureLLVMBB(trueBB);
    EnsureLLVMBB(falseBB);

    LLVMBasicBlockRef llvmTrueBB = trueBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMBasicBlockRef llvmFalseBB = falseBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
    LLVMValueRef result = LLVMBuildCondBr(m_builder, cond, llvmTrueBB, llvmFalseBB);
    EndCurrentBlock();
    g_values[gate] = result;
    std::cout << "VisitBranch  + " << std::endl;
}

void LLVMIRBuilder::VisitSwitch(AddrShift gate, AddrShift input, const std::vector<AddrShift> &outList)
{
    std::cout << "VisitSwitch  - " << std::endl;
    LLVMValueRef cond = g_values[input];
    unsigned caseNum = outList.size();
    BasicBlock *curOutBB = nullptr;
    LLVMBasicBlockRef llvmDefaultOutBB = nullptr;
    for (int i = 0; i < static_cast<int>(caseNum); i++) {
        curOutBB = EnsurBasicBlock(m_instIdMapBbId[m_circuit->GetId(outList[i])]);
        EnsureLLVMBB(curOutBB);
        if (m_circuit->GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            llvmDefaultOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        }
    }
    LLVMValueRef result = LLVMBuildSwitch(m_builder, cond, llvmDefaultOutBB, caseNum - 1);
    LLVMBasicBlockRef llvmCurOutBB = nullptr;
    for (int i = 0; i < static_cast<int>(caseNum - 1); i++) {
        if (m_circuit->GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        curOutBB = EnsurBasicBlock(m_instIdMapBbId[m_circuit->GetId(outList[i])]);
        llvmCurOutBB = curOutBB->GetImpl<LLVMTFBuilderBasicBlockImpl>()->llvm_bb_;
        LLVMAddCase(result, LLVMConstInt(LLVMInt64Type(), m_circuit->GetBitField(outList[i]), 0), llvmCurOutBB);
    }
    EndCurrentBlock();
    g_values[gate] = result;
    std::cout << "VisitSwitch  + " << std::endl;
}

void LLVMIRBuilder::VisitLoad(AddrShift gate, MachineRep rep, AddrShift base) const
{
    std::cout << "VisitLoad base:" << base << std::endl;

    LLVMValueRef baseAddr = g_values[base];
    if (LLVMGetTypeKind(LLVMTypeOf(baseAddr)) == LLVMIntegerTypeKind) {
        baseAddr = LLVMBuildIntToPtr(m_builder, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    }
    baseAddr = LLVMBuildPointerCast(m_builder, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    LLVMValueRef value = LLVMBuildLoad(m_builder, baseAddr, "");
    g_values[gate] = value;
    std::cout << "VisitLoad value:" << value << " "
              << "value type" << LLVMTypeOf(value) << std::endl;
}

void LLVMIRBuilder::VisitStore(AddrShift gate, MachineRep rep, AddrShift base, AddrShift dataToStore) const
{
    std::cout << "VisitStore base:" << base << std::endl;
    LLVMValueRef baseAddr = g_values[base];
    if (LLVMGetTypeKind(LLVMTypeOf(baseAddr)) == LLVMIntegerTypeKind) {
        baseAddr = LLVMBuildIntToPtr(m_builder, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    }
    baseAddr = LLVMBuildPointerCast(m_builder, baseAddr, LLVMPointerType(GetMachineRepType(rep), 0), "");
    LLVMValueRef value = LLVMBuildStore(m_builder, g_values[dataToStore], baseAddr);
    g_values[gate] = value;
    std::cout << "VisitStore value:" << value << " "
              << "value type" << LLVMTypeOf(value) << std::endl;
}
void LLVMIRBuilder::VisitIntOrUintCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMIntPredicate opcode) const
{
    std::cout << "VisitIntOrUintCmp  -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitIntOrUintCmp get gate:" << e1 << "  value:" << g_values[e1] << " impl:" << impl << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;

    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildICmp(m_builder, opcode, e1Value, e2Value, "");

    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntOrUintCmp set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitIntOrUintCmp  +" << std::endl;
}

void LLVMIRBuilder::VisitFloatOrDoubleCmp(AddrShift gate, AddrShift e1, AddrShift e2, LLVMRealPredicate opcode) const
{
    std::cout << "VisitFloatOrDoubleCmp  -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitFloatOrDouble get gate:" << e1 << "  value:" << g_values[e1] << " impl:" << impl << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildFCmp(m_builder, opcode, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitFloatOrDoubleCmp set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitFloatOrDoubleCmp  +" << std::endl;
}

void LLVMIRBuilder::VisitIntRev(AddrShift gate, AddrShift e1) const
{
    std::cout << "VisitIntRev  -" << std::endl;
    std::cout << "VisitIntRev get gate:" << e1 << " value:" << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << std::endl;
    LLVMValueRef result = LLVMBuildNeg(m_builder, e1Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << std::endl;
    std::cout << "VisitIntRev set gate:" << gate << " value:" << result << std::endl;
    std::cout << "VisitIntRev  +" << std::endl;
}

void LLVMIRBuilder::VisitIntAdd(AddrShift gate, AddrShift e1, AddrShift e2, MachineRep rep) const
{
    std::cout << "VisitIntAdd  -" << std::endl;
    std::cout << "VisitIntAdd get gate:" << e1 << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    if (LLVMGetTypeKind(LLVMTypeOf(e1Value)) == LLVMPointerTypeKind) {  // for scenario: pointer + offset
        e1Value = LLVMBuildPtrToInt(m_builder, e1Value, GetMachineRepType(rep), "");
    }
    LLVMValueRef result = LLVMBuildAdd(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntAdd set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitIntAdd  +" << std::endl;
}

void LLVMIRBuilder::VisitFloatAdd(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitFloatAdd  -" << std::endl;
    std::cout << "VisitFloatAdd get gate:" << e1 << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildFAdd(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitFloatAdd set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitFloatAdd  +" << std::endl;
}

void LLVMIRBuilder::VisitFloatSub(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitFloatSub  -" << std::endl;
    std::cout << "VisitFloatSub get gate:" << gate << std::endl;
    std::cout << " LLVMDumpValue e1 " << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildFSub(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitFloatSub set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitFloatSub  +" << std::endl;
}

void LLVMIRBuilder::VisitFloatMul(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitFloatMul  -" << std::endl;
    std::cout << "VisitFloatMul get gate:" << gate << std::endl;
    std::cout << " LLVMDumpValue e1 " << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildFMul(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitFloatMul set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitFloatMul  +" << std::endl;
}

void LLVMIRBuilder::VisitFloatDiv(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitFloatDiv  -" << std::endl;
    std::cout << "VisitFloatDiv get gate:" << gate << std::endl;
    std::cout << " LLVMDumpValue e1 " << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildFDiv(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitFloatDiv set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitFloatDiv  +" << std::endl;
}

void LLVMIRBuilder::VisitIntSub(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitIntSub  -" << std::endl;
    std::cout << "VisitIntSub get gate:" << e1 << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildSub(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntSub set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitIntSub  +" << std::endl;
}

void LLVMIRBuilder::VisitIntMul(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitIntMul  -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitIntMul get gate:" << e1 << "  value:" << (impl->values_[e1]) << " impl:" << impl << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;

    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildMul(m_builder, e1Value, e2Value, "");

    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntMul set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitIntMul  +" << std::endl;
}

void LLVMIRBuilder::VisitIntOr(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitIntOr  -" << std::endl;
    std::cout << "VisitIntOr get gate:" << e1 << std::endl;
    std::cout << " LLVMDumpValue e1 " << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildOr(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntOr set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitIntOr  +" << std::endl;
}

void LLVMIRBuilder::VisitIntAnd(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitIntAnd  -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitIntAnd get gate:" << e1 << "  value:" << (impl->values_[e1]) << " impl:" << impl << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;

    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildAnd(m_builder, e1Value, e2Value, "");

    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntAnd set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitIntAnd  +" << std::endl;
}

void LLVMIRBuilder::VisitIntLsr(AddrShift gate, AddrShift e1, AddrShift e2) const
{
    std::cout << "VisitIntLsr  -" << std::endl;
    std::cout << "VisitIntLsr get gate:" << e1 << std::endl;
    std::cout << " LLVMDumpValue e1 " << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    std::cout << " LLVMDumpValue e2 " << std::endl;
    LLVMValueRef e2Value = g_values[e2];
    LLVMDumpValue(e2Value);
    LLVMValueRef result = LLVMBuildLShr(m_builder, e1Value, e2Value, "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitIntLsr set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitIntLsr  +" << std::endl;
}
void LLVMIRBuilder::VisitZExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    std::cout << "VisitZExtInt -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitZExtInt get gate:" << e1 << "value:" << (impl->values_[e1]) << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    LLVMValueRef result = LLVMBuildZExt(m_builder, e1Value, GetMachineRepType(rep), "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitZExtInt set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitZExtInt +" << std::endl;
}

void LLVMIRBuilder::VisitSExtInt(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    std::cout << "VisitSExtInt -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitSExtInt get gate:" << e1 << "value:" << (impl->values_[e1]) << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    LLVMValueRef result = LLVMBuildSExt(m_builder, e1Value, GetMachineRepType(rep), "");

    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitSExtInt set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitSExtInt +" << std::endl;
}

void LLVMIRBuilder::VisitCastIntXToIntY(AddrShift gate, AddrShift e1, MachineRep rep) const
{
    std::cout << "VisitCastIntXToIntY -" << std::endl;
    LLVMTFBuilderBasicBlockImpl *impl = m_currentBb->GetImpl<LLVMTFBuilderBasicBlockImpl>();
    std::cout << "VisitCastIntXToIntY get gate:" << e1 << "value:" << (impl->values_[e1]) << std::endl;

    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    LLVMValueRef result = LLVMBuildIntCast2(m_builder, e1Value, GetMachineRepType(rep), 1, "");

    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitCastIntXToIntY set gate:" << gate << "  value:" << result << " impl:" << impl << std::endl;
    std::cout << "VisitCastIntXToIntY +" << std::endl;
}

void LLVMIRBuilder::VisitCastIntToDouble(AddrShift gate, AddrShift e1) const
{
    std::cout << "VisitCastIntToDouble -" << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    LLVMValueRef result = LLVMBuildSIToFP(m_builder, e1Value, LLVMDoubleType(), "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitCastIntToDouble set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitCastIntToDouble +" << std::endl;
}

void LLVMIRBuilder::VisitCastDoubleToInt(AddrShift gate, AddrShift e1) const
{
    std::cout << "VisitCastDoubleToInt -" << std::endl;
    LLVMValueRef e1Value = g_values[e1];
    LLVMDumpValue(e1Value);
    LLVMValueRef result = LLVMBuildFPToSI(m_builder, e1Value, LLVMInt64Type(), "");
    g_values[gate] = result;
    LLVMDumpValue(result);
    std::cout << "VisitCastDoubleToInt set gate:" << gate << "  value:" << result << std::endl;
    std::cout << "VisitCastDoubleToInt +" << std::endl;
}
}  // namespace kungfu
