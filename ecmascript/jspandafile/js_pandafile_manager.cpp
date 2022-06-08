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

#include "ecmascript/jspandafile/js_pandafile_manager.h"

#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript {
static const size_t MALLOC_SIZE_LIMIT = 2147483648; // Max internal memory used by the VM declared in options

JSPandaFileManager *JSPandaFileManager::GetInstance()
{
    static JSPandaFileManager jsFileManager;
    return &jsFileManager;
}

JSPandaFileManager::~JSPandaFileManager()
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto pos = extractors_.begin();
    while (pos != extractors_.end()) {
        pos = extractors_.erase(pos);
    }

    auto iter = loadedJSPandaFiles_.begin();
    while (iter != loadedJSPandaFiles_.end()) {
        const JSPandaFile *jsPandaFile = iter->first;
        ReleaseJSPandaFile(jsPandaFile);
        iter = loadedJSPandaFiles_.erase(iter);
    }
}

// generate aot info on host
const JSPandaFile *JSPandaFileManager::LoadAotInfoFromPf(const CString &filename, std::string_view entryPoint,
                                                         std::vector<MethodPcInfo> *methodPcInfos)
{
    JSPandaFile *jsPandaFile = OpenJSPandaFile(filename);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    CString methodName;
    auto pos = entryPoint.find_last_of("::");
    if (pos != std::string_view::npos) {
        methodName = entryPoint.substr(pos + 1);
    } else {
        // default use func_main_0 as entryPoint
        methodName = JSPandaFile::ENTRY_FUNCTION_NAME;
    }

    PandaFileTranslator::TranslateClasses(jsPandaFile, methodName, methodPcInfos);
    InsertJSPandaFile(jsPandaFile);
    return jsPandaFile;
}

const JSPandaFile *JSPandaFileManager::LoadJSPandaFile(const CString &filename, std::string_view entryPoint)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JSPandaFileManager::LoadJSPandaFile");
    const JSPandaFile *jsPandaFile = FindJSPandaFile(filename);
    if (jsPandaFile != nullptr) {
        IncreaseRefJSPandaFile(jsPandaFile);
        return jsPandaFile;
    }

    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    jsPandaFile = GenerateJSPandaFile(pf.release(), filename, entryPoint);
    return jsPandaFile;
}

const JSPandaFile *JSPandaFileManager::LoadJSPandaFile(const CString &filename, std::string_view entryPoint,
                                                       const void *buffer, size_t size)
{
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }

    const JSPandaFile *jsPandaFile = FindJSPandaFile(filename);
    if (jsPandaFile != nullptr) {
        IncreaseRefJSPandaFile(jsPandaFile);
        return jsPandaFile;
    }

    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }
    jsPandaFile = GenerateJSPandaFile(pf.release(), filename, entryPoint);
    return jsPandaFile;
}

JSHandle<Program> JSPandaFileManager::GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "JSPandaFileManager::GenerateProgram");
    LOG_ECMA(INFO) << "GenerateProgram " << jsPandaFile->GetJSPandaFileDesc();
    ASSERT(GetJSPandaFile(jsPandaFile->GetPandaFile()) != nullptr);

    JSHandle<Program> program = PandaFileTranslator::GenerateProgram(vm, jsPandaFile);
    return program;
}

const JSPandaFile *JSPandaFileManager::FindJSPandaFile(const CString &filename)
{
    if (filename.empty()) {
        return nullptr;
    }
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        const JSPandaFile *pf = iter.first;
        if (pf->GetJSPandaFileDesc() == filename) {
            return pf;
        }
    }
    return nullptr;
}

const JSPandaFile *JSPandaFileManager::GetJSPandaFile(const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        const JSPandaFile *jsPandafile = iter.first;
        if (jsPandafile->GetPandaFile() == pf) {
            return jsPandafile;
        }
    }
    return nullptr;
}

void JSPandaFileManager::InsertJSPandaFile(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA(INFO) << "InsertJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();

    os::memory::LockHolder lock(jsPandaFileLock_);
    ASSERT(loadedJSPandaFiles_.find(jsPandaFile) == loadedJSPandaFiles_.end());
    loadedJSPandaFiles_[jsPandaFile] = 1;
}

void JSPandaFileManager::IncreaseRefJSPandaFile(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA(INFO) << "IncreaseRefJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();

    os::memory::LockHolder lock(jsPandaFileLock_);
    ASSERT(loadedJSPandaFiles_.find(jsPandaFile) != loadedJSPandaFiles_.end());
    loadedJSPandaFiles_[jsPandaFile]++;
}

