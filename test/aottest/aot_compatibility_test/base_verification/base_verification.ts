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

declare function print(arg:boolean):string;
declare var ArkTools:any;

{
    var hc0;
    var hc1;

    class C {
        a:number;
        constructor() {
            hc0 = ArkTools.getHClass(this);
            this.a = 2;
        }
    }

    let c = new C();

    hc1 = ArkTools.getHClass(c);
    print(hc0 === hc1);                     // verify no transition occurs in constructor
    print(ArkTools.isTSHClass(c));          // verify the hclass of c is come from compile phase
}
