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

#ifndef IMPORT_DIALOG_H
#define IMPORT_DIALOG_H

#include <Window.h>

#include <memory>

class BButton;
class BFilePanel;
class BLooper;
class BMessage;
class BPopUpMenu;
class BStringView;
class BTextControl;

namespace mozc {
namespace haiku_gui {

class ImportDialog : public BWindow
{
public:
    enum {
        CREATE = 'imcr',
        APPEND = 'imap',
        
        IMPORT = 'btok',
        CANCEL = 'btcl',
        FILE_SELECT = 'btfs',
        
        IMPORT_CLOSE = 'imcl',
    };
    ImportDialog(BWindow* parent, BMessage* message, int32 mode);
    virtual ~ImportDialog();

    virtual void MessageReceived(BMessage* msg);
    virtual bool QuitRequested();

    void ExecInCreateMode();
    void ExecInAppendMode();
private:
    enum {
        MODIFIED = 'txmd',
    };
    BWindow*        mpParent;
    BMessage*       mpMessage;
    int32           mnMode;
    BStringView*    mpNameLabel;
    BTextControl*   mpFileText;
    BTextControl*   mpNameText;
    BPopUpMenu*     mpFormatMenu;
    BPopUpMenu*     mpEncodingMenu;
    BButton*        mpImportButton;
    BButton*        mpCancelButton;
    std::unique_ptr<BFilePanel> fFilePanel;

    void _Reset();
    void _SendMessage();
    void _OpenFileChooser();
    bool IsAcceptButtonEnabled() const;
    void OnFormValueChanged();
    uint32 _FormatValue() const;
    uint32 _EncodingValue() const;
};

} // namespace haiku_gui
} // namespace mozc
#endif // IMPORT_DIALOG_H
