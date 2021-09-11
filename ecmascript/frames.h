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

#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

namespace panda::ecmascript {
enum class FrameType: unsigned int {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME = 1,
    INTERPRETER_FRAME = 2,
};

template<typename Enumeration>
auto as_integer(Enumeration const value)
-> typename std::underlying_type<Enumeration>::type
{
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

class FrameStateBase {
public:
    FrameType frameType;
    uint64_t *prev; // for llvm :c-fp ; for interrupt: thread-fp for gc
};

class LLVMBoundaryFrameState {
public:
    uint64_t *threadFp; // for gc
    FrameStateBase base;
};

constexpr int kSystemPointerSize = sizeof(void*);
class FrameConst {
public:
    uint64_t *prev;
    FrameType frameType;
    static constexpr int kPreOffset = -kSystemPointerSize;
    static constexpr int kFrameType = -2 * kSystemPointerSize;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H