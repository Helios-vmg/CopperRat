/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "../stdafx.h"
#include "SeekBar.h"
#include "MainScreen.h"
#include "../AudioPlayer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <sstream>
#endif

extern const char * const vertex_shader;

SeekBar::SeekBar(SUI *sui, MainScreen *parent):
		GUIElement(sui, parent),
		main_screen(parent),
		region(parent->get_seekbar_region()){
	auto vr = this->sui->get_visible_region();
	this->text_texture = std::make_unique<RenderTarget>(vr.w / 2, vr.h / 2);
	this->intermediate = std::make_unique<RenderTarget>(vr.w / 2, vr.h / 2);
	this->create_shaders();
}

void SeekBar::create_shaders(){
	auto vertex = Shader::create(vertex_shader, 0);
	if (!*vertex){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", vertex->get_error_string().c_str());
		return;
	}
	auto vr = this->sui->get_visible_region();
	static const char * const fragment_shader =
#ifndef __ANDROID__
"#version 120\n"
#else
"#version 100\n"
"precision mediump int;\n"
"precision mediump float;\n"
#endif
"\n"
"varying vec2 texCoord;\n"
"uniform sampler2D tex;\n"
"\n"
"void main(void){\n"
"    vec4 value = texture2D(tex, texCoord);\n"
"    value.r = 1.0 - value.r;\n"
"    value.g = 1.0 - value.g;\n"
"    value.b = 1.0 - value.b;\n"
"    gl_FragColor = value;\n"
"}\n";
	auto frag = Shader::create(fragment_shader);
	if (!*frag){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", frag->get_error_string().c_str());
		return;
	}
	this->shader.add(vertex);
	this->shader.add(frag);
	this->shader.create_internal_object();
	if (!this->shader){
		__android_log_print(ANDROID_LOG_ERROR, "C++Shader", "%s", this->shader.get_error_string().c_str());
	}
}

void SeekBar::update(){
	double total_time = this->main_screen->get_current_total_time();
	if (total_time < 0)
		return;
	auto &player = this->sui->get_player();
	double current_time = this->main_screen->get_player().get_current_time();
	auto vr = this->sui->get_visible_region();

	auto sui_target = this->sui->get_target();
	{
		SDL_Color color;
		color.r = color.g = color.b = 0xFF;
		color.a = 0x80;
		auto rect = this->region;
		rect.w = int(rect.w * (!this->drag_started ? current_time / total_time : this->multiplier));
		rect.y += rect.h / 4 * 3;
		GPU_RectangleFilled(sui_target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, color);
	}
	auto target = this->text_texture->get_target();
	GPU_ClearColor(target, { 0, 0, 0, 0 });
	{
		std::wstringstream stream;
		parse_into_hms(stream, !this->drag_started ? current_time : this->multiplier * total_time);
		stream <<" / ";
		parse_into_hms(stream, total_time);
		stream << std::endl
			<< this->main_screen->get_metadata();
		this->sui->get_font()->draw_text(target, stream.str(), this->region.x / 2, this->region.y / 2, this->sui->get_bounding_square() / 2, 1);
	}

	//GPU_FlushBlitBuffer();
	GPU_Flip(target);
	auto target2 = this->intermediate->get_target();
    GPU_ClearColor(target2, { 0, 0, 0, 0 });
	static const std::pair<int, int> offsets[] = {
		{ 1,  0},
		{-1,  0},
		{ 0,  1},
		{ 0, -1},
	};
    if (this->shader)
		this->shader.activate();
	for (auto &p : offsets)
		GPU_Blit(this->text_texture->get_image().get(), nullptr, target2, vr.w / 4.f + p.first, vr.h / 4.f + p.second);
	if (this->shader)
		GPU_ActivateShaderProgram(0, nullptr);
	GPU_Blit(this->text_texture->get_image().get(), nullptr, target2, vr.w / 4.f, vr.h / 4.f);
	//GPU_FlushBlitBuffer();
	GPU_Flip(target2);
	GPU_BlitScale(this->intermediate->get_image().get(), nullptr, sui_target, vr.w / 2.f, vr.h / 2.f, 2, 2);
}

unsigned SeekBar::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	switch (event.type){
		case SDL_MOUSEBUTTONDOWN:
			{
				auto x = this->sui->transform_mouse_x(event.button.x),
					y = this->sui->transform_mouse_y(event.button.y);
				__android_log_print(ANDROID_LOG_INFO, "C++Button", "Mouse click: (%d, %d)\n", x, y);
				if (!is_inside(x, y, this->region))
					break;
				this->drag_started = 1;
				this->multiplier = (double)x / (double)this->region.w;
				ret |= SUI::REDRAW;
			}
			break;
		case SDL_MOUSEMOTION:
			{
				if (!this->drag_started)
					break;
				auto x = this->sui->transform_mouse_x(event.motion.x);
				this->multiplier = (double)x / (double)this->region.w;
				ret |= SUI::REDRAW;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			{
				if (!this->drag_started)
					break;
				this->sui->request_update();
				auto x = this->sui->transform_mouse_x(event.motion.x);
				this->main_screen->get_player().get_player().request_absolute_scaling_seek((double)x / (double)this->region.w);
				this->drag_started = 0;
				ret |= SUI::REDRAW;
			}
			break;
		default:
			break;
	}
	return ret;
}
