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

import argparse

class Catkeys:
    def __init__(self, path):
        self.path = path
        with open(path, "r") as f:
            lines = f.readlines()
        if not lines:
            raise Exception('no data: ' + path)
        data = {}
        for line in lines[1:]: # ignore first line
            parts = line.split("\t")
            if len(parts) == 4:
                # original \t context \t comment \t translated
                data[(parts[0], parts[1])] = parts[3].rstrip()
        self.data = data
        self.header = lines[0].rstrip()

    def get(self, key):
        return self.data.get(key, None)
    
    def set(self, key, value):
        self.data[key] = value

    def write(self, path):
        with open(path, "w") as f:
            f.write(self.header)
            f.write("\n")
            for key, value in sorted(self.data.items()):
                f.write("{}\t{}\t\t{}\n".format(key[0], key[1], value))

def copy(en, src, dest):
    csrc = Catkeys(src)
    cdest = Catkeys(en)
    
    for key, value in cdest.data.items():
        v = csrc.get(key)
        if v:
            cdest.set(key, v)
    
    cdest.write(dest)
    return 0

def main():
    parser = argparse.ArgumentParser(
                description='Copy translated data into new file')
    parser.add_argument('--cat',
                help='en.catkeys')
    parser.add_argument('--src',
                help='File to read')
    parser.add_argument('--dest',
                help='File to be written')
    args = parser.parse_args()
    return copy(args.cat, args.src, args.dest)

if __name__ == '__main__':
    main()
