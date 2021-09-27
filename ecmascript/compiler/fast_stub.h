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

#ifndef ECMASCRIPT_COMPILER_FASTPATH_STUB_H
#define ECMASCRIPT_COMPILER_FASTPATH_STUB_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/stub.h"

namespace kungfu {
class FastArrayLoadElementStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastArrayLoadElementStub(Circuit *circuit) : Stub("FastArrayLoadElement", 2, circuit) {}
    ~FastArrayLoadElementStub() = default;
    NO_MOVE_SEMANTIC(FastArrayLoadElementStub);
    NO_COPY_SEMANTIC(FastArrayLoadElementStub);
    void GenerateCircuit() override;
};

class FastAddStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastAddStub(Circuit *circuit) : Stub("FastAdd", 2, circuit) {}
    ~FastAddStub() = default;
    NO_MOVE_SEMANTIC(FastAddStub);
    NO_COPY_SEMANTIC(FastAddStub);
    void GenerateCircuit() override;
};

class FastSubStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastSubStub(Circuit *circuit) : Stub("FastSub", 2, circuit) {}
    ~FastSubStub() = default;
    NO_MOVE_SEMANTIC(FastSubStub);
    NO_COPY_SEMANTIC(FastSubStub);
    void GenerateCircuit() override;
};

class FastMulStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastMulStub(Circuit *circuit) : Stub("FastMul", 2, circuit) {}
    ~FastMulStub() = default;
    NO_MOVE_SEMANTIC(FastMulStub);
    NO_COPY_SEMANTIC(FastMulStub);
    void GenerateCircuit() override;
};

class FastDivStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastDivStub(Circuit *circuit) : Stub("FastDiv", 2, circuit) {}
    ~FastDivStub() = default;
    NO_MOVE_SEMANTIC(FastDivStub);
    NO_COPY_SEMANTIC(FastDivStub);
    void GenerateCircuit() override;
};

class FastFindOwnElementStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastFindOwnElementStub(Circuit *circuit) : Stub("FastFindOwnElement", 3, circuit) {}
    ~FastFindOwnElementStub() = default;
    NO_MOVE_SEMANTIC(FastFindOwnElementStub);
    NO_COPY_SEMANTIC(FastFindOwnElementStub);
    void GenerateCircuit() override;
};

class FastGetElementStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastGetElementStub(Circuit *circuit) : Stub("FastGetElement", 3, circuit) {}
    ~FastGetElementStub() = default;
    NO_MOVE_SEMANTIC(FastGetElementStub);
    NO_COPY_SEMANTIC(FastGetElementStub);
    void GenerateCircuit() override;
};

class FastFindOwnElement2Stub : public Stub {
public:
    // 6 : 6 means argument counts
    explicit FastFindOwnElement2Stub(Circuit *circuit) : Stub("FastFindOwnElement2", 6, circuit) {}
    ~FastFindOwnElement2Stub() = default;
    NO_MOVE_SEMANTIC(FastFindOwnElement2Stub);
    NO_COPY_SEMANTIC(FastFindOwnElement2Stub);
    void GenerateCircuit() override;
};

class FastSetElementStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit FastSetElementStub(Circuit *circuit) : Stub("FastSetElement", 5, circuit) {}
    ~FastSetElementStub() = default;
    NO_MOVE_SEMANTIC(FastSetElementStub);
    NO_COPY_SEMANTIC(FastSetElementStub);
    void GenerateCircuit() override;
};

class FastGetPropertyByIndexStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastGetPropertyByIndexStub(Circuit *circuit) : Stub("FastGetPropertyByIndex", 3, circuit) {}
    ~FastGetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(FastGetPropertyByIndexStub);
    NO_COPY_SEMANTIC(FastGetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class FastSetPropertyByIndexStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit FastSetPropertyByIndexStub(Circuit *circuit) : Stub("FastSetPropertyByIndex", 4, circuit) {}
    ~FastSetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(FastSetPropertyByIndexStub);
    NO_COPY_SEMANTIC(FastSetPropertyByIndexStub);
    void GenerateCircuit() override;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_FASTPATH_STUB_H