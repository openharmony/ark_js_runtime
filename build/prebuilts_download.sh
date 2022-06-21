#!/bin/bash
# Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
for i in "$@"; do
  case "$i" in
    -skip-ssl|--skip-ssl) # wget、npm skip ssl check, which will allow
                          # hacker to get and modify data stream between server and client!
    SKIP_SSL=YES
    ;;
  esac
done

if [ "X${SKIP_SSL}" == "XYES" ];then
    wget_ssl_check='--no-check-certificate'
else
    wget_ssl_check=''
fi

if [ -z "$TOOL_REPO" ];then
	tool_repo='https://repo.huaweicloud.com'
else
	tool_repo=$TOOL_REPO
fi
echo "tool_repo=$tool_repo"

if [ -z "$NPM_REGISTRY" ];then
	npm_registry='http://registry.npm.taobao.org'
else
	npm_registry=$NPM_REGISTRY
fi
echo "npm_registry=$npm_registry"

sha256_result=0
check_sha256=''
local_sha256=''

function check_sha256() {
    success_color='\033[1;42mSuccess\033[0m'
    failed_color='\033[1;41mFailed\033[0m'
    check_url=$1 # source URL
    local_file=$2  # local absolute path
    check_sha256=$(curl -s -k ${check_url}.sha256)
    local_sha256=$(sha256sum ${local_file} |awk '{print $1}')
    if [ "X${check_sha256}" == "X${local_sha256}" ];then
        echo -e "${success_color},${check_url} Sha256 check OK."
        sha256_result=0
    else
        echo -e "${failed_color},${check_url} Sha256 check Failed.Retry!"
        sha256_result=1
    fi
}

function check_sha256_by_mark() {
    success_color='\033[1;42mSuccess\033[0m'
    check_url=$1 # source URL
    check_sha256=$(curl -s -k ${check_url}.sha256)
    echo $1
    if [ -f "${code_dir}/${unzip_dir}/${check_sha256}.mark" ];then
        echo -e "${success_color},${check_url} Sha256 markword check OK."
        sha256_result=0
    else
        echo -e "${check_url} Sha256 mismatch or files not downloaded yet."
        sha256_result=1
    fi
}

function hwcloud_download() {
    # when proxy certfication not required : wget -t3 -T10 -O ${bin_dir} -e "https_proxy=http://domain.com:port" ${huaweicloud_url}
    # when proxy certfication required (special char need URL translate, e.g '!' -> '%21'git
    # wget -t3 -T10 -O ${bin_dir} -e "https_proxy=http://username:password@domain.com:port" ${huaweicloud_url}

    download_local_file=$1
    download_source_url=$2
    for((i=1;i<=3;i++));
    do
        if [ -f "${download_local_file}" ];then
            check_sha256 "${download_source_url}" "${download_local_file}"
            if [ ${sha256_result} -gt 0 ];then
                rm -rf "${download_local_file:-/tmp/20210721_not_exit_file}"
            else
                return 0
            fi
        fi
        if [ ! -f "${download_local_file}" ];then
            wget -t3 -T10 ${wget_ssl_check} -O  "${download_local_file}" "${download_source_url}"
        fi
    done
    # three times error, exit
    echo -e """Sha256 check failed!
Download URL: ${download_source_url}
Local file: ${download_local_file}
Remote sha256: ${check_sha256}
Local sha256: ${local_sha256}"""
    exit 1
}

function npm_install() {
    full_code_path=${code_dir}/$1
    if [ ! -d ${full_code_path} ]; then
        echo "${full_code_path} not exist, it shouldn't happen, pls check..."
    else
        cd ${full_code_path}
        export PATH=${code_dir}/prebuilts/build-tools/common/nodejs/${node_js_name}/bin:$PATH
        npm config set registry ${npm_registry}
        if [ "X${SKIP_SSL}" == "XYES" ];then
            npm config set strict-ssl false
        fi
        # npm cache clean -f
        npm install
    fi
}

