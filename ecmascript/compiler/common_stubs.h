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

#ifndef ECMASCRIPT_COMPILER_COMMON_STUBS_H
#define ECMASCRIPT_COMPILER_COMMON_STUBS_H

#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/test_stubs.h"

namespace panda::ecmascript::kungfu {
#define INTERPRETER_STUB_HELPER_LIST(V)  \
    V(AsmInterpreterEntry, 7)            \
    V(SingleStepDebugging, 7)            \
    V(HandleOverflow, 7)

#define COMMON_STUB_LIST(V)              \
    V(Add, 3)                            \
    V(Sub, 3)                            \
    V(Mul, 3)                            \
    V(Div, 3)                            \
    V(Mod, 3)                            \
    V(Equal, 3)                          \
    V(TypeOf, 2)                         \
    V(GetPropertyByName, 3)              \
    V(SetPropertyByName, 4)              \
    V(SetPropertyByNameWithOwn, 4)       \
    V(GetPropertyByIndex, 3)             \
    V(SetPropertyByIndex, 4)             \
    V(SetPropertyByIndexWithOwn, 4)      \
    V(GetPropertyByValue, 3)             \
    V(SetPropertyByValue, 4)             \
    V(SetPropertyByValueWithOwn, 4)      \
    V(TryLoadICByName, 4)                \
    V(TryLoadICByValue, 5)               \
    V(TryStoreICByName, 5)               \
    V(TryStoreICByValue, 6)              \
    V(SetValueWithBarrier, 4)

#define COMMON_STUB_ID_LIST(V)          \
    COMMON_STUB_LIST(V)                 \
    INTERPRETER_STUB_HELPER_LIST(V)     \
    TEST_STUB_LIST(V)

class MulGCTestStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit MulGCTestStub(Circuit *circuit) : Stub("MulGCTest", 3, circuit) {}
    ~MulGCTestStub() = default;
    NO_MOVE_SEMANTIC(MulGCTestStub);
    NO_COPY_SEMANTIC(MulGCTestStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class AddStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit AddStub(Circuit *circuit) : Stub("Add", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_FRAME);
    }
    ~AddStub() = default;
    NO_MOVE_SEMANTIC(AddStub);
    NO_COPY_SEMANTIC(AddStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SubStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit SubStub(Circuit *circuit) : Stub("Sub", 3, circuit) {}
    ~SubStub() = default;
    NO_MOVE_SEMANTIC(SubStub);
    NO_COPY_SEMANTIC(SubStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class MulStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit MulStub(Circuit *circuit) : Stub("Mul", 3, circuit) {}
    ~MulStub() = default;
    NO_MOVE_SEMANTIC(MulStub);
    NO_COPY_SEMANTIC(MulStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class DivStub : public Stub {
public:
    // 3 : 3 means argument counts
    explicit DivStub(Circuit *circuit) : Stub("Div", 3, circuit) {}
    ~DivStub() = default;
    NO_MOVE_SEMANTIC(DivStub);
    NO_COPY_SEMANTIC(DivStub);
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

class SetPropertyByIndexWithOwnStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByIndexWithOwnStub(Circuit *circuit) : Stub("SetPropertyByIndexWithOwn", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByIndexWithOwnStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByIndexWithOwnStub);
    NO_COPY_SEMANTIC(SetPropertyByIndexWithOwnStub);
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

class ModStub : public Stub {
public:
    // 3 means argument counts
    explicit ModStub(Circuit *circuit) : Stub("Mod", 3, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~ModStub() = default;
    NO_MOVE_SEMANTIC(ModStub);
    NO_COPY_SEMANTIC(ModStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class TypeOfStub : public Stub {
public:
    // 2 means argument counts
    explicit TypeOfStub(Circuit *circuit) : Stub("TypeOf", 2, circuit) {}
    ~TypeOfStub() = default;
    NO_MOVE_SEMANTIC(TypeOfStub);
    NO_COPY_SEMANTIC(TypeOfStub);
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
    explicit GetPropertyByValueStub(Circuit *circuit) : Stub("GetPropertyByValue", 3, circuit)
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
    explicit SetPropertyByValueStub(Circuit *circuit) : Stub("SetPropertyByValue", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByValueStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByValueStub);
    NO_COPY_SEMANTIC(SetPropertyByValueStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class SetPropertyByValueWithOwnStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetPropertyByValueWithOwnStub(Circuit *circuit) : Stub("SetPropertyByValueWithOwn", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetPropertyByValueWithOwnStub() = default;
    NO_MOVE_SEMANTIC(SetPropertyByValueWithOwnStub);
    NO_COPY_SEMANTIC(SetPropertyByValueWithOwnStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class EqualStub : public Stub {
public:
    // 3 means argument counts
    explicit EqualStub(Circuit *circuit) : Stub("FastEqual", 3, circuit) {}
    ~EqualStub() = default;
    NO_MOVE_SEMANTIC(EqualStub);
    NO_COPY_SEMANTIC(EqualStub);
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

class SetValueWithBarrierStub : public Stub {
public:
    // 4 : 4 means argument counts
    explicit SetValueWithBarrierStub(Circuit *circuit) : Stub("SetValueWithBarrier", 4, circuit)
    {
        circuit->SetFrameType(panda::ecmascript::FrameType::OPTIMIZED_ENTRY_FRAME);
    }
    ~SetValueWithBarrierStub() = default;
    NO_MOVE_SEMANTIC(SetValueWithBarrierStub);
    NO_COPY_SEMANTIC(SetValueWithBarrierStub);
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

class CommonStubCSigns {
public:
    enum ID {
#define DEF_STUB_ID(name, counter) name,
        COMMON_STUB_ID_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID
        NUM_OF_STUBS
    };

    static void Initialize();

    static void GetCSigns(std::vector<CallSignature*>& callSigns);

    static const CallSignature *Get(size_t index)
    {
        ASSERT(index < NUM_OF_STUBS);
        return &callSigns_[index];
    }
private:
    static CallSignature callSigns_[NUM_OF_STUBS];
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMMON_STUBS_H
