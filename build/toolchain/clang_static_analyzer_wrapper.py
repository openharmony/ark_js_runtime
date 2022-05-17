#!/usr/bin/env python
# -*- coding: utf-8 -*-
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

"""Adds an analysis build step to invocations of the Clang C/C++ compiler.

Usage: clang_static_analyzer_wrapper.py <compiler> [args...]
"""

import argparse
import sys
import wrapper_utils

# Flags used to enable analysis for Clang invocations.
analyzer_enable_flags = [
    '--analyze',
]

# Flags used to configure the analyzer's behavior.
analyzer_option_flags = [
    '-fdiagnostics-show-option',
    '-analyzer-checker=cplusplus',
    '-analyzer-opt-analyze-nested-blocks',
    '-analyzer-eagerly-assume',
    '-analyzer-output=text',
    '-analyzer-config',
    'suppress-c++-stdlib=true',

    # List of checkers to execute.
    # The full list of checkers can be found at
    # https://clang-analyzer.llvm.org/available_checks.html.
    '-analyzer-checker=core',
    '-analyzer-checker=unix',
    '-analyzer-checker=deadcode',
]


# Prepends every element of a list |args| with |token|.
# e.g. ['-analyzer-foo', '-analyzer-bar'] => ['-Xanalyzer', '-analyzer-foo',
#                                             '-Xanalyzer', '-analyzer-bar']
def interleave_args(args, token):
    return list(sum(zip([token] * len(args), args), ()))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mode',
                        choices=['clang', 'cl'],
                        required=True,
                        help='Specifies the compiler argument convention '
                             'to use.')
    parser.add_argument('args', nargs=argparse.REMAINDER)
    parsed_args = parser.parse_args()

    prefix = '-Xclang' if parsed_args.mode == 'cl' else '-Xanalyzer'
    cmd = parsed_args.args + analyzer_enable_flags + interleave_args(
        analyzer_option_flags, prefix)
    returncode, stderr = wrapper_utils.capture_command_stderr(
        wrapper_utils.command_to_run(cmd))
    sys.stderr.write(stderr)

    return_code, stderr = wrapper_utils.capture_command_stderr(
        wrapper_utils.command_to_run(parsed_args.args))
    sys.stderr.write(stderr)

    return return_code


if __name__ == '__main__':
    sys.exit(main())
