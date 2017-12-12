// Copyright 2010-2016, Google Inc.
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


#include "haiku/input_method/bar.h"
#include "haiku/input_method/common.h"
#include "haiku/input_method/looper.h"
#include "haiku/input_method/vector_icons.h"

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Window.h>
#include <private/interface/WindowPrivate.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMethod"

namespace immozc {

static const uint32 BAR_MODE_MENU = 'MnMd';
static const uint32 BAR_TOOLS_MENU = 'MnTl';
static const uint32 BAR_CONTEXT_MENU = 'MnCx';
static const uint32 BAR_BAR_MENU = 'Mnbr';


class BarButton : public BButton
{
public:
    BarButton(const char *name, const char *tooltip, 
                BMessage *msg, MozcBar *target);
    virtual ~BarButton();
    virtual void MouseDown(BPoint where);
    
private:
    MozcBar *fTarget;
};

BarButton::BarButton(const char *name, const char *tooltip, 
                    BMessage *msg, MozcBar *target)
    : BButton(name, NULL, msg),
      fTarget(target)
{
    SetFlat(true);
    SetToolTip(tooltip);
}

BarButton::~BarButton()
{
}

void BarButton::MouseDown(BPoint where)
{
    uint32 buttons;
    GetMouse(&where, &buttons, true);
    // track right click for requesting context menu
    if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        if (fTarget != NULL) {
            BMessage msg(BAR_CONTEXT_MENU);
            fTarget->MessageReceived(&msg);
        }
        return;
    }
    BButton::MouseDown(where);
}


MozcBar::MozcBar(MozcLooper *looper, orientation ort, float size)
    : BWindow(
        BRect(50, 50, 10, 10),
        "",
        kLeftTitledWindowLook,
        B_FLOATING_ALL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS |
        B_NOT_CLOSABLE | B_NOT_ZOOMABLE |
        B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | 
        B_AVOID_FOCUS |
        B_WILL_ACCEPT_FIRST_CLICK),
     fLooper(looper),
     fIconSize(size),
     fOrientation(ort),
     fHidden(false),
     fActive(false)
{
    _Init();
    SetLook(fOrientation == B_HORIZONTAL ? 
            kLeftTitledWindowLook : B_FLOATING_WINDOW_LOOK);
    
    fModeButton = new BarButton("mode", B_TRANSLATE("Mode"), 
                        new BMessage(BAR_MODE_MENU), this);
    fModeButton->SetIcon(fDirectIcon.get());
    
    fToolsButton = new BarButton("tools", B_TRANSLATE("Tools"), 
                        new BMessage(BAR_TOOLS_MENU), this);
    fToolsButton->SetIcon(fToolsIcon.get());
    
    BLayoutBuilder::Group<>(this, fOrientation, 0.)
        .SetInsets(0.)
        .Add(fModeButton)
        .Add(fToolsButton);
    
    fModeMenu = std::unique_ptr<BPopUpMenu>(new BPopUpMenu("modeMenu", false));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Direct"), 
                    new BMessage(MODE_DIRECT)));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Hiragana"), 
                    new BMessage(MODE_HIRAGANA)));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Fullwidth Katakana"), 
                    new BMessage(MODE_FULLWIDTH_KATAKANA)));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Halfwidth Alphabet"), 
                    new BMessage(MODE_HALFWIDTH_ASCII)));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Fullwidth Alphabet"), 
                    new BMessage(MODE_FULLWIDTH_ASCII)));
    fModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Halfwidth Katakana"), 
                    new BMessage(MODE_HALFWIDTH_KATAKANA)));
    
    fModeMenu->ItemAt(1)->SetMarked(true);
    fModeMenu->SetRadioMode(false);
    
    fToolsMenu = std::unique_ptr<BPopUpMenu>(
            new BPopUpMenu("toolsMenu", false));
    fToolsMenu->SetRadioMode(false);
    fToolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Word register"), 
                _CreateToolMessage(TOOL_WORD_REGISTER)));
    fToolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Dictionary"), 
                _CreateToolMessage(TOOL_DICTIONARY)));
    fToolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Character pad"), 
                _CreateToolMessage(TOOL_CHARACTER_PAD)));
    fToolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Handwriting"), 
                _CreateToolMessage(TOOL_HAND_WRITING)));
    fToolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Configuration"), 
                _CreateToolMessage(TOOL_CONFIG)));
    
    fContextMenu = std::unique_ptr<BPopUpMenu>(
            new BPopUpMenu("contextMenu", false));
    fContextMenu->SetRadioMode(false);
    
    BPopUpMenu *toolsMenu = new BPopUpMenu(B_TRANSLATE("Tools"), false);
    toolsMenu->SetRadioMode(false);
    toolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Word register"), 
                _CreateToolMessage(TOOL_WORD_REGISTER)));
    toolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Dictionary"), 
                _CreateToolMessage(TOOL_DICTIONARY)));
    toolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Character pad"), 
                _CreateToolMessage(TOOL_CHARACTER_PAD)));
    toolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Handwriting"), 
                _CreateToolMessage(TOOL_HAND_WRITING)));
    toolsMenu->AddItem(new BMenuItem(B_TRANSLATE("Configuration"), 
                _CreateToolMessage(TOOL_CONFIG)));
    fContextMenu->AddItem(toolsMenu);
    
    BPopUpMenu *barMenu = new BPopUpMenu(B_TRANSLATE("Bar"), false);
    barMenu->SetRadioMode(false);
    barMenu->AddItem(new BMenuItem(B_TRANSLATE("Horizontal"), 
            new BMessage(IM_BAR_HORIZONTAL)));
    barMenu->AddItem(new BMenuItem(B_TRANSLATE("Vertical"), 
            new BMessage(IM_BAR_VERTICAL)));
    barMenu->AddSeparatorItem();
    barMenu->AddItem(new BMenuItem(B_TRANSLATE("Hide bar"), 
            new BMessage(IM_BAR_HIDE_PERMANENT)));
    fContextMenu->AddItem(new BMenuItem(barMenu, 
            new BMessage(BAR_BAR_MENU)));
    fContextMenu->AddSeparatorItem();
    fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Show/hide tools"),
            new BMessage(IM_BAR_TOOLS_ICON_STATE)));
    //fContextMenu->AddItem(new BMenuItem("Reload", new BMessage(IM_RELOAD)));
    fContextMenu->AddSeparatorItem();
    fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("About Mozc"), 
                _CreateToolMessage(TOOL_ABOUT)));
    
    Run(); // start
    // Allows to obtain width and height of these menus.
    fModeMenu->DoLayout();
    fToolsMenu->DoLayout();
    fContextMenu->DoLayout();
    
    _UpdateBarMenu();
}

