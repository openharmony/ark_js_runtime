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

declare function print(str: any): string;
function* genFun() {
    let t = 0;
    for (var i = 0; i < 2; i += 1) {
        for (var j = 0; j < 2; j += 1) {
            for (var k = 0; k < 1; k += 1) {
                t++;
                yield t * 10000;
            }
            for (var k = 0; k < 2; k += 1) {
                try {
                    yield i * 1000 + j * 100 + k * 10 + t;
                } catch (e) {
                    print(e);
                }
            }
        }
    }
}

var func = genFun();
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);
print(func.next().value);