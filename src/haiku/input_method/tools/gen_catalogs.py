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
import os
from os.path import basename, join, splitext

def exec_linkcatkeys(sig, out_path, input_paths):
    # 'linkcatkeys -o lang.catalog -s sig -l lang lang.catkeys'
    try:
        os.mkdir(out_path)
    except:
        pass
    for input_path in input_paths:
        filename = basename(input_path)
        name, ext = splitext(filename)
        if ext != '.catkeys':
            continue
        locale = name
        args = ['linkcatkeys', '-s', sig, '-l', locale, 
                '-o', join(out_path, locale + '.catalog'), 
                join(input_path)]
        p = subprocess.Popen(args)#, stderr=subprocess.PIPE)
        if not p.returncode is None:
            return p.returncode
        # todo, error
    return 0

def main():
    parser = argparse.ArgumentParser(
                description='Compile rdef file into rsrc file.')
    parser.add_argument('--sig',
                help='Application mime-type signature')
    parser.add_argument('--inputpath', nargs='+',
                help='Directory contains .catkeys')
    parser.add_argument('--outpath', 
                help='rsrc file')
    args = parser.parse_args()
    return exec_linkcatkeys(args.sig, args.outpath, args.inputpath)

if __name__ == '__main__':
    sys.exit(main())
