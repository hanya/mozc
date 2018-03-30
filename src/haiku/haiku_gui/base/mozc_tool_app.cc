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

#include "mozc_tool_app.h"

#include <Roster.h>

namespace mozc {
namespace haiku_gui {

#define MOZC_TOOL_APP_SIGNATURE "application/x-vnd.Mozc-MozcTool"

ToolApp::ToolApp(AppType type)
    : BApplication(MOZC_TOOL_APP_SIGNATURE),
      mpWindow(NULL),
      mnType(type)
{
}

ToolApp::~ToolApp()
{
    //delete mpWindow;
    mpWindow = NULL;
}

void ToolApp::MessageReceived(BMessage* msg)
{
    switch (msg->what)
    {
        case WINDOW_ACTIVATE:
        {
            int32 type = msg->GetInt32("type", static_cast<int32>(NONE));
            if (mpWindow != NULL && type == mnType) {
                mpWindow->Activate(true);
                
                BMessage reply(B_REPLY);
                reply.AddInt32("type", mnType);
                msg->SendReply(&reply);
            }
            break;
        }
        default:
        {
            BApplication::MessageReceived(msg);
            break;
        }
    }
}

// Returns true and activate its window specified by an application signature.
bool ToolApp::ActivateIfExists(AppType type)
{
    bool status = false;
    
    BMessage message(WINDOW_ACTIVATE);
    message.AddInt32("type", static_cast<int32>(type));
    BMessenger* messenger = new BMessenger();
    BList list(5);
    be_roster->GetAppList(MOZC_TOOL_APP_SIGNATURE, &list);
    for (int32 i = 0; i < list.CountItems(); ++i) {
        intptr_t id = (intptr_t)list.ItemAt(i);
        BMessage reply;
        if (messenger->SetTo(MOZC_TOOL_APP_SIGNATURE, id) == B_OK) {
            if (messenger->SendMessage(&message, &reply) == B_OK) {
                int32 t = NONE;
                if (reply.FindInt32("type", &t) == B_OK) {
                    status = t == type;
                }
            }
        }
    }
    delete messenger;

    return status;
}

} // namespace haiku_gui
} // namespace mozc
