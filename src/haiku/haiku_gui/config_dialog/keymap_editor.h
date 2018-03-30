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

#ifndef KEYMAP_EDITOR_H
#define KEYMAP_EDITOR_H

#include "haiku/haiku_gui/config_dialog/keybinding_editor.h"
#include "haiku/haiku_gui/config_dialog/keymap_labels.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Window.h>

#include <algorithm>  // for unique
#include <cctype>
#include <string>
#include <vector>
#include <sstream>

#include "base/file_stream.h"
#include "base/config_file_stream.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "base/util.h"
#include "composer/key_parser.h"
#include "protocol/commands.pb.h"
#include "session/internal/keymap.h"

#define tr(s) B_TRANSLATE(s)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "config_dialog"

namespace mozc {
namespace haiku_gui {

class KeymapRow : public GridRow
{
public:
    KeymapRow(const char* mode, const char* key, const char* command)
        : GridRow() {
        maMode = mode;
        maKey = key;
        maCommand = command;
    }
    KeymapRow()
        : GridRow() {
    }
    virtual ~KeymapRow() {}

    const char* Mode() { return maMode.c_str(); }
    const char* Key() { return maKey.c_str(); }
    const char* Command() { return maCommand.c_str(); }
    void SetMode(const char* mode) { maMode = mode; }
    void SetKey(const char* key) { maKey = key; }
    void SetCommand(const char* command) { maCommand = command; }

    typedef bool (*KeymapRowComp)(KeymapRow *r1, KeymapRow *r2);
    static bool ModeComp(KeymapRow *r1, KeymapRow *r2) {
        return r1->maMode.compare(r2->maMode) < 0;
    }
    static bool KeyComp(KeymapRow *r1, KeymapRow *r2) {
        return r1->maKey.compare(r2->maKey) < 0;
    }
    static bool CommandComp(KeymapRow *r1, KeymapRow *r2) {
        return r1->maCommand.compare(r2->maCommand) < 0;
    }
    static KeymapRowComp GetComp(int32 index) {
        switch (index) {
            case 0:
                return *ModeComp;
                break;
            case 1:
                return *KeyComp;
                break;
            case 2:
                return *CommandComp;
                break;
        }
        return NULL;
    }
protected:
    std::string maMode;
    std::string maKey;
    std::string maCommand;
};


template<typename T>
class KeymapKeyColumn : public GridButtonFieldColumn<T>, public BHandler
{
public:
    KeymapKeyColumn(BView* view, int32 column, BHandler* handler);
    virtual ~KeymapKeyColumn() {};

    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow);

    virtual void MessageReceived(BMessage* msg);
protected:
    virtual const char* _Label(T* pRow);
    virtual void _UpdateRow(T* pRow, BMessage* msg);
};

template<typename T>
KeymapKeyColumn<T>::KeymapKeyColumn(BView* view, int32 column, BHandler* handler)
    : GridButtonFieldColumn<T>(view, column, handler)
{
}

template<typename T>
void KeymapKeyColumn<T>::Edit(BView* view, BRect rect, BPoint pos, T* pRow)
{
    this->mpRow = pRow;
    KeyBindingEditor* window = new KeyBindingEditor(view->Window(), this);
    window->Show();
}

template<typename T>
const char* KeymapKeyColumn<T>::_Label(T* pRow)
{
    return pRow ? pRow->Key() : "";
}

template<typename T>
void KeymapKeyColumn<T>::_UpdateRow(T* pRow, BMessage* msg)
{
    if (pRow) {
        const char* keys = msg->GetString("keys", NULL);
        if (keys) {
            pRow->SetKey(keys);
            this->_Modified();
        }
    }
}

template<typename T>
void KeymapKeyColumn<T>::MessageReceived(BMessage* msg)
{
    if (msg->what == KeyBindingEditor::KEY_SPECIFIED) {
        if (this->mpRow) {
            _UpdateRow(this->mpRow, msg);
        }
    }
}

template<typename T>
class KeymapMenuFieldColumn : public GridMenuFieldColumn<T>
{
public:
    KeymapMenuFieldColumn(BView* view, int32 column, BHandler* handler);
    virtual ~KeymapMenuFieldColumn() {}

    virtual void AddLabel(const char* label);
    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
protected:
    virtual void _UpdateRow(T* pRow, BMessage* msg);
    virtual int32 _LabelIndex(T* pRow);
    virtual int32 _CurrentIndex(T* pRow);
};

template<typename T>
KeymapMenuFieldColumn<T>::KeymapMenuFieldColumn(BView* view, int32 column, BHandler* handler)
    : GridMenuFieldColumn<T>(view, column, handler)
{
}

template<typename T>
void KeymapMenuFieldColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    const char* label = this->mnColumn == 0 ? pRow->Mode() : pRow->Command();
    BPoint pos(rect.left + 5, rect.top + rect.Height() * 0.75);
    if (selected) {
        this->_DrawMenuField(view, rect, pos, label);
    } else {
        view->DrawString(label, pos);
    }
}

template<typename T>
void KeymapMenuFieldColumn<T>::AddLabel(const char* label)
{
    this->maLabels.push_back(std::string(label));
    BMessage* message = new BMessage(this->CountLabels() - 1);
    message->AddString("label", label);
    this->mpPopUpMenu->AddItem(new BMenuItem(label, message));
}

template<typename T>
void KeymapMenuFieldColumn<T>::_UpdateRow(T* pRow, BMessage* msg)
{
    const int32 mode = msg->what;
    if (0 <= mode && mode < this->CountLabels()) {
        const char* label = this->Label(mode);
        if (label) {
            if (this->mnColumn == 0) {
                pRow->SetMode(label);
            } else {
                pRow->SetCommand(label);
            }
        }
    }
}

template<typename T>
int32 KeymapMenuFieldColumn<T>::_LabelIndex(T* pRow)
{
    return _LabelIndex(pRow);
}

template<typename T>
int32 KeymapMenuFieldColumn<T>::_CurrentIndex(T* pRow)
{
    const char* current = this->mnColumn == 0 ? pRow->Mode() : pRow->Command();
    int32 index = -1;
    for (int32 i = 0; i < this->CountLabels(); ++i) {
        if (this->maLabels[i] == current) {
            index = i;
            break;
        }
    }
    return index;
}


namespace {

config::Config::SessionKeymap kKeyMaps[] = {
  config::Config::ATOK,
  config::Config::MSIME,
  config::Config::KOTOERI,
};

const char *kKeyMapStatus[] = {
  "DirectInput",
  "Precomposition",
  "Composition",
  "Conversion",
  "Suggestion",
  "Prediction",
};

const char kInsertCharacterCommand[] = "InsertCharacter";
const char kDirectMode[] = "DirectInput";
const char kReportBugCommand[] = "ReportBug";
// Old command name
const char kEditInsertCommand[] = "EditInsert";

// Keymap validator for deciding that input is configurable
class KeyMapValidator {
 public:
  KeyMapValidator() {
    invisible_commands_.insert(kInsertCharacterCommand);
    invisible_commands_.insert(kReportBugCommand);
    // Old command name.
    invisible_commands_.insert(kEditInsertCommand);

    invisible_modifiers_.insert(mozc::commands::KeyEvent::KEY_DOWN);
    invisible_modifiers_.insert(mozc::commands::KeyEvent::KEY_UP);

    invisible_key_events_.insert(mozc::commands::KeyEvent::KANJI);
    invisible_key_events_.insert(mozc::commands::KeyEvent::ON);
    invisible_key_events_.insert(mozc::commands::KeyEvent::OFF);
    invisible_key_events_.insert(mozc::commands::KeyEvent::TEXT_INPUT);
  }

  bool IsVisibleKey(const string &key) {
    mozc::commands::KeyEvent key_event;
    const bool parse_success = mozc::KeyParser::ParseKey(key, &key_event);
    if (!parse_success) {
      VLOG(3) << "key parse failed";
      return false;
    }
    for (size_t i = 0; i < key_event.modifier_keys_size(); ++i) {
      if (invisible_modifiers_.find(key_event.modifier_keys(i))
          != invisible_modifiers_.end()) {
        VLOG(3) << "invisible modifiers: " << key_event.modifier_keys(i);
        return false;
      }
    }
    if (key_event.has_special_key() &&
        (invisible_key_events_.find(key_event.special_key())
         != invisible_key_events_.end())) {
      VLOG(3) << "invisible special key: " << key_event.special_key();
      return false;
    }
    return true;
  }

  bool IsVisibleStatus(const string &status) {
    // no validation for now.
    return true;
  }

  bool IsVisibleCommand(const string &command) {
    if (invisible_commands_.find(command) == invisible_commands_.end()) {
      return true;
    }
    VLOG(3) << "invisible command: " << command;
    return false;
  }

  // Returns true if the key map entry is valid
  // invalid keymaps are not exported/imported.
  bool IsValidEntry(const vector<string> &fields) {
    if (fields.size() < 3) {
      return false;
    }

#ifdef NO_LOGGING
    if (fields[2] == kReportBugCommand) {
      return false;
    }
#endif
    return true;
  }

  // Returns true if the key map entry is configurable and
  // we want to show them.
  bool IsVisibleEntry(const vector<string> &fields) {
    if (fields.size() < 3) {
      return false;
    }
    const string &key = fields[1];
    const string &command = fields[2];
    if (!IsVisibleKey(key)) {
      return false;
    }
    if (!IsVisibleCommand(command)) {
      return false;
    }

    return true;
  }

 private:
  set<uint32> invisible_modifiers_;
  set<uint32> invisible_key_events_;
  set<string> invisible_commands_;
};

class KeyMapTableLoader {
 public:
  KeyMapTableLoader() {
    string line;
    vector<string> fields;
    set<string> status;
    set<string> commands;
    KeyMapValidator *validator = mozc::Singleton<KeyMapValidator>::get();

    // get all command names
    set<string> command_names;
    mozc::keymap::KeyMapManager manager;
    manager.GetAvailableCommandNameDirect(&command_names);
    manager.GetAvailableCommandNamePrecomposition(&command_names);
    manager.GetAvailableCommandNameComposition(&command_names);
    manager.GetAvailableCommandNameConversion(&command_names);
    manager.GetAvailableCommandNameZeroQuerySuggestion(&command_names);
    manager.GetAvailableCommandNameSuggestion(&command_names);
    manager.GetAvailableCommandNamePrediction(&command_names);
    for (set<string>::const_iterator itr = command_names.begin();
         itr != command_names.end(); ++itr) {
      if (validator->IsVisibleCommand(*itr)) {
        commands.insert(*itr);
      }
    }

    for (size_t i = 0; i < arraysize(kKeyMapStatus); ++i) {
      status_.push_back(kKeyMapStatus[i]);
    }

    for (set<string>::const_iterator it = commands.begin();
         it != commands.end(); ++it) {
      commands_.push_back(it->c_str());
    }
  }

  const std::vector<std::string> &status() { return status_; }
  const std::vector<std::string> &commands() { return commands_; }

 private:
  std::vector<std::string> status_;
  std::vector<std::string> commands_;
};
} // namespace


template<typename T>
class KeyMapEditorDialog : public GenericTableEditorDialog<T>
{
public:
    enum {
        ENTRY_NEW = 'ernw',
        ENTRY_REMOVE = 'errm',
        IMPORT_ATOK = 'imat',
        IMPORT_MSIME = 'imms',
        IMPORT_KOTOERI = 'imko',
        IMPORT_FROM = 'imfr',
        EXPORT_TO = 'exto',
    };
    enum {
        KEYMAP_UPDATED = 'kmup',
    };
    KeyMapEditorDialog(BWindow* parent, size_t column_size,
                            const std::string &current_roman_table);
    virtual ~KeyMapEditorDialog() {}

    virtual void MessageReceived(BMessage* msg);
protected:
  virtual void UpdateMenuStatus();
  virtual string GetDefaultFilename() const {
    return "keymap.txt";
  }
  virtual bool LoadFromStream(istream *is);
  virtual bool Update();
private:
  string invisible_keymap_table_;
  set<string> direct_mode_commands_;

  map<string, string> normalized_command_map_;
  map<string, string> normalized_status_map_;

  const char* windowTitle() { return this->Title(); }
  void _ImportFrom(int32 index);
};

template<typename T>
KeyMapEditorDialog<T>::KeyMapEditorDialog(BWindow* parent, size_t column_size,
                            const std::string &current_keymap)
    : GenericTableEditorDialog<T>(parent, column_size)
{
    this->SetTitle(B_TRANSLATE("Mozc keymap editor"));

    BPopUpMenu* import = new BPopUpMenu(B_TRANSLATE("Import predefined mapping"));
    import->SetRadioMode(false);
    BLayoutBuilder::Menu<>(import)
        .AddItem(B_TRANSLATE("ATOK"), new BMessage(IMPORT_ATOK))
        .AddItem(B_TRANSLATE("MS-IME"), new BMessage(IMPORT_MSIME))
        .AddItem(B_TRANSLATE("Kotoeri"), new BMessage(IMPORT_KOTOERI));

    BLayoutBuilder::Menu<>(this->mutable_edit_menu())
        .AddItem(B_TRANSLATE("New entry"), new BMessage(ENTRY_NEW))
        .AddItem(B_TRANSLATE("Remove selected entries"), new BMessage(ENTRY_REMOVE))
        .AddSeparator();
    this->mutable_edit_menu()->AddItem(import);
    BLayoutBuilder::Menu<>(this->mutable_edit_menu())
        .AddSeparator()
        .AddItem(B_TRANSLATE("Import from file..."), new BMessage(IMPORT_FROM))
        .AddItem(B_TRANSLATE("Export to file..."), new BMessage(EXPORT_TO));

    GridTitleView* pTitleView = this->mutable_table_widget()->TitleView();
    pTitleView->AddColumn(B_TRANSLATE("Mode"));
    pTitleView->AddColumn(B_TRANSLATE("Key"));
    pTitleView->AddColumn(B_TRANSLATE("Command"));

    KeyMapTableLoader *loader = Singleton<KeyMapTableLoader>::get();

    const std::vector<std::string> &statuses = loader->status();
    KeymapMenuFieldColumn<KeymapRow>* modeColumn = new KeymapMenuFieldColumn<KeymapRow>(
                this->mutable_table_widget(), 0, this->mutable_table_widget());
    for (size_t i = 0; i < statuses.size(); ++i) {
      const char* i18n_status = B_TRANSLATE_CONTEXT(statuses[i].c_str(), "keymap");
      modeColumn->AddLabel(i18n_status);
      normalized_status_map_.insert(
          make_pair(i18n_status, statuses[i]));
    }

    const std::vector<std::string> &commands = loader->commands();
    KeymapMenuFieldColumn<KeymapRow>* commandColumn = new KeymapMenuFieldColumn<KeymapRow>(
                this->mutable_table_widget(), 2, this->mutable_table_widget());
    for (size_t i = 0; i < commands.size(); ++i) {
      const char* i18n_command = B_TRANSLATE_CONTEXT(commands[i].c_str(), "keymap");
      commandColumn->AddLabel(i18n_command);
      normalized_command_map_.insert(
          make_pair(i18n_command, commands[i]));
    }

    this->mutable_table_widget()->AddColumn(modeColumn, true);
    this->mutable_table_widget()->AddColumn(
            new KeymapKeyColumn<KeymapRow>(
                this->mutable_table_widget(), 1, this->mutable_table_widget()), true);
    this->mutable_table_widget()->AddColumn(commandColumn, true);

    this->LoadFromString(current_keymap);

    this->Layout(true);
}

template<typename T>
bool KeyMapEditorDialog<T>::LoadFromStream(istream *is) {
  if (is == NULL) {
    return false;
  }

  string line;
  if (!getline(*is, line)) {   // must have 1st line
    return false;
  }

  vector<string> fields;
  int row = 0;

  invisible_keymap_table_.clear();
  direct_mode_commands_.clear();
  while (getline(*is, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    Util::ChopReturns(&line);

    fields.clear();
    Util::SplitStringUsing(line, "\t", &fields);
    if (fields.size() < 3) {
      VLOG(3) << "field size < 3";
      continue;
    }

    const string &status = fields[0];
    const string &key = fields[1];
    const string &command = fields[2];

    // don't accept invalid keymap entries.
    if (!Singleton<KeyMapValidator>::get()->IsValidEntry(fields)) {
      VLOG(3) << "invalid entry.";
      continue;
    }

    // don't show invisible (not configurable) keymap entries.
    if (!Singleton<KeyMapValidator>::get()->IsVisibleEntry(fields)) {
      VLOG(3) << "invalid entry to show. add to invisible_keymap_table_";
      invisible_keymap_table_ += status;
      invisible_keymap_table_ += '\t';
      invisible_keymap_table_ += key;
      invisible_keymap_table_ += '\t';
      invisible_keymap_table_ += command;
      invisible_keymap_table_ += '\n';
      continue;
    }

    if (status == kDirectMode) {
      direct_mode_commands_.insert(key);
    }
    this->mutable_table_widget()->AddRow(
            new T(B_TRANSLATE_CONTEXT(status.c_str(), "keymap"),
                  key.c_str(),
                  B_TRANSLATE_CONTEXT(command.c_str(), "keymap")));
    ++row;
  }

  UpdateMenuStatus();

  return true;
}

template<typename T>
bool KeyMapEditorDialog<T>::Update() {
  if (this->mutable_table_widget()->CountRows() == 0) {
    QMessageBox::warning(this,
                         windowTitle(),
                         tr("Current keymap table is empty. "
                            "You might want to import a pre-defined "
                            "keymap table first."));
    return false;
  }

  set<string> new_direct_mode_commands;

  KeyMapValidator *validator = Singleton<KeyMapValidator>::get();
  string *keymap_table = this->mutable_table();

  *keymap_table = "status\tkey\tcommand\n";

  for (int i = 0; i < this->mutable_table_widget()->CountRows(); ++i) {
    T* row = this->mutable_table_widget()->ItemAt(i);
    if (!row) {
        continue;
    }
    const string i18n_status = row->Mode();
    const string key = row->Key();
    const string i18n_command = row->Command();

    const map<string, string>::const_iterator status_it =
        normalized_status_map_.find(i18n_status);
    if (status_it == normalized_status_map_.end()) {
      LOG(ERROR) << "Unsupported i18n status name: " << i18n_status;
      continue;
    }
    const string &status = status_it->second;

    const map<string, string>::const_iterator command_it =
        normalized_command_map_.find(i18n_command);
    if (command_it == normalized_command_map_.end()) {
      LOG(ERROR) << "Unsupported i18n command name:" << i18n_command;
      continue;
    }
    const string &command = command_it->second;

    if (!validator->IsVisibleKey(key)) {
      QMessageBox::warning(this,
                           windowTitle(),
                           (QString(tr("Invalid key:\n%1"))
                            .arg(key.c_str()).getp()));
      return false;
    }
    const string keymap_line = status + "\t" + key + "\t" + command;
    *keymap_table += keymap_line;
    *keymap_table += '\n';

    if (status == kDirectMode) {
      new_direct_mode_commands.insert(key);
    }
  }
  *keymap_table += invisible_keymap_table_;

  if (new_direct_mode_commands != direct_mode_commands_) {
#if defined(OS_WIN) || defined(OS_LINUX)
    QMessageBox::information(
        this,
        windowTitle(),
        tr("Changes of keymaps for direct input mode will apply only to "
           "applications that are launched after making your "
           "modifications."));
#endif  // OS_WIN || OS_LINUX
    direct_mode_commands_ = new_direct_mode_commands;
  }

  return true;
}

template<typename T>
void KeyMapEditorDialog<T>::UpdateMenuStatus() {
  //const bool status = (this->mutable_table_widget()->CountRows() > 0);
  //actions_[REMOVE_INDEX]->setEnabled(status);
  //actions_[EXPORT_TO_FILE_INDEX]->setEnabled(status);
  //UpdateOKButton(status);
}

template<typename T>
void KeyMapEditorDialog<T>::_ImportFrom(int32 import_index) {
  if (import_index >= 0 &&
           import_index < arraysize(kKeyMaps)) {
    const char *keymap_file =
      keymap::KeyMapManager::GetKeyMapFileName(kKeyMaps[import_index]);
    std::unique_ptr<istream> ifs(
      ConfigFileStream::LegacyOpen(keymap_file));
    CHECK(ifs.get() != NULL);  // should never happen
    CHECK(LoadFromStream(ifs.get()));
  }
}

template<typename T>
void KeyMapEditorDialog<T>::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case GenericTableEditorDialog<T>::OK:
        {
            if (Update()) {
                if (this->mpParent) {
                    BMessage message(KEYMAP_UPDATED);
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
        case IMPORT_ATOK:
        {
            _ImportFrom(0);
            break;
        }
        case IMPORT_MSIME:
        {
            _ImportFrom(1);
            break;
        }
        case IMPORT_KOTOERI:
        {
            _ImportFrom(2);
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
        default:
            GenericTableEditorDialog<T>::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc

#endif // KEYMAP_EDITOR_H
