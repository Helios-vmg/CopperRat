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

#include "../stdafx.h"
#include "ListView.h"

ListView::ListView(SUI *sui, GUIElement *parent, const std::vector<std::wstring> &list, unsigned listview_name): GUIElement(sui, parent){
	this->items.resize(list.size());
	size_t i = 0;
	int accum = 0;
	int square = sui->get_bounding_square();
	for (auto &s : list){
		boost::shared_ptr<TextButton> button(new TextButton(sui, this, (unsigned)i));
		button->set_text(s, square, 1);
		button->set_text_size_mm(2.5);
		auto bb = button->get_bounding_box();
		bb.w = square;
		bb.y = accum;
		button->set_bounding_box(bb);
		button->set_minimum_height(10.0);
		bb = button->get_bounding_box();
		accum += bb.h;
		this->items[i++] = button;
	}
	this->visible_region = sui->get_visible_region();
	this->total_length = accum;
	this->drag_started = 0;
	this->buttondown = 0;
	this->offset = 0;
	this->min_offset = this->visible_region.h - this->total_length;
	if (this->min_offset > 0)
		this->min_offset = 0;
	this->movement_speed = 0;
	this->moving = 0;

	this->signal.type = SignalType::LISTVIEW_SIGNAL;
	this->signal.data.listview_signal.listview_name = listview_name;
}

unsigned ListView::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	bool relay = 1;
	switch (event.type){
		case SDL_KEYDOWN:
			if (event.key.keysym.scancode == SDL_SCANCODE_AC_BACK || event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE){
				GuiSignal signal;
				signal.type = SignalType::BACK_PRESSED;
				this->parent->gui_signal(signal);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			this->mousedown_y = event.button.y;
			this->buttondown = 1;
			this->movement_speed = 0;
			break;
		case SDL_MOUSEMOTION:
			if (this->drag_started)
				relay = 0;
			if (this->buttondown){
				int d = event.motion.y - this->mousedown_y;
				if (abs(d) >= 10)
					this->drag_started = 1;
				if (this->drag_started){
					d = event.motion.yrel;
					if (this->offset + d > 0)
						this->offset = 0;
					else if (this->offset + d < this->min_offset)
						this->offset = this->min_offset;
					else
						this->offset += d;
					this->movement_speed = d * 1.5;
					ret |= SUI::REDRAW;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (this->drag_started){
				relay = 0;
				this->drag_started = 0;
				ret |= SUI::REDRAW;
			}
			this->buttondown = 0;
			break;
	}
	if (relay)
		for (auto &b : this->items)
			ret |= b->handle_event(event);
	return ret;
}

void ListView::gui_signal(const GuiSignal &s){
	if (s.type != SignalType::BUTTON_SIGNAL)
		return;
	auto relay = this->signal;
	relay.data.listview_signal.signal = &s;
	this->parent->gui_signal(relay);
}

void ListView::update(){
update_restart:
	auto renderer = this->sui->get_renderer();
	if (this->movement_speed && !this->buttondown){
		this->offset += this->movement_speed;
		if (this->offset > 0){
			this->offset = 0;
			this->movement_speed = 0;
			goto update_restart;
		}
		if (this->offset < this->min_offset){
			this->offset = this->min_offset;
			this->movement_speed = 0;
			goto update_restart;
		}
		if (!this->moving){
			this->sui->start_full_updating();
			this->moving = 1;
		}
		const double k = 0.5;
		this->movement_speed += this->movement_speed > 0 ? -k : k;
	}else if (this->moving){
		this->sui->end_full_updating();
		this->moving = 0;
	}
	Uint8 red, green, blue, alpha; 
	SDL_GetRenderDrawColor(renderer.get(), &red, &green, &blue, &alpha);
	SDL_SetRenderDrawColor(renderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
	int int_offset = (int)this->offset;
	for (auto &b : this->items){
		b->set_offset(0, int_offset);
		b->update();
		auto r = b->get_bounding_box();
		auto h = r.y + r.h + int_offset;
		SDL_RenderDrawLine(renderer.get(), r.x, h, r.x + r.w, h);
	}
	if (this->total_length > this->visible_region.h){
		SDL_SetRenderDrawColor(renderer.get(), 0xFF, 0xFF, 0xFF, 0x80);
		SDL_Rect rect = {
			this->visible_region.w - 10,
			(int)(-this->offset / this->total_length * this->visible_region.h),
			10,
			this->visible_region.h * this->visible_region.h / this->total_length,
		};
		SDL_RenderFillRect(renderer.get(), &rect);
	}
	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);
}

bool ListView::get_input(unsigned &dst, ControlCoroutine &coroutine, boost::shared_ptr<ListView> self){
	unsigned this_name = this->signal.data.listview_signal.listview_name;
	while (1){
		auto signal = coroutine.display(self);
		switch (signal.type){
			case SignalType::BACK_PRESSED:
				return 0;
			case SignalType::LISTVIEW_SIGNAL:
				break;
			default:
				continue;
		}
		if (signal.data.listview_signal.listview_name != this_name)
			continue;
		signal = *signal.data.listview_signal.signal;
		if (signal.type != SignalType::BUTTON_SIGNAL)
			continue;
		dst = signal.data.button_signal;
		return 1;
	}
}
