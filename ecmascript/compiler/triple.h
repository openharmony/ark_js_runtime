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
#ifndef ECMASCRIPT_COMPILER_TRIPLE_H
#define ECMASCRIPT_COMPILER_TRIPLE_H

#include <string>
#include "ecmascript/compiler/compiler_macros.h"

namespace kungfu {
class TripleConst {
public:
    static const char *GetLLVMAmd64Triple()
    {
        return llvmAmd64Triple_;
    }
    static const char *GetLLVMArm64Triple()
    {
        return llvmArm64Triple_;
    }
    static const char *GetLLVMArm32Triple()
    {
        return llvmArm32Triple_;
    }

    static const char *StringTripleToConst(const std::string &triple)
    {
        if (triple.compare(llvmAmd64Triple_) == 0) {
            return llvmAmd64Triple_;
        }
        if (triple.compare(llvmArm64Triple_) == 0) {
            return llvmArm64Triple_;
        }
        if (triple.compare(llvmArm32Triple_) == 0) {
            return llvmArm32Triple_;
        }
        UNREACHABLE();
    }

    static const char *llvmAmd64Triple_;
    static const char *llvmArm64Triple_;
    static const char *llvmArm32Triple_;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_TRIPLE_H
