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
#ifndef ECMASCRIPT_COMPILER_CALL_SIGNATURE_H
#define ECMASCRIPT_COMPILER_CALL_SIGNATURE_H

#include <array>
#include <functional>
#include <memory>

#include "ecmascript/compiler/variable_type.h"
#include "libpandabase/macros.h"
#include "libpandabase/utils/bit_field.h"
#include "test_stubs_signature.h"

namespace panda::ecmascript::kungfu {
class Stub;
class Circuit;

enum class ArgumentsOrder {
    DEFAULT_ORDER,  // Push Arguments in stack from right -> left
};

class CallSignature {
public:
    using TargetConstructor = std::function<void *(void *)>;
    enum class TargetKind : uint8_t {
        COMMON_STUB = 0,
        RUNTIME_STUB,
        RUNTIME_STUB_VARARGS,
        RUNTIME_STUB_NO_GC,
        BYTECODE_HANDLER,
        BYTECODE_DEBUGGER_HANDLER,
        BYTECODE_HELPER_HANDLER,
        JSFUNCTION,

        STUB_BEGIN = COMMON_STUB,
        STUB_END = BYTECODE_HANDLER,
        BCHANDLER_BEGIN = BYTECODE_HANDLER,
        BCHANDLER_END = JSFUNCTION
    };
    enum class CallConv: uint8_t {
        CCallConv = 0,
        GHCCallConv = 1,
        WebKitJSCallConv = 2,
    };
    static constexpr size_t TARGET_KIND_BIT_LENGTH = 3;
    static constexpr size_t CALL_CONV_BIT_LENGTH = 2;
    using TargetKindBit = panda::BitField<TargetKind, 0, TARGET_KIND_BIT_LENGTH>;
    using CallConvBit = TargetKindBit::NextField<CallConv, CALL_CONV_BIT_LENGTH>;
    using VariadicArgsBit = CallConvBit::NextField<bool, 1>;
    using TailCallBit = VariadicArgsBit::NextField<bool, 1>;

    explicit CallSignature(std::string name, int flags, int paramCounter, ArgumentsOrder order, VariableType returnType)
        : name_(name), paramCounter_(paramCounter), order_(order), returnType_(returnType)
    {
        SetTargetKind(TargetKind::COMMON_STUB);
        SetCallConv(CallSignature::CallConv::CCallConv);
        SetTailCall(false);
        SetVariadicArgs(flags);
    }

    CallSignature() = default;

    ~CallSignature() = default;

    CallSignature(CallSignature const &other)
    {
        name_ = other.name_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        id_ = other.id_;
        returnType_ = other.returnType_;
        constructor_ = other.constructor_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
        kind_ = other.kind_;
    }

    CallSignature &operator=(CallSignature const &other)
    {
        name_ = other.name_;
        paramCounter_ = other.paramCounter_;
        order_ = other.order_;
        id_ = other.id_;
        returnType_ = other.returnType_;
        constructor_ = other.constructor_;
        if (paramCounter_ > 0 && other.paramsType_ != nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = other.GetParametersType()[i];
            }
        }
        kind_ = other.kind_;
        return *this;
    }

    bool IsCommonStub() const
    {
        return (GetTargetKind() == TargetKind::COMMON_STUB);
    }

    bool IsRuntimeVAStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB_VARARGS);
    }

    bool IsRuntimeStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB);
    }

    bool IsRuntimeNGCStub() const
    {
        return (GetTargetKind() == TargetKind::RUNTIME_STUB_NO_GC);
    }

    bool IsBCDebuggerStub() const
    {
        return (GetTargetKind() == TargetKind::BYTECODE_DEBUGGER_HANDLER);
    }

    bool IsStub() const
    {
        TargetKind targetKind = GetTargetKind();
        return TargetKind::STUB_BEGIN <= targetKind && targetKind < TargetKind::STUB_END;
    }

    bool IsBCStub() const
    {
        TargetKind targetKind = GetTargetKind();
        return TargetKind::BCHANDLER_BEGIN <= targetKind && targetKind < TargetKind::BCHANDLER_END;
    }

    bool IsBCHandlerStub() const
    {
        return (GetTargetKind() == TargetKind::BYTECODE_HANDLER);
    }

    void SetParameters(VariableType *paramsType)
    {
        if (paramCounter_ > 0 && paramsType_ == nullptr) {
            paramsType_ = std::make_unique<std::vector<VariableType>>(paramCounter_);
            for (int i = 0; i < paramCounter_; i++) {
                (*paramsType_)[i] = paramsType[i];
            }
        }
    }

    VariableType *GetParametersType() const
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

    VariableType GetReturnType() const
    {
        return returnType_;
    }

    ArgumentsOrder GetArgumentsOrder() const
    {
        return order_;
    }

    bool IsVariadicArgs() const
    {
        return VariadicArgsBit::Decode(kind_);
    }

    void SetVariadicArgs(bool variable)
    {
        VariadicArgsBit::Set<uint64_t>(variable, &kind_);
    }

    void SetTailCall(bool tailCall)
    {
        TailCallBit::Set<uint64_t>(tailCall, &kind_);
    }

    bool GetTailCall() const
    {
        return TailCallBit::Decode(kind_);
    }

    TargetKind GetTargetKind() const
    {
        return TargetKindBit::Decode(kind_);
    }

    void SetTargetKind(TargetKind kind)
    {
        TargetKindBit::Set<uint64_t>(kind, &kind_);
    }

    CallConv GetCallConv() const
    {
        return CallConvBit::Decode(kind_);
    }

    void SetCallConv(CallConv cc)
    {
        CallConvBit::Set<uint64_t>(cc, &kind_);
    }

    const std::string &GetName() const
    {
        return name_;
    }

    void SetConstructor(TargetConstructor ctor)
    {
        constructor_ = ctor;
    }

    TargetConstructor GetConstructor() const
    {
        return constructor_;
    }

    bool HasConstructor() const
    {
        return constructor_ != nullptr;
    }

    int GetID() const
    {
        return id_;
    }

    void SetID(int id)
    {
        id_ = id;
    }

