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

#include "haiku/haiku_gui/config_dialog/config_dialog.h"
#include "haiku/haiku_gui/base/compatible.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/base/grid_view.h"
#include "haiku/haiku_gui/config_dialog/roman_table_editor.h"
#include "haiku/haiku_gui/config_dialog/keymap_editor.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include "base/const.h"
#include "base/logging.h"
#include "base/file_util.h"
#include "base/run_level.h"
#include "base/util.h"
#include "client/client.h"
#include "config/config_handler.h"
#include "config/stats_config_util.h"
#include "ipc/ipc.h"
#include "protocol/commands.pb.h"
#include "protocol/config.pb.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Spinner.h>
#include <StringItem.h>
#include <TabView.h>
#include <TextControl.h>
#include <Window.h>


#define tr(s) B_TRANSLATE(s)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "config_dialog"

namespace mozc {
namespace haiku_gui {

class GeneralView : public BView
{
public:
    GeneralView(const char* name);
    virtual ~GeneralView() {}

    enum {
        INPUT_METHOD_ROMAJI = 'inro',
        INPUT_METHOD_KANA = 'inka',
        PUNCTUATION_1 = 'pu01',
        PUNCTUATION_2 = 'pu02',
        PUNCTUATION_3 = 'pu03',
        PUNCTUATION_4 = 'pu04',
        SYMBOL_1 = 'sy01',
        SYMBOL_2 = 'sy02',
        SYMBOL_3 = 'sy03',
        SYMBOL_4 = 'sy04',
        SPACE_INPUT_1 = 'sp01',
        SPACE_INPUT_2 = 'sp02',
        SPACE_INPUT_3 = 'sp03',
        CANDIDATE_0 = 'cd00',
        CANDIDATE_1 = 'cd01',
        CANDIDATE_2 = 'cd02',
        NUMPAD_1 = 'nm01',
        NUMPAD_2 = 'nm02',
        NUMPAD_3 = 'nm03',
        NUMPAD_4 = 'nm04',
        KEYMAP_1 = 'km01',
        KEYMAP_2 = 'km02',
        KEYMAP_3 = 'km03',
        KEYMAP_4 = 'km04',
        KEYMAP_EDIT = 'kmed',
        ROMAJI_TABLE_EDIT = 'rmed',
    };

