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

def gen_catkeys(sig, out_path, input_path):
    # collectcatkeys -s sig -o en.catkeys data.pre
    args = ['collectcatkeys', '-s', sig, '-o', out_path, input_path]
    p = subprocess.Popen(args, stderr=subprocess.PIPE)
    if p.poll():
        e = p.stderr.readline()
        print(e)
        
    return p.returncode

def main():
    parser = argparse.ArgumentParser(
                description='Generates en.catkeys file from pre.')
    parser.add_argument('--sig',
                help='Application mime type signature')
    parser.add_argument('--outpath', 
                help='en.catkeys')
    parser.add_argument('--inputpath',
                help='Merged C++ files as .pre')
    args = parser.parse_args()
    return gen_catkeys(args.sig, args.outpath, args.inputpath)

if __name__ == '__main__':
    sys.exit(main())
