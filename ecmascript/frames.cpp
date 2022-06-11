#include "ecmascript/frames.h"
#include <typeinfo>
namespace panda::ecmascript {

#define FRAME_AND_TYPE_LIST(V)                                           \
    V(OptimizedFrame, OPTIMIZED_FRAME)                                   \
    V(OptimizedEntryFrame, OPTIMIZED_ENTRY_FRAME)                        \
    V(OptimizedJSFunctionFrame, OPTIMIZED_JS_FUNCTION_FRAME)             \
    V(OptimizedLeaveFrame, LEAVE_FRAME)                                  \
    V(OptimizedWithArgvLeaveFrame, LEAVE_FRAME_WITH_ARGV)                \
    V(InterpretedFrame, INTERPRETER_FRAME)                               \
    V(AsmInterpretedFrame, ASM_INTERPRETER_FRAME)                        \
    V(AsmInterpretedFrame, INTERPRETER_CONSTRUCTOR_FRAME)                \
    V(BuiltinFrame, BUILTIN_FRAME)                                       \
    V(BuiltinWithArgvFrame, BUILTIN_FRAME_WITH_ARGV)                     \
    V(BuiltinFrame, BUILTIN_ENTRY_FRAME)                                 \
    V(InterpretedFrame, INTERPRETER_FAST_NEW_FRAME)                      \
    V(InterpretedEntryFrame, INTERPRETER_ENTRY_FRAME)                    \
    V(AsmInterpretedEntryFrame, ASM_INTERPRETER_ENTRY_FRAME)             \
    V(OptimizedJSFunctionFrame, OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME)

#define IMPLEMENT_GET_FRAME(Frame)                  \
    template<>                                      \
    Frame* FrameIterator::GetFrame()                \
    {                                               \
        return Frame::GetFrameFromSp(current_);     \
    }
    FRAME_LIST(IMPLEMENT_GET_FRAME)
#undef IMPLEMENT_GET_FRAME

void FrameIterator::Advance()
{
    ASSERT(!done());
    FrameType t = GetFrameType();
    switch (t) {
        #define CASE(FRAME, Type)               \
        case FrameType::Type : {                \
            auto frame = GetFrame<FRAME>();     \
            current_ = frame->GetPrevFrameFp(); \
            break;                              \
        }
        FRAME_AND_TYPE_LIST(CASE)
        #undef CASE
        default: {
            UNREACHABLE();
        }
    }
}
}  // namespace panda::ecmascript
