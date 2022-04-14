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

#include "ecmascript/regexp/regexp_opcode.h"

#include "ecmascript/regexp/regexp_executor.h"

namespace panda::ecmascript {
using CaptureState = RegExpExecutor::CaptureState;

static SaveStartOpCode g_saveStartOpcode = SaveStartOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static SaveEndOpCode g_saveEndOpcode = SaveEndOpCode();        // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static CharOpCode g_charOpcode = CharOpCode();                 // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static GotoOpCode g_gotoOpcode = GotoOpCode();                 // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static SplitNextOpCode g_splitNextOpcode = SplitNextOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static SplitFirstOpCode g_splitFirstOpcode =
    SplitFirstOpCode();                            // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static MatchOpCode g_matchOpcode = MatchOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static LoopOpCode g_loopOpcode = LoopOpCode();     // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static LoopGreedyOpCode g_loopGreedyOpcode =
    LoopGreedyOpCode();                                        // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static PushCharOpCode g_pushCharOpcode = PushCharOpCode();     // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static CheckCharOpCode g_checkCharOpcode = CheckCharOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static PushOpCode g_pushOpcode = PushOpCode();                 // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static PopOpCode g_popOpcode = PopOpCode();                    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static SaveResetOpCode g_saveResetOpcode = SaveResetOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static LineStartOpCode g_lineStartOpcode = LineStartOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static LineEndOpCode g_lineEndOpcode = LineEndOpCode();        // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static WordBoundaryOpCode g_wordBoundaryOpcode =
    WordBoundaryOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static NotWordBoundaryOpCode g_notWordBoundaryOpcode =
    NotWordBoundaryOpCode();                    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static AllOpCode g_allOpcode = AllOpCode();     // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static DotsOpCode g_dotsOpcode = DotsOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static MatchAheadOpCode g_matchAheadOpcode =
    MatchAheadOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static NegativeMatchAheadOpCode g_negativeMatchAheadOpcode =
    NegativeMatchAheadOpCode();                             // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static MatchEndOpCode g_matchEndOpcode = MatchEndOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static PrevOpCode g_prevOpcode = PrevOpCode();              // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static RangeOpCode g_rangeOpcode = RangeOpCode();           // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static BackReferenceOpCode g_backreferenceOpcode =
    BackReferenceOpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static BackwardBackReferenceOpCode g_backwardBackreferenceOpcode =
    BackwardBackReferenceOpCode();                       // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static Char32OpCode g_char32Opcode = Char32OpCode();     // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static Range32OpCode g_range32Opcode = Range32OpCode();  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static std::vector<RegExpOpCode *> g_intrinsicSet = {
    &g_saveStartOpcode,
    &g_saveEndOpcode,
    &g_charOpcode,
    &g_gotoOpcode,
    &g_splitFirstOpcode,
    &g_splitNextOpcode,
    &g_matchAheadOpcode,
    &g_negativeMatchAheadOpcode,
    &g_matchOpcode,
    &g_loopOpcode,
    &g_loopGreedyOpcode,
    &g_pushCharOpcode,
    &g_checkCharOpcode,
    &g_pushOpcode,
    &g_popOpcode,
    &g_saveResetOpcode,
    &g_lineStartOpcode,
    &g_lineEndOpcode,
    &g_wordBoundaryOpcode,
    &g_notWordBoundaryOpcode,
    &g_allOpcode,
    &g_dotsOpcode,
    &g_matchEndOpcode,
    &g_prevOpcode,
    &g_rangeOpcode,
    &g_backreferenceOpcode,
    &g_backwardBackreferenceOpcode,
    &g_char32Opcode,
    &g_range32Opcode,
};

RegExpOpCode::RegExpOpCode(uint8_t opCode, int size) : opCode_(opCode), size_(size) {}

/* static */
RegExpOpCode *RegExpOpCode::GetRegExpOpCode(const DynChunk &buf, int pc)
{
    uint8_t opCode = buf.GetU8(pc);
    ASSERT_PRINT(opCode <= g_intrinsicSet.size(), "invalid op code");
    return g_intrinsicSet.at(opCode);
}

/* static */
RegExpOpCode *RegExpOpCode::GetRegExpOpCode(uint8_t opCode)
{
    ASSERT_PRINT(opCode <= g_intrinsicSet.size(), "invalid op code");
    return g_intrinsicSet.at(opCode);
}

