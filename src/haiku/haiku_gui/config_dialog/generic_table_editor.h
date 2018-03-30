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

#ifndef GENERIC_TABLE_EDITOR_H
#define GENERIC_TABLE_EDITOR_H

#include <Button.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Window.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <algorithm>  // for unique
#include <cctype>
#include <string>
#include <vector>
#include <sstream>

#include "base/file_stream.h"
#include "base/config_file_stream.h"
#include "base/logging.h"
#include "base/util.h"
#include "protocol/commands.pb.h"

#define tr(s) B_TRANSLATE(s)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "config_dialog"

namespace mozc {
namespace haiku_gui {

template<typename T>
class GenericTableView : public GridView<T>
{
public:
    GenericTableView(BLooper* looper);
    virtual ~GenericTableView() {}

    virtual void Draw(BRect rect);
};

template<typename T>
GenericTableView<T>::GenericTableView(BLooper* looper)
    : GridView<T>(MULTIPLE, looper, NULL)
{
    this->SetExplicitMinSize(BSize(50, this->mnRowHeight * 9));
}

template<typename T>
void GenericTableView<T>::Draw(BRect rect)
{
    const float nRowHeight = this->mnRowHeight;
    const float nTitleHeight = this->mpTitleView->Height();

    int32 nLastRow = std::min(this->mnFirstRow + this->mnRows + 1,
                              static_cast<int32>(this->mpRows.size()));

    this->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
    // draw content
    BPoint pos(0, nTitleHeight + ceil(nRowHeight * 0.75));
    BRect r0(0, nTitleHeight, this->mpTitleView->ColumnWidth(0), nTitleHeight + nRowHeight);
    BRect r1(r0.right, nTitleHeight, r0.right + this->mpTitleView->ColumnWidth(1), r0.bottom);
    BRect r2(r1.right, nTitleHeight, r1.right + this->mpTitleView->ColumnWidth(2), r0.bottom);
    T* pCursorRow = this->_IsCursorValid() ? this->mpRows[this->mnCursorRow] : NULL;
    for (size_t i = this->mnFirstRow; i < nLastRow; ++i) {
        T* pRow = this->mpRows[i];
        if (pRow->IsSelected()) {
            this->SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
            this->FillRect(BRect(0, r0.top, r2.right, r0.bottom), B_SOLID_LOW);
            this->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
        }
        const bool bSelectedRow = pRow == pCursorRow;

        this->mpColumns[0]->DrawAt(this, r0, bSelectedRow && this->mnCursorColumn == 0, pRow);
        r0.top = r0.bottom;
        r0.bottom += nRowHeight;
        pos.x += this->mpTitleView->ColumnWidth(0);

        this->mpColumns[1]->DrawAt(this, r1, bSelectedRow && this->mnCursorColumn == 1, pRow);
        r1.top = r0.top;
        r1.bottom = r0.bottom;
        pos.x += this->mpTitleView->ColumnWidth(1);

        this->mpColumns[2]->DrawAt(this, r2, bSelectedRow && this->mnCursorColumn == 2, pRow);
        r2.top = r0.top;
        r2.bottom = r0.bottom;
        pos.x += this->mpTitleView->ColumnWidth(2);

        pos.x = 0;
        pos.y += nRowHeight;
        if (pRow->IsSelected()) {
            this->SetLowUIColor(B_LIST_BACKGROUND_COLOR);
            this->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
        }
    }
    this->_DrawGrid();
    this->_DrawCursor();
}

namespace {
const size_t kMaxEntrySize = 10000;
} // namespace

template<typename T>
class GenericTableEditorDialog : public BWindow
{
public:
  GenericTableEditorDialog(BWindow* parent, size_t column_size);
  virtual ~GenericTableEditorDialog();

  bool LoadFromString(const string &table);
  const string &table() const { return table_; }

  virtual void MessageReceived(BMessage* msg);
  const char* windowTitle() { return Title(); }
protected:
  virtual void AddNewItem();
  virtual void InsertItem();
  virtual void DeleteSelectedItems();
  virtual void OnContextMenuRequested();
  //virtual void Clicked(QAbstractButton *button);
  virtual void InsertEmptyItem(int row);
  virtual void UpdateOKButton(bool status);
  virtual void Import();
  virtual void Export();

  virtual void UpdateMenuStatus();
  virtual void OnEditMenuAction(QAction *action);
protected:
  string *mutable_table() { return &table_; }
  GenericTableView<T> *mutable_table_widget() { return mpGridView; }
  BPopUpMenu *mutable_edit_menu() { return edit_menu_; }

