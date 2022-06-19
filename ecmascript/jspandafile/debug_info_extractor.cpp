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
#include "libpandafile/line_number_program.h"

namespace panda::ecmascript {
using panda::panda_file::ClassDataAccessor;
using panda::panda_file::DebugInfoDataAccessor;
using panda::panda_file::LineNumberProgramItem;
using panda::panda_file::LineProgramState;
using panda::panda_file::LineNumberProgramProcessor;
using panda::panda_file::MethodDataAccessor;
using panda::panda_file::ProtoDataAccessor;

static const char *GetStringFromConstantPool(const panda_file::File &pf, uint32_t offset)
{
    return reinterpret_cast<const char *>(pf.GetStringData(panda_file::File::EntityId(offset)).data);
}

DebugInfoExtractor::DebugInfoExtractor(const JSPandaFile *jsPandaFile)
{
    Extract(jsPandaFile->GetPandaFile());
}

class LineNumberProgramHandler {
public:
    explicit LineNumberProgramHandler(LineProgramState *state) : state_(state) {}
    ~LineNumberProgramHandler() = default;

    NO_COPY_SEMANTIC(LineNumberProgramHandler);
    NO_MOVE_SEMANTIC(LineNumberProgramHandler);

    LineProgramState *GetState() const
    {
        return state_;
    }

    void ProcessBegin()
    {
        lnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetLine())});
    }

    void ProcessEnd()
    {
    }

    bool HandleAdvanceLine(int32_t lineDiff) const
    {
        state_->AdvanceLine(lineDiff);
        return true;
    }

    bool HandleAdvancePc(uint32_t pcDiff) const
    {
        state_->AdvancePc(pcDiff);
        return true;
    }

    bool HandleSetFile(uint32_t sourceFileId) const
    {
        state_->SetFile(sourceFileId);
        return true;
    }

    bool HandleSetSourceCode(uint32_t sourceCodeId) const
    {
        state_->SetSourceCode(sourceCodeId);
        return true;
    }

    bool HandleSetPrologueEnd() const
    {
        return true;
    }

    bool HandleSetEpilogueBegin() const
    {
        return true;
    }

    bool HandleStartLocal(int32_t regNumber, uint32_t nameId, [[maybe_unused]] uint32_t typeId)
    {
        const char *name = GetStringFromConstantPool(state_->GetPandaFile(), nameId);
        lvt_.insert(std::make_pair(name, regNumber));
        return true;
    }

    bool HandleStartLocalExtended(int32_t regNumber, uint32_t nameId, [[maybe_unused]] uint32_t typeId,
                                  [[maybe_unused]] uint32_t typeSignatureId)
    {
        const char *name = GetStringFromConstantPool(state_->GetPandaFile(), nameId);
        lvt_.insert(std::make_pair(name, regNumber));
        return true;
    }

    bool HandleEndLocal([[maybe_unused]] int32_t regNumber)
    {
        return true;
    }

    bool HandleSetColumn(int32_t columnNumber)
    {
        state_->SetColumn(columnNumber);
        cnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetColumn())});
        return true;
    }

    bool HandleSpecialOpcode(uint32_t pcOffset, int32_t lineOffset)
    {
        state_->AdvancePc(pcOffset);
        state_->AdvanceLine(lineOffset);
        lnt_.push_back({state_->GetAddress(), static_cast<int32_t>(state_->GetLine())});
        return true;
    }

    LineNumberTable GetLineNumberTable() const
    {
        return lnt_;
    }

    LocalVariableTable GetLocalVariableTable() const
    {
        return lvt_;
    }

    ColumnNumberTable GetColumnNumberTable() const
    {
        return cnt_;
    }

    const uint8_t *GetFile() const
    {
        return state_->GetFile();
    }

    const uint8_t *GetSourceCode() const
    {
        return state_->GetSourceCode();
    }

private:
    using Opcode = LineNumberProgramItem::Opcode;

    LineProgramState *state_;
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

            LineNumberProgramHandler handler(&state);
            LineNumberProgramProcessor<LineNumberProgramHandler> programProcessor(program, &handler);
            programProcessor.Process();

            panda_file::File::EntityId methodId = mda.GetMethodId();
            const char *sourceFile = reinterpret_cast<const char *>(handler.GetFile());
            const char *sourceCode = reinterpret_cast<const char *>(handler.GetSourceCode());
            methods_.insert(std::make_pair(methodId.GetOffset(), MethodDebugInfo {sourceFile, sourceCode,
                                           handler.GetLineNumberTable(),
                                           handler.GetColumnNumberTable(),
                                           handler.GetLocalVariableTable()}));
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
