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

// ported from gui/character_pad

#include "haiku/haiku_gui/character_pad/hand_writing.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/character_pad/character_list.h"
#include "haiku/haiku_gui/character_pad/character_window.h"

#include "handwriting/handwriting_manager.h"
#include "handwriting/zinnia_handwriting.h"

#include "base/clock.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "base/util.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Looper.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <StringView.h>

#include <string>
#include <vector>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "character_pad"

namespace {
// Thread-safe copying of strokes.
void CopyStrokes(const mozc::handwriting::Strokes &source,
                 mozc::handwriting::Strokes *target,
                 mozc::Mutex *mutex) {
  DCHECK(target);
  DCHECK(mutex);
  mozc::scoped_lock l(mutex);
  target->clear();
  target->resize(source.size());
  for (size_t i = 0; i < source.size(); ++i) {
    const mozc::handwriting::Stroke &stroke = source[i];
    (*target)[i].resize(stroke.size());
    for (size_t j = 0; j < stroke.size(); ++j) {
      (*target)[i][j] = stroke[j];
    }
  }
}

// Thread-safe copying of candidates.
void CopyCandidates(
    const vector<string> &source, vector<string> *target,
    mozc::Mutex *mutex) {
  DCHECK(target);
  DCHECK(mutex);
  mozc::scoped_lock l(mutex);
  target->clear();
  target->resize(source.size());
  for (size_t i = 0; i < source.size(); ++i) {
    (*target)[i] = source[i];
  }
}
}  // namespace

namespace mozc {
namespace haiku_gui {

class HandWritingThread : public BLooper
{
public:
    enum {
        RECOGNITION_START = 'rcst',
        CANDIDATE_UPDATED = 'cdup',
        STATUS_UPDATED = 'stup',
    };

    HandWritingThread(BHandler* handler);
    virtual ~HandWritingThread();

    virtual void MessageReceived(BMessage* msg);

    void SetStrokes(const handwriting::Strokes &strokes);
    void GetCandidates(std::vector<std::string>* candidates);
    void startRecognition();
private:
    handwriting::Strokes strokes_;
    std::vector<std::string> candidates_;

    uint64 strokes_sec_;
    uint32 strokes_usec_;
    uint64 last_requested_sec_;
    uint32 last_requested_usec_;

    mozc::Mutex strokes_mutex_;
    mozc::Mutex candidates_mutex_;

    bool usage_stats_enabled_;
    BHandler* mpHandler;

    void SendUpdateStatus(handwriting::HandwritingStatus status);
    void CandidateUpdated();
};

HandWritingThread::HandWritingThread(BHandler* handler)
{
    mpHandler = handler;
    strokes_sec_ = 0;
    strokes_usec_ = 0;
    last_requested_sec_ = 0;
    last_requested_usec_ = 0;
}

HandWritingThread::~HandWritingThread()
{
    mpHandler = NULL;
}

void HandWritingThread::SetStrokes(const handwriting::Strokes &strokes)
{
    CopyStrokes(strokes, &strokes_, &strokes_mutex_);
    Clock::GetTimeOfDay(&strokes_sec_, &strokes_usec_);
}

void HandWritingThread::GetCandidates(std::vector<std::string>* candidates)
{
    CopyCandidates(candidates_, candidates, &candidates_mutex_);
}

void HandWritingThread::startRecognition()
{
    if (last_requested_sec_ == strokes_sec_ &&
        last_requested_usec_ == strokes_usec_) {
        LOG(WARNING) << "Already sent that stroke";
        return;
    }
    handwriting::HandwritingStatus status = handwriting::HANDWRITING_NO_ERROR;
    SendUpdateStatus(status);

    handwriting::Strokes strokes;
    CopyStrokes(strokes_, &strokes, &strokes_mutex_);
    if (strokes.empty()) {
        return;
    }

    vector<string> candidates;
    status = handwriting::HandwritingManager::Recognize(strokes, &candidates);
    CopyCandidates(candidates, &candidates_, &candidates_mutex_);
    last_requested_sec_ = strokes_sec_;
    last_requested_usec_ = strokes_usec_;
    SendUpdateStatus(status);
    CandidateUpdated();
}

void HandWritingThread::SendUpdateStatus(handwriting::HandwritingStatus status)
{
    if (mpHandler) {
        BMessage message(STATUS_UPDATED);
        message.AddInt32("status", static_cast<int32>(status));
        mpHandler->MessageReceived(&message);
    }
}

void HandWritingThread::CandidateUpdated()
{
    if (mpHandler) {
        BMessage message(CANDIDATE_UPDATED);
        mpHandler->MessageReceived(&message);
    }
}

void HandWritingThread::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case RECOGNITION_START:
        {
            startRecognition();
            break;
        }
        default:
        {
            BLooper::MessageReceived(msg);
            break;
        }
    }
}


