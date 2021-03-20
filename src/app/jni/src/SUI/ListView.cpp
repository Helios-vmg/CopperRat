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

unsigned ListView::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	bool relay = 1;
	switch (event.type){
		case SDL_KEYDOWN:
			if ((event.key.keysym.scancode == SDL_SCANCODE_AC_BACK || event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) && this->on_cancel)
				this->on_cancel();
			break;
		case SDL_MOUSEBUTTONDOWN:
			{
				this->mousedown_y = this->sui->transform_mouse_y(event.button.y);
				this->buttondown = 1;
				this->movement_speed = 0;
			}
			break;
		case SDL_MOUSEMOTION:
			{
				if (this->drag_started)
					relay = 0;
				if (this->buttondown){
					auto y = this->sui->transform_mouse_y(event.motion.y);
					int d = y - this->mousedown_y;
					if (abs(d) >= 10)
						this->drag_started = 1;
					if (this->drag_started){
						d = this->sui->transform_mouse_y(event.motion.yrel);
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

void ListView::update(){
	auto target = this->sui->get_target();
	while (true){
		if (this->movement_speed && !this->buttondown){
			this->offset += this->movement_speed;
			if (this->offset > 0){
				this->offset = 0;
				this->movement_speed = 0;
				continue;
			}
			if (this->offset < this->min_offset){
				this->offset = this->min_offset;
				this->movement_speed = 0;
				continue;
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
		break;
	}
	int int_offset = (int)this->offset;
	SDL_Color color = { 0xFF, 0xFF, 0xFF, 0xFF };
	for (auto &b : this->items){
		b->set_offset(0, int_offset);
		b->update();
		auto r = b->get_bounding_box();
		auto h = r.y + r.h + int_offset;

		if (h >= 0 && h < target->h)
			GPU_Line(target, r.x, h, r.x + r.w, h, color);
	}
	if (this->total_length > this->visible_region.h){
		color.a = 0x80;
		SDL_Rect rect = {
			this->visible_region.w - 5,
			(int)(-this->offset / this->total_length * this->visible_region.h),
			5,
			this->visible_region.h * this->visible_region.h / this->total_length,
		};
		if (rect.y + rect.h > 0 && rect.y < target->h)
			GPU_RectangleFilled(target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, color);
	}
}
