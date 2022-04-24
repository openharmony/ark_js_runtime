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
class Obj1 {
    value:number;
    constructor(value:number) {
        this.value = value;
    }
}

class Obj2 {
    fun():void { 
        print("Hello World");
    } 
}

var obj1 = new Obj1(0);
print(obj1.value);
var obj2 = new Obj2();
obj2.fun();