class HandWritingCanvas : public BView
{
public:
    enum HandWritingActions
    {
        CANVAS_CLEAR = 'cvcl',
        CANVAS_REVERT = 'cvrv',
        CANVAS_UPDATED = 'cvup',
    };
    HandWritingCanvas();
    virtual ~HandWritingCanvas();

    virtual void MessageReceived(BMessage* msg);
    virtual void Draw(BRect updateRect);
    virtual void MouseDown(BPoint p);
    virtual void MouseUp(BPoint p);
    virtual void MouseMoved(BPoint p, uint32 code, const BMessage* dragMessage);
    virtual BSize MinSize();
    virtual BSize MaxSize();
    size_t strokes_size() const;
    void SetList(CharacterList* pList);
private:
    handwriting::Strokes strokes_;
    bool is_drawing_;
    HandWritingThread* recognizer_thread_;
    handwriting::HandwritingStatus handwriting_status_;

    CharacterList* fCharacterList;

    void clear();
    void revert();
    void update();
    void recognize();
    void statusUpdated(handwriting::HandwritingStatus status);
    void listUpdated();
    void _WrapDraw(const char* text, BPoint pos, float width);
};

HandWritingCanvas::HandWritingCanvas()
    : BView(BRect(0, 0, 169, 169), "canvas",
            B_FOLLOW_NONE, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
      is_drawing_(false),
      handwriting_status_(handwriting::HANDWRITING_NO_ERROR),
      fCharacterList(NULL)
{
    strokes_.reserve(128);
    recognizer_thread_ = new HandWritingThread(this);
    recognizer_thread_->Run();
}

HandWritingCanvas::~HandWritingCanvas()
{
    if (recognizer_thread_) {
        recognizer_thread_->Lock();
        recognizer_thread_->Quit();
    }
    fCharacterList = NULL;
}

BSize HandWritingCanvas::MinSize()
{
    return BSize(170, 170);
}

BSize HandWritingCanvas::MaxSize()
{
    return BSize(170, 170);
}

void HandWritingCanvas::SetList(CharacterList* pList)
{
    fCharacterList = pList;
}

void HandWritingCanvas::update()
{
    Invalidate();
}

void HandWritingCanvas::clear()
{
    handwriting_status_ = handwriting::HANDWRITING_NO_ERROR;
    strokes_.clear();
    update();
    is_drawing_ = false;
}

void HandWritingCanvas::revert()
{
    handwriting_status_ = handwriting::HANDWRITING_NO_ERROR;
    if (!strokes_.empty()) {
        strokes_.resize(strokes_.size() -1);
        update();
        recognize();
    }
    is_drawing_ = false;
}

void HandWritingCanvas::recognize()
{
    if (strokes_.empty()) {
        return;
    }

    recognizer_thread_->SetStrokes(strokes_);
    BMessage message(HandWritingThread::RECOGNITION_START);
    recognizer_thread_->PostMessage(&message);
}

void HandWritingCanvas::listUpdated()
{
    vector<string> candidates;
    recognizer_thread_->GetCandidates(&candidates);

    if (fCharacterList) {
        if (fCharacterList->LockLooper()) {
            fCharacterList->SetChars(candidates);
            fCharacterList->UnlockLooper();
        }
    }
}

void HandWritingCanvas::statusUpdated(handwriting::HandwritingStatus status)
{
    handwriting_status_ = status;
}

void HandWritingCanvas::Draw(BRect rect)
{
    const BRect bounds = Bounds();
    const float height = bounds.Height();
    const float width = bounds.Width();

    const float diff = floor(height * 0.05);
    const float margin = floor(height * 0.04);

    //SetLowColor(make_color(255, 255, 255, 255));
    //FillRect(rect, B_SOLID_LOW);

    const rgb_color lineColor = ui_color(B_CONTROL_BORDER_COLOR);

    BeginLineArray(10);
    AddLine(BPoint(width / 2 - diff, height / 2),
            BPoint(width / 2 + diff, height / 2), lineColor);
    AddLine(BPoint(width / 2, height / 2 - diff),
            BPoint(width / 2, height / 2 + diff), lineColor);

    AddLine(BPoint(margin, margin), BPoint(margin + diff, margin), lineColor);
    AddLine(BPoint(margin, margin), BPoint(margin, margin + diff), lineColor);

    AddLine(BPoint(width - margin - diff, margin),
            BPoint(width - margin, margin), lineColor);
    AddLine(BPoint(width - margin, margin),
            BPoint(width - margin, margin + diff), lineColor);

    AddLine(BPoint(margin, height - margin - diff),
            BPoint(margin, height - margin), lineColor);
    AddLine(BPoint(margin, height - margin),
            BPoint(margin + diff, height - margin), lineColor);

    AddLine(BPoint(width - margin - diff, height - margin),
            BPoint(width - margin, height - margin), lineColor);
    AddLine(BPoint(width - margin, height - margin - diff),
            BPoint(width - margin, height - margin), lineColor);
    EndLineArray();

    if (strokes_.empty()) {
        const rgb_color highColor = HighColor();
        SetHighColor(lineColor);
        _WrapDraw(B_TRANSLATE("Draw a character here"),
            BPoint(margin + 5, margin + 25), width - margin * 2 - 10);
        SetHighColor(highColor);
    }

    // border
    be_control_look->DrawBorder(this, rect, rect,
                    ui_color(B_CONTROL_BORDER_COLOR), B_FANCY_BORDER);

    SetPenSize(3.0f);

    for (int32 i = 0; i < strokes_.size(); ++i) {
        for (int32 j = 1; j < strokes_[i].size(); ++j) {
            StrokeLine(BPoint(strokes_[i][j - 1].first * width,
                              strokes_[i][j - 1].second * height),
                       BPoint(strokes_[i][j].first * width,
                              strokes_[i][j].second * height));
        }
    }

    if (handwriting_status_ != handwriting::HANDWRITING_NO_ERROR) {
        rgb_color red = {255, 0, 0, 0};
        SetHighColor(red);
        SetPenSize(2.0f);
        const char* warning_message = NULL;
        switch (handwriting_status_) {
            case handwriting::HANDWRITING_ERROR:
                warning_message = B_TRANSLATE("error");
                break;
            case handwriting::HANDWRITING_NETWORK_ERROR:
                warning_message = B_TRANSLATE("network error");
                break;
            case handwriting::HANDWRITING_UNKNOWN_ERROR:
                warning_message = B_TRANSLATE("unknown error");
                break;
            default:
                break;
        }
        if (warning_message != NULL) {
            DrawString(warning_message, BPoint(margin + 20, margin + 40));
        }
    }
    BMessage message(CANVAS_UPDATED);
    Window()->PostMessage(&message);
}

void HandWritingCanvas::_WrapDraw(const char* text, BPoint pos_, float width)
{
    float lineWidth = StringWidth(text);
    if (lineWidth <= width) {
        DrawString(text, pos_);
        return;
    }
    font_height height;
    GetFontHeight(&height);
    const float lineHeight = height.ascent + height.descent;

    BPoint pos(pos_);
    size_t subLength = 0;
    const char* pStart = text;
    const char* pEnd = pStart + strlen(text);
    const char* pCurrent = text;
    float totalWidth = 0;

    while (pCurrent < pEnd) {
        size_t n = mozc::Util::OneCharLen(pCurrent);
        float charWidth = StringWidth(pCurrent, n);
        if (totalWidth + charWidth > width) {
            DrawString(pStart, subLength, pos);
            pStart = pCurrent;
            subLength = n;
            totalWidth = charWidth;
            pos.y += lineHeight;
        } else {
            subLength += n;
            totalWidth += charWidth;
        }
        pCurrent += n;
    }
    DrawString(pStart, subLength, pos);
}

void HandWritingCanvas::MouseDown(BPoint p)
{
    uint32 buttons;
    GetMouse(&p, &buttons, true);
    if (buttons == B_PRIMARY_MOUSE_BUTTON) {
        BRect bounds = Bounds();
        strokes_.resize(strokes_.size() + 1);
        strokes_.back().push_back(std::make_pair(p.x / bounds.Width(), p.y / bounds.Height()));
        is_drawing_ = true;
        update();
    }
}

void HandWritingCanvas::MouseUp(BPoint p)
{
    if (is_drawing_) {
        is_drawing_ = false;
        update();
        recognize();
    }
}

void HandWritingCanvas::MouseMoved(BPoint p, uint32 code, const BMessage* dragMessage)
{
    if (!is_drawing_) {
        return;
    }

    BRect bounds = Bounds();
    strokes_.back().push_back(std::make_pair(p.x / bounds.Width(), p.y / bounds.Height()));
    update();
}

size_t HandWritingCanvas::strokes_size() const
{
    return strokes_.size();
}

void HandWritingCanvas::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case CANVAS_CLEAR:
        {
            clear();
            break;
        }
        case CANVAS_REVERT:
        {
            revert();
            break;
        }
        case HandWritingThread::STATUS_UPDATED:
        {
            int32 status = 0;
            if (msg->FindInt32("status", &status) == B_OK) {
                if (handwriting::HANDWRITING_NO_ERROR <= status &&
                    status <= handwriting::HANDWRITING_UNKNOWN_ERROR) {
                    statusUpdated(static_cast<handwriting::HandwritingStatus>(status));
                }
            }
            break;
        }
        case HandWritingThread::CANDIDATE_UPDATED:
        {
            listUpdated();
            break;
        }
        default:
        {
            BView::MessageReceived(msg);
            break;
        }
    }
}


