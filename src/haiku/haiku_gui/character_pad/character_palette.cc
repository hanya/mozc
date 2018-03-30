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

// ported from gui/character_pad

#include "haiku/haiku_gui/character_pad/character_palette.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/character_pad/character_list.h"
#include "haiku/haiku_gui/character_pad/character_window.h"

#include "gui/character_pad/data/local_character_map.h"
#include "gui/character_pad/data/unicode_blocks.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <StringView.h>

#include <map>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "character_pad"

namespace mozc {
namespace haiku_gui {

const char kUNICODEName[]  = "Unicode";
const char kCP932Name[]    = "Shift JIS";
const char kJISX0201Name[] = "JISX 0201";
const char kJISX0208Name[] = "JISX 0208";
const char kJISX0212Name[] = "JISX 0212";

const mozc::gui::CharacterPalette::UnicodeRange kUCS2Range = { 0, 0xffff };

const mozc::gui::CharacterPalette::CP932JumpTo kCP932JumpTo[] = {
    {"半角英数字", 0x0020},
    {"半角カタカナ", 0x00A1},
    {"全角記号", 0x8141},
    {"全角英数字", 0x8250},
    {"ひらがな", 0x829F},
    {"カタカナ", 0x8340},
    {"丸数字", 0x8740},
    {"ローマ数字", 0xFA40},
    {"単位", 0x875F},
    {"その他の記号", 0x8780},
    {"ギリシャ文字", 0x839F},
    {"キリル文字", 0x8440},
    {"罫線", 0x849F},
    {"第一水準漢字", 0x889F},
    {"第二水準漢字", 0x989F},
    {NULL, 0},
};


class CharacterPalette : public CharacterWindow
{
public:
    CharacterPalette();
    virtual ~CharacterPalette();

    virtual void MessageReceived(BMessage *msg);
private:
    enum PaletteActions
    {
        CATEGORY_CHANGED = 'ctsc',
    };

    std::map<std::string, mozc::gui::CharacterPalette::UnicodeRange> unicode_block_map_;

    BOutlineListView*   fCategories;

    void CategoryChanged();
    void ShowUnicodeTableByRange(const mozc::gui::CharacterPalette::UnicodeRange& range);
    void ShowSJISBlockTable(const char* name);
    void ShowUnicodeTableByBlockName(const char* name);
    void ShowLocalTable(const mozc::gui::CharacterPalette::LocalCharacterMap* local_map, size_t local_map_size);
};

CharacterPalette::CharacterPalette()
    : CharacterWindow(
        BRect(50, 50, 780, 530),
        B_TRANSLATE("Mozc Character Palette"))
{
    fCategories = new BOutlineListView("localeList", B_SINGLE_SELECTION_LIST);
    fCategories->SetDrawingMode(B_OP_OVER);
    BScrollView* scrollList = new BScrollView("scrollList", fCategories,
        B_FRAME_EVENTS, false, true);

    BStringItem* unicodeItem = new BStringItem(kUNICODEName);
    fCategories->AddItem(unicodeItem);
    {
        size_t i = 0;
        for (i = 0; kUnicodeBlockTable[i].name != NULL; ++i) {
            // todo, add items to map
            const mozc::gui::CharacterPalette::UnicodeRange &range = kUnicodeBlockTable[i].range;
            unicode_block_map_.insert(std::make_pair(
                std::string(kUnicodeBlockTable[i].name), range));
            unicode_block_map_.insert(std::make_pair(
                std::string(B_TRANSLATE(kUnicodeBlockTable[i].name)), range));
        }
        for (i = i - 1;; --i) {
            fCategories->AddUnder(new BStringItem(kUnicodeBlockTable[i].name), unicodeItem);
            if (i == 0) {
                break;
            }
        }
    }
    fCategories->Collapse(unicodeItem);

    BStringItem* shiftJISItem = new BStringItem(kCP932Name);
    fCategories->AddItem(shiftJISItem);
    {
        volatile size_t i = 0;
        for (i = 0; kCP932JumpTo[i].name != NULL; ++i) {
        }
        for (i = i - 1;; --i) {
            fCategories->AddUnder(new BStringItem(kCP932JumpTo[i].name), shiftJISItem);
            if (i == 0) {
                break;
            }
        }
    }
    fCategories->Expand(shiftJISItem);
    fCategories->AddItem(new BStringItem(kJISX0201Name));
    fCategories->AddItem(new BStringItem(kJISX0208Name));
    fCategories->AddItem(new BStringItem(kJISX0212Name));

    fCharacterList = new CharacterList("list", CharacterList::UNICODE_RANGE,
                        B_NAVIGABLE,
                        new BMessage(CHAR_SELECTION_CHANGED), this,
                        new BMessage(ON_OVER_CHANGED));
    fCharacterList->SetDrawingMode(B_OP_OVER);
    BScrollView* fCharListView = new BScrollView("listview", fCharacterList,
                B_FOLLOW_LEFT_TOP, B_FRAME_EVENTS, true, true, B_FANCY_BORDER);

    fCategories->SetSelectionMessage(new BMessage(CATEGORY_CHANGED));
    fCategories->SetTarget(NULL, this);

    // set default
    fCategories->Select(fCategories->IndexOf(shiftJISItem), false);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL, 5)
            .SetInsets(5, 5, 5, 5)
            .SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP))
            .AddGlue()
            .Add(fFontMF)
            .Add(fFontSizeMF)
        .End()
        .AddSplit(B_HORIZONTAL, 0)
            .SetInsets(5, 0, 5, 0)
            .Add(scrollList, 0.22)
            .Add(fCharListView, 0.78)
        .End()
        .AddGroup(B_HORIZONTAL, 0)
            .SetInsets(5, 0, 0, 0)
            .Add(fStatusText)
        .End();
    Layout(true);
}

