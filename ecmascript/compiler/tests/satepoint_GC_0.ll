; Copyright (c) 2021 Huawei Device Co., Ltd.
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

define  i32 @main() #0  gc "statepoint-example" {
entry:
  %tmp1 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp2 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp3 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp4 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp5 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp6 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp7 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp8 = call i8 addrspace(0)* @malloc(i64 8)
  %tmp9 = call i8 addrspace(1)* @_Znam(i64 8)

  %value1 = bitcast i8 addrspace(0)* %tmp1 to i64 addrspace(0)*
  %value2 = bitcast i8 addrspace(0)* %tmp2 to i64 addrspace(0)*
  %value3 = bitcast i8 addrspace(0)* %tmp3 to i64 addrspace(0)*
  %value4 = bitcast i8 addrspace(0)* %tmp4 to i64 addrspace(0)*
  %value5 = bitcast i8 addrspace(0)* %tmp5 to i64 addrspace(0)*
  %value6 = bitcast i8 addrspace(0)* %tmp6 to i64 addrspace(0)*
  %value7 = bitcast i8 addrspace(0)* %tmp7 to i64 addrspace(0)*
  %value8 = bitcast i8 addrspace(0)* %tmp8 to i64 addrspace(0)*
  %value9 = bitcast i8 addrspace(1)* %tmp9 to i64 addrspace(1)*

  ;; 0x11223344 --> 287454020
  store i64 287454020, i64 addrspace(0)* %value1
  ;; 0x55667788 --> 1432778632‬
  store i64 1432778632, i64 addrspace(0)* %value2
  ;; 0x12345678 --> 305419896‬
  store i64 305419896, i64 addrspace(0)* %value3

  ;; 0x11223344 --> 287454020
  store i64 287454020, i64 addrspace(0)* %value4
  ;; 0x55667788 --> 1432778632‬
  store i64 1432778632, i64 addrspace(0)* %value5
  ;; 0x12345678 --> 305419896‬
  store i64 305419896, i64 addrspace(0)* %value6
  ;; 0x11223344 --> 287454020
  store i64 287454020, i64 addrspace(0)* %value7

  ;; 0x11223344 --> 287454020
  store i64 287454020, i64 addrspace(0)* %value8
  ;; 0x55667788 --> 1432778632‬
  store i64 1432778632, i64 addrspace(1)* %value9

  call void  @bar(i64 addrspace(0)* %value1, i64 addrspace(0)* %value2, i64 addrspace(0)* %value3, i64 addrspace(0)* %value4, i64 addrspace(0)* %value5, i64 addrspace(0)* %value6, i64 addrspace(0)* %value7, i64 addrspace(0)* %value8, i64 addrspace(1)* %value9)

  ret i32 0
}

define  void @bar(i64 addrspace(0)* %value, i64 addrspace(0)* %value2, i64 addrspace(0)* %value3, i64 addrspace(0)* %value4, i64 addrspace(0)* %value5, i64 addrspace(0)* %value6, i64 addrspace(0)* %value7, i64 addrspace(0)* %value8, i64 addrspace(1)* %value9) #0  gc "coreclr" {
entry:
  %ret = call i64  @test3(i64 addrspace(0)* %value, i64 addrspace(0)* %value2, i64 addrspace(0)* %value3, i64 addrspace(0)* %value4, i64 addrspace(0)* %value5, i64 addrspace(0)* %value6, i64 addrspace(0)* %value7, i64 addrspace(0)* %value8, i64 addrspace(1)* %value9)
  ret void
}

define i64  @test3(i64 addrspace(0)* %obj1, i64 addrspace(0)* %obj2, i64 addrspace(0)* %obj3, i64 addrspace(0)* %obj4, i64 addrspace(0)* %obj5, i64 addrspace(0)* %obj6, i64 addrspace(0)* %obj7, i64 addrspace(0)* %obj8, i64 addrspace(1)* %obj9) #0  gc "coreclr" {
entry:
  ;call void @DoSafepoint() [ "deopt"() ]
  call void @DoSafepoint()
  %obj_1 = load i64, i64 addrspace(0)* %obj1
  %obj_2 = load i64, i64 addrspace(0)* %obj2
  %obj_3 = load i64, i64 addrspace(0)* %obj3
  %obj_4 = load i64, i64 addrspace(0)* %obj4
  %obj_5 = load i64, i64 addrspace(0)* %obj5
  %obj_6 = load i64, i64 addrspace(0)* %obj6
  %obj_7 = load i64, i64 addrspace(0)* %obj7
  %obj_8 = load i64, i64 addrspace(0)* %obj8
  %obj_9 = load i64, i64 addrspace(1)* %obj9
  %result_1 = add i64 %obj_1, %obj_2
  %result_2 = add i64 %result_1, %obj_3
  %result_3 = add i64 %result_2, %obj_4
  %result_4 = add i64 %result_3, %obj_5
  %result_5 = add i64 %result_4, %obj_6
  %result_6 = add i64 %result_5, %obj_7
  %result_7 = add i64 %result_5, %obj_8
  %result_8 = add i64 %result_5, %obj_9
  ret i64 %result_8
}

declare  void @DoSafepoint()
declare noalias i8 addrspace(1)* @_Znam(i64)
declare noalias i8* @malloc(i64)

attributes #0 = { "frame-pointer"="all"}