function node_modules_copy() {
    full_code_path=${code_dir}/$1
    tool_path=$2
    if [ -d "${full_code_path}" ] & [ ! -z ${tool_path} ]; then
        if [ -d "${code_dir}/${tool_path}" ]; then
            echo -e "\n"
            echo "${code_dir}/${tool_path} already exist, it will be replaced with node-${node_js_ver}"
            /bin/rm -rf ${code_dir}/${tool_path}
            echo -e "\n"
        fi
        mkdir -p ${code_dir}/${tool_path}
        /bin/cp -R ${full_code_path}/node_modules ${code_dir}/${tool_path}
    fi
}

case $(uname -s) in
    Linux)
        host_platform=linux
        ;;
    Darwin)
        host_platform=darwin
        ;;
    *)
        echo "Unsupported host platform: $(uname -s)"
        exit 1
esac

# sync code directory
script_path=$(cd $(dirname $0);pwd)
code_dir=$(dirname ${script_path})/..
# "prebuilts" directory will be generated under project root which is used to saved binary (arround 9.5GB)
# downloaded files will be automatically unziped under this path
bin_dir=${code_dir}/OpenHarmony_2.0_canary_prebuilts

# download file list
# todo: add product related config

copy_config="""
prebuilts/build-tools/common,${tool_repo}/openharmony/compiler/restool/2.007/restool-2.007.tar.gz
prebuilts/build-tools/${host_platform}-x86/bin,${tool_repo}/openharmony/compiler/gn/1717/${host_platform}/gn-${host_platform}-x86-1717.tar.gz
prebuilts/build-tools/${host_platform}-x86/bin,${tool_repo}/openharmony/compiler/ninja/1.10.1/${host_platform}/ninja-${host_platform}-x86-1.10.1.tar.gz
prebuilts/clang/ohos/${host_platform}-x86_64,${tool_repo}/openharmony/compiler/clang/10.0.1-480513/${host_platform}/clang-480513-${host_platform}-x86_64.tar.bz2
prebuilts/ark_tools,${tool_repo}/openharmony/compiler/llvm_prebuilt_libs/ark_js_prebuilts_20220425.tar.gz
"""

linux_copy_config="""
prebuilts/clang/ohos/${host_platform}-x86_64,${tool_repo}/openharmony/compiler/clang/10.0.1-480513/${host_platform}/libcxx-ndk-480513-${host_platform}-x86_64.tar.bz2
"""

darwin_copy_config="""
prebuilts/previewer/darwin,${tool_repo}/openharmony/develop_tools/previewer/3.1.5.6/previewer-3.1.5.6.mac.tar.gz
prebuilts/clang/ohos/${host_platform}-x86_64,${tool_repo}/openharmony/compiler/clang/10.0.1-480513/${host_platform}/libcxx-ndk-480513-${host_platform}-x86_64.tar.bz2
prebuilts/python,${tool_repo}/openharmony/compiler/python/3.9.2/${host_platform}/python-${host_platform}-x86-3.9.2_202205071615.tar.gz
"""

if [[ "${host_platform}" == "linux" ]]; then
    copy_config+=${linux_copy_config}
elif [[ "${host_platform}" == "darwin" ]]; then
    copy_config+=${darwin_copy_config}
fi

# download and initialize prebuild files
if [ ! -d "${bin_dir}" ];then
    mkdir -p "${bin_dir}"
fi

