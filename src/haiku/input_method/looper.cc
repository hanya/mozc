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

#include "haiku/input_method/looper.h"

#include "haiku/input_method/bar.h"
#include "haiku/input_method/candidate_window.h"
#include "haiku/input_method/common.h"
#include "haiku/input_method/engine.h"
#include "haiku/input_method/indicator.h"
#include "haiku/input_method/method.h"
#include "haiku/input_method/settings_window.h"
//#include "base/logging.h" // todo, remove this
#include "base/file_util.h"
#include "base/system_util.h"

#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <Input.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <OS.h>
#include <private/input/InputServerTypes.h>

#if DEBUG
#include <stdio.h>
#define LOGGER "application/x-vnd.Logger"
#define LOG_COMMAND 'Logg'
#define LOG_NAME "log"
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMethod"

namespace immozc {

#define DESKBAR    "deskbar"

#define MOZC_DIRECT_INPUT 'Mcdi'

// 5 min?
#define DATA_SYNC_DURATION    (5 * 60 * 1000 * 1000)


#define SETTINGS_FILE_NAME  "settings.msg"

#define VAR_BAR_X            "bar.x"
#define VAR_BAR_Y            "bar.y"
#define VAR_BAR_ICON_SIZE    "bar.icon_size"
#define VAR_BAR_VERTICAL     "bar.vertical"
#define VAR_BAR_HIDDEN       "bar.hidden"
#define VAR_BAR_TOOLS_HIDDEN "bar.tools_hidden"
#define VAR_DIRECT_INPUT_USE "direct_input.use"
#define VAR_KANA_MAPPING     "kana.map"

struct Settings
{
    int32 bar_x;
    int32 bar_y;
    int32 bar_icon_size;
    bool bar_vertical;
    bool bar_hidden;
    bool bar_tools_hidden;
    // use Mozc direct input or not. If false, Roman IM is used as direct mode.
    //bool direct_input_use;
    int32 kana_mapping;
    bool changed;
    
    Settings()
    {
        bar_x = 50;
        bar_y = 50;
        bar_icon_size = 20;
        bar_vertical = false;
        bar_hidden = false;
        bar_tools_hidden = false;
        //direct_input_use = false;
        kana_mapping = KANA_MAPPING_JP;
        changed = false;
        
    }
    
