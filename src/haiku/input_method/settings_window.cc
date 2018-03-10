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


#include "settings_window.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>

#if DEBUG
#include <stdio.h>
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputMethod"

namespace immozc {


SettingsWindow::SettingsWindow(BLooper* pLooper)
    : BWindow(
        BRect(50, 50, 100, 100),
        B_TRANSLATE("Input Method Settings"),
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_NOT_RESIZABLE |
        B_AUTO_UPDATE_SIZE_LIMITS |
        B_NOT_ZOOMABLE),
    mpLooper(pLooper)
{
    mpKanaMappingPM = new BPopUpMenu("kana_mappingPM");
    BLayoutBuilder::Menu<>(mpKanaMappingPM)
        .AddItem("JP",
            _CreateKanaMappingMessage(KANA_MAPPING_JP))
        .AddItem("US",
            _CreateKanaMappingMessage(KANA_MAPPING_US));
    mpKanaMappingMF = new BMenuField("kana_mappingMF", "", mpKanaMappingPM);

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(6, 6, 6, 6)
        .AddGrid()
            .Add(new BStringView("kana_mappingL", B_TRANSLATE("Kana mapping")), 0, 1)
            .Add(mpKanaMappingMF, 1, 1)
        .End()
        .AddGlue();

    Layout(true);
    CenterOnScreen();
}

SettingsWindow::~SettingsWindow()
{
    mpLooper = NULL;
}

BMessage* SettingsWindow::_CreateKanaMappingMessage(IM_Kana_Mapping value) const
{
    BMessage* msg = new BMessage(IM_KANA_MAPPING_MSG);
    msg->AddInt32(IM_KANA_MAPPING_VALUE, value);
    return msg;
}

bool SettingsWindow::QuitRequested()
{
    // todo, send message to the looper
    if (mpLooper) {
        BMessage *msg = new BMessage(IM_SETTINGS_WINDOW_CLOSED);
        mpLooper->PostMessage(msg);
    }
    BWindow::QuitRequested();
    return true;
}

void SettingsWindow::_Init(BMessage *msg)
{
#if DEBUG
    BMessage log('Logg');
    log.AddString("log", "SettingsWindow._Init");
    mpLooper->MessageReceived(&log);
#endif
    int32 mapping;
    if (msg->FindInt32(IM_KANA_MAPPING_VALUE, &mapping) == B_OK) {
#if DEBUG
            char s[64];
            sprintf(s, "SettingsWindow.mapping: %d", mapping);
            log.ReplaceString("log", (const char*)s);
            mpLooper->MessageReceived(&log);
#endif
        if (mapping < KANA_MAPPING_CUSTOM || KANA_MAPPING_END < mapping) {
            mapping = KANA_MAPPING_JP;
        }
        if (mpKanaMappingPM) {
            for (int32 i = 0; i < mpKanaMappingPM->CountItems(); ++i) {
                BMenuItem* item = mpKanaMappingPM->ItemAt(i);
                if (item) {
                    BMessage* m = item->Message();
                    if (m) {
                        int32 value = 0;
                        if (m->FindInt32(IM_KANA_MAPPING_VALUE, &value) == B_OK) {
                            item->SetMarked(value == mapping);
                        }
                    }
                }
            }
        }
    }
}

void SettingsWindow::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case IM_SETTINGS_SET:
        {
            _Init(msg);
            break;
        }
        case IM_KANA_MAPPING_MSG:
        {
            if (mpLooper) {
                mpLooper->PostMessage(msg);
            }
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

} // namesapce immozc
