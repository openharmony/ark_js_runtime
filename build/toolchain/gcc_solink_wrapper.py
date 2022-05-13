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
"""Runs 'ld -shared' and generates a .TOC file that's untouched when unchanged.

This script exists to avoid using complex shell commands in
gcc_toolchain.gni's tool("solink"), in case the host running the compiler
does not have a POSIX-like shell (e.g. Windows).
"""

import argparse
import os
import subprocess
import sys
import shutil

import wrapper_utils


def collect_soname(args):
    """Replaces: readelf -d $sofile | grep SONAME"""
    toc = ''
    readelf = subprocess.Popen(wrapper_utils.command_to_run(
        [args.readelf, '-d', args.sofile]),
                               stdout=subprocess.PIPE,
                               bufsize=-1)
    for line in readelf.stdout:
        if b'SONAME' in line:
            toc += line.decode()
    return readelf.wait(), toc


def collect_dyn_sym(args):
    """Replaces: nm --format=posix -g -D $sofile | cut -f1-2 -d' '"""
    toc = ''
    _command = [args.nm]
    if args.sofile.endswith('.dll'):
        _command.append('--extern-only')
    else:
        _command.extend(['--format=posix', '-g', '-D'])
    _command.append(args.sofile)
    nm = subprocess.Popen(wrapper_utils.command_to_run(_command),
                          stdout=subprocess.PIPE,
                          bufsize=-1)
    for line in nm.stdout:
        toc += '{}\n'.format(' '.join(line.decode().split(' ', 2)[:2]))
    return nm.wait(), toc


def collect_toc(args):
    result, toc = collect_soname(args)
    if result == 0:
        result, dynsym = collect_dyn_sym(args)
        toc += dynsym
    return result, toc


def update_toc(tocfile, toc):
    if os.path.exists(tocfile):
        with open(tocfile, 'r') as f:
            old_toc = f.read()
    else:
        old_toc = None
    if toc != old_toc:
        with open(tocfile, 'w') as fp:
            fp.write(toc)


def reformat_rsp_file(rspfile):
    """ Move all implibs from --whole-archive section"""
    with open(rspfile, "r") as fi:
        rspcontent = fi.read()
    result = []
    implibs = []
    naflag = False
    for arg in rspcontent.split(" "):
        if naflag and arg.endswith(".lib"):
            implibs.append(arg)
            continue
        result.append(arg)
        if arg == "-Wl,--whole-archive":
            naflag = True
            continue
        if arg == "-Wl,--no-whole-archive":
            naflag = False
            result.extend(implibs)

    with open(rspfile, "w") as fo:
        fo.write(" ".join(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--readelf',
                        required=True,
                        help='The readelf binary to run',
                        metavar='PATH')
    parser.add_argument('--nm',
                        required=True,
                        help='The nm binary to run',
                        metavar='PATH')
    parser.add_argument('--strip',
                        help='The strip binary to run',
                        metavar='PATH')
    parser.add_argument('--sofile',
                        required=True,
                        help='Shared object file produced by linking command',
                        metavar='FILE')
    parser.add_argument('--tocfile',
                        required=False,
                        help='Output table-of-contents file',
                        metavar='FILE')
    parser.add_argument('--map-file',
                        help=('Use --Wl,-Map to generate a map file. Will be '
                              'gzipped if extension ends with .gz'),
                        metavar='FILE')
    parser.add_argument('--output',
                        required=True,
                        help='Final output shared object file',
                        metavar='FILE')
    parser.add_argument('--libfile', required=False, metavar='FILE')
    parser.add_argument('command', nargs='+', help='Linking command')
    args = parser.parse_args()

    if args.sofile.endswith(".dll"):
        rspfile = None
        for a in args.command:
            if a[0] == "@":
                rspfile = a[1:]
                break
        if rspfile:
            reformat_rsp_file(rspfile)
    # Work-around for gold being slow-by-default. http://crbug.com/632230
    fast_env = dict(os.environ)
    fast_env['LC_ALL'] = 'C'

    # First, run the actual link.
    command = wrapper_utils.command_to_run(args.command)
    result = wrapper_utils.run_link_with_optional_map_file(
        command, env=fast_env, map_file=args.map_file)

    if result != 0:
        return result

    # Next, generate the contents of the TOC file.
    result, toc = collect_toc(args)
    if result != 0:
        return result

    # If there is an existing TOC file with identical contents, leave it alone.
    # Otherwise, write out the TOC file.
    if args.tocfile:
        update_toc(args.tocfile, toc)

    # Finally, strip the linked shared object file (if desired).
    if args.strip:
        result = subprocess.call(
            wrapper_utils.command_to_run(
                [args.strip, '-o', args.output, args.sofile]))
    if args.libfile:
        libfile_name = os.path.basename(args.libfile)
        sofile_output_dir = os.path.dirname(args.sofile)
        unstripped_libfile = os.path.join(sofile_output_dir, libfile_name)
        shutil.copy2(unstripped_libfile, args.libfile)

    return result


if __name__ == "__main__":
    sys.exit(main())
