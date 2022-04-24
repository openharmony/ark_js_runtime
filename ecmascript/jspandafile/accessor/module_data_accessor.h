/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_ACCESSOR_MODULE_DATA_ACCESSOR_H
#define ECMASCRIPT_JSPANDAFILE_ACCESSOR_MODULE_DATA_ACCESSOR_H

#include "libpandafile/field_data_accessor.h"
#include "libpandafile/file.h"
#include "libpandafile/helpers.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript::jspandafile {
using StringData = panda_file::File::StringData;

enum class ModuleTag : uint8_t {
    REGULAR_IMPORT = 0x00,
    NAMESPACE_IMPORT = 0x01,
    LOCAL_EXPORT = 0x02,
    INDIRECT_EXPORT = 0x03,
    STAR_EXPORT = 0x04,
};

class ModuleDataAccessor {
public:
    ModuleDataAccessor(const panda_file::File &panda_file, panda_file::File::EntityId module_data_id);
    ~ModuleDataAccessor() = default;
    DEFAULT_MOVE_CTOR(ModuleDataAccessor)
    DEFAULT_COPY_CTOR(ModuleDataAccessor)
    NO_MOVE_OPERATOR(ModuleDataAccessor);
    NO_COPY_OPERATOR(ModuleDataAccessor);

    template <class Callback>
    inline void EnumerateModuleRecord(const Callback &cb)
    {
        auto sp = entryDataSp_;

        auto regularImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
        for (size_t idx = 0; idx < regularImportNum; idx++) {
            auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
            cb(ModuleTag::REGULAR_IMPORT, 0, moduleRequestIdx, importNameOffset, localNameOffset);
        }

        auto namespaceImportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
        for (size_t idx = 0; idx < namespaceImportNum; idx++) {
            auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
            cb(ModuleTag::NAMESPACE_IMPORT, 0, moduleRequestIdx, 0, localNameOffset);
        }

        auto localExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
        for (size_t idx = 0; idx < localExportNum; idx++) {
            auto localNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            cb(ModuleTag::LOCAL_EXPORT, exportNameOffset, 0, 0, localNameOffset);
        }

        auto indirectExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
        for (size_t idx = 0; idx < indirectExportNum; idx++) {
            auto exportNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto importNameOffset = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&sp));
            auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
            cb(ModuleTag::INDIRECT_EXPORT, exportNameOffset, moduleRequestIdx, importNameOffset, 0);
        }

        auto starExportNum = panda_file::helpers::Read<panda_file::ID_SIZE>(&sp);
        for (size_t idx = 0; idx < starExportNum; idx++) {
            auto moduleRequestIdx = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint16_t)>(&sp));
            cb(ModuleTag::STAR_EXPORT, 0, moduleRequestIdx, 0, 0);
        }
    }

    const panda_file::File &GetPandaFile() const
    {
        return pandaFile_;
    }

    panda_file::File::EntityId GetModuleDataId() const
    {
        return moduleDataId_;
    }

    const std::vector<uint32_t>& getRequestModules() const
    {
        return moduleRequests_;
    }

    using ModuleValue = std::variant<uint32_t, StringData>;

private:
    const panda_file::File &pandaFile_;
    panda_file::File::EntityId moduleDataId_;
    uint32_t numModuleRequests_;
    std::vector<uint32_t> moduleRequests_;
    Span<const uint8_t> entryDataSp_ {nullptr, nullptr};
};
}  // namespace panda::ecmascript::jspandafile
#endif  // ECMASCRIPT_JSPANDAFILE_ACCESSOR_MODULE_DATA_ACCESSOR_H
