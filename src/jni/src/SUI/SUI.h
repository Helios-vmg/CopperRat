#ifndef SUI_H
#define SUI_H

#include "../AudioPlayer.h"
#include "../UserInterface.h"
#include <SDL.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <memory>
#include "Font.h"
#include "../auto_ptr.h"
#include "../Deleters.h"

struct UIInitializationException{
	std::string desc;
	UIInitializationException(const std::string &desc): desc(desc){}
};

#define SDL_PTR_WRAPPER(T) CR_UNIQUE_PTR2(T, void(*)(T *))

class SUI : public UserInterface{
	AudioPlayer &player;
	SDL_PTR_WRAPPER(SDL_Window) window;
	boost::shared_ptr<SDL_Renderer> renderer;
	boost::shared_ptr<Font> font;
	double current_total_time;
	std::string metadata;

	enum InputStatus{
		NOTHING = 0,
		QUIT = 1,
		REDRAW = 2,
	};

	unsigned handle_in_events();
	unsigned handle_out_events();
public:
	SUI(AudioPlayer &player);
	void loop();

	unsigned receive(TotalTimeUpdate &);
	unsigned receive(MetaDataUpdate &);
};

#endif
