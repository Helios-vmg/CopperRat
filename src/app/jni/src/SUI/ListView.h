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
#include <memory>
#include <functional>
#endif

class ControlCoroutine;

class ListView : public GUIElement{
	GuiSignal signal;
	std::vector<std::shared_ptr<TextButton> > items;
	SDL_Rect visible_region;
	bool buttondown = 0,
		drag_started = 0,
		moving = 0;
	int mousedown_y,
		total_length = 0,
		min_offset;
	double movement_speed = 0,
		offset = 0;

	std::function<void()> on_cancel;
	std::function<void(size_t)> on_selection;

	template <typename String>
	void add_button(String &&s, unsigned index, int square){
		auto button = std::make_shared<TextButton>(sui, this, index);
		button->set_text(std::move(s), square, 1);
		button->set_text_size_mm(2.5);
		auto bb = button->get_bounding_box();
		bb.w = square;
		bb.y = this->total_length;
		button->set_bounding_box(bb);
		button->set_minimum_height(10.0);
		bb = button->get_bounding_box();
		this->total_length += bb.h;
		this->items.emplace_back(std::move(button));
	}
public:
	template <typename It>
	ListView(SUI *sui, GUIElement *parent, It begin, It end, unsigned listview_name): GUIElement(sui, parent){
		this->items.reserve(end - begin);
		int square = sui->get_bounding_square();
		for (auto it = begin; it != end; ++it)
			this->add_button(std::move(*it), (unsigned)(it - begin), square);
		this->visible_region = sui->get_visible_region();
		this->min_offset = this->visible_region.h - this->total_length;
		if (this->min_offset > 0)
			this->min_offset = 0;

		this->signal.type = SignalType::LISTVIEW_SIGNAL;
		this->signal.data.listview_signal.listview_name = listview_name;
	}
	unsigned handle_event(const SDL_Event &);
	void update();
	void gui_signal(const GuiSignal &);
	void set_on_cancel(std::function<void()> &&f){
		this->on_cancel = std::move(f);
	}
	void set_on_selection(std::function<void(size_t)> &&f){
		this->on_selection = std::move(f);
	}
};

#endif