    void SetPosition(int32 x, int32 y)
    {
        this->bar_x = x;
        this->bar_y = y;
        this->changed = true;
    }
};

void MozcLooper::_LoadSettings()
{
    if (fSettings) {
        std::string path = _GetSettingsPath();
        BMessage msg;
        BFile f(path.c_str(), B_READ_ONLY);
        if (f.InitCheck() == B_OK && msg.Unflatten(&f) == B_OK) {
            msg.FindInt32(VAR_BAR_X, &fSettings->bar_x);
            msg.FindInt32(VAR_BAR_Y, &fSettings->bar_y);
            msg.FindInt32(VAR_BAR_ICON_SIZE, &fSettings->bar_icon_size);
            msg.FindBool(VAR_BAR_VERTICAL, &fSettings->bar_vertical);
            msg.FindBool(VAR_BAR_HIDDEN, &fSettings->bar_hidden);
            msg.FindBool(VAR_BAR_TOOLS_HIDDEN, &fSettings->bar_tools_hidden);
            //msg.FindBool(VAR_DIRECT_INPUT_USE, &fSettings->direct_input_use);
            msg.FindInt32(VAR_KANA_MAPPING, &fSettings->kana_mapping);
            if (fSettings->kana_mapping < KANA_MAPPING_CUSTOM ||
                  KANA_MAPPING_END < fSettings->kana_mapping) {
                fSettings->kana_mapping = KANA_MAPPING_JP;
            }
        }
    }
}

void MozcLooper::_WriteSettings()
{
    if (fSettings && fSettings->changed) {
        std::string path = _GetSettingsPath();
        BFile f(path.c_str(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
        BMessage msg;
        if (f.InitCheck() == B_OK) {
            msg.AddInt32(VAR_BAR_X, fSettings->bar_x);
            msg.AddInt32(VAR_BAR_Y, fSettings->bar_y);
            msg.AddInt32(VAR_BAR_ICON_SIZE, fSettings->bar_icon_size);
            msg.AddBool(VAR_BAR_VERTICAL, fSettings->bar_vertical);
            msg.AddBool(VAR_BAR_HIDDEN, fSettings->bar_hidden);
            msg.AddBool(VAR_BAR_TOOLS_HIDDEN, fSettings->bar_tools_hidden);
            //msg.AddBool(VAR_DIRECT_INPUT_USE, fSettings->direct_input_use);
            msg.AddInt32(VAR_KANA_MAPPING, fSettings->kana_mapping);
            
            msg.Flatten(&f);
        }
    }
}

std::string MozcLooper::_GetSettingsPath()
{
    // /boot/home/config/settings/mozc/settings.msg
    std::string dir_path = mozc::SystemUtil::GetUserProfileDirectory();
    return mozc::FileUtil::JoinPath(dir_path, SETTINGS_FILE_NAME);
}


MozcLooper::MozcLooper(MozcMethod *method)
    : BLooper("MozcLooper"),
      fOwner(method),
      fCurrentMode(MODE_HIRAGANA),
      fHighlightedPosition(0),
      fCategory(0),
      fMozcActive(false),
      fMethodStarted(false),
      fLastSync(system_time())
{
    if (be_app) {
        if (be_app->Lock()) {
            be_app->AddHandler(this);
            be_app->Unlock();
        }
    }
#if DEBUG
    fLogger = std::unique_ptr<BMessenger>(new BMessenger(LOGGER));
    _SendLog("MozcLooper.ctor");
#endif
    fMessenger = std::unique_ptr<BMessenger>(new BMessenger(NULL, this));
    fSettings = std::unique_ptr<Settings>(new Settings());
    _LoadSettings();
    
    fSettingsWindow = NULL;

    fEngine = std::unique_ptr<MozcEngine>(new MozcEngine());
    
    fBar = std::unique_ptr<MozcBar>(new MozcBar(this, 
            fSettings->bar_vertical ? B_VERTICAL : B_HORIZONTAL, 
            static_cast<float>(fSettings->bar_icon_size)));
    fBar->MoveTo(static_cast<float>(fSettings->bar_x), 
                 static_cast<float>(fSettings->bar_y));
    if (fSettings->bar_hidden) {
        BMessage mess(IM_BAR_HIDE_PERMANENT);
        fBar->PostMessage(&mess);
    }
    if (fSettings->bar_tools_hidden) {
        BMessage mess(IM_BAR_TOOLS_ICON_STATE);
        mess.AddBool("hidden", true);
        fBar->PostMessage(&mess);
    }
    
    // There is no way to obtain location to be input now.
    //fIndicator = std::unique_ptr<Indicator>(new Indicator(this, "Mozc"));
    
    fCandidateWindow = std::unique_ptr<CandidateWindow>(
                              new CandidateWindow(this));
    // Create menu which is shown on the desckbar icon.
    fDeskbarMenu = std::unique_ptr<BMenu>(new BMenu("Menu"));
    
    BLayoutBuilder::Menu<>(fDeskbarMenu.get())
        .AddItem(new BMenuItem(B_TRANSLATE("Hiragana"),
            _CreateModeMessage(MODE_HIRAGANA)))
        .AddItem(new BMenuItem(B_TRANSLATE("Fullwidth Katakana"),
            _CreateModeMessage(MODE_FULLWIDTH_KATAKANA)))
        .AddItem(new BMenuItem(B_TRANSLATE("Halfwidth Alphabet"),
            _CreateModeMessage(MODE_HALFWIDTH_ASCII)))
        .AddItem(new BMenuItem(B_TRANSLATE("Fullwidth Alphabet"),
            _CreateModeMessage(MODE_FULLWIDTH_ASCII)))
        .AddItem(new BMenuItem(B_TRANSLATE("Halfwidth Katakana"),
            _CreateModeMessage(MODE_HALFWIDTH_KATAKANA)))
        .AddSeparator()

        .AddItem(new BMenuItem(B_TRANSLATE("Word register"),
            _CreateToolMessage(TOOL_WORD_REGISTER)))
        .AddItem(new BMenuItem(B_TRANSLATE("Dictionary"),
            _CreateToolMessage(TOOL_DICTIONARY)))
        .AddItem(new BMenuItem(B_TRANSLATE("Character pad"),
            _CreateToolMessage(TOOL_CHARACTER_PAD)))
        .AddItem(new BMenuItem(B_TRANSLATE("Handwriting"),
            _CreateToolMessage(TOOL_HAND_WRITING)))
        .AddItem(new BMenuItem(B_TRANSLATE("Configuration"),
            _CreateToolMessage(TOOL_CONFIG)))
        .AddSeparator()

        .AddItem(new BMenuItem(B_TRANSLATE("Setting..."),
            new BMessage(SettingsWindow::IM_SETTINGS_WINDOW)))
        .AddSeparator()

        .AddItem(new BMenuItem(B_TRANSLATE("Show bar"),
            new BMessage(IM_BAR_SHOW_PERMANENT)))
        .AddSeparator()
    //  .AddItem(new BMenuItem("Mozc direct input",
    //        new BMessage(MOZC_DIRECT_INPUT)))
        .AddItem(new BMenuItem(B_TRANSLATE("About Mozc"),
            _CreateToolMessage(TOOL_ABOUT)));
    
    Run();
}

MozcLooper::~MozcLooper()
{
    if (be_app) {
        if (be_app->Lock()) {
            be_app->RemoveHandler(this);
            be_app->Unlock();
        }
    }
#if DEBUG
    _SendLog("MozcLooper.dector");
#endif
}

BMessage *MozcLooper::_CreateToolMessage(Mozc_Tool tool) const
{
    BMessage *msg = new BMessage(IM_TOOL);
    msg->AddInt32(MOZC_TOOL_TOOL, tool);
    msg->AddBool(DESKBAR, true);
    return msg;
}

BMessage *MozcLooper::_CreateModeMessage(IM_Mode mode) const
{
    BMessage *msg = new BMessage(IM_MODE_CHANGE_REQUEST);
    msg->AddInt32(IM_MODE_MODE, mode);
    msg->AddBool(DESKBAR, true);
    return msg;
}

#if DEBUG
void MozcLooper::_SendLog(const char *s)
{
    if (!fLogger->IsValid()) {
        if (fLogger->SetTo(LOGGER) != B_OK) {
            return;
        }
    }
    BMessage msg(LOG_COMMAND);
    msg.AddString(LOG_NAME, s);
    fLogger->SendMessage(&msg);
}
#endif

void MozcLooper::Quit()
{
    // Quit method is called in the destructor of the input_method but 
    // this method is not called while shutdown. 
	// In general, input_server does not closed while PC is ON. 
    // So we have to store or sync settings in other place too.
    if (fSettingsWindow) {
        BMessage msg(B_QUIT_REQUESTED);
        fSettingsWindow->MessageReceived(&msg);
    }
    fSettingsWindow = NULL;
    _WriteSettings();
    _SyncDataIfRequired(true);
    if (fOwner != NULL) {
        fOwner->SetMenu(NULL, BMessenger());
    }
    if (fBar->Lock()) {
        fBar->Quit();
    }
    if (fCandidateWindow->Lock()) {
        fCandidateWindow->Quit();
    }
    BLooper::Quit();
}

status_t MozcLooper::InitCheck()
{
    // tells initial mode to the bar
    _HandleModeChange(fCurrentMode, true);
    
    return B_OK;
}

void MozcLooper::_SyncDataIfRequired(bool force)
{
    bigtime_t now = system_time();
    if (now - fLastSync > DATA_SYNC_DURATION || force) {
        fEngine->SyncData();
        fLastSync = now;
    }
}

void MozcLooper::_UpdateDeskbarMenu() const
{
    if (fOwner != NULL) {
        fOwner->SetMenu(fDeskbarMenu.get(), *fMessenger);
    }
}

void MozcLooper::EnqueueMessage(BMessage *msg)
{
    if (fOwner != NULL) {
        fOwner->EnqueueMessage(msg);
    }
}

void MozcLooper::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case B_KEY_DOWN:
        {
            _HandleKeyDown(msg);
            break;
        }
        case B_INPUT_METHOD_EVENT:
        {
            uint32 opcode = msg->GetInt32("be:opcode", 0);
            switch (opcode)
            {
                case B_INPUT_METHOD_LOCATION_REQUEST:
                    if (fMethodStarted) {
                        _HandleLocationRequest(msg);
                    }
                    break;
                case B_INPUT_METHOD_STOPPED:
                    // Canceled by some external action, 
                    // send ESC event to the engine.
                    fEngine->Revert();
                    fMethodStarted = false;
                    _HandleInputMethodStopped();
                    break;
                default:
                    break;
            }
            break;
        }
        case IM_METHOD_ACTIVATED:
        {
#if DEBUG
            _SendLog("Mozc.activated");
#endif
            _HandleMethodActivated(true);
            break;
        }
        case IM_METHOD_DEACTIVATED:
        {
#if DEBUG
            _SendLog("Mozc.deactivated");
#endif
            _HandleMethodActivated(false);
            _WriteSettings();
            _SyncDataIfRequired();
            break;
        }
        case IM_MODE_CHANGE_REQUEST:
        {
#if DEBUG
            _SendLog("Mozc.mode_change_request");
#endif
            uint32 mode = msg->GetInt32(IM_MODE_MODE, 0);
            bool deskbar = msg->GetBool(DESKBAR, false);
            if (MODE_DIRECT < mode && mode < MODE_END) {
                if (deskbar && mode == fCurrentMode) {
                    // if mode will not change, the menu should be updated here.
                    _UpdateDeskbarMenu();
                }
                _HandleModeChange((IM_Mode)mode);
            } else if (mode == MODE_DIRECT) {
                // Cancel input and deactivate input method.
                BMessage msg(B_INPUT_METHOD_EVENT);
                msg.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
                MessageReceived(&msg);
                BMessage mess(IM_METHOD_DEACTIVATED);
                MessageReceived(&mess);
            }
            break;
        }
        case IM_SELECT_CANDIDATE:
        {
			// Select a candidate and then close the candidate window.
            int32 id = 0;
            if (msg->FindInt32("id", &id) == B_OK) {
                _HandleSelectCandidate(id);
            }
            break;
        }
        case IM_HIGHLIGHT_CANDIDATE:
        {
			// Only highlight on the candidate window.
            int32 id = 0;
            if (msg->FindInt32("id", &id) == B_OK) {
                _HandleHighlightCandidate(id);
            }
            break;
        }
        case IM_TOOL:
        {
#if DEBUG
            _SendLog("Mozc.tool request");
#endif
            // If the message is came from the deskbar menu, the menu can not 
            // sent any message again, bug? So, set it again.
            bool deskbar = msg->GetBool(DESKBAR, false);
            if (deskbar) {
                _UpdateDeskbarMenu();
            }
            uint32 tool = msg->GetInt32(MOZC_TOOL_TOOL, 0);
            if (TOOL_ABOUT <= tool && tool < TOOL_END) {
                const char *s = fEngine->GetToolName((Mozc_Tool) tool);
                std::string name(s);
                if (name.length() > 0) {
                    fEngine->LaunchTool(name, "");
                }
            }
            break;
        }
        case IM_BAR_SHOW_PERMANENT:
        {
            // disable always hidden mode and show bar
            BMessage mess(IM_BAR_SHOW_PERMANENT);
            fBar->PostMessage(&mess);
            
            if (fSettings->bar_hidden) {
                fSettings->bar_hidden = false;
                fSettings->changed = true;
            }
            _UpdateDeskbarMenu();
            break;
        }
        case IM_BAR_HIDE:
        {
			// The bar was hidden by the user.
            if (fSettings->bar_hidden) {
                fSettings->bar_hidden = true;
                fSettings->changed = true;
            }
            break;
        }
        case IM_BAR_MOVED:
        {
			// The bar was moved by the user.
            BPoint pos;
            if (msg->FindPoint("pos", &pos) == B_OK) {
                if (fSettings->bar_x != static_cast<int32>(pos.x) ||
                    fSettings->bar_y != static_cast<int32>(pos.y)) {
                    fSettings->SetPosition(static_cast<int32>(pos.x),
                                           static_cast<int32>(pos.y));
                }
            }
            break;
        }
        case IM_BAR_ORIENTATION_CHANGED:
        {
			// Orientation of the bar was changed.
            bool vertical;
            if (msg->FindBool("vertical", &vertical) == B_OK) {
                if (fSettings->bar_vertical != vertical) {
                    fSettings->bar_vertical = vertical;
                    fSettings->changed = true;
                }
            }
            break;
        }
        case IM_BAR_TOOLS_ICON_STATE:
        {
			// Visibility of the tools icon on the bar was changed.
            bool hidden;
            if (msg->FindBool("hidden", &hidden) == B_OK) {
                if (fSettings->bar_tools_hidden != hidden) {
                    fSettings->bar_tools_hidden = hidden;
                    fSettings->changed = true;
                }
            }
            break;
        }
        case IM_KANA_MAPPING_MSG:
        {
            int32 mapping;
            if (msg->FindInt32(IM_KANA_MAPPING_VALUE, &mapping) == B_OK) {
                if (KANA_MAPPING_CUSTOM < mapping && mapping < KANA_MAPPING_END) {
                    fSettings->kana_mapping = mapping;
                    fSettings->changed = true;
                    fEngine->SetKanaMapping(static_cast<IM_Kana_Mapping>(mapping));
                }
            }
        }
        case SettingsWindow::IM_SETTINGS_WINDOW:
        {
            _UpdateDeskbarMenu();
            _ShowSettingsWindow();
            break;
        }
        case SettingsWindow::IM_SETTINGS_WINDOW_CLOSED:
        {
#if DEBUG
            _SendLog("Settings window closed");
#endif
            fSettingsWindow = NULL;
            break;
        }
        /*
        case Indicator::IM_INDICATOR_SHOWN:
        {
            _SendMethodStopped();
            break;
        }
        */
#if DEBUG
        case LOG_COMMAND:
        {
            const char* s;
            if (msg->FindString(LOG_NAME, &s) == B_OK) {
                _SendLog(s);
            }
            break;
        }
#endif
        default:
        {
            BLooper::MessageReceived(msg);
            break;
        }
    }
}

void MozcLooper::_ShowSettingsWindow()
{
    if (!fSettingsWindow) {
        BMessage msg(SettingsWindow::IM_SETTINGS_SET);
        msg.AddInt32(IM_KANA_MAPPING_VALUE, static_cast<int32>(fSettings->kana_mapping));

        fSettingsWindow = new SettingsWindow(this);
        fSettingsWindow->MessageReceived(&msg);
        fSettingsWindow->Show();
    }
}

void MozcLooper::_SwitchToDefaultMethod()
{
    BMessage msg(IS_SET_METHOD);
    msg.AddInt32("cookie", 1); // see InputServerMethod.cpp
    be_app->MessageReceived(&msg);
}

void MozcLooper::_ShowCandidateWindow()
{
    BMessage msg(IM_CANDIDATE_WINDOW_SHOW);
    fCandidateWindow->PostMessage(&msg);
}

void MozcLooper::_HideCandidateWindow()
{
    BMessage msg(IM_CANDIDATE_WINDOW_HIDE);
    fCandidateWindow->PostMessage(&msg);
}

void MozcLooper::_HandleInputMethodStopped()
{
    fHighlightedPosition = 0;
    // close candidate window
    _HideCandidateWindow();
}

void MozcLooper::_HandleMethodActivated(bool active)
{
    if (active) {
        // Just make Mozc active
        std::unique_ptr<mozc::commands::Output> output = 
                std::unique_ptr<mozc::commands::Output>(
                        new mozc::commands::Output());
        if (fEngine->TurnOn(output.get())) {
            _HandleStatus(*output);
            fEngine->UpdatePreeditMethod();
        }
        _ModeChanged();
        
        // show the bar
        BMessage message(IM_BAR_SHOW);
        fBar->PostMessage(&message);
        
        // Show indicator
        /*
        _SendMethodStarted();
        BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
        msg->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
        msg->AddMessenger("be:reply_to", BMessenger(NULL, fIndicator.get()));
        EnqueueMessage(msg);
        */
    } else {
        fMethodStarted = false;
        // maybe deactivated already by key down, so check it
        if (fMozcActive) {
            // Deactivate Mozc
            std::unique_ptr<mozc::commands::Output> output = 
                    std::unique_ptr<mozc::commands::Output>(
                            new mozc::commands::Output());
            if (fEngine->TurnOff(output.get())) {
                _HandleStatus(*output);
            }
        }
        _ModeChanged();
        
        if (fOwner != NULL) {
            fOwner->SetMenu(NULL, *fMessenger);
        }
        // todo, option to make the bar always visible and show state like direct
        // hide bar if not shown while direct input by the configuration
        
        // Hide bar if the bar is not in show always mode.
        BMessage message(IM_BAR_HIDE);
        fBar->PostMessage(&message);
        /*
        BMessage mess(Indicator::IM_INDICATOR_HIDE);
        fIndicator->PostMessage(&mess);
        */
    }
}

void MozcLooper::_HandleModeChange(IM_Mode mode, bool forced)
{
    if (forced || mode != fCurrentMode) {
        // Send input mode to the engine and change internal mode if changed.
        mozc::commands::Output output;
        if (fEngine->SwitchInputMode(
                fEngine->ModeToCompositionMode(mode), &output)) {
            if (!_HandleStatus(output)) {
                return;
            }
        }
        _ModeChanged();
    }
}

void MozcLooper::_ModeChanged()
{
    // send new mode to the bar
    _SendModeToBar();
#if DEBUG
    _SendLog("Mozc.mode changed and sent to bar");
#endif
    // remove selected and set to new one
    BMenuItem *item = NULL;
    item = fDeskbarMenu->FindMarked();
    if (item != NULL) {
        item->SetMarked(false);
    }
    for (int i = 0; i < fDeskbarMenu->CountItems(); ++i) {
        item = fDeskbarMenu->ItemAt(i);
        if (item != NULL) {
            BMessage *m = item->Message();
            if (m != NULL && m->what == IM_MODE_CHANGE_REQUEST) {
                int32 mode;
                if (m->FindInt32(IM_MODE_MODE, &mode) == B_OK) {
                    if (mode == fCurrentMode) {
                        item->SetMarked(true);
                        break;
                    }
                }
            }
        }
    }
    _UpdateDeskbarMenu();
    // set repricant icon
    // Repricant icon is not redrawn until method change. Not useful now.
    /*
    const uchar *icon = _GetModeIcon(fCurrentMode);
    if (icon != NULL) {
        fOwner->SetIcon(icon);
    }
    */
}

void MozcLooper::_SendModeToBar()
{
    BMessage message(IM_MODE_CHANGED);
    message.AddInt32(IM_MODE_MODE, static_cast<int32>(fCurrentMode));
    message.AddBool(IM_ACTIVE, fMozcActive);
    fBar->PostMessage(&message);
}

void MozcLooper::_HandleKeyDown(BMessage *msg)
{
    // key: int32, modifires: uint32, when: int64, byte: int8, raw_char: int32
    // Mozc uses UCS4 code point as its input, use it for special key to know
    int32 key = msg->GetInt32("key", 0); // raw keycode
    uint8 byte = (uint8)msg->GetInt8("byte", 0); // ascii code
    uint32 modifiers = (uint32)msg->GetInt32("modifiers", 0);
    /*
    if ((modifiers & B_CONTROL_KEY) != 0) {
        // ignore shortcut?
        EnqueueMessage(DetachCurrentMessage());
        return;
    }
    */
    // Eisu and CapsLock are not passed to input_method, so we can not use them.
#if DEBUG
    char s[64];
    sprintf(s, "keydown, key: 0x%x, mod: 0x%x, byte: 0x%x", 
            key, modifiers, (char)byte);
    _SendLog((const char*)s);
#endif
    std::unique_ptr<mozc::commands::Output> output = 
        std::unique_ptr<mozc::commands::Output>(new mozc::commands::Output());
    if (fEngine->SendKey(byte, key, modifiers, output.get())) {
        // todo, check error_code here?
        if (output->has_consumed()) {
            if (output->consumed()) {
                _HandleOutput(std::move(output));
                return;
            }
        }
#if DEBUG
        _SendLog("Mozc.keydown not consumed, will be sent to input_method");
#endif
#if DEBUG
        if (output->has_key()) {
            char v[64];
            sprintf(v, "Key remained: 0x%x", output->key().key_code());
            if (output->key().modifier_keys_size() > 0) {
                _SendLog("Modifier remained");
            }
        }
#endif
        // Remove custom keys on direct mode. They input invalid character.
        if (fCurrentMode == MODE_DIRECT && 
            (byte == K_MUHENKAN || byte == K_HENKAN)) {
            return;
        }
        // If consumed is not specified, treat as not consumed.
        // Send keydown message to the input server
        EnqueueMessage(DetachCurrentMessage());
    } else {
        // Failed to send key to the server, it might be the server is not 
        // started. Check the path to the server.
#if DEBUG
        _SendLog("Mozc.keydown sendkey failed");
#endif
        // no keys can be parsed, such as B_CONTROL_KEY + some key
        EnqueueMessage(DetachCurrentMessage());
        //mozc::Logging::CloseLogStream();
    }
}

void MozcLooper::_HandleOutput(std::unique_ptr<mozc::commands::Output> output)
{
    // track mode change and update mode icon on the bar
    if (output->has_status()) {
        if (_HandleStatus(*output)) {
            _ModeChanged();
#if DEBUG
            _SendLog("Mozc.mode changed by keydown");
#endif
        }
    }
    // If we have both result and preedit at the same time, 
    // confirm the current result first and then restart the preedit.
    if (output->has_result()) {
        _HandleResult(*output);
    }
    bool show_candidate_window = false;
    if (output->has_preedit()) {
        bool preedit_activated = _HandlePreedit(*output);
        // No preedit, no candidates.
        if (output->has_candidates()) {
            _HandleCandidates(*output, preedit_activated);
            show_candidate_window = true;
        }
    } else if (fMethodStarted) {
        // The preedit is no longer required because the preedit is empty.
        // We have to make caluse empty.
        BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
        msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);
        msg->AddString("be:string", "");
        EnqueueMessage(msg);
        
        _SendMethodStopped();
        _HandleInputMethodStopped();
    }
    // key, event might be remained if consumed=false
    // preedit_method, change of preedit_method?
    // all_candidate_words
    // deletion_range, only with context?
    if (output->has_launch_tool_mode()) {
        _HandleLaunchToolMode(output->launch_tool_mode());
    }
    
