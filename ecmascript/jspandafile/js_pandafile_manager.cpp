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
#include "ecmascript/jspandafile/js_pandafile_allocator.h"
#include "ecmascript/class_linker/program_object-inl.h"

namespace panda::ecmascript {
JsPandaFileInfo::JsPandaFileInfo(const panda_file::File *pf) : pf_(pf)
{
}

JsPandaFileInfo::~JsPandaFileInfo()
{
    if (pf_ != nullptr) {
        delete pf_;
        pf_ = nullptr;
    }
    if (methods_ != nullptr) {
        JsPandaFileAllocator::FreeBuffer(methods_);
        methods_ = nullptr;
    }
}

JsPandaFileManager::~JsPandaFileManager()
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iter = illegalDescJsPandaFiles_.begin();
    while (iter != illegalDescJsPandaFiles_.end()) {
        delete *iter;
        iter++;
    }
    illegalDescJsPandaFiles_.clear();

    auto it = loadedJsPandaFiles_.begin();
    while (it != loadedJsPandaFiles_.end()) {
        JsPandaFileInfo *jsPandaFile = it->second;
        ReleaseJsPandaFileInfo(jsPandaFile);
        it++;
    }
    loadedJsPandaFiles_.clear();
}

JsPandaFileInfo *JsPandaFileManager::FindJsPandaFile(const std::string &descriptor, const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto it = loadedJsPandaFiles_.find(descriptor);
    if (it != loadedJsPandaFiles_.cend()) {
        return it->second;
    }
    if (pf != nullptr) {
        for (auto iter = illegalDescJsPandaFiles_.begin(); iter != illegalDescJsPandaFiles_.end();) {
            JsPandaFileInfo *jsPandaFile = *iter;
            if (jsPandaFile->pf_ == pf) {
                return jsPandaFile;
            }
        }
    }
    return nullptr;
}

bool JsPandaFileManager::IsAvailableDescriptor(const std::string &descriptor)
{
    if (descriptor.empty()) { // empty descriptor is illegal
        return false;
    }

    return true;
}

void JsPandaFileManager::InsertJsPandaFile(const std::string &descriptor, JsPandaFileInfo *jsPandaFileInfo)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    if (!IsAvailableDescriptor(descriptor)) {
        illegalDescJsPandaFiles_.emplace_back(jsPandaFileInfo);
        return;
    }
    ASSERT(FindJsPandaFile(descriptor, nullptr) == nullptr);
    loadedJsPandaFiles_.insert({descriptor, jsPandaFileInfo});
}

JsPandaFileInfo *JsPandaFileManager::TranslatePandafile(EcmaVM *vm, const std::string &filename,
                                                        const panda_file::File *pf, const CString &methodName)
{
    std::string descriptor = filename;
    JsPandaFileInfo *jsPandaFile = nullptr;
    {
        os::memory::LockHolder lock(jsPandaFileLock_);
        jsPandaFile = FindJsPandaFile(descriptor, pf);
        if (jsPandaFile != nullptr) {
            jsPandaFile->IncreaseRefCount();
            return jsPandaFile;
        }
    }
    PandaFileTranslator translator(vm);
    std::vector<BytecodeTranslationInfo> infoList {};
    jsPandaFile = translator.TranslateClasses(*pf, methodName, infoList);

    os::memory::LockHolder lock(jsPandaFileLock_);
    JsPandaFileInfo *existJsPandaFile = FindJsPandaFile(descriptor, jsPandaFile->pf_);
    if (existJsPandaFile != nullptr) {
        ReleaseJsPandaFileInfo(jsPandaFile); // descriptor JsPandaFileInfo have translated in other thread;
        existJsPandaFile->IncreaseRefCount();
        return existJsPandaFile;
    }
    InsertJsPandaFile(descriptor, jsPandaFile);
    jsPandaFile->IncreaseRefCount();
    return jsPandaFile;
}

JSHandle<Program> JsPandaFileManager::GenerateProgram(EcmaVM *vm, const std::string &filename,
                                                      const panda_file::File *pf, const CString &methodName)
{
    JsPandaFileInfo *jsPandaFile = TranslatePandafile(vm, filename, pf, methodName);
    ASSERT(jsPandaFile != nullptr);
    PandaFileTranslator translator(vm);
    auto result = translator.GenerateProgram(jsPandaFile);

    JSThread *thread = vm->GetJSThread();
    return JSHandle<Program>(thread, result);
}

