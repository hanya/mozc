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

// ported from gui/dictionary_tool

#include "haiku/haiku_gui/dictionary_tool/dictionary_tool.h"
#include "haiku/haiku_gui/base/compatible.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/base/cstring_view.h"
#include "haiku/haiku_gui/dictionary_tool/dictionary_list.h"
#include "haiku/haiku_gui/dictionary_tool/dictionary_grid.h"
#include "haiku/haiku_gui/dictionary_tool/import_dialog.h"
#include "haiku/haiku_gui/dictionary_tool/find_dialog.h"

#include <Alignment.h>
#include <Button.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuField.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorItem.h>
#include <StringItem.h>
#include <StringView.h>
#include <UTF8.h>
#include <Window.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/file_stream.h"
#include "base/logging.h"
#include "base/run_level.h"
#include "base/util.h"
#include "client/client.h"
#include "data_manager/pos_list_provider.h"
#include "dictionary/user_dictionary_importer.h"
#include "dictionary/user_dictionary_session.h"
#include "dictionary/user_dictionary_storage.h"
#include "dictionary/user_dictionary_util.h"
#include "gui/base/encoding_util.h"
#include "gui/base/msime_user_dictionary_importer.h"
#include "gui/base/win_util.h"
#include "protocol/user_dictionary_storage.pb.h"

#define tr(s) B_TRANSLATE(s)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "dictionary_tool"


namespace mozc {
namespace haiku_gui {

using ::mozc::user_dictionary::UserDictionary;
using ::mozc::user_dictionary::UserDictionaryCommandStatus;
using ::mozc::user_dictionary::UserDictionarySession;
using ::mozc::user_dictionary::UserDictionaryStorage;


class DictionaryContentTableWidget
{
public:
    DictionaryContentTableWidget(GridView<DictContentRow>* p) { p_ = p; }
    virtual ~DictionaryContentTableWidget() { p_ = NULL; }

    int32 rowCount() const;
    void clearContents();
    void setRowCount(int32 n);
    void setEnabled(bool enabled);
private:
    GridView<DictContentRow>* p_;
};

int32 DictionaryContentTableWidget::rowCount() const
{
    if (p_) {
        return p_->CountRows();
    }
    return 0;
}

void DictionaryContentTableWidget::clearContents()
{
    if (p_) {
        p_->MakeEmpty();
    }
}

void DictionaryContentTableWidget::setRowCount(int32 n)
{
}

void DictionaryContentTableWidget::setEnabled(bool enabled)
{
    if (p_) {
        if (p_->LockLooper()) {
            p_->Invalidate();
            p_->UnlockLooper();
        }
    }
}


const int kSessionTimeout = 100000;

class UTF16TextLineIterator
    : public UserDictionaryImporter::TextLineIteratorInterface {
 public:
  UTF16TextLineIterator(
      UserDictionaryImporter::EncodingType encoding_type,
      const string &filename,
      const QString &message, BWindow *parent)
      : ifs_(new InputFileStream(filename.c_str())) {
    CHECK_EQ(UserDictionaryImporter::UTF16, encoding_type);
    const std::streampos begin = ifs_->tellg();
    ifs_->seekg(0, ios::end);
    const size_t size = static_cast<size_t>(ifs_->tellg() - begin);
    ifs_->seekg(0, ios::beg);
    progress_ = new ProgressDialogWrapper(message.getp(), parent, size);
  }

  virtual ~UTF16TextLineIterator() {
    delete progress_;
  }

  bool IsAvailable() const {
    return ifs_->good() || ifs_->eof();
  }

  bool Next(string *line) {
    if (!ifs_->good()) {
      return false;
    }

    char ch;
    string output_line;
    while (ifs_->good()) {
      ifs_->get(ch);
      if (output_line.empty() && ch == '\n') {
        continue;
      }
      if (ch == '\n') {
        break;
      } else {
        output_line += ch;
      }
    }

    progress_->setValue(ifs_->tellg());

    int32 destLength = output_line.size() * 2;
    char dest[destLength + 1];
    int32 state = 0;
    int32 length = output_line.size();
    convert_to_utf8(B_UTF16_CONVERSION, output_line.c_str(), &length,
                dest, &destLength, &state);
    dest[destLength] = '\0';
    string output;
    output += dest;

    *line = output;

    Util::ChopReturns(line);
    return true;
  }

  void Reset() {
    ifs_->clear();
    ifs_->seekg(0, ios_base::beg);
  }

 private:
  std::unique_ptr<InputFileStream> ifs_;
  ProgressDialogWrapper* progress_;
};

namespace {

class MultiByteTextLineIterator
    : public UserDictionaryImporter::TextLineIteratorInterface {
 public:
  MultiByteTextLineIterator(
      UserDictionaryImporter::EncodingType encoding_type,
      const string &filename,
      const QString &message, BWindow *parent)
      : encoding_type_(encoding_type),
        ifs_(new InputFileStream(filename.c_str())),
        first_line_(true) {
    const std::streampos begin = ifs_->tellg();
    ifs_->seekg(0, ios::end);
    const size_t size = static_cast<size_t>(ifs_->tellg() - begin);
    ifs_->seekg(0, ios::beg);
    progress_ = new ProgressDialogWrapper(message.getp(), parent, size);
  }

  virtual ~MultiByteTextLineIterator() {
    delete progress_;
  }

  bool IsAvailable() const {
    // This means that neither failbit nor badbit is set.
    // TODO(yukawa): Consider to remove |ifs_->eof()|. Furthermore, we should
    // replace IsAvailable() with something easier to understand, e.g.,
    // Done() or HasNext().
    return ifs_->good() || ifs_->eof();
  }

  bool Next(string *line)  {
    if (!ifs_->good()) {
      return false;
    }

    // Can't use getline as getline doesn't support CR only text.
    char ch;
    string output_line;
    while (ifs_->good()) {
      ifs_->get(ch);
      if (output_line.empty() && ch == '\n') {
        continue;
      }
      if (ch == '\n' || ch == '\r') {
        break;
      } else {
        output_line += ch;
      }
    }

    progress_->setValue(ifs_->tellg());

    *line = output_line;

    // We can't use QTextCodec as QTextCodec is not enabled by default.
    // We won't enable it as it increases the binary size.
    if (encoding_type_ == UserDictionaryImporter::SHIFT_JIS) {
      const string input = *line;
      EncodingUtil::SJISToUTF8(input, line);
    }

    // strip UTF8 BOM
    if (first_line_ &&
        encoding_type_ == UserDictionaryImporter::UTF8) {
      Util::StripUTF8BOM(line);
    }

    Util::ChopReturns(line);

    first_line_ = false;
    return true;
  }

  void Reset() {
    // Clear state bit (eofbit, failbit, badbit).
    ifs_->clear();
    ifs_->seekg(0, ios_base::beg);
    first_line_ = true;
  }

 private:
  UserDictionaryImporter::EncodingType encoding_type_;
  std::unique_ptr<InputFileStream> ifs_;
  ProgressDialogWrapper* progress_;
  bool first_line_;
};

UserDictionaryImporter::TextLineIteratorInterface *CreateTextLineIterator(
    UserDictionaryImporter::EncodingType encoding_type,
    const string &filename,
    BWindow *parent) {
  if (encoding_type == UserDictionaryImporter::ENCODING_AUTO_DETECT) {
    encoding_type = UserDictionaryImporter::GuessFileEncodingType(filename);
  }

  if (encoding_type == UserDictionaryImporter::NUM_ENCODINGS) {
    LOG(ERROR) << "GuessFileEncodingType() returns UNKNOWN encoding.";
    // set default encoding
#ifdef OS_WIN
    encoding_type = UserDictionaryImporter::SHIFT_JIS;
#else
    encoding_type = UserDictionaryImporter::UTF16;
#endif
  }

  VLOG(1) << "Setting Encoding to: " << static_cast<int>(encoding_type);

  const QString message = tr("Importing new words...");

  switch (encoding_type) {
    case UserDictionaryImporter::UTF8:
    case UserDictionaryImporter::SHIFT_JIS:
      return new MultiByteTextLineIterator(encoding_type, filename,
                                           message, parent);
      break;
    case UserDictionaryImporter::UTF16:
      return new UTF16TextLineIterator(encoding_type, filename,
                                       message, parent);
      break;
    default:
      return NULL;
  }

  return NULL;
}
}  // namespace


class DictionaryTool : public BWindow
{
public:
    DictionaryTool();
    virtual ~DictionaryTool();
    virtual void MessageReceived(BMessage *msg);
    virtual bool QuitRequested();
    virtual void WindowActivated(bool active);

