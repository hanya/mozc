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

#ifndef DICTIONARY_GRID_H
#define DICTIONARY_GRID_H

#include "haiku/haiku_gui/base/grid_view.h"

namespace mozc {
namespace haiku_gui {

class DictContentRow : public GridRow
{
public:
    DictContentRow(const char* reading, const char* word,
                   int32 ncategory, const char* comment);
    DictContentRow();
    DictContentRow(const DictContentRow* row);
    virtual ~DictContentRow() {}

    const std::string& Reading() const { return maReading; }
    const std::string& Word() const { return maWord; }
    const std::string& Comment() const { return maComment; }
    int32 Category() const { return mnCategory; }
    void SetReading(const char* reading) { maReading = reading; }
    void SetWord(const char* word) { maWord = word; }
    void SetComment(const char* comment) { maComment = comment; }
    void SetCategory(int32 category) { mnCategory = category; }

    typedef bool (*DictContentRowComp)(DictContentRow *r1, DictContentRow *r2);
    static bool ReadingComp(DictContentRow *r1, DictContentRow *r2)
    {
        return r1->Reading().compare(r2->Reading()) < 0;
    }

    static bool WordComp(DictContentRow *r1, DictContentRow *r2)
    {
        return r1->Word().compare(r2->Word()) < 0;
    }

    static bool CategoryComp(DictContentRow *r1, DictContentRow *r2)
    {
        return r1->Category() < r2->Category();
    }

    static bool CommentComp(DictContentRow *r1, DictContentRow *r2)
    {
        return r1->Comment().compare(r2->Comment()) < 0;
    }
    
    static DictContentRowComp GetComp(int32 index)
    {
        switch (index) {
            case 0:
                return *ReadingComp;
                break;
            case 1:
                return *WordComp;
                break;
            case 2:
                return *CategoryComp;
                break;
            case 3:
                return *CommentComp;
                break;
        }
        return NULL;
    }
private:
    std::string maReading;
    std::string maWord;
    int32 mnCategory;
    std::string maComment;
};


template<typename T>
class DictTextColumn : public GridTextColumn<T>
{
public:
    DictTextColumn(BView* view, int32 column, BHandler* handler, ListInputView* p);
    virtual ~DictTextColumn() {};
protected:
    const char* _GetLabel(T* pRow);
    void _SetText(const char* s);
};

template<typename T>
DictTextColumn<T>::DictTextColumn(BView* view, int32 column,
                        BHandler* handler, ListInputView* p)
    : GridTextColumn<T>(view, column, handler, p)
{
}

template<typename T>
const char* DictTextColumn<T>::_GetLabel(T* pRow)
{
    if (pRow) {
        switch (this->mnColumn) {
            case 0:
                return pRow->Reading().c_str();
                break;
            case 1:
                return pRow->Word().c_str();
                break;
            case 3:
                return pRow->Comment().c_str();
            default:
                return "";
        }
    }
    return "";
}

template<typename T>
void DictTextColumn<T>::_SetText(const char* s)
{
    if (this->mpRow) {
        switch (this->mnColumn) {
            case 0:
                this->mpRow->SetReading(s);
                break;
            case 1:
                this->mpRow->SetWord(s);
                break;
            case 3:
                this->mpRow->SetComment(s);
                break;
            default:
                break;
        }
    }
}


template<typename T>
class DictCategoryColumn : public GridMenuFieldColumn<T>
{
public:
    DictCategoryColumn(BView* view, int32 column, BHandler* handler);
    virtual ~DictCategoryColumn() {};

    void AddCategory(const char* label);
    const char* GetLabelForCategory(int32 category);
protected:
    virtual void _UpdateRow(T* pRow, BMessage* msg);
    virtual int32 _LabelIndex(T* pRow);
    virtual int32 _CurrentIndex(T* pRow);
};

template<typename T>
DictCategoryColumn<T>::DictCategoryColumn(BView* view, int32 column, BHandler* handler)
    : GridMenuFieldColumn<T>(view, column, handler)
{
    // index 0 is empty element
    this->maLabels.push_back("");
}

template<typename T>
void DictCategoryColumn<T>::_UpdateRow(T* pRow, BMessage* msg)
{
    if (pRow) {
        const int32 category = msg->what;
        if (1 <= category && category < this->CountLabels()) {
            pRow->SetCategory(category);
            this->_Modified();
        }
    }
}

template<typename T>
int32 DictCategoryColumn<T>::_LabelIndex(T* pRow)
{
    if (pRow) {
        const int32 category = pRow->Category();
        return (1 <= category && category < this->CountLabels()) ? category : -1;
    }
    return -1;
}

template<typename T>
int32 DictCategoryColumn<T>::_CurrentIndex(T* pRow)
{
    if (pRow) {
        const int32 category = pRow->Category();
        return (1 <= category && category < this->CountLabels()) ? category - 1 : -1;
    }
    return -1;
}

template<typename T>
const char* DictCategoryColumn<T>::GetLabelForCategory(int32 category)
{
    if (1 <= category && category < this->CountLabels()) {
        return this->maLabels[category];
    }
    return "";
}

template<typename T>
void DictCategoryColumn<T>::AddCategory(const char* label)
{
    this->maLabels.push_back(label);
    this->mpPopUpMenu->AddItem(
        new BMenuItem(label, new BMessage(this->CountLabels() - 1)));
}

} // haiku_gui
} // mozc

#endif // DICTIONARY_GRID_H
