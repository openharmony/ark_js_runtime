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
var x1 = 16;
var y1 = 2;
var r1 = x1 ** y1;
print(r1);

var x2 = 16.8;
var y2 = 2;
var r2 = x2 ** y2;
print(r2);

var x3 = 16;
var y3 = 1.8;
var r3 = x3 ** y3;
print(r3);

var x4 = 16.8;
var y4 = 1.8;
var r4 = x4 ** y4;
print(r4);

var x5:any = "16";
var y5:number = 2;
var r5 = x5 ** y5;
print(r5);

var x6 = -16;
var y6 = 2;
var r6 = x6 ** y6;
print(r6);

var x7 = 16;
var y7 = -2;
var r7 = x7 ** y7;
print(r7);

// not supported type: string, bigint
