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

#include "haiku/input_method/candidate_window.h"

#include "haiku/input_method/common.h"
#include "haiku/input_method/looper.h"
#include "protocol/commands.pb.h"

#include <Input.h>
#include <MessageRunner.h>
#include <View.h>

#include <algorithm>
#include <mutex>
#include <vector>
#include <sstream>

#include <math.h>

#if DEBUG
#include <stdio.h>
#define LOGGER "application/x-vnd.Logger"
#define LOG_COMMAND 'Logg'
#define LOG_NAME "log"
#endif

namespace immozc {

#define _MOUSE_CLICK     'CVcl'

// Widely used number of candidates
#define CANDIDATE_COUNT  9

static const float border = 1.;


class CandidateView : public BView
{
public:
                    CandidateView(BRect frame);
                    ~CandidateView(void);
    virtual void    Draw(BRect rect);
    virtual void    MouseDown(BPoint where);
    virtual void    MessageReceived(BMessage *msg);
    float           GetValueLeft();
    void            Init();
    void            SetData(std::unique_ptr<mozc::commands::Output> output);
    void            ClearData();
    
private:
    struct ColumnWidth {
        float shortcut;
        float candidate;
        float annotDesc;
    };
    int32            fCandidateCount;
    int32            fLineHeight;
    std::mutex       fDataMutex;
    std::unique_ptr<mozc::commands::Output> fOutput;
    ColumnWidth      fColumnWidth;
    float            fFullwidthSpaceWidth;
    float            fShortcutCharWidth;
    bool             fShowAnnotDesc;
    bool             fShowShortcut;
    bool             fShowPagination;
    bool             fNeedsRecalculate;
    bigtime_t        fPrevClick;
    int32            fClickCount;
    BMessageRunner * fMouseClickRunner;
    
    std::vector<const char*> fShortcutVec;
    
    std::vector<const char*> fCandidateVec;
    std::vector<int32>       fCandidateLengthVec;
    std::vector<float>       fCandidateWidthVec;
    
    std::vector<const char*> fAnnotDescVec;
    std::vector<int32>       fAnnotDescLengthVec;
    std::vector<float>       fAnnotDescWidthVec;
    
