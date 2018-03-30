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

#ifndef KEYMAP_LABELS_H
#define KEYMAP_LABELS_H

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "keymap"

// only for translation purpose

const char* keymap_labels[] = {
    B_TRANSLATE("Backspace"),
    B_TRANSLATE("Cancel"),
    B_TRANSLATE("CancelAndIMEOff"),
    B_TRANSLATE("Commit"),
    B_TRANSLATE("CommitFirstSuggestion"),
    B_TRANSLATE("CommitOnlyFirstSegment"),
    B_TRANSLATE("Composition"),
    B_TRANSLATE("Conversion"),
    B_TRANSLATE("Convert"),
    B_TRANSLATE("ConvertNext"),
    B_TRANSLATE("ConvertNextPage"),
    B_TRANSLATE("ConvertPrev"),
    B_TRANSLATE("ConvertPrevPage"),
    B_TRANSLATE("ConvertToFullAlphanumeric"),
    B_TRANSLATE("ConvertToFullKatakana"),
    B_TRANSLATE("ConvertToHalfAlphanumeric"),
    B_TRANSLATE("ConvertToHalfKatakana"),
    B_TRANSLATE("ConvertToHalfWidth"),
    B_TRANSLATE("ConvertToHiragana"),
    B_TRANSLATE("ConvertWithoutHistory"),
    B_TRANSLATE("Delete"),
    B_TRANSLATE("DeleteSelectedCandidate"),
    B_TRANSLATE("DirectInput"),
    B_TRANSLATE("DisplayAsFullAlphanumeric"),
    B_TRANSLATE("DisplayAsFullKatakana"),
    B_TRANSLATE("DisplayAsHalfAlphanumeric"),
    B_TRANSLATE("DisplayAsHalfKatakana"),
    B_TRANSLATE("DisplayAsHalfWidth"),
    B_TRANSLATE("DisplayAsHiragana"),
    B_TRANSLATE("IMEOff"),
    B_TRANSLATE("IMEOn"),
    B_TRANSLATE("InputModeFullAlphanumeric"),
    B_TRANSLATE("InputModeFullKatakana"),
    B_TRANSLATE("InputModeHalfAlphanumeric"),
    B_TRANSLATE("InputModeHalfKatakana"),
    B_TRANSLATE("InputModeHiragana"),
    B_TRANSLATE("InputModeSwitchKanaType"),
    B_TRANSLATE("InsertAlternateSpace"),
    B_TRANSLATE("InsertFullSpace"),
    B_TRANSLATE("InsertHalfSpace"),
    B_TRANSLATE("InsertSpace"),
    B_TRANSLATE("LaunchConfigDialog"),
    B_TRANSLATE("LaunchDictionaryTool"),
    B_TRANSLATE("LaunchWordRegisterDialog"),
    B_TRANSLATE("MoveCursorLeft"),
    B_TRANSLATE("MoveCursorRight"),
    B_TRANSLATE("MoveCursorToBeginning"),
    B_TRANSLATE("MoveCursorToEnd"),
    B_TRANSLATE("Precomposition"),
    B_TRANSLATE("PredictAndConvert"),
    B_TRANSLATE("Prediction"),
    B_TRANSLATE("Reconvert"),
    B_TRANSLATE("Revert"),
    B_TRANSLATE("SegmentFocusFirst"),
    B_TRANSLATE("SegmentFocusLast"),
    B_TRANSLATE("SegmentFocusLeft"),
    B_TRANSLATE("SegmentFocusRight"),
    B_TRANSLATE("SegmentWidthExpand"),
    B_TRANSLATE("SegmentWidthShrink"),
    B_TRANSLATE("Suggestion"),
    B_TRANSLATE("SwitchKanaType"),
    B_TRANSLATE("ToggleAlphanumericMode"),
    B_TRANSLATE("Undo"),
};

#endif // KEYMAP_LABELS_H
