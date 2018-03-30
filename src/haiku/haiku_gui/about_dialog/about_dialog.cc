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

#include "haiku/haiku_gui/about_dialog/about_dialog.h"
#include "haiku/haiku_gui/base/mozc_tool_app.h"
#include "haiku/haiku_gui/base/cstring_view.h"

#include "base/const.h"
#include "base/file_util.h"
#include "base/process.h"
#include "base/system_util.h"
#include "base/version.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Bitmap.h>
#include <Button.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <TranslationUtils.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "about_dialog"

namespace mozc {
namespace haiku_gui {

class LogoView : public BView
{
public:
    LogoView(const char* version);
    virtual ~LogoView();

    virtual void Draw(BRect rect);
    virtual BSize PreferredSize() { return fLogoSize; }
    void SetWidth(float width);
private:
    BBitmap*        fLogoBitmap;
    BSize           fLogoSize;
    const char*     fVersion;

    BBitmap* _GetLogo();
};

LogoView::LogoView(const char* version)
    : BView(
        BRect(0, 0, 0, 0),
        "logo",
        B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
        B_WILL_DRAW),
      fLogoBitmap(NULL)
{
    fVersion = version;
    fLogoBitmap = _GetLogo();
    if (fLogoBitmap) {
        fLogoSize = fLogoBitmap->Bounds().Size();
        ResizeTo(BSize(Bounds().Width(), fLogoSize.height));
    }
}

LogoView::~LogoView(void)
{
    if (fLogoBitmap != NULL) {
        delete fLogoBitmap;
        fLogoBitmap = NULL;
    }
}

BBitmap* LogoView::_GetLogo()
{
    BPath path;
    if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK) {
        path.Append("mozc/images/product_icon_32bpp-128.png");
        BBitmap* bitmap = BTranslationUtils::GetBitmap(path.Path());
        if (bitmap) {
            return bitmap;
        }
    }
    path.Unset();
    if (find_directory(B_USER_DATA_DIRECTORY, &path) == B_OK) {
        path.Append("mozc/images/product_icon_32bpp-128.png");
        BBitmap* bitmap = BTranslationUtils::GetBitmap(path.Path());
        if (bitmap) {
            return bitmap;
        }
    }
    path.Unset();
    if (find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, &path) == B_OK) {
        path.Append("mozc/images/product_icon_32bpp-128.png");
        BBitmap* bitmap = BTranslationUtils::GetBitmap(path.Path());
        if (bitmap) {
            return bitmap;
        }
    }
    return NULL;
}

void LogoView::SetWidth(float width)
{
    fLogoSize.width = width;
}

void LogoView::Draw(BRect rect)
{
    const rgb_color highColor = HighColor();
    SetHighColor(0xFF, 0xFF, 0xFF);
    FillRect(Frame());

    SetHighColor(0, 0, 0);
    // draw name and version
    BFont font;
    GetFont(&font);
    const float size = font.Size();

    DrawString(fVersion, BPoint(6, size * 4.5));
    font.SetSize(size * 1.5);
    SetFont(&font);
    DrawString(mozc::kProductNameInEnglish, BPoint(6, size * 2.5));
    font.SetSize(size);
    SetFont(&font);

    SetHighColor(highColor);

    if (fLogoBitmap != NULL) {
        // align right
        BRect size = Bounds();
        BRect logoSize = fLogoBitmap->Bounds();
        BPoint point(size.Width() - logoSize.Width() - 5, 0);
        DrawBitmap(fLogoBitmap, point);
    }

    BView::Draw(rect);
}

class LinkedLabel : public BView
{
public:
    typedef struct _Range {
        std::string label;
        std::string link;
        float width;
    } Range;

    LinkedLabel(const char* label);
    virtual ~LinkedLabel();

    virtual void Draw(BRect rect);
    virtual BSize PreferredSize() { return fSize; }
    virtual BSize MinSize() { return fSize; }
    virtual void MouseDown(BPoint pos);
    virtual void MouseMoved(BPoint pos, uint32 code, const BMessage* drag);
private:
    std::vector<Range*> maRanges;
    BSize fSize;

    void _Parse(const char* label);
    Range* _FindRange(const char* label, int32 start, int32* end);
    Range* _FindLabel(const char* label, int32 start, int32* end);
    int32 _InLink(BPoint pos);
    std::string _GetLink(const std::string& link);
};

