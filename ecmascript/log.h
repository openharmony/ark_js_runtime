/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_LOG_H
#define ECMASCRIPT_LOG_H

#include <sstream>

#include "hilog/log.h"

constexpr static unsigned int DOMAIN = 0xD003F00;
constexpr static auto TAG = "ArkCompiler";
constexpr static OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, DOMAIN, TAG};

namespace panda::ecmascript {
template<LogLevel level>
class Log {
public:
    Log() = default;
    ~Log()
    {
        if constexpr (level == LOG_DEBUG) {
            OHOS::HiviewDFX::HiLog::Debug(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_INFO) {
            OHOS::HiviewDFX::HiLog::Info(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_WARN) {
            OHOS::HiviewDFX::HiLog::Warn(LABEL, "%{public}s", stream_.str().c_str());
        } else if constexpr (level == LOG_ERROR) {
            OHOS::HiviewDFX::HiLog::Error(LABEL, "%{public}s", stream_.str().c_str());
        } else {
            OHOS::HiviewDFX::HiLog::Fatal(LABEL, "%{public}s", stream_.str().c_str());
        }
    }
    template<class type>
    std::ostream &operator <<(type input)
    {
        stream_ << input;
        return stream_;
    }

private:
    std::ostringstream stream_;
};
}  // namespace panda::ecmascript

#define HILOG(level) HiLogIsLoggable(DOMAIN, TAG, LOG_##level) && panda::ecmascript::Log<LOG_##level>()

#endif  // ECMASCRIPT_LOG_H
