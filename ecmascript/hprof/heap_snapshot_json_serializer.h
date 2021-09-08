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

#ifndef ECMASCRIPT_HPROF_HEAP_SNAPSHOT_SERIALIZER_H
#define ECMASCRIPT_HPROF_HEAP_SNAPSHOT_SERIALIZER_H

#include <fstream>
#include <sstream>

#include "ecmascript/mem/c_string.h"
#include "os/mem.h"

namespace panda::ecmascript {
using fstream = std::fstream;
using stringstream = std::stringstream;

class HeapSnapShot;

class HeapSnapShotJSONSerializer {
public:
    explicit HeapSnapShotJSONSerializer() = default;
    ~HeapSnapShotJSONSerializer() = default;
    NO_MOVE_SEMANTIC(HeapSnapShotJSONSerializer);
    NO_COPY_SEMANTIC(HeapSnapShotJSONSerializer);
    bool Serialize(HeapSnapShot *snapShot, const CString &fileName);

private:
    void SerializeSnapShotHeader();
    void SerializeNodes();
    void SerializeEdges();
    void SerializeTraceFunctionInfo();
    void SerializeTraceTree();
    void SerializeSamples();
    void SerializeLocations();
    void SerializeStringTable();
    void SerializerSnapShotClosure();

    void WriteJSON(const CString &fileName);
    fstream outputStream_;
    HeapSnapShot *snapShot_{nullptr};
    stringstream stringBuffer_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_SNAPSHOT_SERIALIZER_H
