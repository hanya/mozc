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
import os.path
import time

class Catkeys:
    def __init__(self):
        self.contexts = {}

    def add(self, context, key, value):
        try:
            c = self.contexts[context]
        except:
            c = []
            self.contexts[context] = c
        c.append((key, value))

    def get_contexts(self):
        for context in sorted(self.contexts.keys()):
            c = self.contexts[context]
            for v in sorted(c):
                yield v

def gen_catkeys(sig, out_path, input_path):
    # collectcatkeys -s sig -o en.catkeys data.pre
    args = ['collectcatkeys', '-s', sig, '-o', out_path, input_path]
    p = subprocess.Popen(args, stderr=subprocess.PIPE)
    while True:
        if p.poll():
            print(p.stderr.readline())
            return p.returncode
        else:
            break

    # sort entries
    file_path = os.path.abspath(out_path)

    time.sleep(0.01)
    n = 0
    while n < 10:
        try:
            with open(file_path, "r") as f:
                s = f.read()
                break
        except:
            n += 1
            time.sleep(0.01)
    if n == 10:
        return p.returncode

    lines = s.split("\n")
    catkeys = Catkeys()
    for line in lines[1:]:
        parts = line.split("\t")
        if len(parts) > 2:
            catkeys.add(parts[1], parts[0], line)

    data = [lines[0]]
    for k, v in catkeys.get_contexts():
        data.append(v)
    with open(file_path, "w") as f:
        f.write("\n".join(data))
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
