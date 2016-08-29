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
#include "SeekBar.h"
#include "MainScreen.h"

SeekBar::SeekBar(SUI *sui, MainScreen *parent):
	GUIElement(sui, parent),
	main_screen(main_screen),
	region(parent->get_seekbar_region()),
	drag_started(0){
}

void SeekBar::update(){
	double total_time = this->sui->get_current_total_time();
	if (total_time < 0)
		return;
	auto &player = this->sui->get_player();
	double current_time = player.get_current_time();

	{
		auto target = this->sui->get_target();
		SDL_Color color;
		color.r = color.g = color.b = 0xFF;
		color.a = 0x80;
		auto rect = this->region;
		rect.w = int(rect.w * (!this->drag_started ? current_time / total_time : this->multiplier));
		rect.y += rect.h / 4 * 3;
		GPU_RectangleFilled(target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, color);
	}
	{
		std::wstringstream stream;
		parse_into_hms(stream, !this->drag_started ? current_time : this->multiplier * total_time);
		stream <<" / ";
		parse_into_hms(stream, total_time);
		stream <<std::endl
			<<this->sui->get_metadata();
		this->sui->get_font()->draw_text(stream.str(), this->region.x, this->region.y, this->sui->get_bounding_square(), 2);
	}
}

unsigned SeekBar::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	switch (event.type){
		case SDL_MOUSEBUTTONDOWN:
			{
				auto x = this->sui->transform_mouse_x(event.button.x),
					y = this->sui->transform_mouse_y(event.button.y);
				__android_log_print(ANDROID_LOG_INFO, "C++Button", "Mouse click: (%d, %d)\n", x, y);
				if (!is_inside(x, y, this->region))
					break;
				this->drag_started = 1;
				this->multiplier = (double)x / (double)this->region.w;
				ret |= SUI::REDRAW;
			}
			break;
		case SDL_MOUSEMOTION:
			{
				if (!this->drag_started)
					break;
				auto x = this->sui->transform_mouse_x(event.motion.x);
				this->multiplier = (double)x / (double)this->region.w;
				ret |= SUI::REDRAW;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			{
				if (!this->drag_started)
					break;
				this->sui->request_update();
				auto x = this->sui->transform_mouse_x(event.motion.x);
				this->sui->get_player().request_absolute_scaling_seek((double)x / (double)this->region.w);
				this->drag_started = 0;
				ret |= SUI::REDRAW;
			}
			break;
		default:
			break;
	}
	return ret;
}
