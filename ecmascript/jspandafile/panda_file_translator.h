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

#ifndef ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H
#define ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "libpandafile/bytecode_instruction.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/file-inl.h"
#include "utils/bit_field.h"

namespace panda::ecmascript {
struct BytecodeTranslationInfo {
    std::vector<uint8_t *> pcArray {};
    const JSPandaFile *jsPandaFile {nullptr};
    const JSMethod *method {nullptr};
};

class JSThread;
class Program;
class JSPandaFileManager;
class JSPandaFile;

class PandaFileTranslator {
public:
    enum FixInstructionIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    explicit PandaFileTranslator(const JSPandaFile *jsPandaFile);
    PandaFileTranslator(EcmaVM *vm, const JSPandaFile *jsPandaFile);
    ~PandaFileTranslator() = default;
    NO_COPY_SEMANTIC(PandaFileTranslator);
    NO_MOVE_SEMANTIC(PandaFileTranslator);
    void TranslateAndCollectPandaFile(const CString &methodName, std::vector<BytecodeTranslationInfo> *infoList);
    JSHandle<JSFunction> DefineMethodInLiteral(JSThread *thread, uint32_t methodId, FunctionKind kind,
                                               uint16_t length) const;
    Program *GenerateProgram();
    static void TranslateClasses(JSPandaFile *jsPandaFile, const CString &methodName,
                                 std::vector<BytecodeTranslationInfo> *infoList = nullptr);

private:
    static void TranslateBytecode(JSPandaFile *jsPandaFile, uint32_t insSz, const uint8_t *insArr,
                                  const JSMethod *method, std::vector<BytecodeTranslationInfo> *infoList);
    static void FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder = 0);
    static void FixOpcode(uint8_t *pc);
    static void UpdateICOffset(JSMethod *method, uint8_t *pc);
    void DefineClassInConstPool(const JSHandle<ConstantPool> &constpool) const;

    EcmaVM *ecmaVm_;
    ObjectFactory *factory_;
    JSThread *thread_;
    const JSPandaFile *jsPandaFile_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H
