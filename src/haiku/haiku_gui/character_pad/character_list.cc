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

#include "haiku/haiku_gui/character_pad/character_list.h"

#include "haiku/haiku_gui/base/key_filter.h"

#include <ScrollView.h>

namespace mozc {
namespace haiku_gui {

const char* TITLE_CHARS[] = {
    "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F",
};


CharacterList::CharacterList(const char *name, Mode mode, uint32 flags,
                            BMessage* message, const BLooper* looper,
                            BMessage* onOverMessage)
    : BView(BRect(0, 0, 10, 10), name, B_FOLLOW_ALL_SIDES,
        flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE |
            B_FRAME_EVENTS | B_NAVIGABLE),
      BInvoker(message, NULL, looper),
    mnMode(mode),
    mnFontSize(16),
    mnColumnWidth(1),
    mnRowHeight(1),
    mnColumns(1),
    mnRows(1),
    mnStartColumn(0),
    mnStartRow(0),
    mnColumnCount(16),
    mnRowCount(0),
    mnSelectedColumn(-1),
    mnSelectedRow(-1),
    mnOnOverColumn(-1),
    mnOnOverRow(-1),
    mbShowTitle(true),
    mpScrollView(NULL),
    mpOnOverMessage(onOverMessage)
{
    maRange.SetRange(0, 0);
    maLocalMap = NULL;
    mnLocalMapSize = 0;
    // to receive B_KEY_DOWN message
    AddFilter(new KeyFilter(this));
    _FontReset();
    SetMouseEventMask(B_POINTER_EVENTS, B_FULL_POINTER_HISTORY);
    _SetMode(mnMode);
}

CharacterList::~CharacterList()
{
    mpScrollView = NULL;
    _ClearLocalMapRows();
}

void CharacterList::_SetMode(CharacterList::Mode mode)
{
    mnMode = mode;
    mbShowTitle = mode != TABLE;
    mnColumnCount = mode != TABLE ? 16 : 1;

    mnSelectedRow = -1;
    mnSelectedColumn = -1;

    _FontReset();
    _Reset();
    Invalidate();
}

void CharacterList::SetRange(uint32 first, uint32 last)
{
    if (first <= last) {
        maRange.SetRange(first, last);
        _SetMode(UNICODE_RANGE);
    }
}

void CharacterList::SetLocalMap(
        const mozc::gui::CharacterPalette::LocalCharacterMap* local_map,
        size_t local_map_size)
{
    if (mnMode == LOCAL_MAP && local_map == maLocalMap) {
        // the same local map is shown, no need to update
        return;
    }
    _ClearLocalMapRows();
    maLocalMap = local_map;

    const int32 from_start = local_map[0].from;
    const int32 from_end = local_map[local_map_size - 1].from + 0x10;
    for (int32 i = from_start; i < from_end; i += 0x10) {
        maLocalMapRows.push_back(new LocalMapRow((i / 0x10) * 0x10));
    }

    if (maLocalMapRows.size() > 0) {
        LocalMapRow* row = maLocalMapRows[0];
        int32 nRowStart = row->mnRowStart;
        int32 nRowEnd = nRowStart + 0xf;
        row->mnStart = 0;

        for (int i = 0; i < local_map_size; ++i) {
            int32 from = local_map[i].from;
            if (nRowEnd < from) {
                // next row which the value is belonging to
                for (int32 j = 0; j < maLocalMapRows.size(); ++j) {
                    row = maLocalMapRows[j];
                    nRowStart = row->mnRowStart;
                    nRowEnd = nRowStart + 0xf;
                    if (nRowStart <= from && from <= nRowEnd) {
                        row->mnStart = i;
                        break;
                    }
                }
            }
        }
    }
    maRange.SetRange(maLocalMapRows[0]->mnRowStart,
                     maLocalMapRows[maLocalMapRows.size() -1]->mnRowStart);
    _SetMode(LOCAL_MAP);
}

void CharacterList::JumpToLocalMap(int32 value)
{
    if (mnMode != LOCAL_MAP ||
        !(maRange.first <= value && value <= maRange.last)) {
        return;
    }
    // calculate row
    const int32 row = (value - maRange.first) / 0x10;
    const int32 column = value % 0x10;
    mnSelectedRow = row;
    mnSelectedColumn = column;
    JumpToRow(row);

}

void CharacterList::SetChars(std::vector<std::string> &v)
{
    maChars.clear();
    for (size_t i = 0; i < v.size(); ++i) {
        maChars.push_back(v[i]);
    }
    _SetMode(TABLE);
}

void CharacterList::ClearChars()
{
    maChars.clear();
    Invalidate();
}

void CharacterList::_ClearLocalMapRows()
{
    for (size_t i = 0; i < maLocalMapRows.size(); ++i) {
        delete maLocalMapRows[i];
    }
    maLocalMapRows.clear();
}

void CharacterList::_Reset()
{
    BRect rect = Bounds();
    mnColumns = (rect.Width() - mnRowTitleWidth) / mnColumnWidth;
    mnRows = rect.Height() / mnRowHeight;
    if (mnMode == TABLE) {
        mnColumnCount = mnColumns;
        if (mnColumnCount > 0) {
            mnRowCount = maChars.size() / mnColumnCount;
            //if (maChars.size() - mnRowCount * mnColumnCount > 0) {
                //mnRowCount += 1;
            //}
            //printf("count: %d, columns: %d, rows: %d\n", maChars.size(), mnColumnCount, mnRowCount);
        } else {
            mnRowCount = 0;
        }
    }

    // update scrollbar
    BScrollBar* pBar = ScrollBar(B_VERTICAL);
    if (pBar) {
        int32 rows = ItemCount();
        if (rows <= mnRows) {
            pBar->SetRange(1, 1);
            pBar->SetProportion(1.0f);
            mnStartRow = 0;
        } else {
            if (mnStartRow + mnRows > rows) {
                mnStartRow = rows - mnRows;
            }
            if (mbShowTitle &&
                mnRowHeight - (Bounds().Height() - mnRows * mnRowHeight) > 0) {
                rows += 1;
            }
            int32 ends = rows - mnRows + 1;
            pBar->SetRange(1, ends);
            pBar->SetProportion(1./(ends - 1));
            pBar->SetValue(mnStartRow + 1);
        }
    }
    BScrollBar* pHoriBar = ScrollBar(B_HORIZONTAL);
    if (pHoriBar) {
        int32 columns = mnColumnCount;
        if (columns <= mnColumns) {
            pHoriBar->SetRange(1, 1);
            pHoriBar->SetProportion(1.0f);
            mnStartColumn = 0;
        } else {
            if (mnStartColumn + mnColumns > columns) {
                mnStartColumn = columns - mnColumns;
            }
            if (mnColumnWidth - (Bounds().Width() - mnRowTitleWidth - columns * mnColumnWidth) > 0) {
                columns += 1;
            }
            int32 ends = columns - mnColumns + 1;
            pHoriBar->SetRange(1, ends);
            pHoriBar->SetProportion(1./(ends - 1));
            pHoriBar->SetValue(mnStartColumn + 1);
        }
    }
}

void CharacterList::_FontReset()
{
    const char* fullWidthSpace = "\xe3\x80\x80";

    BFont font;
    GetFont(&font);
    font.SetSize(mnFontSize);
    SetFont(&font);
    font_height height;
    font.GetHeight(&height);

    const float widthFullWidthSpace = StringWidth(fullWidthSpace);
    const float widthM = StringWidth("M");
    float width = widthFullWidthSpace;
    if (widthFullWidthSpace < widthM) {
        width = widthM * 2.;
    } else {
        width *= 1.8;
    }
    mnColumnWidth = floor(width);
    mnRowHeight = floor((height.ascent + height.descent) * 1.3);

    if (mbShowTitle) {
        mnRowTitleWidth = floor(StringWidth(
            mnMode == UNICODE_RANGE ? (maRange.last <= 0xffff ? "U+0000" : "U+00000") :
                            "0x00000") * 1.1);
        mnColumnTitleHeight = mnRowHeight;
    } else {
        mnRowTitleWidth = 0;
        mnColumnTitleHeight = 0;
    }

    // cache width of title characters of column
    char* titles[16];
    for (int32 i = 0; i < 16; ++i) {
        titles[i] = (char*)TITLE_CHARS[i];
    }
    int32 lengthArray[] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
    };
    GetStringWidths(titles, lengthArray, 16, mTitleWidthArray);
    for (int32 i = 0; i < 16; ++i) {
        mTitleWidthArray[i] /= 2;
    }
}

// Number of rows
int32 CharacterList::ItemCount() const
{
    switch (mnMode) {
        case UNICODE_RANGE:
            return (maRange.last - maRange.first) / 0x10 + 1;
            break;
        case LOCAL_MAP:
            return maLocalMapRows.size();
            break;
        case TABLE:
            return mnRowCount;
            break;
    }
    return 0;
}

void CharacterList::SetTableFontSize(int index)
{
    const float sizeArray[] = {32, 28, 20, 18, 16};
    if (0 <= index && index <= 4) {
        mnFontSize = sizeArray[index];
    } else {
        mnFontSize = 16.0f;
    }
    _FontReset();
    _Reset();
    Invalidate();
}

void CharacterList::SetTableFont(const BFont* font)
{
    BView::SetFont(font);
    _FontReset();
    _Reset();
    Invalidate();
}

static void UnicodeToUTF8(std::string &s, uint32 c)
{
    if (0x00 <= c && c <= 0x7F) {
        s += (char)c;
    } else if (0x80 <= c && c <= 0x7FF) {
        s += (char)(0xC0 | ((c & 0x7C0) >> 6));
        s += (char)(0x80 |  (c & 0x3F));
    } else if (0x800 <= c && c <= 0xFFFF) {
        s += (char)(0xE0 | ((c & 0xF000) >> 12));
        s += (char)(0x80 | ((c & 0xFC0) >> 6));
        s += (char)(0x80 |  (c & 0x3F));
    } else if (0x10000 <= c && c <= 0x1FFFFF) {
        s += (char)(0xF0000000 | ((c & 0x7000000) >> 18));
        s += (char)(0x80 | ((c & 0x3F0000) >> 12));
        s += (char)(0x80 | ((c & 0x3F00) >> 6));
        s += (char)(0x80 |  (c & 0x3F));
    }
}

void CharacterList::_CalculatePosition(int32* nStartColumn, int32* nStartRow,
                                       int32* nEndColumn, int32* nEndRow,
                                       float* dx, float* dy)
{
    const BRect bounds = Bounds();
    const int32 nRows = ItemCount();
    *nStartRow = mnStartRow;
    if (mnMode != TABLE) {
        *nEndRow = std::min(*nStartRow + mnRows, nRows);
    } else {
        *nEndRow = std::min(*nStartRow + mnRows, mnRowCount);
    }
    if ((*nStartRow + mnRows) > nRows && (mnRows < nRows)) {
        *nStartRow -= 1;
        *dy = mnRowHeight - (bounds.Height() - mnRows * mnRowHeight);
    }

    *nStartColumn = mnStartColumn;
    *nEndColumn = std::min(*nStartColumn + mnColumns, mnColumnCount);
    if ((*nStartColumn + mnColumns) > 16 && (mnColumns < mnColumnCount)) {
        *nStartColumn -= 1;
        *dx = mnColumnWidth - (bounds.Width() - mnRowTitleWidth - mnColumns * mnColumnWidth);
    }
}

std::string CharacterList::_ItemAt(int32 row, int32 column) const
{
    std::string s;
    switch (mnMode) {
        case UNICODE_RANGE:
            if (0 <= row && column >= 0) {
                uint32 c = maRange.first + row * 0x10 + column;
                UnicodeToUTF8(s, c);
            }
            break;
        case LOCAL_MAP:
            if (0 <= row && row < maLocalMapRows.size() &&
                0 <= column && column <= 0x10) {
                LocalMapRow* pRow = maLocalMapRows[row];
                if (pRow) {
                    if (pRow->mnStart < 0xffffff) {
                        const int32 nRowEnd = pRow->mnRowStart + 0xf;
                        for (int32 j = pRow->mnStart; j < pRow->mnStart + 0x10; ++j) {
                            const int32 from = maLocalMap[j].from;
                            if (from <= nRowEnd &&
                                maLocalMap[j].from % 0x10 == column) {
                                UnicodeToUTF8(s, maLocalMap[j].ucs2);
                                break;
                            } else if (from == 0) {
                                break;
                            }
                        }
                    }
                }
            }
            break;
        case TABLE:
        {
            const int32 nSelected = row * mnColumnCount + column;
            if (0 <= nSelected && nSelected < maChars.size()) {
                s += maChars[nSelected];
            }
            break;
        }
        default:
            break;
    }
    return s;
}

std::string CharacterList::Selection() const
{
    return _ItemAt(mnSelectedRow, mnSelectedColumn);
}

void CharacterList::Draw(BRect updateRect)
{
    switch (mnMode) {
        case UNICODE_RANGE:
            _DrawPalette(updateRect);
            break;
        case LOCAL_MAP:
            _DrawLocalMap(updateRect);
        case TABLE:
            _DrawTable(updateRect);
            break;
    }
}

void CharacterList::_DrawTable(BRect updateRect)
{
    if (maChars.empty()) {
        return;
    }
    const float delta = mnRowHeight;

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    const float halfWidth = mnColumnWidth / 2;
    int32 x = 0;
    BPoint pos(x, delta * 0.75 - dy);
    int32 nColumn = 0;

    const int32 nSelected = mnSelectedRow * mnColumnCount + mnSelectedColumn;
    const int32 nStartIndex = nStartRow * mnColumnCount;
    const int32 nEndIndex = std::min(nStartIndex +
            (nEndRow - nStartRow + 1) * mnColumnCount, (int)maChars.size());
    for (int32 i = nStartIndex; i < nEndIndex; ++i) {
        const float width = StringWidth(maChars[i].c_str(), maChars[i].size());
        pos.x = x + halfWidth - width / 2;
        if (i == nSelected) {
            rgb_color hc = HighColor();
            rgb_color lc = LowColor();
            SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
            SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));

            FillRect(BRect(x, pos.y - delta * 0.75 - dy,
                x + mnColumnWidth, pos.y + delta * 0.25 - dy), B_SOLID_LOW);
            DrawString(maChars[i].c_str(), pos);

            SetLowColor(lc);
            SetHighColor(hc);
        } else {
            DrawString(maChars[i].c_str(), pos);
        }
        x += mnColumnWidth;
        nColumn += 1;
        if (nColumn >= mnColumns) {
            nColumn = 0;
            x = 0;
            pos.y += mnRowHeight;
        }
    }

    // todo, grid
}

