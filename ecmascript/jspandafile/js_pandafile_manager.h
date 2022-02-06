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

#ifndef ECMASCRIPT_JS_PANDAFILE_MANAGER_H
#define ECMASCRIPT_JS_PANDAFILE_MANAGER_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/class_linker/panda_file_translator.h"
#include "ecmascript/tooling/pt_js_extractor.h"
#include "libpandafile/file.h"
#include "libpandabase/utils/logger.h"

namespace panda {
namespace panda_file {
class File;
class EcmaVm;
}  // namespace panda_file

namespace ecmascript {
class Program;

class JsPandaFileInfo {
public:
    JsPandaFileInfo(const panda_file::File *pf);
    ~JsPandaFileInfo();
    void IncreaseRefCount()
    {
        refCount_++;
    }

    void DecreaseRefCount(int times = 1)
    {
        refCount_ -= times;
    }

    bool IsUsing()
    {
        return refCount_ > 0;
    }

private:
    tooling::ecmascript::PtJSExtractor *GetOrCreatePtJSExtractor();

    uint32_t constpoolIndex_ {0};
    uint32_t numMethods_ {0};
    uint32_t mainMethodIndex_ {0};
    JSMethod *methods_ {nullptr};
    std::unordered_map<uint32_t, uint64_t> constpoolMap_;
    const panda_file::File *pf_ {nullptr};
    std::unique_ptr<tooling::ecmascript::PtJSExtractor> ptJSExtractor_;
    int refCount_ {0};

    friend class JsPandaFileManager;
    friend class PandaFileTranslator;
};

class JsPandaFileManager {
public:
    JsPandaFileManager() = default;
    ~JsPandaFileManager();
    JSHandle<Program> GenerateProgram(EcmaVM *vm, const std::string &filename,
                                       const panda_file::File *pf, const CString &methodName);
    const panda_file::File* LoadPfAbc(EcmaVM *vm, const std::string &filename, const CString &methodName);
    const panda_file::File* LoadBufferAbc(EcmaVM *vm, const std::string &filename, const void *buffer,
                                          size_t size, const CString &methodName);
    void DecRefJsPandaFile(const std::string &filename, const panda_file::File *pf, int decRefCount);
    void DecRefJsPandaFile(const panda_file::File *pf, int decRefCount);
    tooling::ecmascript::PtJSExtractor *GetOrCreatePtJSExtractor(const panda_file::File *pf);

private:
    JsPandaFileInfo *FindJsPandaFile(const std::string &descriptor, const panda_file::File *pf);
    void InsertJsPandaFile(const std::string &descriptor, JsPandaFileInfo *jsPandaFileInfo);
    JsPandaFileInfo *TranslatePandafile(EcmaVM *vm, const std::string &filename,
                                  const panda_file::File *pf, const CString &methodName);
    bool IsAvailableDescriptor(const std::string &descriptor);
    void ReleaseJsPandaFileInfo(JsPandaFileInfo *jsPandaFileInfo);

    os::memory::RecursiveMutex jsPandaFileLock_;
    CUnorderedMap<std::string, JsPandaFileInfo *> loadedJsPandaFiles_;
    CVector<JsPandaFileInfo *> illegalDescJsPandaFiles_;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JS_PANDAFILE_MANAGER_H
