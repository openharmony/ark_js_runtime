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

declare function print(tmp:any):string;
var num1:number = 123;
var num2:number = 0;
var num3:number = -0;
var num4:number = 123.456;
var num5:number = NaN;
var str:string = "abc";
var empString:string = "";
var undefinedValue:undefined = undefined;
var nullValue:null = null;
var arr : Array<any> = [1, "a"];
var obj : {[key:string]:any} = {name: "a"};

print(true || true);
print(true || false);
print(false || true);
print(false || false);
print(num1 || 1);
print(num2 || 1);
print(num3 || 1);
print(num4 || 1);
print(num5 || 1);
print(str || 1);
print(empString || 1);
print(undefinedValue || 1);
print(nullValue || 1);
print(arr || 1);
print(obj || 1);