    int32   _PointToIndex(BPoint *where) const;
    void    _DrawVertical();
    bool    _GetCandidateId(int32 index, int32 *id);
    void    _PostClickMessage(int32 command, int32 index);
    void    _CalculateSize();
    
#if DEBUG
    std::unique_ptr<BMessenger>      fLogger;
    void                             _SendLog(const char *s);
#endif
};

CandidateView::CandidateView(BRect frame)
    : BView(
        frame,
        "candidates",
        B_FOLLOW_ALL_SIDES,
        B_WILL_DRAW),
     fCandidateCount(0),
     fLineHeight(14),
     fOutput(nullptr),
     fFullwidthSpaceWidth(0.),
     fShortcutCharWidth(0.),
     fShowAnnotDesc(true),
     fShowShortcut(false),
     fShowPagination(false),
     fPrevClick(system_time()),
     fClickCount(0),
     fMouseClickRunner(nullptr)
{
#if DEBUG
    fLogger = std::unique_ptr<BMessenger>(new BMessenger(LOGGER));
#endif
    fShortcutVec.reserve(CANDIDATE_COUNT);
    
    fCandidateVec.reserve(CANDIDATE_COUNT);
    fCandidateLengthVec.reserve(CANDIDATE_COUNT);
    fCandidateWidthVec.reserve(CANDIDATE_COUNT);
    
    fAnnotDescVec.reserve(CANDIDATE_COUNT);
    fAnnotDescLengthVec.reserve(CANDIDATE_COUNT);
    fAnnotDescWidthVec.reserve(CANDIDATE_COUNT);
}

CandidateView::~CandidateView(void)
{
    if (fMouseClickRunner != NULL) {
        delete fMouseClickRunner;
    }
}

#if DEBUG
void CandidateView::_SendLog(const char *s)
{
    if (!fLogger->IsValid()) {
        if (fLogger->SetTo(LOGGER) != B_OK) {
            return;
        }
    }
    BMessage msg(LOG_COMMAND);
    msg.AddString(LOG_NAME, s);
    fLogger->SendMessage(&msg);
}
#endif

void CandidateView::SetData(std::unique_ptr<mozc::commands::Output> output)
{
    std::lock_guard<std::mutex> lock(fDataMutex);
    
    std::unique_ptr<mozc::commands::Output> prev = std::move(fOutput);
    fOutput = std::move(output);
    // Reduce calculation of candidate width.
    fNeedsRecalculate = true;
    if (prev && fOutput && prev->has_candidates() && fOutput->has_candidates()) {
        if (prev->candidates().category() == fOutput->candidates().category()) {
            // If candidates are the same, we might not need to recalculate 
            // width of the candidates.
            if (prev->candidates().candidate_size() == 
                            fOutput->candidates().candidate_size()) {
                const mozc::commands::Candidates &prev_candidates = 
                                prev->candidates();
                const mozc::commands::Candidates &curr_candidates = 
                                fOutput->candidates();
                bool changed = false;
                for (int i = 0; i < prev_candidates.candidate_size(); ++i) {
                    if (prev_candidates.candidate(i).id() != 
                                curr_candidates.candidate(i).id()) {
                        changed = true;
                        break;
                    }
                }
                if (!changed) {
                    fNeedsRecalculate = false;
                }
            }
        }
    }
    if (!fNeedsRecalculate) {
        return;
    }
    
    fShowShortcut = false;
    if (fOutput && fOutput->has_candidates()) {
        const mozc::commands::Candidates &candidates = fOutput->candidates();
        fCandidateCount = candidates.candidate_size();
        const mozc::commands::Candidates_Candidate &candidate = candidates.candidate(0);
        if (candidate.has_annotation() && candidate.annotation().has_shortcut()) {
            fShowShortcut = candidate.annotation().shortcut().length() != 0;
        }
        
        fShowPagination = (candidates.has_footer() ? 
                            candidates.footer().index_visible() : false);
        const float shortcutAreaWidth = fShortcutCharWidth * 2.5;
        if (fShowShortcut) {
            fColumnWidth.shortcut = shortcutAreaWidth + fFullwidthSpaceWidth * 0.5;
        } else {
            fColumnWidth.shortcut = fShortcutCharWidth * 0.25;
        }
        // zero annotation description might change this later
        fShowAnnotDesc = candidates.category() == mozc::commands::CONVERSION;
    }
}

bool CandidateView::_GetCandidateId(int32 index, int32 *id)
{
    std::lock_guard<std::mutex> lock(fDataMutex);
    
    bool found = false;
    if (fOutput) {
        if (fOutput->has_candidates() && 
            fOutput->candidates().candidate_size() > index) {
            const mozc::commands::Candidates_Candidate &candidate = 
                    fOutput->candidates().candidate(index);
            if (candidate.has_id()) {
                *id = candidate.id();
                found = true;
            }
        }
    }
    return found;
}

void CandidateView::ClearData()
{
    std::lock_guard<std::mutex> lock(fDataMutex);
    
    fOutput.reset(nullptr);
}

void CandidateView::Init()
{
    const char *fullwidthSpace = "\xe3\x80\x80";
    fFullwidthSpaceWidth = StringWidth(fullwidthSpace, 3);
    
    fShortcutCharWidth = StringWidth("M", 1);
}

void CandidateView::MouseDown(BPoint where)
{
    int32 index = _PointToIndex(&where);
    if (index >= 0) {
        uint32 buttons;
        GetMouse(&where, &buttons, true);
        if (buttons & B_PRIMARY_MOUSE_BUTTON) {
            bigtime_t mouseClickSpeed;
            get_click_speed(&mouseClickSpeed);
            fClickCount += 1;
            if (fMouseClickRunner == nullptr) {
                delete fMouseClickRunner;
                BMessage msg(_MOUSE_CLICK);
                msg.AddInt32("index", index);
                BMessenger messenger(this);
                fMouseClickRunner = new BMessageRunner(
                            messenger, &msg, mouseClickSpeed, 1);
            }
            if (fClickCount == 1) {
                _PostClickMessage(IM_HIGHLIGHT_CANDIDATE, index);
            }
        }
    }
}

int32 CandidateView::_PointToIndex(BPoint *where) const
{
    if (fCandidateCount <= 0 || fLineHeight <= 0) {
        return -1;
    }
    int32 y = static_cast<int32>(where->y);
    if (y >= 0) {
        int32 index = y / fLineHeight;
        if (index < fCandidateCount) {
            return index;
        }
    }
    return -1;
}

float CandidateView::GetValueLeft()
{
    return fColumnWidth.shortcut + border;
}

void CandidateView::_PostClickMessage(int32 command, int32 index)
{
    int32 id;
    if (_GetCandidateId(index, &id)) {
        BMessage mess(command);
        mess.AddInt32("id", id);
        Window()->PostMessage(&mess);
    }
}

void CandidateView::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case CANDIDATE_WINDOW_UPDATE:
        {
            Invalidate();
            break;
        }
        case _MOUSE_CLICK:
        {
            if (fClickCount >= 2) {
                int32 index;
                if (msg->FindInt32("index", &index) == B_OK) {
                    _PostClickMessage(IM_SELECT_CANDIDATE, index);
                }
            }
            fClickCount = 0;
            delete fMouseClickRunner;
            fMouseClickRunner = NULL;
            break;
        }
        
