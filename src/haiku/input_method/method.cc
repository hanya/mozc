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


#include "haiku/input_method/method.h"

#include "haiku/input_method/common.h"
#include "haiku/input_method/icons.h"
#include "haiku/input_method/settings_window.h"
#ifndef X86_GCC2
#include "haiku/input_method/looper.h"
#else
#include <Application.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Handler.h>
#include <Input.h>
#include <Path.h>
#include <Roster.h>
#define MOZC_DATA_DIR  "mozc"
#define MOZC_TASK      "mozc_task"
#endif
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Messenger.h>

// Entry pont for the input method.
extern "C" _EXPORT 
BInputServerMethod *instantiate_input_method()
{
    return new immozc::MozcMethod();
}

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMethod"

namespace immozc {
#ifdef X86_GCC2
class MethodHandler : public BHandler
{
public:
    MethodHandler(MozcMethod* method);
    virtual ~MethodHandler() {};

    virtual void MessageReceived(BMessage* msg);
private:
    MozcMethod* fMethod;
};

MethodHandler::MethodHandler(MozcMethod* method)
    : BHandler(),
      fMethod(method)
{
}

void MethodHandler::MessageReceived(BMessage* msg)
{
    if (fMethod) {
        fMethod->MessageFromLooper(msg);
    }
}
#endif // X86_GCC2

namespace {
BMessage *_CreateToolMessage(Mozc_Tool tool)
{
    BMessage *msg = new BMessage(IM_TOOL);
    msg->AddInt32(MOZC_TOOL_TOOL, tool);
    msg->AddBool(DESKBAR, true);
    return msg;
}

BMessage *_CreateModeMessage(IM_Mode mode)
{
    BMessage *msg = new BMessage(IM_MODE_CHANGE_REQUEST);
    msg->AddInt32(IM_MODE_MODE, mode);
    msg->AddBool(DESKBAR, true);
    return msg;
}
} // namespace


MozcMethod::MozcMethod()
    : BInputServerMethod("Mozc", kUiHiraganaIconData)
{
#ifdef X86_GCC2
    fHandler = new MethodHandler(this);
    if (be_app->Lock()) {
        be_app->AddHandler(fHandler);
        be_app->Unlock();
    }
#endif // X86_GCC2
    fDeskbarMenu = new BMenu("Menu");
    BLayoutBuilder::Menu<>(fDeskbarMenu)
        .AddItem(B_TRANSLATE("Hiragana"),
            _CreateModeMessage(MODE_HIRAGANA))
        .AddItem(B_TRANSLATE("Fullwidth Katakana"),
            _CreateModeMessage(MODE_FULLWIDTH_KATAKANA))
        .AddItem(B_TRANSLATE("Halfwidth Alphabet"),
            _CreateModeMessage(MODE_HALFWIDTH_ASCII))
        .AddItem(B_TRANSLATE("Fullwidth Alphabet"),
            _CreateModeMessage(MODE_FULLWIDTH_ASCII))
        .AddItem(B_TRANSLATE("Halfwidth Katakana"),
            _CreateModeMessage(MODE_HALFWIDTH_KATAKANA))
        .AddSeparator()

        .AddItem(B_TRANSLATE("Word register"),
            _CreateToolMessage(TOOL_WORD_REGISTER))
        .AddItem(B_TRANSLATE("Dictionary"),
            _CreateToolMessage(TOOL_DICTIONARY))
        .AddItem(B_TRANSLATE("Character pad"),
            _CreateToolMessage(TOOL_CHARACTER_PAD))
        .AddItem(B_TRANSLATE("Handwriting"),
            _CreateToolMessage(TOOL_HAND_WRITING))
        .AddItem(B_TRANSLATE("Configuration"),
            _CreateToolMessage(TOOL_CONFIG))
        .AddSeparator()

        .AddItem(B_TRANSLATE("Setting..."),
            new BMessage(SettingsWindow::IM_SETTINGS_WINDOW))
        .AddSeparator()

        .AddItem(B_TRANSLATE("Show bar"),
            new BMessage(IM_BAR_SHOW_PERMANENT))
        .AddSeparator()
    //  .AddItem("Mozc direct input",
    //        new BMessage(MOZC_DIRECT_INPUT))
        .AddItem(B_TRANSLATE("About Mozc"),
            _CreateToolMessage(TOOL_ABOUT));
}

MozcMethod::~MozcMethod()
{
    SetMenu(NULL, BMessenger());
#ifndef X86_GCC2
    // This method is not called while shutdown, see InputServer::QuitRequested.
    BLooper *looper = NULL;
    fMozcLooper.Target(&looper);
    if (looper != NULL) {
        if (looper->Lock()) {
            looper->Quit();
        }
        delete looper;
    }
#else
    fMozcLooper.SendMessage(B_QUIT_REQUESTED);
    delete fHandler;
    // todo, kill mozc_task
#endif // X86_GCC2
}

#ifdef X86_GCC2
namespace {
bool CheckDataPath(directory_which n, BEntry *entry)
{
    BPath path;
    if (find_directory(n, &path) == B_OK) {
        path.Append(MOZC_DATA_DIR);
        path.Append(MOZC_TASK);
        entry->SetTo(path.Path());
        return entry->Exists();
    }
    return false;
}

bool FindMozcTask(BEntry *entry)
{
    //BEntry entry;
    if (CheckDataPath(B_USER_NONPACKAGED_DATA_DIRECTORY, entry)) {
        return true;
    }
    if (CheckDataPath(B_USER_DATA_DIRECTORY, entry)) {
        return true;
    }
    if (CheckDataPath(B_SYSTEM_DATA_DIRECTORY, entry)) {
        return true;
    }
    return false;
}
} // namespace
#endif // X86_GCC2

status_t MozcMethod::InitCheck()
{
    status_t e = B_OK;
#ifndef X86_GCC2
    MozcLooper *looper = new MozcLooper(this);
    if (looper->Lock()) {
        e = looper->InitCheck();
        looper->Unlock();
    }
    fMozcLooper = BMessenger(NULL, looper);
#else // X86_GCC2
    // start backend
    BRoster roster;
    if (!roster.IsRunning(MOZC_BACKEND_SIG)) {
        BEntry entry;
        if (FindMozcTask(&entry)) {
            entry_ref ref;
            if (entry.GetRef(&ref) == B_OK) {
                e = roster.Launch(&ref);
            }
        }
    }
    if (e == B_OK) {
        fMozcLooper = BMessenger(MOZC_BACKEND_SIG);
        BMessage message('init');
        message.AddMessenger("method", BMessenger(fHandler, be_app));
        fMozcLooper.SendMessage(&message);
    }
#endif // X86_GCC2
    return e;
}

filter_result MozcMethod::Filter(BMessage *msg, BList *_list)
{
    // Resends only key down message to the looper to process.
    if (msg->what == B_KEY_DOWN) {
        fMozcLooper.SendMessage(msg);
        return B_SKIP_MESSAGE;
    }
    return B_DISPATCH_MESSAGE;
}

// called from _BMethodAddOn_
status_t MozcMethod::MethodActivated(bool active)
{
    BMessage msg(active ? IM_METHOD_ACTIVATED : IM_METHOD_DEACTIVATED);
    fMozcLooper.SendMessage(&msg);
    return BInputServerMethod::MethodActivated(active);
}

void MozcMethod::_UpdateMenu(BMessage* msg)
{
    const int32 nCurrentMode = msg->GetInt32("mode", (int32)MODE_END);
    if (nCurrentMode == MODE_END) {
        return;
    }
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
                    if (mode == nCurrentMode) {
                        item->SetMarked(true);
                        break;
                    }
                }
            }
        }
    }

    BMessenger messenger;
    msg->FindMessenger("messenger", &messenger);
    SetMenu(fDeskbarMenu, messenger);
}

void MozcMethod::MessageFromLooper(BMessage* msg)
{
    switch(msg->what) {
#ifdef X86_GCC2
        case 'MRsm': //IS_SET_METHOD:
        {
            be_app->MessageReceived(msg);
            break;
        }
        case B_KEY_DOWN:
        {
            // send copy
            BMessage* message = new BMessage(*msg);
            EnqueueMessage(message);
            break;
        }
        case B_INPUT_METHOD_EVENT:
        {
            BMessage* message = new BMessage(*msg);
            EnqueueMessage(message);
            break;
        }
#endif // X86_GCC2
        case IM_MODE_CHANGE_REQUEST:
        {
            _UpdateMenu(msg);
            break;
        }
    }
}

} // namespace
