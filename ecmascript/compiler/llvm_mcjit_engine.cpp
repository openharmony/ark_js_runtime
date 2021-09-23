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

#include "llvm_mcjit_engine.h"

#include <vector>
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ADT/APInt.h"
#include "llvm/CodeGen/BuiltinGCs.h"
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
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/DisassemblerTypes.h"



namespace kungfu {
static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    std::cout << "RoundTripAllocateCodeSection object " << object << " - " << std::endl;
    struct CodeState& state = *static_cast<struct CodeState*>(object);
    uint8_t *addr = state.AllocaCodeSection(size, sectionName);
    std::cout << "RoundTripAllocateCodeSection  addr:" << std::hex << reinterpret_cast<std::uintptr_t>(addr) << addr
              << " size:0x" << size << " +" << std::endl;
    return addr;
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                             [[maybe_unused]] LLVMBool isReadOnly)
{
    struct CodeState& state = *static_cast<struct CodeState*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory(void *object, [[maybe_unused]] char **errMsg)
{
    std::cout << "RoundTripFinalizeMemory object " << object << " - " << std::endl;
    return 0;
}

static void RoundTripDestroy(void *object)
{
    std::cout << "RoundTripDestroy object " << object << " - " << std::endl;
    delete static_cast<struct CodeState *>(object);
}

void LLVMMcJitEngine::UseRoundTripSectionMemoryManager()
{
    auto sectionMemoryManager = std::make_unique<llvm::SectionMemoryManager>();
    options_.MCJMM =
        LLVMCreateSimpleMCJITMemoryManager(&codeState_, RoundTripAllocateCodeSection,
            RoundTripAllocateDataSection, RoundTripFinalizeMemory, RoundTripDestroy);
}

bool LLVMMcJitEngine::BuildMCJITEngine()
{
    std::cout << " BuildMCJITEngine  - " << std::endl;
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&engine_, module_, &options_, sizeof(options_), &error_);
    std::cout << " engine_  " << engine_ << std::endl;
    if (ret) {
        std::cout << "error_ : " << error_ << std::endl;
        return false;
    }
    std::cout << " BuildMCJITEngine  ++++++++++++ " << std::endl;
    return true;
}

void LLVMMcJitEngine::BuildAndRunPasses() const
{
    std::cout << "BuildAndRunPasses  - " << std::endl;
    LLVMPassManagerRef pass = LLVMCreatePassManager();
    LLVMAddConstantPropagationPass(pass);
    LLVMAddInstructionCombiningPass(pass);
    llvm::unwrap(pass)->add(llvm::createRewriteStatepointsForGCLegacyPass());
    LLVMDumpModule(module_);
    LLVMRunPassManager(pass, module_);
    LLVMDisposePassManager(pass);
    std::cout << "BuildAndRunPasses  + " << std::endl;
}

LLVMMcJitEngine::LLVMMcJitEngine(LLVMModuleRef module): module_(module), engine_(nullptr),
    hostTriple_(""), error_(nullptr)
{
    Initialize();
    InitMember();
}

LLVMMcJitEngine::~LLVMMcJitEngine()
{
    module_ = nullptr;
    engine_ = nullptr;
    hostTriple_ = "";
    error_ = nullptr;
}

void LLVMMcJitEngine::Run()
{
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    BuildAndRunPasses();
}

void LLVMMcJitEngine::Initialize()
{
#if defined(PANDA_TARGET_AMD64)
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86Disassembler();
    /* this method must be called, ohterwise "Target does not support MC emission" */
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86Target();
#endif
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

void LLVMMcJitEngine::Disassemble(std::map<uint64_t, std::string> addr2name) const
{
    LLVMDisasmContextRef dcr = LLVMCreateDisasm("x86_64-unknown-linux-gnu", nullptr, 0, nullptr, SymbolLookupCallback);
    std::cout << "========================================================================" << std::endl;
    for (auto it : codeState_.GetCodeInfo()) {
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
}

void LLVMMcJitEngine::InitMember()
{
    if (module_ == nullptr) {
        module_ = LLVMModuleCreateWithName("simple_module");
        LLVMSetTarget(module_, "x86_64-unknown-linux-gnu");
    }
}
}  // namespace kungfu
