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
}

let map = new fastmap();
map.set("a", "aa");
map.set("b", "bb");

print("### test TreeMap start ###")
// test get, out: true
print("test get, out:", map.length == 2 && map.get("a") == "aa" && map.get("b") == "bb");
// test hasKey and hasValue, out: true
print("test hasKey and hasValue, out:", map.hasKey("a") && map.hasKey("b") && map.hasValue("aa") &&
      map.hasValue("bb") && !map.hasKey("c") && !map.hasValue("cc"));

map.set("c", "cc");
// test getFirstKey and getLastKey, out: true
print("test getFirstKey and getLastKey, out:", map.getFirstKey() == "a" && map.getLastKey() == "c");
// test getLowerKey and getHigherKey, out: true
print("test getLowerKey and getHigherKey, out:", map.getLowerKey("b") == "a" && map.getLowerKey("a") == undefined &&
      map.getHigherKey("b") == "c" && map.getHigherKey("c") == undefined);
// test keys, out: true
let iteratorKey = map.keys();
print("test keys, out:", iteratorKey.next().value == "a" && iteratorKey.next().value == "b" &&
      iteratorKey.next().value == "c" && iteratorKey.next().value == undefined);
// test values, out: true
let iteratorValues = map.values();
print("test values, out:", iteratorValues.next().value == "aa" && iteratorValues.next().value == "bb" &&
      iteratorValues.next().value == "cc" && iteratorValues.next().value == undefined);
// test entries, out: [c,cc], undefined
let iteratorEntries = map.entries();
iteratorEntries.next().value;
iteratorEntries.next().value;
print("test entries, out:", iteratorEntries.next().value);
print(iteratorEntries.next().value);

// test forof, out: [a, aa], [b, bb], [c, cc]
print("test forof, out:");
for (const item of map) {
    print(item);
}
// test forin, out:
print("test forin, out:");
for (const item in map) {
    print(item);
}
// test forEach, out:
let flag = false;
print("test forEach, out:");
function TestForEach(value, key, map) {
    flag = map.get(key) === value;
    if (!flag) {
        print(false)
    }
}
map.forEach(TestForEach);

let dmap = new fastmap();
// test setAll, out: 3
dmap.setAll(map);
print("test setAll, out:", dmap.length);
// test remove, out: true
print("test remove, out:", dmap.remove("a") == "aa" && dmap.length == 2);
// test replace, out: true
print("test replace, out:", dmap.replace("b", "dd") && dmap.get("b") == "dd");
// test clear, out: 0
dmap.clear();
print("test clear, out:", dmap.length);

try {
    map["aa"] = 3;
} catch (e) {
    print(e);
}
