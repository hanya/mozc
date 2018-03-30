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

#include "haiku/haiku_gui/character_pad/character_window.h"
#include "haiku/haiku_gui/character_pad/character_list.h"
#include "haiku/haiku_gui/character_pad/tooltip_window.h"
#include "haiku/haiku_gui/character_pad/unicode_util.h"

#include <Catalog.h>
#include <Clipboard.h>
#include <Font.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <StringView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "character_pad"

namespace mozc {
namespace haiku_gui {

const bigtime_t CLAER_STATUS_TIMER = 5000000; // 5 sec

CharacterWindow::CharacterWindow(BRect rect, const char* name)
    : BWindow(
        rect,
        name,
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_QUIT_ON_WINDOW_CLOSE | B_CLOSE_ON_ESCAPE)
{
    fCharacterList = NULL;
    fClearStatusRunner = NULL;
    fToolTipWindow = new ToolTipWindow();
    
    fFontMenu = new BPopUpMenu("fontMenu");
    fFontMenu->SetRadioMode(true);
    {
        BFont font;
        fFontMenu->GetFont(&font);
        font_family family;
        font_style style;
        font.GetFamilyAndStyle(&family, &style);

        int32 fonts = count_font_families();
        for (int32 i = 0; i < fonts; ++i) {
            uint32 flags;
            font_family name;
            if (get_font_family(i, &name, &flags) == B_OK) {
                BMenuItem* item = new BMenuItem(name, new BMessage(FONT_CHANGED));
                if (strcmp(name, family) == 0) {
                    item->SetMarked(true);
                }
                fFontMenu->AddItem(item);
            }
        }
    }

    fFontMF = new BMenuField("fontMF", "", fFontMenu);

    fFontSizeMenu = new BPopUpMenu("fontSizeMenu");
	fFontSizeMenu->SetRadioMode(true);
	fFontSizeMenu->AddItem(new BMenuItem(B_TRANSLATE("Largest"), new BMessage(FONT_SIZE_CHANGED)));
    fFontSizeMenu->AddItem(new BMenuItem(B_TRANSLATE("Larger"), new BMessage(FONT_SIZE_CHANGED)));
	fFontSizeMenu->AddItem(new BMenuItem(B_TRANSLATE("Medium"), new BMessage(FONT_SIZE_CHANGED)));
	fFontSizeMenu->AddItem(new BMenuItem(B_TRANSLATE("Smaller"), new BMessage(FONT_SIZE_CHANGED)));
    fFontSizeMenu->AddItem(new BMenuItem(B_TRANSLATE("Smallest"), new BMessage(FONT_SIZE_CHANGED)));
    fFontSizeMenu->ItemAt(fFontSizeMenu->CountItems()-1)->SetMarked(true);

    fFontSizeMF = new BMenuField("fontSizeMF", "", fFontSizeMenu);

    fStatusText = new CStringView("statusL", "");
}

CharacterWindow::~CharacterWindow()
{
    delete fClearStatusRunner;
    if (fToolTipWindow->Lock()) {
        fToolTipWindow->Quit();
    }
}

void CharacterWindow::CopyToClipboard(const char* s)
{
    if (be_clipboard->Lock()) {
        be_clipboard->Clear();
        BMessage* clip = be_clipboard->Data();
        clip->AddData("text/plain", B_MIME_TYPE, s, strlen(s));
        int32 status = be_clipboard->Commit();
        be_clipboard->Unlock();
        if (status == B_OK) {
            std::string v(s);
            v += B_TRANSLATE(" is sent to clipboard");
            SetStatusText(v.c_str());
        }
    }
}

void CharacterWindow::SetStatusText(const char* message)
{
    fStatusText->SetText(message);
    if (strlen(message) == 0) {
        delete fClearStatusRunner;
        fClearStatusRunner = NULL;
        fClearStatusRunner = new BMessageRunner(this, new BMessage(CLEAR_STATUS),
                                                CLAER_STATUS_TIMER, 1);
    }
}

void CharacterWindow::_ToolTipChanged(BMessage* msg)
{
    const char* c;
    if (msg->FindString("char", &c) == B_OK) {
        if (strlen(c) > 0) {
            mozc::CharacterInfo info;
            mozc::GetToolTip(c, info);
            if (fToolTipWindow->Lock()) {
                fToolTipWindow->SetInfo(info);
                BPoint pos;
                if (msg->FindPoint("pos", &pos) == B_OK) {
                    BView* view = fCharacterList->Parent();
                    while (view) {
                        pos += view->Frame().LeftTop();
                        view = view->Parent();
                    }
                    pos = ConvertToScreen(pos);
                    pos.x += 2;
                    pos.y += 3;
                    fToolTipWindow->MoveTo(pos);
                }
                fToolTipWindow->Unlock();
            }
            if (fToolTipWindow->IsHidden()) {
                fToolTipWindow->Show();
            }
        }
    } else {
        if (!fToolTipWindow->IsHidden()) {
            if (fToolTipWindow->Lock()) {
                fToolTipWindow->Hide();
                fToolTipWindow->Unlock();
            }
        }
    }
}

void CharacterWindow::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case CHAR_SELECTION_CHANGED:
        {
            const char* c;
            if (msg->FindString("char", &c) == B_OK) {
                if (strlen(c) > 0) {
                    CopyToClipboard(c);
                }
            }
            break;
        }
        case FONT_SIZE_CHANGED:
        {
            const int32 index = fFontSizeMenu->FindMarkedIndex();
            if (0 <= index && index <= 4) {
                fCharacterList->SetTableFontSize(index);
            }
            break;
        }
        case FONT_CHANGED:
        {
            const BMenuItem* item = fFontMenu->FindMarked();
            if (item) {
                BFont font;
                if (font.SetFamilyAndStyle(item->Label(), 0) == B_OK) {
                    fCharacterList->SetTableFont(&font);
                    if (fToolTipWindow->Lock()) {
                        fToolTipWindow->SetCharacterFont(&font);
                        fToolTipWindow->Unlock();
                    }
                }
            }
            break;
        }
        case CLEAR_STATUS:
        {
            SetStatusText("");
            break;
        }
        case ON_OVER_CHANGED:
        {
            _ToolTipChanged(msg);
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

}; // haiku_gui
}; // mozc