class HandWriting : public CharacterWindow
{
public:
    HandWriting();
    virtual ~HandWriting();

    virtual void MessageReceived(BMessage *msg);
private:
    std::unique_ptr<mozc::handwriting::HandwritingInterface> zinnia_handwriting_;
    HandWritingCanvas*      fCanvas;
    BButton*                fClearButton;
    BButton*                fRevertButton;

    void updateUIStatus();
};

HandWriting::HandWriting(void)
    : CharacterWindow(
        BRect(50, 50, 550, 340),
        B_TRANSLATE("Mozc Hand Writing")),
      zinnia_handwriting_(new mozc::handwriting::ZinniaHandwriting(
        mozc::handwriting::ZinniaHandwriting::GetModelFileName()))
{
    mozc::handwriting::HandwritingManager::SetHandwritingModule(zinnia_handwriting_.get());

    fCharacterList = new CharacterList("list", CharacterList::TABLE,
                        B_NAVIGABLE,
                        new BMessage(CHAR_SELECTION_CHANGED), this,
                        new BMessage(ON_OVER_CHANGED));
    BScrollView* fCharListView = new BScrollView("listview", fCharacterList,
                B_FOLLOW_LEFT_TOP, B_FRAME_EVENTS, true, true, B_FANCY_BORDER);
    fCanvas = new HandWritingCanvas();
    fClearButton = new BButton("clear", B_TRANSLATE("clear"),
                                new BMessage(HandWritingCanvas::CANVAS_CLEAR));
    fRevertButton = new BButton("undo", B_TRANSLATE("revert"),
                                new BMessage(HandWritingCanvas::CANVAS_REVERT));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL, 5)
            .SetInsets(5, 5, 5, 5)
            .SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP))
            .AddGlue()
            .Add(fFontMF)
            .Add(fFontSizeMF)
        .End()
        .AddGroup(B_HORIZONTAL, 5)
            .SetInsets(5, 0, 5, 0)
            .AddGroup(B_VERTICAL, 5)
                .Add(fCanvas)
                .AddGroup(B_HORIZONTAL, 0)
                    .Add(fClearButton)
                    .Add(fRevertButton)
                .End()
                .AddGlue()
                .AddGlue()
            .End()
            .Add(fCharListView)
        .End()
        .AddGroup(B_HORIZONTAL, 0)
            .SetInsets(5, 0, 0, 0)
            .Add(fStatusText)
        .End();
    Layout(true);
    fCanvas->SetList(fCharacterList);
    updateUIStatus();
}

