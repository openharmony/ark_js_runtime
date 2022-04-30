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

declare function print(arg:any):string;

let num:number = 1;
print(typeof(num));

let str:string = "helloworld";
print(typeof(str));

let boolTrue:boolean = true;
print(typeof(boolTrue));

let boolFalse:boolean = false;
print(typeof(boolTrue));

let undefine:any = undefined;
print(typeof(undefine));

let arr:any[] = [];
print(typeof(arr));

let obj = {};
print(typeof(obj));


