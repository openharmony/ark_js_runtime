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

#ifndef ARK_INITIALIZEDEBUGGER_FUZZER_H
#define ARK_INITIALIZEDEBUGGER_FUZZER_H

#include <cstdint>
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>

int makeUniqFile(const uint8_t *data, size_t size, char* file_name_uniq, const char* ext)
{
    int UNIQ_ID = 0;
    int fd = open(file_name_uniq, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        return -1;
    }

    write(fd, data, size);
    close(fd);
    UNIQ_ID++;
    return 0;
}

uint16_t U16_AT(const uint8_t *ptr)
{
    return ((ptr[0] << 8) | ptr[1]);
}

uint32_t U32_AT(const uint8_t *ptr)
{
    return ((ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]);
}

#endif