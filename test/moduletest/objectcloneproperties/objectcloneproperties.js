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

function fn(e) {
    return e;
}
var obj2 = new Int32Array(2);
var obj1 = {
    length: 1,
    0: 'a',
    toString() {return 'obj1';}
};
var obj3 = {
    get length() {throw "should not even consider the length property"},
    toString() {return 'obj3';}
};
var arr = [obj1, obj2, obj3];
var actual = arr.flatMap(fn);
var arrLike = {
    length: 4,
    0: obj1,
    1: obj2,
    2: obj3,
    get 3() {return arrLike},
    toString() {return 'obj4';}
};
actual = [].flatMap.call(arrLike, fn);
print(actual);