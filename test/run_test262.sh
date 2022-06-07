#!/bin/bash
# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e
CODESPACE=$(pwd)
export PATH=${CODESPACE}/prebuilts/build-tools/common/nodejs/node-v12.18.4-linux-x64/bin:$PATH
node -v
npm config set registry https://repo.huaweicloud.com/repository/npm/
pushd ark/ts2abc
    time=$(date +'%Y%m%d%H%M%S')
    if [ ! -d report ];then
        mkdir report
    fi

    pushd ../../
    if [ -d out/rk3568 ];then
        pushd ark/ts2abc
        python3 test262/run_test262.py --es2015 all --threads=16 --libs-dir ../../out/rk3568/clang_x64/ark/ark:../../out/rk3568/clang_x64/ark/ark_js_runtime:../../out/rk3568/clang_x64/thirdparty/icu:../../prebuilts/clang/ohos/linux-x86_64/llvm/lib --ark-tool=../../out/rk3568/clang_x64/ark/ark_js_runtime/ark_js_vm --ark-frontend-tool=../../out/rk3568/clang_x64/ark/ark/build/src/index.js
        popd
    fi
    if [ -d out/hispark_taurus ];then
        pushd ark/ts2abc
        python3 test262/run_test262.py --es2015 all --threads=16 --libs-dir ../../out/hispark_taurus/clang_x64/ark/ark:../../out/hispark_taurus/clang_x64/ark/ark_js_runtime:../../out/hispark_taurus/clang_x64/thirdparty/icu:../../prebuilts/clang/ohos/linux-x86_64/llvm/lib --ark-tool=../../out/hispark_taurus/clang_x64/ark/ark_js_runtime/ark_js_vm --ark-frontend-tool=../../out/hispark_taurus/clang_x64/ark/ark/build/src/index.js
        popd
    fi
    popd
    
    if [ $? -ne 0 ];then
        echo 'execute run_test262.py failed!'
        exit 1;
    fi

    if [ ! -f out/test262/result.txt ];then
        echo 'The result.txt file of test262 is not produced!'
        exit 1;
    fi

    cp out/test262/result.txt report/result_es2015_${time}.txt

    pushd report
        es2015_fail=$(grep FAIL result_es2015_${time}.txt | wc -l)
        threshold=0
        if [ ${es2015_fail} -gt ${threshold} ];then
            echo 'test262 fail case over thresgold'
            exit 1;
        else
            exit 0;
        fi
    popd
popd
