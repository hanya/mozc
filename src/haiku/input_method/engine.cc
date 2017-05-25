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

#include "haiku/input_method/engine.h"

#include "base/init_mozc.h"
#include "base/protobuf/descriptor.h"
#include "base/protobuf/message.h"
#include "base/util.h"
#include "base/version.h"
#include "config/config_handler.h"
#include "protocol/config.pb.h"

#include <InterfaceDefs.h>
#include <Language.h>
#include <Locale.h>

#include <stdlib.h>

using mozc::commands::KeyEvent;

namespace immozc {

MozcEngine::MozcEngine()
{
    // init mozc
    int argc = 1;
    char argv0[] = "mozc_input_method"; // used for file name by logging
    char *_argv[] = {argv0, NULL};
    char **argv = _argv;
    mozc::InitMozc(argv[0], &argc, &argv, false);
    
    fClient = std::unique_ptr<mozc::client::ClientInterface>(
                    mozc::client::ClientFactory::NewClient());//);
    // if text can be deleted around preedit, set mozc::commands::capability.
}

MozcEngine::~MozcEngine()
{
}

bool MozcEngine::LaunchTool(const string &mode, const string &extra_arg)
{
    {
        // Set LANGUAGE environmental variable to make Qt GUI is localized.
        BLocale *locale = new BLocale();
        BLanguage lang;
        if (locale->GetLanguage(&lang) == B_OK) {
            setenv("LANGUAGE", lang.Code(), 1);
        }
        delete locale;
    }
    return fClient->LaunchTool(mode, extra_arg);
}

bool MozcEngine::OpenBrowser(const string &url)
{
    return fClient->OpenBrowser(url);
}

bool MozcEngine::SyncData()
{
    return fClient->SyncData();
}

bool MozcEngine::Shutdown()
{
    return fClient->Shutdown();
}

bool MozcEngine::Reload()
{
    return fClient->Reload();
}

bool MozcEngine::TurnOn(mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::TURN_ON_IME);
    
    return SendCommand(command, output);
}

bool MozcEngine::TurnOff(mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::TURN_OFF_IME);
    
    return SendCommand(command, output);
}

bool MozcEngine::Revert()
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::REVERT);
    mozc::commands::Output output;
    
    return SendCommand(command, &output);
}

bool MozcEngine::SelectCandidate(int32 id, mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::SELECT_CANDIDATE);
    command.set_id(id);
    
    return SendCommand(command, output);
}

bool MozcEngine::HighlightCandidate(int32 id, mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::HIGHLIGHT_CANDIDATE);
    command.set_id(id);
    
    return SendCommand(command, output);
}

bool MozcEngine::SwitchInputMode(mozc::commands::CompositionMode mode, 
                                 mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::SWITCH_INPUT_MODE);
    command.set_composition_mode(mode);
    
    return SendCommand(command, output);
}

bool MozcEngine::ConvertPrevPage(mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::CONVERT_PREV_PAGE);
    
    return SendCommand(command, output);
}

bool MozcEngine::ConvertNextPage(mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::CONVERT_NEXT_PAGE);
    
    return SendCommand(command, output);
}

bool MozcEngine::GetStatus(mozc::commands::Output *output)
{
    mozc::commands::SessionCommand command;
    command.set_type(mozc::commands::SessionCommand::GET_STATUS);
    
    return SendCommand(command, output);
}

bool MozcEngine::SendCommand(const mozc::commands::SessionCommand &command, 
                             mozc::commands::Output *output)
{
    return fClient->SendCommand(command, output);
}

bool MozcEngine::SendKey(uint8 byte, int32 key, uint32 mod, 
                         mozc::commands::Output *output)
{
    mozc::commands::KeyEvent key_event;
    bool key_added = _AddKey(byte, key, &mod, &key_event);
    _AddModifiers(mod, &key_event);
    if (!key_added && key_event.modifier_keys_size() == 0) {
        return false;
    }
    return fClient->SendKey(key_event, output);
}