HandWriting::~HandWriting()
{
}

void HandWriting::updateUIStatus()
{
    const bool enabled = fCanvas->strokes_size() > 0;
    fClearButton->SetEnabled(enabled);
    fRevertButton->SetEnabled(enabled);
}

void HandWriting::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case HandWritingCanvas::CANVAS_CLEAR:
        {
            fCharacterList->ClearChars();
            fCanvas->MessageReceived(msg);
            updateUIStatus();
            break;
        }
        case HandWritingCanvas::CANVAS_REVERT:
        {
            fCharacterList->ClearChars();
            fCanvas->MessageReceived(msg);
            updateUIStatus();
            break;
        }
        case HandWritingCanvas::CANVAS_UPDATED:
        {
            updateUIStatus();
            break;
        }
        default:
        {
            CharacterWindow::MessageReceived(msg);
            break;
        }
    }
}


class HandWritingApp : public ToolApp
{
public:
    HandWritingApp();
    virtual ~HandWritingApp() {}
};

HandWritingApp::HandWritingApp()
    : ToolApp(HAND_WRITING)
{
    mpWindow = new HandWriting();
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}

}; // haiku_gui
}; // mozc

int HaikuRunHandWriting(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                        mozc::haiku_gui::HAND_WRITING)) {
        return -1;
    }

    mozc::haiku_gui::HandWritingApp* app = new mozc::haiku_gui::HandWritingApp();
    app->Run();
    delete app;

    return 0;
}
