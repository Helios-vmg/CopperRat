#include "ListView.h"

ListView::ListView(SUI *sui, GUIElement *parent, const std::vector<std::wstring> &list, unsigned listview_name): GUIElement(sui, parent){
	this->items.resize(list.size());
	size_t i = 0;
	int accum = 0;
	int square = sui->get_bounding_square();
	for (auto &s : list){
		boost::shared_ptr<TextButton> button(new TextButton(sui, this, (unsigned)i));
		button->set_text(s, square, 1);
		button->set_text_size_mm(2.5);
		auto bb = button->get_bounding_box();
		bb.w = square;
		bb.y = accum;
		button->set_bounding_box(bb);
		button->set_minimum_height(10.0);
		bb = button->get_bounding_box();
		accum += bb.h;
		this->items[i++] = button;
	}
	this->visible_region = sui->get_visible_region();
	this->total_length = accum;
	this->drag_started = 0;
	this->buttondown = 0;
	this->offset = 0;
	this->min_offset = this->visible_region.h - this->total_length;
	if (this->min_offset > 0)
		this->min_offset = 0;
	this->movement_speed = 0;
	this->moving = 0;

	this->signal.type = SignalType::LISTVIEW_SIGNAL;
	this->signal.data.listview_signal.listview_name = listview_name;
}

unsigned ListView::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	bool relay = 1;
	switch (event.type){
		case SDL_MOUSEBUTTONDOWN:
			this->mousedown_y = event.button.y;
			this->buttondown = 1;
			this->movement_speed = 0;
			break;
		case SDL_MOUSEMOTION:
			if (this->drag_started)
				relay = 0;
			if (this->buttondown){
				int d = event.motion.y - this->mousedown_y;
				if (abs(d) >= 10)
					this->drag_started = 1;
				if (this->drag_started){
					d = event.motion.yrel;
					if (this->offset + d > 0)
						this->offset = 0;
					else if (this->offset + d < this->min_offset)
						this->offset = this->min_offset;
					else
						this->offset += d;
					this->movement_speed = d * 1.5;
					ret |= SUI::REDRAW;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (this->drag_started){
				relay = 0;
				this->drag_started = 0;
				ret |= SUI::REDRAW;
			}
			this->buttondown = 0;
			break;
	}
	if (relay)
		for (auto &b : this->items)
			ret |= b->handle_event(event);
	return ret;
}

void ListView::gui_signal(const GuiSignal &s){
	if (s.type != SignalType::BUTTON_SIGNAL)
		return;
	auto relay = this->signal;
	relay.data.listview_signal.signal = &s;
	this->parent->gui_signal(relay);
}

void ListView::update(){
update_restart:
	auto renderer = sui->get_renderer();
	if (this->movement_speed && !this->buttondown){
		this->offset += this->movement_speed;
		if (this->offset > 0){
			this->offset = 0;
			this->movement_speed = 0;
			goto update_restart;
		}
		if (this->offset < this->min_offset){
			this->offset = this->min_offset;
			this->movement_speed = 0;
			goto update_restart;
		}
		if (!this->moving){
			this->sui->start_full_updating();
			this->moving = 1;
		}
		const double k = 0.5;
		this->movement_speed += this->movement_speed > 0 ? -k : k;
	}else if (this->moving){
		this->sui->end_full_updating();
		this->moving = 0;
	}
	Uint8 red, green, blue, alpha; 
	SDL_GetRenderDrawColor(renderer.get(), &red, &green, &blue, &alpha);
	SDL_SetRenderDrawColor(renderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
	int int_offset = (int)this->offset;
	for (auto &b : this->items){
		b->set_offset(0, int_offset);
		b->update();
		auto r = b->get_bounding_box();
		auto h = r.y + r.h + int_offset;
		SDL_RenderDrawLine(renderer.get(), r.x, h, r.x + r.w, h);
	}
	if (this->total_length > this->visible_region.h){
		SDL_SetRenderDrawColor(renderer.get(), 0xFF, 0xFF, 0xFF, 0x80);
		SDL_Rect rect = {
			this->visible_region.w - 10,
			(int)(-this->offset / this->total_length * this->visible_region.h),
			10,
			this->visible_region.h * this->visible_region.h / this->total_length,
		};
		SDL_RenderDrawRect(renderer.get(), &rect);
	}
	SDL_SetRenderDrawColor(renderer.get(), red, green, blue, alpha);
}
