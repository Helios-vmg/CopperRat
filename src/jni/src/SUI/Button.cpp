#include "Button.h"
#include "../CommonFunctions.h"

unsigned Button::handle_event(const SDL_Event &event){
	if (event.type != SDL_MOUSEBUTTONUP)
		return SUI::NOTHING;
	auto x = event.button.x - this->offset_x;
	auto y = event.button.y - this->offset_y;
	const auto &bb = this->bounding_box;
	if (x >= bb.x && x < bb.x + bb.w && y >= bb.y && y < bb.y + bb.h)
		this->on_click();
	return SUI::NOTHING;
}

void GraphicButton::update(){
	auto temp = this->bounding_box;
	temp.x += this->offset_x;
	temp.y += this->offset_y;
	this->graphic.draw(this->bounding_box);
}

void TextButton::calculate_bounding_box(){
	this->bounding_box = this->sui->get_font()->calculate_bounding_box(this->text, this->wrapping_limit, this->scale);
}

void TextButton::set_minimum_height(double millimeters){
	auto dpm = get_dots_per_millimeter();
	int height = (int)ceil(dpm * millimeters);
	if (this->bounding_box.h >= height)
		return;
	this->inner_position = (height - this->bounding_box.h) / 2;
	this->bounding_box.h = height;
}

void TextButton::update(){
	this->sui->get_font()->draw_text(
		this->text,
		this->bounding_box.x + this->offset_x,
		this->bounding_box.y + this->offset_y + this->inner_position,
		this->wrapping_limit,
		this->scale
	);
}