  bool IsAvailable() const {
    return is_available_;
  }

private:
    enum Actions
    {
        TOOLS = 'tool',
        EXPORT_DICT = 'exdt',

        DICT_NEW = 'dtnw',
        DICT_RENAME = 'dtrn',
        DICT_DELETE = 'dtdl',
        FIND = 'find',
        IMPORT_AS = 'imas',
        IMPORT_TO = 'imto',
        EXPORT_CURRENT = 'excu',

        WORD_ADD = 'wdad',
        WORD_MOVE = 'wdmv',
        WORD_DELETE = 'wddl',
        WORD_CATEGORY_EDIT = 'wdct',
        WORD_COMMENT_EDIT = 'wdce',

        DICT_SELECTION_CHANGED = 'dtsc',
        DICT_LIST_MENU = 'dtmn',
        ENTRIES_MENU = 'enmn',
    };
    BStringView*     fStatusText;
    DicList*         fDicList;
    GridView<DictContentRow>*        fWordLV;
    BButton*         fToolButton;
    BPopUpMenu*      fToolsMenu;
    BPopUpMenu*      fDicListMenu;
    BPopUpMenu*      fDicMenu;
    BPopUpMenu*      fMoveMenu;
    BPopUpMenu*      fPOSMenu;
    int32            mnLastDicIndex;
    std::unique_ptr<BFilePanel> mpExportFilePanel;

  void CreateDictionary();
  void _CreateDictionary(const char* name);
  void DeleteDictionary();
  void RenameDictionary();
  void _RenameDictionary(const char* name);
  void ImportAndCreateDictionary();
  void ImportAndAppendDictionary();
  //void ImportFromDefaultIME();
  void ExportDictionary();
  void _ExportDictionary(const char* file_name, uint64 id);
  void AddWord();
  void DeleteWord();
  void CloseWindow();
  void UpdateUIStatus();

  void OnDictionarySelectionChanged();
  void OnDeactivate();

  // Data type to provide information on a dictionary.
  struct DictionaryInfo {
    int row;                // Row in the list widget.
    uint64 id;              // ID of the dictionary.
    DicItem* item;
  };

  // Returns information on the current dictionary.
  DictionaryInfo current_dictionary() const;

  void SyncToStorage();

  void SetupDicContentEditor(const DictionaryInfo &dic_info);

  void CreateDictionaryHelper(const QString &dic_name);

  bool InitDictionaryList();

  void PromptForDictionaryName(const QString &text,
                                  const QString &label, int32 what);

  void ReportError();

  void ReportImportError(UserDictionaryImporter::ErrorType error,
                         const string &dic_name,
                         int added_entries_size);

  void ImportHelper(uint64 dic_id,
                    const string &dic_name,
                    const string &file_name,
                    UserDictionaryImporter::IMEType,
                    UserDictionaryImporter::EncodingType encoding_type);

  void SaveAndReloadServer();

  void EditComment();
  void _EditComment(const char* comment);

  void EditPOS(int32 pos);

  void MoveTo(uint64 id);

  static bool IsWritableToExport(const string &file_name);
  static bool IsReadableToImport(const string &file_name);

  void GetSortedSelectedRows(vector<int> *rows) const;

  DicItem* GetFirstSelectedDictionary() const;

  ImportDialog* import_dialog_;
  FindDialog* find_dialog_;
  std::unique_ptr<mozc::user_dictionary::UserDictionarySession> session_;

  uint64 current_dic_id_;

  bool modified_;

  bool monitoring_user_edit_;

  const char* window_title_;

  QPushButton *new_word_button_;
  QPushButton *delete_word_button_;

  QAction *new_action_;
  QAction *rename_action_;
  QAction *delete_action_;
  QAction *find_action_;
  QAction *import_create_action_;
  QAction *import_append_action_;
  QAction *export_action_;
//  QAction *import_default_ime_action_;

  QString statusbar_message_;

  std::unique_ptr<client::ClientInterface> client_;

  bool is_available_;

  int max_entry_size_;

  std::unique_ptr<const POSListProviderInterface> pos_list_provider_;


  QListWidget* dic_list_;
  DictionaryContentTableWidget* dic_content_;
  QStatusBar* statusbar_;

