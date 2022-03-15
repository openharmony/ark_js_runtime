#!/usr/bin/env python3
#coding: utf-8

"""
Copyright (c) 2021 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

"""
Description: run script
    expect_output will get run result, expect_file will get print string
"""

import argparse
import subprocess
import time


def parse_args():
    """parse arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--script-file', help='execute script file')
    parser.add_argument('--script-args', help='args of script')
    parser.add_argument('--expect-output', help='expect output')
    parser.add_argument('--expect-file', help='expect file')
    parser.add_argument('--env-path', help='LD_LIBRARY_PATH env')
    args = parser.parse_args()
    return args


def judge_output(args):
    """run testcase and judge is success or not."""
    start_time = time.time()
    cmd = input_args.script_file
    if input_args.script_args:
        cmd += " " + input_args.script_args
    subp = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
        env={'LD_LIBRARY_PATH': str(input_args.env_path)})
    try:
        out, err = subp.communicate(timeout=300) # units: s
    except subprocess.TimeoutExpired:
        subp.kill()
        out, err = subp.communicate()

    if args.expect_output:
        returncode = str(subp.returncode)
        if returncode != args.expect_output:
            print_str = out.decode('UTF-8')
            print(print_str)
            raise RuntimeError("Run [" + cmd + "] failed!")
    elif args.expect_file:
        with open(args.expect_file, mode='r') as file:
            # skip license header
            expect_output = ''.join(file.readlines()[13:])
            file.close()
            print_str = out.decode('UTF-8')
            if print_str != expect_output:
                raise RuntimeError("\n>>>>> Expect : [" + expect_output \
                    + "]\n>>>>> But got: [" + print_str + "]")
    else:
        raise RuntimeError("Run [" + cmd + "] with no expect !")

    print("Run [" + cmd + "] success!")
    print("used: %.5f seconds" % (time.time() - start_time))


if __name__ == '__main__':
    input_args = parse_args()
    judge_output(input_args)
