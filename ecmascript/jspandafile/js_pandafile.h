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

#ifndef ECMASCRIPT_JS_PANDAFILE_H
#define ECMASCRIPT_JS_PANDAFILE_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/class_linker/panda_file_translator.h"
#include "ecmascript/tooling/pt_js_extractor.h"
#include "libpandafile/file.h"
#include "libpandabase/utils/logger.h"

namespace panda {
namespace panda_file {
class File;
}  // namespace panda_file

namespace ecmascript {
class JSPandaFile {
public:
    JSPandaFile(const panda_file::File *pf);
    JSPandaFile(const panda_file::File *pf, bool isFrameWork);
    ~JSPandaFile();

    static JSPandaFile *CreateJSPandaFileFromPf(const std::string &filename);
    static JSPandaFile *CreateJSPandaFileFromBuffer(const void *buffer, size_t size);
    static JSPandaFile *CreateJSPandaFileFromBuffer(const void *buffer, size_t size, const std::string &filename);

    tooling::ecmascript::PtJSExtractor *GetOrCreatePtJSExtractor();

    std::string GetJSPandaFileDesc() const
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

    std::unordered_map<uint32_t, uint64_t> GetConstpoolMap() const
    {
        return constpoolMap_;
    }

    void SaveTranslatedInfo(uint32_t constpoolIndex, uint32_t numMethods, uint32_t mainMethodIndex, JSMethod *methods,
                            std::unordered_map<uint32_t, uint64_t> &constpoolMap);
    void ParseMethods();

    Span<JSMethod> GetMethodSpan() const
    {
        return {methods_, numMethods_};
    }

    void UpdateConstpoolInfo(uint32_t constpoolIndex, std::unordered_map<uint32_t, uint64_t> &constpoolMap)
    {
        constpoolIndex_ = constpoolIndex;
        constpoolMap_ = constpoolMap;
    }

    void UpdateMainMethodIndex(uint32_t mainMethodIndex)
    {
        mainMethodIndex_ = mainMethodIndex;
    }

    const JSMethod *FindMethods(uint32_t offset) const;

    Span<const uint32_t> GetClasses() const
    {
        return pf_->GetClasses();
    }

    bool IsFrameWorkerPandaFile() const
    {
        return isFrameWorker_;
    }

private:
    uint32_t constpoolIndex_ {0};
    uint32_t numMethods_ {0};
    uint32_t mainMethodIndex_ {0};
    JSMethod *methods_ {nullptr};
    std::unordered_map<uint32_t, uint64_t> constpoolMap_;
    const panda_file::File *pf_ {nullptr};
    bool isFrameWorker_ {false};
    std::unique_ptr<tooling::ecmascript::PtJSExtractor> ptJSExtractor_;
    std::string desc_;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JS_PANDAFILE_H