        default:
        {
            BView::MessageReceived(msg);
            break;
        }
    }
}

void CandidateView::_CalculateSize()
{
    //std::lock_guard<std::mutex> lock(fDataMutex);
    if (!fOutput) {
        return;
    }
    const char *empty = "";
    
    const mozc::commands::Candidates &candidates = fOutput->candidates();
    
    if (!fNeedsRecalculate) {
        fShortcutVec.clear();
        fCandidateVec.clear();
        fAnnotDescVec.clear();
        // update string pointers
        for (int i = 0; i < fCandidateCount; ++i) {
            const mozc::commands::Candidates_Candidate &candidate = candidates.candidate(i);
            fCandidateVec[i] = candidate.value().c_str();
            if (fShowAnnotDesc) {
                if (candidate.has_annotation() && 
                    candidate.annotation().has_description()) {
                    fAnnotDescVec[i] = candidate.annotation().description().c_str();
                } else {
                    fAnnotDescVec[i] = empty;
                }
            }
            if (fShowShortcut) {
                if (candidate.has_annotation() && 
                    candidate.annotation().has_shortcut()) {
                    fShortcutVec[i] = candidate.annotation().shortcut().c_str();
                } else {
                    fShortcutVec[i] = empty;
                }
            }
        }
        fNeedsRecalculate = false;
        return;
    }
    
    BFont font;
    GetFont(&font);
    
    fShortcutVec.clear();
    fCandidateVec.clear();
    fAnnotDescVec.clear();
    fCandidateLengthVec.clear();
    fCandidateWidthVec.clear();
    fAnnotDescLengthVec.clear();
    fAnnotDescWidthVec.clear();
    
    // calculates width of each candidates
    // These variables will be used to draw candidates.
    {
        for (int i = 0; i < fCandidateCount; ++i) {
            const mozc::commands::Candidates_Candidate &candidate = candidates.candidate(i);
            fCandidateVec[i] = candidate.value().c_str();
            fCandidateLengthVec[i] = candidate.value().length();
            if (fShowAnnotDesc) {
                if (candidate.has_annotation() && 
                    candidate.annotation().has_description()) {
                    fAnnotDescVec[i] = candidate.annotation().description().c_str();
                    fAnnotDescLengthVec[i] = candidate.annotation().description().length();
                } else {
                    fAnnotDescVec[i] = empty;
                    fAnnotDescLengthVec[i] = 0;
                }
            }
            if (fShowShortcut) {
                if (candidate.has_annotation() && 
                    candidate.annotation().has_shortcut()) {
                    fShortcutVec[i] = candidate.annotation().shortcut().c_str();
                } else {
                    fShortcutVec[i] = empty;
                }
            }
        }
        float maxCandidateWidth = 0.;
        font.GetStringWidths(fCandidateVec.data(), 
            fCandidateLengthVec.data(), fCandidateCount, fCandidateWidthVec.data());
        for (int i = 0; i < fCandidateCount; ++i) {
            maxCandidateWidth = std::max(maxCandidateWidth, fCandidateWidthVec[i]);
        }
        float maxAnnotDescWidth = 0.;
        if (fShowAnnotDesc) {
            font.GetStringWidths(fAnnotDescVec.data(), 
                fAnnotDescLengthVec.data(), fCandidateCount, fAnnotDescWidthVec.data());
            for (int i = 0; i < fCandidateCount; ++i) {
                maxAnnotDescWidth = std::max(maxAnnotDescWidth, fAnnotDescWidthVec[i]);
            }
        }
        fColumnWidth.candidate = maxCandidateWidth;
        fColumnWidth.annotDesc = maxAnnotDescWidth;
        if (maxAnnotDescWidth < 1.) {
            // no annotation description, no need to keep area for them
            fShowAnnotDesc = false;
        }
    }
    
    // Calculate window size and resize it.
    {
        float lineHeight = fLineHeight;
        float totalHeight = lineHeight * fCandidateCount + border * 2;
        if (fShowPagination) {
            totalHeight += lineHeight;
        }
        float totalWidth = fColumnWidth.candidate + 
                                fColumnWidth.annotDesc + border * 2;
        if (fShowShortcut) {
            totalWidth += fColumnWidth.shortcut;
        }
        if (fShowAnnotDesc) {
            // separation between candidate and annot desc
            totalWidth += fFullwidthSpaceWidth;
        } else {
            totalWidth += fFullwidthSpaceWidth * 0.25;
        }
        if (fShowPagination) {
            // allow " 100/900 "
            totalWidth = std::max(totalWidth, fShortcutCharWidth * 8);
        }
        totalHeight = ceilf(totalHeight);
        totalWidth = ceilf(totalWidth);
        BSize size = Window()->Size();
        if (size.height != totalHeight || 
            size.width != totalWidth) {
            Window()->ResizeTo(totalWidth, totalHeight);
        }
    }
}

