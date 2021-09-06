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

#include "llvm_mcjit_compiler.h"

#include <vector>

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/DisassemblerTypes.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/ADT/APInt.h"
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
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"

namespace kungfu {
static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, unsigned alignment, unsigned sectionID,
                                             const char *sectionName)
{
    std::cout << "RoundTripAllocateCodeSection object " << object << " - " << std::endl;
    struct CompilerState& state = *static_cast<struct CompilerState*>(object);
    uint8_t *addr = state.AllocaCodeSection(size, sectionName);
    std::cout << "RoundTripAllocateCodeSection  addr:" << std::hex << reinterpret_cast<std::uintptr_t>(addr) << addr
              << " size:0x" << size << " +" << std::endl;
    return addr;
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, unsigned alignment, unsigned sectionID,
                                             const char *sectionName, LLVMBool isReadOnly)
{
    struct CompilerState& state = *static_cast<struct CompilerState*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory(void *object, char **errMsg)
{
    std::cout << "RoundTripFinalizeMemory object " << object << " - " << std::endl;
    return 0;
}

static void RoundTripDestroy(void *object)
{
    std::cout << "RoundTripDestroy object " << object << " - " << std::endl;
    delete static_cast<struct CompilerState *>(object);
}

void LLVMMCJITCompiler::UseRoundTripSectionMemoryManager()
{
    auto sectionMemoryManager = std::make_unique<llvm::SectionMemoryManager>();
    m_options.MCJMM =
        LLVMCreateSimpleMCJITMemoryManager(&compilerState, RoundTripAllocateCodeSection,
            RoundTripAllocateDataSection, RoundTripFinalizeMemory, RoundTripDestroy);
}

bool LLVMMCJITCompiler::BuildMCJITEngine()
{
    std::cout << " BuildMCJITEngine  - " << std::endl;
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&m_engine, m_module, &m_options, sizeof(m_options), &m_error);
    std::cout << " m_engine  " << m_engine << std::endl;
    if (ret) {
        std::cout << "m_error : " << m_error << std::endl;
        return false;
    }
    std::cout << " BuildMCJITEngine  ++++++++++++ " << std::endl;
    return true;
}

void LLVMMCJITCompiler::BuildAndRunPasses() const
{
    std::cout << "BuildAndRunPasses  - " << std::endl;
    LLVMPassManagerRef pass = LLVMCreatePassManager();
    LLVMAddConstantPropagationPass(pass);
    LLVMAddInstructionCombiningPass(pass);
    LLVMRunPassManager(pass, m_module);
    LLVMDisposePassManager(pass);
    std::cout << "BuildAndRunPasses  + " << std::endl;
}

LLVMMCJITCompiler::LLVMMCJITCompiler(LLVMModuleRef module): m_module(module), m_engine(nullptr),
    m_hostTriple(""), m_error(nullptr)
{
    Initialize();
    InitMember();
}

LLVMMCJITCompiler::~LLVMMCJITCompiler()
{
    m_module = nullptr;
    m_engine = nullptr;
    m_hostTriple = "";
    m_error = nullptr;
}

void LLVMMCJITCompiler::Run()
{
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    BuildAndRunPasses();
}

void LLVMMCJITCompiler::Initialize()
{
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllDisassemblers();
    /* this method must be called, ohterwise "Target does not support MC emission" */
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMInitializeAllTargets();
    LLVMInitializeMCJITCompilerOptions(&m_options, sizeof(m_options));
    m_options.OptLevel = 2; // opt level 2
    // Just ensure that this field still exists.
    m_options.NoFramePointerElim = false;
}

static const char *SymbolLookupCallback(void *disInfo, uint64_t referenceValue, uint64_t *referenceType,
                                        uint64_t referencePC, const char **referenceName)
{
    *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
    return nullptr;
}

void LLVMMCJITCompiler::Disassemble() const
{
    LLVMDisasmContextRef dcr = LLVMCreateDisasm("x86_64-unknown-linux-gnu", nullptr, 0, nullptr, SymbolLookupCallback);
    std::cout << "========================================================================" << std::endl;
    for (auto it : compilerState.codeInfo) {
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

            fprintf(stderr, "%08x: %08x %s\n", pc, *reinterpret_cast<uint32_t *>(byteSp), outString);
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
    }
    std::cout << "========================================================================" << std::endl;
}

void LLVMMCJITCompiler::InitMember()
{
    if (m_module == nullptr) {
        m_module = LLVMModuleCreateWithName("simple_module");
        LLVMSetTarget(m_module, "x86_64-unknown-linux-gnu");
    }
}
}  // namespace kungfu