    if (show_candidate_window) {
        fCandidateWindow->SetData(std::move(output));
#if DEBUG
        _SendLog("Mozc.data_sent");
#endif
        BMessage msg(CANDIDATE_WINDOW_UPDATE);
        fCandidateWindow->PostMessage(&msg);
    } else {
        // Hide candidate window if no candidates.
        _HideCandidateWindow();
    }
}

bool MozcLooper::_HandleStatus(const mozc::commands::Output &output)
{
    bool status = false;
    if (output.has_status()) {
        if (output.status().has_activated()) {
#if DEBUG
            char s[64];
            sprintf(s, "status.activated: %d", (int)output.status().activated());
            _SendLog((const char*)s);
            if (output.status().has_mode() && output.status().has_comeback_mode()) {
                char s2[64];
                sprintf(s2, "mode: %d, comeback_mode: %d", 
                    output.status().mode(), output.status().comeback_mode());
                _SendLog((const char*)s2);
            }
#endif
            fMozcActive = output.status().activated();
            if (!fMozcActive) {
                _SwitchToDefaultMethod();
                status = true;
            }
        }
        if (output.status().has_mode()) {
            IM_Mode new_mode = fEngine->CompositionModeToMode(
                                            output.status().mode());
            if (new_mode != fCurrentMode) {
                fCurrentMode = new_mode;
                status = true;
            }
        }
    }
    return status;
}