  virtual string GetDefaultFilename() const = 0;
  virtual bool LoadFromStream(istream *is) = 0;
  virtual bool Update() = 0;
  virtual size_t max_entry_size() const;
  void _Import(const char* name);
  void _Export(const char* name);
  enum {
    OK = 'btok',
    CANCEL = 'btcl',
    EDIT = 'bted',
  };
  BWindow* mpParent;
private:
  BPopUpMenu *edit_menu_;
  string table_;
  size_t column_size_;

  GenericTableView<T>* mpGridView;
  BButton* mpEditButton;
  BButton* mpOkButton;
  BButton* mpCancelButton;
  std::unique_ptr<BFilePanel> mpImportFilePanel;
  std::unique_ptr<BFilePanel> mpExportFilePanel;

  QDialogButtonBox* editorButtonBox;
};

template<typename T>
GenericTableEditorDialog<T>::GenericTableEditorDialog(BWindow* parent, size_t column_size)
    : BWindow(
        BRect(0, 0, 450, 400),
        "Mozc table editor",
        B_TITLED_WINDOW_LOOK,
        B_MODAL_SUBSET_WINDOW_FEEL,
        B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      mpParent(parent)
{
    mpImportFilePanel = std::unique_ptr<BFilePanel>(
                        new BFilePanel(B_OPEN_PANEL,
                            new BMessenger(this, NULL), NULL,
                            0, false,
                            NULL, NULL, true, true));
    mpImportFilePanel->Window()->SetTitle(tr("import from file"));
    mpExportFilePanel = std::unique_ptr<BFilePanel>(
                        new BFilePanel(B_SAVE_PANEL,
                            new BMessenger(this, NULL), NULL,
                            0, false,
                            NULL, NULL, true, true));
    mpExportFilePanel->Window()->SetTitle(tr("export to file"));

    mpGridView = new GenericTableView<T>(NULL);
    mpGridView->SetDrawingMode(B_OP_OVER);
    BScrollView* pGridScrollView = new BScrollView("contentview", mpGridView,
                B_FOLLOW_LEFT_TOP, B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
    GridTitleView* pTitleView = new GridTitleView(false, mpGridView);
    pTitleView->SetDrawingMode(B_OP_OVER);
    mpGridView->SetTitleView(pTitleView);

    edit_menu_ = new BPopUpMenu("editmenu");
    edit_menu_->SetRadioMode(false);

    mpEditButton = new BButton(B_TRANSLATE("Edit"), new BMessage(EDIT));
    mpEditButton->SetBehavior(BButton::B_POP_UP_BEHAVIOR);
    mpEditButton->SetPopUpMessage(new BMessage(EDIT));
    mpOkButton = new BButton(B_TRANSLATE("OK"), new BMessage(OK));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5)
        .Add(pGridScrollView)
        .AddGroup(B_HORIZONTAL)
            .Add(mpEditButton)
            .AddGlue()
            .Add(mpOkButton)
            .Add(mpCancelButton)
        .End();

    AddToSubset(parent);

    if (parent) {
        CenterIn(parent->Frame());
    }

    editorButtonBox = new QDialogButtonBox(mpOkButton, mpCancelButton);
}

template<typename T>
GenericTableEditorDialog<T>::~GenericTableEditorDialog()
{
    delete edit_menu_;

    delete editorButtonBox;
}

template<typename T>
bool GenericTableEditorDialog<T>::LoadFromString(const string &str) {
  istringstream istr(str);
  return LoadFromStream(&istr);
}

template<typename T>
void GenericTableEditorDialog<T>::DeleteSelectedItems() {
  vector<int> rows = mutable_table_widget()->SelectedIndex();

  if (rows.empty()) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("No entry is selected"));
    return;
  }
  
  int32 row = 0;
  int32 column = 0;
  mutable_table_widget()->CursorIndex(&row, &column);

  // remove from the buttom
  for (int i = rows.size() - 1; i >= 0; --i) {
    mutable_table_widget()->RemoveRow(rows[i]);
  }

  if (row >= 0 && column >= 0) {
    mutable_table_widget()->SetCursorIndex(
            std::min(row, mutable_table_widget()->CountRows() - 1), column);
  }

