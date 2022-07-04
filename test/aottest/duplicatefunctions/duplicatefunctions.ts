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

function foo1(a:number, b:number)
{
    return a + b;
}

var obj = {
    fun (a:number, b:number) { return a + b; },
};

class Obj1 {
    fun(a:number, b:number):number {
        return a + b;
    }
}

function foo2(a:number, b:number)
{
    return a + b;
}

var obj1 = new Obj1();

print(foo1(1, 1));
print(foo2(2, 2));
print(obj.fun(3, 3));
print(obj1.fun(4, 4));