LinkedLabel::LinkedLabel(const char* label)
    : BView(
        BRect(0, 0, 0, 0),
        "label",
        B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
        B_WILL_DRAW | B_SUPPORTS_LAYOUT)
{
    _Parse(label);
    SetDrawingMode(B_OP_OVER);
    DisableLayoutInvalidation();
    // resizing is strange
    ResizeTo(fSize.width + 10, fSize.height);
}

LinkedLabel::~LinkedLabel()
{
    for (int32 i = 0; i < maRanges.size(); ++i) {
        delete maRanges[i];
        maRanges[i] = NULL;
    }
    maRanges.clear();
}

void LinkedLabel::Draw(BRect rect)
{
    SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    const float y = float(Bounds().Height() * 0.75);
    float x = 0;
    for (int32 i = 0; i < maRanges.size(); ++i) {
        Range* r = maRanges[i];
        if (r) {
            BPoint pos(x, y);
            bool hasLink = !r->link.empty();
            rgb_color color = hasLink ?
                    ui_color(B_LINK_TEXT_COLOR) : ui_color(B_CONTROL_TEXT_COLOR);
            SetHighColor(color);
            DrawString(r->label.c_str(), pos);
            if (hasLink) {
                StrokeLine(BPoint(x, y + 2), BPoint(x + r->width, y + 2));
            }
            x += r->width;
        }
    }
}

void LinkedLabel::MouseDown(BPoint pos)
{
    BPoint point;
    uint32 buttons;
    if (get_mouse(&point, &buttons) == B_OK) {
        if (buttons & B_PRIMARY_MOUSE_BUTTON) {
            const int32 index = _InLink(pos);
            if (0 <= index && index < maRanges.size()) {
                Range* r = maRanges[index];
                if (r) {
                    if (r->link.find("file://") != std::string::npos)  {
                        mozc::Process::OpenBrowser(_GetLink(r->link));
                    } else {
                        mozc::Process::OpenBrowser(r->link);
                    }
                }
            }
        }
    }
}

std::string LinkedLabel::_GetLink(const std::string& link)
{
    if (link.find("file://") == std::string::npos) {
        return "";
    }
    std::string ret("mozc/");
    ret += link.substr(7);
    BPath path;
    if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK) {
        path.Append(ret.c_str());
        BEntry entry(path.Path());
        if (entry.Exists()) {
            ret = "file://";
            ret += path.Path();
            return ret;
        }
    }
    path.Unset();
    if (find_directory(B_USER_DATA_DIRECTORY, &path) == B_OK) {
        path.Append(ret.c_str());
        BEntry entry(path.Path());
        if (entry.Exists()) {
            ret = "file://";
            ret += path.Path();
            return ret;
        }
    }
    path.Unset();
    if (find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, &path) == B_OK) {
        path.Append(ret.c_str());
        BEntry entry(path.Path());
        if (entry.Exists()) {
            ret = "file://";
            ret += path.Path();
            return ret;
        }
    }
    return NULL;
}

void LinkedLabel::MouseMoved(BPoint pos, uint32 code, const BMessage* drag)
{
    const int32 inLink = _InLink(pos);
    BCursor cursor(inLink != -1 ? B_CURSOR_ID_FOLLOW_LINK : B_CURSOR_ID_SYSTEM_DEFAULT);
    SetViewCursor(&cursor, true);
}

int32 LinkedLabel::_InLink(BPoint pos)
{
    if (maRanges.size() <= 0) {
        return false;
    }
    const float x = pos.x;
    float left = 0;
    float right = 0;
    
    for (int32 i = 0; i < maRanges.size(); ++i) {
        Range* r = maRanges[i];
        if (r) {
            right = left + r->width;
            if (left <= x && x <= right) {
                if (!r->link.empty()) {
                    return i;
                }
                return -1;
            }
            left = right;
        }
    }
    return -1;
}

void LinkedLabel::_Parse(const char* label)
{
    float width = 0;

    int32 start = 0;
    int32 end = start;
    
    Range* r = NULL;
    while (true) {
        r = _FindLabel(label, start, &end);
        if (r) {
            maRanges.push_back(r);
            width += r->width;
            start = end;
            end = start;
        }
        r = _FindRange(label, start, &end);
        if (r) {
            maRanges.push_back(r);
            width += r->width;
            start = end + 1;
            end = start;
        } else {
            break;
        }
    }

    font_height height;
    GetFontHeight(&height);
    fSize = BSize(width, floor((height.ascent + height.descent) * 1.2));
}

