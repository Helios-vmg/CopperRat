/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "Button.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <vector>
#include <string>
#include <utility>
#include <memory>
#include <functional>
#endif

class ControlCoroutine;

class ListView : public GUIElement{
	std::vector<std::shared_ptr<TextButton> > items;
	SDL_Rect visible_region;
	bool buttondown = 0,
		drag_started = 0,
		moving = 0;
	int mousedown_y,
		total_length = 0,
		min_offset;
	double movement_speed = 0,
		offset = 0;

	std::function<void()> on_cancel;
	std::function<void(size_t)> on_selection;

	template <typename String>
	void add_button(String &&s, unsigned index, int square){
		auto button = std::make_shared<TextButton>(sui, this);
		button->set_text(std::move(s), square, 1);
		button->set_text_size_mm(2.5);
		auto bb = button->get_bounding_box();
		bb.w = square;
		bb.y = this->total_length;
		button->set_bounding_box(bb);
		button->set_minimum_height(10.0);
		bb = button->get_bounding_box();
		this->total_length += bb.h;
		button->set_on_click([this, index](){
			if (this->on_selection)
				this->on_selection(index);
		});
		this->items.emplace_back(std::move(button));
	}
public:
	template <typename It>
	ListView(SUI *sui, GUIElement *parent, It begin, It end): GUIElement(sui, parent){
		this->items.reserve(end - begin);
		int square = sui->get_bounding_square();
		for (auto it = begin; it != end; ++it)
			this->add_button(std::move(*it), (unsigned)(it - begin), square);
		this->visible_region = sui->get_visible_region();
		this->min_offset = this->visible_region.h - this->total_length;
		if (this->min_offset > 0)
			this->min_offset = 0;
	}
	unsigned handle_event(const SDL_Event &);
	void update();
	void set_on_cancel(std::function<void()> &&f){
		this->on_cancel = std::move(f);
	}
	void set_on_selection(std::function<void(size_t)> &&f){
		this->on_selection = std::move(f);
	}
};

#endif
