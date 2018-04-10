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

#ifndef COMMON_H_
#define COMMON_H_

#define MOZC_BACKEND_SIG "application/x-vnd.Mozc-Task"

namespace immozc {

// Special keys defined by custom keymap.
enum {
    // 0x6c
    K_MUHENKAN = 0xf4,
    // 0x6d
    K_HENKAN = 0xf5,
    // 0x3b Eisu, +Shift -> CapsLock
    K_EISU = 0xf6,
    K_CAPS_LOCK = 0xf7,
};


enum {
    IM_CANDIDATE_WINDOW_SHOW = 'IMcs',
    IM_CANDIDATE_WINDOW_HIDE = 'IMch',
    
    IM_METHOD_ACTIVATED = 'IMac',
    IM_METHOD_DEACTIVATED = 'IMda',
    
    IM_RELOAD = 'IMrl',
    
    IM_MODE_CHANGE_REQUEST = 'IMmr',
    IM_MODE_CHANGED = 'IMmc',
    
    IM_BAR_SHOW = 'IMbs',
    IM_BAR_HIDE = 'IMbh',
    IM_BAR_HORIZONTAL = 'IMoh',
    IM_BAR_VERTICAL = 'IMov',
    IM_BAR_MOVED = 'IMmv',
    IM_BAR_ORIENTATION_CHANGED = 'IMdr',
    IM_BAR_SHOW_PERMANENT = 'IMsp',
    IM_BAR_HIDE_PERMANENT = 'IMhp',
    
    IM_MODE = 'IMmd',
    IM_TOOL = 'IMtl',
    
    IM_SELECT_CANDIDATE = 'IMsc',
    IM_HIGHLIGHT_CANDIDATE = 'IMhc',
    
    IM_KANA_MAPPING_MSG = 'IMkm',
};

#define DESKBAR         "deskbar"
#define IM_ACTIVE       "active"
#define IM_MODE_MODE    "mode"

enum IM_Mode {
    MODE_DIRECT = 1,
    MODE_HIRAGANA = 2,
    MODE_FULLWIDTH_KATAKANA,
    MODE_HALFWIDTH_ASCII,
    MODE_FULLWIDTH_ASCII,
    MODE_HALFWIDTH_KATAKANA,
    
    MODE_END,
};


#define MOZC_TOOL_TOOL    "tool"

enum Mozc_Tool {
    TOOL_ABOUT = 1,
    TOOL_CHARACTER_PAD = 2,
    TOOL_CONFIG,
    TOOL_DICTIONARY,
    TOOL_HAND_WRITING,
    TOOL_WORD_REGISTER,
    
    TOOL_END,
};

#define IM_KANA_MAPPING_VALUE     "kana.map"

enum IM_Kana_Mapping {
    KANA_MAPPING_CUSTOM,
    KANA_MAPPING_JP,
    KANA_MAPPING_US,

    KANA_MAPPING_END,
};

} // namespace immozc

#endif // COMMON_H_
