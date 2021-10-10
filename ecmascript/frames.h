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

//    in aot project, three Frame: Interpreter Frame、Runtime Frame、Optimized Frame.  Optimized Frame split
// Optimized Entry Frame(interpreter call stub) and Optimized Frame(stub call stub) by call scenario.
//    ​we gc trigger, we skip Runtime Frame, thus define OPTIMIZED_FRAME、OPTIMIZED_ENTRY_FRAME、INTERPRETER_FRAME respectively
// represent optimized frame、optimized entry frame、interpreter frame.

// Frame Layout
// Interpreter Frame(alias   **iframe** ) Layout as follow:
// ```
//   +---------------------------------+--------------------+
//   |             argv[n-1]            |                   ^
//   |----------------------------------|                   |
//   |              ........            |                   |
//   |             argv[0]              |                   |
//   |----------------------------------|                   |
//   |              thisArg             |                   |
//   |----------------------------------|                   |
//   |              newTarget           |                   |
//   |----------------------------------|                   |
//   |              callTarget          |                   |
//   +----------------------------------+--------+          |
//   |       FrameType                  |        ^      interpreter frame
//   |----------------------------------|        |          |
//   |      pre(pre stack pointer)      |        |          |
//   |----------------------------------|        |          |
//   |        numActualArgs             |        |          |
//   |----------------------------------|        |          |
//   |        env                       |        |          |
//   |----------------------------------|        |          |
//   |        acc                       |    FrameState     |
//   |----------------------------------|        |          |
//   |        constantpool              |        |          |
//   |----------------------------------|        |          |
//   |        method                    |        |          |
//   |----------------------------------|        |          |
//   |        sp(current stack point)   |        |          |
//   |----------------------------------|        |          |
//   |        pc(bytecode addr)         |        v          v
//   +----------------------------------+--------+----------+
// ```
// address space grow from high address to low address.we add new field  **FrameType** ,
// the field's value is INTERPRETER_FRAME(represent interpreter frame).
// **currentsp**  is pointer to  callTarget field address, sp field 's value is  **currentsp** ,
// pre field pointer pre stack frame point. fill JSthread's sp field with iframe sp field
// by  calling JSThread->SetCurrentSPFrame and save pre Frame address to iframe pre field.

// Runtime  Frame: comply with C-ABI without custom modify, function generat frame.
// ​
// Optimized Frame and Optimized Entry Frame, we also comply with C-ABI,
//the difference from Runtime Frame is that prologue and epilogue is customed.

// Optimized Entry Frame layout as follow, we reserve two stack slot for saving eparately new field  **FrameType**
// which's value is OPTIMIZED_ENTRY_FRAME and
// new field pre that's value is iframe.sp by calling JSThread->GetCurrentSPFrame.
// fp field point to pre frame,  **currentfp**  is pointer to fp field address.
// save JSthread's sp  to pre field and  fill JSthread's sp field with  **currentfp** .


// ```
//     +---------------------------------------------------+
//     |  parameter i         |                            ^
//     |- - - - - - - - - --  |                            |
//     |  parameter n-2       |                          Caller frame
//     |       ...            |                         comply c-abi
//     |  parameter n-1       |                        paramters push to stack from left to right
//     |- - - - - - - - -     |                       i-n th prameter spill to slot
//     |  parameter n         |                            v
//     +--------------------------+------------------------+
//     |   return addr        |   ^                        ^
//     |- - - - - - - - -     |   |                        |
//     |         fp           | Fixed                      |
//     |- - - - - - - - -     | Header <-- frame ptr       |
//     |  FrameType           |   |                       callee frame Header
//     |- - - - - - - - -     |   |                        |
//     |   pre                |   v                        |
//     +--------------------------+------------------------+
// ```

// ​	Optimized Frame layout as follow, we reserve one stack slot for saving FrameType field,
// which's value is OPTIMIZED_FRAME.

