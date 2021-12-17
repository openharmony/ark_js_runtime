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

namespace kungfu {
#ifdef ECMASCRIPT_ENABLE_SPECIFIC_STUBS
class FastAddStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastAddStub(Circuit *circuit, const char* triple) : Stub("FastAdd", 2, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_FRAME);
    }
    ~FastAddStub() = default;
    NO_MOVE_SEMANTIC(FastAddStub);
    NO_COPY_SEMANTIC(FastAddStub);
    void GenerateCircuit() override;
};

class FastMulGCTestStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastMulGCTestStub(Circuit *circuit, const char* triple) : Stub("FastMulGCTest", 3, circuit, triple) {}
    ~FastMulGCTestStub() = default;
    NO_MOVE_SEMANTIC(FastMulGCTestStub);
    NO_COPY_SEMANTIC(FastMulGCTestStub);
    void GenerateCircuit() override;
};
class FastSubStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastSubStub(Circuit *circuit, const char* triple) : Stub("FastSub", 2, circuit, triple) {}
    ~FastSubStub() = default;
    NO_MOVE_SEMANTIC(FastSubStub);
    NO_COPY_SEMANTIC(FastSubStub);
    void GenerateCircuit() override;
};

class FastMulStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastMulStub(Circuit *circuit, const char* triple) : Stub("FastMul", 2, circuit, triple) {}
    ~FastMulStub() = default;
    NO_MOVE_SEMANTIC(FastMulStub);
    NO_COPY_SEMANTIC(FastMulStub);
    void GenerateCircuit() override;
};

class FastDivStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastDivStub(Circuit *circuit, const char* triple) : Stub("FastDiv", 2, circuit, triple) {}
    ~FastDivStub() = default;
    NO_MOVE_SEMANTIC(FastDivStub);
    NO_COPY_SEMANTIC(FastDivStub);
    void GenerateCircuit() override;
};

class FastModStub : public Stub {
public:
    // 3 means argument counts
    explicit FastModStub(Circuit *circuit, const char* triple) : Stub("FastMod", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~FastModStub() = default;
    NO_MOVE_SEMANTIC(FastModStub);
    NO_COPY_SEMANTIC(FastModStub);
    void GenerateCircuit() override;
};

class FastTypeOfStub : public Stub {
public:
    // 2 means argument counts
    explicit FastTypeOfStub(Circuit *circuit, const char* triple) : Stub("FastTypeOf", 2, circuit, triple) {}
    ~FastTypeOfStub() = default;
    NO_MOVE_SEMANTIC(FastTypeOfStub);
    NO_COPY_SEMANTIC(FastTypeOfStub);
    void GenerateCircuit() override;
};

class FastEqualStub : public Stub {
public:
    // 2 means argument counts
    explicit FastEqualStub(Circuit *circuit, const char* triple) : Stub("FastEqual", 2, circuit, triple) {}
    ~FastEqualStub() = default;
    NO_MOVE_SEMANTIC(FastEqualStub);
    NO_COPY_SEMANTIC(FastEqualStub);
    void GenerateCircuit() override;
};

class GetPropertyByIndexStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByIndexStub(Circuit *circuit, const char* triple)
        : Stub("GetPropertyByIndex", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByIndexStub);
    NO_COPY_SEMANTIC(GetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class SetPropertyByIndexStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByIndexStub(Circuit *circuit, const char* triple)
        : Stub("SetPropertyByIndex", 4, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByIndexStub);
    NO_COPY_SEMANTIC(SetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class GetPropertyByNameStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByNameStub(Circuit *circuit, const char* triple)
        : Stub("GetPropertyByName", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByNameStub);
    NO_COPY_SEMANTIC(GetPropertyByNameStub);
    void GenerateCircuit() override;
};
#else
class FastAddStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastAddStub(Circuit *circuit, const char* triple) : Stub("FastAdd", 2, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_FRAME);
    }
    ~FastAddStub() = default;
    NO_MOVE_SEMANTIC(FastAddStub);
    NO_COPY_SEMANTIC(FastAddStub);
    void GenerateCircuit() override;
};

class FastSubStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastSubStub(Circuit *circuit, const char* triple) : Stub("FastSub", 2, circuit, triple) {}
    ~FastSubStub() = default;
    NO_MOVE_SEMANTIC(FastSubStub);
    NO_COPY_SEMANTIC(FastSubStub);
    void GenerateCircuit() override;
};

class FastMulStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastMulStub(Circuit *circuit, const char* triple) : Stub("FastMul", 2, circuit, triple) {}
    ~FastMulStub() = default;
    NO_MOVE_SEMANTIC(FastMulStub);
    NO_COPY_SEMANTIC(FastMulStub);
    void GenerateCircuit() override;
};

class FastMulGCTestStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit FastMulGCTestStub(Circuit *circuit, const char* triple) : Stub("FastMulGCTest", 3, circuit, triple) {}
    ~FastMulGCTestStub() = default;
    NO_MOVE_SEMANTIC(FastMulGCTestStub);
    NO_COPY_SEMANTIC(FastMulGCTestStub);
    void GenerateCircuit() override;
};

class FastDivStub : public Stub {
public:
    // 2 : 2 means argument counts
    explicit FastDivStub(Circuit *circuit, const char* triple) : Stub("FastDiv", 2, circuit, triple) {}
    ~FastDivStub() = default;
    NO_MOVE_SEMANTIC(FastDivStub);
    NO_COPY_SEMANTIC(FastDivStub);
    void GenerateCircuit() override;
};