void MozcLooper::_HandleResult(const mozc::commands::Output &output)
{
    if (!output.has_result() || 
         output.result().type() == mozc::commands::Result::NONE) {
        return;
    }
    // If input method is not started yet but the result is generated. 
    // We have to start input method to make the string inserted correctly.
    if (!fMethodStarted) {
        _SendMethodStarted();
    }
    BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
    msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);
    msg->AddBool("be:confirmed", true);
    msg->AddString("be:string", output.result().value().c_str());
    // How about cursor_offset?
#if DEBUG
    _SendLog("result will be sent");
    if (output.result().value().length() == 3) {
        char s[64];
        sprintf(s, ":%s", output.result().value().c_str());
        _SendLog((const char*)s);
    } else {
        _SendLog(output.result().value().c_str());
    }
#endif
    EnqueueMessage(msg);
    
    // Preedit should be closed.
    _SendMethodStopped();
    _HandleInputMethodStopped();
}

bool MozcLooper::_HandlePreedit(const mozc::commands::Output &output)
{
    bool preedit_activated = false;
    if (!output.has_preedit()) {
        return false;
    }
    if (!fMethodStarted) {
        // Send message to prepare preedit
        _SendMethodStarted();
        preedit_activated = true;
#if DEBUG
        _SendLog("preedit activated");
#endif
    }
    // The candidate window should know character index of 
    // the highlighted segment.
    uint32 highlighted_index = 0;
    
    BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
    msg->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);
    
    std::string text;
    
    uint32 byte_pos = 0;
    uint32 pos = 0;
    for (int i = 0; i < output.preedit().segment_size(); ++i) {
        const mozc::commands::Preedit_Segment &segment = 
                            output.preedit().segment(i);
        text.append(segment.value());
        msg->AddInt32("be:clause_start", pos);
        pos += segment.value_length();
        msg->AddInt32("be:clause_end", pos);
        byte_pos += segment.value().length();
        if (segment.annotation() == mozc::commands::Preedit::Segment::HIGHLIGHT) {
            // byte position
            msg->AddInt32("be:selection", 
                    byte_pos - segment.value().length()); // start
            msg->AddInt32("be:selection", byte_pos); // end
            // character position
            highlighted_index = pos - segment.value_length();
#if DEBUG
            char s[64];
            sprintf(s, "preedit selected: %s, start: %d, end: %d", 
                        segment.value().c_str(), pos - segment.value_length(), pos);
            _SendLog((const char*)s);
#endif
        }
    }
    // How about output.cursor(), there is no way to set carret position now.
    // Cursor should be placed at the start of selected clause?
    msg->AddString("be:string", text.c_str());
