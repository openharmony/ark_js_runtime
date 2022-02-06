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

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/class_linker/program_object-inl.h"

namespace panda::ecmascript {
JSPandaFile::JSPandaFile(const panda_file::File *pf) : pf_(pf)
{
}

JSPandaFile::JSPandaFile(const panda_file::File *pf, bool isFrameWork) : pf_(pf), isFrameWorker_(isFrameWork)
{
}

JSPandaFile::~JSPandaFile()
{
    if (pf_ != nullptr) {
        delete pf_;
        pf_ = nullptr;
    }
    if (methods_ != nullptr) {
        JSPandaFileManager::FreeBuffer(methods_);
        methods_ = nullptr;
    }
}

JSPandaFile *JSPandaFile::CreateJSPandaFileFromPf(const std::string &filename)
{
    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    JSPandaFile *jsPandaFile = new JSPandaFile(pf.release());
    jsPandaFile->desc_ = filename;
    return jsPandaFile;
}

JSPandaFile *JSPandaFile::CreateJSPandaFileFromBuffer(const void *buffer, size_t size)
{
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        return nullptr;
    }
    JSPandaFile *jsPandaFile = new JSPandaFile(pf.release());
    return jsPandaFile;
}

JSPandaFile *JSPandaFile::CreateJSPandaFileFromBuffer(const void *buffer, size_t size, const std::string &filename)
{
    JSPandaFile *jsPandaFile = CreateJSPandaFileFromBuffer(buffer, size);
    jsPandaFile->desc_ = filename;
    return jsPandaFile;
}

tooling::ecmascript::PtJSExtractor *JSPandaFile::GetOrCreatePtJSExtractor()
{
    if (ptJSExtractor_) {
        return ptJSExtractor_.get();
    }
    ptJSExtractor_ = std::make_unique<tooling::ecmascript::PtJSExtractor>(pf_);
    return ptJSExtractor_.get();
}

void JSPandaFile::SaveTranslatedInfo(uint32_t constpoolIndex, uint32_t numMethods, uint32_t mainMethodIndex,
                                     JSMethod *methods, std::unordered_map<uint32_t, uint64_t> &constpoolMap)
{
    constpoolIndex_ = constpoolIndex;
    numMethods_ = numMethods;
    mainMethodIndex_ = mainMethodIndex;
    methods_ = methods;
    constpoolMap_ = constpoolMap;
}

void JSPandaFile::ParseMethods()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
    }
    methods_ = static_cast<JSMethod *>(JSPandaFileManager::AllocateBuffer(sizeof(JSMethod) * numMethods_));
}

const JSMethod *JSPandaFile::FindMethods(uint32_t offset) const
{
    Span<JSMethod> methods = GetMethodSpan();
    auto pred = [offset](const JSMethod &method) { return method.GetFileId().GetOffset() == offset; };
    auto it = std::find_if(methods.begin(), methods.end(), pred);
    if (it != methods.end()) {
        return &*it;
    }
    return nullptr;
}
}  // namespace panda::ecmascript
