#!/bin/bash
# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
set -e
clear

echo "++++++++++++++++++++++++++++++++++++++++"
date +%F' '%H:%M:%S
echo $@
echo "++++++++++++++++++++++++++++++++++++++++"
echo

source_root_dir=$(cd $(dirname $0);pwd)

if [[ "${source_root_dir}x" == "x" ]]; then
  echo "Error: source_root_dir cannot be empty."
  exit 1
fi

case $(uname -s) in
    Darwin)
        HOST_DIR="darwin-x86"
        HOST_OS="mac"
        ;;
    Linux)
        HOST_DIR="linux-x86"
        HOST_OS="linux"
        ;;
    *)
        echo "Unsupported host platform: $(uname -s)"
        RET=1
        exit $RET
esac

export PATH=${source_root_dir}/prebuilts/build-tools/${HOST_DIR}/bin:$PATH

tools="${source_root_dir}/prebuilts/build-tools/${HOST_DIR}/bin"
own_npm="${source_root_dir}/prebuilts/build-tools/common/nodejs/node-v12.18.4-linux-x64/bin/npm"
own_node="${source_root_dir}/prebuilts/build-tools/common/nodejs/node-v12.18.4-linux-x64/bin/node"
build="out/ark/ark/build"
add_path="out:${source_root_dir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib"


if [ "$1" = "c" ]
then
  echo "clear out !"
  rm -rf out *.log test.abc test test.js gen.sh .gn
elif [ "$1" = "log" ]
then
  $tools/gn gen out
  cd out/
  $tools/ninja all -v | tee ../xxx.log
  cd ..
elif [ "$1" = "abc" ]
then
  export LD_LIBRARY_PATH=$add_path
  cd $build
  $own_npm install
  cd -
  $own_node --expose-gc $build/src/index.js test.js
elif [ "$1" = "." ]
then
  export LD_LIBRARY_PATH=$add_path
  ./out/ark_js_vm test.abc
elif [ "$1" = "all" ]
then
  $tools/gn gen out 
  cd out/
  $tools/ninja all
  cd ..
  export LD_LIBRARY_PATH=$add_path
  cd $build
  $own_npm install
  cd -
  $own_node --expose-gc $build/src/index.js test.js
  ./out/ark_js_vm test.abc
elif [ "$1" = "ark" ]
then
  export script_root_dir=$(cd $(dirname $0);pwd)  #获取sh脚本绝对路径
  cp -r $script_root_dir/* $script_root_dir/.gn ./
  echo "cp -r build/compile_script/* ./"
else
  $tools/gn gen out 
  cd out/
  $tools/ninja all
  cd ..
  echo
  echo -e "\033[32m  + c   == clear out !\033[0m"
  echo -e "\033[32m  + .   == execute test.abc !\033[0m"
  echo -e "\033[32m  + abc == get test.abc !\033[0m"
  echo -e "\033[32m  + all == all !\033[0m"
  echo -e "\033[32m  + ark == cp tools !\033[0m"
fi

echo
echo "++++++++++++++++++++++++++++++++++++++++"
echo -e "\033[32m=====$@ successful=====\033[0m"

date +%F' '%H:%M:%S
echo "++++++++++++++++++++++++++++++++++++++++"