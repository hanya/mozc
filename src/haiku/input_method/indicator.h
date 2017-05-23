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

#ifndef INDICATOR_H_
#define INDICATOR_H_

#include <Window.h>

#include <string>

class BMessageRunner;
class BStringView;

namespace immozc {

const char IM_INDICATOR_LABEL_NAME[] = "label";
const char IM_INDICATOR_LOCATION_NAME[] = "location";
const char IM_INDICATOR_DELAY_NAME[] = "delay";

class Label;
class MozcLooper;

class Indicator : public BWindow
{
public:
	Indicator(MozcLooper *looper, const char *label, bigtime_t delay=1000 * 1000);
	virtual ~Indicator();
	virtual void MessageReceived(BMessage *msg);
	
	static const uint32 IM_INDICATOR_SHOW = 'IMis';
	static const uint32 IM_INDICATOR_HIDE = 'IMih';
	static const uint32 IM_INDICATOR_SET_LABEL = 'IMsl';
	static const uint32 IM_INDICATOR_SHOWN = 'IMsh';
private:
	MozcLooper * 	fLooper;
	bigtime_t 		fDelay;
	Label * 		fLabel;
	BMessageRunner * fRunner;
	
	void 		_ShowWithCloseDelay(BPoint point, bigtime_t delay);
};

}

#endif // INDICATOR_H_
