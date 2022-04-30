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

#include <iomanip>
#include <string>
#include <vector>

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/object_factory.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "ecmascript/compiler/call_signature.h"
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

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

using namespace panda::ecmascript;
namespace panda::ecmascript::kungfu {
void LLVMIRGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                                const CompilationConfig *cfg)
{
    LLVMValueRef function = module_->GetFunction(index);
    const CallSignature* cs = module_->GetCSign(index);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, cs->GetCallConv(), enableLog_);
    builder.Build();
}

void LLVMIRGeneratorImpl::GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
    const panda::ecmascript::JSMethod *method)
{
    auto function = module_->AddFunc(method);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, CallSignature::CallConv::WebKitJSCallConv,
                          enableLog_);
    builder.Build();
}

static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    uint8_t *addr = state.AllocaCodeSection(size, sectionName);
    return addr;
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                             [[maybe_unused]] LLVMBool isReadOnly)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory([[maybe_unused]] void *object, [[maybe_unused]] char **errMsg)
{
    return 0;
}

static void RoundTripDestroy([[maybe_unused]] void *object)
{
    return;
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
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&engine_, module_, &options_, sizeof(options_), &error_);
    if (ret) {
        COMPILER_LOG(FATAL) << "error_ : " << error_;
        return false;
    }
    return true;
}

void LLVMAssembler::BuildAndRunPasses()
{
    LLVMPassManagerBuilderRef pmBuilder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(pmBuilder, options_.OptLevel); // using O3 optimization level
    LLVMPassManagerBuilderSetSizeLevel(pmBuilder, 0);

    // pass manager creation:rs4gc pass is the only pass in modPass, other opt module-based pass are in modPass1
    LLVMPassManagerRef funcPass = LLVMCreateFunctionPassManagerForModule(module_);
    LLVMPassManagerRef modPass = LLVMCreatePassManager();
    LLVMPassManagerRef modPass1 = LLVMCreatePassManager();

    // add pass into pass managers
    LLVMPassManagerBuilderPopulateFunctionPassManager(pmBuilder, funcPass);
    llvm::unwrap(modPass)->add(llvm::createRewriteStatepointsForGCLegacyPass()); // rs4gc pass added
    LLVMPassManagerBuilderPopulateModulePassManager(pmBuilder, modPass1);

    LLVMRunPassManager(modPass, module_); // make sure rs4gc pass run first
    LLVMInitializeFunctionPassManager(funcPass);
    for (LLVMValueRef fn = LLVMGetFirstFunction(module_); fn; fn = LLVMGetNextFunction(fn)) {
        LLVMRunFunctionPassManager(funcPass, fn);
    }
    LLVMFinalizeFunctionPassManager(funcPass);
    LLVMRunPassManager(modPass1, module_);

    LLVMPassManagerBuilderDispose(pmBuilder);
    LLVMDisposePassManager(funcPass);
    LLVMDisposePassManager(modPass);
    LLVMDisposePassManager(modPass1);
}

LLVMAssembler::LLVMAssembler(LLVMModuleRef module, LOptions option) : module_(module)
{
    Initialize(option);
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
    std::string originName = llvm::unwrap(module_)->getModuleIdentifier() + ".ll";
    std::string optName = llvm::unwrap(module_)->getModuleIdentifier() + "_opt" + ".ll";
    LLVMPrintModuleToFile(module_, originName.c_str(), &error);
    LLVMVerifyModule(module_, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    BuildAndRunPasses();
    LLVMPrintModuleToFile(module_, optName.c_str(), &error);
}

void LLVMAssembler::Initialize(LOptions option)
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
    options_.OptLevel = option.optLevel;
    // NOTE: Just ensure that this field still exists for PIC option
    options_.NoFramePointerElim = option.genFp;
    options_.CodeModel = LLVMCodeModelSmall;
}

static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                        uint64_t *referenceType, [[maybe_unused]]uint64_t referencePC,
                                        [[maybe_unused]] const char **referenceName)
{
    *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
    return nullptr;
}

void LLVMAssembler::Disassemble(const std::map<uint64_t, std::string> &addr2name, const CompilerLog &log) const
{
    LLVMDisasmContextRef dcr = LLVMCreateDisasm(LLVMGetTarget(module_), nullptr, 0, nullptr, SymbolLookupCallback);
    bool logFlag = false;

    for (auto it : codeInfo_.GetCodeInfo()) {
        uint8_t *byteSp;
        uintptr_t numBytes;
        byteSp = it.first;
        numBytes = it.second;

        unsigned pc = 0;
        const char outStringSize = 100;
        char outString[outStringSize];
        std::string methodName;
        while (numBytes > 0) {
            uint64_t addr = reinterpret_cast<uint64_t>(byteSp);
            if (addr2name.find(addr) != addr2name.end()) {
                methodName = addr2name.at(addr);
                if (logFlag) {
                    COMPILER_LOG(INFO) << "=======================================================================";
                    COMPILER_LOG(INFO) << methodName.c_str() << " disassemble:";
                }
            }
            logFlag = log.IsAlwaysEnabled() ? true : log.IncludesMethod(methodName);

            size_t InstSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            if (InstSize == 0) {
                if (logFlag) {
                    COMPILER_LOG(INFO) << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
                                        << *reinterpret_cast<uint32_t *>(byteSp) << "maybe constant";
                }
                pc += 4; // 4 pc length
                byteSp += 4; // 4 sp offset
                numBytes -= 4; // 4 num bytes
            }
            if (logFlag) {
                COMPILER_LOG(INFO) << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
                                   << *reinterpret_cast<uint32_t *>(byteSp) << " " << outString;
            }
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
    }
    LLVMDisasmDispose(dcr);
}
}  // namespace panda::ecmascript::kungfu
