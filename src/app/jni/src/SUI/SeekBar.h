/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef SEEKBAR_H
#define SEEKBAR_H

#include "SUI.h"

class MainScreen;

class SeekBar : public GUIElement{
	MainScreen *main_screen;
	SDL_Rect region;
	bool drag_started;
	double multiplier;
public:
	SeekBar(SUI *sui, MainScreen *parent);
	void update();
	unsigned handle_event(const SDL_Event &);
};

#endif
