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

//declare function print(str:string):string;
declare function print(str:any):number;

var num1:number = 99
var num2:number = 2
var res:number = num2 - num1
print(res)
var num3: number = 50
var num4: number = 45.5
print(num3 - num4)

var num5:any = "99"
var num6:any = "2"
print(num5 - num6)