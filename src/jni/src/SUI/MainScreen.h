#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "SUI.h"
#include "ListView.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <list>
#endif

class MainScreen : public GUIElement{
	double current_total_time;
	AudioPlayer &player;
	Texture tex_buttons;
	boost::shared_ptr<ListView> listview;

	void prepare_buttons();
	void gui_signal(const GuiSignal &);
public:
	MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player);
	unsigned handle_event(const SDL_Event &);
	void update();
};

#endif