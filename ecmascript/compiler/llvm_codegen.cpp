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

#include "llvm_codegen.h"
#include <string>
#include <vector>
#include "ecmascript/compiler/compiler_macros.h"
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/object_factory.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/DisassemblerTypes.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/ADT/APInt.h"
#include "llvm/CodeGen/BuiltinGCs.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/llvm_stackmap_parser.h"
#include "stub_descriptor.h"

using namespace panda::ecmascript;
namespace panda::ecmascript::kungfu {
void LLVMCodeGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index,
                                                const CompilationConfig *cfg)
{
    auto function = module_->GetStubFunction(index);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg);
    builder.Build();
}

void LLVMModuleAssembler::AssembleModule()
{
    assembler_.Run();
}

void LLVMModuleAssembler::AssembleStubModule(StubModule *module)
{
    auto codeBuff = reinterpret_cast<uintptr_t>(assembler_.GetCodeBuffer());
    auto engine = assembler_.GetEngine();
    std::map<uint64_t, std::string> addr2name;
    for (int i = 0; i < ALL_STUB_MAXCOUNT; i++) {
        auto stubfunction = stubmodule_->GetStubFunction(i);
#ifndef NDEBUG
        COMPILER_LOG(DEBUG) << "  AssembleStubModule :" << i << " th " << std::endl;
#endif
        if (stubfunction != nullptr) {
            uintptr_t stubEntry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, stubfunction));
            module->SetStubEntry(i, stubEntry - codeBuff);
            addr2name[stubEntry] = FastStubDescriptors::GetInstance().GetStubDescriptor(i)->GetName();
#ifndef NDEBUG
            COMPILER_LOG(DEBUG) << "name : " << addr2name[codeBuff] << std::endl;
#endif
        }
    }
    module->SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    module->SetStackMapAddr(reinterpret_cast<uintptr_t>(assembler_.GetStackMapsSection()));
    module->SetStackMapSize(assembler_.GetStackMapsSize());
#ifndef NDEBUG
    assembler_.Disassemble(addr2name);
#endif
}

static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    COMPILER_LOG(DEBUG) << "RoundTripAllocateCodeSection object " << object << " - ";
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    uint8_t *addr = state.AllocaCodeSection(size, sectionName);
    COMPILER_LOG(DEBUG) << "RoundTripAllocateCodeSection  addr:" << std::hex <<
        reinterpret_cast<std::uintptr_t>(addr) << addr << " size:0x" << size << " + ";
    return addr;
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                             [[maybe_unused]] LLVMBool isReadOnly)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory(void *object, [[maybe_unused]] char **errMsg)
{
    COMPILER_LOG(DEBUG) << "RoundTripFinalizeMemory object " << object << " - ";
    return 0;
}

static void RoundTripDestroy(void *object)
{
    COMPILER_LOG(DEBUG) << "RoundTripDestroy object " << object << " - ";
}

void LLVMAssembler::UseRoundTripSectionMemoryManager()
{
    auto sectionMemoryManager = std::make_unique<llvm::SectionMemoryManager>();
    options_.MCJMM =
        LLVMCreateSimpleMCJITMemoryManager(&codeInfo_, RoundTripAllocateCodeSection,
            RoundTripAllocateDataSection, RoundTripFinalizeMemory, RoundTripDestroy);
}

bool LLVMAssembler::BuildMCJITEngine()
{
    COMPILER_LOG(DEBUG) << " BuildMCJITEngine  - ";
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&engine_, module_, &options_, sizeof(options_), &error_);
    if (ret) {
        LOG_ECMA(FATAL) << "error_ : " << error_;
        return false;
    }
    COMPILER_LOG(DEBUG) << " BuildMCJITEngine  + ";
    return true;
}