/* static */
void RegExpOpCode::DumpRegExpOpCode(std::ostream &out, const DynChunk &buf)
{
    out << "OpCode:\t" << std::endl;
    uint32_t pc = RegExpParser::OP_START_OFFSET;
    do {
        RegExpOpCode *byteCode = GetRegExpOpCode(buf, pc);
        pc = byteCode->DumpOpCode(out, buf, pc);
    } while (pc < buf.size_);
}

uint32_t SaveStartOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    auto capture = static_cast<uint8_t>(para & 0xffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    buf->EmitChar(GetOpCode());
    buf->EmitChar(capture);
    return GetDynChunkfSize(*buf);
}

uint32_t SaveStartOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "save_start\t" << buf.GetU8(offset + 1) << std::endl;
    return offset + GetSize();
}

uint32_t SaveEndOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    auto capture = static_cast<uint8_t>(para & 0xffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    buf->EmitChar(GetOpCode());
    buf->EmitChar(capture);
    return GetDynChunkfSize(*buf);
}

uint32_t SaveEndOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "save_end\t" << buf.GetU8(offset + 1) << std::endl;
    return offset + GetSize();
}

uint32_t CharOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    auto paraChar = static_cast<uint16_t>(para & 0xffffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    buf->EmitChar(GetOpCode());
    buf->EmitU16(paraChar);
    return GetDynChunkfSize(*buf);
}

uint32_t CharOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "char\t" << static_cast<char>(buf.GetU16(offset + 1)) << std::endl;
    return offset + GetSize();
}

uint32_t Char32OpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    buf->EmitU32(para);
    return GetDynChunkfSize(*buf);
}

uint32_t Char32OpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "char32\t" << static_cast<char>(buf.GetU32(offset + 1)) << std::endl;
    return offset + GetSize();
}

uint32_t GotoOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    buf->EmitU32(para);
    return GetDynChunkfSize(*buf);
}

void GotoOpCode::UpdateOpPara(DynChunk *buf, uint32_t offset, uint32_t para) const
{
    buf->PutU32(offset + 1, para);
}

uint32_t GotoOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "goto\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t SplitNextOpCode::InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    buf->PutU32(offset + 1, para);
    return GetDynChunkfSize(*buf);
}

uint32_t SplitNextOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "split_next\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t SplitFirstOpCode::InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    buf->PutU32(offset + 1, para);
    return GetDynChunkfSize(*buf);
}

uint32_t SplitFirstOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "split_first\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t LoopOpCode::EmitOpCode(DynChunk *buf, uint32_t start, uint32_t min, uint32_t max) const
{
    buf->EmitChar(GetOpCode());
    buf->EmitU32(start);
    buf->EmitU32(min);
    buf->EmitU32(max);
    return GetDynChunkfSize(*buf);
}

uint32_t LoopOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "loop\t" << buf.GetU32(offset + 1) + offset + GetSize() << "\t"
        << buf.GetU32(offset + RegExpOpCode::OP_SIZE_FIVE) << "\t" << buf.GetU32(offset + RegExpOpCode::OP_SIZE_NINE)
        << std::endl;
    return offset + GetSize();
}

uint32_t LoopGreedyOpCode::EmitOpCode(DynChunk *buf, uint32_t start, uint32_t min, uint32_t max) const
{
    buf->EmitChar(GetOpCode());
    buf->EmitU32(start);
    buf->EmitU32(min);
    buf->EmitU32(max);
    return GetDynChunkfSize(*buf);
}

uint32_t LoopGreedyOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "greedy_loop\t" << buf.GetU32(offset + 1) + offset + GetSize() << "\t"
        << buf.GetU32(offset + RegExpOpCode::OP_SIZE_FIVE) << "\t" << buf.GetU32(offset + RegExpOpCode::OP_SIZE_NINE)
        << std::endl;
    return offset + GetSize();
}

uint32_t PushCharOpCode::InsertOpCode(DynChunk *buf, uint32_t offset) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t PushCharOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "push_char" << std::endl;
    return offset + GetSize();
}

uint32_t PushOpCode::InsertOpCode(DynChunk *buf, uint32_t offset) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t PushOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "push" << std::endl;
    return offset + GetSize();
}

uint32_t PopOpCode::EmitOpCode(DynChunk *buf) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t PopOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "pop" << std::endl;
    return offset + GetSize();
}

uint32_t CheckCharOpCode::EmitOpCode(DynChunk *buf, uint32_t offset) const
{
    buf->EmitChar(GetOpCode());
    buf->EmitU32(offset);
    return GetDynChunkfSize(*buf);
}

