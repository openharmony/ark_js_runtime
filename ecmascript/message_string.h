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

#ifndef ECMASCRIPT_MESSAGE_STRING_H
#define ECMASCRIPT_MESSAGE_STRING_H

#include <string>

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define MESSAGE_STRING_LIST(V)                                     \
    V(SetReadOnlyProperty, "Cannot set readonly property")         \
    V(FunctionCallNotConstructor, "class constructor cannot call") \
    V(SetPropertyWhenNotExtensible, "Cannot add property in prevent extensions ")

class MessageString {
public:
    enum MessageId {
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_MESSAGE_ID(name, string) Message_##name,
        MESSAGE_STRING_LIST(DEF_MESSAGE_ID)
#undef DEF_MESSAGE_ID
            MAX_MESSAGE_COUNT
    };
    static const std::string& GetMessageString(int id);
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_MESSAGE_STRING_ID(name) static_cast<int>((MessageString::MessageId::Message_##name))
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MESSAGE_STRING_H