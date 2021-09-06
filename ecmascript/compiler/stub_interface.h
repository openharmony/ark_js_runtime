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
#ifndef PANDA_RUNTIME_ECMASCRIPT_COMPILER_STUB_INTERFACE_H
#define PANDA_RUNTIME_ECMASCRIPT_COMPILER_STUB_INTERFACE_H

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/code.h"
#include "llvm-c/Types.h"

namespace kungfu {
enum ArgumentsOrder {
    DEFAULT_ORDER,  // Push Arguments in stack from right -> left
};

class StubInterfaceDescriptor {
public:
    explicit StubInterfaceDescriptor(int flags, int paramCounter, ArgumentsOrder order, MachineType returnType)
        : flags_(flags), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
    }
    explicit StubInterfaceDescriptor(int flags, int paramCounter, ArgumentsOrder order, MachineType returnType,
                                     MachineType *paramsType)
        : flags_(flags), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
        paramsType_ = paramsType;
    }
    StubInterfaceDescriptor() = default;
    ~StubInterfaceDescriptor()
    {
        if (paramsType_ != nullptr) {
            delete paramsType_;
        }
    }

    void SetParameters(MachineType *paramsType)
    {
        paramsType_ = paramsType;
    }

    MachineType *GetParametersType() const
    {
        return paramsType_;
    }

    int GetParametersCount() const
    {
        return paramCounter_;
    }

    MachineType GetReturnType() const
    {
        return returnType_;
    }

    ArgumentsOrder GetArgumentsOrder() const
    {
        return order_;
    }

    int GetFlags() const
    {
        return flags_;
    }

private:
    int flags_{0};
    int paramCounter_{0};
    ArgumentsOrder order_{DEFAULT_ORDER};

    MachineType returnType_{MachineType::NONE_TYPE};
    MachineType *paramsType_{nullptr};
};

class StubInterface final {
public:
    explicit StubInterface(panda::ecmascript::JSHandle<panda::ecmascript::Code> code,
                           StubInterfaceDescriptor *descriptor)
        : code_(code), descriptor_(descriptor)
    {
    }
    virtual ~StubInterface() = default;
    DEFAULT_COPY_SEMANTIC(StubInterface);
    DEFAULT_MOVE_SEMANTIC(StubInterface);

    panda::ecmascript::JSHandle<panda::ecmascript::Code> GetCode() const
    {
        return code_;
    }

    StubInterfaceDescriptor *GetDescriptor() const
    {
        return descriptor_;
    }

private:
    panda::ecmascript::JSHandle<panda::ecmascript::Code> code_;
    StubInterfaceDescriptor *descriptor_;
};

class CallStubsImplement {
public:
    CallStubsImplement() = default;
    virtual ~CallStubsImplement() = default;
    NO_MOVE_SEMANTIC(CallStubsImplement);
    NO_COPY_SEMANTIC(CallStubsImplement);
    enum CallStubName {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_FAST_STUB(name, counter) NAME_##name,
        CALL_STUB_LIST(DEF_FAST_STUB)
#undef DEF_FAST_STUB
            CALL_STUB_MAXCOUNT,
    };

    virtual void Initialize() = 0;
    virtual void *GetFastStub(int index) = 0;
    virtual void SetFastStub(int index, void *code) = 0;
    virtual void *GetModule() = 0;
};

class LLVMStubsImplement : public CallStubsImplement {
public:
    LLVMStubsImplement();
    ~LLVMStubsImplement() override = default;
    NO_MOVE_SEMANTIC(LLVMStubsImplement);
    NO_COPY_SEMANTIC(LLVMStubsImplement);
    void Initialize() override;
    void *GetFastStub(int index) override;
    void SetFastStub(int index, void *code) override;
    void *GetModule() override
    {
        return reinterpret_cast<void *>(stubsModule_);
    }

private:
    std::array<LLVMValueRef, CALL_STUB_MAXCOUNT> llvmCallStubs_{nullptr};
    LLVMModuleRef stubsModule_{nullptr};
};
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_STUB_ID(name) CallStubsImplement::NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_STUBDESCRIPTOR(name) FastStubs::GetInstance().GetStubDescriptor(FAST_STUB_ID(name))

class FastStubs {
public:
    static FastStubs &GetInstance()
    {
        static FastStubs instance;
        return instance;
    }

    void InitializeStubDescriptors();

    void InitializeFastStubs()
    {
        InitializeStubDescriptors();
        stubsImpl_->Initialize();
    }

    void *GetFastStub(int index) const
    {
        return stubsImpl_->GetFastStub(index);
    }

    void SetFastStub(int index, void *code)
    {
        return stubsImpl_->SetFastStub(index, code);
    }

    void *GetModule() const
    {
        return stubsImpl_->GetModule();
    }

    StubInterfaceDescriptor *GetStubDescriptor(int index)
    {
        return &callStubsDescriptor_[index];
    }

private:
    FastStubs()
    {
        stubsImpl_ = std::make_unique<LLVMStubsImplement>();
        InitializeFastStubs();
    }
    ~FastStubs() {}
    NO_MOVE_SEMANTIC(FastStubs);
    NO_COPY_SEMANTIC(FastStubs);
    std::unique_ptr<CallStubsImplement> stubsImpl_{nullptr};
    std::array<StubInterfaceDescriptor, CallStubsImplement::CALL_STUB_MAXCOUNT> callStubsDescriptor_{};
};
}  // namespace kungfu
#endif  // PANDA_RUNTIME_ECMASCRIPT_COMPILER_STUB_INTERFACE_H