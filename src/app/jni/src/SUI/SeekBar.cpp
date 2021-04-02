/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "../stdafx.h"
#include "SeekBar.h"
#include "MainScreen.h"
#include "../AudioPlayer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <sstream>
#endif

SeekBar::SeekBar(SUI *sui, MainScreen *parent):
	GUIElement(sui, parent),
	main_screen(parent),
	region(parent->get_seekbar_region()){
}

void SeekBar::update(){
	double total_time = this->main_screen->get_current_total_time();
	if (total_time < 0)
		return;
	auto &player = this->sui->get_player();
	double current_time = this->main_screen->get_player().get_current_time();

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
		stream << std::endl
			<< this->main_screen->get_metadata();
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
				this->main_screen->get_player().get_player().request_absolute_scaling_seek((double)x / (double)this->region.w);
				this->drag_started = 0;
				ret |= SUI::REDRAW;
			}
			break;
		default:
			break;
	}
	return ret;
}
