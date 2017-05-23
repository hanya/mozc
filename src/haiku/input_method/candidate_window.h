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

#ifndef CANDIDATE_WINDOW_H_
#define CANDIDATE_WINDOW_H_

#include <Window.h>

#include <memory>

namespace mozc {
    namespace commands {
        class Output;
    }
}

namespace immozc {

class CandidateView;
class MozcLooper;


#define CANDIDATE_WINDOW_CURSOR_NAME  "cursor"

enum {
    CANDIDATE_WINDOW_UPDATE = 'CWup',
    CANDIDATE_WINDOW_CURSOR_INDEX = 'CWcr',
    CANDIDATE_WINDOW_WINDOW_POSITIONED = 'CWwp',
    
};

class CandidateWindow : public BWindow
{
public:
                 CandidateWindow(MozcLooper *looper);
    virtual void MessageReceived(BMessage *msg);
            void SetData(std::unique_ptr<mozc::commands::Output> output);
    
private:
    CandidateView *      fCandidateView;
    MozcLooper *         fMozcLooper;
    uint32               fCursorIndex;
    BPoint               fLastLocation;
    
#if DEBUG
    std::unique_ptr<BMessenger>      fLogger;
    void                             _SendLog(const char *s);
#endif
};

} // namespace immozc

#endif // CANDIDATE_WINDOW_H_