MozcBar::~MozcBar()
{
}

BMessage *MozcBar::_CreateToolMessage(Mozc_Tool tool) const
{
    BMessage *msg = new BMessage(IM_TOOL);
    msg->AddInt32(MOZC_TOOL_TOOL, tool);
    return msg;
}

void MozcBar::_Init()
{
    BRect rect(0, 0, fIconSize - 1, fIconSize - 1);
    fDirectIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kDirectIcon, sizeof(kDirectIcon), 
        fDirectIcon.get());
    //fDirectIcon.reset(bitmap);
    fHiraganaIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kHiraganaIcon, sizeof(kHiraganaIcon), 
        fHiraganaIcon.get());
    fFullwidthKatakanaIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kFullwidthKatakanaIcon, sizeof(kFullwidthKatakanaIcon), 
        fFullwidthKatakanaIcon.get());
    fHalfwidthAsciiIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kHalfwidthAsciiIcon, sizeof(kHalfwidthAsciiIcon), 
        fHalfwidthAsciiIcon.get());
    fFullwidthAsciiIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kFullwidthAsciiIcon, sizeof(kFullwidthAsciiIcon), 
        fFullwidthAsciiIcon.get());
    fHalfwidthKatakanaIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kHalfwidthKatakanaIcon, sizeof(kHalfwidthKatakanaIcon), 
        fHalfwidthKatakanaIcon.get());
    fToolsIcon = std::unique_ptr<BBitmap>(new BBitmap(rect, 0, B_RGBA32));
    BIconUtils::GetVectorIcon(
        kToolsIcon, sizeof(kToolsIcon), 
        fToolsIcon.get());
}