    BPopUpMenu* inputMethodPM;
    BPopUpMenu* punctuationPM;
    BPopUpMenu* symbolPM;
    BPopUpMenu* spaceInputPM;
    BPopUpMenu* candidatePM;
    BPopUpMenu* numpadPM;
    BPopUpMenu* keymapPM;
    BButton* mpEditRomanTableButton;
};


GeneralView::GeneralView(const char* name)
    : BView(name, B_WILL_DRAW)
{
    inputMethodPM = new BPopUpMenu("inputMethodPM");
    BLayoutBuilder::Menu<>(inputMethodPM)
        .AddItem(B_TRANSLATE("Romaji"), new BMessage(INPUT_METHOD_ROMAJI))
        .AddItem(B_TRANSLATE("Kana"), new BMessage(INPUT_METHOD_KANA));
    // select first item
    BMenuField *inputMethodMF = new BMenuField("inputMethodMF", "", inputMethodPM);

    punctuationPM = new BPopUpMenu("punctuationPM");
    BLayoutBuilder::Menu<>(punctuationPM)
        .AddItem("、。", new BMessage(PUNCTUATION_1))
        .AddItem("，．", new BMessage(PUNCTUATION_2))
        .AddItem("、．", new BMessage(PUNCTUATION_3))
        .AddItem("，。", new BMessage(PUNCTUATION_4));
    BMenuField *punctuationMF = new BMenuField("punctuationMF", "", punctuationPM);

    symbolPM = new BPopUpMenu("symbolPM");
    BLayoutBuilder::Menu<>(symbolPM)
        .AddItem("「」・", new BMessage(SYMBOL_1))
        .AddItem("[]／", new BMessage(SYMBOL_2))
        .AddItem("「」／", new BMessage(SYMBOL_3))
        .AddItem("[]・", new BMessage(SYMBOL_4));
    BMenuField *symbolMF = new BMenuField("symbolMF", "", symbolPM);

    spaceInputPM = new BPopUpMenu("spaceInputPM");
    BLayoutBuilder::Menu<>(spaceInputPM)
        .AddItem(B_TRANSLATE("Follow input mode"), new BMessage(SPACE_INPUT_1))
        .AddItem(B_TRANSLATE("Fullwidth"), new BMessage(SPACE_INPUT_2))
        .AddItem(B_TRANSLATE("Halfwidth"), new BMessage(SPACE_INPUT_3));
    BMenuField *spaceInputMF = new BMenuField("spaceInputMF", "", spaceInputPM);

    candidatePM = new BPopUpMenu("candidatePM");
    BLayoutBuilder::Menu<>(candidatePM)
        .AddItem(B_TRANSLATE("No shortcut"), new BMessage(CANDIDATE_0))
        .AddItem(B_TRANSLATE("1 -- 9"), new BMessage(CANDIDATE_1))
        .AddItem(B_TRANSLATE("A -- L"), new BMessage(CANDIDATE_2));
    BMenuField *candidateMF = new BMenuField("candidateMF", "", candidatePM);

    numpadPM = new BPopUpMenu("numpadPM");
    BLayoutBuilder::Menu<>(numpadPM)
        .AddItem(B_TRANSLATE("Follow input mode"), new BMessage(NUMPAD_1))
        .AddItem(B_TRANSLATE("Fullwidth"), new BMessage(NUMPAD_2))
        .AddItem(B_TRANSLATE("Halfwidth"), new BMessage(NUMPAD_3))
        .AddItem(B_TRANSLATE("Direct input"), new BMessage(NUMPAD_4));
    BMenuField *numpadMF = new BMenuField("numpadMF", "", numpadPM);

    keymapPM = new BPopUpMenu("keymapPM");
    BLayoutBuilder::Menu<>(keymapPM)
        .AddItem(B_TRANSLATE("Custom keymap"), new BMessage(KEYMAP_1))
        .AddItem(B_TRANSLATE("ATOK"), new BMessage(KEYMAP_2))
        .AddItem(B_TRANSLATE("MS-IME"), new BMessage(KEYMAP_3))
        .AddItem(B_TRANSLATE("Kotoeri"), new BMessage(KEYMAP_4));
    BMenuField *keymapMF = new BMenuField("keymapMF", "", keymapPM);

    mpEditRomanTableButton = new BButton(B_TRANSLATE("Edit"), new BMessage(ROMAJI_TABLE_EDIT));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("basicsL", B_TRANSLATE("Basics")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGrid()
            .SetInsets(5, 0, 5, 0)
            .Add(new CStringView("inputModeL", B_TRANSLATE("Input mode")), 0, 1)
            .Add(inputMethodMF, 1, 1)
            .Add(new CStringView("punctuationL", B_TRANSLATE("Punctuation style")), 0, 2)
            .Add(punctuationMF, 1, 2)
            .Add(new CStringView("symbolStyleL", B_TRANSLATE("Symbol style")), 0, 3)
            .Add(symbolMF, 1, 3)
            .Add(new CStringView("spaceInputStyleL", B_TRANSLATE("Space input style")), 0, 4)
            .Add(spaceInputMF, 1, 4)
            .Add(new CStringView("candidateL", B_TRANSLATE("Candidate selection shortcut")), 0, 5)
            .Add(candidateMF, 1, 5)
            .Add(new CStringView("inputFromNumpadL", B_TRANSLATE("Input from numpad keys")), 0, 6)
            .Add(numpadMF, 1, 6)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("keymapL", B_TRANSLATE("Keymap")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGrid()
            .SetInsets(5, 0, 5, 0)
            .Add(new CStringView("keymapStyleL", B_TRANSLATE("Keymap style")), 0, 1)
            .Add(keymapMF, 1, 1)
            .Add(new BButton(B_TRANSLATE("Edit"), new BMessage(KEYMAP_EDIT)), 2, 1)
            .Add(new CStringView("romajiTableL", B_TRANSLATE("Romaji table")), 0, 2)
            .Add(new CStringView("emptyL", ""), 1, 2)
            .Add(mpEditRomanTableButton, 2, 2)
        .End()
        .AddGlue();

    Layout(true);
}


class DictionaryView : public BView
{
public:
    DictionaryView(const char* name);
    virtual ~DictionaryView() {}

    enum {
        PERSONA_ENABLED = 'pren',
        PERSONA_ENABLED_NO_RECORD = 'prnr',
        PERSONA_DISABLED = 'prdi',
        PERSONA_DATA_CLEAR = 'btcp',
        USER_DICT_EDIT = 'bted',
        HOMONYM_DICT = 'chho',
        SC_SINGLE_KANJI = 'scsk',
        SC_SYMBOL = 'scsy',
        SC_EMOTICON = 'scem',
        SC_KATAKANA = 'scka',
        SC_POSTAL_CODE = 'scpo',
        SC_EMOJI = 'scej',
        SC_DATE = 'scdt',
        SC_SPECIAL_NUMBER = 'scsp',
        SC_CALCULATOR = 'sccl',
        SC_SPELLING_CORRECTION = 'scsl',
    };

    BPopUpMenu* personaPM;
    BCheckBox*  fHomonymDictCB;
    BCheckBox*  fSingleKanjiCB;
    BCheckBox*  fSymbolCB;
    BCheckBox*  fEmoticonCB;
    BCheckBox*  fKatakanaCB;
    BCheckBox*  fPostalCodeCB;
    BCheckBox*  fEmojiCB;
    BCheckBox*  fDateCB;
    BCheckBox*  fSpecialNumberCB;
    BCheckBox*  fCalculatorCB;
    BCheckBox*  fSpellingCorrectionCB;
};


DictionaryView::DictionaryView(const char* name)
    : BView(name, B_WILL_DRAW)
{
    personaPM = new BPopUpMenu("personaPM");
    BLayoutBuilder::Menu<>(personaPM)
        .AddItem(B_TRANSLATE("Yes"), new BMessage(PERSONA_ENABLED))
        .AddItem(B_TRANSLATE("Yes (don't record new data)"), new BMessage(PERSONA_ENABLED_NO_RECORD))
        .AddItem(B_TRANSLATE("No"), new BMessage(PERSONA_DISABLED));
    BMenuField *personalizationMF = new BMenuField("personalizationMF", "", personaPM);

    //fHomonymDictCB = new BCheckBox("homo", B_TRANSLATE("Homonym dictionary"),
    //        new BMessage(HOMONYM_DICT));

    fSingleKanjiCB = new BCheckBox("singleKanjiCB", B_TRANSLATE("Single kanji conversion"),
            new BMessage(SC_SINGLE_KANJI));
    fSymbolCB = new BCheckBox("symbolCB", B_TRANSLATE("Symbol conversion"),
            new BMessage(SC_SYMBOL));
    fEmoticonCB = new BCheckBox("emoticonCB", B_TRANSLATE("Emoticon conversion"),
            new BMessage(SC_EMOTICON));
    fKatakanaCB = new BCheckBox("katakanaCB", B_TRANSLATE("Katakana to English conversion"),
            new BMessage(SC_KATAKANA));
    fPostalCodeCB = new BCheckBox("postalCodeCB", B_TRANSLATE("Postal code conversion"),
            new BMessage(SC_POSTAL_CODE));
    fEmojiCB = new BCheckBox("emojiCB", B_TRANSLATE("Emoji conversion"),
            new BMessage(SC_EMOJI));
    fDateCB = new BCheckBox("dateCB", B_TRANSLATE("Date/time conversion"),
            new BMessage(SC_DATE));
    fSpecialNumberCB = new BCheckBox("specialNumberCB", B_TRANSLATE("Special number conversion"),
            new BMessage(SC_SPECIAL_NUMBER));
    fCalculatorCB = new BCheckBox("calculatorCB", B_TRANSLATE("Calculator"),
            new BMessage(SC_CALCULATOR));
    fSpellingCorrectionCB = new BCheckBox("spellingCorrectionCB", B_TRANSLATE("Spelling correction"),
            new BMessage(SC_SPELLING_CORRECTION));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("personalizationSV", B_TRANSLATE("Personalization")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .AddGroup(B_HORIZONTAL)
                .Add(new CStringView("AdjustSV", B_TRANSLATE("Adjust conversion based on previous input")))
                .AddGlue()
                .Add(personalizationMF)
            .End()
            .AddGroup(B_HORIZONTAL)
                .AddGlue()
                .Add(new BButton("clearPerDataB", B_TRANSLATE("Clear personalization data"),
                        new BMessage(PERSONA_DATA_CLEAR)))
            .End()
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("userDictionarySV", B_TRANSLATE("User dictionary")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_HORIZONTAL)
            .SetInsets(5, 0, 5, 0)
            .AddGlue()
            .Add(new BButton("editUserDictB", B_TRANSLATE("Edit user dictionary..."),
                    new BMessage(USER_DICT_EDIT)))
        .End()
        //.AddGroup(B_HORIZONTAL)
        //    .Add(new CStringView("usageDictionarySV", B_TRANSLATE("Usage dictionary")))
        //    .Add(new BSeparatorView(B_HORIZONTAL))
        //.End()
        //.AddGroup(B_VERTICAL)
        //    .SetInsets(5, 0, 5, 0)
        //    .Add(fHomonymDictCB)
        //.End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("spacialConversionsSV", B_TRANSLATE("Special conversions")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGrid(B_USE_DEFAULT_SPACING, B_USE_HALF_ITEM_SPACING)
            .SetInsets(5, 0, 5, 0)
            .Add(fSingleKanjiCB, 0, 0)
            .Add(fSymbolCB, 0, 1)
            .Add(fEmoticonCB, 0, 2)
            .Add(fKatakanaCB, 0, 3)
            .Add(fPostalCodeCB, 0, 4)
            .Add(fEmojiCB, 1, 0)
            .Add(fDateCB, 1, 1)
            .Add(fSpecialNumberCB, 1, 2)
            .Add(fCalculatorCB, 1, 3)
            .Add(fSpellingCorrectionCB, 1, 4)
        .End()
        .AddGlue();
}


class CharacterFormRow : public GridRow
{
public:
    CharacterFormRow(const char* group, int32 preedit, int32 conversion);
    virtual ~CharacterFormRow() {}

    const std::string& Group() const { return maGroup; }
    int32 Preedit() const { return mnPreedit; }
    int32 Conversion() const { return mnConversion; }
    void SetPreedit(int32 n) { mnPreedit = n; }
    void SetConversion(int32 n) { mnConversion = n; }
    bool IsPreeditEnabled() { return mbPreeditEnabled; }
    void SetPreeditEnabled(bool enabled) { mbPreeditEnabled = enabled; }

    typedef bool (*CharacterFormRowComp)(CharacterFormRow *r1, CharacterFormRow *r2);
    static CharacterFormRowComp GetComp(int32 index) { return NULL; }
private:
    bool mbPreeditEnabled;
    std::string maGroup;
    int32 mnPreedit;
    int32 mnConversion;
};

CharacterFormRow::CharacterFormRow(const char* group, int32 preedit, int32 conversion)
    : GridRow(),
      mbPreeditEnabled(true)
{
    maGroup = group;
    mnPreedit = preedit;
    mnConversion = conversion;
}


template<typename T>
class CharacterFormGroupColumn : public GridStringColumn<T>
{
public:
    CharacterFormGroupColumn(BView* view, int32 column);
    virtual ~CharacterFormGroupColumn() {}
protected:
    virtual const char* _GetLabel(T* pRow);
};

template<typename T>
CharacterFormGroupColumn<T>::CharacterFormGroupColumn(BView* view, int32 column)
    : GridStringColumn<T>(view, column)
{
}

template<typename T>
const char* CharacterFormGroupColumn<T>::_GetLabel(T* pRow)
{
    if (pRow && this->mnColumn == 0) {
        return pRow->Group().c_str();
    }
    return "";
}


template<typename T>
class CharacterFormColumn : public GridMenuFieldColumn<T>
{
public:
    CharacterFormColumn(BView* view, int32 column, BHandler* handler);
    virtual ~CharacterFormColumn() {}

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow);

    void AddForm(const char* label);
protected:
    bool mbWidthUpdated;
    std::vector<float> mnWidth;
    virtual void _UpdateRow(T* pRow, BMessage* msg);
    virtual int32 _LabelIndex(T* pRow);
    virtual int32 _CurrentIndex(T* pRow);
};

template<typename T>
CharacterFormColumn<T>::CharacterFormColumn(BView* view, int32 column, BHandler* handler)
    : GridMenuFieldColumn<T>(view, column, handler),
      mbWidthUpdated(false)
{
}

template<typename T>
void CharacterFormColumn<T>::AddForm(const char* label)
{
    this->maLabels.push_back(label);
    this->mpPopUpMenu->AddItem(
        new BMenuItem(label, new BMessage(this->CountLabels() - 1)));
}

template<typename T>
void CharacterFormColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    if (!mbWidthUpdated) {
        mnWidth.clear();
        for (int32 i = 0; i < this->CountLabels(); ++i) {
            mnWidth.push_back(view->StringWidth(this->maLabels[i].c_str()) / 2);
        }
        mbWidthUpdated = true;
    }
    const int32 index = this->_LabelIndex(pRow);
    if (0 <= index && index <= this->CountLabels()) {
        const char* label = this->maLabels[index].c_str();
        BPoint pos(rect.left + 5 + (rect.Width() - 10) / 2 - mnWidth[index],
                   rect.top + rect.Height() * 0.75);
        if (selected && !(this->mnColumn == 1 && !pRow->IsPreeditEnabled())) {
            this->_DrawMenuField(view, rect, pos, label);
        } else {
            view->DrawString(label, pos);
        }
    }
}

template<typename T>
void CharacterFormColumn<T>::Edit(BView* view, BRect rect, BPoint pos, T* pRow)
{
    if (pRow) {
        if (this->mnColumn == 1 && !pRow->IsPreeditEnabled()) {
            return;
        }
        GridMenuFieldColumn<T>::Edit(view, rect, pos, pRow);
    }
}

template<typename T>
void CharacterFormColumn<T>::_UpdateRow(T* pRow, BMessage* msg)
{
    if (pRow) {
        const int32 form = msg->what;
        if (0 <= form && form <= this->CountLabels()) {
            if (this->mnColumn == 1) {
                pRow->SetPreedit(form);
            } else {
                pRow->SetConversion(form);
            }
            this->_Modified();
        }
    }
}

template<typename T>
int32 CharacterFormColumn<T>::_LabelIndex(T* pRow)
{
    if (pRow) {
        const int32 form = (this->mnColumn == 1) ? pRow->Preedit() : pRow->Conversion();
        return (0 <= form && form < this->CountLabels()) ? form : -1;
    }
    return -1;
}

template<typename T>
int32 CharacterFormColumn<T>::_CurrentIndex(T* pRow)
{
    if (pRow) {
        const int32 form = (this->mnColumn == 1) ? pRow->Preedit() : pRow->Conversion();
        return (0 <= form && form < this->CountLabels()) ? form : -1;
    }
    return -1;
}


template<typename T>
class CharacterFormGridView : public GridView<T>
{
public:
    CharacterFormGridView(BLooper* looper);
    virtual ~CharacterFormGridView() {}

    virtual void Draw(BRect rect);

    void Load(const config::Config &config);
    void Save(config::Config *config);
};

template<typename T>
CharacterFormGridView<T>::CharacterFormGridView(BLooper* looper)
    : GridView<T>(SINGLE, looper, NULL)
{
    GridTitleView* pTitleView = new GridTitleView(false);
    pTitleView->AddColumn(B_TRANSLATE("Group"));
    pTitleView->AddColumn(B_TRANSLATE("Composition"));
    pTitleView->AddColumn(B_TRANSLATE("Conversion"));
    this->SetTitleView(pTitleView);

    this->SetExplicitMinSize(BSize(50, this->mnRowHeight * 9));
}

template<typename T>
void CharacterFormGridView<T>::Draw(BRect rect)
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
std::string GroupToString(const string &str) {
  if (str == "ア") {
    return tr("Katakana");
  } else if (str == "0") {
    return tr("Numbers");
  } else if (str == "A") {
    return tr("Alphabets");
  }
  return str;
}

const std::string StringToGroup(const char* str) {
  if (strcmp(str, tr("Katakana")) == 0) {
    return "ア";
  } else if (strcmp(str, tr("Numbers")) == 0) {
    return "0";
  } else if (strcmp(str, tr("Alphabets")) == 0) {
    return "A";
  }
  return std::string(str);
}
}

template<typename T>
void CharacterFormGridView<T>::Load(const config::Config &config)
{
  this->MakeEmpty();

  std::unique_ptr<config::Config> default_config;
  const config::Config *target_config = &config;

  // make sure that table isn't empty.
  if (config.character_form_rules_size() == 0) {
    default_config.reset(new config::Config);
    config::ConfigHandler::GetDefaultConfig(default_config.get());
    target_config = default_config.get();
  }

  const char* katakana = B_TRANSLATE("Katakana");

  for (size_t row = 0;
       row < target_config->character_form_rules_size(); ++row) {
    const config::Config::CharacterFormRule &rule
        = target_config->character_form_rules(row);
    const std::string group = GroupToString(rule.group());
    CharacterFormRow* pRow = new CharacterFormRow(
            GroupToString(group).c_str(),
            rule.preedit_character_form(),
            rule.conversion_character_form());
    this->AddRow(pRow);
    if (group == katakana) {
      pRow->SetPreeditEnabled(false);
    }
  }
}

template<typename T>
void CharacterFormGridView<T>::Save(config::Config *config)
{
  if (this->CountRows() == 0) {
    return;
  }

  config->clear_character_form_rules();
  for (int row = 0; row < this->CountRows(); ++row) {
    CharacterFormRow* pRow = this->ItemAt(row);
    if (pRow) {
      const string group = StringToGroup(pRow->Group().c_str());
      config::Config::CharacterForm preedit_form =
          static_cast<mozc::config::Config::CharacterForm>(pRow->Preedit());
      config::Config::CharacterForm conversion_form =
          static_cast<mozc::config::Config::CharacterForm>(pRow->Conversion());
      config::Config::CharacterFormRule *rule =
          config->add_character_form_rules();
      rule->set_group(group);
      rule->set_preedit_character_form(preedit_form);
      rule->set_conversion_character_form(conversion_form);
    }
  }
}

class AdvanceView : public BView
{
public:
    AdvanceView(const char* name);
    virtual ~AdvanceView() {}

