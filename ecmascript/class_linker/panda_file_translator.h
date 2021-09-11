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

#ifndef ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H
#define ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_function.h"
#include "libpandafile/bytecode_instruction.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/file-inl.h"
#include "utils/bit_field.h"

namespace panda::ecmascript {
class JSThread;
class Program;

class PandaFileTranslator {
public:
    enum FixInstructionIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    enum SecondInstructionOfMidr : uint8_t {
        DEFINE_FUNC_DYN = 0,
        DEFINE_NC_FUNC_DYN,
        DEFINE_GENERATOR_FUNC_DYN,
        DEFINE_ASYNC_FUNC_DYN,
        DEFINE_METHOD
    };

    enum SecondInstructionOfImm : uint8_t {
        CREATE_OBJECT_WITH_BUFFER = 2,
        CREATE_ARRAY_WITH_BUFFER,
        CREATE_OBJECT_HAVING_METHOD
    };

    explicit PandaFileTranslator(EcmaVM *vm);
    ~PandaFileTranslator() = default;
    NO_COPY_SEMANTIC(PandaFileTranslator);
    NO_MOVE_SEMANTIC(PandaFileTranslator);
    static JSHandle<Program> TranslatePandaFile(EcmaVM *vm, const panda_file::File &pf,
                                                const CString &methodName);
    JSHandle<JSFunction> DefineMethodInLiteral(JSThread *thread, uint32_t methodId, FunctionKind kind,
                                               uint16_t length) const;

private:
    enum class ConstPoolType : uint8_t {
        STRING,
        BASE_FUNCTION,
        NC_FUNCTION,
        GENERATOR_FUNCTION,
        ASYNC_FUNCTION,
        CLASS_FUNCTION,
        METHOD,
        ARRAY_LITERAL,
        OBJECT_LITERAL,
        CLASS_LITERAL,
    };

    class ConstPoolValue {
    public:
        ConstPoolValue(ConstPoolType type, uint32_t index)
            : value_(ConstPoolIndexField::Encode(index) | ConstPoolTypeField::Encode(type))
        {
        }

        explicit ConstPoolValue(uint64_t v) : value_(v) {}
        ~ConstPoolValue() = default;
        NO_COPY_SEMANTIC(ConstPoolValue);
        NO_MOVE_SEMANTIC(ConstPoolValue);

        inline uint64_t GetValue() const
        {
            return value_;
        }

        inline uint32_t GetConstpoolIndex() const
        {
            return ConstPoolIndexField::Get(value_);
        }

        inline ConstPoolType GetConstpoolType() const
        {
            return ConstPoolTypeField::Get(value_);
        }

    private:
        // NOLINTNEXTLINE(readability-magic-numbers)
        using ConstPoolIndexField = BitField<uint32_t, 0, 32>;  // 32: 32 bit
        // NOLINTNEXTLINE(readability-magic-numbers)
        using ConstPoolTypeField = BitField<ConstPoolType, 32, 4>;  // 32: offset, 4: 4bit

        uint64_t value_{0};
    };
    uint32_t GetOrInsertConstantPool(ConstPoolType type, uint32_t offset);
    const JSMethod *FindMethods(uint32_t offset) const;
    Program *GenerateProgram(const panda_file::File &pf);
    void TranslateClasses(const panda_file::File &pf, const CString &methodName);
    void TranslateBytecode(uint32_t insSz, const uint8_t *insArr, const panda_file::File &pf, const JSMethod *method);
    void FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder = 0) const;
    void FixOpcode(uint8_t *pc) const;
    void UpdateICOffset(JSMethod *method, uint8_t *pc) const;

    void SetMethods(Span<JSMethod> methods, const uint32_t numMethods)
    {
        methods_ = methods.data();
        numMethods_ = numMethods;
    }

    Span<JSMethod> GetMethods() const
    {
        return {methods_, numMethods_};
    }

    EcmaVM *ecmaVm_;
    ObjectFactory *factory_;
    JSThread *thread_;
    uint32_t constpoolIndex_{0};
    uint32_t numMethods_{0};
    uint32_t mainMethodIndex_{0};
    JSMethod *methods_ {nullptr};

    std::unordered_map<uint32_t, uint64_t> constpoolMap_;
    std::set<const uint8_t *> translated_code_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CLASS_LINKER_PANDA_FILE_TRANSLATOR_H
