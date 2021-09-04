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

#ifndef PANDA_RUNTIME_ECMASCRIPT_COMPILER_FASTPATH_OPTIMIZER_H
#define PANDA_RUNTIME_ECMASCRIPT_COMPILER_FASTPATH_OPTIMIZER_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/stub_optimizer.h"

namespace kungfu {
class FastArrayLoadElementOptimizer : public StubOptimizer {
public:
    explicit FastArrayLoadElementOptimizer(Environment *env) : StubOptimizer(env) {}
    ~FastArrayLoadElementOptimizer() = default;
    NO_MOVE_SEMANTIC(FastArrayLoadElementOptimizer);
    NO_COPY_SEMANTIC(FastArrayLoadElementOptimizer);
    void GenerateCircuit() override;
    uint8_t *AllocMachineCode()
    {
        static constexpr int PROT = PROT_READ | PROT_WRITE | PROT_EXEC;  // NOLINT(hicpp-signed-bitwise)
        static constexpr int FLAGS = MAP_ANONYMOUS | MAP_SHARED;         // NOLINT(hicpp-signed-bitwise)
        auto machineCode = static_cast<uint8_t *>(mmap(nullptr, MAX_MACHINE_CODE_SIZE, PROT, FLAGS, -1, 0));
        return machineCode;
    }
    void FreeMachineCode(uint8_t *machineCode)
    {
        munmap(machineCode, MAX_MACHINE_CODE_SIZE);
    }

private:
    const size_t MAX_MACHINE_CODE_SIZE = (1U << 20U);
};

class FastRuntimeStubs {
public:
    static FastRuntimeStubs &GetInstance()
    {
        static FastRuntimeStubs instance;
        return instance;
    }
    enum FastRuntimeStubId {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_RUNTIME_STUB_ID(name, counter) ID_##name,
        FAST_RUNTIME_STUB_LIST(FAST_RUNTIME_STUB_ID)
#undef FAST_RUNTIME_STUB_ID
            FAST_STUB_MAXNUMBER,
    };

    void GenerateFastRuntimeStubs();

private:
    FastRuntimeStubs();
    ~FastRuntimeStubs();
    NO_MOVE_SEMANTIC(FastRuntimeStubs);
    NO_COPY_SEMANTIC(FastRuntimeStubs);
    std::array<Environment, FAST_STUB_MAXNUMBER> fastRuntimeEnvs_;
    std::array<StubOptimizer *, FAST_STUB_MAXNUMBER> fastRuntimeOptimizers_;
};
}  // namespace kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_FASTPATH_OPTIMIZER_H