  QAction* AddToolsItem(const char* label, int32 n);
  BPoint _MapToScreen(BView* view, BPoint pos);
};

DictionaryTool::DictionaryTool()
    : BWindow(
        BRect(50, 50, 700, 420),
        B_TRANSLATE("Mozc Dictionary Tool"),
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_QUIT_ON_WINDOW_CLOSE),
      session_(new UserDictionarySession(
          UserDictionaryUtil::GetUserDictionaryFileName())),
      current_dic_id_(0), modified_(false), monitoring_user_edit_(false),
      window_title_(tr("Mozc")),
      new_action_(NULL), rename_action_(NULL),
      delete_action_(NULL), find_action_(NULL),
      import_create_action_(NULL), import_append_action_(NULL),
      export_action_(NULL),
      client_(client::ClientFactory::NewClient()),
      is_available_(true),
      max_entry_size_(mozc::UserDictionaryStorage::max_entry_size()),
      pos_list_provider_(new POSListProvider())
{
  import_dialog_ = NULL;

  find_dialog_ = NULL;

  mpExportFilePanel = std::unique_ptr<BFilePanel>(
                        new BFilePanel(B_SAVE_PANEL,
                            new BMessenger(this, NULL), NULL,
                            0, false,
                            NULL, NULL, true, true));

  client_->set_timeout(kSessionTimeout);

  if (session_->Load() !=
      UserDictionaryCommandStatus::USER_DICTIONARY_COMMAND_SUCCESS) {
    LOG(WARNING) << "UserDictionarySession::Load() failed";
  }

  if (!session_->mutable_storage()->Lock()) {
    QMessageBox::information(
        this, window_title_,
        tr("Another process is accessing the user dictionary file."));
    is_available_ = false;
    return;
  }

  // Get a list of POS and set a custom delagate that holds the list.
  vector<string> tmp_pos_vec;
  pos_list_provider_->GetPOSList(&tmp_pos_vec);

    fDicListMenu = new BPopUpMenu("listmenu");
    fDicListMenu->SetRadioMode(false);
    BLayoutBuilder::Menu<>(fDicListMenu)
        .AddItem(B_TRANSLATE("Rename..."), new BMessage(DICT_RENAME))
        .AddItem(B_TRANSLATE("Delete"), new BMessage(DICT_DELETE))
        .AddItem(B_TRANSLATE("Import to this dictionary..."), new BMessage(IMPORT_TO))
        .AddItem(B_TRANSLATE("Export this dictionary..."), new BMessage(EXPORT_CURRENT));

    fMoveMenu = new BPopUpMenu(B_TRANSLATE("Move this word to"));
    fMoveMenu->SetRadioMode(false);

    fPOSMenu = new BPopUpMenu(B_TRANSLATE("Change category to"));
    fPOSMenu->SetRadioMode(false);
    for (size_t i = 0; i < tmp_pos_vec.size(); ++i) {
        BMessage* msg = new BMessage(WORD_CATEGORY_EDIT);
        msg->AddInt32("pos", i + 1);
        fPOSMenu->AddItem(new BMenuItem(tmp_pos_vec[i].c_str(), msg));
    }

    fDicMenu = new BPopUpMenu("contentmenu");
    fDicMenu->SetRadioMode(false);
    BLayoutBuilder::Menu<>(fDicMenu)
        .AddItem(B_TRANSLATE("Add a word"), new BMessage(WORD_ADD))
        //.AddItem(B_TRANSLATE("Move this word to"), new BMessage(WORD_MOVE))
        .AddItem(B_TRANSLATE("Delete this word"), new BMessage(WORD_DELETE))
        .AddSeparator()
        //.AddItem(B_TRANSLATE("Change category to"), new BMessage(WORD_CATEGORY_EDIT))
        .AddItem(B_TRANSLATE("Edit comment"), new BMessage(WORD_COMMENT_EDIT));
    fDicMenu->AddItem(fMoveMenu, 1);
    fDicMenu->AddItem(fPOSMenu, 4);

    mnLastDicIndex = -1;
    fDicList = new DicList("dictLV", this, new BMessage(DICT_LIST_MENU));
    fDicList->SetDrawingMode(B_OP_OVER);
    fDicList->SetSelectionMessage(new BMessage(DICT_SELECTION_CHANGED));
    fDicList->SetTarget(this);
    BScrollView* fDictListView = new BScrollView("listview", fDicList,
                B_FRAME_EVENTS, false, true, B_FANCY_BORDER);

    dic_list_ = new QListWidget(fDicList);

    fWordLV = new GridView<DictContentRow>(MULTIPLE, this, new BMessage(ENTRIES_MENU));
    fWordLV->SetDrawingMode(B_OP_OVER);
    BScrollView* fDictContentListView = new BScrollView("contentview", fWordLV,
                B_FOLLOW_LEFT_TOP, B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
    dic_content_ = new DictionaryContentTableWidget(fWordLV);

    GridTitleView* pTitleView = new GridTitleView(true, fWordLV);
    pTitleView->AddColumn(B_TRANSLATE("Reading"));
    pTitleView->AddColumn(B_TRANSLATE("Word"));
    pTitleView->AddColumn(B_TRANSLATE("Category"));
    pTitleView->AddColumn(B_TRANSLATE("Comment"));
    fWordLV->SetTitleView(pTitleView);

    DictCategoryColumn<DictContentRow>* column =
                new DictCategoryColumn<DictContentRow>(fWordLV, 2, fWordLV);
    for (size_t i = 0; i < tmp_pos_vec.size(); ++i) {
        column->AddCategory(tmp_pos_vec[i].c_str());
    }

    fWordLV->AddColumn(new DictTextColumn<DictContentRow>(
                fWordLV, 0, fWordLV, fWordLV->InputView()), true);
    fWordLV->AddColumn(new DictTextColumn<DictContentRow>(
                fWordLV, 1, fWordLV, fWordLV->InputView()), true);
    fWordLV->AddColumn(column, true);
    fWordLV->AddColumn(new DictTextColumn<DictContentRow>(
                fWordLV, 3, fWordLV, fWordLV->InputView()), true);

    fStatusText = new CStringView("statusL", "");
    statusbar_ = new QStatusBar(fStatusText);

    fToolsMenu = new BPopUpMenu("toolsMenu");
    fToolsMenu->SetRadioMode(false);

    new_action_ = AddToolsItem(B_TRANSLATE("New dictionary..."), DICT_NEW);
    rename_action_ = AddToolsItem(B_TRANSLATE("Rename dictionary..."), DICT_RENAME);
    delete_action_ = AddToolsItem(B_TRANSLATE("Delete dictionary"), DICT_DELETE);
    find_action_ = AddToolsItem(B_TRANSLATE("Find..."), FIND);
    fToolsMenu->AddItem(new BSeparatorItem());
    import_create_action_ = AddToolsItem(B_TRANSLATE("Import as new dictionary..."), IMPORT_AS);
    import_append_action_ = AddToolsItem(B_TRANSLATE("Import to current dictionary..."), IMPORT_TO);
    export_action_ = AddToolsItem(B_TRANSLATE("Export current dictionary..."), EXPORT_CURRENT);

    fToolButton = new BButton(B_TRANSLATE("Tools"), new BMessage(TOOLS));
    fToolButton->SetBehavior(BButton::B_POP_UP_BEHAVIOR);
    fToolButton->SetPopUpMessage(new BMessage(TOOLS));

    BButton* addButton = new BButton(B_TRANSLATE("Add"), new BMessage(WORD_ADD));
    BButton* deleteButton = new BButton(B_TRANSLATE("Remove"), new BMessage(WORD_DELETE));
    new_word_button_ = new QPushButton(addButton);
    delete_word_button_ = new QPushButton(deleteButton);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL, 0)
            .SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP))
            .Add(fToolButton)
            .Add(addButton)
            .Add(deleteButton)
        .End()
        .AddSplit(B_HORIZONTAL, 3)
            .SetInsets(5, 5, 5, 0)
            .Add(fDictListView, 0.25)
            .Add(fDictContentListView, 0.75)
        .End()
        .AddGroup(B_HORIZONTAL, 0)
            .SetInsets(5, 0, 0, 0)
            .Add(fStatusText)
        .End();
    Layout(true);

  InitDictionaryList();

  if (dic_list_->count() != 0) {
    dic_list_->setCurrentRow(0);
  } else {
    // Make sure that the table widget is initialized when there is no
    // dictionary.
    OnDictionarySelectionChanged();
  }

