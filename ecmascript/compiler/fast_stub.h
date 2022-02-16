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
#include "ecmascript/compiler/stub-inl.h"

namespace panda::ecmascript::kungfu {
#ifndef NDEBUG
class PhiGateTestStub : public Stub {
public:
    explicit PhiGateTestStub(Circuit *circuit) : Stub("Phi", 1, circuit) {}
    ~PhiGateTestStub() = default;
    NO_MOVE_SEMANTIC(PhiGateTestStub);
    NO_COPY_SEMANTIC(PhiGateTestStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class LoopTestStub : public Stub {
public:
    explicit LoopTestStub(Circuit *circuit) : Stub("LoopTest", 1, circuit) {}
    ~LoopTestStub() = default;
    NO_MOVE_SEMANTIC(LoopTestStub);
    NO_COPY_SEMANTIC(LoopTestStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class LoopTest1Stub : public Stub {
public:
    explicit LoopTest1Stub(Circuit *circuit) : Stub("LoopTest1", 1, circuit) {}
    ~LoopTest1Stub() = default;
    NO_MOVE_SEMANTIC(LoopTest1Stub);
    NO_COPY_SEMANTIC(LoopTest1Stub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastMulGCTestStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastMulGCTestStub(Circuit *circuit) : Stub("FastMulGCTest", 3, circuit) {}
    ~FastMulGCTestStub() = default;
    NO_MOVE_SEMANTIC(FastMulGCTestStub);
    NO_COPY_SEMANTIC(FastMulGCTestStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};
#endif

class FastAddStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastAddStub(Circuit *circuit) : Stub("FastAdd", 2, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_FRAME);
    }
    ~FastAddStub() = default;
    NO_MOVE_SEMANTIC(FastAddStub);
    NO_COPY_SEMANTIC(FastAddStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastSubStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastSubStub(Circuit *circuit) : Stub("FastSub", 2, circuit) {}
    ~FastSubStub() = default;
    NO_MOVE_SEMANTIC(FastSubStub);
    NO_COPY_SEMANTIC(FastSubStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastMulStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastMulStub(Circuit *circuit) : Stub("FastMul", 2, circuit) {}
    ~FastMulStub() = default;
    NO_MOVE_SEMANTIC(FastMulStub);
    NO_COPY_SEMANTIC(FastMulStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastDivStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastDivStub(Circuit *circuit) : Stub("FastDiv", 2, circuit) {}
    ~FastDivStub() = default;
    NO_MOVE_SEMANTIC(FastDivStub);
    NO_COPY_SEMANTIC(FastDivStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class GetPropertyByIndexStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByIndexStub(Circuit *circuit) : Stub("GetPropertyByIndex", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByIndexStub);
    NO_COPY_SEMANTIC(GetPropertyByIndexStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SetPropertyByIndexStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByIndexStub(Circuit *circuit) : Stub("SetPropertyByIndex", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByIndexStub);
    NO_COPY_SEMANTIC(SetPropertyByIndexStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class GetPropertyByNameStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByNameStub(Circuit *circuit) : Stub("GetPropertyByName", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByNameStub);
    NO_COPY_SEMANTIC(GetPropertyByNameStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SetPropertyByNameStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByNameStub(Circuit *circuit) : Stub("SetPropertyByName", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByNameStub);
    NO_COPY_SEMANTIC(SetPropertyByNameStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SetPropertyByNameWithOwnStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByNameWithOwnStub(Circuit *circuit) : Stub("SetPropertyByNameWithOwn", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByNameWithOwnStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByNameWithOwnStub);
    NO_COPY_SEMANTIC(SetPropertyByNameWithOwnStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastModStub : public Stub {
public:
    // 3 means argument counts
    explicit FastModStub(Circuit *circuit) : Stub("FastMod", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~FastModStub() = default;
    NO_MOVE_SEMANTIC(FastModStub);
    NO_COPY_SEMANTIC(FastModStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastTypeOfStub : public Stub {
public:
    // 2 means argument counts
    explicit FastTypeOfStub(Circuit *circuit) : Stub("FastTypeOf", 2, circuit) {}
    ~FastTypeOfStub() = default;
    NO_MOVE_SEMANTIC(FastTypeOfStub);
    NO_COPY_SEMANTIC(FastTypeOfStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FunctionCallInternalStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit FunctionCallInternalStub(Circuit *circuit) : Stub("FunctionCallInternal", 5, circuit) {}
    ~FunctionCallInternalStub() = default;
    NO_MOVE_SEMANTIC(FunctionCallInternalStub);
    NO_COPY_SEMANTIC(FunctionCallInternalStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class GetPropertyByValueStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByValueStub(Circuit *circuit) : Stub("FastGetPropertyByValue", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByValueStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByValueStub);
    NO_COPY_SEMANTIC(GetPropertyByValueStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SetPropertyByValueStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByValueStub(Circuit *circuit) : Stub("FastSetPropertyByValue", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByValueStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByValueStub);
    NO_COPY_SEMANTIC(SetPropertyByValueStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FastEqualStub : public Stub {
public:
    // 2 means argument counts
    explicit FastEqualStub(Circuit *circuit) : Stub("FastEqual", 2, circuit) {}
    ~FastEqualStub() = default;
    NO_MOVE_SEMANTIC(FastEqualStub);
    NO_COPY_SEMANTIC(FastEqualStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TryLoadICByNameStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit TryLoadICByNameStub(Circuit *circuit) : Stub("TryLoadICByName", 4, circuit) {}
    ~TryLoadICByNameStub() = default;
    NO_MOVE_SEMANTIC(TryLoadICByNameStub);
    NO_COPY_SEMANTIC(TryLoadICByNameStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TryLoadICByValueStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit TryLoadICByValueStub(Circuit *circuit) : Stub("TryLoadICByValue", 5, circuit) {}
    ~TryLoadICByValueStub() = default;
    NO_MOVE_SEMANTIC(TryLoadICByValueStub);
    NO_COPY_SEMANTIC(TryLoadICByValueStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TryStoreICByNameStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit TryStoreICByNameStub(Circuit *circuit) : Stub("TryStoreICByName", 5, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~TryStoreICByNameStub() = default;
    NO_MOVE_SEMANTIC(TryStoreICByNameStub);
    NO_COPY_SEMANTIC(TryStoreICByNameStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TryStoreICByValueStub : public Stub {
public:
    // 6 : 6 means argument counts
    explicit TryStoreICByValueStub(Circuit *circuit) : Stub("TryStoreICByValue", 6, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~TryStoreICByValueStub() = default;
    NO_MOVE_SEMANTIC(TryStoreICByValueStub);
    NO_COPY_SEMANTIC(TryStoreICByValueStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TestAbsoluteAddressRelocationStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit TestAbsoluteAddressRelocationStub(Circuit *circuit) : Stub("TestAbsoluteAddressRelocation", 2, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~TestAbsoluteAddressRelocationStub() = default;
    NO_MOVE_SEMANTIC(TestAbsoluteAddressRelocationStub);
    NO_COPY_SEMANTIC(TestAbsoluteAddressRelocationStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_FASTPATH_STUB_H
