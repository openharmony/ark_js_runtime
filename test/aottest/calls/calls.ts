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

function foo() {
    return "pass";
}

function foo1(a: any) {
    return a + 1;
}

function foo2(a: any, b: any) {
    return a + b;
}

function foo3(a: any, b: any, c: any) {
    return a + b + c;
}

function foo4(a: any, b: any, c: any, d: any) {
    return a + b + c + d;
}

print(foo());
print(foo1(1));
print(foo2(1, 2));
print(foo3(1, 2, 3));
print(foo4(1, 2, 3, 4));