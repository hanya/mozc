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

#ifndef GRID_VIEW_H
#define GRID_VIEW_H

#include "haiku/haiku_gui/base/key_filter.h"

#include <ControlLook.h>
#include <Cursor.h>
#include <Input.h>
#include <LayoutUtils.h>
#include <Looper.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextView.h>
#include <ScrollView.h>
#include <String.h>
#include <View.h>

#include <algorithm>
#include <vector>
#include <string>

class BHandler;
class BPopUpMenu;
class BScrollView;

namespace mozc {
namespace haiku_gui {

enum {
    GRID_SORT = 'grsr',
};


class GridTitleView : public BView
{
public:
    static constexpr float MIN_WIDTH = 35.0f;

    GridTitleView(bool bEnableResizing=true, BHandler* handler=NULL);
    virtual ~GridTitleView();

    virtual void Draw(BRect rect);
    virtual void MouseDown(BPoint p);
    virtual void MouseUp(BPoint p);
    virtual void MouseMoved(BPoint p, uint32 code, const BMessage* message);

    void SetWidth(float width);
    float Height() const { return mnHeight; }
    float ColumnWidth(size_t n) const;
    void AddColumn(const char* label);
    int32 CountItems() const;
    float ColumnLeft(int32 column) const;
    float TotalWidth(int32 nStart, int32 nEnd) const;
protected:
    std::vector<std::string> maTitles;
    std::vector<float> mnTitleWidths;
    std::vector<float> mnTitleTextHalfWidth;
    float mnHeight;
    bool mbEnableResizing;
    bool mbResizing;
    float mnStartX;
    int32 mnResizeColumn;
    BHandler* mpHandler;

    void _SetCursor(BCursorID id);
    void _CheckLastColumnWidth();
    inline bool _IsValidColumnIndex(int32 index) const {
        return 0 <= index && index < maTitles.size(); }
    void _SortRequest(int32 index);
};


class GridRow
{
public:
    GridRow();
    virtual ~GridRow();

    bool IsSelected() const { return mbSelected; }
    void Select() { mbSelected = true; }
    void Deselect() { mbSelected = false; }

    typedef bool (*GridRowComp)(GridRow *r1, GridRow *r2);
    static GridRowComp GetComp(int32 index) { return NULL; }
protected:
    bool mbSelected;
};


class InputTextView : public BTextView
{
public:
    enum {
        APPLY = 'appl',
        UPDATE = 'updt',
    };
    InputTextView(const char* name);
    virtual ~InputTextView();

    virtual void KeyDown(const char* bytes, int32 num);
    virtual void FrameResized(float width, float height);
    virtual void MakeFocus(bool focus);
    virtual void Paste(BClipboard* clipboard);

    void EditDone();
    bool IsModified() const { return mbModified; }
    void SetModified(bool state);
    void EnableMonitorModification();
    void DisableMonitorModification();
protected:
    virtual void InsertText(const char* text, int32 length, int32 offset,
                            const text_run_array* runs);
    void DeleteText(int32 fromOffset, int32 toOffset);
private:
    bool mbModified;
    bool mbMonitorModified;
    void _SendKeyDown(const char* byte);
    void _RequestToApply();
    void _SendUpdate();
};


// Container to help to fill a hole cell.
class ListInputView : public BView
{
public:
    ListInputView(InputTextView* p);
    virtual ~ListInputView();
private:
    InputTextView* mpInputTextView;
};


template<typename T>
class GridColumn
{
public:
    enum {
        MODIFIED = 'modf',
    };
    GridColumn(BView* view, int32 column, BHandler* handler);
    virtual ~GridColumn() {};

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow) {}
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow) {}
    virtual bool Apply() { return false; }
protected:
    int32 mnColumn;
    T* mpRow;
    BHandler* mpHandler;

    virtual void _Modified();
};

template<typename T>
GridColumn<T>::GridColumn(BView* view, int32 column, BHandler* handler)
    : mnColumn(column),
      mpRow(NULL),
      mpHandler(handler)
{
}

template<typename T>
void GridColumn<T>::_Modified()
{
    if (mpHandler) {
        BMessage msg(MODIFIED);
        msg.AddInt32("column", mnColumn);
        mpHandler->MessageReceived(&msg);
    }
}


template<typename T>
class GridStringColumn : public GridColumn<T>
{
public:
    GridStringColumn(BView* view, int32 column);
    virtual ~GridStringColumn() {}

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow) {}
    virtual bool Apply() { return false; }
protected:
    virtual const char* _GetLabel(T* pRow) { return ""; }
};


template<typename T>
GridStringColumn<T>::GridStringColumn(BView* view, int32 column)
    : GridColumn<T>(view, column, NULL)
{
}

template<typename T>
void GridStringColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    const char* label = _GetLabel(pRow);
    BPoint pos(rect.left + 5, rect.top + rect.Height() * 0.75);
    if (selected) {
        view->DrawString(label, pos); // todo
    } else {
        view->DrawString(label, pos);
    }
}


template<typename T>
class GridTextColumn : public GridColumn<T>
{
public:
    GridTextColumn(BView* view, int32 column, BHandler* handler, ListInputView* p);
    virtual ~GridTextColumn();

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow);
    virtual bool Apply();
protected:
    InputTextView* mpTextView;
    ListInputView* mpInputView;

    virtual const char* _GetLabel(T* pRow);
    virtual void _SetText(const char* s);
};


template<typename T>
GridTextColumn<T>::GridTextColumn(BView* view, int32 column,
                                BHandler* handler, ListInputView* p)
    : GridColumn<T>(view, column, handler),
      mpInputView(p)
{
    mpTextView = static_cast<InputTextView*>(mpInputView->ChildAt(0));
}

