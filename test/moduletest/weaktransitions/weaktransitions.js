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

var count = 0;

function addProperty(obj, index) {
    let key = count.toString() + "-" +index.toString() + "key";
    let val = {value: index.toString() + "value"};
    Object.defineProperty(obj, key, val);
}

for (let idx = 0; idx < 1000; ++idx) {
    let o = {a:1};
    for (let i = 0; i < 1000; ++i) {
        addProperty(o, i);
    }

    count++;
}

print("success");