void LLVMAssembler::RewritePatchPointIdOfStatePoint(LLVMValueRef instruction, uint64_t &callInsNum, uint64_t &funcNum)
{
    if (LLVMIsACallInst(instruction) && !LLVMIsAIntrinsicInst(instruction)) {
        const char attrName[] = "statepoint-id";
        uint64_t id =  (funcNum << 16) | callInsNum;
        std::string patchId = std::to_string(id);
        LLVMAttributeRef attr = LLVMCreateStringAttribute(LLVMGetGlobalContext(),
            attrName, sizeof(attrName) - 1, patchId.c_str(), patchId.size());
        LLVMAddCallSiteAttribute(instruction, LLVMAttributeFunctionIndex, attr);
        callInsNum++;
        LLVMValueRef targetStoreInst = LLVMGetPreviousInstruction(LLVMGetPreviousInstruction(
            LLVMGetPreviousInstruction(instruction)));
        if (LLVMGetInstructionOpcode(targetStoreInst) == LLVMStore) {
            LLVMSetOperand(targetStoreInst, 0, LLVMConstInt(LLVMInt64Type(), id, 0));
        }
    }
}

void LLVMAssembler::FillPatchPointIDs()
{
    LLVMValueRef func;
    uint64_t funcNum = 0;
    func = LLVMGetFirstFunction(module_);
    while (func) {
        if (LLVMIsDeclaration(func)) {
            func = LLVMGetNextFunction(func);
            funcNum++;
            continue;
        }
        LLVMBasicBlockRef bb;
        LLVMValueRef instruction;
        unsigned instructionNum = 0;
        unsigned bbNum = 0;
        uint64_t callInsNum = 0;
        for (bb = LLVMGetFirstBasicBlock(func); bb; bb = LLVMGetNextBasicBlock(bb)) {
            bbNum++;
            for (instruction = LLVMGetFirstInstruction(bb); instruction;
                instruction = LLVMGetNextInstruction(instruction)) {
                instructionNum++;
                RewritePatchPointIdOfStatePoint(instruction, callInsNum, funcNum);
            }
        }
        func = LLVMGetNextFunction(func);
        funcNum++;
    }
}

void LLVMAssembler::BuildAndRunPasses()
{
    COMPILER_LOG(DEBUG) << "BuildAndRunPasses  - ";
    LLVMPassManagerBuilderRef pmBuilder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(pmBuilder, 3); // using O3 optimization level
    LLVMPassManagerBuilderSetSizeLevel(pmBuilder, 0);
    LLVMPassManagerRef funcPass = LLVMCreateFunctionPassManagerForModule(module_);
    LLVMPassManagerRef modPass = LLVMCreatePassManager();
    LLVMPassManagerBuilderPopulateFunctionPassManager(pmBuilder, funcPass);
    llvm::unwrap(modPass)->add(llvm::createRewriteStatepointsForGCLegacyPass());
    LLVMPassManagerBuilderPopulateModulePassManager(pmBuilder, modPass);
    LLVMPassManagerBuilderDispose(pmBuilder);
    LLVMInitializeFunctionPassManager(funcPass);
    for (LLVMValueRef fn = LLVMGetFirstFunction(module_); fn; fn = LLVMGetNextFunction(fn)) {
        LLVMRunFunctionPassManager(funcPass, fn);
    }
    FillPatchPointIDs();
    LLVMFinalizeFunctionPassManager(funcPass);
    LLVMRunPassManager(modPass, module_);
    LLVMDisposePassManager(funcPass);
    LLVMDisposePassManager(modPass);
    COMPILER_LOG(DEBUG) << "BuildAndRunPasses  + ";
}

LLVMAssembler::LLVMAssembler(LLVMModuleRef module)
    : module_(module)
{
    Initialize();
}

LLVMAssembler::~LLVMAssembler()
{
    if (engine_ != nullptr) {
        if (module_ != nullptr) {
            char *error = nullptr;
            LLVMRemoveModule(engine_, module_, &module_, &error);
            if (error != nullptr) {
                LLVMDisposeMessage(error);
            }
        }
        LLVMDisposeExecutionEngine(engine_);
        engine_ = nullptr;
    }
    module_ = nullptr;
    error_ = nullptr;
}

