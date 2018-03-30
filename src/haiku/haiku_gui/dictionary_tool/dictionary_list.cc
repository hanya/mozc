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

#include "haiku/haiku_gui/dictionary_tool/dictionary_list.h"

#include <Looper.h>

namespace mozc {
namespace haiku_gui {

DicList::DicList(const char* name, BLooper* looper, BMessage* menuRequest)
    : BListView(name, B_SINGLE_SELECTION_LIST),
      mpLooper(looper),
      mpMenuRequestMessage(menuRequest)
{
}

void DicList::MouseDown(BPoint pos)
{
    BListView::MouseDown(pos);
    uint32 buttons;
    GetMouse(&pos, &buttons, false);
    if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        if (mpLooper && mpMenuRequestMessage) {
            BMessage message(*mpMenuRequestMessage);
            message.AddPoint("pos", pos);
            mpLooper->PostMessage(&message);
        }
    }
}

void DicList::repaint()
{
    if (LockLooper()) {
        Invalidate();
        UnlockLooper();
    }
}


DicItem::DicItem(const char* label, uint64 id)
    : BStringItem(label),
      dic_id_(id)
{
}

void DicItem::setText(const char* text)
{
    SetText(text);
}

} // haiku_gui
} // mozc
