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

#ifndef ECMASCRIPT_JS_PANDAFILE_ALLOCATOR_H
#define ECMASCRIPT_JS_PANDAFILE_ALLOCATOR_H

#include "ecmascript/class_linker/panda_file_translator.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
/**
 * allocate C buffer cross vm instance
 */
class JsPandaFileAllocator {
public:
    static void *AllocateBuffer(size_t size);
    static void FreeBuffer(void *mem);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JS_PANDAFILE_ALLOCATOR_H
