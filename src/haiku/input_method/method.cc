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
#include "haiku/input_method/looper.h"

#include <Messenger.h>

// Entry pont for the input method.
extern "C" _EXPORT 
BInputServerMethod *instantiate_input_method()
{
    return new immozc::MozcMethod();
}

namespace immozc {

MozcMethod::MozcMethod()
    : BInputServerMethod("Mozc", kUiHiraganaIconData)
{
}

MozcMethod::~MozcMethod()
{
    // This method is not called while shutdown, see InputServer::QuitRequested.
    BLooper *looper = NULL;
    fMozcLooper.Target(&looper);
    if (looper != NULL) {
        if (looper->Lock()) {
            looper->Quit();
        }
        delete looper;
    }
}

status_t MozcMethod::InitCheck()
{
    status_t e;
    MozcLooper *looper = new MozcLooper(this);
    if (looper->Lock()) {
        e = looper->InitCheck();
        looper->Unlock();
    }
    fMozcLooper = BMessenger(NULL, looper);
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

} // namespace

