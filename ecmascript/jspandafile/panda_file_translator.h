/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "libpandafile/bytecode_instruction-inl.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/file-inl.h"
#include "utils/bit_field.h"

namespace panda::ecmascript {
struct MethodPcInfo {
    const JSMethod *method {nullptr};
    std::vector<uint8_t *> pcArray {};
};

struct BytecodeTranslationInfo {
    const JSPandaFile *jsPandaFile {nullptr};
    JSHandle<JSTaggedValue> constantPool;
    std::vector<MethodPcInfo> methodPcInfos {};
};

class JSThread;
class Program;
class JSPandaFileManager;
class JSPandaFile;

class PUBLIC_API PandaFileTranslator {
public:
    enum FixInstructionIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    PandaFileTranslator() = default;
    ~PandaFileTranslator() = default;
    NO_COPY_SEMANTIC(PandaFileTranslator);
    NO_MOVE_SEMANTIC(PandaFileTranslator);
    static JSHandle<Program> GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile);
    static void TranslateClasses(JSPandaFile *jsPandaFile, const CString &methodName,
                                 std::vector<MethodPcInfo> *methodPcInfos = nullptr);

private:
    static void TranslateBytecode(JSPandaFile *jsPandaFile, uint32_t insSz, const uint8_t *insArr,
                                  const JSMethod *method, std::vector<MethodPcInfo> *methodPcInfos);
    static void FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder = 0);
    static void FixOpcode(uint8_t *pc);
    static void UpdateICOffset(JSMethod *method, uint8_t *pc);
    static JSTaggedValue ParseConstPool(EcmaVM *vm, const JSPandaFile *jsPandaFile);
    static void DefineClassesInConstPool(JSThread *thread, JSHandle<ConstantPool> constpool,
                                         const JSPandaFile *jsPandaFile);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_PANDA_FILE_TRANSLATOR_H
