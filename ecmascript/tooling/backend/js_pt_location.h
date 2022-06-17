/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TOOLING_BACKEND_JS_PT_LOCATION_H
#define ECMASCRIPT_TOOLING_BACKEND_JS_PT_LOCATION_H

#include <cstring>

#include "libpandafile/file.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
class JSPtLocation {
public:
    using EntityId = panda_file::File::EntityId;

    explicit JSPtLocation(const char *pandaFile, EntityId methodId, uint32_t bytecodeOffset)
        : pandaFile_(pandaFile), methodId_(methodId), bytecodeOffset_(bytecodeOffset)
    {
    }

    const char *GetPandaFile() const
    {
        return pandaFile_;
    }

    EntityId GetMethodId() const
    {
        return methodId_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bytecodeOffset_;
    }

    bool operator==(const JSPtLocation &location) const
    {
        return methodId_ == location.methodId_ && bytecodeOffset_ == location.bytecodeOffset_ &&
               ::strcmp(pandaFile_, location.pandaFile_) == 0;
    }

    ~JSPtLocation() = default;

    DEFAULT_COPY_SEMANTIC(JSPtLocation);
    DEFAULT_MOVE_SEMANTIC(JSPtLocation);

private:
    const char *pandaFile_;
    EntityId methodId_;
    uint32_t bytecodeOffset_ {0};
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_BACKEND_JS_PT_LOCATION_H
