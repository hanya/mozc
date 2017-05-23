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


#ifndef BAR_H_
#define BAR_H_

#include "haiku/input_method/common.h"

#include <Bitmap.h>
#include <Window.h>

#include <memory>

class BPopUpMenu;

namespace immozc {

class BarButton;
class MozcLooper;

#define IM_BAR_TOOLS_ICON_STATE 'Ibts'

class MozcBar : public BWindow
{
public:
                     MozcBar(MozcLooper *looper, 
                             orientation ort=B_HORIZONTAL, float size=16.);
    virtual          ~MozcBar();
    virtual void     MessageReceived(BMessage *msg);
    virtual void     FrameMoved(BPoint pos);
    
private:
    MozcLooper *    fLooper;
    float           fIconSize;
    orientation     fOrientation;
    
    std::unique_ptr<BBitmap>    fDirectIcon;
    std::unique_ptr<BBitmap>    fHiraganaIcon;
    std::unique_ptr<BBitmap>    fFullwidthKatakanaIcon;
    std::unique_ptr<BBitmap>    fHalfwidthAsciiIcon;
    std::unique_ptr<BBitmap>    fFullwidthAsciiIcon;
    std::unique_ptr<BBitmap>    fHalfwidthKatakanaIcon;
    std::unique_ptr<BBitmap>    fToolsIcon;
    
    BarButton *  fModeButton;
    BarButton *  fToolsButton;
    
    bool         fHidden;
    bool         fActive;
    
    std::unique_ptr<BPopUpMenu>        fModeMenu;
    std::unique_ptr<BPopUpMenu>        fToolsMenu;
    std::unique_ptr<BPopUpMenu>        fContextMenu;
    
    void         _Init();
    void         _ModeChanged(IM_Mode mode);
    BMessage *    _CreateToolMessage(Mozc_Tool tool) const;
    void         _ChangeOrientation(orientation ort);
    bool         _GetMenuPosition(const char *name, 
                                  BPopUpMenu *menu, BPoint *where);
    void         _UpdateBarMenu();
    BBitmap *    _GetModeIcon(bool active, IM_Mode mode);
    
};

} // namespace immozc

#endif // BAR_H_
