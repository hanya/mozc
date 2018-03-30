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

#ifndef CHARACTER_WINDOW_H
#define CHARACTER_WINDOW_H

#include "haiku/haiku_gui/base/cstring_view.h"

#include <Window.h>

class BMenuField;
class BMessageRunner;
class BPopUpMenu;
class BStringView;

namespace mozc {
namespace haiku_gui {

class CharacterList;
class ToolTipWindow;

class CharacterWindow : public BWindow
{
public:
    CharacterWindow(BRect rect, const char* name);
    virtual ~CharacterWindow();

    virtual void MessageReceived(BMessage *msg);
protected:
    enum Actions
    {
        FONT_SIZE_CHANGED = 'fsch',
        FONT_CHANGED = 'fnch',
        CHAR_SELECTION_CHANGED = 'chsc',
        CLEAR_STATUS = 'clst',
        ON_OVER_CHANGED = 'ooch',
    };

    BMenuField*         fFontMF;
    BPopUpMenu*         fFontMenu;
    BMenuField*         fFontSizeMF;
    BPopUpMenu*         fFontSizeMenu;
    CharacterList*      fCharacterList;
    CStringView*        fStatusText;
    BMessageRunner*     fClearStatusRunner;
    ToolTipWindow*      fToolTipWindow;

    void CopyToClipboard(const char* s);
    void SetStatusText(const char* message);
    void _ToolTipChanged(BMessage* msg);
};

}; // haiku_gui
}; // mozc

#endif // CHARACTER_WINDOW_H
