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

#include "haiku/haiku_gui/config_dialog/keybinding_editor.h"
#include "haiku/haiku_gui/base/key_filter.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include "base/logging.h"
#include "base/util.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>

#include <string>
#include <vector>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "config_dialog"

namespace mozc {
namespace haiku_gui {

namespace {

// Special keys defined by custom keymap.
enum {
    // 0x6c
    K_MUHENKAN = 0xf4,
    // 0x6d
    K_HENKAN = 0xf5,
};

struct HaikuKeyEntry {
    int32 haiku_key;
    const char* mozc_key_name;
};

const HaikuKeyEntry kHakuKeyModifierNonRequiredTable[] = {
    { B_BACKSPACE, "Backspace" },
    { B_RETURN, "Enter" },
    { B_ENTER, "Enter" },
    { B_SPACE, "Space" },
    { B_TAB, "Tab" },
    { B_ESCAPE, "Escape" },
    { B_LEFT_ARROW, "Left" },
    { B_RIGHT_ARROW, "Right" },
    { B_UP_ARROW, "Up" },
    { B_DOWN_ARROW, "Down" },
    { B_INSERT, "Insert" },
    { B_DELETE, "Delete" },
    { B_HOME, "Home" },
    { B_END, "End" },
    { B_PAGE_UP, "PageUp" },
    { B_PAGE_DOWN, "PageDown" },
    //{ B_KATAKANA_HIRAGANA, "Hiragana" }, // with Shift makes Katakana
    { B_HANKAKU_ZENKAKU, "Hankaku/Zenkaku" },
    { K_MUHENKAN, "Muhenkan" },
    { K_HENKAN, "Henkan" },
};

const HaikuKeyEntry kHaikuKeyFunctions[] = {
    { B_F1_KEY, "F1" },
    { B_F2_KEY, "F2" },
    { B_F3_KEY, "F3" },
    { B_F4_KEY, "F4" },
    { B_F5_KEY, "F5" },
    { B_F6_KEY, "F6" },
    { B_F7_KEY, "F7" },
    { B_F8_KEY, "F8" },
    { B_F9_KEY, "F9" },
    { B_F10_KEY, "F10" },
    { B_F11_KEY, "F11" },
    { B_F12_KEY, "F12" },
};

bool IsAlphabet(const char key) {
  return (key >= 'a' && key <= 'z');
}
}  // namespace


class KeyView : public BView
{
public:
    KeyView();
    virtual ~KeyView() {};

    virtual void MessageReceived(BMessage* msg);
    virtual void MakeFocus(bool focus);
    virtual void Draw(BRect rect);
    virtual void MouseDown(BPoint pos);
    virtual BSize MinSize();

    const char* Label() { return maLabel.c_str(); }
    void SetLabel(const char* label);
    
  enum KeyState {
    DENY_KEY,
    ACCEPT_KEY,
    SUBMIT_KEY
  };
  enum {
    STATE_CHANGED = 'stch',
  };
private:
    std::string maLabel;

    bool committed_;
    bool ctrl_pressed_;
    bool alt_pressed_;
    bool shift_pressed_;
    std::string modifier_required_key_;
    std::string modifier_non_required_key_;
    std::string unknown_key_;

