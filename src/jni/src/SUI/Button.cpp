#include "Button.h"

unsigned Button::handle_event(const SDL_Event &event){
	if (event.type != SDL_MOUSEBUTTONUP)
		return SUI::NOTHING;
	auto x = event.button.x;
	auto y = event.button.y;
	const auto &bb = this->bounding_box;
	if (x >= bb.x && x < bb.x + bb.w && y >= bb.y && y < bb.y + bb.h)
		this->on_click();
	return SUI::NOTHING;
}

void GraphicButton::update(){
	this->graphic.draw(this->bounding_box);
}

void TextButton::calculate_bounding_box(){
	this->bounding_box = this->sui->get_font()->calculate_bounding_box(this->text, this->wrapping_limit, this->scale);
}

void TextButton::update(){
	this->sui->get_font()->draw_text(this->text, this->bounding_box.x, this->bounding_box.y, this->scale);
}