const panda_file::File* JsPandaFileManager::LoadPfAbc(EcmaVM *vm, const std::string &filename,
                                                      const CString &methodName)
{
    std::string descriptor = filename;
    JsPandaFileInfo *jsPandaFile = FindJsPandaFile(descriptor, nullptr);
    if (jsPandaFile != nullptr) {
        return jsPandaFile->pf_;
    }
    auto pf = panda_file::OpenPandaFileOrZip(descriptor, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        return nullptr;
    }
    const panda_file::File *pfPtr = pf.release();
    PandaFileTranslator translator(vm);
    std::vector<BytecodeTranslationInfo> infoList {};
    jsPandaFile = translator.TranslateClasses(*pfPtr, methodName, infoList);

    os::memory::LockHolder lock(jsPandaFileLock_);
    JsPandaFileInfo *existJsPandaFile = FindJsPandaFile(descriptor, jsPandaFile->pf_);
    if (existJsPandaFile != nullptr) {
        ReleaseJsPandaFileInfo(jsPandaFile); // descriptor JsPandaFileInfo have translated in other thread;
        return existJsPandaFile->pf_;
    }
    InsertJsPandaFile(descriptor, jsPandaFile);
    return pfPtr;
}

const panda_file::File* JsPandaFileManager::LoadBufferAbc(EcmaVM *vm, const std::string &filename,
                                                          const void *buffer, size_t size, const CString &methodName)
{
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }
    std::string descriptor = filename;
    JsPandaFileInfo *jsPandaFile = FindJsPandaFile(descriptor, nullptr);
    if (jsPandaFile != nullptr) {
        return jsPandaFile->pf_;
    }
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        return nullptr;
    }
    const panda_file::File *pfPtr = pf.release();
    PandaFileTranslator translator(vm);
    std::vector<BytecodeTranslationInfo> infoList {};
    jsPandaFile = translator.TranslateClasses(*pfPtr, methodName, infoList);

    os::memory::LockHolder lock(jsPandaFileLock_);
    JsPandaFileInfo *existJsPandaFile = FindJsPandaFile(descriptor, jsPandaFile->pf_);
    if (existJsPandaFile != nullptr) {
        ReleaseJsPandaFileInfo(jsPandaFile); // descriptor JsPandaFileInfo have translated in other thread;
        return existJsPandaFile->pf_;
    }
    InsertJsPandaFile(descriptor, jsPandaFile);
    return pfPtr;
}

void JsPandaFileManager::DecRefJsPandaFile(const std::string &filename, const panda_file::File *pf, int decRefCount)
{
    std::string descriptor = filename;
    if (!IsAvailableDescriptor(descriptor)) {
        DecRefJsPandaFile(pf, decRefCount);
        return;
    }
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto it = loadedJsPandaFiles_.find(descriptor);
    if (it == loadedJsPandaFiles_.cend()) {
        return;
    }
    JsPandaFileInfo *jsPandaFile = it->second;
    jsPandaFile->DecreaseRefCount(decRefCount);
    if (!jsPandaFile->IsUsing()) {
        ReleaseJsPandaFileInfo(jsPandaFile);
        loadedJsPandaFiles_.erase(it);
    }
}

void JsPandaFileManager::DecRefJsPandaFile(const panda_file::File *pf, int decRefCount)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (auto iter = illegalDescJsPandaFiles_.begin(); iter != illegalDescJsPandaFiles_.end();) {
        JsPandaFileInfo *jsPandaFile = *iter;
        if (jsPandaFile->pf_ == pf) {
            jsPandaFile->DecreaseRefCount(decRefCount);
            if (!jsPandaFile->IsUsing()) {
                ReleaseJsPandaFileInfo(jsPandaFile);
                illegalDescJsPandaFiles_.erase(iter);
            }
            return;
        }
    }
    for (auto iter = loadedJsPandaFiles_.begin(); iter != loadedJsPandaFiles_.end(); iter++) {
        JsPandaFileInfo *jsPandaFile = iter->second;
        if (jsPandaFile->pf_ == pf) {
            jsPandaFile->DecreaseRefCount(decRefCount);
            if (!jsPandaFile->IsUsing()) {
                ReleaseJsPandaFileInfo(jsPandaFile);
                loadedJsPandaFiles_.erase(iter);
            }
            return;
        }
    }
}

void JsPandaFileManager::ReleaseJsPandaFileInfo(JsPandaFileInfo *jsPandaFileInfo)
{
    if (jsPandaFileInfo == nullptr) {
        return;
    }
    delete jsPandaFileInfo;
}

tooling::ecmascript::PtJSExtractor *JsPandaFileManager::GetOrCreatePtJSExtractor(const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    std::string descriptor = pf->GetFilename();
    JsPandaFileInfo *existJsPandaFile = FindJsPandaFile(descriptor, pf);
    if (existJsPandaFile == nullptr) {
        LOG_ECMA(FATAL) << "can not get PtJSExtrjsPandaFile from unknown jsPandaFile";
        return nullptr;
    }
    return existJsPandaFile->GetOrCreatePtJSExtractor();
}

tooling::ecmascript::PtJSExtractor *JsPandaFileInfo::GetOrCreatePtJSExtractor()
{
    if (ptJSExtractor_) {
        return ptJSExtractor_.get();
    }
    ptJSExtractor_ = std::make_unique<tooling::ecmascript::PtJSExtractor>(pf_);
    return ptJSExtractor_.get();
}
}  // namespace panda::ecmascript