void CharacterList::_DrawLocalMap(BRect updateRect)
{
    if (maLocalMap == NULL) {
        return;
    }
    const float delta = mnRowHeight;

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    const float halfWidth = mnColumnWidth / 2;
    int32 x = mnRowTitleWidth;
    BPoint pos(x, delta * 1.75 - dy);

    std::string s;
    s.reserve(0x10 * 10 * 4);
    const char* pChar = s.c_str();
    size_t len = s.size();

    for (int32 i = nStartRow; i < nEndRow; ++i) {
        LocalMapRow* row = maLocalMapRows[i];
        if (row->mnStart < 0xffffff) {
            const int32 nRowEnd = row->mnRowStart + 0xf;
            for (int32 j = row->mnStart; j < row->mnStart + 0x1f; ++j) {
                const int32 from = maLocalMap[j].from;
                if (0 < from && from <= nRowEnd) {
                    UnicodeToUTF8(s, maLocalMap[j].ucs2);
                    size_t charLen = s.size() - len;
                    if (charLen <= 0) {
                        break;
                    }
                    const float width = StringWidth(pChar, charLen);
                    const int32 columnIndex = from % 0x10;
                    const int32 x = mnRowTitleWidth + columnIndex * mnColumnWidth;
                    pos.x = x + halfWidth - width / 2;

                    if (i == mnSelectedRow && columnIndex == mnSelectedColumn) {
                        rgb_color hc = HighColor();
                        rgb_color lc = LowColor();
                        SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
                        SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));

                        FillRect(BRect(x - dx, (i - nStartRow + 1) * mnRowHeight - dy,
                            x + mnColumnWidth - dx, 
                            (i - nStartRow + 2) * mnRowHeight - dy), B_SOLID_LOW);
                        DrawString(pChar, pos);

                        SetLowColor(lc);
                        SetHighColor(hc);
                    } else {
                        DrawString(pChar, pos);
                    }
                    pChar += charLen;
                    len = s.size();
                } else {
                    break;
                }
            }
        }
        x = mnRowTitleWidth;
        pos.y += mnRowHeight;
    }

    _DrawTitleAndGrid();
}

