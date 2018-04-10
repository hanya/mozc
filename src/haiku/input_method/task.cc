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

#include "haiku/input_method/task.h"

#include "haiku/input_method/common.h"
#include "haiku/input_method/looper.h"

#include <Messenger.h>

namespace immozc {

MozcTask::MozcTask()
    : BApplication(MOZC_BACKEND_SIG)
{
    MozcLooper* looper = new MozcLooper();
    if (looper->Lock()) {
        looper->InitCheck();
        looper->Unlock();
    }
    fMozcLooper = BMessenger(NULL, looper);
}

MozcTask::~MozcTask()
{
    BLooper* looper = NULL;
    fMozcLooper.Target(&looper);
    if (looper != NULL) {
        if (looper->Lock()) {
            looper->Quit();
        }
        delete looper;
    }
}

bool MozcTask::QuitRequested()
{
    BLooper* looper = NULL;
    fMozcLooper.Target(&looper);
    if (looper != NULL) {
        if (looper->Lock()) {
            looper->Quit();
        }
        delete looper;
        fMozcLooper.SetTo((const BHandler*)NULL);
    }
    return true;
}

void MozcTask::MessageReceived(BMessage* msg)
{
    switch(msg->what) {
        case B_KEY_DOWN:
            fMozcLooper.SendMessage(msg);
            break;
        case IM_METHOD_ACTIVATED:
            fMozcLooper.SendMessage(msg);
            break;
        case IM_METHOD_DEACTIVATED:
            fMozcLooper.SendMessage(msg);
            break;
        case 'init':
          fMozcLooper.SendMessage(msg);
          break;
        default:
            BApplication::MessageReceived(msg);
            break;
    }
}

} // namespace immozc

int main(int argc, char* argv[]) {
    immozc::MozcTask *app = new immozc::MozcTask();
    app->Run();
    delete app;
    return 0;
}