void CandidateView::Draw(BRect)
{
    std::lock_guard<std::mutex> lock(fDataMutex);
    if (!fOutput || !fOutput->has_candidates() || 
        fOutput->candidates().candidate_size() == 0) {
        Window()->Hide();
        return;
    }
    _DrawVertical();
}

void CandidateView::_DrawVertical()
{
    // [Shortcut [Space]] Candidate [FullwidthSpace [Attribute]] [Margin?]
    rgb_color highColor = HighColor();
    static const rgb_color borderColor = {0xB0, 0xB0, 0xB0, 1};
    static const rgb_color shortcutBackColor = {0xF2, 0xF2, 0xF2, 1};
    static const rgb_color annotDescColor = {0x90, 0x90, 0x90, 1};
    
    BFont font;
    GetFont(&font);
    float lineHeight;
    float ascent;
    {
        font_height fontHeight;
        font.GetHeight(&fontHeight);
        ascent = ceilf(fontHeight.ascent);
        lineHeight = ceilf(fontHeight.ascent + fontHeight.descent + 
                           fontHeight.leading);
        fLineHeight = lineHeight;
    }
    float firstLineY = border + ascent;
    
    const mozc::commands::Candidates &candidates = fOutput->candidates();
    
    // can we check existance of the shortcut by checking the first entry?
    const float shortcutAreaWidth = fShortcutCharWidth * 2.5;
    
    _CalculateSize();
    
    float width = Frame().Width();
    float height = Frame().Height();
    
    // draw shortcut background and clear 
    {
        BRect areaRect(border, border, 
                       width - border * 2, height - border * 2);
        if (fShowShortcut) {
            SetHighColor(shortcutBackColor);
            FillRect(BRect(border, border, 
                           shortcutAreaWidth, lineHeight * fCandidateCount));
            SetHighColor(highColor);
            areaRect.left = shortcutAreaWidth; // remove shortcut area
        }
        FillRect(areaRect, B_SOLID_LOW); // clear all
    }
    
    // todo, draw border with border color from the system?
    // draw outer border to increase visibility
    SetPenSize(border);
    SetHighColor(borderColor);
    StrokeRect(Frame());
    // todo, draw pagination separator here?
    SetHighColor(highColor);
    
    // draw candidates
    {
        float y = firstLineY;
        BPoint pos(fColumnWidth.shortcut, y);
        BPoint shortcutPos((shortcutAreaWidth - fShortcutCharWidth) * 0.5, y);
        for (int i = 0; i < fCandidateCount; i++) {
            if (fShowShortcut) {
                // todo, cash shortcut width
                shortcutPos.x = (shortcutAreaWidth - font.StringWidth(fShortcutVec[i], 1)) * 0.5;
                DrawString(fShortcutVec[i], 1, shortcutPos);
            }
            DrawString(fCandidateVec[i], fCandidateLengthVec[i], pos);
            y += lineHeight;
            pos.y = y;
            shortcutPos.y = y;
        }
    }
    
    // draw attribute
    if (fShowAnnotDesc) {
        SetHighColor(annotDescColor);
        float y = firstLineY;
        BPoint annotDescPos(fColumnWidth.shortcut + 
                            fColumnWidth.candidate + fFullwidthSpaceWidth, 
                            y);
        for (int i = 0; i < fCandidateCount; i++) {
            DrawString(fAnnotDescVec[i], fAnnotDescLengthVec[i], annotDescPos);
            y += lineHeight;
            annotDescPos.y = y;
        }
        //SetHighColor(highColor);
    }

    // draw backgrond for selected item
    if (candidates.has_focused_index()) {
        uint32 selected_index = fCandidateCount;
        uint32 focused_index = candidates.focused_index();
        for (int i = 0; i < candidates.candidate_size(); ++i) {
            if (candidates.candidate(i).index() == focused_index) {
                selected_index = i;
                break;
            }
        }
        if (selected_index < fCandidateCount) {
            source_alpha srcAlpha;
            alpha_function alphaFunc;
            GetBlendingMode(&srcAlpha, &alphaFunc);
            drawing_mode drawingMode = DrawingMode();
            SetDrawingMode(B_OP_ALPHA);
            SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
            
            float y = lineHeight * selected_index;
            BRect selectedRect = BRect(border, y + border, 
                                       width - border, y + lineHeight + border);
            SetHighColor(0x90, 0x90, 0xFF, 15);
            FillRect(selectedRect);
            SetHighColor(0x90, 0x90, 0xFF, 65);
            StrokeRect(selectedRect);
            //SetHighColor(highColor);
            
            SetBlendingMode(srcAlpha, alphaFunc);
            SetDrawingMode(drawingMode); // set to previous mode
        }
    }
    // draw pagination
    if (fShowPagination) {
        float y = lineHeight * fCandidateCount;
        // Draw separator line between candidates and pagination.
        SetHighColor(borderColor);
        StrokeLine(BPoint(border, y), 
                   BPoint(width - border, y));
        
        // Construct pagination, such as 1/10.
        std::ostringstream s;
        s << (candidates.focused_index() + 1) << '/' << candidates.size();
        std::string pagination = s.str();
        
        // calculate pagination width and draw it
        float paginationWidth = font.StringWidth(
                                    pagination.c_str(), pagination.length());
        BPoint point(width - paginationWidth - border - fFullwidthSpaceWidth * 0.25, 
                     y + ascent + border);
        SetHighColor(highColor);
        DrawString(pagination.c_str(), pagination.length(), point);
    }
    //SetHighColor(highColor);
}


