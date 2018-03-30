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

#include "haiku/haiku_gui/base/compatible.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <Spinner.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "compatible"

int32 QMessageBox::message(const char* title,
                        const char* message, 
                        int32 button,
                        int32 focus, alert_type type)
{
    int32 nOk = -1;
    int32 nCancel = -1;
    const char* labelOk = "";
    const char* labelCancel = "";
    if (button & QMessageBox::Ok) {
        labelOk = B_TRANSLATE("OK");
        nOk = QMessageBox::Ok;
    } else if (button & QMessageBox::Yes) {
        labelOk = B_TRANSLATE("YES");
        nOk = QMessageBox::Yes;
    }
    if (button & QMessageBox::Cancel) {
        labelCancel = B_TRANSLATE("Cancel");
        nCancel = QMessageBox::Cancel;
    } else if (button & QMessageBox::No) {
        labelCancel = B_TRANSLATE("NO");
        nCancel = QMessageBox::No;
    }
    BAlert* alert = new BAlert(title, message, 
            "", labelCancel, labelOk,
            B_WIDTH_AS_USUAL, B_OFFSET_SPACING, type);
    alert->SetLook(B_TITLED_WINDOW_LOOK);
    uint32 flags = alert->Flags();
    flags ^= B_NOT_CLOSABLE;
    alert->SetFlags(flags | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE);
    alert->SetShortcut(0, B_ESCAPE);
    if ((button & QMessageBox::Ok) && (focus & QMessageBox::Ok)) {
        BButton* button = alert->ButtonAt(0);
        if (button) {
            button->MakeFocus(true);
        }
    }
    if ((button & QMessageBox::Cancel) && (focus & QMessageBox::Cancel)) {
        BButton* button = alert->ButtonAt(1);
        if (button) {
            button->MakeFocus(true);
        }
    }

    const int32 index = alert->Go();
    if (index == 1) {
        return nOk;
    } else if (index == 0) {
        return nCancel;
    } else {
        return -1;
    }
}

int32 QMessageBox::information(void* view, const char* title,
                        const char* message, 
                        int32 button, int32 focus)
{
    return QMessageBox::message(title, message,
                        button, focus, B_INFO_ALERT);
}

int32 QMessageBox::question(void* view, const char* title,
                        const char* message, 
                        int32 button, int32 focus)
{
    return QMessageBox::message(title, message,
                        button, focus, B_INFO_ALERT);
}

int32 QMessageBox::warning(void* view, const char* title,
                        const char* message, 
                        int32 button, int32 focus)
{
    return QMessageBox::message(title, message,
                        button, focus, B_WARNING_ALERT);
}

int32 QMessageBox::critical(void* view, const char* title,
                        const char* message, 
                        int32 button, int32 focus)
{
    return QMessageBox::message(title, message,
                        button, focus, B_STOP_ALERT);
}

InputDialog::InputDialog(BLooper* looper, BMessage* message,
                    const char* title, const char* label,
                    const char* text)
    : BWindow(
        BRect(0, 0, 100, 100),
        title,
        B_MODAL_WINDOW,
        B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE |
        B_AUTO_UPDATE_SIZE_LIMITS),
      mpLooper(looper),
      mpMessage(message)
{
    SetLook(B_TITLED_WINDOW_LOOK);
    mpLabel = new BStringView("label", label);
    mpText = new BTextControl("edit", "", text, NULL);
    mpOkButton = new BButton(B_TRANSLATE("OK"), new BMessage(OK));
    mpCancelButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(CANCEL));
    
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .Add(mpLabel)
        .Add(mpText)
        .AddGroup(B_HORIZONTAL)
            .Add(mpOkButton)
            .Add(mpCancelButton)
        .End();
    Layout(true);
    CenterOnScreen();
    mpText->MakeFocus(true);
    
    SetDefaultButton(mpOkButton);
}

InputDialog::~InputDialog()
{
}

void InputDialog::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case OK:
        {
            if (mpLooper && mpMessage) {
                BMessage message(*mpMessage);
                message.AddString("text", mpText->Text());
                mpLooper->PostMessage(&message);
            }
            Quit();
            break;
        }
        case CANCEL:
        {
            Quit();
            break;
        }
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}


QByteArray::QByteArray()
{
}

QByteArray::QByteArray(vector<uint8> v)
{
    v_ = v;
}

QByteArray::~QByteArray()
{
}

size_t QByteArray::size() const
{
    return v_.size();
}

