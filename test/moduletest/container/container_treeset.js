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

var fastset = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastset = ArkPrivate.Load(ArkPrivate.TreeSet);

    let map = new Map();
    let set = new fastset();
    set.add("aa");
    set.add("bb");

    // test has: true
    map.set("test has:", set.length == 2 && set.has("aa") && set.has("bb") && !set.has("cc"));

    set.add("cc");
    // test getFirstKey and getLastKey: true
    map.set("test getFirstKey and getLastKey:", set.getFirstValue() == "aa" && set.getLastValue() == "cc");
    // test getLowerValue and getHigherValue out: true
    map.set("test getLowerValue and getHigherValue", set.getLowerValue("bb") == "aa" &&
            set.getLowerValue("aa") == undefined && set.getHigherValue("bb") == "cc" &&
            set.getHigherValue("cc") == undefined);

    // test values: true
    let iteratorSetValues = set.values();
    map.set("test values:", iteratorSetValues.next().value == "aa" && iteratorSetValues.next().value == "bb" &&
            iteratorSetValues.next().value == "cc" && iteratorSetValues.next().value == undefined);
    // test entries: [cc, cc], undefined
    let iteratorSetEntries = set.entries();
    iteratorSetEntries.next().value;
    iteratorSetEntries.next().value;
    map.set("test entries1:", iteratorSetEntries.next().value != undefined);
    map.set("test entries2:", iteratorSetEntries.next().value == undefined);

    // test forof: aa, bb, cc
    let arr = ["aa", "bb", "cc"];
    let i = 0;
    for (const item of set) {
        map.set(arr[i], item == arr[i]);
        i++;
    }

    // test forin:
    for (const item in set) {
        map.set("test forin:", item);
    }

    // test forEach:
    let setFlag = false;
    function TestForEach(value, key, set) {
        setFlag= set.has(key) && set.has(value);
        map.set("test forEach" + key, setFlag);
    }
    set.forEach(TestForEach);

    // test isEmpty: false
    map.set("test isEmpty:", !set.isEmpty());

    set.add("ee");
    set.add("dd");
    // test popFirst and popLast: true
    map.set("test popFirst and popLast:", set.length == 5 && set.popFirst() == "aa" &&
          set.popLast() == "ee" && !set.has("aa"));
    // test remove: true
    map.set("test remove:", set.remove("bb") && set.length == 2 && !set.has("bb"));
    // test clear: true
    set.clear();
    map.set("test clear:", set.length == 0 && !set.has("cc") && set.isEmpty());

    let flag = false;
    try {
        set["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    map.set("test set throw error", flag);

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
        print("Test TreeSet success!!!");
    } else {
        print("Test TreeSet fail: " + flag);
    }
}
