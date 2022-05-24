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

#include "message_string.h"
#include <array>
#include "libpandabase/macros.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static std::array<std::string, MessageString::MAX_MESSAGE_COUNT> g_messageString = {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_COMMON_MESSAGE(name, string) (string),
    COMMON_MESSAGE_STRING_LIST(DEF_COMMON_MESSAGE)
#undef DEF_COMMON_MESSAGE
#define DEF_ASM_INTERPRETER_STUB_MESSAGE(name) #name,
    ASM_INTERPRETER_BC_STUB_LIST(DEF_ASM_INTERPRETER_STUB_MESSAGE, DEF_ASM_INTERPRETER_STUB_MESSAGE,
                              DEF_ASM_INTERPRETER_STUB_MESSAGE)
    ASM_INTERPRETER_BC_HELPER_STUB_LIST(DEF_ASM_INTERPRETER_STUB_MESSAGE)
#undef DEF_ASM_INTERPRETER_STUB_MESSAGE
};

const std::string& MessageString::GetMessageString(int id)
{
    ASSERT(id < MessageString::MAX_MESSAGE_COUNT);
    return g_messageString[id];
}
}  // namespace panda::ecmascript