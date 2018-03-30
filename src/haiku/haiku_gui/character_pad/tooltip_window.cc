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

#include "haiku/haiku_gui/character_pad/tooltip_window.h"

#include <Catalog.h>
#include <View.h>

#include <string>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "character_pad"

namespace mozc {
namespace haiku_gui {

class ToolTipView : public BView
{
public:
    ToolTipView();
    virtual ~ToolTipView();

    virtual void Draw(BRect rect);
    void _GetPreferredSize(float* width, float* height);
    void SetInfo(const CharacterInfo& info);
    void SetCharacterFont(const BFont& font);
    void Init();
private:
    CharacterInfo maInfo;
    float mnLabelWidth;
    const char* maLabels[7];
};

ToolTipView::ToolTipView()
    : BView(
        BRect(),
        "tooltip_view",
        B_FOLLOW_NONE,
        B_WILL_DRAW)
{
    mnLabelWidth = 0;
}

ToolTipView::~ToolTipView()
{
}

void ToolTipView::Init()
{
    maLabels[0] = B_TRANSLATE("Kun Reading");
    maLabels[1] = B_TRANSLATE("On Reading");
    maLabels[2] = B_TRANSLATE("Source");
    maLabels[3] = B_TRANSLATE("Unicode");
    maLabels[4] = B_TRANSLATE("UTF-8");
    maLabels[5] = B_TRANSLATE("Shift-JIS");
    maLabels[6] = B_TRANSLATE("EUC-JP");

    for (size_t i = 0; i < 7; ++i) {
        mnLabelWidth = std::max(mnLabelWidth, StringWidth(maLabels[i]));
    }
    mnLabelWidth += StringWidth(": ", 2);
    mnLabelWidth = ceil(mnLabelWidth);
}

void ToolTipView::SetInfo(const CharacterInfo& info)
{
    maInfo.maChar = info.maChar;
    maInfo.maDescription = info.maDescription;
    maInfo.maKun = info.maKun;
    maInfo.maOn = info.maOn;
    maInfo.maSource = info.maSource;
    maInfo.maCode[0] = info.maCode[0];
    maInfo.maCode[1] = info.maCode[1];
    maInfo.maCode[2] = info.maCode[2];
    maInfo.maCode[3] = info.maCode[3];

    float width;
    float height;
    _GetPreferredSize(&width, &height);
    Window()->ResizeTo(width, height);
    ResizeTo(Window()->Size());
    Invalidate();
}

void ToolTipView::SetCharacterFont(const BFont& font)
{
    BFont f;
    GetFont(&f);
    BFont newFont(font);
    newFont.SetSize(f.Size());

    SetFont(&newFont);
}

void ToolTipView::_GetPreferredSize(float* width, float* height)
{
    if (maInfo.maChar.size() == 0) {
        *width = 0;
        *height = 0;
        return;
    }
    const float TOP_MARGIN = 5;
    const float MARGIN = 3;
    float w = TOP_MARGIN + MARGIN + 2;
    float h = MARGIN * 2 + 2;

    BFont font;
    GetFont(&font);
    float size = font.Size();
    font.SetSize(28);
    SetFont(&font);
    font_height charHeight;
    font.GetHeight(&charHeight);
    h += charHeight.ascent + charHeight.descent + charHeight.leading;

    font.SetSize(size);
    SetFont(&font);
    font_height lineFontHeight;
    font.GetHeight(&lineFontHeight);
    const float lineHeight = ceil(lineFontHeight.ascent + lineFontHeight.descent + lineFontHeight.leading);
    // other lines
    int32 lineCount = 0;
    float localWidth = 0;
    float descriptionWidth = 0;
    if (!maInfo.maDescription.empty()) {
        descriptionWidth = StringWidth(maInfo.maDescription.c_str());
        lineCount += 1;
    }
    if (!maInfo.maKun.empty()) {
        localWidth = std::max(localWidth, StringWidth(maInfo.maKun.c_str()));
        lineCount += 1;
    }
    if (!maInfo.maOn.empty()) {
        localWidth = std::max(localWidth, StringWidth(maInfo.maOn.c_str()));
        lineCount += 1;
    }
    if (!maInfo.maSource.empty()) {
        localWidth = std::max(localWidth, StringWidth(maInfo.maSource.c_str()));
        lineCount += 1;
    }
    for (int32 i = 0; i < 4; ++i) {
        localWidth = std::max(localWidth, StringWidth(maInfo.maCode[i].c_str()));
    }
    lineCount += 4;

    h += lineHeight * lineCount;
    w += std::max(descriptionWidth, mnLabelWidth + localWidth);

    *width = ceil(w);
    *height = ceil(h);
}

void ToolTipView::Draw(BRect rect)
{
    BRect bounds = Bounds();

    SetPenSize(1.0f);
    SetLowColor(ui_color(B_TOOL_TIP_BACKGROUND_COLOR));
    FillRect(Frame(), B_SOLID_LOW);
    SetHighColor(ui_color(B_TOOL_TIP_TEXT_COLOR));
    StrokeRect(Frame(), B_SOLID_HIGH);

    BFont font;
    GetFont(&font);
    float size = font.Size();
    BPoint pos(0, 0);
    {
        font.SetSize(28);
        SetFont(&font);
        font_height charHeight;
        font.GetHeight(&charHeight);

        pos.y = charHeight.ascent + 1;
        pos.x = bounds.Width() / 2 - StringWidth(maInfo.maChar.c_str()) / 2 + 1;
        DrawString(maInfo.maChar.c_str(), pos);
        pos.y += ceil(charHeight.descent);
    }
    font.SetSize(size);
    SetFont(&font);
    font_height lineFontHeight;
    font.GetHeight(&lineFontHeight);
    const float lineHeight = ceil(lineFontHeight.ascent + lineFontHeight.descent + lineFontHeight.leading);

    BPoint labelPos(1 + 3, pos.y + lineHeight);
    BPoint bodyPos(1 + 3 + mnLabelWidth, pos.y + lineHeight);
    pos.y += lineHeight;

    if (!maInfo.maDescription.empty()) {
        pos.x = bounds.Width() / 2 - StringWidth(maInfo.maDescription.c_str()) / 2;
        DrawString(maInfo.maDescription.c_str(), pos);
        labelPos.y += lineHeight;
        bodyPos.y = labelPos.y;
    }
    if (!maInfo.maKun.empty()) {
        DrawString(maLabels[0], labelPos);
        DrawString(maInfo.maKun.c_str(), bodyPos);
        labelPos.y += lineHeight;
        bodyPos.y = labelPos.y;
    }
    if (!maInfo.maOn.empty()) {
        DrawString(maLabels[1], labelPos);
        DrawString(maInfo.maOn.c_str(), bodyPos);
        labelPos.y += lineHeight;
        bodyPos.y = labelPos.y;
    }
    if (!maInfo.maSource.empty()) {
        DrawString(maLabels[2], labelPos);
        DrawString(maInfo.maSource.c_str(), bodyPos);
        labelPos.y += lineHeight;
        bodyPos.y = labelPos.y;
    }
    for (size_t i = 0; i < 4; ++i) {
        DrawString(maLabels[i + 3], labelPos);
        DrawString(maInfo.maCode[i].c_str(), bodyPos);
        labelPos.y += lineHeight;
        bodyPos.y = labelPos.y;
    }
}


ToolTipWindow::ToolTipWindow()
    : BWindow(
        BRect(0, 0, 0, 0),
        "tooltip_window",
        B_NO_BORDER_WINDOW_LOOK,
        B_FLOATING_ALL_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE |
        B_NOT_MINIMIZABLE | B_NOT_MOVABLE |
        B_AVOID_FOCUS |
        B_NOT_ANCHORED_ON_ACTIVATE)
{
    mpView = new ToolTipView();
    mpView->SetDrawingMode(B_OP_OVER);
    AddChild(mpView);

    Run();
    mpView->Init();
}

ToolTipWindow::~ToolTipWindow()
{
}

void ToolTipWindow::SetInfo(const CharacterInfo& info)
{
    if (mpView) {
        mpView->SetInfo(info);
    }
}

void ToolTipWindow::SetCharacterFont(const BFont& font)
{
    if (mpView) {
        mpView->SetCharacterFont(font);
    }
}

}; // haiku_gui
}; // mozc
