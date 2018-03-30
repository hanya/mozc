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

#ifndef CHARACTER_PAD_H
#define CHARACTER_PAD_H

#include <Invoker.h>
#include <View.h>

#include <string>
#include <vector>

class BScrollView;

namespace mozc {
namespace gui {

class CharacterPalette
{
public:
    struct UnicodeRange {
        uint32 first;
        uint32 last;
    };

    struct UnicodeBlock
    {
        const char* name;
        UnicodeRange range;
    };

    struct LocalCharacterMap
    {
        uint32 from;
        uint32 ucs2;
    };

    struct CP932JumpTo {
        const char* name;
        uint32 from;
    };
};

}; // gui
}; // mozc

namespace mozc {
namespace haiku_gui {

class KeyFilter;

class CharacterList : public BView, public BInvoker
{
public:
    typedef struct Range_
    {
        uint32 first;
        uint32 last;

        void SetRange(uint32 first_, uint32 last_) {
            first = first_;
            last = last_;
        }
    } Range;

    enum Mode
    {
        UNICODE_RANGE,
        LOCAL_MAP,
        TABLE,
    };

    class LocalMapRow
    {
    public:
        LocalMapRow(int32 rowStart) {
            mnRowStart = rowStart;
            mnStart = 0xffffff;
        };
        virtual ~LocalMapRow() {};

        int32  mnRowStart;
        size_t mnStart;
    };

    CharacterList(const char *name, Mode mode, uint32 flags,
                    BMessage* message, const BLooper* looper,
                    BMessage* onOverMessage);
    virtual ~CharacterList();

    virtual void MessageReceived(BMessage* msg);
    virtual void Draw(BRect updateRect);
    virtual void MakeFocus(bool focus);
    virtual void FrameResized(float newWidth, float newHeight);
    virtual void TargetedByScrollView(BScrollView* pScrollView);
    virtual void ScrollTo(BPoint p);
    virtual void MouseDown(BPoint p);
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* message);

    void SetRange(uint32 first, uint32 last);
    void SetLocalMap(const mozc::gui::CharacterPalette::LocalCharacterMap* local_map,
                size_t local_map_size);
    void SetChars(std::vector<std::string> &v);
    void ClearChars();
    void JumpToLocalMap(int32 value);
    void JumpToRow(int32 row);

    virtual int32 ItemCount() const;
    virtual std::string Selection() const;

    virtual void SetTableFontSize(int index);
    virtual void SetTableFont(const BFont* font);

private:
    Mode    mnMode;
    float   mnFontSize;
    float   mnColumnWidth;
    float   mnRowHeight;
    int32   mnColumns; // number of columns can be shown in the view
    int32   mnRows; // number of rows can be shown in the view
    int32   mnStartColumn;
    int32   mnStartRow;
    int32   mnColumnCount;
    int32   mnRowCount;
    int32   mnSelectedColumn;
    int32   mnSelectedRow;
    int32   mnOnOverColumn;
    int32   mnOnOverRow;
    float   mnColumnTitleHeight;
    float   mnRowTitleWidth;
    bool    mbShowTitle;

    Range    maRange; // UNICODE_RANGE
    const mozc::gui::CharacterPalette::LocalCharacterMap*    maLocalMap; // LOCAL_MAP
    size_t                mnLocalMapSize; // size for LOCAL_MAP
    std::vector<LocalMapRow*> maLocalMapRows; // LOCAL_MAP
    std::vector<std::string> maChars; // TABLE

    BScrollView*    mpScrollView;
    BLooper*        mpSelectionHandler;
    float           mTitleWidthArray[16];
    BMessage*       mpOnOverMessage;

    void _Reset();
    void _FontReset();
    void _SetMode(Mode mode);
    void _DrawPalette(BRect rect);
    void _DrawLocalMap(BRect rect);
    void _DrawTable(BRect rect);
    void _DrawTitleAndGrid();
    void _CalculatePosition(int32* nStartColumn, int32* nStartRow,
                            int32* nEndColumn, int32* nEndRow,
                            float* dx, float* dy);
    void _ClearLocalMapRows();
    void _SelectionChanged();
    void _OnOverChanged(BPoint pos);
    void _OnMouseExit();
    std::string _ItemAt(int32 row, int32 column) const;
};

}; // haiku_gui
}; // mozc

#endif // CHARACTER_PAD_H
