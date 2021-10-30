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
#include "llvm/llvm_stackmap_parser.h"
#include <vector>
#include "ecmascript/object_factory.h"
#include "stub_descriptor.h"
#include "ecmascript/ecma_macros.h"
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
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/DisassemblerTypes.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"

using namespace panda::ecmascript;
namespace kungfu {
void LLVMCodeGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, int index)
{
    auto function = module_->GetStubFunction(index);

    LLVMIRBuilder builder(&graph, circuit, module_, function);
    builder.Build();
}

void LLVMModuleAssembler::AssembleModule()
{
    assembler_.Run();
}

void LLVMModuleAssembler::AssembleStubModule(StubModule *module)
{
    auto codeBuff = reinterpret_cast<Address>(assembler_.GetCodeBuffer());
    auto engine = assembler_.GetEngine();
    std::map<uint64_t, std::string> addr2name;
    for (int i = 0; i < FAST_STUB_MAXCOUNT; i++) {
        auto stubfunction = stubmodule_->GetStubFunction(i);
        LOG_ECMA(INFO) << "  AssembleStubModule :" << i << " th " << std::endl;
        if (stubfunction != nullptr) {
            Address stubEntry = reinterpret_cast<Address>(LLVMGetPointerToGlobal(engine, stubfunction));
            module->SetStubEntry(i, stubEntry - codeBuff);
            addr2name[stubEntry] = GET_STUBDESCRIPTOR_BY_ID(i)->GetName();
            LOG_ECMA(INFO) << "name : " << addr2name[codeBuff] << std::endl;
        }
    }
    module->SetHostCodeSectionAddr(codeBuff);
    // stackmaps ptr and size
    module->SetStackMapAddr(reinterpret_cast<Address>(assembler_.GetStackMapsSection()));
    module->SetStackMapSize(assembler_.GetStackMapsSize());
    assembler_.Disassemble(addr2name);
}

static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    LOG_ECMA(INFO) << "RoundTripAllocateCodeSection object " << object << " - ";
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    uint8_t *addr = state.AllocaCodeSection(size, sectionName);
    LOG_ECMA(INFO) << "RoundTripAllocateCodeSection  addr:" << std::hex << reinterpret_cast<std::uintptr_t>(addr) <<
        addr << " size:0x" << size << " + ";
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
    LOG_ECMA(INFO) << "RoundTripFinalizeMemory object " << object << " - ";
    return 0;
}

static void RoundTripDestroy(void *object)
{
    LOG_ECMA(INFO) << "RoundTripDestroy object " << object << " - ";
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
    LOG_ECMA(INFO) << " BuildMCJITEngine  - ";
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&engine_, module_, &options_, sizeof(options_), &error_);
    if (ret) {
        LOG_ECMA(ERROR) << "error_ : " << error_;
        return false;
    }
    LOG_ECMA(INFO) << " BuildMCJITEngine  + ";
    return true;
}

void LLVMAssembler::BuildAndRunPasses() const
{
    std::cout << "BuildAndRunPasses  - ";
    LLVMPassManagerRef pm = LLVMCreatePassManager();
    LLVMAddConstantPropagationPass(pm);
    LLVMAddInstructionCombiningPass(pm);
    llvm::unwrap(pm)->add(llvm::createRewriteStatepointsForGCLegacyPass());
    char *info = LLVMPrintModuleToString(module_);
    std::cout << "Current Module: " << info;
    LLVMDumpModule(module_);
    LLVMDisposeMessage(info);
    LLVMRunPassManager(pm, module_);
    LLVMDisposePassManager(pm);
    std::cout << "BuildAndRunPasses  + ";
}

LLVMAssembler::LLVMAssembler(LLVMModuleRef module, const char* triple): module_(module), engine_(nullptr),
    hostTriple_(triple), error_(nullptr)
{
    Initialize();
}

LLVMAssembler::~LLVMAssembler()
{
    module_ = nullptr;
    if (engine_ != nullptr) {
        LLVMDisposeExecutionEngine(engine_);
    }
    hostTriple_ = "";
    error_ = nullptr;
}

void LLVMAssembler::Run()
{
    char *error = nullptr;
    LLVMVerifyModule(module_, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    BuildAndRunPasses();
}

void LLVMAssembler::Initialize()
{
    if (hostTriple_.compare(AMD64_TRIPLE) == 0) {
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        /* this method must be called, ohterwise "Target does not support MC emission" */
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86Target();
    } else if (hostTriple_.compare(ARM64_TRIPLE) == 0) {
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64Disassembler();
        LLVMInitializeAArch64AsmPrinter();
        LLVMInitializeAArch64AsmParser();
        LLVMInitializeAArch64Target();
    } else if (hostTriple_.compare(ARM32_TRIPLE) == 0) {
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

static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                        uint64_t *referenceType, [[maybe_unused]]uint64_t referencePC,
                                        [[maybe_unused]] const char **referenceName)
{
    *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
    return nullptr;
}

void LLVMAssembler::Disassemble(std::map<uint64_t, std::string> addr2name) const
{
    LLVMDisasmContextRef dcr = LLVMCreateDisasm(hostTriple_.c_str(), nullptr, 0, nullptr, SymbolLookupCallback);
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
                fprintf(stderr, "%08x: %08x maybe constant\n", pc, *reinterpret_cast<uint32_t *>(byteSp));
                pc += 4; // 4 pc length
                byteSp += 4; // 4 sp offset
                numBytes -= 4; // 4 num bytes
            }
            uint64_t addr = reinterpret_cast<uint64_t>(byteSp);
            if (addr2name.find(addr) != addr2name.end()) {
                std::cout << addr2name[addr].c_str() << ":" << std::endl;
            }
            (void)fprintf(stderr, "%08x: %08x %s\n", pc, *reinterpret_cast<uint32_t *>(byteSp), outString);
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
    }
    std::cout << "========================================================================" << std::endl;
    LLVMDisasmDispose(dcr);
}
}  // namespace kungfu
