// Copyright 2010-2018, Google Inc.
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

#include "haiku/haiku_gui/config_dialog/config_dialog.h"
#include "haiku/haiku_gui/dictionary_tool/dictionary_tool.h"
#include "haiku/haiku_gui/word_register_dialog/word_register_dialog.h"
#include "haiku/haiku_gui/error_message_dialog/error_message_dialog.h"
#include "haiku/haiku_gui/about_dialog/about_dialog.h"
#include "haiku/haiku_gui/character_pad/character_palette.h"
#include "haiku/haiku_gui/character_pad/hand_writing.h"


#include "base/flags.h"

DEFINE_string(mode, "about_dialog", "mozc_tool mode");


int main(int argc, char* argv[])
{
    mozc_flags::ParseCommandLineFlags(&argc, &argv, false);

    if (FLAGS_mode == "config_dialog") {
        return HaikuRunConfigDialog(argc, argv);
    } else if (FLAGS_mode == "dictionary_tool") {
        return HaikuRunDictionaryTool(argc, argv);
    } else if (FLAGS_mode == "word_register_dialog") {
        return HaikuRunWordRegisterDialog(argc, argv);
    } else if (FLAGS_mode == "error_message_dialog") {
        return HaikuRunErrorMessageDialog(argc, argv);
    } else if (FLAGS_mode == "about_dialog") {
        return HaikuRunAboutDialog(argc, argv);
    } else if (FLAGS_mode == "character_palette") {
        return HaikuRunCharacterPalette(argc, argv);
    } else if (FLAGS_mode == "hand_writing") {
        return HaikuRunHandWriting(argc, argv);
    }
}
