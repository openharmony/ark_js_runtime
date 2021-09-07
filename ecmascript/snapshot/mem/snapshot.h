/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H

#include "libpandabase/macros.h"
#include "libpandafile/file.h"

#include "ecmascript/snapshot/mem/slot_bit.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
class Program;
class EcmaVM;

class SnapShot final {
public:
    explicit SnapShot(EcmaVM *vm) : vm_(vm) {}
    ~SnapShot() = default;

    void MakeSnapShotProgramObject(Program *program, const panda_file::File *pf,
                                   const CString &fileName = "./snapshot");
    std::unique_ptr<const panda_file::File> DeserializeGlobalEnvAndProgram(
        const CString &fileName = "./snapshot");

private:
    struct Header {
        uint32_t snapshot_size;
        uint32_t panda_file_begin;
    };

private:
    size_t AlignUpPageSize(size_t spaceSize);
    std::pair<bool, CString> VerifyFilePath(const CString &filePath);

    NO_MOVE_SEMANTIC(SnapShot);
    NO_COPY_SEMANTIC(SnapShot);

    EcmaVM *vm_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H