template<typename T>
GridTextColumn<T>::~GridTextColumn()
{
    mpTextView = NULL;
    mpInputView = NULL;
}

template<typename T>
bool GridTextColumn<T>::Apply()
{
    if (this->mpRow && mpTextView->IsModified()) {
        _SetText(mpTextView->Text());
        this->mpRow = NULL;
        mpTextView->EditDone();
        if (!mpInputView->IsHidden()) {
            mpInputView->Hide();
        }
        return true;
    }
    return false;
}

template<typename T>
void GridTextColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    const char* label = _GetLabel(pRow);
    BPoint pos(rect.left + 5, rect.top + floor(rect.Height() * 0.75));
    BString s(label);
    view->TruncateString(&s, B_TRUNCATE_END, rect.Width() - 10);
    view->DrawString(s.String(), pos);
}

template<typename T>
void GridTextColumn<T>::Edit(BView* view, BRect rect, BPoint pos, T* pRow)
{
    this->mpRow = pRow;
    if (mpInputView && mpTextView) {
        BRect r(rect);
        r.InsetBy(1, 1);
        mpInputView->MoveTo(r.LeftTop());
        mpInputView->ResizeTo(r.Width(), r.Height());
        if (mpInputView->IsHidden()) {
            mpInputView->Show();
        }

        const char* label = _GetLabel(pRow);
        rect.InsetBy(4, 1);
        mpTextView->DisableMonitorModification();
        mpTextView->SetText(label);
        mpTextView->EnableMonitorModification();
        mpTextView->MoveTo(BPoint(4, 0));
        mpTextView->ResizeTo(rect.Width(), rect.Height());
        if (mpTextView->IsHidden()) {
            mpTextView->Show();
        }
        mpTextView->MakeFocus(true);
    }
}

template<typename T>
const char* GridTextColumn<T>::_GetLabel(T* pRow)
{
    return "";
}

template<typename T>
void GridTextColumn<T>::_SetText(const char* s)
{
}


template<typename T>
class GridButtonFieldColumn : public GridColumn<T>
{
public:
    enum {
        UPDATE = 'updt',
    };
    GridButtonFieldColumn(BView* view, int32 column, BHandler* handler);
    virtual ~GridButtonFieldColumn() {}

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow);
protected:
    virtual const char* _Label(T* pRow);
    virtual void _DrawButton(BView* view, BRect rect, BPoint pos, const char* label);
};

template<typename T>
GridButtonFieldColumn<T>::GridButtonFieldColumn(BView* view, int32 column, BHandler* handler)
    : GridColumn<T>(view, column, handler)
{
}

template<typename T>
void GridButtonFieldColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    const char* label = _Label(pRow);
    BPoint pos(rect.left + 5, rect.top + rect.Height() * 0.75);
    if (selected) {
        _DrawButton(view, rect, pos, label);
    } else {
        view->DrawString(label, pos);
    }
}

template<typename T>
void GridButtonFieldColumn<T>::Edit(BView* view, BRect rect, BPoint pos, T* pRow)
{
}

template<typename T>
const char* GridButtonFieldColumn<T>::_Label(T* pRow)
{
    return "";
}


template<typename T>
void GridButtonFieldColumn<T>::_DrawButton(BView* view, BRect rect, BPoint pos, const char* label)
{
    be_control_look->DrawButtonBackground(
        view, rect, view->Bounds(), ui_color(B_CONTROL_BACKGROUND_COLOR));

    const rgb_color cc = view->LowColor();
    view->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
    view->SetLowColor(ui_color(B_CONTROL_BACKGROUND_COLOR));

    view->DrawString(label, pos);

    view->SetLowColor(cc);
}


template<typename T>
class GridMenuFieldColumn : public GridColumn<T>
{
public:
    enum {
        UPDATE = 'updt',
    };
    GridMenuFieldColumn(BView* view, int32 column, BHandler* handler);
    virtual ~GridMenuFieldColumn();

    virtual void DrawAt(BView* view, BRect rect, bool selected, T* pRow);
    virtual void Edit(BView* view, BRect rect, BPoint pos, T* pRow);

    const char* Label(int32 n);
    virtual void AddLabel(const char* label);

    int32 CountLabels() const { return maLabels.size(); }
protected:
    BPopUpMenu* mpPopUpMenu;
    std::vector<std::string> maLabels;

    void _DrawMenuField(BView* view, BRect rect, BPoint pos, const char* label);

    virtual bool _UpdatePopUp(T* pRow);
    virtual void _UpdateRow(T* pRow, BMessage* msg);
    virtual int32 _LabelIndex(T* pRow);
    virtual int32 _CurrentIndex(T* pRow);
};


template<typename T>
GridMenuFieldColumn<T>::GridMenuFieldColumn(BView* view, int32 column, BHandler* handler)
    : GridColumn<T>(view, column, handler)
{
    mpPopUpMenu = new BPopUpMenu("pos");
    mpPopUpMenu->SetRadioMode(true);
}

template<typename T>
GridMenuFieldColumn<T>::~GridMenuFieldColumn()
{
    delete mpPopUpMenu;
    mpPopUpMenu = NULL;
}

template<typename T>
const char* GridMenuFieldColumn<T>::Label(int32 n)
{
    if (0 <= n && n < CountLabels()) {
        return maLabels[n].c_str();
    }
    return "";
}

template<typename T>
void GridMenuFieldColumn<T>::AddLabel(const char* label)
{
    maLabels.push_back(std::string(label));
    mpPopUpMenu->AddItem(new BMenuItem(label, new BMessage(CountLabels() - 1)));
}