LinkedLabel::Range* LinkedLabel::_FindLabel(const char* label, int32 start, int32* end)
{
    int32 n = start;
    char c = label[n];
    while (c != '\0') {
        if (c == '[') {
            Range* r = new Range();
            r->label = std::string(label, start, n - start);
            r->width = StringWidth(r->label.c_str());
            *end = n - 1;
            return r;
        }
        n += 1;
        c = label[n];
    }
    if (start != n) {
        Range* r = new Range();
        r->label = std::string(label, start, n - start);
        r->width = StringWidth(r->label.c_str());
        *end = n - 1;
        return r;
    }
    return NULL;
}

LinkedLabel::Range* LinkedLabel::_FindRange(const char* label, int32 start, int32* end)
{
    Range* r = NULL;
    int32 labelStart = start;
    int32 labelEnd = -1;
    int32 linkStart = start;
    int32 linkEnd = -1;
    
    char s = '[';
    int32 n = start;
    char c = label[n];
    while (c != '\0') {
        if (c == s) {
            if (s == '[') {
                labelStart = n + 1;
                s = '|';
            } else if (s == '|') {
                labelEnd = n - 1;
                linkStart = n + 1;
                s = ']';
            } else if (s == ']') {
                linkEnd = n - 1;
                *end = n;
                break;
            }
        }
        n += 1;
        c = label[n];
    }
    if (labelEnd >= 0 && linkEnd >= 0) {
        r = new Range();
        r->label = std::string(label, labelStart, labelEnd - labelStart + 1);
        r->link = std::string(label, linkStart, linkEnd - linkStart + 1);
        r->width = StringWidth(r->label.c_str());
    }
    return r;
}


class AboutWindow : public BWindow
{
public:
    AboutWindow(const char* version);
    virtual ~AboutWindow();

    virtual void MessageReceived(BMessage* msg);
private:
    enum Actions
    {
        OK = 'btok',
        CREDITS = 'btcr',
        PRODUCT_INFO = 'btpi',
        ISSUES = 'btis',
    };
    LogoView*       fLogoView;
};


AboutWindow::AboutWindow(const char* version)
    : BWindow(
        BRect(50, 50, 490, 250),
        B_TRANSLATE("About Mozc"),
        B_TITLED_WINDOW_LOOK,
        B_NORMAL_WINDOW_FEEL,
        B_AUTO_UPDATE_SIZE_LIMITS |
        B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE | B_QUIT_ON_WINDOW_CLOSE)
{
    fLogoView = new LogoView(version);

    BButton* okButton = new BButton("ok", "OK", new BMessage(OK));

    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .Add(fLogoView)
        .AddGroup(B_VERTICAL)
        .SetInsets(6, 0, 6, 6)
            .Add(new CStringView("copyright", "Copyright \xc2\xa9 2018 Google Inc. All Rights Reserved."))
            .Add(new LinkedLabel(B_TRANSLATE("Mozc is made possible by [open source software|file://credits_en.html]")))
            .Add(new LinkedLabel(B_TRANSLATE("Mozc [product information|https://github.com/google/mozc] [issues|https://github.com/google/mozc/issues]")))
            .AddGroup(B_HORIZONTAL)
                .AddGlue()
                .Add(okButton)
            .End()
        .End();
    Layout(true);

    okButton->MakeFocus(true);
}

AboutWindow::~AboutWindow()
{
    fLogoView = NULL;
}

void AboutWindow::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case OK:
        {
            Quit();
            break;
        }
        default:
        {
            BWindow::MessageReceived(msg);
            break;
        }
    }
}


class AboutApp : public ToolApp
{
public:
    AboutApp();
    virtual ~AboutApp() {}
};

AboutApp::AboutApp()
    : ToolApp(ABOUT_DIALOG)
{
    std::string version_info = "(";
    version_info += mozc::Version::GetMozcVersion().c_str();
    version_info += ")";

    mpWindow = new AboutWindow(version_info.c_str());
    mpWindow->CenterOnScreen();
    mpWindow->Show();
}

} // namespace haiku_gui
} // namespace mozc

int HaikuRunAboutDialog(int argc, char* argv[])
{
    if (mozc::haiku_gui::ToolApp::ActivateIfExists(
                                mozc::haiku_gui::ABOUT_DIALOG)) {
        return -1;
    }

    mozc::haiku_gui::AboutApp* app = new mozc::haiku_gui::AboutApp();
    app->Run();
    delete app;

    return 0;
}
