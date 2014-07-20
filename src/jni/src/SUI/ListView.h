/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "Button.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#endif

class ControlCoroutine;

class ListView : public GUIElement{
	GuiSignal signal;
	std::vector<boost::shared_ptr<TextButton> > items;
	SDL_Rect visible_region;
	bool buttondown,
		drag_started,
		moving;
	int mousedown_y,
		total_length,
		min_offset;
	double movement_speed,
		offset;
public:
	ListView(SUI *sui, GUIElement *parent, const std::vector<std::wstring> &list, unsigned listview_name);
	unsigned handle_event(const SDL_Event &);
	void update();
	void gui_signal(const GuiSignal &);
	bool get_input(unsigned &dst, ControlCoroutine &, boost::shared_ptr<ListView> self);
};

#endif