template<typename T>
bool GridMenuFieldColumn<T>::_UpdatePopUp(T* pRow)
{
    const int32 index = _CurrentIndex(pRow);
    const int32 selected = mpPopUpMenu->FindMarkedIndex();
    if (index != selected) {
        if (0 <= index && index < CountLabels()) {
            BMenuItem* item = mpPopUpMenu->ItemAt(index);
            if (item) {
                item->SetMarked(true);
            }
        } else if (index == 0) {
            // select first
            BMenuItem* item = mpPopUpMenu->ItemAt(0);
            if (item) {
                item->SetMarked(true);
            }
        }
    }
    return true;
}

template<typename T>
void GridMenuFieldColumn<T>::_UpdateRow(T* pRow, BMessage* msg)
{
}

template<typename T>
int32 GridMenuFieldColumn<T>::_LabelIndex(T* pRow)
{
    return -1;
}

template<typename T>
int32 GridMenuFieldColumn<T>::_CurrentIndex(T* pRow)
{
    return -1;
}

template<typename T>
void GridMenuFieldColumn<T>::DrawAt(BView* view, BRect rect, bool selected, T* pRow)
{
    const int32 index = _LabelIndex(pRow);
    if (0 <= index && index <= CountLabels()) {
        const char* label = maLabels[index].c_str();
        BPoint pos(rect.left + 5, rect.top + rect.Height() * 0.75);
        if (selected) {
            _DrawMenuField(view, rect, pos, label);
        } else {
            view->DrawString(label, pos);
        }
    }
}

template<typename T>
void GridMenuFieldColumn<T>::Edit(BView* view, BRect rect, BPoint pos, T* pRow)
{
    if (!_UpdatePopUp(pRow)) {
        return;
    }
    this->mpRow = pRow;
    BMenuItem* item = mpPopUpMenu->Go(pos, false, false);
    if (item) {
        BMessage* msg = item->Message();
        if (msg) {
            _UpdateRow(pRow, msg);
        }
    }
}

template<typename T>
void GridMenuFieldColumn<T>::_DrawMenuField(BView* view, BRect rect, BPoint pos, const char* label)
{
    be_control_look->DrawMenuFieldBackground(
        view, rect, view->Bounds(), ui_color(B_MENU_BACKGROUND_COLOR), true, 0);

    const rgb_color cc = view->LowColor();
    view->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
    view->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));

    view->DrawString(label, pos);

    view->SetLowColor(cc);
}


enum SelectionType {
    SINGLE,
    MULTIPLE,
};


template<typename T>
class GridView : public BView
{
public:
    enum {
        MODIFIED = 'modf',
    };
    enum SortType {
        NO_SORT,
        NOT_SORTED,
        ASCENDING,
        DESCENDING,
    };
    GridView(SelectionType type, BLooper* looper=NULL, BMessage* menuMessage=NULL);
    virtual ~GridView();

    virtual void Draw(BRect rect);
    virtual void MouseDown(BPoint p);
    virtual BSize MinSize();
    virtual void FrameResized(float newWidth, float newHeight);
    virtual void ScrollTo(BPoint p);
    virtual void TargetedByScrollView(BScrollView* pScrollView);
    virtual void MakeFocus(bool focus);
    virtual void MessageReceived(BMessage* msg);

    void AddRow(T* pRow);
    void InsertRow(int32 index, T* pRow);
    void RemoveRow(int32 index);
    void MakeEmpty();
    bool IsEmpty() const { return mpRows.empty(); }
    int32 CountRows() const { return mpRows.size(); }
    void DeselectAll();
    T* ItemAt(int32 index) const;

    void SetModified(bool modified);
    void ShowItem(int32 index);
    void EditItem(int32 index, int32 column);
    void StopEditing();
    int32 CurrentSelection() const;
    std::vector<T*> SelectedItems() const;
    std::vector<int32> SelectedIndex() const;
    void CursorIndex(int32* row, int32* column) const;
    void SetCursorIndex(int32 row, int32 column);

    GridTitleView* TitleView() { return mpTitleView; }
    void SetTitleView(GridTitleView* view);
    void AddColumn(GridColumn<T>* column, bool sort=false);
    ListInputView* InputView() { return mpInputView; }

    void LockUpdate() { mbUpdateLock = true; }
    void UnlockUpdate();
    void SetTarget(BLooper* looper) { mpLooper = looper; }
    
    typedef bool (*RowComp)(T *r1, T *r2);
protected:
    BScrollView*        mpScrollView;
    GridTitleView*      mpTitleView;
    InputTextView*      mpInputText;
    ListInputView*      mpInputView;
    std::vector<GridColumn<T>*> mpColumns;
    std::vector<T*>     mpRows;
    float   mnRowHeight;
    int32   mnRows;
    int32   mnFirstRow;
    int32   mnCursorRow;
    int32   mnCursorColumn;
    SelectionType mnSelectionType;
    bool    mbSingleSelection;
    bool    mbRangeSelectionMode;
    int32   mnSingleSelectedRow;
    int32   mnRangeStartRow;
    bool    mbModified;
    bool    mbUpdateLock;
    BLooper* mpLooper;
    BMessage* mpMenuRequestMessage;
    std::vector<SortType> mnSortType;