CandidateWindow::CandidateWindow(MozcLooper *looper)
    : BWindow(
        BRect(150, 150, 200, 250), 
        "CandidateWindow", 
        B_NO_BORDER_WINDOW_LOOK,
        B_FLOATING_ALL_WINDOW_FEEL,
        B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE |
        B_NOT_MINIMIZABLE | B_NOT_MOVABLE |
        B_AVOID_FOCUS |
        B_NOT_ANCHORED_ON_ACTIVATE),
     fMozcLooper(looper),
     fCursorIndex(0),
     fLastLocation(BPoint(0, 0))
{
    fCandidateView = new CandidateView(Bounds());
    AddChild(fCandidateView);
    
    Run();
    fCandidateView->Init();
#if DEBUG
    fLogger = std::unique_ptr<BMessenger>(new BMessenger(LOGGER));
#endif
}

#if DEBUG
void CandidateWindow::_SendLog(const char *s)
{
    if (!fLogger->IsValid()) {
        if (fLogger->SetTo(LOGGER) != B_OK) {
            return;
        }
    }
    BMessage msg(LOG_COMMAND);
    msg.AddString(LOG_NAME, s);
    fLogger->SendMessage(&msg);
}
#endif

void CandidateWindow::SetData(std::unique_ptr<mozc::commands::Output> output)
{
    fCandidateView->SetData(std::move(output));
}

