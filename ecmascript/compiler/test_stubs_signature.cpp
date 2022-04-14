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

#include "ecmascript/compiler/call_signature.h"
namespace panda::ecmascript::kungfu {
DEF_CALL_SIGNATURE(FooAOT)
{
    // 7 : 7 input parameters
    CallSignature fooAot("FooAOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = fooAot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(Foo1AOT)
{
    // 7 : 7 input parameters
    CallSignature foo1Aot("Foo1AOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = foo1Aot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(Foo2AOT)
{
    // 7 : 7 input parameters
    CallSignature foo2Aot("Foo2AOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = foo2Aot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(FooNativeAOT)
{
    // 7 : 7 input parameters
    CallSignature foo2Aot("FooNativeAOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = foo2Aot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(FooBoundAOT)
{
    // 7 : 7 input parameters
    CallSignature foo2Aot("FooBoundAOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = foo2Aot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(Bar1AOT)
{
    // 8 : 8 input parameters
    CallSignature barAot("Bar1AOT", 0, 8,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = barAot;
    std::array<VariableType, 8> params = { // 8 : 8 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
        VariableType::JS_ANY(),     // c
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}

DEF_CALL_SIGNATURE(BarAOT)
{
    // 7 : 7 input parameters
    CallSignature barAot("BarAOT", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *callSign = barAot;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),     // calltarget
        VariableType::JS_ANY(),     // newTarget
        VariableType::JS_ANY(),     // thisTarget
        VariableType::JS_ANY(),     // a
        VariableType::JS_ANY(),     // b
    };
    callSign->SetParameters(params.data());
    callSign->SetCallConv(CallSignature::CallConv::WebKitJSCallConv);
}
}  // namespace panda::ecmascript::kungfu