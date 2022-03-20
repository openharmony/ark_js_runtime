/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_ASM_DEFINES_H
#define ECMASCRIPT_ASM_DEFINES_H

#ifdef PANDA_TARGET_64
#define ASM_GLUE_CURRENT_FRAME_OFFSET     (16)
#define ASM_GLUE_LEAVE_FRAME_OFFSET       (24)
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET (2080)
#endif

#ifdef PANDA_TARGET_32
#define ASM_GLUE_CURRENT_FRAME_OFFSET     (16)
#define ASM_GLUE_LEAVE_FRAME_OFFSET       (24)
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET (1056)
#endif

#define OPTIMIZE_FRAME_TYPE              (0)
#define JS_ENTRY_FRAME_TYPE              (1)
#define LEAVE_FRAME_TYPE                 (3)
#define ASM_LEAVE_FRAME_TYPE             (5)
#define JSUNDEFINED                     (0xa)

#endif  // ECMASCRIPT_ASM_DEFINES_H
