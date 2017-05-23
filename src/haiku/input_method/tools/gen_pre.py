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

import sys
import argparse
import subprocess

def gen_pre(parser, includes, outpath, input_paths):
    inc = ["-I" + i for i in includes]
    data = []
    args = [parser, '-E', '-x', 'c++', 
            '-DB_COLLECTING_CATKEYS', '-DOS_LINUX', '-DOS_HAIKU']
    args.extend(inc)
    args.extend(input_paths)
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.poll() is None:
        out = p.stdout.read()
        data.extend([line for line in out.split("\n") 
                        if not line.startswith("#")])
    else:
        print(p.stderr.read())
        return -1
    with open(outpath, "w") as f:
        f.write("\n".join(data))
    return 0

def main():
    parser = argparse.ArgumentParser(
                description='Generates pre file from B_TRANSLATE.')
    parser.add_argument('--parser',
                help='C compiler')
    parser.add_argument('--outpath', 
                help='pre file')
    parser.add_argument('--includes', nargs='+',
                help='Include dirs')
    parser.add_argument('--inputpath', nargs='+',
                help='C++ files to be extracted')
    args = parser.parse_args()
    return gen_pre(args.parser, args.includes, args.outpath, args.inputpath)

if __name__ == '__main__':
    sys.exit(main())
