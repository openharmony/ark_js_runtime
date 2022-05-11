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
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
JSPandaFile::JSPandaFile(const panda_file::File *pf, const CString &descriptor) : pf_(pf), desc_(descriptor)
{
    ASSERT(pf_ != nullptr);
    Initialize();
}

JSPandaFile::~JSPandaFile()
{
    if (pf_ != nullptr) {
        delete pf_;
        pf_ = nullptr;
    }
    methodMap_.clear();
    if (methods_ != nullptr) {
        JSPandaFileManager::FreeBuffer(methods_);
        methods_ = nullptr;
    }
}

uint32_t JSPandaFile::GetOrInsertConstantPool(ConstPoolType type, uint32_t offset)
{
    auto it = constpoolMap_.find(offset);
    if (it != constpoolMap_.cend()) {
        ConstPoolValue value(it->second);
        return value.GetConstpoolIndex();
    }
    ASSERT(constpoolIndex_ != UINT32_MAX);
    uint32_t index = constpoolIndex_++;
    ConstPoolValue value(type, index);
    constpoolMap_.insert({offset, value.GetValue()});
    return index;
}

uint32_t JSPandaFile::GetIdInConstantPool(uint32_t offset) const
{
    auto it = constpoolMap_.find(offset);
    if (it != constpoolMap_.cend()) {
        ConstPoolValue value(it->second);
        return value.GetConstpoolIndex();
    }
    return NOT_FOUND_IDX;
}

void JSPandaFile::Initialize()
{
    Span<const uint32_t> classIndexes = pf_->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf_->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf_, classId);
        numMethods_ += cda.GetMethodsNumber();
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (!isModule_ && std::strcmp(MODULE_CLASS, desc) == 0) {
            isModule_ = true;
        }

        if (!HasTSTypes() && std::strcmp(TS_TYPES_CLASS, desc) == 0) {
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &fieldAccessor) -> void {
                panda_file::File::EntityId fieldNameId = fieldAccessor.GetNameId();
                panda_file::File::StringData sd = pf_->GetStringData(fieldNameId);
                const char *fieldName = utf::Mutf8AsCString(sd.data);
                if (std::strcmp(TYPE_FLAG, fieldName) == 0) {
                    hasTSTypes_ = fieldAccessor.GetValue<uint8_t>().value() != 0;
                }
                if (std::strcmp(TYPE_SUMMARY_INDEX, fieldName) == 0) {
                    typeSummaryIndex_ = fieldAccessor.GetValue<uint32_t>().value();
                }
            });
        }
    }
    methods_ = static_cast<JSMethod *>(JSPandaFileManager::AllocateBuffer(sizeof(JSMethod) * numMethods_));
}

JSMethod *JSPandaFile::FindMethods(uint32_t offset) const
{
    return methodMap_.at(offset);
}

bool JSPandaFile::IsModule() const
{
    return isModule_;
}
}  // namespace panda::ecmascript