class FindOwnElement2Stub : public Stub {
public:
    // 6 : 6 means argument counts
    explicit FindOwnElement2Stub(Circuit *circuit, const char* triple) : Stub("FindOwnElement2", 6, circuit, triple) {}
    ~FindOwnElement2Stub() = default;
    NO_MOVE_SEMANTIC(FindOwnElement2Stub);
    NO_COPY_SEMANTIC(FindOwnElement2Stub);
    void GenerateCircuit() override;
};

class GetPropertyByIndexStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByIndexStub(Circuit *circuit, const char* triple)
        : Stub("GetPropertyByIndex", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByIndexStub);
    NO_COPY_SEMANTIC(GetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class SetPropertyByIndexStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByIndexStub(Circuit *circuit, const char* triple)
        : Stub("SetPropertyByIndex", 4, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByIndexStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByIndexStub);
    NO_COPY_SEMANTIC(SetPropertyByIndexStub);
    void GenerateCircuit() override;
};

class GetPropertyByNameStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByNameStub(Circuit *circuit, const char* triple)
        : Stub("GetPropertyByName", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByNameStub);
    NO_COPY_SEMANTIC(GetPropertyByNameStub);
    void GenerateCircuit() override;
};

class SetPropertyByNameStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByNameStub(Circuit *circuit, const char* triple)
        : Stub("SetPropertyByName", 4, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByNameStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByNameStub);
    NO_COPY_SEMANTIC(SetPropertyByNameStub);
    void GenerateCircuit() override;
};

class SetPropertyByNameWithOwnStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByNameWithOwnStub(Circuit *circuit, const char* triple)
        : Stub("SetPropertyByNameWithOwn", 4, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByNameWithOwnStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByNameWithOwnStub);
    NO_COPY_SEMANTIC(SetPropertyByNameWithOwnStub);
    void GenerateCircuit() override;
};

class FastModStub : public Stub {
public:
    // 3 means argument counts
    explicit FastModStub(Circuit *circuit, const char* triple) : Stub("FastMod", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~FastModStub() = default;
    NO_MOVE_SEMANTIC(FastModStub);
    NO_COPY_SEMANTIC(FastModStub);
    void GenerateCircuit() override;
};

class FastTypeOfStub : public Stub {
public:
    // 2 means argument counts
    explicit FastTypeOfStub(Circuit *circuit, const char* triple) : Stub("FastTypeOf", 2, circuit, triple) {}
    ~FastTypeOfStub() = default;
    NO_MOVE_SEMANTIC(FastTypeOfStub);
    NO_COPY_SEMANTIC(FastTypeOfStub);
    void GenerateCircuit() override;
};

class FunctionCallInternalStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit FunctionCallInternalStub(Circuit *circuit, const char* triple)
        : Stub("FunctionCallInternal", 5, circuit, triple) {}
    ~FunctionCallInternalStub() = default;
    NO_MOVE_SEMANTIC(FunctionCallInternalStub);
    NO_COPY_SEMANTIC(FunctionCallInternalStub);
    void GenerateCircuit() override;
};

class GetPropertyByValueStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit GetPropertyByValueStub(Circuit *circuit, const char* triple)
        : Stub("FastGetPropertyByValue", 3, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~GetPropertyByValueStub() = default;
    NO_MOVE_SEMANTIC(GetPropertyByValueStub);
    NO_COPY_SEMANTIC(GetPropertyByValueStub);
    void GenerateCircuit() override;
};

class SetPropertyByValueStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByValueStub(Circuit *circuit, const char* triple)
        : Stub("FastSetPropertyByValue", 4, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByValueStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByValueStub);
    NO_COPY_SEMANTIC(SetPropertyByValueStub);
    void GenerateCircuit() override;
};

class FastEqualStub : public Stub {
public:
    // 2 means argument counts
    explicit FastEqualStub(Circuit *circuit, const char* triple) : Stub("FastEqual", 2, circuit, triple) {}
    ~FastEqualStub() = default;
    NO_MOVE_SEMANTIC(FastEqualStub);
    NO_COPY_SEMANTIC(FastEqualStub);
    void GenerateCircuit() override;
};

class TryLoadICByNameStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit TryLoadICByNameStub(Circuit *circuit, const char* triple) : Stub("TryLoadICByName", 4, circuit, triple) {}
    ~TryLoadICByNameStub() = default;
    NO_MOVE_SEMANTIC(TryLoadICByNameStub);
    NO_COPY_SEMANTIC(TryLoadICByNameStub);
    void GenerateCircuit() override;
};

class TryLoadICByValueStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit TryLoadICByValueStub(Circuit *circuit, const char* triple)
        : Stub("TryLoadICByValue", 5, circuit, triple) {}
    ~TryLoadICByValueStub() = default;
    NO_MOVE_SEMANTIC(TryLoadICByValueStub);
    NO_COPY_SEMANTIC(TryLoadICByValueStub);
    void GenerateCircuit() override;
};

class TryStoreICByNameStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit TryStoreICByNameStub(Circuit *circuit, const char* triple) : Stub("TryStoreICByName", 5, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~TryStoreICByNameStub() = default;
    NO_MOVE_SEMANTIC(TryStoreICByNameStub);
    NO_COPY_SEMANTIC(TryStoreICByNameStub);
    void GenerateCircuit() override;
};

class TryStoreICByValueStub : public Stub {
public:
    // 6 : 6 means argument counts
    explicit TryStoreICByValueStub(Circuit *circuit, const char* triple)
        : Stub("TryStoreICByValue", 6, circuit, triple)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~TryStoreICByValueStub() = default;
    NO_MOVE_SEMANTIC(TryStoreICByValueStub);
    NO_COPY_SEMANTIC(TryStoreICByValueStub);
    void GenerateCircuit() override;
};
#endif
}  // namespace kungfu

#endif  // ECMASCRIPT_COMPILER_FASTPATH_STUB_H