#if DEBUG
    _SendLog(text.c_str());
#endif
    EnqueueMessage(msg);
    
    // send highlighted index to the candidate window
    BMessage mess(CANDIDATE_WINDOW_CURSOR_INDEX);
    mess.AddInt32(CANDIDATE_WINDOW_CURSOR_NAME, 
                  static_cast<int32>(highlighted_index));
    fCandidateWindow->PostMessage(&mess);
    
    return preedit_activated;
}

void MozcLooper::_HandleCandidates(const mozc::commands::Output &output, 
                                   bool newly_activated)
{
    // update candidate window and show result
    if (!output.has_candidates()) {
        return;
    }
    // todo, if category is changed, tell candidate window to set position
    // should we do here? or view?
    if (output.candidates().has_category()) {
        if (fCategory != output.candidates().category()) {
            // Reposition candidate window if category has been changed.
            fCategory = output.candidates().category();
            newly_activated = true;
#if DEBUG
            char s[64];
            sprintf(s, "Category changed: %d", fCategory);
            _SendLog((const char*)s);
#endif
        }
    }
    // If the candidate window is requested first time or position to 
	// show the window is changed.
    uint32 position = output.preedit().highlighted_position();
    if (newly_activated || position != fHighlightedPosition) {
        fHighlightedPosition = position;
        // To obtain the position where the window should be shown. 
        _SendMethodLocationRequest();
    }
}

