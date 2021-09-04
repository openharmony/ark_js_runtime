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

#include "ecmascript/compiler/fastpath_optimizer.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tagged_hash_table-inl.h"

namespace kungfu {
void FastArrayLoadElementOptimizer::GenerateCircuit()
{
    auto env = GetEnvironment();
    AddrShift aVal = Int64Argument(0);
    AddrShift indexVal = Int32Argument(1);

    // load a.length
    AddrShift lengthOffset = GetInteger32Constant(panda::ecmascript::JSArray::GetArrayLengthOffset());
    if (PtrValueCode() == ValueCode::INT64) {
        lengthOffset = SExtInt32ToInt64(lengthOffset);
    } else if (PtrValueCode() == ValueCode::INT32) {
        aVal = TruncInt64ToInt32(aVal);
    }
    AddrShift taggegLength = Load(MachineType::TAGGED_TYPE, aVal, lengthOffset);

    AddrShift intLength = TaggedCastToInt32(taggegLength);
    // if index < length
    StubOptimizerLabel ifTrue(env);
    StubOptimizerLabel ifFalse(env);
    Branch(Int32LessThan(indexVal, intLength), &ifTrue, &ifFalse);
    Bind(&ifTrue);
    Return(LoadFromObject(MachineType::TAGGED_TYPE, aVal, indexVal));
    Bind(&ifFalse);
    Return(GetUndefinedConstant());
}

FastRuntimeStubs::FastRuntimeStubs()
    : fastRuntimeEnvs_{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_RUNTIME_STUB_ENV(name, arguments) Environment(#name, arguments),
        FAST_RUNTIME_STUB_LIST(FAST_RUNTIME_STUB_ENV)
#undef FAST_RUNTIME_STUB_ENV
    }, fastRuntimeOptimizers_{new FastArrayLoadElementOptimizer(&fastRuntimeEnvs_[1])}
{
}
}  // namespace kungfu