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

#ifndef ECMASCRIPT_TOOLING_TEST_TEST_LIST_H
#define ECMASCRIPT_TOOLING_TEST_TEST_LIST_H

#include <utility>
#include <vector>

#include "ecmascript/mem/c_string.h"
#include "libpandafile/file_items.h"

namespace panda::tooling::ecmascript::test {
using panda::ecmascript::CString;

std::vector<const char *> GetTestList(panda_file::SourceLang language);

void SetCurrentTestName(const char *testName);

std::pair<CString, CString> GetTestEntryPoint(const char *testName);
}  // namespace panda::tooling::ecmascript::test

#endif  // ECMASCRIPT_TOOLING_TEST_TEST_LIST_H
