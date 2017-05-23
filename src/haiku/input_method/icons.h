// Copyright 2010-2016, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _MOZC_ICON_DATA_H
#define _MOZC_ICON_DATA_H

#include <SupportDefs.h>

// 16x16 pixel mode icons for an icon in the deskbar.
// Following data was converted from data which can be found in 
// src/data/images/unix directory.

namespace immozc {

const uchar kUiDirectIconData[] = {
0xff,0xff,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x5c,0x5c,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,
0x98,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x62,0xda,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,
0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x1d,0x1f,0xa5,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0x1f,0xa4,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1e,0x1f,0x82,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x62,0x62,0x1f,0xda,0xa5,0x7d,0x7d,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x1d,0xa4,0x1e,0x1f,0xa5,0x7d,0x7d,0x7d,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0xda,0x1e,0x1f,0xa4,0x7d,0x7d,0x7d,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1e,0xda,0xda,0x1f,0x82,0xa5,0x7d,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x62,0x82,0xa5,0xa5,0x1f,0x62,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0xa5,0x1d,0x83,0xa5,0xa5,0x1d,0x1e,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x83,0x1f,0x82,0xa5,0xa5,0x1e,0x1f,0x82,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0xa5,0x82,0xda,0x82,0xa5,0xa4,0xda,0xda,0x82,0xa5,0xa5,0xa5,0xa5,
0xff,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,
0xff,0xff,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};


const uchar kUiHiraganaIconData[] = {
0xff,0xff,0x98,0x98,0x7d,0x7d,0xa5,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x7d,0x82,0x1e,0x83,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,
0x98,0x98,0x98,0xa5,0xa5,0x82,0x1f,0x83,0x84,0x84,0x83,0x82,0x82,0xa5,0x7d,0x7d,
0x98,0x7d,0x82,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0xda,0xa5,0x7d,0x7d,
0x98,0x7d,0x83,0xda,0xda,0x1c,0x1f,0x1b,0x82,0x62,0x61,0xa4,0xa5,0x7d,0x7d,0x7d,
0x7d,0x7d,0x7d,0xa5,0xa5,0x82,0x1f,0x83,0x83,0x61,0x1f,0x83,0xa5,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0xa5,0x84,0x1c,0x1f,0x1f,0x1f,0x1f,0x1f,0x1c,0x84,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa5,0x82,0x1f,0x1f,0x1f,0x82,0x82,0x1f,0x1c,0x1f,0x1f,0x84,0xa5,0xa5,
0x7d,0x7d,0x84,0x1f,0x1e,0x62,0x1f,0xa4,0x1c,0x1f,0xa4,0x83,0x1f,0x1b,0xa5,0xa5,
0x7d,0x7d,0x62,0x1f,0xa4,0x82,0x1f,0x61,0x1f,0x62,0xa5,0xc5,0x1f,0x1f,0xa5,0xa5,
0x7d,0xa5,0x1f,0x1f,0xc5,0x82,0x1f,0x1f,0x1e,0xa5,0xa5,0x83,0x1f,0x1b,0xa5,0xa5,
0x7d,0xa5,0x1e,0x1f,0x83,0x1b,0x1f,0x1e,0xa4,0xa5,0x82,0x1f,0x1f,0x82,0xa5,0xa5,
0x7d,0x7d,0x82,0x1f,0x1f,0x1f,0x1c,0xa4,0x1b,0x1f,0x1f,0x1f,0x62,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa5,0x83,0x82,0x83,0xa5,0xa5,0x1b,0x1f,0x62,0x84,0xa5,0xa5,0xa5,0xa5,
0xff,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,
0xff,0xff,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};


const uchar kUiFullwidthKatakanaIconData[] = {
0xff,0xff,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x98,0xa5,0xa5,0xa5,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0x7d,0xff,
0x98,0x98,0x83,0x83,0x83,0x83,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x83,0xa5,0x7d,
0x98,0x98,0x61,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x82,0xa5,0x7d,
0x98,0x98,0x83,0x83,0x83,0x82,0x82,0x82,0x82,0x83,0x82,0x1f,0x1f,0x83,0xa5,0x7d,
0x7d,0x7d,0x98,0xa5,0xa5,0xa5,0x82,0x1f,0x83,0xc5,0x83,0x1f,0x1d,0xc5,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x82,0x1f,0x82,0x82,0x1f,0x1f,0xa4,0xa5,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x82,0x1f,0x1f,0x1f,0x1f,0x83,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x82,0x1f,0x1d,0x82,0xa4,0xa5,0x7d,0x7d,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xc5,0x1d,0x1f,0xa4,0xa5,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0xa5,0x82,0x1f,0x1d,0xc5,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa5,0xa4,0x61,0x1f,0x1f,0x83,0xa5,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa4,0x1f,0x1f,0x1f,0x83,0xa5,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa5,0x83,0x61,0xa4,0xa5,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0xff,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,
0xff,0xff,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};


const uchar kUiHalfwidthAlphaIconData[] = {
0xff,0xff,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x5c,0x5c,0x7d,0x7d,0x7d,0xff,
0x98,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xd9,0xda,0xa5,0x7d,0x7d,0x7d,
0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x1d,0x1f,0xa5,0x7d,0x7d,0x7d,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0x1f,0xa4,0x7d,0x7d,0x7d,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1e,0x1f,0x82,0xa5,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xd9,0x82,0x1f,0xda,0xa5,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x1d,0xa4,0x1e,0x1f,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0xda,0x1e,0x1f,0xa4,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1e,0xda,0xda,0x1f,0x82,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0xd9,0x82,0xa5,0xa4,0x1f,0x62,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x1d,0xa4,0xa5,0xa5,0x1d,0x1e,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x83,0x1f,0x82,0xa5,0xa5,0x1e,0x1f,0x82,0xa5,
0x7d,0x7d,0x1e,0x1e,0x1e,0x1e,0xa5,0x82,0xda,0x82,0xa5,0xa4,0xda,0xda,0x82,0xa5,
0xff,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0x1f,
0xff,0xff,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};


const uchar kUiFullwidthAlphaIconData[] = {
0xff,0xff,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x1e,0x1c,0xa5,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,
0x98,0x98,0x98,0x98,0x7d,0x7d,0x83,0x1f,0x1f,0x83,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,
0x98,0x98,0x98,0x7d,0x7d,0x7d,0xda,0x1f,0x1f,0x62,0xa5,0x7d,0x7d,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0x1f,0x1f,0x1f,0xa4,0x7d,0x7d,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x62,0x1f,0xda,0x1f,0x1f,0x82,0xa5,0x7d,0x7d,0x7d,0xa5,
0x7d,0x7d,0x7d,0x7d,0xa5,0x1e,0x62,0xa4,0x1f,0x1f,0x1c,0xa5,0x7d,0x7d,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x83,0x1f,0xa4,0xc5,0x1c,0x1f,0x1f,0x83,0xa5,0x7d,0xa5,0xa5,
0x7d,0x7d,0x7d,0xa5,0xda,0x1f,0xda,0xda,0x1e,0x1f,0x1f,0x62,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0xa4,0x1f,0x1c,0xda,0xda,0xda,0x1e,0x1f,0x1f,0xa4,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x62,0x1f,0xa4,0xa5,0xa5,0xa5,0x82,0x1f,0x1f,0x82,0xa5,0xa5,0xa5,
0x7d,0x7d,0xa5,0x1f,0xda,0xa5,0x7d,0x7d,0x7d,0xa4,0x1f,0x1f,0x1e,0xc5,0xa5,0xa5,
0x7d,0xa4,0x62,0x1f,0x62,0xa4,0x7d,0x7d,0xa5,0xa4,0x1c,0x1f,0x1f,0x62,0xa4,0xa5,
0xa5,0xda,0x1f,0x1f,0x1f,0x82,0xa5,0x7d,0xa5,0xda,0x1f,0x1f,0x1f,0x1f,0x82,0xa5,
0x1e,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xc5,0xc5,0xa5,0xa5,0x1f,
0xff,0xff,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};


const uchar kUiHalfwidthKatakanaIconData[] = {
0xff,0xff,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xff,0xff,
0xff,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0x7d,0xff,
0x98,0x98,0x98,0x98,0x7d,0x7d,0x7d,0x1f,0x1f,0x1f,0x1f,0x1f,0x1e,0x83,0xa5,0xa5,
0x98,0x98,0x98,0x7d,0x7d,0x7d,0x7d,0x83,0x83,0x83,0x83,0x83,0x1b,0x1e,0xa5,0xa5,
0x98,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x83,0xa5,0xa5,0xa4,0x1f,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0xa5,0xa5,0x1c,0x1c,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0xa5,0x82,0x1f,0xa4,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1f,0x1c,0x1f,0x83,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x83,0x1f,0x1c,0x83,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0xd9,0xd9,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa4,0x1f,0x83,0xa5,0x7d,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x82,0x1f,0xa4,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x7d,0x7d,0x7d,0x7d,0xa5,0x83,0x1f,0x82,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0x7d,0x1f,0x1f,0x1f,0x1f,0xa5,0x1f,0x1b,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,
0xff,0x7d,0x7d,0x7d,0x7d,0xa5,0x83,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,
0xff,0xff,0x7d,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xff,0xff,
};

} // namespace immozc

#endif // _MOZC_ICON_DATA_H