for i in $(echo ${copy_config})
do
    unzip_dir=$(echo $i|awk -F ',' '{print $1}')
    huaweicloud_url=$(echo $i|awk -F ',' '{print $2}')
    md5_huaweicloud_url=$(echo ${huaweicloud_url}|md5sum|awk '{print $1}')
    bin_file=$(basename ${huaweicloud_url})
    bin_file_suffix=${bin_file#*.}
    #huaweicloud_file_name=$(echo ${huaweicloud_url}|awk -F '/' '{print $NF}')

    if [ ! -d "${code_dir}/${unzip_dir}" ];then
        mkdir -p "${code_dir}/${unzip_dir}"
    fi

    check_sha256_by_mark ${huaweicloud_url}
    if [ ${sha256_result} -gt 0 ]; then
        hwcloud_download "${bin_dir}/${md5_huaweicloud_url}.${bin_file}"  "${huaweicloud_url}"
        if [ "X${bin_file:0-3}" = "Xzip" ];then
            unzip -o "${bin_dir}/${md5_huaweicloud_url}.${bin_file}" -d "${code_dir}/${unzip_dir}/"
        elif [ "X${bin_file:0-6}" = "Xtar.gz" ];then
            tar -xvzf "${bin_dir}/${md5_huaweicloud_url}.${bin_file}"  -C  "${code_dir}/${unzip_dir}"
        else
            tar -xvf "${bin_dir}/${md5_huaweicloud_url}.${bin_file}"  -C  "${code_dir}/${unzip_dir}"
        fi
        # it is used to handle some redundant files under prebuilts path
        # todo: remove redundant files before prebuilts_download
        if [ -d "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/prebuilts_gcc_linux-x86_arm_gcc-linaro-7.5.0-arm-linux-gnueabi" ];then
            mv "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/prebuilts_gcc_linux-x86_arm_gcc-linaro-7.5.0-arm-linux-gnueabi" "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi2/"
            rm -rf "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi"
            mv "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi2/" "${code_dir}/prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/"
        fi
        if [ -d "${code_dir}/prebuilts/clang/ohos/windows-x86_64/clang-480513" ];then
            rm -rf "${code_dir}/prebuilts/clang/ohos/windows-x86_64/llvm"
            mv "${code_dir}/prebuilts/clang/ohos/windows-x86_64/clang-480513" "${code_dir}/prebuilts/clang/ohos/windows-x86_64/llvm"
            ln -snf 10.0.1 "${code_dir}/prebuilts/clang/ohos/windows-x86_64/llvm/lib/clang/current"
        fi
        if [ -d "${code_dir}/prebuilts/clang/ohos/linux-x86_64/clang-480513" ];then
            rm -rf "${code_dir}/prebuilts/clang/ohos/linux-x86_64/llvm"
            mv "${code_dir}/prebuilts/clang/ohos/linux-x86_64/clang-480513" "${code_dir}/prebuilts/clang/ohos/linux-x86_64/llvm"
            ln -snf 10.0.1 "${code_dir}/prebuilts/clang/ohos/linux-x86_64/llvm/lib/clang/current"
        fi
        if [ -d "${code_dir}/prebuilts/clang/ohos/darwin-x86_64/clang-480513" ];then
            rm -rf "${code_dir}/prebuilts/clang/ohos/darwin-x86_64/llvm"
            mv "${code_dir}/prebuilts/clang/ohos/darwin-x86_64/clang-480513" "${code_dir}/prebuilts/clang/ohos/darwin-x86_64/llvm"
            ln -snf 10.0.1 "${code_dir}/prebuilts/clang/ohos/darwin-x86_64/llvm/lib/clang/current"
        fi
        if [ -d "${code_dir}/prebuilts/gcc/linux-x86/esp/esp-2019r2-8.2.0/xtensa-esp32-elf" ];then
            chmod 755 "${code_dir}/prebuilts/gcc/linux-x86/esp/esp-2019r2-8.2.0" -R
        fi
        echo 0 > "${code_dir}/${unzip_dir}/${check_sha256}.mark"
    fi
    echo "k"
done


# npm env setup and install

node_js_ver=v12.18.4
node_js_name=node-${node_js_ver}-${host_platform}-x64
node_js_pkg=${node_js_name}.tar.gz
mkdir -p ${code_dir}/prebuilts/build-tools/common/nodejs
cd ${code_dir}/prebuilts/build-tools/common/nodejs
if [ ! -f "${node_js_pkg}" ]; then
    wget -t3 -T10 ${wget_ssl_check} ${tool_repo}/nodejs/${node_js_ver}/${node_js_pkg}
    tar zxf ${node_js_pkg}
fi

npm_install_config="""
ts2abc/ts2panda,prebuilts/build-tools/common/ts2abc
"""

for i in $(echo ${npm_install_config})
do
    code_path=$(echo $i|awk -F ',' '{print $1}')
    modules_path=$(echo $i|awk -F ',' '{print $2}')
    npm_install ${code_path}
    echo ${code_path}
    echo ${modules_path}
    node_modules_copy ${code_path} ${modules_path}
done

cd ${code_dir}
echo -e "\n"
