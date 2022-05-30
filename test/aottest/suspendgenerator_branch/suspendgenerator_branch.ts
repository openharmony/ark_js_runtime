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
function* genfunc(a:number, b:number) {
    var c:number = 0;
    var d:number = 0;
    var e:number = 0;
    if (a > 0) {
        c = b + 11;
        d = c + 55;
        yield c;
        e = d + 22;
    } else {
        c = b + 33;
        d = c + 66;
        yield c;
        e = d + 44;
    }
    yield e;
}

let gen1 = genfunc(1, 2);
var a = gen1.next().value;
var b = gen1.next().value;

let gen2 = genfunc(0, 2);
var c = gen2.next().value;
var d = gen2.next().value;

print(a);
print(b);
print(c);
print(d);