    enum {
        AUTO_HALFWIDTH = 'chah',
        CONVERT_PUNCTUATIONS = 'chpn',
        CONVERT_PERIOD = 'cvpr',
        CONVERT_COMMA = 'cvcm',
        CONVERT_QUESTION = 'cvqu',
        CONVERT_EXCLAMATION = 'cvex',
        SHIFT_SWITCH_1 = 'shs1',
        SHIFT_SWITCH_2 = 'shs2',
        SHIFT_SWITCH_3 = 'shs3',
    };

    BPopUpMenu* shiftSwitchPM;
    BCheckBox*  fAutoHalfwidthCB;
    BCheckBox*  fConvertPunctuationsCB;
    BCheckBox*  mpKutenCB;
    BCheckBox*  mpToutenCB;
    BCheckBox*  mpQuestionMarkCB;
    BCheckBox*  mpExclamationMarkCB;

    CharacterFormGridView<CharacterFormRow>*    mpCharacterFormGridView;
};


AdvanceView::AdvanceView(const char* name)
    : BView(name, B_WILL_DRAW)
{
    fAutoHalfwidthCB = new BCheckBox("autoHalfwidthCB",
        B_TRANSLATE("Automatically switch to halfwidth"), new BMessage(AUTO_HALFWIDTH));
    fConvertPunctuationsCB = new BCheckBox("convertPunctuationsCB",
        B_TRANSLATE("Convert at punctuations"), new BMessage(CONVERT_PUNCTUATIONS));

    mpKutenCB = new BCheckBox("periodCB", "。", new BMessage(CONVERT_PERIOD));
    mpToutenCB = new BCheckBox("commaCB", "、", new BMessage(CONVERT_COMMA));
    mpQuestionMarkCB = new BCheckBox("questionCB", "？", new BMessage(CONVERT_QUESTION));
    mpExclamationMarkCB = new BCheckBox("exclamationCB", "！", new BMessage(CONVERT_EXCLAMATION));

    shiftSwitchPM = new BPopUpMenu("shiftSwitchPM");
    BLayoutBuilder::Menu<>(shiftSwitchPM)
        .AddItem(B_TRANSLATE("Off"), new BMessage(SHIFT_SWITCH_1))
        .AddItem(B_TRANSLATE("Alphanumeric"), new BMessage(SHIFT_SWITCH_2))
        .AddItem(B_TRANSLATE("Katakana"), new BMessage(SHIFT_SWITCH_3));
    BMenuField *shiftSwitchMF = new BMenuField("shiftSwitchMF", "", shiftSwitchPM);

    mpCharacterFormGridView = new CharacterFormGridView<CharacterFormRow>(NULL);
    mpCharacterFormGridView->SetDrawingMode(B_OP_OVER);
    BScrollView* pCharacterFormList = new BScrollView("contentview", mpCharacterFormGridView,
                B_FOLLOW_LEFT_TOP, B_FRAME_EVENTS, false, true, B_FANCY_BORDER);

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("input", B_TRANSLATE("Input Assistance")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGrid()
            .SetInsets(5, 0, 5, 0)
            .Add(fAutoHalfwidthCB, 0, 0)
            .Add(fConvertPunctuationsCB, 0, 1)
            .AddGroup(B_HORIZONTAL, 1, 1, 1)
                .Add(mpKutenCB)
                .Add(mpToutenCB)
                .Add(mpQuestionMarkCB)
                .Add(mpExclamationMarkCB)
            .End()
            .Add(new CStringView("shiftSwitch", B_TRANSLATE("Shift key mode switch")), 0, 2)
            .Add(shiftSwitchMF, 1, 2)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("charForm", B_TRANSLATE("Fullwidth/Halfwidth")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .Add(pCharacterFormList)
        .End()
        .AddGlue();
}


class SuggestView : public BView
{
public:
    SuggestView(const char* name);
    virtual ~SuggestView() {}

    enum {
        USE_INPUT_HISTORY = 'chih',
        USE_SYSTEM_DICTIONARY = 'chsd',
        USE_REALTIME_CONVERSION = 'chrc',
        CLEAR_UNUSED_HISTORY = 'cluh',
        CLEAR_ALL_HISTORY = 'clah',
        MAXIMUM_NUMBER_OF_SUGGESTIONS = 'mxsg',
    };

    BCheckBox*  fUseInputHistoryCB;
    BCheckBox*  fUseSystemDictionaryCB;
    BCheckBox*  fUseRealtimeConversionCB;
    BSpinner*   fMaxSuggestionsS;
};


SuggestView::SuggestView(const char* name)
    : BView(name, B_WILL_DRAW)
{
    fUseInputHistoryCB = new BCheckBox("useInputHistoryCB",
        B_TRANSLATE("Use input history"), new BMessage(USE_INPUT_HISTORY));
    fUseSystemDictionaryCB = new BCheckBox("useSystemDictionaryCB",
        B_TRANSLATE("Use system dictionary"), new BMessage(USE_SYSTEM_DICTIONARY));
    fUseRealtimeConversionCB = new BCheckBox("useRealtimeConversionCB",
        B_TRANSLATE("Use realtime conversion"), new BMessage(USE_REALTIME_CONVERSION));

    fMaxSuggestionsS = new BSpinner("maximumNumberOfSuggestionsS", "",
        new BMessage(MAXIMUM_NUMBER_OF_SUGGESTIONS));
    fMaxSuggestionsS->SetRange(1, 9);

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("sourceDataL", B_TRANSLATE("Source data")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .Add(fUseInputHistoryCB)
            .AddGroup(B_HORIZONTAL)
                .AddGlue()
                .Add(new BButton("clearUnusedHistoryB",
                    B_TRANSLATE("Clear unused history"), new BMessage(CLEAR_UNUSED_HISTORY)))
                .Add(new BButton("clearAllHistoryB",
                    B_TRANSLATE("Clear all history"), new BMessage(CLEAR_ALL_HISTORY)))
            .End()
            .Add(fUseSystemDictionaryCB)
            .Add(fUseRealtimeConversionCB)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("sourceDataL", B_TRANSLATE("Source data")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .AddGroup(B_HORIZONTAL)
                .Add(new CStringView("maximumNumberOfSuggestionsL",
                    B_TRANSLATE("Maximum number of suggestions")))
                .AddGlue()
                .Add(fMaxSuggestionsS)
            .End()
        .End()
        .AddGlue();
}


class PrivacyView : public BView
{
public:
    PrivacyView(const char* name);
    virtual ~PrivacyView() {}

    enum {
        SECRET_MODE = 'secm',
        PRESENTATION_MODE = 'pres',
    };

    BCheckBox*  fSecretModeCB;
    BCheckBox*  fPresentationModeCB;
};


PrivacyView::PrivacyView(const char* name)
    : BView(name, B_WILL_DRAW)
{
    fSecretModeCB = new BCheckBox("secretModeL",
        B_TRANSLATE("Temporarily disable conversion personalization, "
            "history-based suggestions and user dictionary"),
        new BMessage(SECRET_MODE));

    fPresentationModeCB = new BCheckBox("presentationMode",
        B_TRANSLATE("Temporarily disable all suggestions"),
        new BMessage(PRESENTATION_MODE));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("secretModeL", B_TRANSLATE("Secret mode")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .Add(fSecretModeCB)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("presentationModeL", B_TRANSLATE("Presentation mode")))
            .Add(new BSeparatorView(B_HORIZONTAL))
        .End()
        .AddGroup(B_VERTICAL)
            .SetInsets(5, 0, 5, 0)
            .Add(fPresentationModeCB)
        .End()
        .AddGlue();
}


class ConfigDialog : public BWindow
{
public:
    ConfigDialog();
    virtual ~ConfigDialog();

    void MessageReceived(BMessage *msg);
    const char* windowTitle() { return Title(); }
private:
    enum Actions
    {
        OK = 'btok',
        CANCEL = 'btcl',
        RESET = 'btrs',
        APPLY = 'btal',
    };

    BButton* mpOkButton;
    BButton* mpCancelButton;
    BButton* mpApplyButton;

  virtual void ClearUserHistory();
  virtual void ClearUserPrediction();
  virtual void ClearUnusedUserPrediction();
  virtual void EditUserDictionary();
  virtual void EditKeymap();
  virtual void EditRomanTable();
  virtual void ResetToDefaults();
  virtual void SelectInputModeSetting(int index);
  virtual void SelectAutoConversionSetting(int state);
  virtual void SelectSuggestionSetting(int state);
  //
  virtual void EnableApplyButton();

  bool GetConfig(config::Config *config);
  bool SetConfig(const config::Config &config);
/*
  void SetSendStatsCheckBox();
  void GetSendStatsCheckBox() const;
  */
  void ConvertToProto(config::Config *config) const;
  void ConvertFromProto(const config::Config &config);
  bool Update();
  void Reload();

  std::unique_ptr<client::ClientInterface> client_;
  string custom_keymap_table_;
  string custom_roman_table_;
  config::Config::InformationListConfig information_list_config_;
  int initial_preedit_method_;
  bool initial_use_keyboard_to_change_preedit_method_;
  bool initial_use_mode_indicator_;
  map<std::string, config::Config::SessionKeymap> keymapname_sessionkeymap_map_;

  GeneralView* mpGeneralView;
  DictionaryView* mpDictionaryView;
  AdvanceView* mpAdvanceView;
  SuggestView* mpSuggestView;
  PrivacyView* mpPrivacyView;

  QDialogButtonBox* configDialogButtonBox;
  CompButton* editRomanTableButton;
  CompCheckBox* kutenCheckBox;
  CompCheckBox* toutenCheckBox;
  CompCheckBox* questionMarkCheckBox;
  CompCheckBox* exclamationMarkCheckBox;
  CompCheckBox* historySuggestCheckBox;
  CompCheckBox* dictionarySuggestCheckBox;
  CompCheckBox* realtimeConversionCheckBox;
  CompCheckBox* presentationModeCheckBox;

  QComboBox* inputModeComboBox;
  QComboBox* punctuationsSettingComboBox;
  QComboBox* symbolsSettingComboBox;
  QComboBox* spaceCharacterFormComboBox;
  QComboBox* selectionShortcutModeComboBox;
  QComboBox* numpadCharacterFormComboBox;
  QComboBox* keymapSettingComboBox;

  QComboBox* historyLearningLevelComboBox;
  CompCheckBox* singleKanjiConversionCheckBox;
  CompCheckBox* symbolConversionCheckBox;
  CompCheckBox* emoticonConversionCheckBox;
  CompCheckBox* dateConversionCheckBox;
  CompCheckBox* emojiConversionCheckBox;
  CompCheckBox* numberConversionCheckBox;
  CompCheckBox* calculatorCheckBox;
  CompCheckBox* t13nConversionCheckBox;
  CompCheckBox* zipcodeConversionCheckBox;
  CompCheckBox* spellingCorrectionCheckBox;

  CompCheckBox* useAutoImeTurnOff;
  CompCheckBox* useAutoConversion;
  QComboBox* shiftKeyModeSwitchComboBox;

  CompSpinBox* suggestionsSizeSpinBox;

  CompCheckBox* incognitoModeCheckBox;
};

ConfigDialog::ConfigDialog()
    : BWindow(
        BRect(50, 50, 525, 440),
        B_TRANSLATE("Mozc Settings"),
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS |
        B_CLOSE_ON_ESCAPE | B_QUIT_ON_WINDOW_CLOSE),
      client_(client::ClientFactory::NewClient()),
      initial_preedit_method_(0),
      initial_use_keyboard_to_change_preedit_method_(false),
      initial_use_mode_indicator_(true)
{
    mpGeneralView = new GeneralView(B_TRANSLATE("General"));
    mpDictionaryView = new DictionaryView(B_TRANSLATE("Dictionary"));
    mpAdvanceView = new AdvanceView(B_TRANSLATE("Advanced"));
    mpSuggestView = new SuggestView(B_TRANSLATE("Suggest"));
    mpPrivacyView = new PrivacyView(B_TRANSLATE("Privacy"));
    BTabView *tabview = new BTabView("tabview", B_WIDTH_FROM_LABEL);
    tabview->AddTab(mpGeneralView);
    tabview->AddTab(mpDictionaryView);
    tabview->AddTab(mpAdvanceView);
    tabview->AddTab(mpSuggestView);
    tabview->AddTab(mpPrivacyView);
    tabview->SetBorder(B_NO_BORDER);

    mpOkButton = new BButton(B_TRANSLATE("OK"), new BMessage(OK));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));
    mpApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(APPLY));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(0, 5, 0, 5)
        .Add(tabview)
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_HORIZONTAL)
            .SetInsets(5, 0)
            .Add(new BButton(B_TRANSLATE("Reset to defaults"),
                        new BMessage(RESET)))
            .AddGlue()
            .AddGroup(B_HORIZONTAL)
                .Add(mpOkButton)
                .Add(mpCancelButton)
                .Add(mpApplyButton)
            .End();

  keymapname_sessionkeymap_map_[B_TRANSLATE("ATOK")] = config::Config::ATOK;
  keymapname_sessionkeymap_map_[B_TRANSLATE("MS-IME")] = config::Config::MSIME;
  keymapname_sessionkeymap_map_[B_TRANSLATE("Kotoeri")] = config::Config::KOTOERI;

    CharacterFormColumn<CharacterFormRow>* preeditColumn =
            new CharacterFormColumn<CharacterFormRow>(
                mpAdvanceView->mpCharacterFormGridView, 1, this);
    CharacterFormColumn<CharacterFormRow>* conversionColumn =
            new CharacterFormColumn<CharacterFormRow>(
                mpAdvanceView->mpCharacterFormGridView, 2, this);
    preeditColumn->AddForm(B_TRANSLATE("Halfwidth"));
    preeditColumn->AddForm(B_TRANSLATE("Fullwidth"));
    preeditColumn->AddForm(B_TRANSLATE("Remember"));
    conversionColumn->AddForm(B_TRANSLATE("Halfwidth"));
    conversionColumn->AddForm(B_TRANSLATE("Fullwidth"));
    conversionColumn->AddForm(B_TRANSLATE("Remember"));

    mpAdvanceView->mpCharacterFormGridView->AddColumn(
        new CharacterFormGroupColumn<CharacterFormRow>(mpAdvanceView->mpCharacterFormGridView, 0));
    mpAdvanceView->mpCharacterFormGridView->AddColumn(preeditColumn);
    mpAdvanceView->mpCharacterFormGridView->AddColumn(conversionColumn);

  mpAdvanceView->mpCharacterFormGridView->SetTarget(this);

  configDialogButtonBox = new QDialogButtonBox(mpOkButton, mpCancelButton, mpApplyButton);
  editRomanTableButton = new CompButton(mpGeneralView->mpEditRomanTableButton);
  kutenCheckBox = new CompCheckBox(mpAdvanceView->mpKutenCB);
  toutenCheckBox = new CompCheckBox(mpAdvanceView->mpToutenCB);
  questionMarkCheckBox = new CompCheckBox(mpAdvanceView->mpQuestionMarkCB);
  exclamationMarkCheckBox = new CompCheckBox(mpAdvanceView->mpExclamationMarkCB);
  historySuggestCheckBox = new CompCheckBox(mpSuggestView->fUseInputHistoryCB);
  dictionarySuggestCheckBox = new CompCheckBox(mpSuggestView->fUseSystemDictionaryCB);
  realtimeConversionCheckBox = new CompCheckBox(mpSuggestView->fUseRealtimeConversionCB);
  presentationModeCheckBox = new CompCheckBox(mpPrivacyView->fPresentationModeCB);

  inputModeComboBox = new QComboBox(mpGeneralView->inputMethodPM);
  punctuationsSettingComboBox = new QComboBox(mpGeneralView->punctuationPM);
  symbolsSettingComboBox = new QComboBox(mpGeneralView->symbolPM);
  spaceCharacterFormComboBox = new QComboBox(mpGeneralView->spaceInputPM);
  selectionShortcutModeComboBox = new QComboBox(mpGeneralView->candidatePM);
  numpadCharacterFormComboBox = new QComboBox(mpGeneralView->numpadPM);
  keymapSettingComboBox = new QComboBox(mpGeneralView->keymapPM);

  historyLearningLevelComboBox = new QComboBox(mpDictionaryView->personaPM);
  singleKanjiConversionCheckBox = new CompCheckBox(mpDictionaryView->fSingleKanjiCB);
  symbolConversionCheckBox = new CompCheckBox(mpDictionaryView->fSymbolCB);
  emoticonConversionCheckBox = new CompCheckBox(mpDictionaryView->fEmoticonCB);
  dateConversionCheckBox = new CompCheckBox(mpDictionaryView->fDateCB);
  emojiConversionCheckBox = new CompCheckBox(mpDictionaryView->fEmojiCB);
  numberConversionCheckBox = new CompCheckBox(mpDictionaryView->fSpecialNumberCB);
  calculatorCheckBox = new CompCheckBox(mpDictionaryView->fCalculatorCB);
  t13nConversionCheckBox = new CompCheckBox(mpDictionaryView->fKatakanaCB);
  zipcodeConversionCheckBox = new CompCheckBox(mpDictionaryView->fPostalCodeCB);
  spellingCorrectionCheckBox = new CompCheckBox(mpDictionaryView->fSpellingCorrectionCB);

  useAutoImeTurnOff = new CompCheckBox(mpAdvanceView->fAutoHalfwidthCB);
  useAutoConversion = new CompCheckBox(mpAdvanceView->fConvertPunctuationsCB);
  shiftKeyModeSwitchComboBox = new QComboBox(mpAdvanceView->shiftSwitchPM);

  suggestionsSizeSpinBox = new CompSpinBox(mpSuggestView->fMaxSuggestionsS);

  incognitoModeCheckBox = new CompCheckBox(mpPrivacyView->fSecretModeCB);

  mpApplyButton->SetEnabled(false);

  Reload();

  Layout(true);
}

ConfigDialog::~ConfigDialog() {
  delete configDialogButtonBox;
  delete editRomanTableButton;
  delete kutenCheckBox;
  delete toutenCheckBox;
  delete questionMarkCheckBox;
  delete exclamationMarkCheckBox;
  delete historySuggestCheckBox;
  delete dictionarySuggestCheckBox;
  delete realtimeConversionCheckBox;
  delete presentationModeCheckBox;

  delete inputModeComboBox;
  delete punctuationsSettingComboBox;
  delete symbolsSettingComboBox;
  delete spaceCharacterFormComboBox;
  delete selectionShortcutModeComboBox;
  delete numpadCharacterFormComboBox;
  delete keymapSettingComboBox;

  delete historyLearningLevelComboBox;
  delete singleKanjiConversionCheckBox;
  delete symbolConversionCheckBox;
  delete emoticonConversionCheckBox;
  delete dateConversionCheckBox;
  delete emojiConversionCheckBox;
  delete numberConversionCheckBox;
  delete calculatorCheckBox;
  delete t13nConversionCheckBox;
  delete zipcodeConversionCheckBox;
  delete spellingCorrectionCheckBox;

  delete useAutoImeTurnOff;
  delete useAutoConversion;
  delete shiftKeyModeSwitchComboBox;

  delete suggestionsSizeSpinBox;

  delete incognitoModeCheckBox;
}

bool ConfigDialog::SetConfig(const config::Config &config) {
  if (!client_->CheckVersionOrRestartServer()) {
    LOG(ERROR) << "CheckVersionOrRestartServer failed";
    return false;
  }

  if (!client_->SetConfig(config)) {
    LOG(ERROR) << "SetConfig failed";
    return false;
  }

  return true;
}

bool ConfigDialog::GetConfig(config::Config *config) {
  if (!client_->CheckVersionOrRestartServer()) {
    LOG(ERROR) << "CheckVersionOrRestartServer failed";
    return false;
  }

  if (!client_->GetConfig(config)) {
    LOG(ERROR) << "GetConfig failed";
    return false;
  }

  return true;
}

void ConfigDialog::Reload() {
  config::Config config;
  if (!GetConfig(&config)) {
    QMessageBox::critical(this, windowTitle(),
                          tr("Failed to get current config values."));
  }
  ConvertFromProto(config);

  SelectAutoConversionSetting(static_cast<int>(config.use_auto_conversion()));

  initial_preedit_method_ = static_cast<int>(config.preedit_method());
  initial_use_keyboard_to_change_preedit_method_ =
      config.use_keyboard_to_change_preedit_method();
  initial_use_mode_indicator_ = config.use_mode_indicator();
}

bool ConfigDialog::Update() {
  config::Config config;
  ConvertToProto(&config);

  if (config.session_keymap() == config::Config::CUSTOM &&
      config.custom_keymap_table().empty()) {
    QMessageBox::warning(this, windowTitle(),
        tr("The current custom keymap table is empty. "
           "When custom keymap is selected, "
           "you must customize it."));
    return false;
  }

  if (!SetConfig(config)) {
    QMessageBox::critical(this, windowTitle(),
                          tr("Failed to update config"));
  }

  return true;
}

// void ConfigDialog::SetSendStatsCheckBox()
// void ConfigDialog::GetSendStatsCheckBox() const

#define SET_COMBOBOX(combobox, enumname, field) \
do { \
  (combobox)->setCurrentIndex(static_cast<int>(config.field()));  \
} while (0)

#define SET_CHECKBOX(checkbox, field) \
do { (checkbox)->setChecked(config.field()); } while (0)

#define GET_COMBOBOX(combobox, enumname, field) \
do {  \
  config->set_##field(static_cast<config::Config_##enumname> \
                   ((combobox)->currentIndex())); \
} while (0)

#define GET_CHECKBOX(checkbox, field) \
do { config->set_##field((checkbox)->isChecked());  } while (0)


namespace {
static const int kPreeditMethodSize = 2;

void SetComboboxForPreeditMethod(const config::Config &config,
                                 QComboBox *combobox) {
  int index = static_cast<int>(config.preedit_method());
  combobox->setCurrentIndex(index);
}

void GetComboboxForPreeditMethod(const QComboBox *combobox,
                                 config::Config *config) {
  int index = combobox->currentIndex();
  if (index >= kPreeditMethodSize) {
    // |use_keyboard_to_change_preedit_method| should be true and
    // |index| should be adjusted to smaller than kPreeditMethodSize.
    config->set_preedit_method(
        static_cast<config::Config_PreeditMethod>(index - kPreeditMethodSize));
    config->set_use_keyboard_to_change_preedit_method(true);
  } else {
    config->set_preedit_method(
        static_cast<config::Config_PreeditMethod>(index));
    config->set_use_keyboard_to_change_preedit_method(false);
  }
}
} // namespace

void ConfigDialog::ConvertFromProto(const config::Config &config) {
  // tab1
  SetComboboxForPreeditMethod(config, inputModeComboBox);
  SET_COMBOBOX(punctuationsSettingComboBox, PunctuationMethod,
               punctuation_method);
  SET_COMBOBOX(symbolsSettingComboBox, SymbolMethod, symbol_method);
  SET_COMBOBOX(spaceCharacterFormComboBox, FundamentalCharacterForm,
               space_character_form);
  SET_COMBOBOX(selectionShortcutModeComboBox, SelectionShortcut,
               selection_shortcut);
  SET_COMBOBOX(numpadCharacterFormComboBox, NumpadCharacterForm,
               numpad_character_form);
  SET_COMBOBOX(keymapSettingComboBox, SessionKeymap, session_keymap);

  custom_keymap_table_ = config.custom_keymap_table();
  custom_roman_table_ = config.custom_roman_table();

  // tab2
  SET_COMBOBOX(historyLearningLevelComboBox, HistoryLearningLevel,
               history_learning_level);
  SET_CHECKBOX(singleKanjiConversionCheckBox, use_single_kanji_conversion);
  SET_CHECKBOX(symbolConversionCheckBox, use_symbol_conversion);
  SET_CHECKBOX(emoticonConversionCheckBox, use_emoticon_conversion);
  SET_CHECKBOX(dateConversionCheckBox, use_date_conversion);
  SET_CHECKBOX(emojiConversionCheckBox, use_emoji_conversion);
  SET_CHECKBOX(numberConversionCheckBox, use_number_conversion);
  SET_CHECKBOX(calculatorCheckBox, use_calculator);
  SET_CHECKBOX(t13nConversionCheckBox, use_t13n_conversion);
  SET_CHECKBOX(zipcodeConversionCheckBox, use_zip_code_conversion);
  SET_CHECKBOX(spellingCorrectionCheckBox, use_spelling_correction);

  // tab3
  SET_CHECKBOX(useAutoImeTurnOff, use_auto_ime_turn_off);

  SET_CHECKBOX(useAutoConversion, use_auto_conversion);
  kutenCheckBox->setChecked(
      config.auto_conversion_key() &
      config::Config::AUTO_CONVERSION_KUTEN);
  toutenCheckBox->setChecked(
      config.auto_conversion_key() &
      config::Config::AUTO_CONVERSION_TOUTEN);
  questionMarkCheckBox->setChecked(
      config.auto_conversion_key() &
      config::Config::AUTO_CONVERSION_QUESTION_MARK);
  exclamationMarkCheckBox->setChecked(
      config.auto_conversion_key() &
      config::Config::AUTO_CONVERSION_EXCLAMATION_MARK);

  SET_COMBOBOX(shiftKeyModeSwitchComboBox,
               ShiftKeyModeSwitch,
               shift_key_mode_switch);

  //SET_CHECKBOX(useJapaneseLayout, use_japanese_layout);

  //SET_CHECKBOX(useModeIndicator, use_mode_indicator);

  // tab4
  SET_CHECKBOX(historySuggestCheckBox, use_history_suggest);
  SET_CHECKBOX(dictionarySuggestCheckBox, use_dictionary_suggest);
  SET_CHECKBOX(realtimeConversionCheckBox, use_realtime_conversion);

  suggestionsSizeSpinBox->setValue
      (max(1, min(9, static_cast<int>(config.suggestions_size()))));

  // tab5
  //SetSendStatsCheckBox();
  SET_CHECKBOX(incognitoModeCheckBox, incognito_mode);
  SET_CHECKBOX(presentationModeCheckBox, presentation_mode);

  // tab6
  //SET_COMBOBOX(verboseLevelComboBox, int, verbose_level);
  //SET_CHECKBOX(checkDefaultCheckBox, check_default);
  //SET_COMBOBOX(yenSignComboBox, YenSignCharacter, yen_sign_character);

  //characterFormEditor->Load(config);
  //SET_CHECKBOX(cloudHandwritingCheckBox, allow_cloud_handwriting);

  mpAdvanceView->mpCharacterFormGridView->Load(config);
}

void ConfigDialog::ConvertToProto(config::Config *config) const {
  // tab1
  GetComboboxForPreeditMethod(inputModeComboBox, config);
  GET_COMBOBOX(punctuationsSettingComboBox, PunctuationMethod,
               punctuation_method);
  GET_COMBOBOX(symbolsSettingComboBox, SymbolMethod, symbol_method);
  GET_COMBOBOX(spaceCharacterFormComboBox, FundamentalCharacterForm,
               space_character_form);
  GET_COMBOBOX(selectionShortcutModeComboBox, SelectionShortcut,
               selection_shortcut);
  GET_COMBOBOX(numpadCharacterFormComboBox, NumpadCharacterForm,
               numpad_character_form);
  GET_COMBOBOX(keymapSettingComboBox, SessionKeymap, session_keymap);

  config->set_custom_keymap_table(custom_keymap_table_);

  config->clear_custom_roman_table();
  if (!custom_roman_table_.empty()) {
    config->set_custom_roman_table(custom_roman_table_);
  }

  // tab2
  GET_COMBOBOX(historyLearningLevelComboBox, HistoryLearningLevel,
               history_learning_level);
  GET_CHECKBOX(singleKanjiConversionCheckBox, use_single_kanji_conversion);
  GET_CHECKBOX(symbolConversionCheckBox, use_symbol_conversion);
  GET_CHECKBOX(emoticonConversionCheckBox, use_emoticon_conversion);
  GET_CHECKBOX(dateConversionCheckBox, use_date_conversion);
  GET_CHECKBOX(emojiConversionCheckBox, use_emoji_conversion);
  GET_CHECKBOX(numberConversionCheckBox, use_number_conversion);
  GET_CHECKBOX(calculatorCheckBox, use_calculator);
  GET_CHECKBOX(t13nConversionCheckBox, use_t13n_conversion);
  GET_CHECKBOX(zipcodeConversionCheckBox, use_zip_code_conversion);
  GET_CHECKBOX(spellingCorrectionCheckBox, use_spelling_correction);

  config->mutable_information_list_config()->set_use_local_usage_dictionary(false);

  // tab3
  GET_CHECKBOX(useAutoImeTurnOff, use_auto_ime_turn_off);

  GET_CHECKBOX(useAutoConversion, use_auto_conversion);

  //GET_CHECKBOX(useJapaneseLayout, use_japanese_layout);

  //GET_CHECKBOX(useModeIndicator, use_mode_indicator);

  uint32 auto_conversion_key = 0;
  if (kutenCheckBox->isChecked()) {
    auto_conversion_key |=
        config::Config::AUTO_CONVERSION_KUTEN;
  }
  if (toutenCheckBox->isChecked()) {
    auto_conversion_key |=
        config::Config::AUTO_CONVERSION_TOUTEN;
  }
  if (questionMarkCheckBox->isChecked()) {
    auto_conversion_key |=
        config::Config::AUTO_CONVERSION_QUESTION_MARK;
  }
  if (exclamationMarkCheckBox->isChecked()) {
    auto_conversion_key |=
        config::Config::AUTO_CONVERSION_EXCLAMATION_MARK;
  }
  config->set_auto_conversion_key(auto_conversion_key);

  GET_COMBOBOX(shiftKeyModeSwitchComboBox,
               ShiftKeyModeSwitch,
               shift_key_mode_switch);

  // tab4
  GET_CHECKBOX(historySuggestCheckBox, use_history_suggest);
  GET_CHECKBOX(dictionarySuggestCheckBox, use_dictionary_suggest);
  GET_CHECKBOX(realtimeConversionCheckBox, use_realtime_conversion);

  config->set_suggestions_size
      (static_cast<uint32>(suggestionsSizeSpinBox->value()));

  // tab5
  //GetSendStatsCheckBox();
  GET_CHECKBOX(incognitoModeCheckBox, incognito_mode);
  GET_CHECKBOX(presentationModeCheckBox, presentation_mode);

  // tab6
  //config->set_verbose_level(verboseLevelComboBox->currentIndex());
  //GET_CHECKBOX(checkDefaultCheckBox, check_default);
  //GET_COMBOBOX(yenSignComboBox, YenSignCharacter, yen_sign_character);

  //characterFormEditor->Save(config);

  mpAdvanceView->mpCharacterFormGridView->Save(config);
}

//void ConfigDialog::clicked(QAbstractButton *button)

void ConfigDialog::ClearUserHistory() {
  if (QMessageBox::Ok !=
      QMessageBox::question(
          this,
          windowTitle(),
          tr("Do you want to clear personalization data? "
             "Input history is not reset with this operation. "
             "Please open \"suggestion\" tab to remove input history data."),
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Cancel)) {
    return;
  }

  client_->CheckVersionOrRestartServer();

  if (!client_->ClearUserHistory()) {
    QMessageBox::critical(
        this,
        windowTitle(),
        tr("Mozc Converter is not running. "
           "Settings were not saved."));
  }
}

void ConfigDialog::ClearUserPrediction() {
  if (QMessageBox::Ok !=
      QMessageBox::question(
          this,
          windowTitle(),
          tr("Do you want to clear all history data?"),
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Cancel)) {
    return;
  }

  client_->CheckVersionOrRestartServer();

  if (!client_->ClearUserPrediction()) {
    QMessageBox::critical(
        this,
        windowTitle(),
        tr("Mozc Converter is not running. "
           "Settings were not saved."));
  }
}

void ConfigDialog::ClearUnusedUserPrediction() {
  if (QMessageBox::Ok !=
      QMessageBox::question(
          this,
          windowTitle(),
          tr("Do you want to clear unused history data?"),
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Cancel)) {
    return;
  }

  client_->CheckVersionOrRestartServer();

  if (!client_->ClearUnusedUserPrediction()) {
    QMessageBox::critical(
        this,
        windowTitle(),
        tr("Mozc Converter is not running. "
           "Operation was not executed."));
  }
}

void ConfigDialog::EditUserDictionary() {
  client_->LaunchTool("dictionary_tool", "");
}

void ConfigDialog::EditKeymap() {
  std::string current_keymap_table = "";
  const std::string keymap_name = keymapSettingComboBox->currentText().getp();
  const map<std::string, config::Config::SessionKeymap>::const_iterator itr =
      keymapname_sessionkeymap_map_.find(keymap_name);
  if (itr != keymapname_sessionkeymap_map_.end()) {
    // Load from predefined mapping file.
    const char *keymap_file =
        keymap::KeyMapManager::GetKeyMapFileName(itr->second);
    std::unique_ptr<istream> ifs(
        ConfigFileStream::LegacyOpen(keymap_file));
    CHECK(ifs.get() != NULL);  // should never happen
    stringstream buffer;
    buffer << ifs->rdbuf();
    current_keymap_table = buffer.str();
  } else {
    current_keymap_table = custom_keymap_table_;
  }

  KeyMapEditorDialog<KeymapRow>* window =
        new KeyMapEditorDialog<KeymapRow>(this, 3, current_keymap_table);
  window->Show();
}

void ConfigDialog::EditRomanTable() {
  RomanTableEditorDialog<RomanTableRow>* window =
        new RomanTableEditorDialog<RomanTableRow>(this, 3, custom_roman_table_);
  window->Show();
}

void ConfigDialog::SelectInputModeSetting(int index) {
  // enable "EDIT" button if roman mode is selected
  editRomanTableButton->setEnabled((index == 0));
}

void ConfigDialog::SelectAutoConversionSetting(int state) {
  kutenCheckBox->setEnabled(static_cast<bool>(state));
  toutenCheckBox->setEnabled(static_cast<bool>(state));
  questionMarkCheckBox->setEnabled(static_cast<bool>(state));
  exclamationMarkCheckBox->setEnabled(static_cast<bool>(state));
}

void ConfigDialog::SelectSuggestionSetting(int state) {
  if (historySuggestCheckBox->isChecked() ||
      dictionarySuggestCheckBox->isChecked() ||
      realtimeConversionCheckBox->isChecked()) {
    presentationModeCheckBox->setEnabled(true);
  } else {
    presentationModeCheckBox->setEnabled(false);
  }
}

void ConfigDialog::ResetToDefaults() {
  if (QMessageBox::Ok ==
      QMessageBox::question(
          this,
          windowTitle(),
          tr("When you reset Mozc settings, any changes "
             "you've made will be reverted to the default settings. "
             "Do you want to reset settings? "
             "The following items are not reset with this operation.\n"
             " - Personalization data\n"
             " - Input history\n"
             " - Usage statistics and crash reports\n"
             " - Administrator settings"),
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Cancel)) {
    // TODO(taku): remove the dependency to config::ConfigHandler
    // nice to have GET_DEFAULT_CONFIG command
    config::Config config;
    config::ConfigHandler::GetDefaultConfig(&config);
    ConvertFromProto(config);
  }
}

// void ConfigDialog::LaunchAdministrationDialog()

void ConfigDialog::EnableApplyButton() {
  configDialogButtonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

// bool ConfigDialog::eventFilter(QObject *obj, QEvent *event)

void ConfigDialog::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case OK:
        {
            if (Update()) {
                Quit();
            }
            break;
        }
        case CANCEL:
        {
            QuitRequested();
            Quit();
            break;
        }
        case RESET:
        {
            ResetToDefaults();
            break;
        }
        case APPLY:
        {
            Update();
            break;
        }
        case GeneralView::INPUT_METHOD_ROMAJI:
        {
            SelectInputModeSetting(0);
            break;
        }
        case GeneralView::INPUT_METHOD_KANA:
        {
            SelectInputModeSetting(1);
            break;
        }
        case GeneralView::KEYMAP_EDIT:
        {
            EditKeymap();
            break;
        }
        case GeneralView::ROMAJI_TABLE_EDIT:
        {
            EditRomanTable();
            break;
        }
        case DictionaryView::PERSONA_DATA_CLEAR:
        {
            ClearUserHistory();
            break;
        }
        case DictionaryView::USER_DICT_EDIT:
        {
            EditUserDictionary();
            break;
        }
        case AdvanceView::CONVERT_PUNCTUATIONS:
        {
            SelectAutoConversionSetting(useAutoConversion->isChecked());
            break;
        }
        case SuggestView::CLEAR_UNUSED_HISTORY:
        {
            ClearUnusedUserPrediction();
            break;
        }
        case SuggestView::CLEAR_ALL_HISTORY:
        {
            ClearUserPrediction();
            break;
        }
        case RomanTableEditorDialog<RomanTableRow>::ROMAN_TABLE_UPDATED:
        {
            const char* output = msg->GetString("output", NULL);
            if (output) {
                custom_roman_table_ = output;
            }
            break;
        }
        case KeyMapEditorDialog<KeymapRow>::KEYMAP_UPDATED:
        {
            const char* output = msg->GetString("output", NULL);
            if (output) {
                custom_keymap_table_ = output;
                // choose Custom in the combobox
                keymapSettingComboBox->setCurrentIndex(0);
            }
            break;
        }
        case GridColumn<RomanTableRow>::MODIFIED:
        {
            mpAdvanceView->mpCharacterFormGridView->Invalidate();
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}


class ConfigApp : public ToolApp
{
public:
    ConfigApp();
    virtual ~ConfigApp() {}
};

ConfigApp::ConfigApp()
    : ToolApp(CONFIG_DIALOG)
{
    mpWindow = new ConfigDialog();
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}

} // haiku_gui
} // mozc

int HaikuRunConfigDialog(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                        mozc::haiku_gui::CONFIG_DIALOG)) {
        return -1;
    }

    mozc::haiku_gui::ConfigApp* app = new mozc::haiku_gui::ConfigApp();
    app->Run();
    delete app;

    return 0;
}