uint8 QByteArray::operator[](size_t n)
{
    return v_[n];
}

QString::QString(const char* s)
{
    if (s) {
        s_ = s;
    } else {
        s_ = "";
    }
}

QString::QString(const QString &s)
{
    s_ = s.s_;
}

QString::QString()
{
    s_ = "";
}

QString::QString(std::string& s)
{
    s_ = s;
}

QString::~QString()
{
}

bool QString::isEmpty() const
{
    return s_.empty();
}

int QString::count() const
{
    return s_.size();
}

void QString::sprintf(const char* format, ...)
{
    char s[64];
    va_list args;
    va_start(args, format);
    ::vsprintf(s, format, args);
    va_end(args);
    s_ += static_cast<const char*>(s);
}

std::string QString::toStdString() const
{
    return std::string(s_);
}

QByteArray QString::toUtf8() const
{
    vector<uint8> v;
    const char* p = s_.c_str();
    for (int32 i = 0; i < s_.size(); ++i) {
        v.push_back(*p);
        p++;
    }
    QByteArray a(v);
    return a;
}

QString QString::fromUtf8(const char* p)
{
    return QString(p);
}

QString& QString::operator+=(const QString& s)
{
    s_ += s.get().c_str();
    return *this;
}

void QString::clear()
{
    s_.clear();
}

QString QString::arg(const char* v)
{
    return _arg(v);
}

QString QString::arg(int32 n)
{
    return _arg(std::to_string(n).c_str());
}

QString QString::_arg(const char* v)
{
    char current = ':';
    string::size_type lastPos = string::npos;
    string::size_type pos = 0;
    while (pos != string::npos) {
        pos = s_.find("%", pos);
        if (pos != string::npos && pos + 1 < s_.size()) {
            char& c = s_.at(pos + 1);
            if ('1' <= c && c <= '9' && c <= current) {
                current = c;
                lastPos = pos;
            }
            pos += 2;
        } else {
            break;
        }
    }
    if (current != ':' && lastPos != string::npos) {
        std::string f = s_.substr(0, lastPos);
        f += v;
        f += s_.substr(lastPos + 2);
        return QString(f);
    }
    return QString(this->getp());
}


CompButton::CompButton(BButton *p)
{
    p_ = p;
}

CompButton::~CompButton()
{
    p_ = nullptr;
}

void CompButton::setEnabled(bool enabled)
{
    if (p_) {
        p_->SetEnabled(enabled);
    }
}

void CompButton::setVisible(bool visible)
{
    if (p_) {
        if (visible) {
            p_->Show();
        } else {
            p_->Hide();
        }
    }
}

CompCheckBox::CompCheckBox(BCheckBox* p)
{
    p_ = p;
}

CompCheckBox::~CompCheckBox()
{
    p_ = nullptr;
}

bool CompCheckBox::isChecked()
{
    if (p_) {
        return p_->Value() == B_CONTROL_ON;
    }
    return false;
}

void CompCheckBox::setChecked(bool checked)
{
    if (p_) {
        p_->SetValue(checked ? B_CONTROL_ON : B_CONTROL_OFF);
    }
}

void CompCheckBox::setEnabled(bool enabled)
{
    if (p_) {
        p_->SetEnabled(enabled);
    }
}

void CompCheckBox::setDisabled(bool disabled)
{
    if (p_) {
        p_->SetEnabled(!disabled);
    }
}

void CompCheckBox::setVisible(bool visible)
{
    if (p_) {
        if (visible) {
            p_->Show();
        } else {
            p_->Hide();
        }
    }
}

CompEdit::CompEdit(BTextControl* p)
{
    p_ = p;
}

CompEdit::~CompEdit()
{
    p_ = nullptr;
}

void CompEdit::setFocus(Qt::FocusReason reason)
{
    if (p_) {
        p_->MakeFocus(true);
    }
}

QString CompEdit::text() const
{
    if (p_) {
        return QString(p_->Text());
    } else {
        return QString("");
    }
}

void CompEdit::setText(const QString &s)
{
    if (p_) {
        p_->SetText(s.get().c_str());
    }
}

void CompEdit::setText(const char* s)
{
    if (p_) {
        p_->SetText(s);
    }
}

void CompEdit::selectAll()
{
    if (p_) {
        p_->TextView()->SelectAll();
    }
}

void CompEdit::setMaxLength(int32 length)
{
    if (p_) {
        p_->TextView()->SetMaxBytes(length);
    }
}