    void _Reset();
    void _Clear();
    void _Edit();
    float _GetColumnLeft(int32 column) const;
    void _Key(BMessage* msg);
    BRect _CursorRect() const;
    void _SelectRow(int32 row);
    void _DeselectRow(int32 row);
    void _SelectRows(std::vector<int32> &rows);
    void _DeselectRows(std::vector<int32> &rows);
    void _DeselectAll();
    void _MoveCursor(int32 dx, int32 dy, uint32 mod=0);
    void _MoveToNextCell(bool forward);
    void _SetVerticalScrollValue(int32 value);
    void _DrawGrid();
    void _DrawCursor();
    inline bool _IsCursorValid() const { return 0 <= mnCursorRow && mnCursorRow < mpRows.size(); }
    inline bool _IsRowValid(int32 row) const { return 0 <= row && row < mpRows.size(); }
    void _Sort(int32 index);
};


template<typename T>
GridView<T>::GridView(SelectionType type, BLooper* looper, BMessage* menuMessage)
    : BView(
        BRect(0, 0, 10, 10),
        "dictlist", B_FOLLOW_ALL_SIDES,
        B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS | B_NAVIGABLE |
        B_INPUT_METHOD_AWARE),
      mpScrollView(NULL),
      mpTitleView(NULL),
      mnFirstRow(0),
      mnCursorRow(-1),
      mnCursorColumn(-1),
      mnSelectionType(type),
      mbSingleSelection(true),
      mbRangeSelectionMode(false),
      mnSingleSelectedRow(-1),
      mnRangeStartRow(-1),
      mbModified(false),
      mbUpdateLock(false),
      mpLooper(looper),
      mpMenuRequestMessage(menuMessage)
{
    mpInputText = new InputTextView("input");
    mpInputView = new ListInputView(mpInputText);
    AddChild(mpInputView);
    mpInputView->Hide();

    font_height height;
    GetFontHeight(&height);
    mnRowHeight = ceil((height.ascent + height.descent) * 1.2);
    AddFilter(new KeyFilter(this));
}


template<typename T>
GridView<T>::~GridView()
{
    _Clear();
    const size_t nColumns = mpColumns.size();
    for (size_t i = 0; i < nColumns; ++i) {
        delete mpColumns[i];
    }
    mpColumns.clear();
}

template<typename T>
void GridView<T>::CursorIndex(int32* row, int32* column) const
{
    *row = mnCursorRow;
    *column = mnCursorColumn;
}

template<typename T>
void GridView<T>::SetCursorIndex(int32 row, int32 column)
{
    mnCursorRow = row;
    mnCursorColumn = column;

    if (LockLooper()) {
        Invalidate();
        UnlockLooper();
    }
}

template<typename T>
void GridView<T>::SetTitleView(GridTitleView* view)
{
    if (!mpTitleView) {
        mpTitleView = view;
        AddChild(mpTitleView);
    }
}

template<typename T>
void GridView<T>::AddColumn(GridColumn<T>* column, bool sort)
{
    mpColumns.push_back(column);
    mnSortType.push_back(sort ? NOT_SORTED : NO_SORT);
}

template<typename T>
void GridView<T>::_Clear()
{
    const size_t nRowCount = mpRows.size();
    for (size_t i = 0; i < nRowCount; ++i) {
        delete mpRows[i];
    }
    mpRows.clear();
}

template<typename T>
void GridView<T>::UnlockUpdate()
{
    mbUpdateLock = false;
    _Reset();
    Invalidate();
}

template<typename T>
void GridView<T>::MakeEmpty()
{
    int32 len = mpRows.size();
    _Clear();
    _Reset();

    mnFirstRow = 0;
    mnCursorRow = -1;
    mnCursorColumn = -1;
    mbSingleSelection = true;
    mbRangeSelectionMode = false;
    mbModified = mbModified || len > 0;
    Invalidate();
}

template<typename T>
void GridView<T>::SetModified(bool modified)
{
    mbModified = modified;
}

template<typename T>
void GridView<T>::_Reset()
{
    BScrollBar* pBar = ScrollBar(B_VERTICAL);
    if (pBar) {
        int32 rows = CountRows();
        if (rows <= mnRows) {
            pBar->SetRange(1, 1);
            pBar->SetProportion(1);
        } else {
            pBar->SetRange(1, rows - mnRows + 1);
            pBar->SetProportion(0.2);
        }
    }
}

template<typename T>
void GridView<T>::_SetVerticalScrollValue(int32 value)
{
    BScrollBar* pBar = ScrollBar(B_VERTICAL);
    if (pBar) {
        if (pBar->LockLooper()) {
            pBar->SetValue(value);
            pBar->UnlockLooper();
        }
    }
}

template<typename T>
void GridView<T>::FrameResized(float newWidth, float newHeight)
{
    mpTitleView->SetWidth(newWidth);
    mnRows = ((Bounds().Height() - mpTitleView->Height() ) / mnRowHeight);
    _Reset();
    Invalidate();
}

template<typename T>
BSize GridView<T>::MinSize()
{
    return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(50, mnRowHeight * 5));
}

template<typename T>
void GridView<T>::MakeFocus(bool focus)
{
    if (mpScrollView) {
        mpScrollView->SetBorderHighlighted(focus);
    }
    BView::MakeFocus(focus);
}

template<typename T>
void GridView<T>::ScrollTo(BPoint p)
{
    if (p.y > 0) {
        mnFirstRow = static_cast<int32>(p.y - 1);
    }
    /*
    if (p.x > 0) {
        //mnFirstColumn = static_cast<int32(p.x - 1);
    }
    */
    Invalidate();

    if (!mpInputView->IsHidden()) {
        // todo, check the input field is shown
        BRect rect = _CursorRect();
        rect.InsetBy(1, 1);
        mpInputView->MoveTo(rect.LeftTop());
    }
}

template<typename T>
void GridView<T>::TargetedByScrollView(BScrollView* pScrollView)
{
    mpScrollView = pScrollView;
}