uint32_t CheckCharOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "check_char\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t SaveResetOpCode::InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t start, uint32_t end) const
{
    auto captureStart = static_cast<uint8_t>(start & 0xffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    auto captureEnd = static_cast<uint8_t>(end & 0xffU);      // NOLINTNEXTLINE(readability-magic-numbers)
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    buf->PutU8(offset + RegExpOpCode::OP_SIZE_ONE, captureStart);
    buf->PutU8(offset + RegExpOpCode::OP_SIZE_TWO, captureEnd);
    return GetDynChunkfSize(*buf);
}

uint32_t SaveResetOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "save_reset\t" << buf.GetU8(offset + RegExpOpCode::OP_SIZE_ONE) << "\t"
        << buf.GetU8(offset + RegExpOpCode::OP_SIZE_TWO) << std::endl;
    return offset + GetSize();
}

uint32_t MatchOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t MatchOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "match" << std::endl;
    return offset + GetSize();
}

uint32_t MatchEndOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t MatchEndOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "match_end" << std::endl;
    return offset + GetSize();
}

uint32_t LineStartOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t LineStartOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "line_start" << std::endl;
    return offset + GetSize();
}

uint32_t LineEndOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t LineEndOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "line_end" << std::endl;
    return offset + GetSize();
}

uint32_t WordBoundaryOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t WordBoundaryOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "word_boundary" << std::endl;
    return offset + GetSize();
}

uint32_t NotWordBoundaryOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t NotWordBoundaryOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf,
                                           uint32_t offset) const
{
    out << offset << ":\t"
        << "not_word_boundary" << std::endl;
    return offset + GetSize();
}

uint32_t AllOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t AllOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "all" << std::endl;
    return offset + GetSize();
}

uint32_t DotsOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t DotsOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "dots" << std::endl;
    return offset + GetSize();
}

uint32_t MatchAheadOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "match_ahead\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t RangeOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "range\t";
    size_t size = buf.GetU16(offset + 1);
    for (size_t i = 0; i < size; i++) {
        out << buf.GetU16(offset + RegExpOpCode::OP_SIZE_THREE + (i * RegExpOpCode::OP_SIZE_FOUR)) << "\t"
            << buf.GetU16(offset + RegExpOpCode::OP_SIZE_THREE +
                          (i * RegExpOpCode::OP_SIZE_FOUR + RegExpOpCode::OP_SIZE_TWO))
            << "\t";
    }
    out << std::endl;
    return offset + size * RegExpOpCode::OP_SIZE_FOUR + RegExpOpCode::OP_SIZE_THREE;
}

uint32_t RangeOpCode::InsertOpCode(DynChunk *buf, const RangeSet &rangeSet) const
{
    buf->EmitChar(GetOpCode());
    size_t size = rangeSet.rangeSet_.size();
    buf->EmitU16(size);
    for (auto range : rangeSet.rangeSet_) {
        buf->EmitU16(range.first);
        buf->EmitU16(range.second);
    }
    return GetDynChunkfSize(*buf);
}

uint32_t Range32OpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "range32\t";
    size_t size = buf.GetU16(offset + 1);
    for (size_t i = 0; i < size; i++) {
        out << buf.GetU32(offset + RegExpOpCode::OP_SIZE_THREE + (i * RegExpOpCode::OP_SIZE_EIGHT)) << "\t"
            << buf.GetU32(offset + RegExpOpCode::OP_SIZE_THREE +
                          (i * RegExpOpCode::OP_SIZE_EIGHT + RegExpOpCode::OP_SIZE_FOUR))
            << "\t";
    }
    out << std::endl;
    return offset + size * +RegExpOpCode::OP_SIZE_EIGHT + RegExpOpCode::OP_SIZE_THREE;
}

uint32_t Range32OpCode::InsertOpCode(DynChunk *buf, const RangeSet &rangeSet) const
{
    buf->EmitChar(GetOpCode());
    size_t size = rangeSet.rangeSet_.size();
    buf->EmitU16(size);
    for (auto range : rangeSet.rangeSet_) {
        buf->EmitU32(range.first);
        buf->EmitU32(range.second);
    }
    return GetDynChunkfSize(*buf);
}

uint32_t MatchAheadOpCode::InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    buf->PutU32(offset + 1, para);
    return GetDynChunkfSize(*buf);
}

uint32_t NegativeMatchAheadOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "negative_match_ahead\t" << buf.GetU32(offset + 1) + offset + GetSize() << std::endl;
    return offset + GetSize();
}

