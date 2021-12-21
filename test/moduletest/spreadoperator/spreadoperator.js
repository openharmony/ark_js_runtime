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

//test spread Array
var arr1 = [...Array(16).keys()];
print(arr1.length);
print(arr1);

var arr2 = [1, 2, 4, 6, 7, 8, 9, 54];
var arr3 = [...arr2];
print(arr3.length);
print(arr3);

//test spread Set
const set1 = new Set();
set1.add(42);
set1.add(42);
set1.add(13);
set1.add(23);

var arr4 = [...set1.keys()];
print(arr4.length);
print(arr4);

var arr5 = [...set1];
print(arr5.length);
print(arr5);

//test spread map
const map1 = new Map();
map1.set('a', 1);
map1.set('b', 2);
map1.set('c', 3);

var arr6 = [...map1.keys()];
print(arr6.length);
print(arr6);

var arr7 = [...map1.values()];
print(arr7.length);
print(arr7);