template<typename T>
void GridView<T>::DeselectAll()
{
    _DeselectAll();
    Invalidate();
}

template<typename T>
void GridView<T>::_SelectRow(int32 row)
{
    if (0 <= row && row < mpRows.size()) {
        mpRows[row]->Select();
        if (mnSingleSelectedRow == -1) {
            mnSingleSelectedRow = row;
        } else if (mnSingleSelectedRow != row) {
            mnSingleSelectedRow = -1;
        }
    }
}

template<typename T>
void GridView<T>::_DeselectRow(int32 row)
{
    if (0 <= row && row < mpRows.size()) {
        mpRows[row]->Deselect();
        if (mnSingleSelectedRow == row) {
            mnSingleSelectedRow = -1;
        }
    }
}

template<typename T>
void GridView<T>::_SelectRows(std::vector<int32> &rows)
{
    const int32 nRowCount = mpRows.size();
    const int32 count = rows.size();
    for (int32 i = 0; i < count; ++i) {
        const int32 row = rows[i];
        if (0 <= row && row < nRowCount) {
            mpRows[row]->Select();
        }
    }
    if (nRowCount == 1) {
        if (mnSingleSelectedRow == -1) {
            mnSingleSelectedRow = rows[0];
        } else if (rows[0] != mnSingleSelectedRow) {
            mnSingleSelectedRow = -1;
        }
    } else if (nRowCount > 1) {
        mnSingleSelectedRow = -1;
    }
}

template<typename T>
void GridView<T>::_DeselectRows(std::vector<int32> &rows)
{
    const int32 nRowCount = mpRows.size();
    const int32 count = rows.size();
    for (int32 i = 0; i < count; ++i) {
        const int32 row = rows[i];
        if (0 <= row && row < nRowCount) {
            mpRows[row]->Deselect();
        }
    }
}

template<typename T>
void GridView<T>::_DeselectAll()
{
    const int32 nRowCount = mpRows.size();
    for (int32 i = 0; i < nRowCount; ++i) {
        mpRows[i]->Deselect();
    }
    mnSingleSelectedRow = -1;
}

template<typename T>
int32 GridView<T>::CurrentSelection() const
{
    if (mnSelectionType == SINGLE) {
        if (mnSingleSelectedRow >= 0) {
            return mnSingleSelectedRow;
        } else {
            return mnCursorRow;
        }
    } else {
        // first selected row if exists
        if (mnSingleSelectedRow >= 0) {
            return mnSingleSelectedRow;
        }
        return mnCursorRow;
    }
}

template<typename T>
std::vector<T*> GridView<T>::SelectedItems() const
{
    std::vector<T*> v;
    if (mnSelectionType == SINGLE) {
        if (_IsRowValid(mnSingleSelectedRow)) {
            v.push_back(mpRows[mnSingleSelectedRow]);
        }
    } else {
        const int32 nRowCount = mpRows.size();
        for (int32 i = 0; i < nRowCount; ++i) {
            if (mpRows[i]->IsSelected()) {
                v.push_back(mpRows[i]);
            }
        }
    }
    return v;
}

template<typename T>
std::vector<int32> GridView<T>::SelectedIndex() const
{
    std::vector<int32> v;
    if (mnSelectionType == SINGLE) {
        if (_IsCursorValid()) {
            v.push_back(mnCursorRow);
        }
    } else {
        const int32 nRowCount = mpRows.size();
        for (int32 i = 0; i < nRowCount; ++i) {
            if (mpRows[i]->IsSelected()) {
                v.push_back(i);
            }
        }
    }
    return v;
}

template<typename T>
void GridView<T>::AddRow(T* pRow)
{
    if (pRow) {
        mpRows.push_back(pRow);
        mbModified = true;
        if (!mbUpdateLock) {
            _Reset();
            Invalidate();
        }
    }
}

template<typename T>
void GridView<T>::InsertRow(int32 index, T* pRow)
{
    if (pRow) {
        mpRows.insert(mpRows.begin() + index, pRow);
        mbModified = true;
        if (!mbUpdateLock) {
            _Reset();
            Invalidate();
        }
    }
}

template<typename T>
void GridView<T>::RemoveRow(int32 index)
{
    if (0 <= index && index < mpRows.size()) {
        T* row = mpRows[index];
        mpRows.erase(mpRows.begin() + index);
        delete row;
        mbModified = true;
        // todo, update cursor
        if (mnCursorRow >= mpRows.size() - 1) {
            mnCursorRow = mpRows.size() - 1;
        }
        if (mnSingleSelectedRow == index) {
            mnSingleSelectedRow = -1;
        } else if (mnSingleSelectedRow >= mpRows.size() - 1) {
            mnSingleSelectedRow = mpRows.size() - 1;
        }
        if (!mbUpdateLock) {
            _Reset();
            Invalidate();
        }
    }
}

template<typename T>
T* GridView<T>::ItemAt(int32 index) const
{
    return (0 <= index && index < mpRows.size()) ? mpRows[index] : NULL;
}

