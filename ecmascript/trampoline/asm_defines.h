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
#define ASM_GLUE_CURRENT_FRAME_OFFSET     2184
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET 4248
#endif

#ifdef PANDA_TARGET_32
#define ASM_GLUE_CURRENT_FRAME_OFFSET     2184
#define ASM_GLUE_RUNTIME_FUNCTIONS_OFFSET 3224
#endif

#define OPTIMIZE_FRAME_TYPE              0
#define JS_ENTRY_FRAME_TYPE              1
#define LEAVE_FRAME_TYPE                 3

#endif  // ECMASCRIPT_ASM_DEFINES_H
