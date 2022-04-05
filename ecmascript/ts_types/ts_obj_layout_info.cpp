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
#include "ts_obj_layout_info.h"
#include "ecmascript/mem/assert_scope.h"

namespace panda::ecmascript {
void TSObjLayoutInfo::SetKey(const JSThread *thread, [[maybe_unused]] int index, const JSTaggedValue &key,
                             const JSTaggedValue &typeIdVal)
{
    DISALLOW_GARBAGE_COLLECTION;
    int number = NumberOfElements();
    ASSERT(number == index);
    SetNumberOfElements(thread, number + 1);
    SetPropertyInit(thread, number, key, typeIdVal);
}
}  // namespace panda::ecmascript