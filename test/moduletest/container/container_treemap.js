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

var fastmap = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastmap = ArkPrivate.Load(ArkPrivate.TreeMap);

    let res = new Map();
    let map = new fastmap();
    map.set("a", "aa");
    map.set("b", "bb");

    // test get: true
    res.set("test get:", map.length == 2 && map.get("a") == "aa" && map.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", map.hasKey("a") && map.hasKey("b") && map.hasValue("aa") &&
            map.hasValue("bb") && !map.hasKey("c") && !map.hasValue("cc"));

    map.set("c", "cc");
    // test getFirstKey and getLastKey: true
    res.set("test getFirstKey and getLastKey:", map.getFirstKey() == "a" && map.getLastKey() == "c");
    // test getLowerKey and getHigherKey: true
    res.set("test getLowerKey and getHigherKey:", map.getLowerKey("b") == "a" && map.getLowerKey("a") == undefined &&
            map.getHigherKey("b") == "c" && map.getHigherKey("c") == undefined);
    // test keys: true
    let iteratorKey = map.keys();
    res.set("test keys:", iteratorKey.next().value == "a" && iteratorKey.next().value == "b" &&
            iteratorKey.next().value == "c" && iteratorKey.next().value == undefined);
    // test values: true
    let iteratorValues = map.values();
    res.set("test values:", iteratorValues.next().value == "aa" && iteratorValues.next().value == "bb" &&
            iteratorValues.next().value == "cc" && iteratorValues.next().value == undefined);
    // test entries: [c,cc], undefined
    let iteratorEntries = map.entries();
    iteratorEntries.next().value;
    iteratorEntries.next().value;
    res.set("test entries1:", iteratorEntries.next().value != undefined);
    res.set("itest entries2:", iteratorEntries.next().value == undefined);

    // test forof: [a, aa], [b, bb], [c, cc]
    let arr = ["aa", "bb", "cc"];
    let i = 0;
    for (const item of map) {
        res.set(arr[i], item[1] == arr[i]);
        i++;
    }
    // test forin:
    for (const item in map) {
        res.set("test forin", false);
    }
    // test forEach:
    let flag = false;
    function TestForEach(value, key, map) {
        flag = map.get(key) === value;
        res.set("test forEach" + key, flag)
    }
    map.forEach(TestForEach);

    let dmap = new fastmap();
    // test setAll: 3
    dmap.setAll(map);
    res.set("test setAll:", dmap.length == 3);
    // test remove: true
    res.set("test remove:", dmap.remove("a") == "aa" && dmap.length == 2);
    // test replace: true
    res.set("test replace:", dmap.replace("b", "dd") && dmap.get("b") == "dd");
    // test clear: 0
    dmap.clear();
    res.set("test clear:", dmap.length == 0);

    flag = false;
    try {
        map["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    res.set("test map throw error", flag);
    flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    res.forEach(elements);
    if (!flag) {
        print("Test TreeMap success!!!");
    } else {
        print("Test TreeMap fail: " + flag);
    }
}
