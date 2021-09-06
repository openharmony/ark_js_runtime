/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

import { Car } from './B.js';   // Test direct Export & use after import
import { Star } from './C.js';  // Test indirect Export & use after import

let A = Car;

var myCar = {
     name: "HWCar_Test",
     type: "HW_Test",
     price: "CNY:XXW_Test"
 }

var infoA = A.carInfo.apply(myCar);

let C = Star;

 var myStar = {
     name: "Polaris_Test",
     type: "fixedStar_Test",
     color: "Yellow_Test"
 }

var infoC = Star.starColor.apply(myStar);

if (infoA != "HWCar_Test:HW_Test:CNY:XXW_Test" ) {
    print("Direct Export Fail");
} else if (infoC != "Polaris_Test:fixedStar_Test:Yellow_Test") {
    print("Indirect Export Fail");
} else {
    print("Pass!!");
}

