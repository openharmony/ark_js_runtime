/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/stubs/runtime_stubs.h"

namespace panda::ecmascript::kungfu {
CallSignature BytecodeStubCSigns::callSigns_[BytecodeStubCSigns::NUM_OF_VALID_STUBS];

void BytecodeStubCSigns::Initialize()
{
#define INIT_SIGNATURES(name)                                            \
    BytecodeHandlerCallSignature::Initialize(&callSigns_[name]);         \
    callSigns_[name].SetID(ID_##name);                                   \
    callSigns_[name].SetCallConv(CallSignature::CallConv::GHCCallConv);  \
    callSigns_[name].SetConstructor(                                     \
    [](void* ciruit) {                                                   \
        return static_cast<void*>(                                       \
            new name##Stub(static_cast<Circuit*>(ciruit)));              \
    });
    INTERPRETER_BC_STUB_LIST(INIT_SIGNATURES)
#undef INIT_SIGNATURES
#define INIT_HELPER_SIGNATURES(name)                                                        \
    BytecodeHandlerCallSignature::Initialize(&callSigns_[name]);                            \
    callSigns_[name].SetID(HELPER_ID_##name);                                               \
    callSigns_[name].SetCallConv(CallSignature::CallConv::GHCCallConv);                     \
    callSigns_[name].SetTargetKind(CallSignature::TargetKind::BYTECODE_HELPER_HANDLER);     \
    callSigns_[name].SetConstructor(                                                        \
    [](void* ciruit) {                                                                      \
        return static_cast<void*>(                                                          \
            new name##Stub(static_cast<Circuit*>(ciruit)));                                 \
    });
    ASM_INTERPRETER_BC_HELPER_STUB_LIST(INIT_HELPER_SIGNATURES)
#undef INIT_HELPER_SIGNATURES
}

void BytecodeStubCSigns::GetCSigns(std::vector<const CallSignature*>& outCSigns)
{
    for (size_t i = 0; i < NUM_OF_VALID_STUBS; i++) {
        outCSigns.push_back(&callSigns_[i]);
    }
}
}  // namespace panda::ecmascript::kungfu