    void Reset();
    KeyState Encode(std::string *result);
    KeyState AddKey(int32 key, uint8 byte, uint32 mods, std::string *result);
};

KeyView::KeyView()
    : BView(
        BRect(0, 0, 0, 0),
        "keyview",
        B_FOLLOW_NONE,
        B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_NAVIGABLE),
      committed_(false),
      ctrl_pressed_(false),
      alt_pressed_(false),
      shift_pressed_(false)
{
    AddFilter(new KeyFilter(this));
    Reset();
}

void KeyView::Reset()
{
  ctrl_pressed_ = false;
  alt_pressed_ = false;
  shift_pressed_ = false;
  modifier_required_key_.clear();
  modifier_non_required_key_.clear();
  unknown_key_.clear();
  committed_ = true;
}

KeyView::KeyState KeyView::Encode(std::string *result)
{
  if (modifier_non_required_key_ == "Hiragana" ||
      modifier_non_required_key_ == "Katakana" ||
      modifier_non_required_key_ == "Eisu" ||
      modifier_non_required_key_ == "Hankaku/Zenkaku") {
    *result = modifier_non_required_key_;
    return KeyView::SUBMIT_KEY;
  }
  
  std::vector<std::string> results;
  if (ctrl_pressed_) {
    results.push_back("Ctrl");
  }
  
  if (shift_pressed_) {
    results.push_back("Shift");
  }
  
  const bool has_modifier = !results.empty();

  if (!modifier_non_required_key_.empty()) {
    results.push_back(modifier_non_required_key_);
  }

  if (!modifier_required_key_.empty()) {
    results.push_back(modifier_required_key_);
  }
  
  // in release binary, unknown_key_ is hidden
#ifndef NO_LOGGING
  if (!unknown_key_.empty()) {
    results.push_back(unknown_key_);
  }
#endif
  KeyView::KeyState result_state = KeyView::ACCEPT_KEY;
  
  if (!unknown_key_.empty()) {
    result_state = KeyView::DENY_KEY;
  }
  
  const char key = modifier_required_key_.empty() ?
      0 : modifier_required_key_[0];

  // Alt or Ctrl or these combinations
  if ((alt_pressed_ || ctrl_pressed_) &&
      modifier_non_required_key_.empty() &&
      modifier_required_key_.empty()) {
    result_state = KeyView::DENY_KEY;
  }
  
  // Don't support Shift only
  if (shift_pressed_ && !ctrl_pressed_ && !alt_pressed_ &&
      modifier_required_key_.empty() &&
      modifier_non_required_key_.empty()) {
    result_state = KeyView::DENY_KEY;
  }
  
  // Don't support Shift + 'a' only
  if (shift_pressed_ && !ctrl_pressed_ && !alt_pressed_ &&
      !modifier_required_key_.empty() && IsAlphabet(key)) {
    result_state = KeyView::DENY_KEY;
  }

  // Don't support Shift + Ctrl + '@'
  if (shift_pressed_ && !modifier_required_key_.empty() &&
      !IsAlphabet(key)) {
    result_state = KeyView::DENY_KEY;
  }
  
  // no modifer for modifier_required_key
  if (!has_modifier && !modifier_required_key_.empty()) {
    result_state = KeyView::DENY_KEY;
  }

  // modifier_required_key and modifier_non_required_key
  // cannot co-exist
  if (!modifier_required_key_.empty() &&
      !modifier_non_required_key_.empty()) {
    result_state = KeyView::DENY_KEY;
  }

  // no valid key
  if (results.empty()) {
    result_state = KeyView::DENY_KEY;
  }
  
  std::string s;
  for (int i = 0; i < results.size(); ++i) {
    s += results[i];
    if (i < results.size() - 1) {
      s += ' ';
    }
  }
  
  *result = s;
  
  return result_state;
}

KeyView::KeyState KeyView::AddKey(int32 key, uint8 byte, uint32 mods, std::string *result)
{
    if (mods & B_COMMAND_KEY) {
        ctrl_pressed_ = true;
    }
    if (mods & B_OPTION_KEY) {
    }
    if (mods & B_CONTROL_KEY) {
        alt_pressed_ = true;
    }
    if (mods & B_SHIFT_KEY) {
        shift_pressed_ = true;
    }
    
    for (size_t i = 0; i < arraysize(kHakuKeyModifierNonRequiredTable); ++i) {
        if (kHakuKeyModifierNonRequiredTable[i].haiku_key == byte) {
            modifier_non_required_key_ =
                kHakuKeyModifierNonRequiredTable[i].mozc_key_name;
            return Encode(result);
        }
    }
    if (byte == B_F1_KEY && B_F1_KEY <= key && key <= B_F12_KEY) {
        modifier_non_required_key_ =
            kHaikuKeyFunctions[key - B_F1_KEY].mozc_key_name;
        return Encode(result);
    }
    if (byte == B_KATAKANA_HIRAGANA) {
        if (shift_pressed_) {
            modifier_non_required_key_ = "Katakana";
        } else {
            modifier_non_required_key_ = "Hiragana";
        }
        return Encode(result);
    }
    // alphabets comes as lowercase on Haiku
    if ((0x21 <= byte && byte <= 0x40) ||
        (0x5B <= byte && byte <= 0x7E)) {
        //(0x7B <= byte && byte <= 0x7E)) {
        modifier_required_key_ = static_cast<char>(byte);
        return Encode(result);
    }
    
    char s[64];
    sprintf(s, "<UNK:0x%x 0x%x 0x%x>", byte, key, mods);
    unknown_key_ = static_cast<const char*>(s);
    
    return Encode(result);
}

void KeyView::SetLabel(const char* label)
{
    maLabel = label;
    Invalidate();
}

BSize KeyView::MinSize()
{
    font_height height;
    GetFontHeight(&height);

    return BSize(100, floor((height.ascent + height.descent)) * 1.4);
}

void KeyView::MakeFocus(bool focus)
{
    const bool b = IsFocus();
    BView::MakeFocus(focus);
    if (b != focus) {
        Invalidate();
    }
}

void KeyView::Draw(BRect rect)
{
    BRect bounds = Bounds();

    BPoint pos(5, bounds.Height() * 0.75);
    DrawString(maLabel.c_str(), pos);

    be_control_look->DrawTextControlBorder(this, bounds, rect,
        LowColor(),
        IsFocus() && Window()->IsActive() ? BControlLook::B_FOCUSED : 0);
}

void KeyView::MouseDown(BPoint p)
{
    MakeFocus(true);
}

void KeyView::MessageReceived(BMessage* msg)
{
    if (msg->what == B_KEY_DOWN)  {
        int32 key = msg->GetInt32("key", 0);
        uint8 byte = static_cast<uint8>(msg->GetInt8("byte", 0));
        uint32 mods = static_cast<uint32>(msg->GetInt32("modifiers", 0));
        
        std::string result;
        const KeyView::KeyState state = AddKey(key, byte, mods, &result);
        if (state != KeyView::DENY_KEY) {
            SetLabel(result.c_str());
        } else {
            SetLabel("");
        }
        BMessage message(STATE_CHANGED);
        message.AddBool("state", state != KeyView::DENY_KEY);
        Window()->PostMessage(&message);
        if (state == KeyView::SUBMIT_KEY) {
            committed_ = true;
        }
    } else {
        BView::MessageReceived(msg);
    }
}


KeyBindingEditor::KeyBindingEditor(BWindow* parent, BHandler* handler)
    : BWindow(
        BRect(50, 50, 340, 180),
        B_TRANSLATE("Mozc key binding editor"),
        B_TITLED_WINDOW_LOOK,
        B_MODAL_APP_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS |
        B_CLOSE_ON_ESCAPE),
      mpParent(parent),
      mpHandler(handler)
{
    mpOkButton = new BButton(B_TRANSLATE("OK"), new BMessage(OK));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));
    mpKeyView = new KeyView();

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5)
        .AddGroup(B_HORIZONTAL)
            .Add(new CStringView("label", B_TRANSLATE("Input key assignments:")))
            .Add(mpKeyView)
        .End()
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(mpOkButton)
            .Add(mpCancelButton)
        .End();

    if (mpParent) {
        CenterIn(mpParent->Frame());
    }

    Layout(true);
    mpKeyView->MakeFocus(true);
}

KeyBindingEditor::~KeyBindingEditor()
{
}

void KeyBindingEditor::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case OK:
        {
            if (mpParent || mpHandler) {
                const char* label = mpKeyView->Label();
                if (strlen(label) > 0) {
                    BMessage message(KEY_SPECIFIED);
                    message.AddString("keys", label);
                    if (mpHandler) {
                        mpHandler->MessageReceived(&message);
                    } else {
                        mpParent->PostMessage(&message);
                    }
                }
            }
            Quit();
            break;
        }
        case CANCEL:
            Quit();
            break;
        case KeyView::STATE_CHANGED:
        {
            const bool state = msg->GetBool("state", false);
            mpOkButton->SetEnabled(state);
            break;
        }
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc
