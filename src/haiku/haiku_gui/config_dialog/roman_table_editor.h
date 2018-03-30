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

#ifndef ROMAN_TABLE_EDITOR_H
#define ROMAN_TABLE_EDITOR_H

#include "haiku/haiku_gui/config_dialog/generic_table_editor.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Window.h>

#include <algorithm>
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

class RomanTableRow : public GridRow
{
public:
    RomanTableRow(const char* input, const char* output, const char* next)
        : GridRow() {
        maInput = input;
        maOutput = output;
        maNext = next;
    }
    RomanTableRow()
        : GridRow() {
    }
    virtual ~RomanTableRow() {}

    const char* Input() const { return maInput.c_str(); }
    const char* Output() const { return maOutput.c_str(); }
    const char* Next() const { return maNext.c_str(); }
    void SetInput(const char* input) { maInput = input; }
    void SetOutput(const char* output) { maOutput = output; }
    void SetNext(const char* next) { maNext = next; }

    typedef bool (*RomantableRowComp)(RomanTableRow *r1, RomanTableRow *r2);
    
    static bool InputComp(RomanTableRow *r1, RomanTableRow *r2) {
        return strcmp(r1->Input(), r2->Input()) < 0;
    }

    static bool OutputComp(RomanTableRow *r1, RomanTableRow *r2) {
        return strcmp(r1->Output(), r2->Output()) < 0;
    }

    static bool NextComp(RomanTableRow *r1, RomanTableRow *r2) {
        return strcmp(r1->Next(), r2->Next()) < 0;
    }
    
    static RomantableRowComp GetComp(int32 index)
    {
        switch (index) {
            case 0:
                return *InputComp;
                break;
            case 1:
                return *OutputComp;
                break;
            case 2:
                return *NextComp;
                break;
        }
        return NULL;
    }
protected:
    std::string maInput;
    std::string maOutput;
    std::string maNext;
};


template<typename T>
class RomanTableColumn : public GridTextColumn<T>
{
public:
    RomanTableColumn(BView* view, int32 column, BHandler* handler, ListInputView* p);
    virtual ~RomanTableColumn() {};
protected:
    virtual const char* _GetLabel(T* pRow);
    virtual void _SetText(const char* s);
};

template<typename T>
RomanTableColumn<T>::RomanTableColumn(BView* view, int32 column, BHandler* handler, ListInputView* p)
    : GridTextColumn<T>(view, column, handler, p)
{
}

template<typename T>
const char* RomanTableColumn<T>::_GetLabel(T* pRow)
{
    if (pRow) {
        switch (this->mnColumn) {
            case 0:
                return pRow->Input();
                break;
            case 1:
                return pRow->Output();
                break;
            case 2:
                return pRow->Next();
                break;
        }
    }
    return "";
}

template<typename T>
void RomanTableColumn<T>::_SetText(const char* s)
{
    if (this->mpRow) {
        switch (this->mnColumn) {
            case 0:
                this->mpRow->SetInput(s);
                break;
            case 1:
                this->mpRow->SetOutput(s);
                break;
            case 2:
                this->mpRow->SetNext(s);
                break;
        }
    }
}


namespace {
const char kRomanTableFile[] = "system://romanji-hiragana.tsv";
}  // namespace

template<typename T>
class RomanTableEditorDialog : public GenericTableEditorDialog<T>
{
public:
    enum {
        ENTRY_NEW = 'ernw',
        ENTRY_REMOVE = 'errm',
        IMPORT_FROM = 'imfr',
        EXPORT_TO = 'exto',
        RESET = 'rest',
    };
    enum {
        ROMAN_TABLE_UPDATED = 'rmup',
    };
    RomanTableEditorDialog(BWindow* parent, size_t column_size,
                            const std::string &current_roman_table);
    virtual ~RomanTableEditorDialog() {}

    virtual void MessageReceived(BMessage* msg);
protected:
  virtual void UpdateMenuStatus();
  virtual string GetDefaultFilename() const {
    return "romantable.txt";
  }
  virtual bool LoadFromStream(istream *is);
  virtual bool Update();
private:
  bool LoadDefaultRomanTable();
  static string GetDefaultRomanTable();
};

template<typename T>
RomanTableEditorDialog<T>::RomanTableEditorDialog(BWindow* parent, size_t column_size,
                            const std::string &current_roman_table)
    : GenericTableEditorDialog<T>(parent, column_size)
{
    this->SetTitle(B_TRANSLATE("Mozc Romaji table editor"));
    this->ResizeTo(350, 400);

    GridTitleView* pTitleView = this->mutable_table_widget()->TitleView();
    pTitleView->AddColumn(B_TRANSLATE("Input"));
    pTitleView->AddColumn(B_TRANSLATE("Output"));
    pTitleView->AddColumn(B_TRANSLATE("Next input"));

    this->mutable_table_widget()->AddColumn(
        new RomanTableColumn<RomanTableRow>(
                this->mutable_table_widget(), 0, this->mutable_table_widget(),
                        this->mutable_table_widget()->InputView()), true);
    this->mutable_table_widget()->AddColumn(
        new RomanTableColumn<RomanTableRow>(
                this->mutable_table_widget(), 1, this->mutable_table_widget(),
                        this->mutable_table_widget()->InputView()), true);
    this->mutable_table_widget()->AddColumn(
        new RomanTableColumn<RomanTableRow>(
                this->mutable_table_widget(), 2, this->mutable_table_widget(),
                        this->mutable_table_widget()->InputView()), true);

    BLayoutBuilder::Menu<>(this->mutable_edit_menu())
        .AddItem(B_TRANSLATE("New entry"), new BMessage(ENTRY_NEW))
        .AddItem(B_TRANSLATE("Remove entry"), new BMessage(ENTRY_REMOVE))
        .AddSeparator()
        .AddItem(B_TRANSLATE("Import from file..."), new BMessage(IMPORT_FROM))
        .AddItem(B_TRANSLATE("Export to file..."), new BMessage(EXPORT_TO))
        .AddSeparator()
        .AddItem(B_TRANSLATE("Reset to defaults"), new BMessage(RESET));

    if (current_roman_table.empty()) {
        LoadDefaultRomanTable();
    } else {
        this->LoadFromString(current_roman_table);
    }

    this->Layout(true);
}

