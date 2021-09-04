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

var p = new Promise((resolve, reject) => {
    resolve(1479);
})
var p1 = Promise.reject(1357);
var p2 = Promise.resolve(2468);
var p3 = Promise.race([p1, p2]);
p3.then(
    (value) => {
        print("resolve");
        print(value);
    },
    (value) => {
        print("reject");
        print(value);
    }
)

p3.catch((value) => {
    print("catch");
    print(value);
})

var p4 = Promise.all([p, p2]);
p4.then(
    (value) => {
        print("resolve");
        print(value);
    },
    (value) => {
        print("reject");
        print(value);
    }
)
