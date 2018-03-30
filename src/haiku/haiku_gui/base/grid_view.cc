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

#include "haiku/haiku_gui/base/grid_view.h"

#include <Cursor.h>
#include <Region.h>
#include <ScrollBar.h>

namespace mozc {
namespace haiku_gui {


GridTitleView::GridTitleView(bool bEnableResizing, BHandler* handler)
    : BView(
        BRect(0, 0, 10, 10),
        "title", 0,
        B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      mbEnableResizing(bEnableResizing),
      mbResizing(false),
      mnStartX(0),
      mnResizeColumn(-1),
      mpHandler(handler)
{
    font_height height;
    GetFontHeight(&height);
    mnHeight = ceil((height.ascent + height.descent) * 1.2);

    if (mbEnableResizing) {
        SetEventMask(B_POINTER_EVENTS, B_FULL_POINTER_HISTORY);
    }

    SetDrawingMode(B_OP_OVER);
}

GridTitleView::~GridTitleView()
{
}

int32 GridTitleView::CountItems() const
{
    return maTitles.size();
}

void GridTitleView::AddColumn(const char* label)
{
    maTitles.push_back(label);
    mnTitleWidths.push_back(0);
    const float width = StringWidth(label);
    mnTitleTextHalfWidth.push_back(ceil(width / 2));
}

float GridTitleView::ColumnLeft(int32 column) const
{
    if (!_IsValidColumnIndex(column)) {
        return -1;
    }
    float width = 0;
    for (int32 i = 0; i < column; ++i) {
        width += mnTitleWidths[i];
    }
    return width;
}

float GridTitleView::TotalWidth(int32 nStart, int32 nEnd) const
{
    const int32 nColumnCount = mnTitleWidths.size();
    if (nStart > nEnd ||
        nStart < 0 || nStart >= nColumnCount ||
        nEnd >= nColumnCount) {
        return 0;
    }
    float width = 0;
    for (int32 i = nStart; i <= nEnd; ++i) {
        width += mnTitleWidths[i];
    }
    return width;
}

void GridTitleView::_SetCursor(BCursorID id)
{
    BCursor cursor(id);
    SetViewCursor(&cursor, true);
}

float GridTitleView::ColumnWidth(size_t n) const
{
    return (0 <= n && n < mnTitleWidths.size()) ? mnTitleWidths[n] : 0;
}

void GridTitleView::_CheckLastColumnWidth()
{
    const int32 nLastColumn = CountItems() - 1;
    if (nLastColumn < 0) {
        return;
    }
    if (mnTitleWidths[nLastColumn] < MIN_WIDTH) {
        mnTitleWidths[nLastColumn] = MIN_WIDTH;
    }
}

void GridTitleView::SetWidth(float width)
{
    const int32 nLastColumn = CountItems() - 1;
    if (nLastColumn < 0) {
        return;
    }
    if (mnTitleWidths[0] > 0) {
        const float w = ColumnLeft(nLastColumn);
        mnTitleWidths[nLastColumn] = width - w;

        mnTitleWidths[nLastColumn] = Bounds().Width() - ColumnLeft(nLastColumn);
        _CheckLastColumnWidth();
    } else {
        // initialize
        float w = floor(width / mnTitleWidths.size());
        for (size_t i = 0; i < mnTitleWidths.size(); ++i) {
            mnTitleWidths[i] = w;
        }
        mnTitleWidths[mnTitleWidths.size() - 1] += 1;
    }
    ResizeTo(width, mnHeight - 1);
}

void GridTitleView::Draw(BRect rect)
{
    // fill background
    const color_which low = LowUIColor();
    SetLowUIColor(B_CONTROL_BACKGROUND_COLOR);
    FillRect(rect, B_SOLID_LOW);

    const int32 nTitles = mnTitleWidths.size();
    // draw grid
    {
        SetHighUIColor(B_CONTROL_BORDER_COLOR);
        BRegion r(Bounds());
        ConstrainClippingRegion(&r);

        BPoint start(0, 0);
        BPoint end(0, rect.Height());
        for (int32 i = 0; i < nTitles - 1; ++i) {
            start.x += mnTitleWidths[i];
            end.x = start.x;
            StrokeLine(start, end);
        }
        StrokeLine(BPoint(0, rect.Height()), BPoint(rect.Width(), rect.Height()));
    }
    // draw titles
    {
        SetHighUIColor(B_CONTROL_TEXT_COLOR);
        BRegion r;
        float x = 0;
        BPoint pos(0, floor(mnHeight * 0.75));
        for (int32 i = 0; i < nTitles; ++i) {
            pos.x = x + mnTitleWidths[i] / 2 - mnTitleTextHalfWidth[i];
            r.Set(BRect(x, 0, x + mnTitleWidths[i], mnHeight));
            ConstrainClippingRegion(&r);

            DrawString(maTitles[i].c_str(), pos);
            x += mnTitleWidths[i];
        }
    }

    SetLowUIColor(low);
}

void GridTitleView::MouseMoved(BPoint p, uint32 code, const BMessage* message)
{
    if (!mbEnableResizing) {
        return;
    }
    if (code == B_EXITED_VIEW) {
        if (!mbResizing) {
            _SetCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
        }
        return;
    } else if (mbResizing) {
        return;
    }
    const int32 nTitles = mnTitleWidths.size();
    const float x = p.x;
    bool bStarted = false;
    float w = 0;
    for (int32 i = 0; i < nTitles; ++i) {
        w += mnTitleWidths[i];
        if (w - 4 <= x && x <= w + 4) {
            _SetCursor(B_CURSOR_ID_RESIZE_EAST_WEST);
            // start resizing
            bStarted = true;
            break;
        }
    }
    if (!bStarted) {
        _SetCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
    }
}

void GridTitleView::MouseDown(BPoint p)
{
    //if (!mbEnableResizing) {
    //    return;
    //}
    uint32 buttons;
    GetMouse(&p, &buttons, false);
    if (!(buttons & B_PRIMARY_MOUSE_BUTTON)) {
        return;
    }
    if (p.y > mnHeight) {
        return;
    }
    int32 index = -1;
    const int32 nTitles = mnTitleWidths.size();
    const float x = p.x;
    float w = 0;
    for (int32 i = 0; i < nTitles; ++i) {
        w += mnTitleWidths[i];
        if (mbEnableResizing && w - 4 <= x && x <= w + 4) {
            _SetCursor(B_CURSOR_ID_RESIZE_EAST_WEST);
            // start resizing
            mbResizing = true;
            mnResizeColumn = i;
            break;
        } else if (index == -1 && x <= w) {
            index = i;
        }
    }
    if (mbResizing) {
        mnStartX = x;
    } else {
        _SortRequest(index);
    }
}

void GridTitleView::MouseUp(BPoint p)
{
    uint32 buttons;
    GetMouse(&p, &buttons, false);
    if (mbResizing) {
        const float distance = p.x - mnStartX;
        if (_IsValidColumnIndex(mnResizeColumn)) {
            float width = mnTitleWidths[mnResizeColumn] + distance;
            mnTitleWidths[mnResizeColumn] = width > MIN_WIDTH ? width : MIN_WIDTH;

            const int32 nLastColumn = CountItems() - 1;
            mnTitleWidths[nLastColumn] = Bounds().Width() - ColumnLeft(nLastColumn);
            _CheckLastColumnWidth();
            Invalidate();
        }
        _SetCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
        mbResizing = false;
        Parent()->Invalidate();
    }
}

void GridTitleView::_SortRequest(int32 index)
{
    if (mpHandler && _IsValidColumnIndex(index)) {
        BMessage message(GRID_SORT);
        message.AddInt32("index", index);
        mpHandler->MessageReceived(&message);
    }
}


GridRow::GridRow()
    : mbSelected(false)
{
}

GridRow::~GridRow()
{
}


InputTextView::InputTextView(const char* name)
    : BTextView(name),
      mbMonitorModified(false)
{
    SetWordWrap(false);
}

InputTextView::~InputTextView()
{
}

void InputTextView::Paste(BClipboard* clipboard)
{
    BTextView::Paste(clipboard);
    Invalidate();
}

void InputTextView::MakeFocus(bool focus)
{
    if (focus == IsFocus()) {
        return;
    }
    if (focus) {
        SelectAll();
    }
    BTextView::MakeFocus(focus);
}

void InputTextView::EditDone()
{
    if (!Parent()->IsHidden()) {
        Parent()->Parent()->MakeFocus(true);
        Parent()->Hide();
    }
    SetModified(false);
}

void InputTextView::EnableMonitorModification()
{
    mbMonitorModified = true;
}

void InputTextView::DisableMonitorModification()
{
    mbMonitorModified = false;
}

void InputTextView::FrameResized(float width, float height)
{
    SetTextRect(Bounds());
}

void InputTextView::InsertText(const char* text, int32 length, int32 offset,
                            const text_run_array* runs)
{
    if (length == 1) {
        if (*text == '\n' || *text == '\r') {
            BTextView::InsertText(" ", length, offset, runs);
        } else {
            BTextView::InsertText(text, length, offset, runs);
        }
    } else {
        BString t(text, length);
        t.ReplaceAll('\n', ' ');
        t.ReplaceAll('\r', ' ');
        BTextView::InsertText(t.String(), length, offset, runs);
    }

    if (mbMonitorModified) {
        SetModified(true);
    }
}

void InputTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
    BTextView::DeleteText(fromOffset, toOffset);

    if (mbMonitorModified) {
        SetModified(true);
    }
}

void InputTextView::SetModified(bool state)
{
    mbModified = state;
    if (state) {
        _SendUpdate();
    }
}

void InputTextView::_RequestToApply()
{
    BMessage message(APPLY);
    Parent()->Parent()->MessageReceived(&message);
    EditDone();
}

void InputTextView::_SendKeyDown(const char* byte)
{
    BMessage message(B_KEY_DOWN);
    message.AddInt8("byte", *byte);
    message.AddInt32("modifiers", static_cast<int32>(modifiers()));
    Parent()->Parent()->MessageReceived(&message);
}

void InputTextView::_SendUpdate()
{
    BMessage message(UPDATE);
    Parent()->Parent()->MessageReceived(&message);
}

void InputTextView::KeyDown(const char* bytes, int32 num)
{
    switch (*bytes) {
        case B_UP_ARROW:
            _RequestToApply();
            _SendKeyDown(bytes);
            break;
        case B_DOWN_ARROW:
            _RequestToApply();
            _SendKeyDown(bytes);
            break;
        case B_ENTER:
            _RequestToApply();
            break;
        case B_ESCAPE:
            // cancel edit, dispose modification
            SetModified(false);
            EditDone();
            break;
        case B_TAB:
            _RequestToApply();
            //BView::KeyDown(bytes, num);
            _SendKeyDown(bytes);
            break;
        default:
            BTextView::KeyDown(bytes, num);
            break;
    }
}


ListInputView::ListInputView(InputTextView* p)
    : BView(
        BRect(),
        "inputview",
        B_FOLLOW_NONE,
        B_WILL_DRAW)
{
    AddChild(p);
}

ListInputView::~ListInputView()
{
}

} // haiku_gui
} // mozc
