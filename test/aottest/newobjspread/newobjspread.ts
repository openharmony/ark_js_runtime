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

var A=[1,2,"a"];
class obj{
    a; b; c;
    constructor(...rest:any) {
        this.a = arguments[0];
        this.b = arguments[1];
        this.c = arguments[2];
    }
}
let c = new obj(...A);
print(c.a);
print(c.b);
print(c.c);