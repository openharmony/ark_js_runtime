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
Description: Copy test resource file to correct path
"""

import os
import argparse
import shutil


def parse_args():
    """parse arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--src-path', help='test xml source path')
    parser.add_argument('--src-xml', help='test xml source file')
    parser.add_argument('--dst-path', help='the copy dest path')
    args = parser.parse_args()
    return args


def copy_xml(args):
    """copy resource xml to test direction."""
    src_xml_file = os.path.join(args.src_path, args.src_xml)
    dst_xml_file = os.path.join(args.dst_path, args.src_xml)

    if not os.path.isfile(src_xml_file):
        print(args.src_xml + " not exist.")
        return

    if not os.path.exists(args.dst_path):
        os.makedirs(args.dst_path)

    shutil.copyfile(src_xml_file, dst_xml_file)


if __name__ == '__main__':
    input_args = parse_args()
    copy_xml(input_args)