  // If this is the first time for the user dictionary is used, create
  // a defautl dictionary.
  if (!session_->mutable_storage()->Exists()) {
    CreateDictionaryHelper(tr("User Dictionary 1"));
  }

  UpdateUIStatus();
}

QAction* DictionaryTool::AddToolsItem(const char* label, int32 n)
{
    BMenuItem* item = new BMenuItem(label, new BMessage(n));
    fToolsMenu->AddItem(item);
    return new QAction(item);
}

DictionaryTool::~DictionaryTool()
{
  delete fToolsMenu;
  delete fDicListMenu;
  delete fDicMenu;

  delete new_action_;
  delete rename_action_;
  delete delete_action_;
  delete find_action_;
  delete import_create_action_;
  delete import_append_action_;
  delete export_action_;

  delete new_word_button_;
  delete delete_word_button_;

  delete dic_list_;
  delete dic_content_;
  delete statusbar_;
}

//void DictionaryTool::closeEvent(QCloseEvent *event)
bool DictionaryTool::QuitRequested()
{
  fToolsMenu->MakeFocus(true);

  SyncToStorage();
  SaveAndReloadServer();

  if (session_->mutable_storage()->GetLastError()
      == mozc::UserDictionaryStorage::TOO_BIG_FILE_BYTES) {
    QMessageBox::warning(
        this, window_title_,
        tr("Making dangerously large user dictionary file. "
           "If the dictionary file turns out to be larger than 256Mbyte, "
           "the dictionary loader skips to handle all the words to prevent "
           "the converter from being halted."));
  }

    return true;
}

void DictionaryTool::OnDeactivate() {
  SyncToStorage();
  SaveAndReloadServer();
}

void DictionaryTool::WindowActivated(bool active)
{
    if (!active) {
        // todo
    }
}

/*
bool DictionaryTool::eventFilter(QObject *obj, QEvent *event) {
  // When application focus leaves, reload the server so
  // that new dictionary can be loaded.
  // This is an approximation of dynamic relading.
  //
  // We cannot call OnDeactivate() inside event filter,
  // as pending changes are not correctly synced to the disk.
  // Seems that all pending changes are committed to the UI
  // AFTER DictionaryTool receives ApplicationDeactivate event.
  // Here we delayed the execution of OnDeactivate() using QTimer.
  // This is an workaround for http://b/2190275.
  // TODO(taku): Find out a better way.
  if (event->type() == QEvent::ApplicationDeactivate) {
    const int kDelayOnDeactivateTime = 200;
    QTimer::singleShot(kDelayOnDeactivateTime, this, SLOT(OnDeactivate()));
  }
  return QWidget::eventFilter(obj, event);
}
*/

void DictionaryTool::OnDictionarySelectionChanged() {
  SyncToStorage();

  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    current_dic_id_ = 0;
    //StopMonitoringUserEdit();
    dic_content_->clearContents();
    dic_content_->setRowCount(0);
    dic_content_->setEnabled(false);
    //StartMonitoringUserEdit();
    new_word_button_->setEnabled(false);
    delete_word_button_->setEnabled(false);
    rename_action_->setEnabled(false);
    delete_action_->setEnabled(false);
    import_append_action_->setEnabled(false);
    export_action_->setEnabled(false);
  } else {
    current_dic_id_ = dic_info.id;
    max_entry_size_ = mozc::UserDictionaryStorage::max_entry_size();
    SetupDicContentEditor(dic_info);
  }
}

void DictionaryTool::SetupDicContentEditor(
    const DictionaryInfo &dic_info) {
  UserDictionary *dic =
      session_->mutable_storage()->GetUserDictionary(dic_info.id);

  if (dic == NULL) {
    LOG(ERROR) << "Failed to load the dictionary: " << dic_info.id;
    ReportError();
    return;
  }

  rename_action_->setEnabled(true);
  delete_action_->setEnabled(true);
  import_append_action_->setEnabled(true);
  export_action_->setEnabled(true);

  dic_content_->clearContents();
  dic_content_->setRowCount(dic->entries_size());

  fWordLV->LockUpdate();
  {
    ProgressDialogWrapper progress(
            tr("Updating the current view data..."),
            this,
            dic->entries_size());

    for (size_t i = 0; i < dic->entries_size(); ++i) {
      fWordLV->AddRow(new DictContentRow(
                    dic->entries(i).key().c_str(),
                    dic->entries(i).value().c_str(),
                    dic->entries(i).pos(),
                    dic->entries(i).comment().c_str()));
      ++progress;
    }
  }
  fWordLV->UnlockUpdate();
  fWordLV->SetModified(false);
  dic_content_->setEnabled(true);

  UpdateUIStatus();

  const bool dictionary_is_full = dic_content_->rowCount() >= max_entry_size_;
  new_word_button_->setEnabled(!dictionary_is_full);

  modified_ = false;
}

void DictionaryTool::CreateDictionary() {
  const int max_size =
      static_cast<int>(mozc::UserDictionaryStorage::max_dictionary_size());
  if (dic_list_->count() >= max_size) {
    QMessageBox::critical(
        this, window_title_,
        QString(tr("You can't have more than %1 dictionaries.")).arg(max_size).getp());
    return;
  }

  PromptForDictionaryName("", tr("Name of the new dictionary"), DICT_NEW);
}

void DictionaryTool::_CreateDictionary(const char* name) {
  if (name == NULL || strlen(name) <= 0) {
    return;
  }
  SyncToStorage();

  CreateDictionaryHelper(QString(name));
}

void DictionaryTool::DeleteDictionary() {
  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    QMessageBox::information(this, window_title_,
                             tr("No dictionary is selected."));
    return;
  }

  if (QMessageBox::question(
          this, window_title_,
          QString(tr("Do you want to delete %1?")).arg(dic_info.item->Text()).getp(),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
      != QMessageBox::Yes) {
    return;
  }

  if (!session_->mutable_storage()->DeleteDictionary(dic_info.id)) {
    LOG(ERROR) << "Failed to delete the dictionary.";
    ReportError();
    return;
  }

  modified_ = false;
  BListItem* item = fDicList->RemoveItem(dic_info.row);
  delete item;

  UpdateUIStatus();
}

void DictionaryTool::RenameDictionary() {
  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    QMessageBox::information(this, window_title_,
                             tr("No dictionary is selected."));
    return;
  }

  PromptForDictionaryName(dic_info.item->Text(),
                          tr("New name of the dictionary"), DICT_RENAME);
}

void DictionaryTool::_RenameDictionary(const char* name) {
  if (name == NULL || strlen(name) <= 0) {
    return;
  }

  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    return;
  }

  if (!session_->mutable_storage()->RenameDictionary(dic_info.id, name)) {
    LOG(ERROR) << "Failed to rename the dictionary.";
    ReportError();
    return;
  }

  dic_info.item->setText(name);

  UpdateUIStatus();

  fDicList->repaint();
}