void CharacterList::_DrawPalette(BRect updateRect)
{
    const float delta = mnRowHeight;

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    const float halfWidth = mnColumnWidth / 2;
    int32 x = mnRowTitleWidth;
    BPoint pos(x, delta * 1.75 - dy);

    std::string s;
    s.reserve((nEndRow - nStartRow + 1) * 16 * 4);
    const char* pChar = s.c_str();
    size_t len = s.size();
    uint32 nCharValue = maRange.first + nStartRow * 0x10;
    for (int32 i = nStartRow; i < nEndRow; ++i) {
        for (int32 j = nStartColumn; j < nEndColumn; ++j) {
            UnicodeToUTF8(s, nCharValue);
            size_t charLen = s.size() - len;
            if (charLen <= 0) {
                break;
            }
            const float width = StringWidth(pChar, charLen);
            pos.x = x + halfWidth - width / 2;
            if (i == mnSelectedRow && j == mnSelectedColumn) {
                rgb_color hc = HighColor();
                rgb_color lc = LowColor();
                SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
                SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));

                FillRect(BRect(x - dx, (i - nStartRow + 1) * mnRowHeight - dy,
                    x + mnColumnWidth - dx, (i - nStartRow + 2) * mnRowHeight - dy), B_SOLID_LOW);
                DrawString(pChar, pos);

                SetLowColor(lc);
                SetHighColor(hc);
            } else {
                DrawString(pChar, pos);
            }
            x += mnColumnWidth;
            nCharValue += 1;
            pChar += charLen;
            len = s.size();
        }
        x = mnRowTitleWidth;
        pos.y += mnRowHeight;
    }

    _DrawTitleAndGrid();
}

