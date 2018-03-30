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

#include "haiku/haiku_gui/dictionary_tool/import_dialog.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include <Button.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Looper.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "base/util.h"
#include "dictionary/user_dictionary_storage.h"
#include "dictionary/user_dictionary_importer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "dictionary_tool"

namespace mozc {
namespace haiku_gui {

ImportDialog::ImportDialog(BWindow* parent, BMessage* message, int32 mode)
    : BWindow(
        BRect(0, 0, 200, 50),
        B_TRANSLATE("Mozc Dictionary Tool"),
        B_TITLED_WINDOW_LOOK,
        B_MODAL_SUBSET_WINDOW_FEEL,
        B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE |
        B_AUTO_UPDATE_SIZE_LIMITS),
      mpParent(parent),
      mpMessage(message),
      mnMode(mode)
{
    SetLook(B_TITLED_WINDOW_LOOK);
    
    BMessenger messenger(NULL, this);
    fFilePanel = std::unique_ptr<BFilePanel>(
        new BFilePanel(B_OPEN_PANEL,
            &messenger, NULL,
            0, false,
            NULL, NULL, false, true));
    
    mpNameLabel = new CStringView("namelabel", B_TRANSLATE("Dictionary Name"));
    mpFileText = new BTextControl("fileedit", "", "", NULL);
    mpNameText = new BTextControl("nameedit", "", "", NULL);
    mpFileText->SetModificationMessage(new BMessage(MODIFIED));
    mpNameText->SetModificationMessage(new BMessage(MODIFIED));
    BSize size = mpFileText->MinSize();
    size.width = 150;
    mpFileText->SetExplicitMinSize(size);
    
    mpFormatMenu = new BPopUpMenu("formatmenu");
    mpFormatMenu->SetRadioMode(true);
    BLayoutBuilder::Menu<>(mpFormatMenu)
        .AddItem(B_TRANSLATE("Auto detection"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::IME_AUTO_DETECT)))
        .AddItem(B_TRANSLATE("Google"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::MOZC)))
        .AddItem(B_TRANSLATE("Kotoeri"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::KOTOERI)))
        .AddItem(B_TRANSLATE("ATOK"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::ATOK)))
        .AddItem(B_TRANSLATE("Microsoft IME"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::MSIME)));
    
    mpEncodingMenu = new BPopUpMenu("encodingmenu");
    mpEncodingMenu->SetRadioMode(true);
    BLayoutBuilder::Menu<>(mpEncodingMenu)
        .AddItem(B_TRANSLATE("Auto detection"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::ENCODING_AUTO_DETECT)))
        .AddItem(B_TRANSLATE("Unicode"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::UTF16)))
        .AddItem(B_TRANSLATE("Shift JIS"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::SHIFT_JIS)))
        .AddItem(B_TRANSLATE("UTF-8"),
            new BMessage(static_cast<uint32>(UserDictionaryImporter::UTF8)));
    
    mpFormatMenu->ItemAt(0)->SetMarked(true);
    mpEncodingMenu->ItemAt(0)->SetMarked(true);
    
    mpImportButton = new BButton(B_TRANSLATE("Import"), new BMessage(IMPORT));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));
    
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5)
        .AddGrid(B_USE_DEFAULT_SPACING, B_USE_HALF_ITEM_SPACING)
            .Add(new CStringView("filelabel", B_TRANSLATE("File Location")), 0, 0)
            .Add(mpFileText, 1, 0)
            .Add(new BButton(B_TRANSLATE("Select file..."), new BMessage(FILE_SELECT)), 2, 0)
            .Add(mpNameLabel, 0, 1)
            .Add(mpNameText, 1, 1)
            .Add(new CStringView("formatlabel", B_TRANSLATE("Format")), 0, 2)
            .Add(new BMenuField("formatfield", "", mpFormatMenu), 1, 2)
            .Add(new CStringView("encodinglabel", B_TRANSLATE("Encoding")), 0, 3)
            .Add(new BMenuField("encodingfield", "", mpEncodingMenu), 1, 3)
        .End()
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(mpImportButton)
            .Add(mpCancelButton)
        .End();
    _Reset();
    Layout(true);
    if (mpParent) {
        CenterIn(mpParent->Frame());
        AddToSubset(mpParent);
    }
}

ImportDialog::~ImportDialog()
{
    if (mpParent) {
        RemoveFromSubset(mpParent);
    }
}

bool ImportDialog::QuitRequested()
{
    if (mpParent) {
        BMessage msg(IMPORT_CLOSE);
        mpParent->PostMessage(&msg);
    }
    return true;
}

void ImportDialog::ExecInCreateMode()
{
    mnMode = CREATE;
    _Reset();
    Show();
}

void ImportDialog::ExecInAppendMode()
{
    mnMode = APPEND;
    _Reset();
    Show();
}

bool ImportDialog::IsAcceptButtonEnabled() const
{
    const bool is_enabled =
        (mnMode == CREATE &&
            !mpFileText->TextView()->TextLength() == 0 &&
            !mpNameText->TextView()->TextLength() == 0) ||
        (mnMode == APPEND &&
            !mpFileText->TextView()->TextLength() == 0);
    return is_enabled;
}

void ImportDialog::OnFormValueChanged() {
    mpImportButton->SetEnabled(IsAcceptButtonEnabled());
}

void ImportDialog::_Reset()
{
    mpFileText->TextView()->Clear();
    mpNameText->TextView()->Clear();
    mpFormatMenu->ItemAt(0)->SetMarked(true);
    mpEncodingMenu->ItemAt(0)->SetMarked(true);
    
    if (mnMode == CREATE) {
        mpNameLabel->Show();
        mpNameText->Show();
    } else {
        mpNameLabel->Hide();
        mpNameText->Hide();
    }
    
    OnFormValueChanged();
    mpFileText->MakeFocus(true);
}

void ImportDialog::_SendMessage()
{
    if (mpParent && mpMessage) {
        BMessage message(*mpMessage);
        message.AddString("path", mpFileText->TextView()->Text());
        message.AddString("name", mpNameText->TextView()->Text());
        message.AddInt32("format", _FormatValue());
        message.AddInt32("encoding", _EncodingValue());
        mpParent->PostMessage(&message);
    }
}

uint32 ImportDialog::_FormatValue() const
{
    BMenuItem* item = mpFormatMenu->FindMarked();
    if (item) {
        BMessage* msg = item->Message();
        if (msg) {
            return msg->what;
        }
    }
    return 0;
}

uint32 ImportDialog::_EncodingValue() const
{
    BMenuItem* item = mpEncodingMenu->FindMarked();
    if (item) {
        BMessage* msg = item->Message();
        if (msg) {
            return msg->what;
        }
    }
    return 0;
}

void ImportDialog::_OpenFileChooser()
{
    fFilePanel->Show();
}

void ImportDialog::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK) {
                BPath path(&ref);
                mpFileText->TextView()->SetText(static_cast<const char*>(path.Path()));
            }
            break;
        }
        case IMPORT:
            if (IsAcceptButtonEnabled()) {
                _SendMessage();
                Quit();
            }
            break;
        case CANCEL:
            QuitRequested();
            Quit();
            break;
        case FILE_SELECT:
            _OpenFileChooser();
            break;
        case MODIFIED:
            OnFormValueChanged();
            break;
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

} // namespace haiku_gui
} // namespace mozc