static const uint8 KEYPAD[] = {
                       // 0x23-0x25
                       KeyEvent::DIVIDE, KeyEvent::MULTIPLY, KeyEvent::SUBTRACT, 
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,
    // 0x37-0x3a
    KeyEvent::NUMPAD7, KeyEvent::NUMPAD8, KeyEvent::NUMPAD9, KeyEvent::ADD,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    // 0x48-0x4a
    KeyEvent::NUMPAD4, KeyEvent::NUMPAD5, KeyEvent::NUMPAD6,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    // 0x58-0x5b
    KeyEvent::NUMPAD1, KeyEvent::NUMPAD2, KeyEvent::NUMPAD3, KeyEvent::ENTER,
    0,0,0,0,0, 0,0,0,
    // 0x64-0x65
    KeyEvent::NUMPAD0, KeyEvent::DECIMAL,
};

static const uint8 FUNCTION_KEYS[] = {
    KeyEvent::F1,KeyEvent::F2,KeyEvent::F3,KeyEvent::F4,
    KeyEvent::F5,KeyEvent::F6,KeyEvent::F7,KeyEvent::F8,
    KeyEvent::F9,KeyEvent::F10,KeyEvent::F11,KeyEvent::F12,
};

// If key code is changed, this table should be updated.
// no B_SUBSTITUTE in this table
// 0x10: KeyEvent::F1 is dummy, it have to be recalculated as function key
static const uint8 SPECIAL_KEYS[] = {
    0,KeyEvent::HOME,0,0,KeyEvent::END, 
        KeyEvent::INSERT,0,0,KeyEvent::BACKSPACE,KeyEvent::TAB, 
        KeyEvent::ENTER,KeyEvent::PAGE_UP,KeyEvent::PAGE_DOWN,0,0, 0, // 0f
    KeyEvent::F1,0,0,0,0, 0,0,0,0,0, 
        0,KeyEvent::ESCAPE,KeyEvent::LEFT,KeyEvent::RIGHT,KeyEvent::UP, 
        KeyEvent::DOWN, // 1f
    KeyEvent::SPACE,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0, // 2f
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, KeyEvent::DEL, // 7f
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,
    0,0,KeyEvent::KANA,KeyEvent::HANKAKU,KeyEvent::MUHENKAN, 
        KeyEvent::HENKAN,0,0,0,0, 
        0,0,0,0,0, 0, // ff
};

bool MozcEngine::_AddKey(uint8 byte, int32 key, uint32 *mod, 
                        KeyEvent *key_event) const
{
    // keypad, see keymap
    static_assert(sizeof(KEYPAD) == 67, "Invalid size of KEYPAD");
    if (0x23 <= key && key <= 0x65) {
        uint8 sk = KEYPAD[key - 0x23];
        if (sk != 0) {
            key_event->set_special_key(
                static_cast<mozc::commands::KeyEvent::SpecialKey>(sk));
            return true;
        }
    }
    if ('!' <= byte && byte <= '~') {
        // Ascii characters
        *mod = *mod & (~B_SHIFT_KEY);
        key_event->set_key_code(byte);
        return true;
    }
    static_assert(sizeof(SPECIAL_KEYS) == 256, "Invalid size of SPECIAL_KEYS");
    uint8 sk = SPECIAL_KEYS[byte];
    if (sk != KeyEvent::F1) {
        if (sk != 0) {
            // Convert Hiragana + Shift -> Katakana
            if (sk == KeyEvent::KANA && (*mod & B_SHIFT_KEY) > 0) {
                sk = KeyEvent::KATAKANA;
                // remove KeyEvent::SHIFT from modifier to make this work well
                *mod = *mod & (~B_SHIFT_KEY);
            } 
            key_event->set_special_key(
                static_cast<mozc::commands::KeyEvent::SpecialKey>(sk));
            return true;
        }
    } else if (B_F1_KEY <= key && key <= B_F12_KEY) {
        // recalculate as function key
        // no support on F13-F24 by Haiku OS
        // If byte is B_FUNCTION_KEY(0x10), key is function key.
        static_assert(sizeof(FUNCTION_KEYS) == 12, "Invalid size of FUNCTION_KEYS");
        key_event->set_special_key(
            static_cast<mozc::commands::KeyEvent::SpecialKey>(
                    FUNCTION_KEYS[key - B_F1_KEY]));
        return true;
    }
    return false;
}