template<typename T>
string RomanTableEditorDialog<T>::GetDefaultRomanTable() {
  std::unique_ptr<istream> ifs(ConfigFileStream::LegacyOpen(kRomanTableFile));
  CHECK(ifs.get() != NULL);  // should never happen
  string line, result;
  vector<string> fields;
  while (getline(*ifs.get(), line)) {
    if (line.empty()) {
      continue;
    }
    Util::ChopReturns(&line);
    fields.clear();
    Util::SplitStringAllowEmpty(line, "\t", &fields);
    if (fields.size() < 2) {
      VLOG(3) << "field size < 2";
      continue;
    }
    result += fields[0];
    result += '\t';
    result += fields[1];
    if (fields.size() >= 3) {
      result += '\t';
      result += fields[2];
    }
    result += '\n';
  }
  return result;
}

template<typename T>
bool RomanTableEditorDialog<T>::LoadFromStream(istream *is) {
  CHECK(is);
  string line;
  vector<string> fields;

  int row = 0;
  while (getline(*is, line)) {
    if (line.empty()) {
      continue;
    }
    Util::ChopReturns(&line);

    fields.clear();
    Util::SplitStringAllowEmpty(line, "\t", &fields);
    if (fields.size() < 2) {
      VLOG(3) << "field size < 2";
      continue;
    }

    if (fields.size() == 2) {
      fields.push_back("");
    }
    RomanTableRow* item = new RomanTableRow(
                            fields[0].c_str(), fields[1].c_str(), fields[2].c_str());
    this->mutable_table_widget()->AddRow(item);
    ++row;

    if (row >= this->max_entry_size()) {
      QMessageBox::warning(
          this,
          tr("Mozc settings"),
          QString(tr("You can't have more than %1 entries")).arg(this->max_entry_size()).getp());
      break;
    }
  }

  //UpdateMenuStatus();

  return true;
}

template<typename T>
bool RomanTableEditorDialog<T>::LoadDefaultRomanTable() {
  std::unique_ptr<istream> ifs(ConfigFileStream::LegacyOpen(kRomanTableFile));
  CHECK(ifs.get() != NULL);  // should never happen
  CHECK(LoadFromStream(ifs.get()));
  return true;
}

template<typename T>
bool RomanTableEditorDialog<T>::Update() {
  if (this->mutable_table_widget()->CountRows() == 0) {
    QMessageBox::warning(this,
                         tr("Mozc settings"),
                         tr("Romaji to Kana table is empty."));
    return false;
  }

  bool contains_capital = false;
  string *table = this->mutable_table();
  table->clear();
  for (int i = 0; i < this->mutable_table_widget()->CountRows(); ++i) {
    RomanTableRow* pRow = this->mutable_table_widget()->ItemAt(i);
    if (!pRow) {
        continue;
    }
    const string input = pRow->Input();
    const string output = pRow->Output();
    const string pending = pRow->Next();
    if (input.empty() || (output.empty() && pending.empty())) {
      continue;
    }
    *table += input;
    *table += '\t';
    *table += output;
    if (!pending.empty()) {
      *table += '\t';
      *table += pending;
    }
    *table += '\n';

    if (!contains_capital) {
      string lower = input;
      Util::LowerString(&lower);
      contains_capital = (lower != input);
    }
  }

  if (contains_capital) {
    // TODO(taku):
    // Want to see the current setting and suppress this
    // dialog if the shift-mode-switch is already off.
    QMessageBox::information(
        this,
        tr("Mozc settings"),
        tr("Input fields contain capital characters. "
           "\"Shift-mode-switch\" function is disabled "
           "with this new mapping."));
  }

  return true;
}

template<typename T>
void RomanTableEditorDialog<T>::UpdateMenuStatus() {
}

template<typename T>
void RomanTableEditorDialog<T>::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case GenericTableEditorDialog<T>::OK:
        {
            if (Update()) {
                if (this->mpParent) {
                    BMessage message(ROMAN_TABLE_UPDATED);
                    message.AddString("output", this->table().c_str());
                    this->mpParent->PostMessage(&message);
                }
                this->Quit();
            }
            break;
        }
        case ENTRY_NEW:
        {
            this->AddNewItem();
            break;
        }
        case ENTRY_REMOVE:
        {
            this->DeleteSelectedItems();
            break;
        }
        case IMPORT_FROM:
        {
            this->Import();
            break;
        }
        case EXPORT_TO:
        {
            this->Export();
            break;
        }
        case RESET:
        {
            LoadDefaultRomanTable();
            break;
        }
        default:
            GenericTableEditorDialog<T>::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc

#endif // ROMAN_TABLE_EDITOR_H
