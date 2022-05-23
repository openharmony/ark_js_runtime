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
}

let vector = new FastVector();
vector.add(4); // index is 0
vector.add(3);
vector.add(1);
vector.add(5);
vector.add(14);
print("### test Vector start ###")
let res = vector.toString();
print("test add and toString, out:", res);
// test insert, length, get, getIndexOf
vector.insert(2, 2);
print("test length, out:", vector.length == 6);
print("test get(index is 2), out:", vector.get(2) == 2);
print("test get(index is 3), out:", vector.get(3) == 3); // false
print("test getIndexOf(target is 3), out:", vector.getIndexOf(3) == 1); // true
print("test getIndexOf(target is 2), out:", vector.getIndexOf(2) == 5); // false
// test isEmpty
print("test isEmpty, out:", vector.isEmpty());

let vec = vector.clone();
// test clear
vector.clear();
print("test clear, out:", vector.isEmpty());
// test set, clone
vec.set(2, 8);
print("test set, out:", vec.get(2) == 8 && vec.length == 6);
// test subvector
let subVec = vec.subVector(0, 3);
print("test subVector and tostring, out:", subVec.toString());
// test replaceAllElements
subVec.replaceAllElements((item, index) => {
    return (item = 2 * item);
});
print("test replaceAllElements, out:", subVec.toString() == "8,6,16");
// GetFirstElement
print("test GetFirstElement, out:", subVec.getFirstElement() == 8 &&
                                    vec.getFirstElement() == 4);
print("test forof, out:");
for (const item of vec) {
    print(item);
}

try {
    vec["aa"] = 3;
} catch (e) {
    print(e);
}