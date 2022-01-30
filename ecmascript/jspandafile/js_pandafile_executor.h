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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_EXECUTOR_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_EXECUTOR_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
class JSPandaFileExecutor {
public:
    static bool ExecuteFromFile(JSThread *thread, const std::string &filename, std::string_view entryPoint,
                                const std::vector<std::string> &args);
    static bool ExecuteFromBuffer(JSThread *thread, const void *buffer, size_t size, std::string_view entryPoint,
                                  const std::vector<std::string> &args, const std::string &filename = "");
private:
    static bool Execute(JSThread *thread, const JSPandaFile *jsPandaFile, std::string_view entryPoint,
                        const std::vector<std::string> &args);

friend class SourceTextModule;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_EXECUTOR_H
