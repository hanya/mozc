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

#ifndef COMPATIBLE_H
#define COMPATIBLE_H

#include <Alert.h>

#include <string>

class BButton;
class BCheckBox;
class BListView;
class BPopUpMenu;
class BSpinner;
class BStringView;
class BTextControl;

class QString;

class Qt
{
public:
    enum FocusReason
    {
        OtherFocusReason,
    };
    enum {
        WindowTitleHint,
        WindowSystemMenuHint,
    };
};

class QMessageBox
{
public:
    static const int32    Ok = 1;
    static const int32    Cancel = 2;
    static const int32    Yes = 4;
    static const int32    No = 8;

    static int32 message(const char* title, const char* message, 
            int32 button=Ok, int32 focus=Ok,
            alert_type type=B_EMPTY_ALERT);

    static int32 information(void* view, const char* title,
                        const char* message,
                        int32 button=Ok, int32 focus=Ok);
    static int32 question(void* view, const char* title,
                        const char* message,
                        int32 button=Ok, int32 focus=Ok);
    static int32 warning(void* view, const char* title,
                        const char* message,
                        int32 button=Ok, int32 focus=Ok);
    static int32 critical(void* view, const char* title,
                        const char* message,
                        int32 button=Ok, int32 focus=Ok);
};

class QLineEdit
{
public:
    enum {
        Normal,
    };
};

class InputDialog : public BWindow
{
public:
    enum {
        OK = 'btok',
        CANCEL = 'btcl',
    };
    InputDialog(BLooper* looper, BMessage* message,
                const char* title, const char* label, const char* text);
    virtual ~InputDialog();

    virtual void MessageReceived(BMessage* msg);
private:
    BStringView* mpLabel;
    BTextControl* mpText;
    BButton* mpOkButton;
    BButton* mpCancelButton;
    BLooper* mpLooper;
    BMessage* mpMessage;
};

class QByteArray
{
public:
    QByteArray();
    QByteArray(vector<uint8> v);
    virtual ~QByteArray();

    size_t size() const;
    uint8 operator[](size_t n);

    std::vector<uint8> get() const { return v_; };
private:
    std::vector<uint8> v_;
};

class QString
{
public:
    QString(const char* s);
    QString(const QString &s);
    QString();
    QString(std::string& s);
    virtual ~QString();

    bool isEmpty() const;
    int count() const;
    void sprintf(const char* format, ...);
    std::string toStdString() const;
    QByteArray toUtf8() const;
    static QString fromUtf8(const char* p);
    QString& operator+=(const QString& s);
    void clear();
    QString arg(const char* v);
    QString arg(int32 n);

    std::string get() const { return s_; };
    const char* getp() const { return s_.c_str(); };

private:
    std::string s_;
    QString _arg(const char* v);
};

class CompButton
{
public:
    CompButton(BButton* p);
    virtual ~CompButton();

    void setEnabled(bool enabled);
    void setVisible(bool visible);
private:
    BButton* p_;
};

class CompCheckBox
{
public:
    CompCheckBox(BCheckBox* p);
    virtual ~CompCheckBox();

    bool isChecked();
    void setChecked(bool checked);
    void setEnabled(bool enabled);
    void setDisabled(bool disabled);
    void setVisible(bool visible);
private:
    BCheckBox* p_;
};

class CompEdit
{
public:
    CompEdit(BTextControl* p);
    virtual ~CompEdit();

    void setFocus(Qt::FocusReason reason);
    QString text() const;
    void setText(const QString &s);
    void setText(const char* s);
    void selectAll();
    void setMaxLength(int32 length);
    
private:
    BTextControl* p_;
};

class CompComboBox
{
public:
    CompComboBox(BPopUpMenu* p);
    virtual ~CompComboBox();

    int currentIndex() const;
    void setCurrentIndex(int index);
    QString currentText() const;
    void addItem(const char* label);

    void selectFirst();
private:
    BPopUpMenu* p_;
};

#define QComboBox CompComboBox

class CompSpinBox
{
public:
    CompSpinBox(BSpinner* p);
    virtual ~CompSpinBox();

    int32 value() const;
    void setValue(int32 n);
    void setRange(int32 min, int32 max);
private:
    BSpinner* p_;
};

class QListWidget
{
public:
    QListWidget(BListView* p);
    virtual ~QListWidget() { p_ = NULL; };

    int32 count() const;
    void clear();
    void setCurrentRow(int32 n);
    void repaint();
private:
    BListView* p_;
};

class QAbstractButton
{
public:
    QAbstractButton(BButton* p);
    virtual ~QAbstractButton();

    void setEnabled(bool enabled);
private:
    BButton* p_;
};

class QDialogButtonBox
{
public:
    enum ButtonType
    {
        Ok = 1,
        Cancel = 2,
        Apply = 3,
    };
    enum ButtonRole
    {
        None,
        AcceptRole,
    };
    QDialogButtonBox(BButton* ok, BButton* cancel, BButton* apply=nullptr);
    virtual ~QDialogButtonBox();

    QAbstractButton* button(ButtonType type);
    ButtonRole buttonRole(QAbstractButton* button);
private:
    QAbstractButton* okButton;
    QAbstractButton* cancelButton;
    QAbstractButton* applyButton;
};

class QPushButton
{
public:
    QPushButton(BButton* btn);
    virtual ~QPushButton() { p_ = NULL; };

    void setEnabled(bool enabled);
private:
    BButton* p_;
};

class QAction
{
public:
    QAction(BMenuItem* item);
    virtual ~QAction();

    void setEnabled(bool enabled);
private:
    BMenuItem* mpItem;
};

class QStatusBar
{
public:
    QStatusBar(BStringView* p);
    virtual ~QStatusBar() { p_ = NULL; };

    void showMessage(const QString& s);
private:
    BStringView* p_;
};

class ProgressDialog;

class ProgressDialogWrapper
{
public:
    ProgressDialogWrapper(const char* label, BWindow* parent, int32 max);
    virtual ~ProgressDialogWrapper();
    ProgressDialogWrapper& operator++();
    void setValue(int32 n);
private:
    ProgressDialog* mpDialog;
};

#endif // COMPATIBLE_H
