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


#include "haiku/input_method/indicator.h"

#include "haiku/input_method/looper.h"

#include <Input.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <StringView.h>


namespace immozc {

static const float border = 1.;

class Label : public BView
{
public:
    Label(const char *name, const char *label);
    virtual ~Label();
    virtual void Draw(BRect rect);
    void SetText(const char *label);
    
private:
    std::string     fLabel;
    BPoint             fLabelOrigin;
    bool             fResized;
    
    void _CalculateSize();
};

Label::Label(const char *name, const char *label)
    : BView(
        BRect(0, 0, 10, 10),
        name,
        B_FOLLOW_ALL_SIDES,
        B_WILL_DRAW),
     fLabelOrigin(BPoint(0, 0)),
     fResized(false)
{
    SetText(label);
}

Label::~Label()
{
}

void Label::SetText(const char *label)
{
    if (label != NULL) {
        fLabel = label;
        fResized = false;
        Invalidate();
    }
}

void Label::_CalculateSize()
{
    BFont font;
    GetFont(&font);
    font_height fontHeight;
    font.GetHeight(&fontHeight);
    float lineHeight = fontHeight.ascent + fontHeight.descent + fontHeight.leading;
    float labelWidth = StringWidth(fLabel.c_str(), fLabel.length());
    float horiMargin = StringWidth(" ", 1) * 2;
    float vertMargin = lineHeight * 0.25;
    // calculate label position
    fLabelOrigin = BPoint(horiMargin + border, 
                          fontHeight.ascent + vertMargin + border);
    // resize window
    float width = horiMargin * 2 + labelWidth + border * 2;
    float height = lineHeight + vertMargin * 2 + border * 2;
    Window()->ResizeTo(width, height);
    ResizeTo(width, height);
    fResized = true;
}

void Label::Draw(BRect rect)
{
    if (!fResized) {
        _CalculateSize();
    }
    rgb_color highColor = HighColor();
    static const rgb_color borderColor = {0xB0, 0xB0, 0xB0, 1};
    
    // fill
    FillRect(Frame(), B_SOLID_LOW);
    // draw border
    SetPenSize(1.);
    SetHighColor(borderColor);
    StrokeRect(Frame());
    SetHighColor(highColor);
    // draw label
    DrawString(fLabel.c_str(), fLabel.length(), fLabelOrigin);
}


Indicator::Indicator(MozcLooper *looper, const char *label, bigtime_t delay)
    : BWindow(
        BRect(50, 50, 50, 50),
        "Indicator",
        B_NO_BORDER_WINDOW_LOOK,
        B_FLOATING_ALL_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE |
        B_NOT_MINIMIZABLE | B_NOT_MOVABLE |
        B_AVOID_FOCUS |
        B_NOT_ANCHORED_ON_ACTIVATE),
     fLooper(looper),
     fDelay(delay)
{
    fLabel = new Label("label", label);
    AddChild(fLabel);
    
    Run();
}

Indicator::~Indicator()
{
}

void Indicator::_ShowWithCloseDelay(BPoint point, bigtime_t delay)
{
    MoveTo(point);
    SetWorkspaces(B_CURRENT_WORKSPACE);
    Show();
    BMessage msg(IM_INDICATOR_HIDE);
    fRunner = new BMessageRunner(BMessenger(this), &msg, delay, 1);
}

void Indicator::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case IM_INDICATOR_SHOW:
        {
            BPoint point;
            if (msg->FindPoint(IM_INDICATOR_LOCATION_NAME, &point) != B_OK) {
                point = BPoint(100, 100);
            }
            if (IsHidden()) {
                bigtime_t delay;
                if (msg->FindInt64(IM_INDICATOR_DELAY_NAME, &delay) != B_OK) {
                    delay = fDelay;
                }
                _ShowWithCloseDelay(point, delay);
            }
            break;
        }
        case IM_INDICATOR_HIDE:
        {
            if (!IsHidden()) {
                Hide();
                delete fRunner;
                fRunner = NULL;
            }
            break;
        }
        case IM_INDICATOR_SET_LABEL:
        {
            const char *label;
            if (msg->FindString(IM_INDICATOR_LABEL_NAME, &label) == B_OK) {
                fLabel->SetText(label);
            }
            break;
        }
        case B_INPUT_METHOD_EVENT:
        {
            uint32 opcode = msg->GetInt32("be:opcode", 0);
            if (opcode == B_INPUT_METHOD_LOCATION_REQUEST) {
                BPoint point;
                float height;
                if (msg->FindPoint("be:location_reply", 0, &point) == B_OK &&
                    msg->FindFloat("be:height_reply", 0, &height) == B_OK) {
                    point.y += height + 1.;
                } else {
                    point = BPoint(100, 100);
                }
                if (IsHidden()) {
                    _ShowWithCloseDelay(point, fDelay);
                }
            }
            BMessage mess(IM_INDICATOR_SHOWN);
            fLooper->PostMessage(&mess);
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}

} // namespace immozc

