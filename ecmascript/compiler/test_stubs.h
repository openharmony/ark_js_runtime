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

#ifndef ECMASCRIPT_COMPILER_TEST_STUBS_H
#define ECMASCRIPT_COMPILER_TEST_STUBS_H

#include "ecmascript/compiler/stub.h"

namespace panda::ecmascript::kungfu {
#define IGNORE_TEST_STUB(name, count)

#ifdef ECMASCRIPT_ENABLE_TEST_STUB

#define TEST_STUB_LIST(V)               \
    V(FooAOT, 7)                        \
    V(Foo1AOT, 7)                       \
    V(Foo2AOT, 7)                       \
    V(FooNativeAOT, 7)                  \
    V(FooBoundAOT, 7)                   \
    V(BarAOT, 7)                        \
    V(Bar1AOT, 8)                       \
    V(FooProxyAOT, 7)                   \
    V(FooProxy2AOT, 7)                  \
    V(Bar2AOT, 7)
#else
    #define TEST_STUB_LIST(V)
#endif

class FooAOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit FooAOTStub(Circuit *circuit) : Stub("FooAOT", 7, circuit)
    {
    }
    ~FooAOTStub() = default;
    NO_MOVE_SEMANTIC(FooAOTStub);
    NO_COPY_SEMANTIC(FooAOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class BarAOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit BarAOTStub(Circuit *circuit) : Stub("BarAOT", 7, circuit)
    {
    }
    ~BarAOTStub() = default;
    NO_MOVE_SEMANTIC(BarAOTStub);
    NO_COPY_SEMANTIC(BarAOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class Foo1AOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit Foo1AOTStub(Circuit *circuit) : Stub("Foo1AOT", 7, circuit)
    {
    }
    ~Foo1AOTStub() = default;
    NO_MOVE_SEMANTIC(Foo1AOTStub);
    NO_COPY_SEMANTIC(Foo1AOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class Foo2AOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit Foo2AOTStub(Circuit *circuit) : Stub("Foo2AOT", 7, circuit)
    {
    }
    ~Foo2AOTStub() = default;
    NO_MOVE_SEMANTIC(Foo2AOTStub);
    NO_COPY_SEMANTIC(Foo2AOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FooNativeAOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit FooNativeAOTStub(Circuit *circuit) : Stub("FooNativeAOT", 7, circuit)
    {
    }
    ~FooNativeAOTStub() = default;
    NO_MOVE_SEMANTIC(FooNativeAOTStub);
    NO_COPY_SEMANTIC(FooNativeAOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FooBoundAOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit FooBoundAOTStub(Circuit *circuit) : Stub("FooBoundAOT", 7, circuit)
    {
    }
    ~FooBoundAOTStub() = default;
    NO_MOVE_SEMANTIC(FooBoundAOTStub);
    NO_COPY_SEMANTIC(FooBoundAOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class Bar1AOTStub : public Stub {
public:
    // 8 : 8 means argument counts
    explicit Bar1AOTStub(Circuit *circuit) : Stub("Bar1AOT", 8, circuit)
    {
    }
    ~Bar1AOTStub() = default;
    NO_MOVE_SEMANTIC(Bar1AOTStub);
    NO_COPY_SEMANTIC(Bar1AOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FooProxyAOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit FooProxyAOTStub(Circuit *circuit) : Stub("FooProxyAOT", 7, circuit)
    {
    }
    ~FooProxyAOTStub() = default;
    NO_MOVE_SEMANTIC(FooProxyAOTStub);
    NO_COPY_SEMANTIC(FooProxyAOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class FooProxy2AOTStub : public Stub {
public:
    // 7 : 7 means argument counts
    explicit FooProxy2AOTStub(Circuit *circuit) : Stub("FooProxy2AOTStub", 7, circuit)
    {
    }
    ~FooProxy2AOTStub() = default;
    NO_MOVE_SEMANTIC(FooProxy2AOTStub);
    NO_COPY_SEMANTIC(FooProxy2AOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};

class Bar2AOTStub : public Stub {
public:
    // 5 : 5 means argument counts
    explicit Bar2AOTStub(Circuit *circuit) : Stub("Bar2AOT", 5, circuit)
    {
    }
    ~Bar2AOTStub() = default;
    NO_MOVE_SEMANTIC(Bar2AOTStub);
    NO_COPY_SEMANTIC(Bar2AOTStub);
    void GenerateCircuit(const CompilationConfig *cfg) override;
};
}
#endif