void CharacterList::_DrawTitleAndGrid()
{
    float delta = mnRowHeight;
    BRect bounds = Bounds();

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    if (mbShowTitle) {
        rgb_color lowColor = LowColor();
        SetLowColor(ui_color(B_CONTROL_BACKGROUND_COLOR));

        // background for column header
        FillRect(BRect(0, 0, bounds.right, delta), B_SOLID_LOW);

        // column title
        BPoint pos(mnRowTitleWidth + mnColumnWidth * 0.5, delta * 0.75);
        float x = pos.x;
        for (int32 i = nStartColumn; i < nEndColumn; ++i) {
            pos.x -= mTitleWidthArray[i];
            DrawString(TITLE_CHARS[i], pos);

            x += mnColumnWidth;
            pos.x = x;
        }

        // background for row header
        FillRect(BRect(0, delta, mnRowTitleWidth, bounds.bottom), B_SOLID_LOW);

        const char* aRowLabel = mnMode == UNICODE_RANGE ? "U+%04X" : "0x%0X";
        // row title
        char aTitle[8];
        pos.x = mnRowTitleWidth * 0.05;
        pos.y = delta * 1.75 - dy;
        int32 nRowTitleValue = maRange.first + nStartRow * 0x10;
        for (int32 i = 0; i < nEndRow; ++i) {
            sprintf(aTitle, aRowLabel, nRowTitleValue);
            DrawString((const char*)aTitle, pos);

            pos.y += delta;
            nRowTitleValue += 0x10;
        }

        // fill top left corner to erase some text
        FillRect(BRect(0, 0, mnRowTitleWidth, delta), B_SOLID_LOW);

        SetLowColor(lowColor);
    }
    const rgb_color lineColor = ui_color(B_CONTROL_BORDER_COLOR);

    // todo, re calculate
    BeginLineArray(mnRows + 1 + 16);

    const int32 rowCount = mnMode != TABLE ? nEndRow : std::min(mnRows, mnRowCount);

    // column grid
    BPoint start(mnRowTitleWidth, 0);
    BPoint end(start.x, (rowCount + 1) * mnRowHeight);

    if (mbShowTitle) {
        AddLine(start, end, lineColor);
    }
    start.x += mnColumnWidth;
    end.x = start.x;

    const int32 columnCount = mnMode != TABLE ? 16 : mnColumnCount;
    for (int32 i = 1; i < columnCount; ++i) {
        AddLine(start, end, lineColor);

        start.x += mnColumnWidth;
        end.x = start.x;
    }
    // column right side border
    AddLine(start, end, lineColor);

    // row grid
    start = BPoint(0, delta);
    end = BPoint(mnRowTitleWidth + mnColumnWidth * columnCount, delta);
    if (mbShowTitle) {
        // border below the title
        AddLine(start, end, lineColor);
        start.y += delta - dy;
        end.y = start.y;
    }

    for (int32 i = 0; i < rowCount; ++i) {
        AddLine(start, end, lineColor);

        start.y += delta;
        end.y = start.y;
    }

    EndLineArray();
}

