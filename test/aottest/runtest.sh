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

expect_output="expect_output.txt"
run_output="run_output.txt"
test_name=""
test_dir=$(dirname $0)
product="hispark_taurus"
run_args=""
run_mode="aot"
make_opt=""
cur_dir=$(pwd)
timeout=20

check_result_fexit()
{
    if [ $? -ne 0 ]; then
        echo -e "\033[31;2m[error] $1\033[0m"
        exit 1
    elif [ -n "$2" ]; then
        echo -e "$2"
    fi
}

check_failed_exit()
{
    ret1=$(grep 'FAILED' $1)
    ret2=$(grep 'failed' $1)
    ret3=$(grep 'FAULT' $1)
    ret4=$(grep 'fault' $1)
    if [ -n "$ret1" -o -n "$ret2" -o -n "$ret3" -o -n "$ret4" ]; then
        echo -e "\033[31;2m[error] $2\033[0m"
        exit 1
    fi
}

echo_pass()
{
    echo -e "\033[32;2m$1\033[0m"
}

echo_fail()
{
    echo -e "\033[31;2m[error] $1\033[0m"
}

run_check()
{
    timeout $@
    
    # FIXME: run result can not be checked by $? when run unexpectedly exit, such as segmentation fault
    ret=0
    ret=$?
    if [ $ret -eq 124 -o $ret -eq 142 ]; then
        echo_fail "Run timeout, be killed!"
        exit 1
    elif [ $ret -ne 0 ]; then
        echo_fail "Run FAILED!"
        exit 1
    fi
}

usage()
{
    echo -e "Usage: runtest.sh [options] test_name
    Options:
    -mode [opt]     run mode option: aot, int(interpret mode), asmint(asm interpret mode)
    -make [opt]     pass option to make, supported opt: abc, aot, aotd, run, rund, int, intd, asmint, asmintd
    -debug          run on debug mode
    -timeout n      specify seconds of test timeout, n > 0
    -v              show version
    -h              print this usage statement"
}

while [ $# -gt 0 ]
do
    case $1 in
        -mode)
            run_mode=$2
            shift 2 ;;
        -make)
            make_opt=$2
            shift 2 ;;
        -debug)
            run_args="$run_args debug=yes"
            shift 1 ;;
        -arm)
            product="rk3568"
            run_args="$run_args arm=yes"
            shift 1 ;;
        -timeout)
            timeout=$2
            shift 2 ;;
        -v)
            tail -n +14 $test_dir/version
            exit 0 ;;
        -h)
            usage
            exit 0 ;;
        -*)
            echo "invalid option $1"
            exit 0 ;;
        *)
            test_name="$1"
            shift 1 ;;
    esac
done

if [ ! -d "$cur_dir/ark" ]; then
    echo "Please run at openharmony root dir that ark located"
    exit 0
fi

# run test
test_name=$(basename $test_name)
echo "Run test: $test_dir/$test_name ================="

out_dir="out/$product/clang_x64/aottest"
if [ ! -f "$out_dir/stub.m" ]; then
    make -f $test_dir/makefile $run_args stub
    check_result_fexit "make stub.m FAILED"
fi

module=""
if [ -n "$(grep 'import' $(ls $test_dir/$test_name/$test_name.[tj]s))" ]; then
    module="module=yes"
fi

make_cmd="make -f $test_dir/makefile $run_args test=$test_name $module"

if [ -n "$make_opt" ]; then
    $make_cmd $make_opt
    check_result_fexit "make $make_opt FAILED"
    exit 0
fi

$make_cmd -n abc
run_check $timeout $make_cmd -s abc

case "$run_mode" in
    "aot")
        $make_cmd -n aot
        run_check $timeout $make_cmd -s aot
        $make_cmd -n run
        run_check $timeout $make_cmd -s run > $out_dir/$test_name/$run_output
        ;;
    "int")
        $make_cmd -n int
        run_check $timeout $make_cmd -s int > $out_dir/$test_name/$run_output
        ;;
    "asmint")
        $make_cmd -n asmint
        run_check $timeout $make_cmd -s asmint > $out_dir/$test_name/$run_output
        ;;
esac

tail -n +14 $test_dir/$test_name/$expect_output > $out_dir/$test_name/$expect_output
diff $out_dir/$test_name/$run_output $out_dir/$test_name/$expect_output

check_result_fexit "Test Case FAILED!"
echo_pass "======================= Test Case PASSED!"