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

#include "ecmascript/regexp/regexp_executor.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/regexp/dyn_chunk.h"
#include "ecmascript/regexp/regexp_opcode.h"
#include "securec.h"

namespace panda::ecmascript {
using RegExpState = RegExpExecutor::RegExpState;
using MatchResult = RegExpExecutor::MatchResult;
bool RegExpExecutor::Execute(const uint8_t *input, uint32_t lastIndex, uint32_t length, uint8_t *buf, bool isWideChar)
{
    DynChunk buffer(buf, chunk_);
    input_ = const_cast<uint8_t *>(input);
    inputEnd_ = const_cast<uint8_t *>(input + length * (isWideChar ? WIDE_CHAR_SIZE : CHAR_SIZE));
    uint32_t size = buffer.GetU32(0);
    nCapture_ = buffer.GetU32(RegExpParser::NUM_CAPTURE__OFFSET);
    nStack_ = buffer.GetU32(RegExpParser::NUM_STACK_OFFSET);
    flags_ = buffer.GetU32(RegExpParser::FLAGS_OFFSET);
    isWideChar_ = isWideChar;

    uint32_t captureResultSize = sizeof(CaptureState) * nCapture_;
    uint32_t stackSize = sizeof(uintptr_t) * nStack_;
    stateSize_ = sizeof(RegExpState) + captureResultSize + stackSize;
    stateStackLen_ = 0;

    if (captureResultSize != 0) {
        captureResultList_ = chunk_->NewArray<CaptureState>(nCapture_);
        if (memset_s(captureResultList_, captureResultSize, 0, captureResultSize) != EOK) {
            LOG_ECMA(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    if (stackSize != 0) {
        stack_ = chunk_->NewArray<uintptr_t>(nStack_);
        if (memset_s(stack_, stackSize, 0, stackSize) != EOK) {
            LOG_ECMA(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    SetCurrentPtr(input + lastIndex * (isWideChar ? WIDE_CHAR_SIZE : CHAR_SIZE));
    SetCurrentPC(RegExpParser::OP_START_OFFSET);

    // first split
    if ((flags_ & RegExpParser::FLAG_STICKY) == 0) {
        PushRegExpState(STATE_SPLIT, RegExpParser::OP_START_OFFSET);
    }
    return ExecuteInternal(buffer, size);
}

bool RegExpExecutor::MatchFailed(bool isMatched)
{
    while (true) {
        if (stateStackLen_ == 0) {
            return true;
        }
        RegExpState *state = PeekRegExpState();
        if (state->type_ == StateType::STATE_SPLIT) {
            if (!isMatched) {
                PopRegExpState();
                return false;
            }
        } else {
            isMatched = (state->type_ == StateType::STATE_MATCH_AHEAD && isMatched) ||
                        (state->type_ == StateType::STATE_NEGATIVE_MATCH_AHEAD && !isMatched);
            if (isMatched) {
                if (state->type_ == StateType::STATE_MATCH_AHEAD) {
                    PopRegExpState(false);
                    return false;
                }
                if (state->type_ == StateType::STATE_NEGATIVE_MATCH_AHEAD) {
                    PopRegExpState();
                    return false;
                }
            }
        }
        DropRegExpState();
    }

    return true;
}

bool RegExpExecutor::HandleFirstSplit()
{
    if (GetCurrentPC() == RegExpParser::OP_START_OFFSET && stateStackLen_ == 0 &&
        (flags_ & RegExpParser::FLAG_STICKY) == 0) {
        if (IsEOF()) {
            if (MatchFailed()) {
                return false;
            }
        } else {
            AdvanceCurrentPtr();
            PushRegExpState(STATE_SPLIT, RegExpParser::OP_START_OFFSET);
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpAll(uint8_t opCode)
{
    if (IsEOF()) {
        return !MatchFailed();
    }
    uint32_t currentChar = GetCurrentChar();
    if ((opCode == RegExpOpCode::OP_DOTS) && IsTerminator(currentChar)) {
        return !MatchFailed();
    }
    Advance(opCode);
    return true;
}

bool RegExpExecutor::HandleOpChar(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t expectedChar;
    if (opCode == RegExpOpCode::OP_CHAR32) {
        expectedChar = byteCode.GetU32(GetCurrentPC() + 1);
    } else {
        expectedChar = byteCode.GetU16(GetCurrentPC() + 1);
    }
    if (IsEOF()) {
        return !MatchFailed();
    }
    uint32_t currentChar = GetCurrentChar();
    if (IsIgnoreCase()) {
        currentChar = RegExpParser::Canonicalize(currentChar, IsUtf16());
    }
    if (currentChar == expectedChar) {
        Advance(opCode);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpWordBoundary(uint8_t opCode)
{
    if (IsEOF()) {
        if (opCode == RegExpOpCode::OP_WORD_BOUNDARY) {
            Advance(opCode);
        } else {
            if (MatchFailed()) {
                return false;
            }
        }
        return true;
    }
    bool preIsWord = false;
    if (GetCurrentPtr() != input_) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        preIsWord = IsWordChar(PeekPrevChar(currentPtr_, input_));
    }
    bool currentIsWord = IsWordChar(PeekChar(currentPtr_, inputEnd_));
    if (((opCode == RegExpOpCode::OP_WORD_BOUNDARY) &&
        ((!preIsWord && currentIsWord) || (preIsWord && !currentIsWord))) ||
        ((opCode == RegExpOpCode::OP_NOT_WORD_BOUNDARY) &&
        ((preIsWord && currentIsWord) || (!preIsWord && !currentIsWord)))) {
        Advance(opCode);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpLineStart(uint8_t opCode)
{
    if (IsEOF()) {
        return !MatchFailed();
    }
    if ((GetCurrentPtr() == input_) ||
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ((flags_ & RegExpParser::FLAG_MULTILINE) != 0 && PeekPrevChar(currentPtr_, input_) == '\n')) {
        Advance(opCode);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpLineEnd(uint8_t opCode)
{
    if (IsEOF() ||
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ((flags_ & RegExpParser::FLAG_MULTILINE) != 0 && PeekChar(currentPtr_, inputEnd_) == '\n')) {
        Advance(opCode);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

void RegExpExecutor::HandleOpSaveStart(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t captureIndex = byteCode.GetU8(GetCurrentPC() + 1);
    ASSERT(captureIndex < nCapture_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    CaptureState *captureState = &captureResultList_[captureIndex];
    captureState->captureStart = GetCurrentPtr();
    Advance(opCode);
}

void RegExpExecutor::HandleOpSaveEnd(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t captureIndex = byteCode.GetU8(GetCurrentPC() + 1);
    ASSERT(captureIndex < nCapture_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    CaptureState *captureState = &captureResultList_[captureIndex];
    captureState->captureEnd = GetCurrentPtr();
    Advance(opCode);
}

void RegExpExecutor::HandleOpSaveReset(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t catpureStartIndex = byteCode.GetU8(GetCurrentPC() + SAVE_RESET_START);
    uint32_t catpureEndIndex = byteCode.GetU8(GetCurrentPC() + SAVE_RESET_END);
    for (uint32_t i = catpureStartIndex; i <= catpureEndIndex; i++) {
        CaptureState *captureState =
            &captureResultList_[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        captureState->captureStart = nullptr;
        captureState->captureEnd = nullptr;
    }
    Advance(opCode);
}

void RegExpExecutor::HandleOpMatch(const DynChunk &byteCode, uint8_t opCode)
{
    auto type = static_cast<StateType>(opCode - RegExpOpCode::OP_SPLIT_NEXT);
    ASSERT(type == STATE_SPLIT || type == STATE_MATCH_AHEAD || type == STATE_NEGATIVE_MATCH_AHEAD);
    uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
    Advance(opCode);
    uint32_t splitPc = GetCurrentPC() + offset;
    PushRegExpState(type, splitPc);
}

void RegExpExecutor::HandleOpSplitFirst(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
    Advance(opCode);
    PushRegExpState(STATE_SPLIT, GetCurrentPC());
    AdvanceOffset(offset);
}

bool RegExpExecutor::HandleOpPrev(uint8_t opCode)
{
    if (GetCurrentPtr() == input_) {
        if (MatchFailed()) {
            return false;
        }
    } else {
        PrevPtr(&currentPtr_, input_);
        Advance(opCode);
    }
    return true;
}

void RegExpExecutor::HandleOpLoop(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t quantifyMin = byteCode.GetU32(GetCurrentPC() + LOOP_MIN_OFFSET);
    uint32_t quantifyMax = byteCode.GetU32(GetCurrentPC() + LOOP_MAX_OFFSET);
    uint32_t pcOffset = byteCode.GetU32(GetCurrentPC() + LOOP_PC_OFFSET);
    Advance(opCode);
    uint32_t loopPcEnd = GetCurrentPC();
    uint32_t loopPcStart = GetCurrentPC() + pcOffset;
    bool isGreedy = opCode == RegExpOpCode::OP_LOOP_GREEDY;
    uint32_t loopMax = isGreedy ? quantifyMax : quantifyMin;

    uint32_t loopCount = PeekStack();
    SetStackValue(++loopCount);
    if (loopCount < loopMax) {
        // greedy failed, goto next
        if (loopCount >= quantifyMin) {
            PushRegExpState(STATE_SPLIT, loopPcEnd);
        }
        // Goto loop start
        SetCurrentPC(loopPcStart);
    } else {
        if (!isGreedy && (loopCount < quantifyMax)) {
            PushRegExpState(STATE_SPLIT, loopPcStart);
        }
    }
}

bool RegExpExecutor::HandleOpRange32(const DynChunk &byteCode)
{
    if (IsEOF()) {
        return !MatchFailed();
    }
    uint32_t currentChar = GetCurrentChar();
    if (IsIgnoreCase()) {
        currentChar = RegExpParser::Canonicalize(currentChar, IsUtf16());
    }
    uint16_t rangeCount = byteCode.GetU16(GetCurrentPC() + 1);
    bool isFound = false;
    int32_t idxMin = 0;
    int32_t idxMax = rangeCount - 1;
    int32_t idx = 0;
    uint32_t low = 0;
    uint32_t high =
        byteCode.GetU32(GetCurrentPC() + RANGE32_HEAD_OFFSET + idxMax * RANGE32_MAX_OFFSET + RANGE32_MAX_HALF_OFFSET);
    if (currentChar <= high) {
        while (idxMin <= idxMax) {
            idx = (idxMin + idxMax) / RANGE32_OFFSET;
            low = byteCode.GetU32(GetCurrentPC() + RANGE32_HEAD_OFFSET + idx * RANGE32_MAX_OFFSET);
            high = byteCode.GetU32(GetCurrentPC() + RANGE32_HEAD_OFFSET + idx * RANGE32_MAX_OFFSET +
                RANGE32_MAX_HALF_OFFSET);
            if (currentChar < low) {
                idxMax = idx - 1;
            } else if (currentChar > high) {
                idxMin = idx + 1;
            } else {
                isFound = true;
                break;
            }
        }
    }
    if (isFound) {
        AdvanceOffset(rangeCount * RANGE32_MAX_OFFSET + RANGE32_HEAD_OFFSET);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpRange(const DynChunk &byteCode)
{
    if (IsEOF()) {
        return !MatchFailed();
    }
    uint32_t currentChar = GetCurrentChar();
    if (IsIgnoreCase()) {
        currentChar = RegExpParser::Canonicalize(currentChar, IsUtf16());
    }
    uint16_t rangeCount = byteCode.GetU16(GetCurrentPC() + 1);
    bool isFound = false;
    int32_t idxMin = 0;
    int32_t idxMax = rangeCount - 1;
    int32_t idx = 0;
    uint32_t low = 0;
    uint32_t high =
        byteCode.GetU16(GetCurrentPC() + RANGE32_HEAD_OFFSET + idxMax * RANGE32_MAX_HALF_OFFSET + RANGE32_OFFSET);
    if (currentChar <= high) {
        while (idxMin <= idxMax) {
            idx = (idxMin + idxMax) / RANGE32_OFFSET;
            low = byteCode.GetU16(GetCurrentPC() + RANGE32_HEAD_OFFSET + idx * RANGE32_MAX_HALF_OFFSET);
            high =
                byteCode.GetU16(GetCurrentPC() + RANGE32_HEAD_OFFSET + idx * RANGE32_MAX_HALF_OFFSET + RANGE32_OFFSET);
            if (currentChar < low) {
                idxMax = idx - 1;
            } else if (currentChar > high) {
                idxMin = idx + 1;
            } else {
                isFound = true;
                break;
            }
        }
    }
    if (isFound) {
        AdvanceOffset(rangeCount * RANGE32_MAX_HALF_OFFSET + RANGE32_HEAD_OFFSET);
    } else {
        if (MatchFailed()) {
            return false;
        }
    }
    return true;
}

bool RegExpExecutor::HandleOpBackReference(const DynChunk &byteCode, uint8_t opCode)
{
    uint32_t captureIndex = byteCode.GetU8(GetCurrentPC() + 1);
    if (captureIndex >= nCapture_) {
        return !MatchFailed();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const uint8_t *captureStart = captureResultList_[captureIndex].captureStart;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const uint8_t *captureEnd = captureResultList_[captureIndex].captureEnd;
    if (captureStart == nullptr || captureEnd == nullptr) {
        Advance(opCode);
        return true;
    }
    bool isMatched = true;
    if (opCode == RegExpOpCode::OP_BACKREFERENCE) {
        const uint8_t *refCptr = captureStart;
        while (refCptr < captureEnd) {
            if (IsEOF()) {
                isMatched = false;
                break;
            }
            // NOLINTNEXTLINE(readability-identifier-naming)
            uint32_t c1 = GetChar(&refCptr, captureEnd);
            // NOLINTNEXTLINE(readability-identifier-naming)
            uint32_t c2 = GetChar(&currentPtr_, inputEnd_);
            if (IsIgnoreCase()) {
                c1 = RegExpParser::Canonicalize(c1, IsUtf16());
                c2 = RegExpParser::Canonicalize(c2, IsUtf16());
            }
            if (c1 != c2) {
                isMatched = false;
                break;
            }
        }
        if (!isMatched) {
            if (MatchFailed()) {
                return false;
            }
        } else {
            Advance(opCode);
        }
    } else {
        const uint8_t *refCptr = captureEnd;
        while (refCptr > captureStart) {
            if (GetCurrentPtr() == input_) {
                isMatched = false;
                break;
            }
            // NOLINTNEXTLINE(readability-identifier-naming)
            uint32_t c1 = GetPrevChar(&refCptr, captureStart);
            // NOLINTNEXTLINE(readability-identifier-naming)
            uint32_t c2 = GetPrevChar(&currentPtr_, input_);
            if (IsIgnoreCase()) {
                c1 = RegExpParser::Canonicalize(c1, IsUtf16());
                c2 = RegExpParser::Canonicalize(c2, IsUtf16());
            }
            if (c1 != c2) {
                isMatched = false;
                break;
            }
        }
        if (!isMatched) {
            if (MatchFailed()) {
                return false;
            }
        } else {
            Advance(opCode);
        }
    }
    return true;
}

// NOLINTNEXTLINE(readability-function-size)
bool RegExpExecutor::ExecuteInternal(const DynChunk &byteCode, uint32_t pcEnd)
{
    while (GetCurrentPC() < pcEnd) {
        // first split
        if (!HandleFirstSplit()) {
            return false;
        }
        uint8_t opCode = byteCode.GetU8(GetCurrentPC());
        switch (opCode) {
            case RegExpOpCode::OP_DOTS:
            case RegExpOpCode::OP_ALL:
                if (!HandleOpAll(opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_CHAR32:
            case RegExpOpCode::OP_CHAR:
                if (!HandleOpChar(byteCode, opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_NOT_WORD_BOUNDARY:
            case RegExpOpCode::OP_WORD_BOUNDARY:
                if (!HandleOpWordBoundary(opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_LINE_START:
                if (!HandleOpLineStart(opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_LINE_END:
                if (!HandleOpLineEnd(opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_SAVE_START:
                HandleOpSaveStart(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SAVE_END:
                HandleOpSaveEnd(byteCode, opCode);
                break;
            case RegExpOpCode::OP_GOTO: {
                uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
                Advance(opCode, offset);
            } break;
            case RegExpOpCode::OP_MATCH: {
                // jump to match ahead
                if (MatchFailed(true)) {
                    return false;
                }
            } break;
            case RegExpOpCode::OP_MATCH_END: {
                return true;
            } break;
            case RegExpOpCode::OP_SAVE_RESET:
                HandleOpSaveReset(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SPLIT_NEXT:
            case RegExpOpCode::OP_MATCH_AHEAD:
            case RegExpOpCode::OP_NEGATIVE_MATCH_AHEAD:
                HandleOpMatch(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SPLIT_FIRST:
                HandleOpSplitFirst(byteCode, opCode);
                break;
            case RegExpOpCode::OP_PREV:
                if (!HandleOpPrev(opCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_LOOP_GREEDY:
            case RegExpOpCode::OP_LOOP:
                HandleOpLoop(byteCode, opCode);
                break;
            case RegExpOpCode::OP_PUSH_CHAR: {
                PushStack(reinterpret_cast<uintptr_t>(GetCurrentPtr()));
                Advance(opCode);
            } break;
            case RegExpOpCode::OP_CHECK_CHAR: {
                if (PopStack() != reinterpret_cast<uintptr_t>(GetCurrentPtr())) {
                    Advance(opCode);
                } else {
                    uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
                    Advance(opCode, offset);
                }
            } break;
            case RegExpOpCode::OP_PUSH: {
                PushStack(0);
                Advance(opCode);
            } break;
            case RegExpOpCode::OP_POP: {
                PopStack();
                Advance(opCode);
            } break;
            case RegExpOpCode::OP_RANGE32:
                if (!HandleOpRange32(byteCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_RANGE:
                if (!HandleOpRange(byteCode)) {
                    return false;
                }
                break;
            case RegExpOpCode::OP_BACKREFERENCE:
            case RegExpOpCode::OP_BACKWARD_BACKREFERENCE:
                if (!HandleOpBackReference(byteCode, opCode)) {
                    return false;
                }
                break;
            default:
                UNREACHABLE();
        }
    }
    // for loop match
    return true;
}

void RegExpExecutor::DumpResult(std::ostream &out) const
{
    out << "captures:" << std::endl;
    for (uint32_t i = 0; i < nCapture_; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CaptureState *captureState = &captureResultList_[i];
        int32_t len = captureState->captureEnd - captureState->captureStart;
        if ((captureState->captureStart != nullptr && captureState->captureEnd != nullptr) && (len >= 0)) {
            out << i << ":\t" << CString(reinterpret_cast<const char *>(captureState->captureStart), len)
                << std::endl;
        } else {
            out << i << ":\t"
                << "undefined" << std::endl;
        }
    }
}

MatchResult RegExpExecutor::GetResult(const JSThread *thread, bool isSuccess) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    MatchResult result;
    std::vector<std::pair<bool, JSHandle<EcmaString>>> captures;
    result.isSuccess_ = isSuccess;
    if (isSuccess) {
        for (uint32_t i = 0; i < nCapture_; i++) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            CaptureState *captureState = &captureResultList_[i];
            if (i == 0) {
                result.index_ = captureState->captureStart - input_;
                if (isWideChar_) {
                    result.index_ /= WIDE_CHAR_SIZE;
                }
            }
            int32_t len = captureState->captureEnd - captureState->captureStart;
            std::pair<bool, JSHandle<EcmaString>> pair;
            if ((captureState->captureStart != nullptr && captureState->captureEnd != nullptr) && (len >= 0)) {
                pair.first = false;
                if (isWideChar_) {
                    // create utf-16 string
                    pair.second =
                        factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(captureState->captureStart), len / 2);
                } else {
                    // create utf-8 string
                    CVector<uint8_t> buffer(len + 1);
                    uint8_t *dest = buffer.data();
                    if (memcpy_s(dest, len + 1, reinterpret_cast<const uint8_t *>(captureState->captureStart), len) !=
                        EOK) {
                        LOG_ECMA(FATAL) << "memcpy_s failed";
                        UNREACHABLE();
                    }
                    dest[len] = '\0';  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    pair.second = factory->NewFromUtf8(reinterpret_cast<const uint8_t *>(buffer.data()), len);
                }
            } else {
                // undefined
                pair.first = true;
            }
            captures.emplace_back(pair);
        }
        result.captures_ = captures;
        result.endIndex_ = currentPtr_ - input_;
        if (isWideChar_) {
            result.endIndex_ /= WIDE_CHAR_SIZE;
        }
    }
    return result;
}

void RegExpExecutor::PushRegExpState(StateType type, uint32_t pc)
{
    ReAllocStack(stateStackLen_ + 1);
    auto state = reinterpret_cast<RegExpState *>(
        stateStack_ +  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        stateStackLen_ * stateSize_);
    state->type_ = type;
    state->currentPc_ = pc;
    state->currentStack_ = currentStack_;
    state->currentPtr_ = GetCurrentPtr();
    size_t listSize = sizeof(CaptureState) * nCapture_;
    if (memcpy_s(state->captureResultList_, listSize, GetCaptureResultList(), listSize) != EOK) {
        LOG_ECMA(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    uint8_t *stackStart =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        reinterpret_cast<uint8_t *>(state->captureResultList_) + sizeof(CaptureState) * nCapture_;
    if (stack_ != nullptr) {
        size_t stackSize = sizeof(uintptr_t) * nStack_;
        if (memcpy_s(stackStart, stackSize, stack_, stackSize) != EOK) {
            LOG_ECMA(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    stateStackLen_++;
}

RegExpState *RegExpExecutor::PopRegExpState(bool copyCaptrue)
{
    if (stateStackLen_ != 0) {
        auto state = PeekRegExpState();
        size_t listSize = sizeof(CaptureState) * nCapture_;
        if (copyCaptrue) {
            if (memcpy_s(GetCaptureResultList(), listSize, state->captureResultList_, listSize) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
        }
        SetCurrentPtr(state->currentPtr_);
        SetCurrentPC(state->currentPc_);
        currentStack_ = state->currentStack_;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uint8_t *stackStart = reinterpret_cast<uint8_t *>(state->captureResultList_) + listSize;
        if (stack_ != nullptr) {
            size_t stackSize = sizeof(uintptr_t) * nStack_;
            if (memcpy_s(stack_, stackSize, stackStart, stackSize) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
        }
        stateStackLen_--;
        return state;
    }
    return nullptr;
}

void RegExpExecutor::ReAllocStack(uint32_t stackLen)
{
    if (stackLen > stateStackSize_) {
        uint32_t newStackSize = std::max(stateStackSize_ * 2, MIN_STACK_SIZE);  // 2: double the size
        uint32_t stackByteSize = newStackSize * stateSize_;
        auto newStack = chunk_->NewArray<uint8_t>(stackByteSize);
        if (memset_s(newStack, stackByteSize, 0, stackByteSize) != EOK) {
            LOG_ECMA(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        if (stateStack_ != nullptr) {
            size_t stackSize = stateStackSize_ * stateSize_;
            if (memcpy_s(newStack, stackSize, stateStack_, stackSize) != EOK) {
                return;
            }
        }
        stateStack_ = newStack;
        stateStackSize_ = newStackSize;
    }
}
}  // namespace panda::ecmascript