uint32_t NegativeMatchAheadOpCode::InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const
{
    buf->Insert(offset, GetSize());
    buf->PutU8(offset, GetOpCode());
    buf->PutU32(offset + 1, para);
    return GetDynChunkfSize(*buf);
}

uint32_t PrevOpCode::EmitOpCode(DynChunk *buf, [[maybe_unused]] uint32_t para) const
{
    buf->EmitChar(GetOpCode());
    return GetDynChunkfSize(*buf);
}

uint32_t PrevOpCode::DumpOpCode(std::ostream &out, [[maybe_unused]] const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "prev" << std::endl;
    return offset + GetSize();
}

uint32_t BackReferenceOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    auto capture = static_cast<uint8_t>(para & 0xffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    buf->EmitChar(GetOpCode());
    buf->EmitChar(capture);
    return GetDynChunkfSize(*buf);
}

uint32_t BackReferenceOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "backreference\t" << buf.GetU8(offset + 1) << std::endl;
    return offset + GetSize();
}

uint32_t BackwardBackReferenceOpCode::EmitOpCode(DynChunk *buf, uint32_t para) const
{
    auto capture = static_cast<uint8_t>(para & 0xffU);  // NOLINTNEXTLINE(readability-magic-numbers)
    buf->EmitChar(GetOpCode());
    buf->EmitChar(capture);
    return GetDynChunkfSize(*buf);
}

uint32_t BackwardBackReferenceOpCode::DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const
{
    out << offset << ":\t"
        << "backward_backreference\t" << buf.GetU8(offset + 1) << std::endl;
    return offset + GetSize();
}

void RangeSet::Insert(uint32_t start, uint32_t end)
{
    if (start > end) {
        return;
    }
    std::pair<uint32_t, uint32_t> pairElement = std::make_pair(start, end);
    if (rangeSet_.empty()) {
        rangeSet_.emplace_back(pairElement);
    } else {
        for (auto iter = rangeSet_.begin(); iter != rangeSet_.end(); iter++) {
            if (IsIntersect(start, end, iter->first, iter->second) ||
                IsAdjacent(start, end, iter->first, iter->second)) {
                iter->first = std::min(iter->first, start);
                iter->second = std::max(iter->second, end);
                return;
            }
            if (iter->first > end) {
                rangeSet_.insert(iter, pairElement);
                return;
            }
        }
        rangeSet_.emplace_back(pairElement);
    }
}

void RangeSet::Insert(const RangeSet &s1)
{
    if (s1.rangeSet_.empty()) {
        return;
    }
    if (rangeSet_.empty()) {
        rangeSet_ = s1.rangeSet_;
    } else {
        for (auto range : s1.rangeSet_) {
            Insert(range.first, range.second);
        }
        Compress();
    }
}

void RangeSet::Invert(bool isUtf16)
{
    uint32_t maxValue = isUtf16 ? UINT32_MAX : UINT16_MAX;
    if (rangeSet_.empty()) {
        rangeSet_.emplace_back(std::make_pair(0, maxValue));
        return;
    }

    auto iter = rangeSet_.begin();
    auto iter2 = rangeSet_.begin();
    if (iter->first == 0 && iter->second == maxValue) {
        rangeSet_.clear();
        return;
    }
    iter2++;

    uint32_t first = iter->first;

    for (iter = rangeSet_.begin(); iter != rangeSet_.end(); iter++) {
        if (iter->second == maxValue) {
            rangeSet_.erase(iter);
            break;
        }
        iter->first = iter->second + 1;
        if (iter2 != rangeSet_.end()) {
            iter->second = iter2->first - 1;
            iter2++;
        } else {
            iter->second = maxValue;
        }
    }
    if (first > 0) {
        std::pair<uint32_t, uint32_t> pair1 = std::make_pair(0, first - 1);
        rangeSet_.push_front(pair1);
    }
    Compress();
}

void RangeSet::Compress()
{
    auto iter = rangeSet_.begin();
    auto iter2 = rangeSet_.begin();
    iter2++;
    while (iter2 != rangeSet_.end()) {
        if (IsIntersect(iter->first, iter->second, iter2->first, iter2->second) ||
            IsAdjacent(iter->first, iter->second, iter2->first, iter2->second)) {
            iter->first = std::min(iter->first, iter2->first);
            iter->second = std::max(iter->second, iter2->second);
            iter2 = rangeSet_.erase(iter2);
        } else {
            iter++;
            iter2++;
        }
    }
}
}  // namespace panda::ecmascript
