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

var arrayList = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    arrayList = ArkPrivate.Load(ArkPrivate.ArrayList);
    let arr = new arrayList();
    arr.add(1);
    arr.add(2);

    let map = new Map();
    let flag1 = false;
    try {
        arr["aa"] = 3;
    } catch (e) {
        flag1 = true;
    }
    map.set("flag1", flag1);

    let flag2 = true;
    for (let i = 0; i < arr.length; i++) {
        if (arr[i] != (i + 1)) {
            flag2 = false;
            break;
        }
    }
    map.set("flag2", flag2);
    let flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    map.forEach(elements);
    if (!flag) {
        print("Test ArrayList success!!!");
    } else {
        print("Test ArrayList fail: " + flag);
    }
}

