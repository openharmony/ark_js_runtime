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

namespace panda::ecmascript::kungfu {
class AssemblerModule {
public:
    void Run(const std::string &triple, Chunk* chunk);

    size_t GetFunctionCount() const
    {
        return offsetTable_.size();
    }

    size_t GetFunction(size_t index) const
    {
        return offsetTable_[index];
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

    const std::vector<CallSignature*> &GetCSigns() const
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
private:
    std::vector<CallSignature *> asmCallSigns_;
    std::vector<size_t> offsetTable_;
    size_t codeBufferOffset_ {0};
    uint8_t* buffer_ {nullptr};
    size_t bufferSize_ {0};
};
}  // namespace panda::ecmascript::kunfu
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_H