void LLVMAssembler::Run()
{
    char *error = nullptr;
#if ECMASCRIPT_ENABLE_COMPILER_LOG
    char *info = LLVMPrintModuleToString(module_);
    COMPILER_LOG(INFO) << "Current Module: " << info;
    LLVMDisposeMessage(info);
    LLVMDumpModule(module_);
#endif
    LLVMPrintModuleToFile(module_, "stub.ll", &error);
    LLVMVerifyModule(module_, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    BuildAndRunPasses();
    LLVMPrintModuleToFile(module_, "opt_stub.ll", &error);
}

void LLVMAssembler::Initialize()
{
    std::string triple(LLVMGetTarget(module_));
    if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        /* this method must be called, ohterwise "Target does not support MC emission" */
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86Target();
    } else if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64Disassembler();
        LLVMInitializeAArch64AsmPrinter();
        LLVMInitializeAArch64AsmParser();
        LLVMInitializeAArch64Target();
    } else if (triple.compare("arm-unknown-linux-gnu") == 0) {
        LLVMInitializeARMTargetInfo();
        LLVMInitializeARMTargetMC();
        LLVMInitializeARMDisassembler();
        LLVMInitializeARMAsmPrinter();
        LLVMInitializeARMAsmParser();
        LLVMInitializeARMTarget();
    } else {
        UNREACHABLE();
    }
    llvm::linkAllBuiltinGCs();
    LLVMInitializeMCJITCompilerOptions(&options_, sizeof(options_));
    options_.OptLevel = 2; // opt level 2
    // Just ensure that this field still exists.
    options_.NoFramePointerElim = true;
}

#if ECMASCRIPT_ENABLE_COMPILER_LOG
static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                        uint64_t *referenceType, [[maybe_unused]]uint64_t referencePC,
                                        [[maybe_unused]] const char **referenceName)
{
    *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
    return nullptr;
}
#endif

void LLVMAssembler::Disassemble(std::map<uint64_t, std::string> addr2name) const
{
#if ECMASCRIPT_ENABLE_COMPILER_LOG
    LLVMDisasmContextRef dcr = LLVMCreateDisasm(LLVMGetTarget(module_), nullptr, 0, nullptr, SymbolLookupCallback);
    std::cout << "========================================================================" << std::endl;
    for (auto it : codeInfo_.GetCodeInfo()) {
        uint8_t *byteSp;
        uintptr_t numBytes;
        byteSp = it.first;
        numBytes = it.second;
        std::cout << " byteSp:" << std::hex << reinterpret_cast<std::uintptr_t>(byteSp) << "  numBytes:0x" << numBytes
                  << std::endl;

        unsigned pc = 0;
        const char outStringSize = 100;
        char outString[outStringSize];
        while (numBytes != 0) {
            size_t InstSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            if (InstSize == 0) {
                std::cerr.fill('0');
                std::cerr.width(8); // 8:fixed hex print width
                std::cerr << std::hex << pc << ":";
                std::cerr.width(8); // 8:fixed hex print width
                std::cerr << std::hex << *reinterpret_cast<uint32_t *>(byteSp) << "maybe constant"  << std::endl;
                pc += 4; // 4 pc length
                byteSp += 4; // 4 sp offset
                numBytes -= 4; // 4 num bytes
            }
            uint64_t addr = reinterpret_cast<uint64_t>(byteSp);
            if (addr2name.find(addr) != addr2name.end()) {
                std::cout << addr2name[addr].c_str() << ":" << std::endl;
            }
            std::cerr.fill('0');
            std::cerr.width(8); // 8:fixed hex print width
            std::cerr << std::hex << pc << ":";
            std::cerr.width(8); // 8:fixed hex print width
            std::cerr << std::hex << *reinterpret_cast<uint32_t *>(byteSp) << " " << outString << std::endl;
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
    }
    std::cout << "========================================================================" << std::endl;
    LLVMDisasmDispose(dcr);
#endif
}
}  // namespace panda::ecmascript::kungfu
