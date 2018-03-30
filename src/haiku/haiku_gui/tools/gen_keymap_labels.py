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
import xml.sax
import xml.sax.handler
from os.path import basename


class TSHandler(xml.sax.handler.ContentHandler):
    def __init__(self, context):
        self.TYPE_SOURCE = 1
        self.TYPE_TRANSLATION = 2
        self.messages = {}
        self.source = ""
        self.translation = ""
        self.type = None
        self.context = context
    
    def startElement(self, name, attrs):
        if name == "source":
            self.type = self.TYPE_SOURCE
        elif name == "translation":
            self.type = self.TYPE_TRANSLATION
    
    def endElement(self, name):
        if name == "message":
            if self.source and self.translation:
                key = self.source.replace("\n", "\\n").replace("\"", "\\\"")
                data = self.translation.replace("\n", "\\n").replace("\"", "\\\"").encode("utf-8")
                self.messages[(key, self.context)] = data
            self.source = ""
            self.translation = ""
            self.type = None
        elif name == "source":
            self.type = ""
        elif name == "translation":
            self.type = ""
    
    def characters(self, content):
        if self.type == self.TYPE_SOURCE and content:
            if self.source:
                self.source = self.source + content
            else:
                self.source = content
        elif self.type == self.TYPE_TRANSLATION and content:
            if self.translation:
                self.translation = self.translation + content
            else:
                self.translation = content


def update(src, ts):
    handler = TSHandler("keymap")
    try:
        xml.sax.parse(ts, handler)
    except:
        return
    labels = []
    for key, value, in handler.messages.items():
        labels.append((key[0], value))
    labels.sort()
    
    for label in labels:
        print("    B_TRANSLATE(\"{}\"),".format(label[0]))


def main():
    parser = argparse.ArgumentParser(
                description='Generate file which contains keymap labels')
    parser.add_argument('--dest',
                help='dest file')
    parser.add_argument('--ts',
                help='ts file')
    args = parser.parse_args()
    update(args.dest, args.ts)

if __name__ == '__main__':
    sys.exit(main())
