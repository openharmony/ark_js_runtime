/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_H

#include <string>
#include <vector>

#include "assembler/assembler.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/stubs/runtime_stubs.h"

namespace panda::ecmascript::kungfu {
class AssemblerModule {
public:
    AssemblerModule() = default;
    ~AssemblerModule()
    {
        for (auto it : symbolTable_) {
            delete it.second;
        }
    }

    void Run(const std::string &triple, Chunk* chunk);

    size_t GetFunctionCount() const
    {
        return symbolTable_.size();
    }

    size_t GetFunction(int id) const
    {
        panda::ecmascript::Label *label = GetFunctionLabel(id);
        if (label->IsBound()) {
            return label->GetPos();
        } else {
            return -1;
        }
    }

    panda::ecmascript::Label* GetFunctionLabel(int id) const
    {
        return symbolTable_.at(id);
    }

    uint8_t* GetBuffer() const
    {
        return buffer_;
    }

    size_t GetBufferSize() const
    {
        return bufferSize_;
    }

    void SetUpForAsmStubs();

    const std::vector<const CallSignature*> &GetCSigns() const
    {
        return asmCallSigns_;
    }

    size_t GetCodeBufferOffset() const
    {
        return codeBufferOffset_;
    }

    void SetCodeBufferOffset(size_t offset)
    {
        codeBufferOffset_ = offset;
    }
    void GenerateStubsX64(Chunk* chunk);
    void GenerateStubsAarch64(Chunk* chunk);
private:
    std::vector<const CallSignature *> asmCallSigns_;
    std::map<int, panda::ecmascript::Label *> symbolTable_;
    size_t codeBufferOffset_ {0};
    uint8_t* buffer_ {nullptr};
    size_t bufferSize_ {0};
};

class AssemblerStub {
public:
    virtual void GenerateX64(Assembler* assembler) = 0;
    virtual void GenerateAarch64(Assembler* assembler) = 0;
    virtual ~AssemblerStub() = default;
};

#define DECLARE_ASM_STUB_CLASS(name)                        \
class name##Stub : public AssemblerStub {                   \
public:                                                     \
    ~name##Stub() = default;                                \
    void GenerateX64(Assembler* assembler) override;        \
    void GenerateAarch64(Assembler* assembler) override;    \
};
RUNTIME_ASM_STUB_LIST(DECLARE_ASM_STUB_CLASS)
#undef DECLARE_ASM_STUB_CLASS
}  // namespace panda::ecmascript::kunfu
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_H
