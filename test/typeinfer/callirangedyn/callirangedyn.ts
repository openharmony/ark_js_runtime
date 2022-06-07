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
        v1:number;
        v2:string;
        v3:boolean;
        v4:number;
        constructor(v1:number, v2:string, v3:boolean, v4:number) {
            this.v1 = v1;
            this.v2 = v2;
            this.v3 = v3;
            this.v4 = v4;
        }
    }
    function foo(v1:number, v2:string, v3:boolean, v4:number):A
    {
        let a = new A(v1, v2, v3, v4);
        return a;
    }
    typeof(foo(0, "1", true, 3));
}
