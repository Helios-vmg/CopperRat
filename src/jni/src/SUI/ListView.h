#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "Button.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#endif

class ListView : public GUIElement{
	GuiSignal signal;
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
	ListView(SUI *sui, GUIElement *parent, const std::vector<std::wstring> &list, unsigned listview_name);
	unsigned handle_event(const SDL_Event &);
	void update();
	void gui_signal(const GuiSignal &);
};

#endif