void CandidateWindow::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case IM_CANDIDATE_WINDOW_SHOW:
        {
            if (IsHidden()) {
                SetWorkspaces(B_CURRENT_WORKSPACE);
                // move window to the specified location
                // todo, we should not show before view updated or window moved.
                // So we need flag?
                Show();
            }
            break;
        }
        case IM_CANDIDATE_WINDOW_HIDE:
        {
            if (!IsHidden()) {
                Hide();
            }
            break;
        }
        case CANDIDATE_WINDOW_UPDATE:
        {
            fCandidateView->MessageReceived(msg);
            break;
        }
        case CANDIDATE_WINDOW_CURSOR_INDEX:
        {
            fCursorIndex = (uint32)msg->GetInt32(CANDIDATE_WINDOW_CURSOR_NAME, 0);
            break;
        }
        case B_INPUT_METHOD_EVENT:
        {
            uint32 opcode = msg->GetInt32("be:opcode", 0);
            if (opcode == B_INPUT_METHOD_LOCATION_REQUEST) {
                // move window position to the start of selected char
                // be:height_reply: float, be:location_reply: BPoint
                BPoint point;
                float height;
                if (msg->FindPoint("be:location_reply", fCursorIndex, &point) == B_OK &&
                    msg->FindFloat("be:height_reply", fCursorIndex, &height) == B_OK) {
                    point.y += height + 1.;
                    fLastLocation = point;
                }
                float x = fLastLocation.x - fCandidateView->GetValueLeft();
                MoveTo(x >= 0 ? x : 0,
                       fLastLocation.y);
            }
            break;
        }
        case IM_SELECT_CANDIDATE:
        {
            fMozcLooper->PostMessage(msg);
            break;
        }
        case IM_HIGHLIGHT_CANDIDATE:
        {
            fMozcLooper->PostMessage(msg);
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

