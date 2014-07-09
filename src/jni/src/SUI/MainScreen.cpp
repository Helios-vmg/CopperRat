#include "../stdafx.h"
#include "MainScreen.h"
#include "../CommonFunctions.h"
#include "Button.h"
#include "../File.h"

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
}

unsigned MainScreen::handle_event(const SDL_Event &event){
	if (!!this->listview)
		return this->listview->handle_event(event);
	return GUIElement::handle_event(event);
}

void MainScreen::update(){
	if (!!this->listview){
		this->listview->update();
		return;
	}
	std::wstringstream stream;
	parse_into_hms(stream, player.get_current_time());
	stream <<" / ";
	parse_into_hms(stream, this->sui->get_current_total_time());
	stream <<std::endl
		<<this->sui->get_metadata();
	this->sui->draw_picture();
	this->sui->get_font()->draw_text(stream.str(), 0, 0, this->sui->get_bounding_square(), 2);
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
			this->player.request_seek(-5);
			break;
		case ButtonSignal::SEEKFORTH:
			this->player.request_seek(5);
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
	for (i = 0; i < n; i++){
		Subtexture st(this->tex_buttons, rects[i]);
		boost::shared_ptr<GraphicButton> button(new GraphicButton(this->sui, this));
		button->set_graphic(st);
		button->set_position(rects[i].x, rects[i].y + square);
		button->set_signal_value(i);
		this->children.push_back(button);
	}
}