void CharacterList::MouseDown(BPoint p)
{
    MakeFocus(true);

    const int32 nOldSelectedRow = mnSelectedRow;
    const int32 nOldSelectedColumn = mnSelectedColumn;

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    int32 row = floor((p.y - mnRowHeight + dy) / mnRowHeight);
    int32 column = floor((p.x - mnRowTitleWidth + dx) / mnColumnWidth);

    if (mnMode != TABLE) {
        if (0 <= column && column < mnColumnCount &&
            0 <= row && row <= mnRows) {
            if (mnMode == UNICODE_RANGE) {
                uint32 c = maRange.first + row * 0x10 + column;
                if (maRange.last < c) {
                    return;
                }
            }
            mnSelectedColumn = column + nStartColumn;
            mnSelectedRow = row + nStartRow;
        }

        if (nOldSelectedColumn != mnSelectedColumn ||
            nOldSelectedRow != mnSelectedRow) {
            Invalidate();
            _SelectionChanged();
        }
    } else {
        if (p.x < mnColumnCount * mnColumnWidth &&
            p.y <= mnRowCount * mnRowHeight) {

            const int32 selected = (nStartRow + row + 1) * mnColumnCount + column;
            if (0 <= selected && selected < maChars.size()) {
                mnSelectedColumn = column;
                mnSelectedRow = nStartRow + row + 1;
                if (nOldSelectedColumn != mnSelectedColumn ||
                    nOldSelectedRow != mnSelectedRow) {
                    Invalidate();
                    _SelectionChanged();
                }
            }
        }
    }
}

void CharacterList::MouseMoved(BPoint p, uint32 code, const BMessage* message)
{
    if (code == B_EXITED_VIEW) {
        _OnMouseExit();
        return;
    }
    const int32 nOldOnOverRow = mnOnOverRow;
    const int32 nOldOnOverColumn = mnOnOverColumn;

    float dx = 0;
    float dy = 0;
    int32 nStartRow = 0;
    int32 nEndRow = 0;
    int32 nStartColumn = 0;
    int32 nEndColumn = 0;

    _CalculatePosition(&nStartColumn, &nStartRow, &nEndColumn, &nEndRow, &dx, &dy);

    int32 row = floor((p.y - mnRowHeight + dy) / mnRowHeight);
    int32 column = floor((p.x - mnRowTitleWidth + dx) / mnColumnWidth);

    if (mnMode == TABLE) {
        row += 1;
    }
    if (0 <= column && column < mnColumnCount &&
        0 <= row && row <= mnRows) {
        if (mnMode == UNICODE_RANGE) {
            uint32 c = maRange.first + row * 0x10 + column;
            if (maRange.last < c) {
                _OnMouseExit();
                return;
            }
        }
        mnOnOverRow = row + nStartRow;
        mnOnOverColumn = column + nStartColumn;
    } else {
        _OnMouseExit();
    }

    if (nOldOnOverRow != mnOnOverRow ||
        nOldOnOverColumn != mnOnOverColumn) {
        BPoint pos(mnRowTitleWidth + dx + mnColumnWidth * (mnOnOverColumn - nStartColumn),
                   mnColumnTitleHeight + dy + mnRowHeight * (mnOnOverRow + 1 - nStartRow));
        _OnOverChanged(pos);
    }
}