void MozcLooper::_HandleLocationRequest(BMessage *msg)
{
    // be:opcode, be:location_reply: BPoint, be:height_reply: float
#if DEBUG
            _SendLog("Mozc.location_request");
#endif
    // Multiple locations are contained for each characters in the preedit.
    // Send it to the candidate window.
    fCandidateWindow->PostMessage(msg);
    
    // Show candidate window now.
    _ShowCandidateWindow();
}

void MozcLooper::_HandleLaunchToolMode(mozc::commands::Output::ToolMode mode)
{
    switch (mode)
    {
        case mozc::commands::Output::CONFIG_DIALOG:
            fEngine->LaunchTool("config_dialog", "");
            break;
        case mozc::commands::Output::DICTIONARY_TOOL:
            fEngine->LaunchTool("dictionary_tool", "");
            break;
        case mozc::commands::Output::WORD_REGISTER_DIALOG:
            fEngine->LaunchTool("word_register_dialog", "");
            break;
        default:
            break;
    }
}

void MozcLooper::_HandleSelectCandidate(int32 id)
{
#if DEBUG
    _SendLog("Mozc.select_candidate");
#endif
    std::unique_ptr<mozc::commands::Output> output = 
        std::unique_ptr<mozc::commands::Output>(new mozc::commands::Output());
    if (fEngine->SelectCandidate(id, output.get())) {
        if (output->has_consumed() && output->consumed()) {
#if DEBUG
            char s[64];
            sprintf(s, "Mozc.has_candidates: %d, has_result: %d", 
                        (int)output->has_candidates(),
                        (int)output->has_result());
            _SendLog((const char*)s);
#endif
            _HandleOutput(std::move(output));
            return;
        }
    }
}

