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
}

let set = new fastset();
set.add("aa");
set.add("bb");

print("### test TreeSet start ###")
// test has, out: true
print("test has, out:", set.length == 2 && set.has("aa") && set.has("bb") && !set.has("cc"));

set.add("cc");
// test getFirstKey and getLastKey, out: true
print("test getFirstKey and getLastKey, out:", set.getFirstValue() == "aa" && set.getLastValue() == "cc");
// test getLowerValue and getHigherValue out: true
print("test getLowerValue and getHigherValue out:", set.getLowerValue("bb") == "aa" &&
      set.getLowerValue("aa") == undefined && set.getHigherValue("bb") == "cc" && set.getHigherValue("cc") == undefined);

// test values, out: true
let iteratorSetValues = set.values();
print("test values, out:", iteratorSetValues.next().value == "aa" && iteratorSetValues.next().value == "bb" &&
      iteratorSetValues.next().value == "cc" && iteratorSetValues.next().value == undefined);
// test entries, out: [cc, cc], undefined
let iteratorSetEntries = set.entries();
iteratorSetEntries.next().value;
iteratorSetEntries.next().value;
print("test entries, out:", iteratorSetEntries.next().value);
print(iteratorSetEntries.next().value);

// test forof, out: aa, bb, cc
print("test forof, out:");
for (const item of set) {
    print(item);
}

// test forin, out:
print("test forin, out:");
for (const item in set) {
    print(item);
}

// test forEach, out:
let setFlag = false;
print("test forEach, out:");
function TestForEach(value, key, set) {
    setFlag= set.has(key) && set.has(value);
    if (!setFlag) {
        print(false);
    }
}
set.forEach(TestForEach);

// test isEmpty, out: false
print("test isEmpty, out:", set.isEmpty());

set.add("ee");
set.add("dd");
// test popFirst and popLast, out: true
print("test popFirst and popLast, out:", set.length == 5 && set.popFirst() == "aa" &&
      set.popLast() == "ee" && !set.has("aa"));
// test remove, out: true
print("test remove, out:", set.remove("bb") && set.length == 2 && !set.has("bb"));
// test clear, out: true
set.clear();
print("test clear, out:", set.length == 0 && !set.has("cc") && set.isEmpty());

try {
    set["aa"] = 3;
} catch (e) {
    print(e);
}
