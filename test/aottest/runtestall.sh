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

expected="expect_output.txt"
generate_mode="no"
run_mode="aot"
test_range=""
run_args=""
out_dir=""
test_dir=$(dirname $0)
cur_dir=$(pwd)
time_string=`date +%Y%m%d%H%M%S`

usage()
{
    echo -e "Usage: runtestall.sh [options]
    Options:
    -mode:aot       run on aot mode, default
    -mode:int       run on interpret mode
    -mode:asmint    run on asm interpret mode
    -debug          run on debug mode
    -v              show version
    -h              print this usage statement"
}

echo_pass()
{
    echo -e "\033[32;2m$1\033[0m"
}

echo_fail()
{
    echo -e "\033[31;2m$1\033[0m"
}

while [ $# -gt 0 ]
do
    case $1 in
        -mode:aot)
            run_mode="aot"
            shift 1 ;;
        -mode:asmint)
            run_mode="asmint"
            shift 1 ;;
        -mode:int)
            run_mode="int"
            shift 1 ;;
        -debug)
            run_args="$run_args -debug"
            shift 1 ;;
        -v)
            tail -n +14 $test_dir/version
            exit 0 ;;
        -h)
            usage
            exit 0 ;;
        -*)
            echo "invalid option $1"
            exit 0;;
        *)
            test_range=$1
            shift 1;;
    esac
done

passed_count=0
failed_count=0
test_package=""
log_file=""
baseline_file=""

if [ ! -d "$cur_dir/ark" ]; then
    echo "Please run at openharmony root dir that ark located"
    exit 0
fi

for test in "$test_dir"/*;
do
    if [ -d "$test" -a -f "$test/$expected" ]; then
        test_name=$(basename $test)
        echo $test_name
        $test_dir/runtest.sh -mode:$run_mode $run_args $test_name
        if [ $? -ne 0 ]; then
            failed_test_array[failed_count]=$test_name
            let failed_count++
        else
            let passed_count++
        fi
    fi
done

echo "PASSED: $passed_count"
echo "FAILED: $failed_count"
if [ $failed_count -eq 0 ]; then
    echo_pass "================================== All tests Run PASSED!"
else
    echo_fail "Run FAILED Cases:"
    echo_fail "=================================="
    for name in ${failed_test_array[@]}; do
        echo $name
    done
    echo_fail "=================================="
fi