void JSPandaFileManager::DecreaseRefJSPandaFile(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA(INFO) << "DecreaseRefJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();

    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iter = loadedJSPandaFiles_.find(jsPandaFile);
    if (iter != loadedJSPandaFiles_.end()) {
        if (iter->second > 1) {
            iter->second--;
            return;
        }
        loadedJSPandaFiles_.erase(iter);
    }
    extractors_.erase(jsPandaFile);
    ReleaseJSPandaFile(jsPandaFile);
}

JSPandaFile *JSPandaFileManager::OpenJSPandaFile(const CString &filename)
{
    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    JSPandaFile *jsPandaFile = NewJSPandaFile(pf.release(), filename);
    return jsPandaFile;
}

JSPandaFile *JSPandaFileManager::NewJSPandaFile(const panda_file::File *pf, const CString &desc)
{
    return new JSPandaFile(pf, desc);
}

void JSPandaFileManager::ReleaseJSPandaFile(const JSPandaFile *jsPandaFile)
{
    if (jsPandaFile == nullptr) {
        return;
    }
    LOG_ECMA(INFO) << "ReleaseJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();
    delete jsPandaFile;
}

tooling::JSPtExtractor *JSPandaFileManager::GetJSPtExtractor(const JSPandaFile *jsPandaFile)
{
    LOG_IF(jsPandaFile == nullptr, FATAL, ECMASCRIPT) << "GetJSPtExtractor error, js pandafile is nullptr";

    os::memory::LockHolder lock(jsPandaFileLock_);
    ASSERT(loadedJSPandaFiles_.find(jsPandaFile) != loadedJSPandaFiles_.end());

    auto iter = extractors_.find(jsPandaFile);
    if (iter == extractors_.end()) {
        auto extractorPtr = std::make_unique<tooling::JSPtExtractor>(jsPandaFile);
        tooling::JSPtExtractor *extractor = extractorPtr.get();
        extractors_[jsPandaFile] = std::move(extractorPtr);
        return extractor;
    }

    return iter->second.get();
}

const JSPandaFile *JSPandaFileManager::GenerateJSPandaFile(const panda_file::File *pf, const CString &desc,
                                                           std::string_view entryPoint)
{
    ASSERT(GetJSPandaFile(pf) == nullptr);
    JSPandaFile *newJsPandaFile = NewJSPandaFile(pf, desc);

    CString methodName;
    auto pos = entryPoint.find_last_of("::");
    if (pos != std::string_view::npos) {
        methodName = entryPoint.substr(pos + 1);
    } else {
        // default use func_main_0 as entryPoint
        methodName = JSPandaFile::ENTRY_FUNCTION_NAME;
    }

    PandaFileTranslator::TranslateClasses(newJsPandaFile, methodName);
    {
        os::memory::LockHolder lock(jsPandaFileLock_);
        const JSPandaFile *jsPandaFile = FindJSPandaFile(desc);
        if (jsPandaFile != nullptr) {
            IncreaseRefJSPandaFile(jsPandaFile);
            ReleaseJSPandaFile(newJsPandaFile);
            return jsPandaFile;
        }
        InsertJSPandaFile(newJsPandaFile);
    }

    return newJsPandaFile;
}

void *JSPandaFileManager::AllocateBuffer(size_t size)
{
    return JSPandaFileAllocator::AllocateBuffer(size);
}

void *JSPandaFileManager::JSPandaFileAllocator::AllocateBuffer(size_t size)
{
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    if (size >= MALLOC_SIZE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "size must be less than the maximum";
        UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *ptr = malloc(size);
    if (ptr == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed";
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ptr, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA_MEM(FATAL) << "memset failed";
        UNREACHABLE();
    }
#endif
    return ptr;
}

void JSPandaFileManager::FreeBuffer(void *mem)
{
    JSPandaFileAllocator::FreeBuffer(mem);
}

void JSPandaFileManager::JSPandaFileAllocator::FreeBuffer(void *mem)
{
    if (mem == nullptr) {
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void JSPandaFileManager::RemoveJSPandaFile(void *pointer, void *data)
{
    if (pointer == nullptr || data == nullptr) {
        return;
    }
    auto jsPandaFile = static_cast<JSPandaFile *>(pointer);
    LOG_ECMA(INFO) << "RemoveJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();
    // dec ref in filemanager
    JSPandaFileManager *jsPandaFileManager = static_cast<JSPandaFileManager *>(data);
    jsPandaFileManager->DecreaseRefJSPandaFile(jsPandaFile);
}
}  // namespace panda::ecmascript