void MozcLooper::_HandleHighlightCandidate(int32 id)
{
    std::unique_ptr<mozc::commands::Output> output = 
        std::unique_ptr<mozc::commands::Output>(new mozc::commands::Output());
    if (fEngine->HighlightCandidate(id, output.get())) {
        if (output->has_consumed() && output->consumed()) {
            _HandleOutput(std::move(output));
            return;
        }
    }
}

void MozcLooper::_SendMethodLocationRequest()
{
    BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
    msg->AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
    EnqueueMessage(msg);
}

void MozcLooper::_SendMethodStarted()
{
    BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
    msg->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
    msg->AddMessenger("be:reply_to", BMessenger(NULL, this));
    EnqueueMessage(msg);
    fMethodStarted = true;
}

void MozcLooper::_SendMethodStopped()
{
    BMessage *msg = new BMessage(B_INPUT_METHOD_EVENT);
    msg->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
    EnqueueMessage(msg);
    fMethodStarted = false;
}
/*
const uchar *MozcLooper::_GetModeIcon(IM_Mode mode) const
{
    const uchar *icon = nullptr;
    switch (mode)
    {
        case MODE_DIRECT:
            icon = kUiDirectIconData;
            break;
        case MODE_HIRAGANA:
            icon = kUiHiraganaIconData;
            break;
        case MODE_FULLWIDTH_KATAKANA:
            icon = kUiFullwidthKatakanaIconData;
            break;
        case MODE_HALFWIDTH_ASCII:
            icon = kUiHalfwidthAlphaIconData;
            break;
        case MODE_FULLWIDTH_ASCII:
            icon = kUiFullwidthAlphaIconData;
            break;
        case MODE_HALFWIDTH_KATAKANA:
            icon = kUiHalfwidthKatakanaIconData;
            break;
        default:
            break;
    }
    return icon;
}
*/
} // namespace immozc

