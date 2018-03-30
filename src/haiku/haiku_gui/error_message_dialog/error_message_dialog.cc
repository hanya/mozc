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

#include "haiku/haiku_gui/error_message_dialog/error_message_dialog.h"

#include "base/flags.h"

#include <Application.h>
#include <Alert.h>
#include <Catalog.h>

DEFINE_string(error_type, "", "type of error");


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "error_message_dialog"

namespace mozc {
namespace haiku_gui {

#define APP_SIGNATURE "application/x-vnd.mozc.ErrorMessageDialog"


class ErrorMessageApp : public BApplication
{
public:
    ErrorMessageApp(const char* title, const char* message);
    virtual ~ErrorMessageApp() {};
};

ErrorMessageApp::ErrorMessageApp(const char* title, const char* message)
    : BApplication(APP_SIGNATURE)
{
    {
        BAlert *pWindow = new BAlert(title, message,
                        "", "", "OK", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
        // make title bar visible
        pWindow->SetLook(B_TITLED_WINDOW_LOOK);
        uint32 flags = pWindow->Flags();
        flags ^= B_NOT_CLOSABLE;
        pWindow->SetFlags(flags | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE);
        pWindow->SetShortcut(0, B_ESCAPE);
        pWindow->Go();

        pWindow = NULL;
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
}

} // namespace haiku_gui
} // namespace mozc


int HaikuRunErrorMessageDialog(int argc, char *argv[])
{
    const char* message = nullptr;
    if (FLAGS_error_type == "server_timeout") {
        message = B_TRANSLATE("Conversion engine is not responding. "
                        "Please restart this application.");
    } else if (FLAGS_error_type == "server_broken_message") {
        message = B_TRANSLATE("Connecting to an incompatible conversion engine. "
                        "Please restart your computer to enable Mozc. "
                        "If this problem persists, please uninstall Mozc "
                        "and install it again.");
    } else if (FLAGS_error_type == "server_version_mismatch") {
        message = B_TRANSLATE("Conversion engine has been upgraded. "
                        "Please restart this application to enable conversion engine. "
                        "If the problem persists, please restart your computer.");
    } else if (FLAGS_error_type == "server_shutdown") {
        message = B_TRANSLATE("Conversion engine is killed unexceptionally. "
                        "Restarting the engine...");
    } else if (FLAGS_error_type == "server_fatal") {
        message = B_TRANSLATE("Cannot start conversion engine. "
                        "Please restart your computer.");
    } else if (FLAGS_error_type == "renderer_version_mismatch") {
        message = B_TRANSLATE("Candidate window renderer has been upgraded. "
                        "Please restart this application to enable new candidate window renderer. "
                        "If the problem persists, please restart your computer.");
    } else if (FLAGS_error_type == "renderer_fatal") {
        message = B_TRANSLATE("Cannot start candidate window renderer. "
                        "Please restart your computer.");
    }

    if (message) {
        mozc::haiku_gui::ErrorMessageApp *app =
            new mozc::haiku_gui::ErrorMessageApp(B_TRANSLATE("Mozc Fatal Error"), message);
        app->Run();
        delete app;
    }
    return 0;
}
