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

#include "haiku/haiku_gui/dictionary_tool/find_dialog.h"
#include "haiku/haiku_gui/base/compatible.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <TextControl.h>

#include "base/logging.h"
#include "base/run_level.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "dictionary_tool"

#define tr(s) B_TRANSLATE(s)

namespace mozc {
namespace haiku_gui {

FindDialog::FindDialog(BLooper* looper, GridView<DictContentRow>* grid)
    : BWindow(
        BRect(0, 0, 250, 100),
        B_TRANSLATE("Mozc Dictionary Find Dialog"),
        B_TITLED_WINDOW,
        B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE |
        B_AUTO_UPDATE_SIZE_LIMITS),
      mpLooper(looper),
      mpGridView(grid)
{
    mpLabel = new CStringView("label", B_TRANSLATE("Reading or Word:"));
    mpText = new BTextControl("edit", "", "", NULL);
    mpText->SetModificationMessage(new BMessage(MODIFIED));
    mpUpButton = new BButton(B_TRANSLATE("Up"), new BMessage(UP));
    mpDownButton = new BButton(B_TRANSLATE("Down"), new BMessage(DOWN));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .Add(mpLabel)
        .Add(mpText)
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(mpUpButton)
            .Add(mpDownButton)
            .Add(mpCancelButton)
        .End();

    SetDefaultButton(mpDownButton);
    Layout(true);
    CenterOnScreen();

    UpdateUIStatus();
    mpText->MakeFocus(true);
}

FindDialog::~FindDialog()
{
    mpLooper = NULL;
}

bool FindDialog::QuitRequested()
{
    if (mpLooper) {
        BMessage msg(FIND_CLOSE);
        mpLooper->PostMessage(&msg);
    }
    return true;
}

void FindDialog::UpdateUIStatus()
{
    const bool enabled = mpText->TextView()->TextLength() != 0;
    mpUpButton->SetEnabled(enabled);
    mpDownButton->SetEnabled(enabled);
}

void FindDialog::FindForward()
{
    SetDefaultButton(mpDownButton);
    Find(FORWARD);
}

void FindDialog::FindBackward()
{
    SetDefaultButton(mpUpButton);
    Find(BACKWARD);
}

void FindDialog::Find(FindDialog::Direction direction)
{
    const char* query = mpText->Text();
    int32 start_row = 0;
    int32 start_column = 0; // 0 or 1
    mpGridView->CursorIndex(&start_row, &start_column);
    start_row = max((int32)0, start_row);
    start_column = std::max((int32)0, std::min(start_column, (int32)1));
    int32 matched_row = -1;
    int32 matched_column = -1;

    switch (direction) {
        case FORWARD:
            start_column += 1;
            if (start_column > 1) {
                start_row += 1;
                start_column = 0;
            }
            for (int32 row = start_row; row < mpGridView->CountRows(); ++row) {
                DictContentRow* pRow = mpGridView->ItemAt(row);
                if (pRow) {
                    if (start_column == 0) {
                        const std::string& s = pRow->Reading();
                        if (s.find(query) != std::string::npos) {
                            matched_row = row;
                            matched_column = 0;
                            goto FOUND;
                        }
                    }
                    {
                        const std::string& s = pRow->Word();
                        if (s.find(query) != std::string::npos) {
                            matched_row = row;
                            matched_column = 1;
                            goto FOUND;
                        }
                    }
                }
                start_column = 0;
            }
            break;
        case BACKWARD:
            if (start_column <= 0) {
                start_row -= 1;
                start_column = 1;
            } else {
                start_column -= 1;
            }
            for (int32 row = start_row; row >= 0; --row) {
                DictContentRow* pRow = mpGridView->ItemAt(row);
                if (pRow) {
                    if (start_column == 1) {
                        const std::string& s = pRow->Word();
                        if (s.find(query) != std::string::npos) {
                            matched_row = row;
                            matched_column = 1;
                            goto FOUND;
                        }
                    }
                    {
                        const std::string& s = pRow->Reading();
                        if (s.find(query) != std::string::npos) {
                            matched_row = row;
                            matched_column = 0;
                            goto FOUND;
                        }
                    }
                }
                start_column = 1;
            }
            break;
        default:
            LOG(FATAL) << "Unknown direction: " << static_cast<int>(direction);
            break;
    }

    FOUND:

    if (matched_row >= 0 && matched_column >= 0) {
        mpGridView->SetCursorIndex(matched_row, matched_column);
        mpGridView->ShowItem(matched_row);
    } else {
        QMessageBox::information(this, this->Title(),
                         QString(tr("Cannot find pattern %1")).arg(query).getp());

    }
}

void FindDialog::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case UP:
            FindBackward();
            break;
        case DOWN:
            FindForward();
            break;
        case CANCEL:
            QuitRequested();
            Quit();
            break;
        case MODIFIED:
            UpdateUIStatus();
            break;
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc
