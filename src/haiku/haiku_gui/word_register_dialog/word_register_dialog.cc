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

// ported from gui/word_register_dialog

#include "haiku/haiku_gui/word_register_dialog/word_register_dialog.h"
#include "haiku/haiku_gui/base/compatible.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <StringItem.h>
#include <TextControl.h>
#include <Window.h>

#include <string>
#include <vector>

#include "base/const.h"
#include "base/logging.h"
#include "base/util.h"
#include "client/client.h"
#include "data_manager/pos_list_provider.h"
#include "dictionary/user_dictionary_session.h"
#include "dictionary/user_dictionary_storage.h"
#include "dictionary/user_dictionary_util.h"
#include "protocol/user_dictionary_storage.pb.h"

#define tr(s) B_TRANSLATE(s)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "word_register_dialog"

namespace mozc {
namespace haiku_gui {

using mozc::user_dictionary::UserDictionary;
using mozc::user_dictionary::UserDictionaryCommandStatus;
using mozc::user_dictionary::UserDictionarySession;
using mozc::user_dictionary::UserDictionaryStorage;

const int kSessionTimeout = 100000;
const int kMaxEditBytes = 300;
const int kMaxReverseConversionLength = 30;

QString GetEnv(const char *envname) {
  return ::getenv(envname);
}

class WordRegisterDialog : public BWindow
{
public:
    WordRegisterDialog();
    virtual ~WordRegisterDialog();

    void MessageReceived(BMessage *msg);

    bool IsAvailable() const;
    void Clicked(QAbstractButton *button);
    void LineEditChanged();
    void LaunchDictionaryTool();

private:
    enum MessageType
    {
        INFORMATION,
        WARNING,
    };

    enum ErrorCode
    {
        SAVE_SUCCESS,
        SAVE_FAILURE,
        INVALID_KEY,
        INVALID_VALUE,
        EMPTY_KEY,
        EMPTY_VALUE,
        FATAL_ERROR,
    };

    ErrorCode SaveEntry();
    void UpdateUIStatus();
    void SetDefaultEntryFromClipboard();
    bool SetDefaultEntryFromEnvironmentVariable();
    const QString GetReading(const QString& value);
    const QString TrimValue(const QString& value) const;

    void _MessageBox(MessageType type, const char* title, const char* message);

    bool is_available_;
    std::unique_ptr<mozc::user_dictionary::UserDictionarySession> session_;
    std::unique_ptr<client::ClientInterface> client_;
    const char* window_title_;
    std::unique_ptr<const POSListProviderInterface> pos_list_provider_;

    enum Actions
    {
        OK = 'btok',
        CANCEL = 'btcl',
        EDIT_DICT = 'btdt',
        WORD = 'word',
        READING = 'read',
    };
    BTextControl*   fWordTC;
    BTextControl*   fReadingTC;
    BMenuField*     fPartOfSpeechMF;
    BPopUpMenu*     fPartOfSpeechPM;
    BMenuField*     fDictionaryMF;
    BPopUpMenu*     fDictionaryPM;
    BButton*        fOkButton;
    BButton*        fCancelButton;

    CompEdit*       WordlineEdit;
    CompEdit*       ReadinglineEdit;
    CompComboBox*   PartOfSpeechcomboBox;
    CompComboBox*   DictionarycomboBox;
    QDialogButtonBox* WordRegisterDialogbuttonBox;
};


WordRegisterDialog::WordRegisterDialog()
    : BWindow(
        BRect(50, 50, 340, 180),
        B_TRANSLATE("Mozc Word Register Dialog"),
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
        B_QUIT_ON_WINDOW_CLOSE | B_CLOSE_ON_ESCAPE),
      is_available_(true),
      session_(new UserDictionarySession(
          UserDictionaryUtil::GetUserDictionaryFileName())),
      client_(client::ClientFactory::NewClient()),
      window_title_(tr("Mozc")),
      pos_list_provider_(new POSListProvider())
{
    fWordTC = new BTextControl("wordTC", "", "", new BMessage(WORD));
    fWordTC->TextView()->SetMaxBytes(kMaxEditBytes);
    fReadingTC = new BTextControl("readingTC", "", "", new BMessage(READING));
    fReadingTC->TextView()->SetMaxBytes(kMaxEditBytes);

    fPartOfSpeechPM = new BPopUpMenu("partofspeechPM");
    fPartOfSpeechPM->SetRadioMode(true);
    fPartOfSpeechMF = new BMenuField("partofspeechMF", "", fPartOfSpeechPM);
    fDictionaryPM = new BPopUpMenu("dictionaryPM");
    fDictionaryPM->SetRadioMode(true);
    fDictionaryMF = new BMenuField("dictionaryMF", "", fDictionaryPM);

    BView *spacer = new BView("empty", B_FULL_UPDATE_ON_RESIZE);
    spacer->SetResizingMode(B_FOLLOW_NONE);
    spacer->ResizeTo(60, 5);

    fOkButton = new BButton(B_TRANSLATE("OK"), new BMessage(OK));
    fCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(6, 6, 6, 6)
        .AddGrid()
            .Add(new BStringView("wordL", B_TRANSLATE("Word")), 0, 0)
            .Add(fWordTC, 1, 0)
            .Add(new BStringView("readingL", B_TRANSLATE("Reading")), 0, 1)
            .Add(fReadingTC, 1, 1)
            .Add(new BStringView("partofspeechL", B_TRANSLATE("Part of Speech")), 0, 2)
            .AddGroup(B_HORIZONTAL, 0, 1, 2)
                .Add(fPartOfSpeechMF)
                .Add(spacer)
            .End()
            .Add(new BStringView("dictionaryL", B_TRANSLATE("Dictionary")), 0, 3)
            .Add(fDictionaryMF, 1, 3)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new BButton(B_TRANSLATE("Edit user dictionary..."),
                    new BMessage(EDIT_DICT)))
            .AddGroup(B_HORIZONTAL)
                .Add(fOkButton)
                .Add(fCancelButton)
            .End()
        .End();
    Layout(true);
    SetDefaultButton(fOkButton);

    WordlineEdit = new CompEdit(fWordTC);
    ReadinglineEdit = new CompEdit(fReadingTC);
    PartOfSpeechcomboBox = new CompComboBox(fPartOfSpeechPM);
    DictionarycomboBox = new CompComboBox(fDictionaryPM);
    WordRegisterDialogbuttonBox = new QDialogButtonBox(fOkButton, fCancelButton);

  if (!SetDefaultEntryFromEnvironmentVariable()) {
    SetDefaultEntryFromClipboard();
  }

  client_->set_timeout(kSessionTimeout);

  if (session_->Load() !=
      UserDictionaryCommandStatus::USER_DICTIONARY_COMMAND_SUCCESS) {
    LOG(WARNING) << "UserDictionarySession::Load() failed";
  }

  if (!session_->mutable_storage()->Lock()) {
    QMessageBox::information(
        this, window_title_,
        tr("Close dictionary tool before using word register dialog."));
    is_available_ = false;
    return;
  }

  // Initialize ComboBox
  vector<string> pos_set;
  pos_list_provider_->GetPOSList(&pos_set);
  CHECK(!pos_set.empty());

  for (size_t i = 0; i < pos_set.size(); ++i) {
    CHECK(!pos_set[i].empty());
    PartOfSpeechcomboBox->addItem(pos_set[i].c_str());
  }

  // Create new dictionary if empty
  if (!session_->mutable_storage()->Exists() ||
      session_->storage().dictionaries_size() == 0) {
    const QString name = tr("User Dictionary 1");
    uint64 dic_id = 0;
    if (!session_->mutable_storage()->CreateDictionary(
            name.toStdString(), &dic_id)) {
      LOG(ERROR) << "Failed to create a new dictionary.";
      is_available_ = false;
      return;
    }
  }

  // Load Dictionary List
  {
    const UserDictionaryStorage &storage = session_->storage();
    CHECK_GT(storage.dictionaries_size(), 0);
    for (size_t i = 0; i < storage.dictionaries_size(); ++i) {
      DictionarycomboBox->addItem(storage.dictionaries(i).name().c_str());
    }
  }

  if (!WordlineEdit->text().isEmpty()) {
    ReadinglineEdit->setFocus(Qt::OtherFocusReason);
    if (!ReadinglineEdit->text().isEmpty()) {
      ReadinglineEdit->selectAll();
    }
  }

    if (fWordTC->TextView()->TextLength() != 0) {
        fReadingTC->MakeFocus(true);
        if (fReadingTC->TextView()->TextLength() != 0) {
            fReadingTC->TextView()->SelectAll();
        }
    } else {
        fWordTC->MakeFocus(true);
    }

  UpdateUIStatus();

  // select first item of the menu field
  PartOfSpeechcomboBox->selectFirst();
  DictionarycomboBox->selectFirst();
}

WordRegisterDialog::~WordRegisterDialog()
{
    delete WordlineEdit;
    delete ReadinglineEdit;
    delete PartOfSpeechcomboBox;
    delete DictionarycomboBox;
    delete WordRegisterDialogbuttonBox;
}

bool WordRegisterDialog::IsAvailable() const {
  return is_available_;
}

void WordRegisterDialog::LineEditChanged()
{
  UpdateUIStatus();
}

void WordRegisterDialog::UpdateUIStatus() {
  const bool enabled =
      !ReadinglineEdit->text().isEmpty() &&
      !WordlineEdit->text().isEmpty();

  QAbstractButton *button =
      WordRegisterDialogbuttonBox->button(QDialogButtonBox::Ok);
  if (button != NULL) {
    button->setEnabled(enabled);
  }
}

void WordRegisterDialog::Clicked(QAbstractButton *button) {
  switch (WordRegisterDialogbuttonBox->buttonRole(button)) {
    case QDialogButtonBox::AcceptRole:
      switch (SaveEntry()) {
        case EMPTY_KEY:
        case EMPTY_VALUE:
          LOG(FATAL) << "key/value is empty. This case will never occur.";
          return;
        case INVALID_KEY:
          QMessageBox::warning(
              this, window_title_,
              tr("Reading part contains invalid characters."));
          return;
        case INVALID_VALUE:
          QMessageBox::warning(
              this, window_title_,
              tr("Word part contains invalid characters."));
          return;
        case FATAL_ERROR:
          QMessageBox::warning(
              this, window_title_, tr("Unexpected error occurs."));
          break;
        case SAVE_FAILURE:
          QMessageBox::warning(
              this, window_title_, tr("Failed to update user dictionary."));
          break;
        case SAVE_SUCCESS:
          break;
        default:
          return;
      }
      Quit();
      break;
    default:
      break;
  }
}

WordRegisterDialog::ErrorCode WordRegisterDialog::SaveEntry() {
  const string key = ReadinglineEdit->text().toStdString();
  const string value = WordlineEdit->text().toStdString();
  UserDictionary::PosType pos = UserDictionaryUtil::ToPosType(
      PartOfSpeechcomboBox->currentText().toStdString().c_str());

  if (key.empty()) {
    return EMPTY_KEY;
  }

  if (value.empty()) {
    return EMPTY_VALUE;
  }

  if (!UserDictionaryUtil::IsValidReading(key)) {
    return INVALID_KEY;
  }

  if (!UserDictionary::PosType_IsValid(pos)) {
    LOG(ERROR) << "POS is invalid";
    return FATAL_ERROR;
  }

  const int index = DictionarycomboBox->currentIndex();
  if (index < 0 || index >= session_->storage().dictionaries_size()) {
    LOG(ERROR) << "index is out of range";
    return FATAL_ERROR;
  }

  UserDictionary *dic =
      session_->mutable_storage()->mutable_dictionaries(index);
  CHECK(dic);

  if (dic->name() != DictionarycomboBox->currentText().toStdString()) {
    LOG(ERROR) << "Inconsitent dictionary name";
    return FATAL_ERROR;
  }

  UserDictionary::Entry *entry = dic->add_entries();
  CHECK(entry);
  entry->set_key(key);
  entry->set_value(value);
  entry->set_pos(pos);

  if (!session_->mutable_storage()->Save() &&
      session_->mutable_storage()->GetLastError() ==
      mozc::UserDictionaryStorage::SYNC_FAILURE) {
    LOG(ERROR) << "Cannot save dictionary";
    return SAVE_FAILURE;
  }

  if (!client_->PingServer()) {
    LOG(WARNING) << "Server is not running. Do nothing";
    return SAVE_SUCCESS;
  }

  if (!client_->Reload()) {
    LOG(ERROR) << "Reload command failed";
    return SAVE_SUCCESS;
  }

  return SAVE_SUCCESS;
}

void WordRegisterDialog::LaunchDictionaryTool()
{
  session_->mutable_storage()->UnLock();
  client_->LaunchTool("dictionary_tool", "");
  Quit();
}

const QString WordRegisterDialog::GetReading(const QString &str) {
  if (str.isEmpty()) {
    LOG(ERROR) << "given string is empty";
    return "";
  }

  if (str.count() >= kMaxReverseConversionLength) {
    LOG(ERROR) << "too long input";
    return "";
  }

  commands::Output output;
  {
    commands::KeyEvent key;
    key.set_special_key(commands::KeyEvent::ON);
    if (!client_->SendKey(key, &output)) {
      LOG(ERROR) << "SendKey failed";
      return "";
    }

    commands::SessionCommand command;
    command.set_type(commands::SessionCommand::CONVERT_REVERSE);
    command.set_text(str.toStdString());

    if (!client_->SendCommand(command, &output)) {
      LOG(ERROR) << "SendCommand failed";
      return "";
    }

    commands::Output dummy_output;
    command.set_type(commands::SessionCommand::REVERT);
    client_->SendCommand(command, &dummy_output);
  }

  if (!output.has_preedit()) {
    LOG(ERROR) << "No preedit";
    return "";
  }

  string key;
  for (size_t segment_index = 0;
       segment_index < output.preedit().segment_size();
       ++segment_index) {
    const commands::Preedit::Segment &segment =
        output.preedit().segment(segment_index);
    if (!segment.has_key()) {
      LOG(ERROR) << "No segment";
      return "";
    }
    key.append(segment.key());
  }

  if (key.empty() ||
      !UserDictionaryUtil::IsValidReading(key)) {
    LOG(WARNING) << "containing invalid characters";
    return "";
  }

  return QString(key.c_str());
}

void WordRegisterDialog::SetDefaultEntryFromClipboard()
{
    const char* text;
    ssize_t len = 0;
    if (be_clipboard->Lock()) {
        BMessage *clip = be_clipboard->Data();
        clip->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &len);
        // utf-8?
        be_clipboard->Unlock();

        if (len > 0) {
            std::string value(text, len);

            WordlineEdit->setText(value.c_str());
            ReadinglineEdit->setText(GetReading(QString(value.c_str())));
        }
    }
}

bool WordRegisterDialog::SetDefaultEntryFromEnvironmentVariable() {
  const QString entry = TrimValue(GetEnv(mozc::kWordRegisterEnvironmentName));
  if (entry.isEmpty()) {
    return false;
  }
  WordlineEdit->setText(entry);

  QString reading_string =
      TrimValue(GetEnv(mozc::kWordRegisterEnvironmentReadingName));
  if (reading_string.isEmpty()) {
    reading_string = GetReading(entry);
  }
  ReadinglineEdit->setText(reading_string);

  return true;
}

const QString WordRegisterDialog::TrimValue(const QString& value) const
{
    BString s(value.get().c_str());
    return QString(s.Trim().ReplaceAll("\r", "", 0).ReplaceAll("\n", "", 0).String());
}

void WordRegisterDialog::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case OK:
        {
            Clicked(WordRegisterDialogbuttonBox->button(QDialogButtonBox::Ok));
            break;
        }
        case CANCEL:
        {
            Quit();
            break;
        }
        case WORD:
        {
            LineEditChanged();
            break;
        }
        case READING:
        {
            LineEditChanged();
            break;
        }
        case EDIT_DICT:
        {
            LaunchDictionaryTool();
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

void WordRegisterDialog::_MessageBox(MessageType type,
                const char* title, const char* message)
{
    alert_type t = type == MessageType::WARNING ? B_WARNING_ALERT : B_INFO_ALERT;
    BAlert* alert = new BAlert(title, message,
                    "", "", "OK", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, t);
    alert->SetShortcut(0, B_ESCAPE);
    alert->Go();
}


class WordRegisterApp : public ToolApp
{
public:
    WordRegisterApp();
    virtual ~WordRegisterApp() {}
};

WordRegisterApp::WordRegisterApp()
    : ToolApp(WORD_REGISTER_DIALOG)
{
    mpWindow = new WordRegisterDialog();
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}

} // namespace haiku_gui
} // namespace mozc

int HaikuRunWordRegisterDialog(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                        mozc::haiku_gui::WORD_REGISTER_DIALOG)) {
        return -1;
    }

    mozc::haiku_gui::WordRegisterApp *app = new mozc::haiku_gui::WordRegisterApp();
    app->Run();
    delete app;

    return 0;
}