private:
    std::string name_;
    int paramCounter_ {0};
    int id_ {-1};
    ArgumentsOrder order_ {ArgumentsOrder::DEFAULT_ORDER};
    VariableType returnType_ {VariableType::VOID()};
    std::unique_ptr<std::vector<VariableType>> paramsType_ {nullptr};
    TargetConstructor constructor_ {nullptr};
    uint64_t kind_ {0};
};

#define EXPLICIT_CALL_SIGNATURE_LIST(V)     \
    V(Add)                                  \
    V(Sub)                                  \
    V(Mul)                                  \
    V(MulGCTest)                            \
    V(Div)                                  \
    V(Mod)                                  \
    V(TypeOf)                               \
    V(Equal)                                \
    V(SetPropertyByName)                    \
    V(SetPropertyByNameWithOwn)             \
    V(SetPropertyByValue)                   \
    V(SetPropertyByValueWithOwn)            \
    V(GetPropertyByName)                    \
    V(GetPropertyByIndex)                   \
    V(SetPropertyByIndex)                   \
    V(SetPropertyByIndexWithOwn)            \
    V(GetPropertyByValue)                   \
    V(TryLoadICByName)                      \
    V(TryLoadICByValue)                     \
    V(TryStoreICByName)                     \
    V(TryStoreICByValue)                    \
    V(SetValueWithBarrier)                  \
    V(TestAbsoluteAddressRelocation)        \
    V(GetTaggedArrayPtrTest)                \
    V(BytecodeHandler)                      \
    V(BytecodeDebuggerHandler)              \
    V(CallRuntime)                          \
    V(AsmInterpreterEntry)                  \
    V(JSCallDispatch)                       \
    V(CallRuntimeWithArgv)                  \
    V(OptimizedCallOptimized)               \
    V(PushCallArgs0AndDispatch)             \
    V(PushCallArgsAndDispatchNative)       \
    V(PushCallArgs0AndDispatchSlowPath)     \
    V(PushCallArgs1AndDispatch)             \
    V(PushCallArgs1AndDispatchSlowPath)     \
    V(PushCallArgs2AndDispatch)             \
    V(PushCallArgs2AndDispatchSlowPath)     \
    V(PushCallArgs3AndDispatch)             \
    V(PushCallArgs3AndDispatchSlowPath)     \
    V(PushCallIRangeAndDispatch)            \
    V(PushCallIRangeAndDispatchNative)      \
    V(PushCallIRangeAndDispatchSlowPath)    \
    V(PushCallIThisRangeAndDispatch)        \
    V(PushCallIThisRangeAndDispatchSlowPath)\
    V(ResumeRspAndDispatch)                 \
    V(ResumeRspAndReturn)                   \
    V(ResumeCaughtFrameAndDispatch)         \
    V(StringsAreEquals)                     \
    V(BigIntEquals)                         \
    V(DebugPrint)                           \
    V(FatalPrint)                           \
    V(InsertOldToNewRSet)                   \
    V(DoubleToInt)                          \
    V(FloatMod)                             \
    V(FindElementWithCache)                 \
    V(MarkingBarrier)                       \
    V(CallArg0Dyn)                          \
    V(CallArg1Dyn)                          \
    V(CallArgs2Dyn)                         \
    V(CallArgs3Dyn)                         \
    V(CallIThisRangeDyn)                    \
    V(CallIRangeDyn)                        \
    V(JSCall)                               \
    V(JSCallWithArgV)                       \
    V(CreateArrayFromList)                  \
    V(JSObjectGetMethod)                    \
    V(JsProxyCallInternal)                  \
    V(JSFunctionEntry)                      \
    TEST_STUB_SIGNATRUE_LIST(V)

#define DECL_CALL_SIGNATURE(name)                                  \
class name##CallSignature final {                                  \
    public:                                                        \
        static void Initialize(CallSignature *descriptor);         \
    };
EXPLICIT_CALL_SIGNATURE_LIST(DECL_CALL_SIGNATURE)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_SIGNATURE(name)                                  \
    void name##CallSignature::Initialize([[maybe_unused]] CallSignature *callSign)
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_CALL_SIGNATURE_H
