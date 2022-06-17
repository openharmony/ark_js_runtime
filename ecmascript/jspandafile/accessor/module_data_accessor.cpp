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

#include "ecmascript/jspandafile/accessor/module_data_accessor.h"
#include "libpandafile/helpers.h"

namespace panda::ecmascript::jspandafile {
ModuleDataAccessor::ModuleDataAccessor(const panda_file::File &pandaFile, panda_file::File::EntityId moduleDataId)
    : pandaFile_(pandaFile), moduleDataId_(moduleDataId)
{
    auto sp = pandaFile_.GetSpanFromId(moduleDataId);

    auto moduleSp = sp.SubSpan(panda_file::ID_SIZE); // skip literalnum

    numModuleRequests_ = panda_file::helpers::Read<panda_file::ID_SIZE>(&moduleSp);

    for (size_t idx = 0; idx < numModuleRequests_; idx++) {
        auto value = static_cast<uint32_t>(panda_file::helpers::Read<sizeof(uint32_t)>(&moduleSp));
        moduleRequests_.emplace_back(value);
    }

    entryDataSp_ = moduleSp;
}
}  // namespace panda::ecmascript::jspandafile
