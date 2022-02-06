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
#include "ecmascript/class_linker/program_object-inl.h"

namespace panda::ecmascript {
static const size_t MALLOC_SIZE_LIMIT = 2147483648; // Max internal memory used by the VM declared in options

JSPandaFileManager::~JSPandaFileManager()
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iter = loadedJSPandaFilesLock_.begin();
    while (iter != loadedJSPandaFilesLock_.end()) {
        const JSPandaFile *jsPandaFile = iter->first;
        ReleaseJSPandaFile(jsPandaFile);
        iter = loadedJSPandaFilesLock_.erase(iter);
    }
}

void JSPandaFileManager::IncreaseRefJSPandaFile(const JSPandaFile *jsPandaFile)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (auto &iter : loadedJSPandaFilesLock_) {
        const JSPandaFile *pf = iter.first;
        if ((pf == jsPandaFile) ||
            (!(jsPandaFile->GetJSPandaFileDesc().empty()) &&
            (jsPandaFile->GetJSPandaFileDesc() == pf->GetJSPandaFileDesc()))) {
            iter.second += 1;
            return;
        }
    }
    loadedJSPandaFilesLock_[jsPandaFile] = 1;
}

void JSPandaFileManager::TranslateJSPandafile(EcmaVM *vm, const JSPandaFile *jsPandaFile, const CString &methodName)
{
    PandaFileTranslator translator(vm, jsPandaFile);
    std::vector<BytecodeTranslationInfo> infoList {};
    translator.TranslateClasses(methodName, infoList);
}

JSHandle<Program> JSPandaFileManager::GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                      const CString &methodName)
{
    const JSPandaFile *findPf = FindJSPandaFile(jsPandaFile->GetJSPandaFileDesc());
    if (findPf == nullptr) {
        TranslateJSPandafile(vm, jsPandaFile, methodName);
        {
            os::memory::LockHolder lock(jsPandaFileLock_);
            findPf = FindJSPandaFile(jsPandaFile->GetJSPandaFileDesc());
            if (findPf != nullptr) { // already translated in other vm
                ReleaseJSPandaFile(jsPandaFile);
            } else {
                findPf = jsPandaFile;
            }
            IncreaseRefJSPandaFile(findPf);
        }
    } else { // already translated in other vm
        ReleaseJSPandaFile(jsPandaFile);
        IncreaseRefJSPandaFile(findPf);
    }

    PandaFileTranslator translator(vm, findPf);
    auto result = translator.GenerateProgram();
    JSThread *thread = vm->GetJSThread();
    return JSHandle<Program>(thread, result);
}

const JSPandaFile *JSPandaFileManager::FindJSPandaFile(const std::string &filename)
{
    if (filename.empty()) {
        return nullptr;
    }
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (auto iter : loadedJSPandaFilesLock_) {
        const JSPandaFile *pf = iter.first;
        if (pf->GetJSPandaFileDesc() == filename) {
            return pf;
        }
    }
    return nullptr;
}

const JSPandaFile *JSPandaFileManager::LoadPfAbc(const std::string &filename)
{
    const JSPandaFile *pf = FindJSPandaFile(filename);
    if (pf != nullptr) {
        return pf;
    }
    pf = JSPandaFile::CreateJSPandaFileFromPf(filename);
    return pf;
}

const JSPandaFile *JSPandaFileManager::LoadBufferAbc(const std::string &filename, const void *buffer, size_t size)
{
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }
    if (filename.empty()) { // empty filename
        return JSPandaFile::CreateJSPandaFileFromBuffer(buffer, size);
    }
    const JSPandaFile *pf = FindJSPandaFile(filename);
    if (pf != nullptr) {
        return pf;
    }
    pf = JSPandaFile::CreateJSPandaFileFromBuffer(buffer, size, filename);
    return pf;
}

void JSPandaFileManager::DecRefJSPandaFile(const JSPandaFile *jsPandaFile)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iter = loadedJSPandaFilesLock_.find(jsPandaFile);
    ASSERT(iter != loadedJSPandaFilesLock_.end());
    iter->second -= 1;
    if (iter->second <= 0) {
        ReleaseJSPandaFile(jsPandaFile);
        loadedJSPandaFilesLock_.erase(iter);
    }
}

void JSPandaFileManager::ReleaseJSPandaFile(const JSPandaFile *jsPandaFile)
{
    if (jsPandaFile == nullptr) {
        return;
    }
    delete jsPandaFile;
}

tooling::ecmascript::PtJSExtractor *JSPandaFileManager::GetOrCreatePtJSExtractor(const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    const JSPandaFile *existJSPandaFile = GetJSPandaFile(pf);
    if (existJSPandaFile == nullptr) {
        LOG_ECMA(FATAL) << "can not get PtJSExtrjsPandaFile from unknown jsPandaFile";
        return nullptr;
    }
    return const_cast<JSPandaFile *>(existJSPandaFile)->GetOrCreatePtJSExtractor();
}

const JSPandaFile *JSPandaFileManager::GetOrCreateJSPandaFile(const panda_file::File *pf)
{
    const JSPandaFile *jsPandaFile = GetJSPandaFile(pf);
    if (jsPandaFile != nullptr) {
        return jsPandaFile;
    }
    jsPandaFile = new JSPandaFile(pf);
    return jsPandaFile;
}

const JSPandaFile *JSPandaFileManager::GetJSPandaFile(const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iter = loadedJSPandaFilesLock_.begin();
    while (iter != loadedJSPandaFilesLock_.end()) {
        const JSPandaFile *jsPandafile = iter->first;
        if (jsPandafile->GetPandaFile() == pf) {
            return jsPandafile;
        }
    }
    return nullptr;
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

void JSPandaFileManager::RemoveJSPandaFile(void *pointer, void *data)
{
    if (pointer == nullptr || data == nullptr) {
        return;
    }
    auto jsPandaFile = reinterpret_cast<JSPandaFile *>(pointer);
    // dec ref in filemanager and remove jsPandaFile
    EcmaVM *vm = static_cast<EcmaVM *>(data);
    vm->RemoveJSPandaFileRecord(jsPandaFile);

    JSPandaFileManager *jsPandaFileManager = vm->GetJSPandaFileManager();
    ASSERT(jsPandaFileManager != nullptr);
    jsPandaFileManager->DecRefJSPandaFile(jsPandaFile);
}
}  // namespace panda::ecmascript