void CharacterList::_SelectionChanged()
{
    BMessage* msg = Message();
    if (msg) {
        BMessage* message = new BMessage(*msg);
        std::string s = Selection();
        message->AddString("char", s.c_str());
        Invoke(message);
    }
}

void CharacterList::_OnOverChanged(BPoint pos)
{
    if (mpOnOverMessage) {
        BMessage* message = new BMessage(*mpOnOverMessage);
        std::string s = _ItemAt(mnOnOverRow, mnOnOverColumn);
        if (s.size() > 0) {
            message->AddString("char", s.c_str());
            message->AddPoint("pos", pos);
        }
        Invoke(message);
    }
}

void CharacterList::_OnMouseExit()
{
    if (mpOnOverMessage) {
        BMessage* message = new BMessage(*mpOnOverMessage);
        Invoke(message);
    }
}

void CharacterList::MakeFocus(bool focus)
{
    if (mpScrollView) {
        mpScrollView->SetBorderHighlighted(focus);
    }
    BView::MakeFocus(focus);
}

void CharacterList::TargetedByScrollView(BScrollView* pScrollView)
{
    mpScrollView = pScrollView;
}

void CharacterList::JumpToRow(int32 row)
{
    BScrollBar* pBar = ScrollBar(B_VERTICAL);
    if (pBar) {
        pBar->SetValue(row + 1);
    }
}

void CharacterList::ScrollTo(BPoint p)
{
    // zero value will not come to here, start value is 1
    if (p.x >= 1) {
        mnStartColumn = static_cast<int32>(p.x - 1);
        Invalidate();
    }
    if (p.y >= 1) {
        mnStartRow = static_cast<int32>(p.y - 1);
        Invalidate();
    }
}

void CharacterList::FrameResized(float newWidth, float newHeight)
{
    _Reset();
    Invalidate();
}

