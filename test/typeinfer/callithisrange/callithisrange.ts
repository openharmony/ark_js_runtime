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

{
    class A {
        v:number;
        constructor(v:number) {
            this.v = v;
        }
        fun(value:number):number { 
            return this.v + value;
        }
        fun1():void { 
            this.v += 1;
        }
        fun2():string { 
            return "hello";
        }
        fun3():boolean { 
            return true;
        }
    }
    let a = new A(1);
    typeof(a.fun(2));
    typeof(a.fun1());
    typeof(a.fun2());
    typeof(a.fun3());
}
