/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/jspandafile/debug_info_extractor.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "libpandafile/debug_data_accessor-inl.h"
#include "libpandafile/line_program_state.h"

namespace panda::ecmascript {
using panda::panda_file::ClassDataAccessor;
using panda::panda_file::DebugInfoDataAccessor;
using panda::panda_file::LineNumberProgramItem;
using panda::panda_file::LineProgramState;
using panda::panda_file::MethodDataAccessor;

static const char *GetStringFromConstantPool(const panda_file::File &pf, uint32_t offset)
{
    return reinterpret_cast<const char *>(pf.GetStringData(panda_file::File::EntityId(offset)).data);
}

DebugInfoExtractor::DebugInfoExtractor(const JSPandaFile *jsPandaFile)
{
    Extract(jsPandaFile->GetPandaFile());
}

class LineNumberProgramProcessor {
public:
    LineNumberProgramProcessor(LineProgramState state, const uint8_t *program) : state_(state), program_(program) {}

    ~LineNumberProgramProcessor() = default;

    NO_COPY_SEMANTIC(LineNumberProgramProcessor);
    NO_MOVE_SEMANTIC(LineNumberProgramProcessor);

    void Process()
    {
        auto opcode = ReadOpcode();
        lnt_.push_back({state_.GetAddress(), state_.GetLine()});
        while (opcode != Opcode::END_SEQUENCE) {
            switch (opcode) {
                case Opcode::ADVANCE_LINE: {
                    HandleAdvanceLine();
                    break;
                }
                case Opcode::ADVANCE_PC: {
                    HandleAdvancePc();
                    break;
                }
                case Opcode::SET_FILE: {
                    HandleSetFile();
                    break;
                }
                case Opcode::SET_SOURCE_CODE: {
                    HandleSetSourceCode();
                    break;
                }
                case Opcode::SET_PROLOGUE_END:
                case Opcode::SET_EPILOGUE_BEGIN:
                    break;
                case Opcode::START_LOCAL: {
                    HandleStartLocal();
                    break;
                }
                case Opcode::START_LOCAL_EXTENDED: {
                    HandleStartLocalExtended();
                    break;
                }
                case Opcode::RESTART_LOCAL: {
                    LOG(FATAL, PANDAFILE) << "Opcode RESTART_LOCAL is not supported";
                    break;
                }
                case Opcode::END_LOCAL: {
                    HandleEndLocal();
                    break;
                }
                case Opcode::SET_COLUMN: {
                    HandleSetColumn();
                    break;
                }
                default: {
                    HandleSpecialOpcode(opcode);
                    break;
                }
            }
            opcode = ReadOpcode();
        }
    }

    LineNumberTable GetLineNumberTable() const
    {
        return lnt_;
    }

    ColumnNumberTable GetColumnNumberTable() const
    {
        return cnt_;
    }

    LocalVariableTable GetLocalVariableTable() const
    {
        return lvt_;
    }

    const uint8_t *GetFile() const
    {
        return state_.GetFile();
    }

    const uint8_t *GetSourceCode() const
    {
        return state_.GetSourceCode();
    }

private:
    using Opcode = LineNumberProgramItem::Opcode;

    Opcode ReadOpcode()
    {
        auto opcode = static_cast<Opcode>(*program_);
        ++program_;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return opcode;
    }

    int32_t ReadRegisterNumber()
    {
        auto [regiserNumber, n, isFull] = leb128::DecodeSigned<int32_t>(program_);
        LOG_IF(!isFull, FATAL, COMMON) << "Cannot read a register number";
        program_ += n;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return regiserNumber;
    }

    void HandleAdvanceLine()
    {
        auto line_diff = state_.ReadSLeb128();
        state_.AdvanceLine(line_diff);
    }

    void HandleAdvancePc()
    {
        auto pc_diff = state_.ReadULeb128();
        state_.AdvancePc(pc_diff);
    }

    void HandleSetFile()
    {
        state_.SetFile(state_.ReadULeb128());
    }

    void HandleSetSourceCode()
    {
        state_.SetSourceCode(state_.ReadULeb128());
    }

    void HandleSetPrologueEnd() {}

    void HandleSetEpilogueBegin() {}

    void HandleStartLocal()
    {
        auto regNumber = ReadRegisterNumber();
        auto nameIndex = state_.ReadULeb128();
        [[maybe_unused]] auto typeIndex = state_.ReadULeb128();
        const char *name = GetStringFromConstantPool(state_.GetPandaFile(), nameIndex);
        lvt_.insert(std::make_pair(name, regNumber));
    }

    void HandleStartLocalExtended()
    {
        auto regNumber = ReadRegisterNumber();
        auto nameIndex = state_.ReadULeb128();
        [[maybe_unused]] auto typeIndex = state_.ReadULeb128();
        [[maybe_unused]] auto typeSignature_index = state_.ReadULeb128();
        const char *name = GetStringFromConstantPool(state_.GetPandaFile(), nameIndex);
        lvt_.insert(std::make_pair(name, regNumber));
    }

