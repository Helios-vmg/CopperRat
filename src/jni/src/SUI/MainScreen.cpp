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
#include "MainScreen.h"
#include "../CommonFunctions.h"
#include "Button.h"
#include "../File.h"
#include "ListView.h"
#include "SeekBar.h"

enum class ButtonSignal{
	PLAY = 0,
	PAUSE,
	STOP,
	LOAD,
	PREVIOUS,
	SEEKBACK,
	SEEKFORTH,
	NEXT,
};

MainScreen::MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player):
		GUIElement(sui, parent),
		player(player){
	this->prepare_buttons();
	this->children.push_back(boost::shared_ptr<GUIElement>(new SeekBar(sui, this)));
}

unsigned MainScreen::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	if (event.type == SDL_KEYDOWN && (event.key.keysym.scancode == SDL_SCANCODE_MENU || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)){
		GuiSignal signal;
		signal.type = SignalType::MAINSCREEN_MENU;
		this->parent->gui_signal(signal);
	}
	return ret | GUIElement::handle_event(event);
}

void MainScreen::update(){
	this->sui->draw_picture();
	GUIElement::update();
}

void MainScreen::gui_signal(const GuiSignal &signal){
	if (signal.type != SignalType::BUTTON_SIGNAL)
		return;
	switch ((ButtonSignal)signal.data.button_signal){
		case ButtonSignal::PLAY:
			this->player.request_hardplay();
			break;
		case ButtonSignal::PAUSE:
			this->player.request_pause();
			break;
		case ButtonSignal::STOP:
			this->player.request_stop();
			break;
		case ButtonSignal::LOAD:
			{
				GuiSignal signal;
				signal.type = SignalType::MAINSCREEN_LOAD;
				this->parent->gui_signal(signal);
			}
			break;
		case ButtonSignal::PREVIOUS:
			this->player.request_previous();
			break;
		case ButtonSignal::SEEKBACK:
			this->player.request_relative_seek(-5);
			break;
		case ButtonSignal::SEEKFORTH:
			this->player.request_relative_seek(5);
			break;
		case ButtonSignal::NEXT:
			this->player.request_next();
			break;
	}
}

void MainScreen::prepare_buttons(){
	static const char *button_paths[] = {
		BASE_PATH "button_play.png",
		BASE_PATH "button_pause.png",
		BASE_PATH "button_stop.png",
		BASE_PATH "button_load.png",
		BASE_PATH "button_previous.png",
		BASE_PATH "button_seekback.png",
		BASE_PATH "button_seekforth.png",
		BASE_PATH "button_next.png",
	};
	const size_t n = sizeof(button_paths) / sizeof(*button_paths);
	surface_t button_surfaces[n];
	int i = 0;
	const int square = this->sui->get_bounding_square();
	for (auto p : button_paths){
		auto image = load_image_from_file(p);
		if (!image)
			throw UIInitializationException("Failed to load a button.");
		image = bind_surface_to_square(image, square / 4);
		if (!image)
			throw UIInitializationException("Failed to transform a button.");
		button_surfaces[i++] = image;
	}
	int w = button_surfaces[0]->w;
	int h = button_surfaces[0]->h;
	auto new_surface = create_rgba_surface(w * 4, h * 2);
	i = 0;
	SDL_Rect rects[n];
	for (auto &surface : button_surfaces){
		SDL_Rect dst = { w * (i % 4), h * (i / 4), w, h, };
		rects[i] = dst;
		SDL_BlitSurface(button_surfaces[i].get(), 0, new_surface.get(), &dst);
		i++;
	}
	this->tex_buttons.set_renderer(this->sui->get_renderer());
	this->tex_buttons.load(new_surface);
	this->tex_buttons.set_alpha(0.5);
	for (i = 0; i < n; i++){
		Subtexture st(this->tex_buttons, rects[i]);
		boost::shared_ptr<GraphicButton> button(new GraphicButton(this->sui, this));
		button->set_graphic(st);
		button->set_position(rects[i].x, rects[i].y + square);
		button->set_signal_value(i);
		this->children.push_back(button);
	}
}
