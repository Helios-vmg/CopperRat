#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "Button.h"
#include <vector>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>

class ListView : public GUIElement{
	std::vector<boost::shared_ptr<TextButton> > items;
	SDL_Rect visible_region;
	bool buttondown,
		drag_started,
		moving;
	int mousedown_y,
		total_length,
		min_offset;
	double movement_speed,
		offset;
public:
	ListView(SUI *sui, GUIElement *parent, const std::vector<std::wstring> &list);
	unsigned handle_event(const SDL_Event &);
	void update();
	void gui_signal(unsigned);
};

#endif
