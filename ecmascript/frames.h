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

// OptimizedEntry Frame are divided up into four regions.
// The first region, Caller Frame: Interpreter Call Stub comply c calling convention, if parameter number
// larger than caller register number, then spill to stack frame.
// The second region, Fixed Header: return addr、saved c frame ptr(pointer to caller frame)、OPTIMIZED_ENTRY_FRAME
// (codegen prologue insert frame type)、saved gc frame ptr(pointer to interpreter iframe)
// The third region, which contains the callee-saved registers must be
// reserved after register allocation, since its size can only be precisely
// determined after register allocation once the number of used callee-saved
// register is certain.
// The fourth region is spill slot. Comply c calling convertion, local variable is larger callee save register,
// spill local variable to stack; heap ptr also spill stack slot before call runtime function, after call we will
// rewrite heap ptr
//     slot  OptimizedEntry frame
//        +-----------------+--------------------------------
//        |  parameter n    |                            ^
//        |- - - - - - - - -|                            |
//        |  parameter n-1  |                          Caller
//        |       ...       |                          frame
//        |  parameter n-2  |                       comply c calling convention
//        |- - - - - - - - -|                       i-n th prameter spill to slot
//        |  parameter i    |                            v
//   -----+-----------------+--------------------------------
//    0   |   return addr       |   ^                        ^
//        |- - - - - - - - -    |   |                        |
//    1   | saved c frame ptr   | Fixed                      |
//        |- - - - - - - - -    | Header <-- frame ptr       |
//    2   |OPTIMIZED_ENTRY_FRAME|   |                        |
//        |- - - - - - - - -    |   |                        |
//    3   | saved gc frame ptr  |   v                        |
//        +-----------------+----                            |
//    4   |  callee-saved 1 |   ^                            |
//        |- - - - - - - - -|   |                            |
//        |      ...        | Callee-saved                   |
//        |- - - - - - - - -|   |                            |
//  r+3   |  callee-saved r |   v                            |
//        +-----------------+----                            |
//                              ^
//  r+4   | local variables |   |                            |
//        |- - - - - - - - -|   |                            |
//        |      ...        | spill slot                     |
//        |- - - - - - - - -|   |                            |
//        |  heap ptr       |   v                            v
//   -----+-----------------+----- <-- stack ptr -------------


//  Optimized Frame is similar to OptimizedEntry Frame, only difference is Fixed Header.
//   slot  OptimizedEntry frame
//        +-----------------+--------------------------------
//        |  parameter n    |                            ^
//        |- - - - - - - - -|                            |
//        |  parameter n-1  |                          Caller
//        |       ...       |                          frame
//        |  parameter n-2  |                       comply c calling convention
//        |- - - - - - - - -|                       i-n th prameter spill to slot
//        |  parameter i    |                            v
//   -----+-----------------+--------------------------------
//    0   |   return addr       |   ^                        ^
//        |- - - - - - - - -    |   |                        |
//    1   | saved c frame ptr   | Fixed                      |
//        |- - - - - - - - -    | Header <-- frame ptr       |
//    2   |  OPTIMIZED_FRAME    |   |                        |
//    3   | - - - - - - - - -   |   v                        |
//        +-----------------+----                            |
//    4   |  callee-saved 1 |   ^                            |
//        |- - - - - - - - -|   |                            |
//        |      ...        | Callee-saved                   |
//        |- - - - - - - - -|   |                            |
//  r+3   |  callee-saved r |   v                            |
//        +-----------------+----                            |
//                              ^
//  r+4   | local variables |   |                            |
//        |- - - - - - - - -|   |                            |
//        |      ...        | spill slot                     |
//        |- - - - - - - - -|   |                            |
//        |  heap ptr       |   v                            v
//   -----+-----------------+----- <-- stack ptr -------------

//  for exampe:
//          calls        calls
//     foo --------> bar ------> baz
// (interpreted)  (compiled)  (runtime)
//  baz trigger gc, baz get current frame pointer(bar gc frame pointer), then visit foo's
//  interpreter frame by reading bar gc frame pointer value
//   ----+----------------+ <--- ManagedThread::GetCurrentFrame()
// /     |                |                     |
//       |                |                     |
//   baz |                |                     |
//       | return address |                     |
//  ---- +----------------+                     |
//       |      data      |                     |
//       +----------------+                     |
//       +----------------+ <+------------------
//  bar  | pointer to the | ----------------------+
//       | interpreter    |                       |
//       | frame(gc frame |                       |
//       |  pointer)      |                       |
//       +----------------+                       |
//       +----------------+                       |
//       | OPTIMIZED_ENTRY_                       |
//       |   FRAME        |                       |
//       +----------------+                       |
//       | c frame pointer| <-----+
//       | return address |       |               |
//       |                |       |               |
//  ---- +----------------+       |               |
//  E    | native frame   |       |               |
//  x u  | of             |       |               |
//  e t  | interpreter    |       |               |
//  c e  |c frame pointer | <---- |               |
//  ---- +----------------+                       |
//       |      ...       |                       |
//       +----------------+ <+--------------------
//       | foo's          |
//       | interpreter    |
//       | frame          |
//       +----------------+
#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

namespace panda::ecmascript {
class JSThread;
enum class FrameType: uint64_t {
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

class OptFrameStateBase {
public:
    FrameType frameType;
    uint64_t *prev; // for llvm :c-fp ; for interrupt: thread-fp for gc
};

class InterPretFrameStateBase {
public:
    uint64_t *prev; // for llvm :c-fp ; for interrupt: thread-fp for gc
    FrameType frameType;
};

class LLVMOptimizedEntryFrameState {
public:
    uint64_t *threadFp; // for gc
    OptFrameStateBase base;
};

constexpr int kSystemPointerSize = sizeof(void*);
class FrameConst {
public:
    uint64_t *prev;
    FrameType frameType;
    static constexpr int kFrameType = -kSystemPointerSize;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H