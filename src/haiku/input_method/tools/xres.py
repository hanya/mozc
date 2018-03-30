# Copyright 2010-2016, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import stat
import sys
import argparse
import subprocess
import shutil

def exec_xres(input_path, out_path, rsrc, make_executable=False):
    # copy 
    shutil.copyfile(input_path, out_path)
    
    # merge
    # 'xres -o out input.rsrc'
    args = ['xres', '-o', out_path, rsrc]
    p = subprocess.Popen(args, stderr=subprocess.PIPE)
    if p.returncode is None and make_executable:
        os.chmod(out_path,
            os.stat(out_path).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    return p.returncode

def main():
    parser = argparse.ArgumentParser(
                description='Merge rsrc file into executable file.')
    parser.add_argument('--outpath', 
                help='executable file with resource')
    parser.add_argument('--inputpath',
                help='input executable file')
    parser.add_argument('--rsrc',
                help='rsrc file')
    parser.add_argument('--exe', action='store_true', default=False,
                help='set executable mode')
    args = parser.parse_args()
    return exec_xres(args.inputpath, args.outpath, args.rsrc, args.exe)

if __name__ == '__main__':
    sys.exit(main())