#define ADD_MOD(MOD_VAR, EVENT_VAR, B_MOD, MOZC_MOD) \
    if ((MOD_VAR & (B_MOD)) != 0) { EVENT_VAR->add_modifier_keys(MOZC_MOD); }

void MozcEngine::_AddModifiers(uint32 mod, 
            mozc::commands::KeyEvent *key_event) const
{
    if (mod == 0) {
        return;
    }
    // Send shift separately, KeyEvent::SHIFT does not work with KATAKANA.
    ADD_MOD(mod, key_event, B_LEFT_SHIFT_KEY, KeyEvent::LEFT_SHIFT);
    ADD_MOD(mod, key_event, B_RIGHT_SHIFT_KEY, KeyEvent::RIGHT_SHIFT);
    // Shift + Enter requires KeyEvent::SHIFT
    ADD_MOD(mod, key_event, B_SHIFT_KEY, KeyEvent::SHIFT);
    ADD_MOD(mod, key_event, 
        (B_COMMAND_KEY | B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY), 
        KeyEvent::CTRL);
    
    ADD_MOD(mod, key_event, 
        (B_CONTROL_KEY | B_LEFT_CONTROL_KEY | B_RIGHT_CONTROL_KEY), 
        KeyEvent::ALT);
    
    ADD_MOD(mod, key_event, B_CAPS_LOCK, KeyEvent::CAPS);
}

IM_Mode MozcEngine::CompositionModeToMode(
                    mozc::commands::CompositionMode mode) const
{
    switch (mode)
    {
        case mozc::commands::DIRECT:
            return MODE_DIRECT;
            break;
        case mozc::commands::HIRAGANA:
            return MODE_HIRAGANA;
            break;
        case mozc::commands::FULL_KATAKANA:
            return MODE_FULLWIDTH_KATAKANA;
            break;
        case mozc::commands::HALF_ASCII:
            return MODE_HALFWIDTH_ASCII;
            break;
        case mozc::commands::FULL_ASCII:
            return MODE_FULLWIDTH_ASCII;
            break;
        case mozc::commands::HALF_KATAKANA:
            return MODE_HALFWIDTH_KATAKANA;
            break;
        default:
            break;
    }
    return MODE_DIRECT;
}

mozc::commands::CompositionMode MozcEngine::ModeToCompositionMode(
                                    IM_Mode mode) const
{
    switch (mode)
    {
        case MODE_DIRECT:
            return mozc::commands::DIRECT;
            break;
        case MODE_HIRAGANA:
            return mozc::commands::HIRAGANA;
            break;
        case MODE_FULLWIDTH_KATAKANA:
            return mozc::commands::FULL_KATAKANA;
            break;
        case MODE_HALFWIDTH_ASCII:
            return mozc::commands::HALF_ASCII;
            break;
        case MODE_FULLWIDTH_ASCII:
            return mozc::commands::FULL_ASCII;
            break;
        case MODE_HALFWIDTH_KATAKANA:
            return mozc::commands::HALF_KATAKANA;
            break;
        default:
            break;
    }
    return mozc::commands::DIRECT;
}

const char * MozcEngine::GetToolName(Mozc_Tool tool) const
{
    switch (tool)
    {
        case TOOL_ABOUT:
            return "about_dialog";
            break;
        case TOOL_CHARACTER_PAD:
            return "character_palette";
            break;
        case TOOL_CONFIG:
            return "config_dialog";
            break;
        case TOOL_DICTIONARY:
            return "dictionary_tool";
            break;
        case TOOL_HAND_WRITING:
            return "hand_writing";
            break;
        case TOOL_WORD_REGISTER:
            return "word_register_dialog";
            break;
        default:
            return "";
            break;
    }
    return "";
}

} // namespace immozc

