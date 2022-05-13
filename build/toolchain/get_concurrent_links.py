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

# This script computes the number of concurrent links we want to run
# in the build as a function of machine spec. It's based
# on GetDefaultConcurrentLinks in GYP.

import multiprocessing
import optparse
import os
import re
import subprocess
import sys


def _get_total_memory_in_bytes():
    if sys.platform in ('win32', 'cygwin'):
        import ctypes

        class MEMORYSTATUSEX(ctypes.Structure):
            _fields_ = [
                ("dwLength", ctypes.c_ulong),
                ("dwMemoryLoad", ctypes.c_ulong),
                ("ullTotalPhys", ctypes.c_ulonglong),
                ("ullAvailPhys", ctypes.c_ulonglong),
                ("ullTotalPageFile", ctypes.c_ulonglong),
                ("ullAvailPageFile", ctypes.c_ulonglong),
                ("ullTotalVirtual", ctypes.c_ulonglong),
                ("ullAvailVirtual", ctypes.c_ulonglong),
                ("sullAvailExtendedVirtual", ctypes.c_ulonglong),
            ]

        stat = MEMORYSTATUSEX(dwLength=ctypes.sizeof(MEMORYSTATUSEX))
        ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(stat))
        return stat.ullTotalPhys
    elif sys.platform.startswith('linux'):
        if os.path.exists("/proc/meminfo"):
            with open("/proc/meminfo") as meminfo:
                memtotal_re = re.compile(r'^MemTotal:\s*(\d*)\s*kB')
                for line in meminfo:
                    match = memtotal_re.match(line)
                    if not match:
                        continue
                    return float(match.group(1)) * 2**10
    elif sys.platform == 'darwin':
        try:
            return int(subprocess.check_output(['sysctl', '-n', 'hw.memsize']))
        except Exception:
            return 0
    return 0


def _get_default_concurrent_links(mem_per_link_gb, reserve_mem_gb):
    mem_total_bytes = _get_total_memory_in_bytes()
    mem_total_bytes = max(0, mem_total_bytes - reserve_mem_gb * 2**30)
    num_concurrent_links = int(
        max(1, mem_total_bytes / mem_per_link_gb / 2**30))
    hard_cap = max(1, int(os.getenv('GYP_LINK_CONCURRENCY_MAX', 2**32)))

    try:
        cpu_cap = multiprocessing.cpu_count()
    except: # noqa E722
        cpu_cap = 1

    return min(num_concurrent_links, hard_cap, cpu_cap)


def main():
    parser = optparse.OptionParser()
    parser.add_option('--mem_per_link_gb',
                      action="store",
                      type="int",
                      default=8)
    parser.add_option('--reserve_mem_gb',
                      action="store",
                      type="int",
                      default=0)
    parser.disable_interspersed_args()
    options, _ = parser.parse_args()

    print(
        _get_default_concurrent_links(options.mem_per_link_gb,
                                      options.reserve_mem_gb))
    return 0


if __name__ == '__main__':
    sys.exit(main())
