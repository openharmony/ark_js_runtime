#!/usr/bin/env python3
#coding: utf-8

"""
Copyright (c) 2021-2022 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: run script
    expect_output will get run result,
    expect_sub_output will catch pivotal sub output,
    expect_file will get print string
"""

import argparse
import subprocess
import time


def parse_args():
    """parse arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--script-file', help='execute script file')
    parser.add_argument('--script-options', help='execute script options')
    parser.add_argument('--script-args', help='args of script')
    parser.add_argument('--expect-output', help='expect output')
    parser.add_argument('--expect-sub-output', help='expect sub output')
    parser.add_argument('--expect-file', help='expect file')
    parser.add_argument('--expect-gatetype', help='expect gatetype')
    parser.add_argument('--env-path', help='LD_LIBRARY_PATH env')
    parser.add_argument('--timeout-limit', help='timeout limit')
    args = parser.parse_args()
    return args

def isSubSequence(total, subList):
    idx = 0
    if len(subList) == 1 and subList[0] == '':
        return True
    for obj in total:
        if idx == len(subList):
            return True
        if obj == subList[idx]:
            idx = idx + 1
    return idx == len(subList)

def judge_output(args):
    """run testcase and judge is success or not."""
    start_time = time.time()
    cmd = input_args.script_file
    if input_args.script_options:
        cmd += input_args.script_options
    if input_args.script_args:
        cmd += " " + input_args.script_args
    if input_args.timeout_limit:
        timeout_limit = int(input_args.timeout_limit)
    else:
        timeout_limit = 120  # units: s
    subp = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        env={'LD_LIBRARY_PATH': str(input_args.env_path)})
    try:
        out, err = subp.communicate(timeout=timeout_limit)
    except subprocess.TimeoutExpired:
        subp.kill()
        out, err = subp.communicate()

    if args.expect_output:
        returncode = str(subp.returncode)
        if returncode != args.expect_output:
            out_str = out.decode('UTF-8')
            err_str = err.decode('UTF-8')
            print(out_str)
            print(err_str)
            print(">>>>> Expect return: [" + args.expect_output \
                + "]\n>>>>> But got: [" + returncode + "]")
            raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_sub_output:
        err_str = err.decode('UTF-8')  # log system use std::cerr
        if err_str.find(args.expect_sub_output) == -1:
            out_str = out.decode('UTF-8')
            print(out_str)
            print(">>>>> Expect contain: [" + args.expect_sub_output \
                + "]\n>>>>> But got: [" + err_str + "]")
            raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_file:
        with open(args.expect_file, mode='r') as file:
            # skip license header
            expect_output = ''.join(file.readlines()[13:])
            file.close()
            out_str = out.decode('UTF-8')
            if out_str != expect_output:
                err_str = err.decode('UTF-8')
                print(err_str)
                print(">>>>> Expect : [" + expect_output \
                    + "]\n>>>>> But got: [" + out_str + "]")
                raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_gatetype:
        with open(args.expect_gatetype, mode='r') as file:
            # skip license header
            expect_output = ''.join(file.readlines()[13:])
            file.close()
            err_str = err.decode('UTF-8')
            err_list = []
            for item in err_str.splitlines():
                if "TestInfer:" in item:
                    err_list.append(item.split("&")[1:])
            expect_output = expect_output.replace('\n', '')
            expect_list = [elements.split(",") for elements in expect_output.split("----")]
            for obj1, obj2 in zip(err_list, expect_list):
                if not isSubSequence(obj1, obj2):
                    print(">>>>> Expect :", end = " ")
                    print(obj2)
                    print(">>>>> But got:", end = " ")
                    print(obj1)
                    raise RuntimeError("Run [" + cmd + "] failed!")
    else:
        raise RuntimeError("Run [" + cmd + "] with no expect !")

    print("Run [" + cmd + "] success!")
    print("used: %.5f seconds" % (time.time() - start_time))


if __name__ == '__main__':
    input_args = parse_args()
    judge_output(input_args)