// ```
//     +---------------------------------------------------+
//     |  parameter n         |                            ^
//     |- - - - - - - - - --  |                            |
//     |  parameter n-1       |                          Caller frame
//     |       ...            |                          comply c-abi
//     |  parameter n-2       |                       paramters push to stack from left to right
//     |- - - - - - - - -     |                       i-n th prameter spill to slot
//     |  parameter i         |                            v
//     +--------------------------+------------------------+
//     |   return addr        |   ^                        ^
//     |- - - - - - - - -     |   |                        |
//     |         fp           | Fixed                      |
//     |- - - - - - - - -     | Header <-- frame ptr      callee frame Header
//     |  FrameType           |   V                        |
//     +--------------------------+------------------------+
// ```

// ​	JSthread's sp will be updated dynamic, the scenarios is as follows by different Frames:
// **SAVE**  is to save JSthread's sp to current Frame pre field,
// **UPDATE**  is to fill  JSthread's sp with currentsp or currentfp when current frame is iframe
// or Optimized EntryFrame/Optimized Frame.
// **nop**  represent don't call SAVE or UPDATE,  **illegal**  represent this secnarios don't existed.

// ```
//              	iframe	         OptimizedEntry	  Optimized	  RunTime
// iframe	     	SAVE/UPATE	     SAVE	         illegal	SAVE/UPATE
// OptimizedEntry	UPATE		     illegal	     nop	         UPATE
// Optimized	    UPATE	         illegal	     nop	         UPATE
// RunTime		    nop		          illegal	     illegal	      nop
// ```

// ​	Iterator Frame from Runtime Frame,  the process is as flollows:

// 1 calling JSThread->GetCurrentSPFrame get Current Frame, then get FrameType field

// 2  Iterator previous Frame:

// ​    accessing fp field when the current frame is Optimized Frame;

// ​    accessing pre field when the current frame is Optimized Entry Frame or Interpreter Frame

// 3 repeat process 2 until Current Frame is nullptr

// For Example:
// ```
//                     call                call                 call
//     foo    -----------------> bar   --------------->baz---------------------> rtfunc
// (interpret frame)         (Optimized Entry Frame)   (Optimized Frame)      (Runtime Frame)
// ```

// Frame Layout as follow:
// ```
// +---------------------------------+--------------------+
//   |             argv[n-1]            |                   ^
//   |----------------------------------|                   |
//   |              ........            |                   |
//   |             argv[0]              |                   |
//   |----------------------------------|                   |
//   |              thisArg             |                   |
//   |----------------------------------|                   |
//   |              newTarget           |                   |
//   |----------------------------------|                   |
//   |              callTarget          |                   |
//   +----------------------------------+--------+          |
//   |       FrameType                  |        ^      foo's frame
//   |----------------------------------|        |          |
//   |      pre(pre stack pointer)      |        |          |
//   |----------------------------------|        |          |
//   |        numActualArgs             |        |          |
//   |----------------------------------|        |          |
//   |        env                       |        |          |
//   |----------------------------------|        |          |
//   |        acc                       |    FrameState     |
//   |----------------------------------|        |          |
//   |        constantpool              |        |          |
//   |----------------------------------|        |          |
//   |        method                    |        |          |
//   |----------------------------------|        |          |
//   |        sp(current stack point)   |        |          |
//   |----------------------------------|        |          |
//   |        pc(bytecode addr)         |        v          v
//   +----------------------------------+--------+----------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |   return addr        |   ^                           ^
//   |- - - - - - - - -     |   |                           |
//   |         fp           | Fixed                         |
//   |- - - - - - - - -     | Header <-- frame ptr          |
//   |  FrameType           |   |                       bar's frame Header
//   |- - - - - - - - -     |   |                           |
//   |   pre                |   v                           |
//   +--------------------------+---------------------------+
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |   return addr        |   ^                           ^
//   |- - - - - - - - -     |   |                           |
//   |         fp           | Fixed                         |
//   |- - - - - - - - -     | Header <-- frame ptr          |
//   |  FrameType           |   v                       baz's frame Header
//   |                   .............                      |
//   +--------------------------+---------------------------+
//   |                                                      |
//   |                 rtfunc's Frame                       |
//   |                                                      |
//   +------------------------------------------------------+
// ```
// Iterator:
// rtfunc get baz's Frame **currentfp** by calling GetCurrentSPFrame.
// baz'Frame fp field point to bar's Frame **currentfp**, then get bar's Frame pre field.
// bar's Frame pre field point to foo's Frame **currentsp**.
// finally we can iterator foo's Frame.

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