void MozcBar::_ChangeOrientation(orientation ort)
{
    if (ort != fOrientation) {
        fOrientation = ort;
        SetLook(fOrientation == B_HORIZONTAL ? 
                kLeftTitledWindowLook : B_FLOATING_WINDOW_LOOK);
        BGroupLayout *layout = dynamic_cast<BGroupLayout *>(GetLayout());
        if (layout != NULL) {
            layout->SetOrientation(fOrientation);
        }
        _UpdateBarMenu();
        
        // store bar orientation
        BMessage msg(IM_BAR_ORIENTATION_CHANGED);
        msg.AddBool("vertical", fOrientation == B_VERTICAL);
        fLooper->PostMessage(&msg);
    }
}

void MozcBar::_UpdateBarMenu()
{
    BMenuItem *entry = fContextMenu->FindItem(BAR_BAR_MENU);
    if (entry != NULL) {
        BMenu *subMenu = entry->Submenu();
        if (subMenu != NULL) {
            BMenuItem *item = subMenu->FindMarked();
            if (item != NULL) {
                item->SetMarked(false);
            }
            item = subMenu->FindItem(fOrientation == B_HORIZONTAL ? 
                                    IM_BAR_HORIZONTAL : IM_BAR_VERTICAL);
            if (item != NULL) {
                item->SetMarked(true);
            }
        }
    }
}

void MozcBar::_ModeChanged(IM_Mode mode)
{
    // Update icon shown in the mode button.
    BBitmap *icon = _GetModeIcon(fActive, mode);
    if (icon != NULL) {
        fModeButton->SetIcon(icon);
    }
    
    // Update selected item in the mode menu.
    BMenuItem *item = NULL;
    while (true) {
        item = fModeMenu->FindMarked();
        if (item != NULL) {
            item->SetMarked(false);
        } else {
            break;
        }
    }
    item = fModeMenu->FindItem((uint32)mode);
    if (item != NULL) {
        item->SetMarked(true);
    }
}

BBitmap *MozcBar::_GetModeIcon(bool active, IM_Mode mode)
{
    if (active) {
        BBitmap *icon = NULL;
        switch (mode)
        {
            // Direct is not a mode but for icon to indicate current input mode
            // while the bar is not hidden in direct mode.
            case MODE_DIRECT:
                icon = fDirectIcon.get();
                break;
            case MODE_HIRAGANA:
                icon = fHiraganaIcon.get();
                break;
            case MODE_FULLWIDTH_KATAKANA:
                icon = fFullwidthKatakanaIcon.get();
                break;
            case MODE_HALFWIDTH_ASCII:
                icon = fHalfwidthAsciiIcon.get();
                break;
            case MODE_FULLWIDTH_ASCII:
                icon = fFullwidthAsciiIcon.get();
                break;
            case MODE_HALFWIDTH_KATAKANA:
                icon = fHalfwidthKatakanaIcon.get();
                break;
            default:
                break;
        }
        return icon;
    } else {
        return fDirectIcon.get();
    }
}

void MozcBar::FrameMoved(BPoint pos)
{
    BMessage msg(IM_BAR_MOVED);
    msg.AddPoint("pos", pos);
    fLooper->PostMessage(&msg);
    
    BWindow::FrameMoved(pos);
}

bool MozcBar::_GetMenuPosition(const char *name, 
                                BPopUpMenu *menu, BPoint *where)
{
    bool status = false;
    BView *view = FindView(name);
    if (view != nullptr && menu != nullptr) {
        status = true;
        BRect pos = view->Frame();
        ConvertToScreen(&pos);
        BRect screen = BScreen(this).Frame();
        if (fOrientation == B_HORIZONTAL) {
            if (pos.top < (screen.Height() * 0.5)) {
                // show menu below
                *where = pos.LeftBottom();
            } else {
                BPoint p = pos.LeftTop();
                p.y -= menu->Bounds().Height();
                *where = p;
            }
        } else {
            if (pos.left < (screen.Width() * 0.5)) {
                // show menu right
                *where = pos.RightTop();
            } else {
                BPoint p = pos.LeftTop();
                p.x -= menu->Bounds().Width();
                *where = p;
            }
        }
    }
    return status;
}