template<typename T>
void GridView<T>::Draw(BRect rect)
{
    const BRect bounds = Bounds();
    const float nRowHeight = mnRowHeight;
    const float nTitleHeight = mpTitleView->Height();

    int32 nLastRow = std::min(mnFirstRow + mnRows + 1,
                              static_cast<int32>(mpRows.size()));

    SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
    // draw content
    BPoint pos(0, nTitleHeight + ceil(nRowHeight * 0.75));
    BRect r0(0, nTitleHeight, mpTitleView->ColumnWidth(0), nTitleHeight + nRowHeight);
    BRect r1(r0.right, nTitleHeight, r0.right + mpTitleView->ColumnWidth(1), r0.bottom);
    BRect r2(r1.right, nTitleHeight, r1.right + mpTitleView->ColumnWidth(2), r0.bottom);
    BRect r3(r2.right, nTitleHeight, bounds.Width(), r0.bottom);
    T* pCursorRow = _IsCursorValid() ? mpRows[mnCursorRow] : NULL;
    for (size_t i = mnFirstRow; i < nLastRow; ++i) {
        T* pRow = mpRows[i];
        if (pRow->IsSelected()) {
            SetLowColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
            FillRect(BRect(0, r0.top, r3.right, r0.bottom), B_SOLID_LOW);
            SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
        }
        const bool bSelectedRow = pRow == pCursorRow;

        mpColumns[0]->DrawAt(this, r0, bSelectedRow && mnCursorColumn == 0, pRow);
        r0.top = r0.bottom;
        r0.bottom += nRowHeight;
        pos.x += mpTitleView->ColumnWidth(0);
        // todo, clip region

        mpColumns[1]->DrawAt(this, r1, bSelectedRow && mnCursorColumn == 1, pRow);
        r1.top = r0.top;
        r1.bottom = r0.bottom;
        pos.x += mpTitleView->ColumnWidth(1);

        mpColumns[2]->DrawAt(this, r2, bSelectedRow && mnCursorColumn == 2, pRow);
        r2.top = r0.top;
        r2.bottom = r0.bottom;
        pos.x += mpTitleView->ColumnWidth(2);

        mpColumns[3]->DrawAt(this, r3, bSelectedRow && mnCursorColumn == 3, pRow);
        r3.top = r0.top;
        r3.bottom = r0.bottom;

        pos.x = 0;
        pos.y += nRowHeight;
        if (pRow->IsSelected()) {
            SetLowUIColor(B_LIST_BACKGROUND_COLOR);
            SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);
        }
    }

    _DrawGrid();
    _DrawCursor();
}

template<typename T>
void GridView<T>::_DrawCursor()
{
    const int32 nLastRow = std::min(mnFirstRow + mnRows + 1,
                              static_cast<int32>(mpRows.size()));
    if (mnFirstRow <= mnCursorRow && mnCursorRow <= nLastRow) {
        BRect rect = _CursorRect();
        SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
        StrokeRect(rect, B_SOLID_HIGH);
    }
}

template<typename T>
void GridView<T>::_DrawGrid()
{
    SetHighUIColor(B_CONTROL_BORDER_COLOR);
    const int32 nLastRow = std::min(mnFirstRow + mnRows + 1,
                              static_cast<int32>(mpRows.size()));
    // horizontal
    BPoint start(0, mpTitleView->Height() + mnRowHeight);
    BPoint end(Bounds().Width(), start.y);
    for (size_t i = mnFirstRow; i < nLastRow; ++i) {
        StrokeLine(start, end);
        start.y += mnRowHeight;
        end.y = start.y;
    }
    // vertical
    const int32 nColumnCount = mpColumns.size();
    start = BPoint(mpTitleView->ColumnWidth(0), mpTitleView->Height());
    end = BPoint(start.x, start.y + (nLastRow - mnFirstRow) * mnRowHeight);
    for (size_t i = 1; i < nColumnCount; ++i) {
        StrokeLine(start, end);
        start.x += mpTitleView->ColumnWidth(i);
        end.x = start.x;
    }
}

template<typename T>
BRect GridView<T>::_CursorRect() const
{
    // todo, check
    BRect rect(_GetColumnLeft(mnCursorColumn),
               mpTitleView->Height() + (mnCursorRow - mnFirstRow) * mnRowHeight,
               0, 0);
    rect.right = rect.left + mpTitleView->ColumnWidth(mnCursorColumn);
    rect.bottom = rect.top + mnRowHeight;
    return rect;
}

template<typename T>
void GridView<T>::ShowItem(int32 row)
{
    if (row < 0 || row > CountRows()) {
        return;
    }
    int32 n = -1;
    const int32 nLastRow = mnFirstRow + mnRows;
    if (row < mnFirstRow) {
        // show the row at the top
        n = row;
    } else if (nLastRow - 1 < row) {
        // show the row at the bottom
        n = std::min(row - mnRows + 1, static_cast<int32>(CountRows()) - mnRows + 1);
    }
    if (n != -1) {
        _SetVerticalScrollValue(n + 1);
    }
}

template<typename T>
void GridView<T>::EditItem(int32 index, int32 column)
{
    // move cursor to the item
    if (0 <= index && index < mpRows.size() &&
        0 <= column && column < mpColumns.size()) {
        mnCursorRow = index;
        mnCursorColumn = column;
        _Edit();
    }
}

template<typename T>
void GridView<T>::StopEditing()
{
    mpInputText->SetModified(false);
    if (!mpInputView->IsHidden()) {
        mpInputView->Hide();
    }
}

template<typename T>
float GridView<T>::_GetColumnLeft(int32 column) const
{
    float x = 0;
    if (0 <= column && column < mpColumns.size()) {
        for (size_t i = 0; i < column; ++i) {
            x += mpTitleView->ColumnWidth(i);
        }
    }
    return x;
}

