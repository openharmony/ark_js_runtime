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

#ifndef ECMASCRIPT_TOOLING_INTERFACE_FILE_STREAM_H
#define ECMASCRIPT_TOOLING_INTERFACE_FILE_STREAM_H

#include <fstream>

#include "ecmascript/tooling/interface/stream.h"

namespace panda::ecmascript {
class FileStream : public Stream {
public:
    FileStream(const std::string &fileName);
    ~FileStream() override;

    void EndOfStream() override;

    // Get chunk's size
    int GetSize() override
    {
        const static int fileChunkSize = 10240;
        return fileChunkSize;
    }

    // Writes the chunk of data into the stream
    bool WriteChunk(char* data, int size) override;

private:
    void Initialize(const std::string &fileName);
    std::pair<bool, std::string> FilePathValid(const std::string &fileName);

    std::fstream fileStream_;
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_INTERFACE_FILE_STREAM_H