void MozcBar::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case IM_MODE_CHANGED:
        {
            // sent by looper to update mode icon on this bar
            //int32 mode;
            //if (msg->FindInt32(IM_MODE_MODE, &mode) == B_OK) {
            int32 mode = msg->GetInt32(IM_MODE_MODE, 0);
            fActive = msg->GetBool(IM_ACTIVE, true);
            _ModeChanged((IM_Mode)mode);
            //}
            break;
        }
        case BAR_MODE_MENU:
        {
            BPoint where;
            if (_GetMenuPosition("mode", fModeMenu.get(), &where)) {
                BMenuItem *item = fModeMenu->Go(where, true);
                if (item != NULL) {
                    BMessage *m = item->Message();
                    if (m != NULL) {
                        // looper sends IM_MODE_CHANGED later to tell new mode.
                        BMessage message(IM_MODE_CHANGE_REQUEST);
                        message.AddInt32(IM_MODE_MODE, m->what);
                        fLooper->PostMessage(&message);
                    }
                }
            }
            break;
        }
        case BAR_TOOLS_MENU:
        {
            BPoint where;
            if (_GetMenuPosition("tools", fToolsMenu.get(), &where)) {
                BMenuItem *item = fToolsMenu->Go(where, true);
                if (item != NULL) {
                    BMessage *m = item->Message();
                    if (m != NULL && m->what == IM_TOOL) {
                        fLooper->PostMessage(m);
                    }
                }
            }
            break;
        }
        case BAR_CONTEXT_MENU:
        {
            BPoint where;
            if (_GetMenuPosition("mode", fContextMenu.get(), &where)) {
                BMenuItem *item = fContextMenu->Go(where, true);
                if (item != NULL) {
                    BMessage *m = item->Message();
                    if (m != NULL && m->what != BAR_CONTEXT_MENU) {
                        if (m->what == IM_TOOL) {
                            fLooper->PostMessage(m);
                        } else {
                            MessageReceived(m);
                        }
                    }
                }
            }
            break;
        }
        case IM_BAR_HORIZONTAL:
        {
            _ChangeOrientation(B_HORIZONTAL);
            break;
        }
        case IM_BAR_VERTICAL:
        {
            _ChangeOrientation(B_VERTICAL);
            break;
        }
        case IM_BAR_SHOW:
        {
            if (!fHidden && IsHidden()) {
                SetWorkspaces(B_CURRENT_WORKSPACE);
                Show();
            }
            break;
        }
        case IM_BAR_HIDE:
        {
            if (!IsHidden()) {
                Hide();
            }
            break;
        }
        case IM_BAR_SHOW_PERMANENT:
        {
            fHidden = false;
            BMessage mess(IM_BAR_SHOW);
            MessageReceived(&mess);
            break;
        }
        case IM_BAR_HIDE_PERMANENT:
        {
            fHidden = true;
            if (!IsHidden()) {
                Hide();
            }
            fLooper->PostMessage(msg); // hide always
            break;
        }
        case IM_BAR_TOOLS_ICON_STATE:
        {
            bool hidden;
            bool swch = msg->FindBool("hidden", &hidden) != B_OK;
            
            BLayout *layout = GetLayout();
            if (layout != NULL) {
                for (int i = layout->CountItems()-1; i >= 0; --i) {
                    BLayoutItem *item = layout->ItemAt(i);
                    if (item != NULL) {
                        BView *view = item->View();
                        if (view != NULL) {
                            BButton *button = dynamic_cast<BButton *>(view);
                            if (button != NULL) {
                                BMessage *m = button->Message();
                                if (m != NULL && m->what == BAR_TOOLS_MENU) {
                                    bool state = swch ? view->IsHidden() : !hidden;
                                    if (state) {
                                        view->Show();
                                    } else {
                                        view->Hide();
                                    }
                                    if (swch) {
                                        BMessage mess(IM_BAR_TOOLS_ICON_STATE);
                                        mess.AddBool("hidden", view->IsHidden());
                                        fLooper->PostMessage(&mess);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case IM_RELOAD:
        {
            fLooper->PostMessage(msg);
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

} // namespace immozc

