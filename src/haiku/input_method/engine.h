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

#ifndef ENGINE_H_
#define ENGINE_H_

#include "client/client.h"
#include "protocol/commands.pb.h"

#include "haiku/input_method/common.h"

#include <memory>

namespace immozc {

class MozcEngine
{
public:
            MozcEngine();
    virtual ~MozcEngine();
    
    bool SendKey(uint8 byte, int32 key, uint32 mod, 
                mozc::commands::Output *output);
    bool SendCommand(const mozc::commands::SessionCommand &command, 
                     mozc::commands::Output *output);
    bool SelectCandidate(int32 id, mozc::commands::Output *output);
    bool HighlightCandidate(int32 id, mozc::commands::Output *output);
    bool SwitchInputMode(mozc::commands::CompositionMode mode, 
                         mozc::commands::Output *output);
    bool ConvertPrevPage(mozc::commands::Output *output);
    bool ConvertNextPage(mozc::commands::Output *output);
    bool GetStatus(mozc::commands::Output *output);
    bool Revert();
    bool SyncData();
    bool Shutdown();
    bool Reload();
    bool TurnOn(mozc::commands::Output *output);
    bool TurnOff(mozc::commands::Output *output);
    bool OpenBrowser(const string &url);
    bool LaunchTool(const string &mode, const string &extra_arg);
    IM_Mode CompositionModeToMode(mozc::commands::CompositionMode mode) const;
    mozc::commands::CompositionMode ModeToCompositionMode(IM_Mode mode) const;
    const char * GetToolName(Mozc_Tool tool) const;
    void UpdatePreeditMethod();
    void SetKanaMapping(IM_Kana_Mapping mapping);

private:
    std::unique_ptr<mozc::client::ClientInterface>    fClient;
    
    bool _AddKey(uint8 byte, int32 key, uint32 *mod, 
                 mozc::commands::KeyEvent *key_event) const;
    void _AddModifiers(uint32 mod, 
                       mozc::commands::KeyEvent *key_event) const;

    mozc::config::Config::PreeditMethod     nPreeditMethod;
    IM_Kana_Mapping mnKanaMapping;
    const char** mpKanaMapping;
    const char** mpKanaShiftMapping;
};

} // namespace immozc

#endif // ENGINE_H_
