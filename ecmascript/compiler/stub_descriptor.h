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
#ifndef ECMASCRIPT_COMPILER_STUB_DESCRIPTOR_H
#define ECMASCRIPT_COMPILER_STUB_DESCRIPTOR_H

#include <array>
#include <memory>

#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/compiler/machine_type.h"
#include "libpandabase/macros.h"
#include "llvm-c/Types.h"

namespace kungfu {
enum class ArgumentsOrder {
    DEFAULT_ORDER,  // Push Arguments in stack from right -> left
};

class StubDescriptor {
public:
    enum class CallStubKind {
        CODE_STUB,
        RUNTIME_STUB,
        TEST_FUNC,
    };
    explicit StubDescriptor(std::string name, int flags, int paramCounter, ArgumentsOrder order,
                                     MachineType returnType)
        : name_(name), flags_(flags), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
    }
    StubDescriptor() = default;
    ~StubDescriptor() = default;
    StubDescriptor(StubDescriptor const &other)
    {
        name_ = other.name_;
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

    StubDescriptor &operator=(StubDescriptor const &other)
    {
        name_ = other.name_;
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

    const std::string &GetName()
    {
        return name_;
    }

private:
    std::string name_;
    CallStubKind kind_ {CallStubKind::CODE_STUB};
    int flags_ {0};
    int paramCounter_ {0};
    ArgumentsOrder order_ {ArgumentsOrder::DEFAULT_ORDER};

    MachineType returnType_ {MachineType::NONE_TYPE};
    std::unique_ptr<std::vector<MachineType>> paramsType_ {nullptr};
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_STUBDESCRIPTOR(name) FastStubDescriptors::GetInstance().GetStubDescriptor(FAST_STUB_ID(name))
#define GET_STUBDESCRIPTOR_BY_ID(id) FastStubDescriptors::GetInstance().GetStubDescriptor(id)

class FastStubDescriptors {
public:
    static FastStubDescriptors &GetInstance()
    {
        static FastStubDescriptors instance;
        return instance;
    }

    void InitializeStubDescriptors();

    StubDescriptor *GetStubDescriptor(int index)
    {
        return &callStubsDescriptor_[index];
    }

private:
    FastStubDescriptors()
    {
        InitializeStubDescriptors();
    }
    ~FastStubDescriptors() {}
    NO_MOVE_SEMANTIC(FastStubDescriptors);
    NO_COPY_SEMANTIC(FastStubDescriptors);
    std::array<StubDescriptor, CALL_STUB_MAXCOUNT> callStubsDescriptor_ {};
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_DESCRIPTOR_H