CompComboBox::CompComboBox(BPopUpMenu* p)
{
    p_ = p;
}

CompComboBox::~CompComboBox()
{
    p_ = nullptr;
}

int CompComboBox::currentIndex() const
{
    if (p_) {
        return p_->FindMarkedIndex();
    } else {
        return -1;
    }
}

void CompComboBox::setCurrentIndex(int index)
{
    if (p_) {
        BMenuItem* item = p_->ItemAt(index);
        if (item) {
            item->SetMarked(true);
            item->InvokeNotify(item->Message());
        }
    }
}

QString CompComboBox::currentText() const
{
    if (p_) {
        BMenuItem* item = p_->FindMarked();
        if (item) {
            return QString(item->Label());
        }
    }
    return QString("");
}

void CompComboBox::addItem(const char* label)
{
    if (p_) {
        p_->AddItem(new BMenuItem(label, new BMessage()));
    }
}

void CompComboBox::selectFirst()
{
    if (p_) {
        if (p_->CountItems() > 0) {
            p_->ItemAt(0)->SetMarked(true);
        }
    }
}

CompSpinBox::CompSpinBox(BSpinner* p)
    : p_(p)
{
}

CompSpinBox::~CompSpinBox()
{
    p_ = nullptr;
}

int32 CompSpinBox::value() const
{
    if (p_) {
        return p_->Value();
    }
    return 0;
}

void CompSpinBox::setValue(int32 n)
{
    if (p_) {
        p_->SetValue(n);
    }
}

void CompSpinBox::setRange(int32 min, int32 max)
{
    if (p_) {
        p_->SetRange(min, max);
    }
}


QAbstractButton::QAbstractButton(BButton* p)
{
    p_ = p;
}

QAbstractButton::~QAbstractButton()
{
    p_ = nullptr;
}

void QAbstractButton::setEnabled(bool enabled)
{
    if (p_) {
        p_->SetEnabled(enabled);
    }
}


QDialogButtonBox::QDialogButtonBox(BButton* ok, BButton* cancel, BButton* apply)
{
    okButton = new QAbstractButton(ok);
    cancelButton = new QAbstractButton(cancel);
    applyButton = new QAbstractButton(apply);
}

QDialogButtonBox::~QDialogButtonBox()
{
    delete okButton;
    delete cancelButton;
    delete applyButton;
}

QAbstractButton* QDialogButtonBox::button(ButtonType type)
{
    if (type == QDialogButtonBox::Ok) {
        return okButton;
    } else if (type == QDialogButtonBox::Cancel) {
        return cancelButton;
    } else if (type == QDialogButtonBox::Apply) {
        return applyButton;
    }
    return nullptr;
}

QListWidget::QListWidget(BListView* p)
    : p_(p)
{
}

int32 QListWidget::count() const
{
    if (p_) {
        return p_->CountItems();
    }
    return 0;
}

void QListWidget::clear()
{
    if (p_) {
        p_->MakeEmpty();
    }
}

void QListWidget::setCurrentRow(int32 n)
{
    if (p_) {
        p_->Select(n, 0, false);
    }
}

void QListWidget::repaint()
{
    if (p_) {
        if (p_->LockLooper()) {
            p_->Invalidate();
            p_->UnlockLooper();
        }
    }
}

QDialogButtonBox::ButtonRole QDialogButtonBox::buttonRole(QAbstractButton* button)
{
    if (button == okButton) {
        return QDialogButtonBox::AcceptRole;
    }/* else if (button == cancelButton) {
        // todo
    }*/
    return QDialogButtonBox::None;
}

QPushButton::QPushButton(BButton* btn)
    : p_(btn)
{
}

void QPushButton::setEnabled(bool enabled)
{
    if (p_) {
        p_->SetEnabled(enabled);
    }
}



QAction::QAction(BMenuItem* item)
    : mpItem(item)
{
}

QAction::~QAction()
{
    mpItem = NULL;
}

void QAction::setEnabled(bool enabled)
{
    if (mpItem) {
        mpItem->SetEnabled(enabled);
    }
}

QStatusBar::QStatusBar(BStringView* p)
    : p_(p)
{
}

void QStatusBar::showMessage(const QString& s)
{
    if (p_) {
        p_->SetText(s.getp());
    }
}



class ProgressView : public BView
{
public:
    ProgressView(int32 max, int32 start=0);
    virtual ~ProgressView();
    
    virtual void Draw(BRect rect);
    virtual BSize MinSize();
    virtual BSize PreferredSize();
    
