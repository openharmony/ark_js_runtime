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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H

#include "ecmascript/class_linker/panda_file_translator.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tooling/backend/js_pt_extractor.h"
#include "libpandafile/file.h"
#include "libpandabase/utils/logger.h"

namespace panda {
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
#define ENTRY_FUNCTION_NAME "func_main_0"

class JSPandaFile {
public:
    JSPandaFile(const panda_file::File *pf, const CString &descriptor);
    ~JSPandaFile();

    tooling::JSPtExtractor *GetJSPtExtractor();

    CString GetJSPandaFileDesc() const
    {
        return desc_;
    }

    const panda_file::File *GetPandaFile() const
    {
        return pf_;
    }

    JSMethod *GetMethods() const
    {
        return methods_;
    }

    void SetMethodToMap(JSMethod *method)
    {
        if (method != nullptr) {
            methodMap_.emplace(method->GetFileId().GetOffset(), method);
        }
    }

    uint32_t GetNumMethods() const
    {
        return numMethods_;
    }

    uint32_t GetConstpoolIndex() const
    {
        return constpoolIndex_;
    }

    uint32_t GetMainMethodIndex() const
    {
        return mainMethodIndex_;
    }

    const std::unordered_map<uint32_t, uint64_t> &GetConstpoolMap() const
    {
        return constpoolMap_;
    }

    uint32_t GetOrInsertConstantPool(ConstPoolType type, uint32_t offset);

    void UpdateMainMethodIndex(uint32_t mainMethodIndex)
    {
        mainMethodIndex_ = mainMethodIndex;
    }

    const JSMethod *FindMethods(uint32_t offset) const;

    Span<const uint32_t> GetClasses() const
    {
        return pf_->GetClasses();
    }

private:
    void InitMethods();

    uint32_t constpoolIndex_ {0};
    std::unordered_map<uint32_t, uint64_t> constpoolMap_;
    uint32_t numMethods_ {0};
    uint32_t mainMethodIndex_ {0};
    JSMethod *methods_ {nullptr};
    CUnorderedMap<uint32_t, JSMethod *> methodMap_;
    const panda_file::File *pf_ {nullptr};
    std::unique_ptr<tooling::JSPtExtractor> JSPtExtractor_;
    CString desc_;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_H