  UpdateMenuStatus();
}

template<typename T>
void GenericTableEditorDialog<T>::InsertEmptyItem(int row) {
  T* item = new T();
  mutable_table_widget()->InsertRow(row, item);
  mutable_table_widget()->ShowItem(row);

  UpdateMenuStatus();
}

template<typename T>
void GenericTableEditorDialog<T>::InsertItem() {
  const int32 row = mutable_table_widget()->CurrentSelection();
  if (row < 0) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("No entry is selected"));
    return;
  }
  InsertEmptyItem(row + 1);
}

template<typename T>
void GenericTableEditorDialog<T>::AddNewItem() {
  if (mutable_table_widget()->CountRows() >= max_entry_size()) {
    QMessageBox::warning(
        this,
        windowTitle(),
        QString(tr("You can't have more than %1 entries")).arg(max_entry_size()).getp());
    return;
  }

  InsertEmptyItem(mutable_table_widget()->CountRows());
}

template<typename T>
void GenericTableEditorDialog<T>::Import() {
  mpImportFilePanel->Show();
}

template<typename T>
void GenericTableEditorDialog<T>::_Import(const char* name) {
  std::string filename = name;
  if (filename.empty()) {
    return;
  }

  BEntry entry(filename.c_str());
  if (!entry.Exists()) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("File not found"));
    return;
  }

  {
  BFile file(&entry, B_READ_ONLY);
  const off_t kMaxSize = 100 * 1024;
  off_t size = kMaxSize + 1;
  file.GetSize(&size);
  if (size >= kMaxSize) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("The specified file is too large (>=100K byte)"));
    return;
  }
  }

  InputFileStream ifs(filename.c_str());
  if (!LoadFromStream(&ifs)) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("Import failed"));
    return;
  }
}

template<typename T>
void GenericTableEditorDialog<T>::Export() {
  if (!Update()) {
    return;
  }
  mpExportFilePanel->SetSaveText(GetDefaultFilename().c_str());
  mpExportFilePanel->Show();
}

template<typename T>
void GenericTableEditorDialog<T>::_Export(const char* name) {
  std::string filename = name;
  if (filename.empty()) {
    return;
  }

  OutputFileStream ofs(filename.c_str());
  if (!ofs) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("Export failed"));
    return;
  }

  ofs << table();
}

//void GenericTableEditorDialog::Clicked(QAbstractButton *button) {

template<typename T>
void GenericTableEditorDialog<T>::OnContextMenuRequested() {
}

template<typename T>
void GenericTableEditorDialog<T>::UpdateOKButton(bool status) {
  QAbstractButton *button = editorButtonBox->button(QDialogButtonBox::Ok);
  if (button != NULL) {
    button->setEnabled(status);
  }
}

template<typename T>
size_t GenericTableEditorDialog<T>::max_entry_size() const {
  return kMaxEntrySize;
}

template<typename T>
bool GenericTableEditorDialog<T>::LoadFromStream(istream *is) {
  return true;
}

template<typename T>
bool GenericTableEditorDialog<T>::Update() {
  return true;
}

template<typename T>
void GenericTableEditorDialog<T>::UpdateMenuStatus() {}

template<typename T>
void GenericTableEditorDialog<T>::OnEditMenuAction(QAction *action) {}

template<typename T>
void GenericTableEditorDialog<T>::MessageReceived(BMessage *msg) {
    switch (msg->what) {
        case CANCEL:
            Quit();
            break;
        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK) {
                BEntry entry(&ref, true);
                BPath path(&entry);
                _Import(path.Path());
            }
            break;
        }
        case B_SAVE_REQUESTED:
        {
            entry_ref ref;
            const char* name = NULL;
            if (msg->FindRef("directory", &ref) == B_OK &&
                msg->FindString("name", &name) == B_OK) {
                BPath path(&ref);
                path.Append(name);
                _Export(path.Path());
            }
            break;
        }
        case GridColumn<T>::MODIFIED:
        {
            if (mutable_table_widget()->LockLooper()) {
                mutable_table_widget()->Invalidate();
                mutable_table_widget()->UnlockLooper();
            }
            break;
        }
        case EDIT:
        {
            BPopUpMenu* editMenu = mutable_edit_menu();
            BPoint where = mpEditButton->Frame().LeftBottom();
            ConvertToScreen(&where);
            BMenuItem *item = editMenu->Go(where, true);
            if (item != NULL) {
                BMessage *message = item->Message();
                if (message != NULL) {
                    MessageReceived(message);
                }
            }
            break;
        }
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc

#endif // GENERIC_TABLE_EDITOR_H