void CharacterList::MessageReceived(BMessage* msg)
{
    if (msg->what == B_KEY_DOWN && ItemCount() > 0) {
        //const int32 key = msg->GetInt32("key", 0);
        const uint8 byte = static_cast<uint8>(msg->GetInt8("byte", 0));
        //const uint32 mod = static_cast<uint32>(msg->GetInt32("modifiers", 0));
        //const uint32 c = static_cast<uint32>(msg->GetInt32("raw_char", 0));
        bool bCheck = false;
        switch (byte) {
            case B_LEFT_ARROW:
                mnSelectedColumn = mnSelectedColumn > 0 ? mnSelectedColumn - 1 : 0;
                bCheck = true;
                break;
            case B_RIGHT_ARROW:
                if (mnMode != TABLE) {
                    mnSelectedColumn = mnSelectedColumn < (mnColumnCount - 1) ?
                                    mnSelectedColumn + 1 : (mnColumnCount -1);
                } else {
                    if (mnSelectedColumn + 1 < mnColumnCount &&
                        mnSelectedRow * mnColumnCount + mnSelectedColumn + 1 < maChars.size()) {
                        mnSelectedColumn += 1;
                    }
                }
                bCheck = true;
                break;
            case B_UP_ARROW:
                mnSelectedRow = mnSelectedRow > 0 ? mnSelectedRow - 1 : 0;
                bCheck = true;
                break;
            case B_DOWN_ARROW:
                if (mnMode != TABLE) {
                    mnSelectedRow = mnSelectedRow < ItemCount() - 2 ?
                                    mnSelectedRow + 1 : ItemCount() - 1;
                } else {
                    if (mnSelectedRow < ItemCount() - 1 &&
                        (mnSelectedRow + 1) * mnColumnCount + mnSelectedColumn < maChars.size()) {
                        mnSelectedRow += 1;
                    }
                }
                bCheck = true;
                break;
            case B_HOME:
                mnSelectedColumn = 0;
                bCheck = true;
                break;
            case B_END:
                if (mnMode != TABLE) {
                    mnSelectedColumn = mnColumnCount - 1;
                } else {
                    if (mnSelectedRow == ItemCount() - 1) {
                        if (maChars.size() % mnColumnCount == 0) {
                            mnSelectedColumn = mnColumnCount - 1;
                        } else {
                            mnSelectedColumn = (maChars.size() % mnColumnCount) - 1;
                        }
                    } else {
                        mnSelectedColumn = mnColumnCount - 1;
                    }
                }
                bCheck = true;
                break;
            case B_PAGE_UP:
                mnSelectedRow = mnSelectedRow > (mnRows - 1) ?
                                    mnSelectedRow - (mnRows - 1) : 0;
                bCheck = true;
                break;
            case B_PAGE_DOWN:
                if (mnMode != TABLE) {
                    mnSelectedRow = mnSelectedRow + (mnRows - 1) < ItemCount() - 2 ?
                                    mnSelectedRow + (mnRows - 1) : ItemCount() - 1;
                } else {
                    if (mnSelectedRow + (mnRows - 1) < ItemCount() - 1) {
                        mnSelectedRow += (mnRows - 1);
                    } else {
                        if (maChars.size() % mnColumnCount == 0 ||
                            (ItemCount() - 1) * mnColumnCount + mnSelectedColumn < maChars.size()) {
                            mnSelectedRow = ItemCount() - 1;
                        } else {
                            mnSelectedRow = ItemCount() - 2;
                        }
                    }
                }
                bCheck = true;
                break;
            case B_ENTER:
                _SelectionChanged();
                break;
            case B_SPACE:
                _SelectionChanged();
                break;
            default:
                break;
        }
        if (bCheck) {
            if (mnSelectedColumn < 0) {
                mnSelectedColumn = 0;
            }
            if (mnSelectedRow < 0) {
                mnSelectedRow = 0;
            }
            Invalidate();
        }
        return;
    }
    BView::MessageReceived(msg);
}

}; // haiku_gui
}; // mozc

