#include "../AudioPlayer.h"
#include <SDL.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <memory>

struct UIInitializationException{
	std::string desc;
	UIInitializationException(const std::string &desc): desc(desc){}
};

#define SDL_PTR_WRAPPER(T) std::unique_ptr<T, void(*)(T *)>

class SUI{
	AudioPlayer &player;
	SDL_PTR_WRAPPER(SDL_Window) window;
	SDL_PTR_WRAPPER(SDL_Renderer) renderer;
	SDL_PTR_WRAPPER(SDL_Texture) font;
	double current_total_time;

	bool handle_in_events();
	void handle_out_events();
public:
	SUI(AudioPlayer &player);
	void loop();
};
