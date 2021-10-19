/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
#define ECMASCRIPT_JS_RUNTIME_OPTIONS_H_

#include "runtime/include/runtime_options.h"
#include "utils/logger.h"

// namespace panda {
namespace panda::ecmascript {
class JSRuntimeOptions : public RuntimeOptions {
public:
    explicit JSRuntimeOptions(const std::string &exe_path = "") : RuntimeOptions(exe_path) {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    void AddOptions(PandArgParser *parser)
    {
        RuntimeOptions::AddOptions(parser);
        parser->Add(&enable_ark_tools_);
    }

    bool IsEnableArkTools() const
    {
        return enable_ark_tools_.GetValue();
    }

    void SetEnableArkTools(bool value)
    {
        enable_ark_tools_.SetValue(value);
    }

    bool WasSetEnableArkTools() const
    {
        return enable_ark_tools_.WasSet();
    }

private:
    PandArg<bool> enable_ark_tools_{"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