    void HandleEndLocal()
    {
        [[maybe_unused]] auto regNumber = ReadRegisterNumber();
    }

    void HandleSetColumn()
    {
        auto cn = state_.ReadULeb128();
        state_.SetColumn(cn);
        cnt_.push_back({state_.GetAddress(), state_.GetColumn()});
    }

    void HandleSpecialOpcode(LineNumberProgramItem::Opcode opcode)
    {
        ASSERT(static_cast<uint8_t>(opcode) >= LineNumberProgramItem::OPCODE_BASE);

        auto adjust_opcode = static_cast<uint8_t>(static_cast<uint8_t>(opcode) - LineNumberProgramItem::OPCODE_BASE);
        auto pc_offset = static_cast<uint32_t>(adjust_opcode / LineNumberProgramItem::LINE_RANGE);
        int32_t line_offset =
            static_cast<int32_t>(adjust_opcode) % LineNumberProgramItem::LINE_RANGE + LineNumberProgramItem::LINE_BASE;
        state_.AdvancePc(pc_offset);
        state_.AdvanceLine(line_offset);
        lnt_.push_back({state_.GetAddress(), state_.GetLine()});
    }

    LineProgramState state_;
    const uint8_t *program_;
    LineNumberTable lnt_;
    LocalVariableTable lvt_;
    ColumnNumberTable cnt_;
};

void DebugInfoExtractor::Extract(const panda_file::File *pf)
{
    ASSERT(pf != nullptr);
    const auto &pandaFile = *pf;
    auto classes = pf->GetClasses();
    for (size_t i = 0; i < classes.Size(); i++) {
        panda_file::File::EntityId id(classes[i]);
        if (pandaFile.IsExternal(id)) {
            continue;
        }

        ClassDataAccessor cda(pandaFile, id);

        auto sourceFileId = cda.GetSourceFileId();

        cda.EnumerateMethods([&](MethodDataAccessor &mda) {
            auto debugInfoId = mda.GetDebugInfoId();
            if (!debugInfoId) {
                return;
            }

            DebugInfoDataAccessor dda(pandaFile, debugInfoId.value());
            const uint8_t *program = dda.GetLineNumberProgram();
            LineProgramState state(pandaFile, sourceFileId.value_or(panda_file::File::EntityId(0)), dda.GetLineStart(),
                                   dda.GetConstantPool());

            LineNumberProgramProcessor programProcessor(state, program);
            programProcessor.Process();

            panda_file::File::EntityId methodId = mda.GetMethodId();
            const char *sourceFile = reinterpret_cast<const char *>(programProcessor.GetFile());
            const char *sourceCode = reinterpret_cast<const char *>(programProcessor.GetSourceCode());
            methods_.insert(std::make_pair(methodId.GetOffset(), MethodDebugInfo {sourceFile, sourceCode,
                                           programProcessor.GetLineNumberTable(),
                                           programProcessor.GetColumnNumberTable(),
                                           programProcessor.GetLocalVariableTable()}));
        });
    }
}

const LineNumberTable &DebugInfoExtractor::GetLineNumberTable(panda_file::File::EntityId methodId) const
{
    static const LineNumberTable EMPTY_LINE_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        return EMPTY_LINE_TABLE;
    }
    return iter->second.lineNumberTable;
}

const ColumnNumberTable &DebugInfoExtractor::GetColumnNumberTable(panda_file::File::EntityId methodId) const
{
    static const ColumnNumberTable EMPTY_COLUMN_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        return EMPTY_COLUMN_TABLE;
    }
    return iter->second.columnNumberTable;
}

const LocalVariableTable &DebugInfoExtractor::GetLocalVariableTable(panda_file::File::EntityId methodId) const
{
    static const LocalVariableTable EMPTY_VARIABLE_TABLE {};

    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        return EMPTY_VARIABLE_TABLE;
    }
    return iter->second.localVariableTable;
}

const std::string &DebugInfoExtractor::GetSourceFile(panda_file::File::EntityId methodId) const
{
    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        LOG(FATAL, DEBUGGER) << "Get source file of unknown method id: " << methodId.GetOffset();
    }
    return iter->second.sourceFile;
}

const std::string &DebugInfoExtractor::GetSourceCode(panda_file::File::EntityId methodId) const
{
    auto iter = methods_.find(methodId.GetOffset());
    if (iter == methods_.end()) {
        LOG(FATAL, DEBUGGER) << "Get source code of unknown method id: " << methodId.GetOffset();
    }
    return iter->second.sourceCode;
}

CVector<panda_file::File::EntityId> DebugInfoExtractor::GetMethodIdList() const
{
    CVector<panda_file::File::EntityId> list;

    for (const auto &method : methods_) {
        list.push_back(panda_file::File::EntityId(method.first));
    }
    return list;
}
}  // namespace panda::ecmascript
