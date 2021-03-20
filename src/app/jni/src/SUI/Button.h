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

#ifndef BUTTON_H
#define BUTTON_H

#include "SUI.h"
#include "../Deleters.h"
#include "../CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#include <functional>
#endif

class Button : public GUIElement{
protected:
	SDL_Rect bounding_box;
	int offset_x,
		offset_y;
	std::function<void()> on_click_f;
public:
	Button(SUI *sui, GUIElement *parent): GUIElement(sui, parent), offset_x(0), offset_y(0){}
	virtual ~Button(){}
	unsigned handle_event(const SDL_Event &);
	void set_bounding_box(const SDL_Rect &bb){
		this->bounding_box = bb;
	}
	const SDL_Rect &get_bounding_box() const{
		return this->bounding_box;
	}
	void set_offset(int offset_x, int offset_y){
		this->offset_x = offset_x;
		this->offset_y = offset_y;
	}
	void set_on_click(std::function<void()> &&f){
		this->on_click_f = std::move(f);
	}
	void on_click(){
		//__android_log_print(ANDROID_LOG_INFO, "IntegerSignalButton", "on_click()\n", this->signal.data.button_signal);
		if (this->on_click_f)
			this->on_click_f();
	}
};

class GraphicButton : public Button{
protected:
	Subtexture graphic;
public:
	GraphicButton(SUI *sui, GUIElement *parent): Button(sui, parent){}
	void set_position(int x, int y){
		__android_log_print(ANDROID_LOG_INFO, "GraphicButton", "set_position(%d, %d)\n", x, y);
		this->bounding_box.x = x;
		this->bounding_box.y = y;
	}
	void set_graphic(Subtexture graphic){
		this->graphic = graphic;
		auto rect = graphic.get_rect();
		this->bounding_box.x = 0;
		this->bounding_box.y = 0;
		this->bounding_box.w = rect.w;
		this->bounding_box.h = rect.h;
	}
	void update();
};

class TextButton : public Button{
protected:
	std::wstring text;
	int wrapping_limit = 0;
	double scale = 1;
	int inner_position = 0;

	void calculate_bounding_box();
public:
	TextButton(SUI *sui, GUIElement *parent): Button(sui, parent){
		this->bounding_box.w = max_possible_value(this->bounding_box.w);
		this->bounding_box.h = max_possible_value(this->bounding_box.h);
	}
	void set_text(const std::wstring &text, int max_width = INT_MAX, double scale = 1.0){
		this->text = text;
		this->wrapping_limit = max_width;
		this->scale = scale;
		this->calculate_bounding_box();
	}
	void set_text(const wchar_t *text, int max_width = INT_MAX, double scale = 1.0){
		this->text = text;
		this->wrapping_limit = max_width;
		this->scale = scale;
		this->calculate_bounding_box();
	}
	void set_text_size_mm(double millimeters = 1.0);
	void update();
	const std::wstring &get_text() const{
		return this->text;
	}
	void set_minimum_height(double millimeters);
};

#endif
