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

#include <chrono>
#include <iostream>
#include <signal.h>  // NOLINTNEXTLINE(modernize-deprecated-headers)
#include <vector>

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/ecma_language_context.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "include/runtime.h"
#include "libpandabase/os/native_stack.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"
#include "libpandafile/file.h"
#include "pass_manager.h"

namespace panda::ecmascript::kungfu {
void BlockSignals()
{
#if defined(PANDA_TARGET_UNIX)
    sigset_t set;
    if (sigemptyset(&set) == -1) {
        LOG(ERROR, RUNTIME) << "sigemptyset failed";
        return;
    }
    int rc = 0;

    if (rc < 0) {
        LOG(ERROR, RUNTIME) << "sigaddset failed";
        return;
    }

    if (panda::os::native_stack::g_PandaThreadSigmask(SIG_BLOCK, &set, nullptr) != 0) {
        LOG(ERROR, RUNTIME) << "g_PandaThreadSigmask failed";
    }
#endif  // PANDA_TARGET_UNIX
}

int Main(const int argc, const char **argv)
{
    auto startTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
    BlockSignals();
    Span<const char *> sp(argv, argc);
    JSRuntimeOptions runtimeOptions(sp[0]);

    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print compiler and runtime options");
    // tail arguments
    panda::PandArg<arg_list_t> files("files", {""}, "path to pandafiles", ":");
    panda::PandArg<std::string> entrypoint("entrypoint", "_GLOBAL::func_main_0",
                                           "full name of entrypoint function or method");
    panda::PandArgParser paParser;

    runtimeOptions.AddOptions(&paParser);

    paParser.Add(&help);
    paParser.Add(&options);
    paParser.PushBackTail(&files);
    paParser.PushBackTail(&entrypoint);
    paParser.EnableTail();
    paParser.EnableRemainder();

    if (!paParser.Parse(argc, argv) || files.GetValue().empty() || entrypoint.GetValue().empty() || help.GetValue()) {
        std::cerr << paParser.GetErrorString() << std::endl;
        std::cerr << "Usage: "
                  << "panda"
                  << " [OPTIONS] [file1:file2:file3] [entrypoint] -- [arguments]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;
        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    arg_list_t arguments = paParser.GetRemainder();

    if (runtimeOptions.IsStartupTime()) {
        std::cout << "\n"
                  << "Startup start time: " << startTime << std::endl;
    }

    auto runtimeOptionsErr = runtimeOptions.Validate();
    if (runtimeOptionsErr) {
        std::cerr << "Error: " << runtimeOptionsErr.value().GetMessage() << std::endl;
        return 1;
    }

    runtimeOptions.SetShouldLoadBootPandaFiles(false);
    runtimeOptions.SetShouldInitializeIntrinsics(false);
    runtimeOptions.SetBootClassSpaces({"ecmascript"});
    runtimeOptions.SetRuntimeType("ecmascript");
    JSNApi::SetOptions(runtimeOptions);
    static EcmaLanguageContext lcEcma;
    bool ret = Runtime::Create(runtimeOptions, {&lcEcma});
    if (!ret) {
        std::cerr << "Error: cannot Create Runtime" << std::endl;
        return -1;
    }
    auto runtime = Runtime::GetCurrent();

    if (options.GetValue()) {
        std::cout << paParser.GetRegularArgs() << std::endl;
    }

    EcmaVM *vm = EcmaVM::Cast(runtime->GetPandaVM());

    LocalScope scope(vm);
    if (!vm->GetJSOptions().EnableTSAot()) {
        std::cerr << "Error: non-aot execution!!!" << std::endl;
        return -1;
    }
    std::string entry = entrypoint.GetValue();

    arg_list_t pandaFileNames = files.GetValue();
    PassManager passManager(vm, entry);
    std::string triple = runtimeOptions.GetAotTargetTriple();
    std::string outputFileName = runtimeOptions.GetAOTOutputFile();
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    for (const auto &fileName : pandaFileNames) {
        LOG_ECMA(DEBUG) << "start to execute ark file: " << fileName;
        if (passManager.Compile(fileName, triple, outputFileName) == false) {
            ret = false;
            break;
        }
    }

    if (!Runtime::Destroy()) {
        std::cerr << "Error: cannot destroy Runtime" << std::endl;
        return -1;
    }
    paParser.DisableTail();
    return ret ? 0 : -1;
}
} // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    auto result = panda::ecmascript::kungfu::Main(argc, argv);
    std::cout << (result == 0 ? "ts aot execute success" : "ts aot execute failed") << std::endl;
    return result;
}