void DictionaryTool::ImportAndCreateDictionary() {
  const int max_size = static_cast<int>(
      mozc::UserDictionaryStorage::max_dictionary_size());
  if (dic_list_->count() >= max_size) {
    QMessageBox::critical(
        this, window_title_,
        QString(tr("You can't have more than %1 dictionaries.")).arg(max_size).getp());
    return;
  }

  if (import_dialog_ == NULL) {
    import_dialog_ = new ImportDialog(
                        this, new BMessage(IMPORT_AS), ImportDialog::CREATE);
    import_dialog_->Show();
  }
}

void DictionaryTool::ImportAndAppendDictionary() {
  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    LOG(WARNING) << "No dictionary to import is selected";
    QMessageBox::information(this, window_title_,
                             tr("No dictionary is selected."));
    return;
  }

  if (dic_content_->rowCount() >= max_entry_size_) {
    QMessageBox::critical(this, window_title_,
                          QString(tr("You can't have more than %1 "
                             "words in one dictionary.")).arg(max_entry_size_).getp());
    return;
  }

  if (import_dialog_ == NULL) {
    import_dialog_ = new ImportDialog(
                        this, new BMessage(IMPORT_TO), ImportDialog::APPEND);
    import_dialog_->Show();
  }
}

void DictionaryTool::ReportImportError(UserDictionaryImporter::ErrorType error,
                                       const string &dic_name,
                                       int added_entries_size) {
  switch (error) {
    case UserDictionaryImporter::IMPORT_NO_ERROR:
      QMessageBox::information(
          this, window_title_,
          QString(tr("%1 entries are imported to %2."))
          .arg(added_entries_size).arg(dic_name.c_str()).getp());
      break;
    case UserDictionaryImporter::IMPORT_NOT_SUPPORTED:
      QMessageBox::information(
          this, window_title_,
          tr("You have imported a file in an invalid or "
             "unsupported file format.\n\n"
             "Please check the file format. "
             "ATOK11 or older format is not supported by "
             "Mozc."));
      break;
    case UserDictionaryImporter::IMPORT_TOO_MANY_WORDS:
      QMessageBox::information(
          this, window_title_,
          QString(tr("%1 doesn't have enough space to import all words in "
             "the file. First %2 entries "
             "are imported.")).arg(dic_name.c_str()).arg(added_entries_size).getp());
      break;
    case UserDictionaryImporter::IMPORT_INVALID_ENTRIES:
      QMessageBox::information(
          this, window_title_,
          QString(tr("%1 entries are imported to %2.\n\nSome imported "
             "words were not recognized by %3. "
             "Please check the original import file.")).
          arg(added_entries_size).arg(dic_name.c_str()).arg(window_title_).getp());
      break;
    case UserDictionaryImporter::IMPORT_FATAL:
      QMessageBox::critical(this, window_title_,
                            tr("Failed to open user dictionary"));
      break;
    default:
      break;
  }
}

void DictionaryTool::ImportHelper(
    uint64 dic_id,
    const string &dic_name,
    const string &file_name,
    UserDictionaryImporter::IMEType ime_type,
    UserDictionaryImporter::EncodingType encoding_type) {
  if (!IsReadableToImport(file_name)) {
    LOG(ERROR) << "File is not readable to import.";
    QMessageBox::critical(this, window_title_,
                          QString(tr("Can't open %1.")).arg(file_name.c_str()).getp());
    return;
  }

  if (dic_id == 0 &&
      !session_->mutable_storage()->CreateDictionary(dic_name, &dic_id)) {
    LOG(ERROR) << "Failed to create the dictionary.";
    ReportError();
    return;
  }

  UserDictionary *dic = session_->mutable_storage()->GetUserDictionary(dic_id);

  if (dic == NULL) {
    LOG(ERROR) << "Cannot find dictionary id: " << dic_id;
    ReportError();
    return;
  }

  if (dic->name() != dic_name) {
    LOG(ERROR) << "Dictionary name is inconsistent";
    ReportError();
    return;
  }

  // Everything looks Okey so far. Now starting import operation.
  SyncToStorage();

  // Open dictionary
  std::unique_ptr<UserDictionaryImporter::TextLineIteratorInterface> iter(
      CreateTextLineIterator(encoding_type, file_name, this));
  if (iter.get() == NULL) {
    LOG(ERROR) << "CreateTextLineIterator returns NULL";
    return;
  }

  const int old_size = dic->entries_size();
  const UserDictionaryImporter::ErrorType error =
      UserDictionaryImporter::ImportFromTextLineIterator(ime_type,
                                                         iter.get(),
                                                         dic);

  const int added_entries_size = dic->entries_size() - old_size;

  // Update window state.
  InitDictionaryList();

  for (int row = 0; row < fDicList->CountItems(); ++row) {
    DicItem* item = dynamic_cast<DicItem*>(fDicList->ItemAt(row));
    if (item) {
      if (dic_id == item->Id()) {
        fDicList->Select(row);
        break;
      }
    }
  }
  dic_list_->repaint();

  UpdateUIStatus();

  ReportImportError(error, dic_name, added_entries_size);
}

// void DictionaryTool::ImportFromDefaultIME()

void DictionaryTool::ExportDictionary() {
  DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item == NULL) {
    LOG(WARNING) << "No dictionary to export is selected";
    QMessageBox::information(this, window_title_,
                             tr("No dictionary is selected."));
    return;
  }

  mpExportFilePanel->Show();
}

void DictionaryTool::_ExportDictionary(const char* file_name, uint64 id) {
  SyncToStorage();

  if (!session_->mutable_storage()->ExportDictionary(id, file_name)) {
    LOG(ERROR) << "Failed to export the dictionary.";
    ReportError();
    return;
  }

  QMessageBox::information(this, window_title_,
                           tr("Dictionary export finished."));
}

void DictionaryTool::AddWord() {
  const int row = dic_content_->rowCount();
  if (row >= max_entry_size_) {
    QMessageBox::information(
        this, window_title_,
        QString(tr("You can't have more than %1 words in one dictionary.")).arg(
            max_entry_size_).getp());
    return;
  }

  fWordLV->AddRow(new DictContentRow());

  if (row + 1 >= max_entry_size_) {
    new_word_button_->setEnabled(false);
  }

  const int32 index = fWordLV->CountRows() - 1;
  fWordLV->ShowItem(index);
  fWordLV->EditItem(index, 0);

  UpdateUIStatus();
}

void DictionaryTool::GetSortedSelectedRows(vector<int> *rows) const {
  DCHECK(rows);
  rows->clear();

  std::vector<int32> items = fWordLV->SelectedIndex();
  if (items.empty()) {
    return;
  }
  rows->reserve(items.size());
  for (int i = 0; i < items.size(); ++i) {
    rows->push_back(items[i]);
  }

  sort(rows->begin(), rows->end(), greater<int>());
  vector<int>::const_iterator end = unique(rows->begin(), rows->end());

  rows->resize(end - rows->begin());
}

DicItem* DictionaryTool::GetFirstSelectedDictionary() const {
  int32 index = fDicList->CurrentSelection(0);
  DicItem* item = dynamic_cast<DicItem*>(fDicList->ItemAt(index));
  if (item == NULL) {
    LOG(WARNING) << "No current dictionary.";
    return NULL;
  }
  return item;
}

