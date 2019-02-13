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


#ifndef LOOPER_H_
#define LOOPER_H_

#include "haiku/input_method/common.h"

#include "protocol/commands.pb.h"

#include <Looper.h>
#include <Messenger.h>

#include <memory>

namespace mozc {
    namespace commands {
        class Output;
    }
}

class BMenu;

namespace immozc {

class CandidateWindow;
//class Indicator;
class MozcBar;
class MozcEngine;
#ifndef X86_GCC2
class MozcMethod;
#endif // X86_GCC2
struct Settings;
class SettingsWindow;

class MozcLooper : public BLooper
{
public:
#ifndef X86_GCC2
                            MozcLooper(MozcMethod *method);
#else // X86_GCC2
                            MozcLooper();
#endif // X86_GCC2
    virtual                 ~MozcLooper();
    virtual void            Quit();
    virtual void            MessageReceived(BMessage *msg);
    virtual void            EnqueueMessage(BMessage *msg);
            status_t        InitCheck();

private:
#ifndef X86_GCC2
    MozcMethod *                         fOwner;
#else // X86_GCC2
    BMessenger                           fMethod;
#endif // X86_GCC2
    std::unique_ptr<MozcEngine>          fEngine;
    //std::unique_ptr<Indicator>         fIndicator;
    MozcBar*                             fBar;
    std::unique_ptr<CandidateWindow>     fCandidateWindow;
    std::unique_ptr<BMessenger>          fMessenger;
    // composition mode of Mozc
    IM_Mode                              fCurrentMode;
    uint32                               fHighlightedPosition;
    // category of candidates
    int32                                fCategory;
    // true when Mozc is activated
    bool                                 fMozcActive;
    // true between B_INPUT_METHOD_STARTED and STOPPED.
    bool                                 fMethodStarted;
    bigtime_t                            fLastSync;
    std::unique_ptr<Settings>            fSettings;
    SettingsWindow*                      fSettingsWindow;

    void _HandleKeyDown(BMessage *msg);
    void _HandleLocationRequest(BMessage *msg);

    void         _SendMethodStarted(void);
    void         _SendMethodStopped(void);
    void         _HandleMethodActivated(bool active);
    void         _HandleModeChange(IM_Mode mode, bool forced=false);
    void         _ModeChanged();
    void         _SendModeToBar();
    void         _UpdateDeskbarMenu(void) const;
    bool         _HandleStatus(const mozc::commands::Output &output);
    void         _SwitchToDefaultMethod();
    void         _HandleInputMethodStopped();
    void         _SyncDataIfRequired(bool force=false);
    void         _HandleOutput(std::unique_ptr<mozc::commands::Output> output);
    void         _HandleCandidates(const mozc::commands::Output &output, 
                                   bool newly_activated);
    bool         _HandlePreedit(const mozc::commands::Output &output);
    void         _HandleResult(const mozc::commands::Output &output);
    void         _HandleLaunchToolMode(mozc::commands::Output::ToolMode mode);
    void         _SendMethodLocationRequest();
    void         _HandleSelectCandidate(int32 id);
    void         _HandleHighlightCandidate(int32 id);
    void         _ShowCandidateWindow();
    void         _HideCandidateWindow();
    void         _LoadSettings();
    void         _WriteSettings();
    std::string  _GetSettingsPath();
    //const uchar * _GetModeIcon(IM_Mode mode) const;
    void         _ShowSettingsWindow();

#if DEBUG
    std::unique_ptr<BMessenger>      fLogger;
    void                             _SendLog(const char *s);
#endif
};

} // namespace immozc

#endif // LOOPER_H_
