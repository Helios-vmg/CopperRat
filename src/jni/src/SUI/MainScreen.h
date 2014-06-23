#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "SUI.h"
#include <list>

class MainScreen : public GUIElement{
	double current_total_time;
	AudioPlayer &player;
	Texture tex_buttons;

	void prepare_buttons();
public:
	MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player);
	void update();
};

#endif