template<typename T>
void GridView<T>::_Edit()
{
    if (!(_IsCursorValid() &&
          0 <= mnCursorColumn && mnCursorColumn < mpColumns.size())) {
        return;
    }
    int32 nLastRow = std::min(mnFirstRow + mnRows + 1,
                              static_cast<int32>(mpRows.size()));
    if (!(mnFirstRow <= mnCursorRow && mnCursorRow <= nLastRow)) {
        // scroll to show current cursor row and column
        ShowItem(mnCursorRow);
    }

    BPoint pos;
    if (mpScrollView) {
        BView* parent = mpScrollView;
        while (parent) {
            pos += parent->Frame().LeftTop();
            parent = parent->Parent();
        }
    }
    pos += Frame().LeftTop();
    pos.x += _GetColumnLeft(mnCursorColumn);
    pos.y += mpTitleView->Height();
    pos.y += (mnCursorRow - mnFirstRow + 1) * mnRowHeight;
    pos = Window()->ConvertToScreen(pos);
    pos += BPoint(1, 1); // grid line

    BRect rect(_GetColumnLeft(mnCursorColumn),
               mpTitleView->Height() + (mnCursorRow - mnFirstRow) * mnRowHeight,
               0, 0);
    rect.right = rect.left + mpTitleView->ColumnWidth(mnCursorColumn);
    rect.bottom = rect.top + mnRowHeight;

    mpColumns[mnCursorColumn]->Edit(this, rect, pos, mpRows[mnCursorRow]);
}

template<typename T>
void GridView<T>::MouseDown(BPoint p)
{
    MakeFocus(true);
    uint32 buttons;
    GetMouse(&p, &buttons, false);
    if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        if (mpLooper && mpMenuRequestMessage) {
            BMessage message(*mpMenuRequestMessage);
            message.AddPoint("pos", p);
            mpLooper->PostMessage(&message);
        }
        return;
    } else if (!(buttons & B_PRIMARY_MOUSE_BUTTON)) {
        return;
    }
    const uint32 mods = modifiers();
    const float nTitleHeight = mpTitleView->Height();
    int32 nRow = (p.y - nTitleHeight) / mnRowHeight + mnFirstRow;
    if (nRow >= mpRows.size()) {
        // todo, out of rows, remove all selections
        return;
    }

    int32 nColumn = mpColumns.size() - 1;
    const float x = p.x;
    float w = mpTitleView->ColumnWidth(0);
    for (size_t i = 0; i < mpColumns.size() - 1; ++i) {
        if (x < w) {
            nColumn = i;
            break;
        }
        w += mpTitleView->ColumnWidth(i + 1);
    }

    bool bChanged = mnCursorRow != nRow || mnCursorColumn != nColumn;
    if (bChanged && 0 <= mnCursorColumn && mnCursorColumn < mpColumns.size()) {
        // exit current edit
        bool bState = mpColumns[mnCursorColumn]->Apply();
        if (!mpInputView->IsHidden()) {
            mpInputView->Hide();
        }
        if (bState && !bChanged) {
            bChanged = true;
        }
    }

    bool bEdit = true;
    if (mnSelectionType == SINGLE) {
        if (_IsCursorValid() && (mods & B_COMMAND_KEY) && nRow == mnCursorRow) {
            _DeselectRow(mnCursorRow);
        } else {
            _DeselectRow(mnCursorRow);
            _SelectRow(nRow);
        }
    } else {
        if (mods & B_SHIFT_KEY) {
            if (!mbRangeSelectionMode) {
                mbRangeSelectionMode = true;
                mnRangeStartRow = mnCursorRow; // cursor as start row
            }
            // select between mnRangeStartRow and clicked row
            if (mbSingleSelection && _IsCursorValid()) {
                _DeselectRow(mnCursorRow);
            } else {
                _DeselectAll();
            }
            const int32 start = std::min(mnRangeStartRow, nRow);
            const int32 end = std::max(mnRangeStartRow, nRow);
            for (int32 i = start; i <= end; ++i) {
                _SelectRow(i);
            }
            mbSingleSelection = false;
            bEdit = false;
        } else if (mods & B_COMMAND_KEY) {
            if (mpRows[nRow]->IsSelected()) {
                //  make unselected
                _DeselectRow(nRow);
            } else {
                _SelectRow(nRow);
            }
            mbSingleSelection = false;
            mbRangeSelectionMode = false;
            bEdit = false;
        } else {
            if (mbSingleSelection && _IsCursorValid()) {
                _DeselectRow(mnCursorRow);
            } else {
                _DeselectAll();
            }
            _SelectRow(nRow);
            mbSingleSelection = true;
            mbRangeSelectionMode = false;
        }
    }

    mnCursorRow = nRow;
    mnCursorColumn = nColumn;

    if (bChanged) {
        Invalidate();
    } else if (bEdit) {
        _Edit();
    }
}

template<typename T>
void GridView<T>::_MoveToNextCell(bool forward)
{
    int32 dx = 0;
    int32 dy = 0;
    if (forward) {
        dx += 1;
        if (mnCursorColumn >= mpColumns.size()) {
            dx = 0;
            dy = 1;
        }
    } else {
        dx -= 1;
        if (mnCursorColumn <= 0) {
            dx = 0;
            dy = -1;
        }
    }
    _MoveCursor(dx, dy, 0);
}

template<typename T>
void GridView<T>::_Sort(int32 index)
{
    RowComp cmp = T::GetComp(index);
    if (cmp) {
        if (0 <= index && index < mnSortType.size()) {
            const SortType type = mnSortType[index];
            if (type == NO_SORT) {
                return;
            }
            for (int32 i = 0; i < mnSortType.size(); ++i) {
                if (mnSortType[i] != NO_SORT) {
                    mnSortType[i] = NOT_SORTED;
                }
            }
            // apply before sorting
            BMessage message(InputTextView::APPLY);
            this->MessageReceived(&message);
            const bool reverse = type == ASCENDING;
            std::sort(mpRows.begin(), mpRows.end(), cmp);
            if (reverse) {
                // cmp function for reverse sort?
                std::reverse(mpRows.begin(), mpRows.end());
            }
            mnSortType[index] = reverse ? DESCENDING : ASCENDING;
            Invalidate();
        }
    }
}

