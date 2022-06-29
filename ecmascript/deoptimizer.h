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

#ifndef ECMASCRIPT_DEOPTIMIZER_H
#define ECMASCRIPT_DEOPTIMIZER_H
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/llvm_stackmap_type.h"
namespace panda::ecmascript {
class JSThread;
using DeoptFromOptimizedEntryType = uint64_t (*)(uintptr_t curSp, uintptr_t vargsStart,
                                         uint32_t vargsNumber, JSTaggedType pc, JSTaggedType acc);
enum class SpecVregIndex: int {
    PC_INDEX = -1,
    ACC_INDEX = -2,
    BC_OFFSET_INDEX = -3,
};

struct Context {
    uintptr_t callsiteSp;
    uintptr_t callsiteFp;
};

class Deoptimizer {
public:
    Deoptimizer(JSTaggedType *lastLeaveFrame, JSThread * thread): lastLeaveFrame_(lastLeaveFrame),
        thread_(const_cast<JSThread*>(thread))
    {
        callTarget_ = JSMutableHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    }
    void CollectVregs(kungfu::DeoptBundleVec deoptBundle);
    kungfu::DeoptBundleVec CollectDeoptBundleVec();
    void ConstructASMInterpretFrame();
private:
    JSTaggedValue DeoptFromOptimizedEntry();
    JSTaggedType *lastLeaveFrame_ {nullptr};
    JSThread *thread_ {nullptr};
    JSTaggedType *preFrame_ {nullptr};
    std::unordered_map<kungfu::OffsetType, JSHandle<JSTaggedValue>> vregs_;
    struct Context context_ {0, 0};
    uint32_t pc_;
    JSMutableHandle<JSTaggedValue> callTarget_;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_DEOPTIMIZER_H