void DictionaryTool::DeleteWord() {
  vector<int> rows;
  GetSortedSelectedRows(&rows);
  if (rows.size() == 0) {
    return;
  }

  QString message;
  if (rows.size() == 1) {
    message = tr("Do you want to delete this word?");
  } else {
    message = tr("Do you want to delete the selected words?");
  }
  if (QMessageBox::question(
          this, window_title_, message.getp(),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
      != QMessageBox::Yes) {
    return;
  }

  fWordLV->LockUpdate();
  {
    ProgressDialogWrapper progress(
            tr("Deleting the selected words..."),
            this,
            rows.size());

    for (size_t i = 0; i < rows.size(); ++i) {
      fWordLV->RemoveRow(rows[i]);
      ++progress;
    }
  }
  fWordLV->UnlockUpdate();
  dic_content_->setEnabled(true);

  if (dic_content_->rowCount() < max_entry_size_) {
    new_word_button_->setEnabled(true);
  }

  UpdateUIStatus();

  modified_ = true;
}

void DictionaryTool::EditPOS(int32 pos) {
  std::vector<DictContentRow*> items = fWordLV->SelectedItems();
  if (items.empty()) {
    return;
  }

  for (int i = 0; i < items.size(); ++i) {
    DictContentRow* row = items[i];
    if (row) {
      row->SetCategory(pos);
    }
  }

  dic_content_->setEnabled(true);

  modified_ = true;
}

void DictionaryTool::MoveTo(uint64 id) {
  UserDictionary *target_dict = NULL;
  {
    const DicItem* selected_dict = GetFirstSelectedDictionary();
    if (selected_dict == NULL) {
      return;
    }

    DicItem* target_dict_item = NULL;
    for (int i = 0; i < fDicList->CountItems(); ++i) {
        DicItem* item = dynamic_cast<DicItem*>(fDicList->ItemAt(i));
        if (item) {
            if (item->Id() == id) {
                target_dict_item = item;
                break;
            }
        }
    }

    DCHECK(target_dict_item);
    if (target_dict_item == selected_dict) {
      LOG(WARNING) << "Target dictionary is the current dictionary.";
      return;
    }

    target_dict = session_->mutable_storage()->GetUserDictionary(
        target_dict_item->Id());
  }

  vector<int> rows;
  GetSortedSelectedRows(&rows);
  if (rows.size() == 0) {
    return;
  }

  const size_t target_max_entry_size =
      mozc::UserDictionaryStorage::max_entry_size();

  if (target_dict->entries_size() + rows.size() > target_max_entry_size) {
    QMessageBox::critical(this, window_title_,
                          QString(tr("Cannot move all the selected items.\n"
                             "The target dictionary can have maximum "
                             "%1 entries.")).arg(target_max_entry_size).getp());
    return;
  }

  {
    const int progress_max = rows.size() * 2;
    ProgressDialogWrapper progress(
            tr("Moving the selected words..."),
            this,
            progress_max);

    if (target_dict) {
      for (size_t i = 0; i < rows.size(); ++i) {
        DictContentRow* row = fWordLV->ItemAt(i);
        if (row) {
          UserDictionary::Entry *entry = target_dict->add_entries();
          entry->set_key(row->Reading());
          entry->set_value(row->Word());
          entry->set_pos(static_cast<mozc::user_dictionary::UserDictionary_PosType>(row->Category()));
          entry->set_comment(row->Comment());
          UserDictionaryUtil::SanitizeEntry(entry);
          ++progress;
        }
      }
    }
    for (size_t i = 0; i < rows.size(); ++i) {
      fWordLV->RemoveRow(rows[i]);
      ++progress;
    }
  }
  dic_content_->setEnabled(true);

  UpdateUIStatus();

  modified_ = true;
}

void DictionaryTool::EditComment() {
  std::vector<int32> items = fWordLV->SelectedIndex();
  if (items.empty()) {
    return;
  }

  InputDialog* dialog = new InputDialog(this, new BMessage(WORD_COMMENT_EDIT),
                    window_title_, tr("New comment"), "");
  dialog->Show();
}

void DictionaryTool::_EditComment(const char* comment) {
  std::vector<DictContentRow*> items = fWordLV->SelectedItems();
  if (items.empty()) {
    return;
  }

  for (int i = 0; i < items.size(); ++i) {
    DictContentRow* row = items[i];
    if (row) {
      row->SetComment(comment);
    }
  }

  dic_content_->setEnabled(true);
  modified_ = true;
}

// void DictionaryTool::OnItemChanged(QTableWidgetItem *item)

// void DictionaryTool::OnContextMenuRequestedForContent(const QPoint &pos)
// void DictionaryTool::OnContextMenuRequestedForList(const QPoint &pos)

DictionaryTool::DictionaryInfo DictionaryTool::current_dictionary() const {
  DictionaryInfo retval = { -1, 0, NULL };

  DicItem* selected_dict = GetFirstSelectedDictionary();
  if (selected_dict == NULL) {
    return retval;
  }

  retval.row = fDicList->IndexOf(selected_dict);
  retval.id = selected_dict->Id();
  retval.item = selected_dict;
  return retval;
}

void DictionaryTool::SyncToStorage() {
  if (current_dic_id_ == 0 || !modified_) {
    return;
  }

  UserDictionary *dic =
      session_->mutable_storage()->GetUserDictionary(current_dic_id_);

  if (dic == NULL) {
    LOG(ERROR) << "No save dictionary: " << current_dic_id_;
    return;
  }

  dic->clear_entries();

  for (int i = 0; i < dic_content_->rowCount(); ++i) {
    DictContentRow* row = fWordLV->ItemAt(i);
    if (row) {
      UserDictionary::Entry *entry = dic->add_entries();
      entry->set_key(row->Reading());
      entry->set_value(row->Word());
      entry->set_pos(
          static_cast<mozc::user_dictionary::UserDictionary_PosType>(row->Category()));
      entry->set_comment(row->Comment());
      UserDictionaryUtil::SanitizeEntry(entry);
    }
  }

  modified_ = false;
}

void DictionaryTool::CreateDictionaryHelper(const QString &dic_name) {
  uint64 new_dic_id = 0;
  if (!session_->mutable_storage()->CreateDictionary(
          dic_name.toStdString(),
          &new_dic_id)) {
    LOG(ERROR) << "Failed to create a new dictionary.";
    ReportError();
    return;
  }

  DicItem* item = new DicItem(dic_name.getp(), new_dic_id);
  DCHECK(item);
  fDicList->AddItem(item);
  const int32 index = fDicList->IndexOf(item);
  if (0 <= index) {
    fDicList->Select(index, false);
  }

  //AddWord();
}

bool DictionaryTool::InitDictionaryList() {
  dic_list_->clear();
  const UserDictionaryStorage &storage = session_->storage();

  for (size_t i = 0; i < storage.dictionaries_size(); ++i) {
    DicItem* item = new DicItem(storage.dictionaries(i).name().c_str(),
                                storage.dictionaries(i).id());
    fDicList->AddItem(item);
  }

  UpdateUIStatus();

  return true;
}

void DictionaryTool::PromptForDictionaryName(const QString &text,
                                                const QString &label,
                                                int32 what) {
  InputDialog* dialog = new InputDialog(this, new BMessage(what),
                    window_title_, label.getp(), text.getp());
  dialog->Show();
}

void DictionaryTool::ReportError() {
  switch (session_->mutable_storage()->GetLastError()) {
    case mozc::UserDictionaryStorage::INVALID_CHARACTERS_IN_DICTIONARY_NAME:
      LOG(ERROR) << "Dictionary name contains an invalid character.";
      QMessageBox::critical(
          this, window_title_,
          tr("An invalid character is included in the dictionary name."));
      break;
    case mozc::UserDictionaryStorage::EMPTY_DICTIONARY_NAME:
      LOG(ERROR) << "Dictionary name is empty.";
      QMessageBox::critical(this, window_title_,
                            tr("Dictionary name is empty."));
      break;
    case mozc::UserDictionaryStorage::TOO_LONG_DICTIONARY_NAME:
      LOG(ERROR) << "Dictionary name is too long.";
      QMessageBox::critical(this, window_title_,
                            tr("Dictionary name is too long."));
      break;
    case mozc::UserDictionaryStorage::DUPLICATED_DICTIONARY_NAME:
      LOG(ERROR) << "duplicated dictionary name";
      QMessageBox::critical(this, window_title_,
                            tr("Dictionary already exists."));
      break;
    default:
      LOG(ERROR) << "A fatal error occurred";
      QMessageBox::critical(this, window_title_,
                            tr("A fatal error occurred."));
      break;
  }
}

//void DictionaryTool::StartMonitoringUserEdit()
//void DictionaryTool::StopMonitoringUserEdit()

void DictionaryTool::SaveAndReloadServer() {
  if (!session_->mutable_storage()->Save() &&
      session_->mutable_storage()->GetLastError() ==
      mozc::UserDictionaryStorage::SYNC_FAILURE) {
    LOG(ERROR) << "Cannot save dictionary";
    return;
  }

  // If server is not running, we don't need to
  // execute Reload command.
  if (!client_->PingServer()) {
    LOG(WARNING) << "Server is not running. Do nothing";
    return;
  }

  // Update server version if need be.
  if (!client_->CheckVersionOrRestartServer()) {
    LOG(ERROR) << "CheckVersionOrRestartServer failed";
    return;
  }

  // We don't show any dialog even when an error happens, since
  // dictionary serialization is finished correctly.
  if (!client_->Reload()) {
    LOG(ERROR) << "Reload command failed";
  }
}

bool DictionaryTool::IsReadableToImport(const string &file_name) {
  BEntry entry(file_name.c_str());
  if (entry.InitCheck() == B_OK && entry.Exists()) {
    mode_t permission;
    if (entry.GetPermissions(&permission) == B_OK) {
      return (permission & S_IRUSR) > 0;
    }
  }
  return false;
}

bool DictionaryTool::IsWritableToExport(const string &file_name) {
  BEntry entry(file_name.c_str());
  if (entry.InitCheck() == B_OK) {
    if (entry.Exists()) {
      if (entry.IsFile()) {
        mode_t permission;
        if (entry.GetPermissions(&permission) == B_OK) {
          return (permission & S_IWUSR) > 0;
        }
      }
    } else {
      BEntry dir(entry);
      do {
        dir.GetParent(&dir);
      } while (!dir.Exists());
      mode_t permission;
      if (dir.GetPermissions(&permission) == B_OK) {
        return (permission & S_IWUSR) > 0;
      }
    }
  }
  return false;
}

void DictionaryTool::UpdateUIStatus() {
  const bool is_enable_new_dic =
      dic_list_->count() < session_->mutable_storage()->max_dictionary_size();
  new_action_->setEnabled(is_enable_new_dic);
  import_create_action_->setEnabled(is_enable_new_dic);

  delete_action_->setEnabled(dic_list_->count() > 0);
  import_append_action_->setEnabled(dic_list_->count() > 0);
#ifdef OS_WIN
  import_default_ime_action_->setEnabled(dic_list_->count() > 0);
#endif

  const bool is_enable_new_word =
      dic_list_->count() > 0 &&
      dic_content_->rowCount() < max_entry_size_;

  new_word_button_->setEnabled(is_enable_new_word);
  delete_word_button_->setEnabled(dic_content_->rowCount() > 0);

  const DictionaryInfo dic_info = current_dictionary();
  if (dic_info.item != NULL) {
    statusbar_message_ =  QString(tr("%1: %2 entries")).arg(
        dic_info.item->Text()).arg(
            dic_content_->rowCount());
  } else {
    statusbar_message_.clear();
  }

  statusbar_->showMessage(statusbar_message_);
}

void DictionaryTool::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case DICT_NEW:
        {
            const char* name = NULL;
            if (msg->FindString("text", &name) == B_OK) {
                if (strlen(name) > 0) {
                    _CreateDictionary(name);
                }
            } else {
                CreateDictionary();
            }
            break;
        }
        case DICT_RENAME:
        {
            const char* name = NULL;
            if (msg->FindString("text", &name) == B_OK) {
                _RenameDictionary(name);
            } else {
                RenameDictionary();
            }
            break;
        }
        case DICT_DELETE:
        {
            DeleteDictionary();
            break;
        }
        case FIND:
        {
            if (find_dialog_ == NULL) {
                find_dialog_ = new FindDialog(this, fWordLV);
                find_dialog_->Show();
            }
            break;
        }
        case FindDialog::FIND_CLOSE:
        {
            find_dialog_ = NULL;
            break;
        }
        case IMPORT_AS:
        {
            const char* path = NULL;
            const char* name = NULL;
            int32 format = 0;
            int32 encoding = 0;
            if (msg->FindString("path", &path) == B_OK &&
                msg->FindString("name", &name) == B_OK &&
                msg->FindInt32("format", &format) == B_OK &&
                msg->FindInt32("encoding", &encoding) == B_OK) {
                if (mozc::UserDictionaryImporter::IME_AUTO_DETECT <= format &&
                    format <  mozc::UserDictionaryImporter::NUM_IMES &&
                    mozc::UserDictionaryImporter::ENCODING_AUTO_DETECT <= encoding &&
                    encoding < mozc::UserDictionaryImporter::NUM_ENCODINGS) {
                    ImportHelper(0,   // dic_id == 0 means that "CreateNewDictonary" mode
                        name, path,
                        static_cast<mozc::UserDictionaryImporter::IMEType>(format),
                        static_cast<mozc::UserDictionaryImporter::EncodingType>(encoding));
                }
            } else {
                ImportAndCreateDictionary();
            }
            break;
        }
        case IMPORT_TO:
        {
            const char* path = NULL;
            int32 format = 0;
            int32 encoding = 0;
            if (msg->FindString("path", &path) == B_OK &&
                msg->FindInt32("format", &format) == B_OK &&
                msg->FindInt32("encoding", &encoding) == B_OK) {
                if (mozc::UserDictionaryImporter::IME_AUTO_DETECT <= format &&
                    format <  mozc::UserDictionaryImporter::NUM_IMES &&
                    mozc::UserDictionaryImporter::ENCODING_AUTO_DETECT <= encoding &&
                    encoding < mozc::UserDictionaryImporter::NUM_ENCODINGS) {
                    DictionaryInfo dic_info = current_dictionary();
                    if (dic_info.item == NULL) {
                        return;
                    }
                    ImportHelper(dic_info.id, dic_info.item->Text(), path,
                        static_cast<mozc::UserDictionaryImporter::IMEType>(format),
                        static_cast<mozc::UserDictionaryImporter::EncodingType>(encoding));
                }
            } else {
                ImportAndAppendDictionary();
            }
            break;
        }
        case ImportDialog::IMPORT_CLOSE:
        {
            import_dialog_ = NULL;
            break;
        }
        case EXPORT_CURRENT:
        {
            const char* name = NULL;
            int64 tmp_id = 0;
            if (msg->FindString("path", &name) == B_OK &&
                msg->FindInt64("id", &tmp_id) == B_OK) {
                const uint64 id = static_cast<uint64>(tmp_id);
                _ExportDictionary(name, id);
            } else {
                ExportDictionary();
            }
            break;
        }
        case WORD_ADD:
        {
            AddWord();
            break;
        }
        case WORD_MOVE:
        {
            int64 tmp_id = 0;
            if (msg->FindInt64("id", &tmp_id) == B_OK) {
                const uint64 id = static_cast<uint64>(tmp_id);
                MoveTo(id);
            }
            break;
        }
        case WORD_DELETE:
        {
            DeleteWord();
            break;
        }
        case WORD_CATEGORY_EDIT:
        {
            int32 pos = 0;
            if (msg->FindInt32("pos", &pos) == B_OK) {
                if (mozc::user_dictionary::UserDictionary_PosType_IsValid(pos)) {
                    EditPOS(pos);
                }
            }
            break;
        }
        case WORD_COMMENT_EDIT:
        {
            const char* comment = NULL;
            if (msg->FindString("text", &comment) == B_OK) {
                _EditComment(comment);
            } else {
                EditComment();
            }
            break;
        }
        case DICT_SELECTION_CHANGED:
        {
            const int32 index = fDicList->CurrentSelection();
            if (index < 0 && mnLastDicIndex < fDicList->CountItems()) {
                // not selected, do not allow to non selected state
                fDicList->Select(mnLastDicIndex);
            } else {
                if (mnLastDicIndex != index) {
                    fWordLV->StopEditing();
                    mnLastDicIndex = index;
                    OnDictionarySelectionChanged();
                }
            }
            break;
        }
        case DICT_LIST_MENU:
        {
            BPoint pos;
            if (msg->FindPoint("pos", &pos) == B_OK) {
                pos = _MapToScreen(fDicList, pos);
                BMenuItem* item = fDicListMenu->Go(pos, true);
                if (item) {
                    BMessage* message = item->Message();
                    if (message != NULL) {
                        MessageReceived(message);
                    }
                }
            }
            break;
        }
        case ENTRIES_MENU:
        {
            BPoint pos;
            if (msg->FindPoint("pos", &pos) == B_OK) {
                pos = _MapToScreen(fDicList, pos);
                std::vector<int32> items = fWordLV->SelectedIndex();
                const char* labelDelete = tr("Delete this word");
                const char* labelMove = tr("Move this word to");
                if (items.size() > 0) {
                    labelDelete = tr("Delete the selected words");
                    labelMove = tr("Move the selected words to");
                }
                fDicMenu->ItemAt(2)->SetLabel(labelDelete); // delete
                fMoveMenu->SetName(labelMove);
                if (dic_list_->count() > 1) {
                    if (fMoveMenu->CountItems() > 0) {
                        for (int32 i = 0; i < fMoveMenu->CountItems(); ++i) {
                            BMenuItem* item = fMoveMenu->RemoveItem(i);
                            delete item;
                        }
                    }
                    const int32 selected = fDicList->CurrentSelection(0);
                    if (selected >= 0) {
                        const DicItem* selected_dict = dynamic_cast<DicItem*>(fDicList->ItemAt(selected));
                        for (int32 i = 0; i < fDicList->CountItems(); ++i) {
                            const DicItem* item = dynamic_cast<DicItem*>(fDicList->ItemAt(i));
                            if (item == selected_dict) {
                                continue;
                            }
                            BMessage* msg = new BMessage(WORD_MOVE);
                            //msg->AddString("name", item->Text());
                            msg->AddInt64("id", item->Id());
                            fMoveMenu->AddItem(new BMenuItem(item->Text(), msg));
                        }
                    }
                    fMoveMenu->SetEnabled(!items.empty());
                } else {
                    fMoveMenu->SetEnabled(false);
                }
                const bool selected = !items.empty();
                fPOSMenu->SetEnabled(selected);
                fDicMenu->ItemAt(2)->SetEnabled(selected);
                fDicMenu->ItemAt(5)->SetEnabled(selected);

                BMenuItem* item = fDicMenu->Go(pos, true);
                if (item) {
                    BMessage* message = item->Message();
                    if (message != NULL) {
                        MessageReceived(message);
                    }
                }
            }
            break;
        }
        case TOOLS:
        {
            BPoint where = fToolButton->Frame().LeftBottom();
            ConvertToScreen(&where);
            BMenuItem *item = fToolsMenu->Go(where, true);
            if (item != NULL) {
                BMessage *message = item->Message();
                if (message != NULL) {
                    MessageReceived(message);
                }
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
                const DicItem* selected_dict = GetFirstSelectedDictionary();
                if (selected_dict) {
                    BMessage message(EXPORT_CURRENT);
                    message.AddString("path", path.Path());
                    message.AddInt64("id", static_cast<int64>(selected_dict->Id()));
                    MessageReceived(&message);
                }
            }
            break;
        }
        case B_DELETE:
        {
            DeleteWord();
            break;
        }
        case GridView<DictContentRow>::MODIFIED:
        {
            modified_ = true;
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

BPoint DictionaryTool::_MapToScreen(BView* view, BPoint pos)
{
    BPoint p(pos);
    BView* parent = view->Parent();
    while (parent) {
        p += parent->Frame().LeftTop();
        parent = parent->Parent();
    }
    p = ConvertToScreen(p);
    return p;
}


class DictionaryToolApp : public ToolApp
{
public:
    DictionaryToolApp();
    virtual ~DictionaryToolApp() {}
};

DictionaryToolApp::DictionaryToolApp()
    : ToolApp(DICTIONARY_TOOL)
{
    mpWindow = new DictionaryTool();
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}


} // namespace haiku_gui
} // namespace mozc


int HaikuRunDictionaryTool(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                        mozc::haiku_gui::DICTIONARY_TOOL)) {
        return -1;
    }

    mozc::haiku_gui::DictionaryToolApp *app = new mozc::haiku_gui::DictionaryToolApp();
    app->Run();
    delete app;

    return 0;
}

