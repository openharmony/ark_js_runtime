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
echo "++++++++++++++++++++++++++++++++++"
echo "build llvm"
#date +%F ' '%H:%M:%S
#echo $@

BIN_PATH=$(cd $(dirname $0);pwd)
JSRUNTIME_HOME=$(dirname $(dirname ${BIN_PATH}))
BASE_HOME=${JSRUNTIME_HOME}/../../

echo ${BIN_PATH}
echo ${BASE_HOME}

if [ ! -d "${BASE_HOME}/third_party/llvm-project" ]; then
    cd ${BASE_HOME}/third_party
	dd if=/dev/zero of=/tmp/mem.swap bs=1M count=4096
    git clone git@gitee.com:openharmony-sig/third_party_llvm-project.git -b llvmorg-12.0.1-ark
    git checkout 12.0.1-ark-1.0
fi

cd ${BASE_HOME}/third_party/llvm-project
if [ ! -d "build" ];then
    mkdir build && cd build
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_ARK_GC_SUPPORT=ON -DLLVM_ENABLE_TERMINFO=OFF DLLVM_STATIC_LINK_CXX_STDLIB=OFF -DLLVM_ENABLE_ZLIB=OFF ../llvm
    ninja
else
    cd build
    if [ ! -d "lib" ]; then
        rm -rf *
        cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_ARK_GC_SUPPORT=ON -DLLVM_ENABLE_TERMINFO=OFF DLLVM_STATIC_LINK_CXX_STDLIB=OFF -DLLVM_ENABLE_ZLIB=OFF ../llvm
        ninja
    fi
fi

echo "++++++++++++++++++++++++++++++++++"
