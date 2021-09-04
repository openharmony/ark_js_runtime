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

#include "ecmascript/js_dataview.h"

namespace panda::ecmascript {
int32_t JSDataView::GetElementSize(DataViewType type)
{
    int32_t size;
    switch (type) {
        case DataViewType::INT8:
        case DataViewType::UINT8:
        case DataViewType::UINT8_CLAMPED:
            size = 1;
            break;
        case DataViewType::INT16:
        case DataViewType::UINT16:
            size = 2;  // 2 means the length
            break;
        case DataViewType::INT32:
        case DataViewType::UINT32:
        case DataViewType::FLOAT32:
            size = 4;  // 4 means the length
            break;
        case DataViewType::FLOAT64:
            size = 8;  // 8 means the length
            break;
        default:
            UNREACHABLE();
    }
    return size;
}
}  // namespace panda::ecmascript