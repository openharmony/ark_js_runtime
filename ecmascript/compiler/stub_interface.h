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
#ifndef ECMASCRIPT_COMPILER_STUB_INTERFACE_H
#define ECMASCRIPT_COMPILER_STUB_INTERFACE_H

#include <array>
#include <memory>

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/machine_type.h"
#include "libpandabase/macros.h"
#include "llvm-c/Types.h"

namespace kungfu {
enum ArgumentsOrder {
    DEFAULT_ORDER,  // Push Arguments in stack from right -> left
};

class StubInterfaceDescriptor {
public:
    enum CallStubKind {
        CODE_STUB,
        RUNTIME_STUB,
    };
    explicit StubInterfaceDescriptor(int flags, int paramCounter, ArgumentsOrder order, MachineType returnType)
        : flags_(flags), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
    }
    StubInterfaceDescriptor() = default;
    ~StubInterfaceDescriptor() = default;
    StubInterfaceDescriptor(StubInterfaceDescriptor const &other)
    {
        flags_ = other.flags_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        kind_ = other.kind_;
        returnType_ = other.returnType_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<MachineType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
    }

    StubInterfaceDescriptor &operator=(StubInterfaceDescriptor const &other)
    {
        flags_ = other.flags_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        kind_ = other.kind_;
        returnType_ = other.returnType_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<MachineType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
        return *this;
    }

    void SetParameters(MachineType *paramsType)
    {
        if (paramCounter_ > 0 && paramsType_ == nullptr) {
            paramsType_ = std::make_unique<std::vector<MachineType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = paramsType[i];
            }
        }
    }

    MachineType *GetParametersType() const
    {
        if (paramsType_ != nullptr) {
            return paramsType_->data();
        } else {
            return nullptr;
        }
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

    CallStubKind GetStubKind() const
    {
        return kind_;
    }

    void SetStubKind(CallStubKind kind)
    {
        kind_ = kind;
    }

private:
    CallStubKind kind_ {CODE_STUB};
    int flags_ {0};
    int paramCounter_ {0};
    ArgumentsOrder order_ {DEFAULT_ORDER};

    MachineType returnType_ {MachineType::NONE_TYPE};
    std::unique_ptr<std::vector<MachineType>> paramsType_ {nullptr};
};

class CallStubsImplement {
public:
    CallStubsImplement() = default;
    virtual ~CallStubsImplement() = default;
    NO_MOVE_SEMANTIC(CallStubsImplement);
    NO_COPY_SEMANTIC(CallStubsImplement);
    virtual void Initialize() = 0;
    virtual void *GetFastStub(int index) = 0;
    virtual void SetFastStub(int index, void *code) = 0;
    virtual void *GetRunTimeLLVMType(int index) = 0;
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

    void *GetRunTimeLLVMType(int index) override;
    void *GetModule() override
    {
        return reinterpret_cast<void *>(stubsModule_);
    }

private:
    std::array<LLVMValueRef, CALL_STUB_MAXCOUNT> llvmCallStubs_ {nullptr};
    std::array<LLVMTypeRef, CALL_STUB_MAXCOUNT> llvm_fuction_type_ {nullptr};
    LLVMModuleRef stubsModule_ {nullptr};
};
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_STUB_ID(name) NAME_##name
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

    void *GetRunTimeLLVMType(int index)
    {
        return stubsImpl_->GetRunTimeLLVMType(index);
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
    std::unique_ptr<CallStubsImplement> stubsImpl_ {nullptr};
    std::array<StubInterfaceDescriptor, CALL_STUB_MAXCOUNT> callStubsDescriptor_ {};
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_INTERFACE_H