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

var FastVector = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    FastVector = ArkPrivate.Load(ArkPrivate.Vector);

    let map = new Map();
    let vector = new FastVector();
    vector.add(4); // index is 0
    vector.add(3);
    vector.add(1);
    vector.add(5);
    vector.add(14);
    let res = vector.toString();
    map.set("test add and toString:", res);
    // test insert, length, get, getIndexOf
    vector.insert(2, 2);
    map.set("test length:", vector.length == 6);
    map.set("test get(index is 2):", vector.get(2) == 2);
    map.set("test get(index is 3):", vector.get(3) !== 3); // false
    map.set("test getIndexOf(target is 3):", vector.getIndexOf(3) == 1); // true
    map.set("test getIndexOf(target is 2):", vector.getIndexOf(2) !== 5); // false
    // test isEmpty
    map.set("test isEmpty:", !vector.isEmpty());

    let vec = vector.clone();
    // test clear
    vector.clear();
    map.set("test clear:", vector.isEmpty());
    // // test set, clone
    vec.set(2, 8);
    map.set("test set:", vec.get(2) == 8 && vec.length == 6);
    // test subvector
    let subVec = vec.subVector(0, 3);
    map.set("test subVector and tostring:", subVec.toString());
    // test replaceAllElements
    subVec.replaceAllElements((item, index) => {
        return (item = 2 * item);
    });
    map.set("test replaceAllElements:", subVec.toString() == "8,6,16");
    // GetFirstElement
    map.set("test GetFirstElement:", subVec.getFirstElement() == 8 &&
            vec.getFirstElement() == 4);

    let arr = [4, 3, 8, 1, 5, 14];
    for (let i = 0; i < vector.length; i++) {
        map.set("for of " + arr[i], vec.get(i) == arr[i]);
    }

    let flag = false;
    try {
        vec["aa"] = 3;
    } catch (e) {
        flag = true;
    }

    map.set("test vector throw error", flag);
    flag = undefined;
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
        print("Test Vector success!!!");
    } else {
        print("Test Vector fail: " + flag);
    }
}
