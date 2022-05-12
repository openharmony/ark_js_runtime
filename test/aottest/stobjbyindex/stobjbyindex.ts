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

declare function print(str:any):string;

// test array
var array = [100, "hello"];
print(array[0]);
print(array[1]);
array[0] = "helloworld";
array[1] = 200;
print(array[0]);
print(array[1]);

// test object
let phrase: { [key: number | string]: any } = {
    1 : "100",
    "100" : "hello",

    get fullPhrase() {
        return `${this[1]} ${this["100"]}`;
    },

    set fullPhrase(value) {
        [this[1], this["100"]] = value.split(" ");
    }
};
print(phrase[1]);
print(phrase["100"]);
phrase[1] = "helloworld";
phrase["100"] = 1;
print(phrase[1]);
print(phrase["100"]);

// test getter and setter
print(phrase.fullPhrase);
phrase.fullPhrase = "world hello";
print(phrase.fullPhrase);