CharacterPalette::~CharacterPalette()
{
}

void CharacterPalette::CategoryChanged()
{
    int32 index = fCategories->CurrentSelection();
    if (index < 0) {
        return;
    }
    BStringItem* item = static_cast<BStringItem*>(fCategories->ItemAt(index));
    if (item == NULL) {
        return;
    }
    const char* text = item->Text();
    BStringItem* parent = static_cast<BStringItem*>(fCategories->Superitem(item));
    if (strcmp(text, kUNICODEName) == 0) {
        ShowUnicodeTableByRange(kUCS2Range);
    } else if (parent != NULL && strcmp(parent->Text(), kUNICODEName) == 0) {
        ShowUnicodeTableByBlockName(text);
    } else if (parent != NULL && strcmp(parent->Text(), kCP932Name) == 0) {
        ShowSJISBlockTable(text);
    } else if (strcmp(text, kJISX0201Name) == 0) {
        ShowLocalTable(kJISX0201Map, kJISX0201MapSize);
    } else if (strcmp(text, kJISX0208Name) == 0) {
        ShowLocalTable(kJISX0208Map, kJISX0208MapSize);
    } else if (strcmp(text, kJISX0212Name) == 0) {
        ShowLocalTable(kJISX0212Map, kJISX0212MapSize);
    } else if (strcmp(text, kCP932Name) == 0) {
        ShowLocalTable(kCP932Map, kCP932MapSize);
    }
}

void CharacterPalette::ShowUnicodeTableByRange(const mozc::gui::CharacterPalette::UnicodeRange& range)
{
    fCharacterList->SetRange(range.first, range.last);
}

void CharacterPalette::ShowSJISBlockTable(const char* name)
{
    const mozc::gui::CharacterPalette::CP932JumpTo* block = NULL;
    for (int32 i = 0; kCP932JumpTo[i].name != NULL; ++i) {
        if (strcmp(name, kCP932JumpTo[i].name) == 0) {
            block = &kCP932JumpTo[i];
            break;
        }
    }
    if (block == NULL) {
        return;
    }
    ShowLocalTable(kCP932Map, kCP932MapSize);
    fCharacterList->JumpToLocalMap(block->from);
}

void CharacterPalette::ShowUnicodeTableByBlockName(const char* name)
{
    std::map<std::string, mozc::gui::CharacterPalette::UnicodeRange>::const_iterator it =
                        unicode_block_map_.find(std::string(name));
    if (it == unicode_block_map_.end()) {
        return;
    }
    ShowUnicodeTableByRange(it->second);
}

void CharacterPalette::ShowLocalTable(
    const mozc::gui::CharacterPalette::LocalCharacterMap* local_map,
    size_t local_map_size)
{
    fCharacterList->SetLocalMap(local_map, local_map_size);
}

void CharacterPalette::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case CATEGORY_CHANGED:
        {
            CategoryChanged();
            break;
        }
        default:
        {
            CharacterWindow::MessageReceived(msg);
            break;
        }
    }
}


class CharacterPaletteApp : public ToolApp
{
public:
    CharacterPaletteApp();
    virtual ~CharacterPaletteApp() {}
};

CharacterPaletteApp::CharacterPaletteApp()
    : ToolApp(CHARACTER_PALETTE)
{
    mpWindow = new CharacterPalette();
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}

}; // haiku_gui
}; // mozc

int HaikuRunCharacterPalette(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                        mozc::haiku_gui::CHARACTER_PALETTE)) {
        return -1;
    }

    mozc::haiku_gui::CharacterPaletteApp* app = new mozc::haiku_gui::CharacterPaletteApp();
    app->Run();
    delete app;

    return 0;
}