template<typename T>
void GridView<T>::_MoveCursor(int32 dx, int32 dy, uint32 mod)
{
    int32 column = mnCursorColumn + dx;
    if (column < 0) {
        column = 0;
    } else if (column > (mpColumns.size() - 1)) {
        column = mpColumns.size() - 1;
    }
    int32 row = mnCursorRow + dy;
    if (row < 0) {
        row = 0;
    } else if (row > mpRows.size()) {
        row = mpRows.size();
    }
    int32 nCursorRow = row == mpRows.size() ? row - 1 : row;
    const bool bChanged = mnCursorColumn != column || mnCursorRow != row;
    mnCursorColumn = column;
    if (row != mnCursorRow) {
        if (mnSelectionType == SINGLE) {
            // move cursor only if command is pushed
            if (!(mod & B_COMMAND_KEY)) {
                _DeselectRow(mnSingleSelectedRow);
                _DeselectRow(mnCursorRow);
                _SelectRow(nCursorRow);
            }
            mnCursorRow = nCursorRow;
        } else {
            if (mod & B_SHIFT_KEY) {
                // expand selection
                if (!mbRangeSelectionMode) {
                    mbRangeSelectionMode = true;
                    mnRangeStartRow = mnCursorRow; // cursor as start row
                }
                // todo, make this fast by treating difference only
                {
                    const int32 start = std::min(mnRangeStartRow, mnCursorRow);
                    const int32 end = std::max(mnRangeStartRow, mnCursorRow);
                    for (int32 i = start; i <= end; ++i) {
                        _DeselectRow(i);
                    }
                }
                {
                    const int32 start = std::min(mnRangeStartRow, row);
                    const int32 end = std::max(mnRangeStartRow, row);
                    for (int32 i = start; i <= end; ++i) {
                        _SelectRow(i);
                    }
                }
                mbSingleSelection = false;
            } else if (mod & B_COMMAND_KEY) {
                // move cursor only
                mbSingleSelection = false;
                mbRangeSelectionMode = false;
            } else {
                // move selection
                if (mbSingleSelection && _IsCursorValid()) {
                    _DeselectRow(mnCursorRow);
                } else {
                    _DeselectAll();
                }
                _SelectRow(nCursorRow);
                mbSingleSelection = true;
                mbRangeSelectionMode = false;
            }
            mnCursorRow = nCursorRow;
        }
    }
    // scroll to show the cursor
    ShowItem(row);
    if (bChanged) {
        Invalidate();
    }
}

template<typename T>
void GridView<T>::_Key(BMessage* msg)
{
    const uint8 byte = static_cast<uint8>(msg->GetInt8("byte", 0));
    const uint32 modifiers = static_cast<uint32>(msg->GetInt32("modifiers", 0));
    switch (byte) {
        case B_LEFT_ARROW:
            _MoveCursor(-1, 0, modifiers);
            break;
        case B_RIGHT_ARROW:
            _MoveCursor(1, 0, modifiers);
            break;
        case B_UP_ARROW:
            _MoveCursor(0, -1, modifiers);
            break;
        case B_DOWN_ARROW:
            _MoveCursor(0, 1, modifiers);
            break;
        case B_HOME:
            _MoveCursor(-100, 0, modifiers);
            break;
        case B_END:
            _MoveCursor(100, 0, modifiers);
            break;
        case B_PAGE_UP:
            _MoveCursor(0, -mnRows, modifiers);
            break;
        case B_PAGE_DOWN:
            _MoveCursor(0, mnRows, modifiers);
            break;
        case B_TAB:
            //_MoveCursor((modifiers & B_SHIFT_KEY) ? -1 : 1, 0, modifiers);
            _MoveToNextCell((modifiers & B_SHIFT_KEY) == 0);
            break;
        case B_ENTER:
            break;
        case B_DELETE:
            if (mpLooper) {
                BMessage message(B_DELETE);
                mpLooper->PostMessage(&message);
            }
            break;
        default:
            if (' ' <= byte && byte <= '~') {
                // start edit and put the value in the box
                _Edit();
                const char s = static_cast<char>(byte);
                mpInputText->SetText(&s, 1, NULL);
                mpInputText->Select(1, 1);
                mpInputText->SetModified(true);
            } else {
                BView::MessageReceived(msg);
            }
            break;
    }
}

template<typename T>
void GridView<T>::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case B_KEY_DOWN:
            _Key(msg);
            break;
        case InputTextView::UPDATE:
            Invalidate();
            if (mpLooper) {
                BMessage message(MODIFIED);
                mpLooper->PostMessage(&message);
            }
            break;
        case InputTextView::APPLY:
            if (0 <= mnCursorColumn && mnCursorColumn < mpColumns.size()) {
                if (mpColumns[mnCursorColumn]->Apply()) {
                    mbModified = true;
                    Invalidate();
                }
            }
            break;
        case B_INPUT_METHOD_EVENT:
        {
            int32 opcode = 0;
            if (msg->FindInt32("be:opcode", &opcode) == B_OK) {
                switch (opcode) {
                    case B_INPUT_METHOD_STARTED:
                        _Edit();
                        mpInputText->SetText("");
                        mpInputText->MessageReceived(msg);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case GRID_SORT:
        {
            const int32 index = msg->GetInt32("index", -1);
            if (0 <= index && index <= mpColumns.size()) {
                _Sort(index);
            }
            break;
        }
        default:
            BView::MessageReceived(msg);
            break;
    }
}

} // haiku_gui
} // mozc

#endif // GRID_VIEW_H