    void Increment();
    void SetValue(int32 n);
private:
    int32 mnMax;
    int32 mnCurrent;
};

ProgressView::ProgressView(int32 max, int32 start)
    : BView(
        BRect(0, 0, 10, 10),
        "progress", 0,
        B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      mnMax(max),
      mnCurrent(start)
{
    if (mnCurrent > mnMax) {
        mnCurrent = mnMax;
    }
}

ProgressView::~ProgressView()
{
}

BSize ProgressView::MinSize()
{
    return BSize(200.0f, 30.0f);
}

BSize ProgressView::PreferredSize()
{
    return BSize(200.0f, 30.0f);
}

void ProgressView::Draw(BRect rect)
{
    be_control_look->DrawBorder(this, rect, rect,
            ui_color(B_CONTROL_BORDER_COLOR), B_PLAIN_BORDER);
    SetHighColor(ui_color(B_STATUS_BAR_COLOR));
    BRect progress(1, 1, rect.Width() / mnMax * mnCurrent + 1, rect.Height() + 1);
    FillRect(progress);
}

void ProgressView::Increment()
{
    mnCurrent += 1;
    if (mnCurrent > mnMax) {
        mnCurrent = mnMax;
    }
}

void ProgressView::SetValue(int32 n)
{
    mnCurrent = n;
    if (mnCurrent > mnMax) {
        mnCurrent = mnMax;
    }
}

class ProgressDialog : public BWindow
{
public:
    enum {
        UPDATE = 'updt',
    };
    ProgressDialog(const char* label, int32 max);
    virtual ~ProgressDialog();
    
    void Increment();
    void SetValue(int32 n);
    
    virtual void MessageReceived(BMessage* msg);
private:
    BStringView* mpLabel;
    ProgressView* mpProgressView;
    BMessageRunner* mpRunner;
    BMessenger* mpMessenger;

    void _InitRunner();
};

ProgressDialog::ProgressDialog(const char* label, int32 max)
    : BWindow(
        BRect(),
        "",
        B_MODAL_WINDOW,
        B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_CLOSE_ON_ESCAPE |
        B_AUTO_UPDATE_SIZE_LIMITS),
      mpRunner(NULL),
      mpMessenger(NULL)
{
    mpLabel = new BStringView("label", label);
    mpProgressView = new ProgressView(max, 0);
    
    BLayoutBuilder::Group<>(this, B_VERTICAL)
        .SetInsets(5, 5, 5, 5)
        .Add(mpLabel)
        .Add(mpProgressView);
    Layout(true);
    CenterOnScreen();
}

ProgressDialog::~ProgressDialog()
{
    mpProgressView = NULL;
    if (mpRunner) {
        delete mpRunner;
        delete mpMessenger;
    }
}

void ProgressDialog::_InitRunner()
{
    mpMessenger = new BMessenger(NULL, this);
    mpRunner = new BMessageRunner(*mpMessenger,
                            new BMessage(UPDATE), 300000, -1);
    mpRunner->InitCheck();
}

void ProgressDialog::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case UPDATE:
        {
            if (mpProgressView && mpProgressView->LockLooper()) {
                mpProgressView->Invalidate();
                mpProgressView->UnlockLooper();
            }
            break;
        }
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

void ProgressDialog::Increment()
{
    if (mpProgressView) {
        mpProgressView->Increment();
        if (!mpRunner) {
            _InitRunner();
        }
    }
}

void ProgressDialog::SetValue(int32 n)
{
    if (mpProgressView) {
        mpProgressView->SetValue(n);
        if (!mpRunner) {
            _InitRunner();
        }
    }
}

ProgressDialogWrapper::ProgressDialogWrapper(const char* label, BWindow* parent, int32 max)
{
    mpDialog = new ProgressDialog(label, max);
    if (parent) {
        mpDialog->CenterIn(parent->Frame());
    }
    mpDialog->Show();
}

ProgressDialogWrapper::~ProgressDialogWrapper()
{
    if (mpDialog) {
        if (mpDialog->LockLooper()) {
            mpDialog->Quit();
        }
    }
    mpDialog = NULL;
}

ProgressDialogWrapper& ProgressDialogWrapper::operator++()
{
    if (mpDialog) {
        mpDialog->Increment();
    }
    return *this;
}

void ProgressDialogWrapper::setValue(int32 n)
{
    if (mpDialog) {
        mpDialog->SetValue(n);
    }
}

