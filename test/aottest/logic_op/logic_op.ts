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

var num1:number = 99;
var num2:number = 2;

var num3: number = 50
var num4: number = 45.5

var str1: string = "Runoob";
var str2: string = "hello";

var str3: any = "hello the world"
var str4: any = "hello"

var flag1 : any = false
var flag2 : boolean = true

print("operator == test list:")
print(num1 == num2)
print(num3 == num4)
print(str1 == str2)
print(str1 == str1)
print(str3 == str4)
print(flag1 == flag2)

print("operator != test list:")
print(num1 != num2)
print(num3 != num4)
print(str1 != str2)
print(str1 != str1)
print(str3 != str4)
print(flag1 != flag2)

print("operator < test list:")
print(num1 < num2)
print(num3 < num4)
print(str1 < str2)
print(str1 < str1)
print(str3 < str4)
print(flag1 < flag2)

print("operator <= test list:")
print(num1 <= num2)
print(num3 <= num4)
print(str1 <= str2)
print(str1 <= str1)
print(str3 <= str4)
print(flag1 <= flag2)

print("operator > test list:")
print(num1 > num2)
print(num3 > num4)
print(str1 > str2)
print(str1 > str1)
print(str3 > str4)
print(flag1 > flag2)

print("operator >= test list:")
print(num1 >= num2)
print(num3 >= num4)
print(str1 >= str2)
print(str1 >= str1)
print(str3 >= str4)
print(flag1 >= flag2)
