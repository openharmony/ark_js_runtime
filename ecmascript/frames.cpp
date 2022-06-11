#include "ecmascript/frames.h"
namespace panda::ecmascript {

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