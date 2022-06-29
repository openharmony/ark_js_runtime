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

#ifndef ECMASCRIPT_LLVM_STACKMAP_TYPE_H
#define ECMASCRIPT_LLVM_STACKMAP_TYPE_H
#include <iostream>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>
namespace panda::ecmascript::kungfu {
using OffsetType = int32_t;
using LargeInt = int64_t;
using DwarfRegType = uint16_t;
using DwarfRegAndOffsetType = std::pair<DwarfRegType, OffsetType>;
using CallSiteInfo = std::vector<DwarfRegAndOffsetType>;
using Fun2InfoType = std::pair<uintptr_t, DwarfRegAndOffsetType>;
using Pc2CallSiteInfo = std::unordered_map<uintptr_t, CallSiteInfo>;
using FpDelta = std::pair<int, uint32_t>;
using Func2FpDelta = std::unordered_map<uintptr_t, FpDelta>; // value: fpDelta & funcSize
using ConstInfo = std::vector<OffsetType>;
using Pc2ConstInfo = std::unordered_map<uintptr_t, ConstInfo>;
using DeoptBundle = std::variant<OffsetType, DwarfRegAndOffsetType, LargeInt>;
using DeoptBundleVec = std::vector<DeoptBundle>;
using Pc2DeoptBundle = std::unordered_map<uintptr_t, DeoptBundleVec>;
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_LLVM_STACKMAP_TYPE_H