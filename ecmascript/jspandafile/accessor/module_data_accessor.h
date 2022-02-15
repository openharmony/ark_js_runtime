/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MODULE_DATA_ACCESSOR_H_
#define ECMASCRIPT_MODULE_DATA_ACCESSOR_H_

#include "field_data_accessor.h"
#include "file.h"
#include "utils/span.h"

namespace panda::ecmascript::jspandafile {
using StringData = panda_file::File::StringData;

enum class ModuleTag : uint8_t {
    REGULARIMPORT = 0x00,
    NAMESPACEIMPORT = 0x01,
    LOCALEXPORT = 0x02,
    INDIRECTEXPORT = 0x03,
    STAREXPORT = 0x04,
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
    void EnumerateModuleRecord(const Callback &cb);

    const panda_file::File &GetPandaFile() const
    {
        return pandaFile_;
    }

    panda_file::File::EntityId GetModuleDataId() const
    {
        return moduleDataId_;
    }

    std::vector<uint32_t>& getRequestModules()
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

#endif  // ECMASCRIPT_MODULE_DATA_ACCESSOR_H_
