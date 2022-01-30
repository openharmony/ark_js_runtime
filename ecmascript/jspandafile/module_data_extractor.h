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

#ifndef ECMASCRIPT_MODULE_DATA_EXTRACTOR_H
#define ECMASCRIPT_MODULE_DATA_EXTRACTOR_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
using EntityId = panda_file::File::EntityId;

class ModuleDataExtractor {
public:
    explicit ModuleDataExtractor() = default;
    virtual ~ModuleDataExtractor() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(ModuleDataExtractor);
    DEFAULT_COPY_SEMANTIC(ModuleDataExtractor);

    static void ExtractModuleDatas(JSThread *thread, const panda_file::File &pf,
                                   panda_file::File::EntityId moduleId,
                                   JSHandle<SourceTextModule> &moduleRecord);
    static JSHandle<JSTaggedValue> ParseModule(JSThread *thread, const panda_file::File &pf,
                                               const std::string &descriptor);
    static bool FindModule(const panda_file::File &pf);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_DATA_EXTRACTOR_H
