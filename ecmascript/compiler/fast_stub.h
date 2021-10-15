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

class FindOwnElementStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FindOwnElementStub(Circuit *circuit) : Stub("FindOwnElement", 3, circuit) {}
    ~FindOwnElementStub() = default;
    NO_MOVE_SEMANTIC(FindOwnElementStub);
    NO_COPY_SEMANTIC(FindOwnElementStub);
    void GenerateCircuit() override;
};

class GetElementStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetElementStub(Circuit *circuit) : Stub("GetElement", 3, circuit) {}
    ~GetElementStub() = default;
    NO_MOVE_SEMANTIC(GetElementStub);
    NO_COPY_SEMANTIC(GetElementStub);
    void GenerateCircuit() override;
};

class FindOwnElement2Stub : public Stub {
public:
    // 6 : 6 means argument counts
    explicit FindOwnElement2Stub(Circuit *circuit) : Stub("FindOwnElement2", 6, circuit) {}
    ~FindOwnElement2Stub() = default;
    NO_MOVE_SEMANTIC(FindOwnElement2Stub);
    NO_COPY_SEMANTIC(FindOwnElement2Stub);
    void GenerateCircuit() override;
};

class SetElementStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit SetElementStub(Circuit *circuit) : Stub("SetElement", 5, circuit) {}
    ~SetElementStub() = default;
    NO_MOVE_SEMANTIC(SetElementStub);
    NO_COPY_SEMANTIC(SetElementStub);
    void GenerateCircuit() override;
};

class GetPropertyByIndexStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByIndexStub(Circuit *circuit) : Stub("GetPropertyByIndex", 3, circuit) {}
    ~GetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByIndexStub);
    NO_COPY_SEMANTIC(GetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class SetPropertyByIndexStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByIndexStub(Circuit *circuit) : Stub("SetPropertyByIndex", 4, circuit) {}
    ~SetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByIndexStub);
    NO_COPY_SEMANTIC(SetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class GetPropertyByNameStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByNameStub(Circuit *circuit) : Stub("GetPropertyByName", 3, circuit) {}
    ~GetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByNameStub);
    NO_COPY_SEMANTIC(GetPropertyByNameStub);
    void GenerateCircuit() override;
};

class FastModStub : public Stub {
public:
    // 2 means argument counts
    explicit FastModStub(Circuit *circuit) : Stub("FastMod", 2, circuit) {}
    ~FastModStub() = default;
    NO_MOVE_SEMANTIC(FastModStub);
    NO_COPY_SEMANTIC(FastModStub);
    void GenerateCircuit() override;
};

class FastTypeOfStub : public Stub {
public:
    // 2 means argument counts
    explicit FastTypeOfStub(Circuit *circuit) : Stub("FastTypeOf", 2, circuit) {}
    ~FastTypeOfStub() = default;
    NO_MOVE_SEMANTIC(FastTypeOfStub);
    NO_COPY_SEMANTIC(FastTypeOfStub);
    void GenerateCircuit() override;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_FASTPATH_STUB_H