/*

Copyright (c) 2014, Helios
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../stdafx.h"
#include "SUI.h"
#include "../CommonFunctions.h"
#include "../Image.h"
#include "../File.h"
#include "MainScreen.h"
#include "FileBrowser.h"
#include "ListView.h"
#include "../Settings.h"
#include "../AudioBuffer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <boost/shared_array.hpp>
#endif

//#define LIMIT_FPS

ControlCoroutine::ControlCoroutine():
	co([this](co_t::pull_type &pt){ this->antico = &pt; this->entry_point(); }){}

GuiSignal ControlCoroutine::display(boost::shared_ptr<GUIElement>){
	(*this->antico)();
	return this->antico->get();
}

SUIControlCoroutine::SUIControlCoroutine(SUI &sui): sui(&sui){}

void SUIControlCoroutine::start(){
	this->co(GuiSignal());
}

void SUIControlCoroutine::relay(const GuiSignal &s){
	this->co(s);
}

GuiSignal SUIControlCoroutine::display(boost::shared_ptr<GUIElement> el){
	this->sui->current_element = el;
	this->sui->request_update();
	return ControlCoroutine::display(el);
}

bool SUIControlCoroutine::load_file(std::wstring &dst, bool only_directories){
	boost::shared_ptr<FileBrowser> browser(new FileBrowser(this->sui, this->sui, !only_directories, application_settings.get_last_browse_directory()));
	bool ret = browser->get_input(dst, *this, browser);
	if (ret)
		application_settings.set_last_browse_directory(browser->get_new_initial_directory());
	return ret;
}

void SUIControlCoroutine::load_file_menu(){
	static const wchar_t *options[] = {
		L"Load file...",
		L"Load directory...",
		L"Enqueue file...",
		L"Enqueue directory...",
	};
	std::vector<std::wstring> strings(options, options + sizeof(options) / sizeof(*options));
	boost::shared_ptr<ListView> lv(new ListView(this->sui, this->sui, strings, 0));
	unsigned button;
	if (!lv->get_input(button, *this, lv))
		return;

	bool load = button / 2 == 0;
	bool file = button % 2 == 0;
	std::wstring path;
	if (!this->load_file(path, !file))
		return;

	this->sui->load(load, file, path);
}

void SUIControlCoroutine::options_menu(){
	while (1){
		auto &playlist = this->sui->get_player().get_playlist();
		auto playback_mode = playlist.get_playback_mode();
		bool shuffling = playlist.get_shuffle();
		auto visualization_mode = application_settings.get_visualization_mode();
		auto display_fps = application_settings.get_display_fps();
		std::vector<std::wstring> strings;
		strings.push_back(L"Playback mode: " + to_string(playback_mode));
		strings.push_back(std::wstring(L"Shuffling: O") + (shuffling ? L"N" : L"FF"));
		strings.push_back(L"Visualization mode: " + to_string(visualization_mode));
		strings.push_back(std::wstring(L"Display framerate: O") + (display_fps ? L"N" : L"FF"));
		boost::shared_ptr<ListView> lv(new ListView(this->sui, this->sui, strings, 0));
		unsigned button;
		if (!lv->get_input(button, *this, lv))
			return;
		switch (button){
			case 0:
				application_settings.set_playback_mode(playlist.cycle_mode());
				break;
			case 1:
				application_settings.set_shuffle(playlist.toggle_shuffle());
				break;
			case 2:
				visualization_mode = (VisualizationMode)(((int)visualization_mode + 1) % (int)VisualizationMode::END);
				application_settings.set_visualization_mode(visualization_mode);
				this->sui->set_visualization_mode(visualization_mode);
				break;
			case 3:
				display_fps = !display_fps;
				application_settings.set_display_fps(display_fps);
				this->sui->set_display_fps(display_fps);
				break;
		}
	}
}

void SUIControlCoroutine::entry_point(){
	boost::shared_ptr<MainScreen> main_screen(new MainScreen(this->sui, this->sui, this->sui->player));
	while (1){
		auto signal = this->display(main_screen);
		switch (signal.type){
			case SignalType::MAINSCREEN_LOAD:
				this->load_file_menu();
				continue;
			case SignalType::MAINSCREEN_MENU:
				this->options_menu();
				application_settings.commit();
				continue;
			default:
				continue;
		}
	}
}

unsigned GUIElement::handle_event(const SDL_Event &e){
	unsigned ret = SUI::NOTHING;
	auto it = this->children.begin();
	auto end = this->children.end();
	for (; it != end; ++it)
		ret |= (*it)->handle_event(e);
	return ret;
}

unsigned GUIElement::receive(TotalTimeUpdate &x){
	unsigned ret = SUI::NOTHING;
	for (auto &child : this->children)
		ret |= child->receive(x);
	return ret;
}

unsigned GUIElement::receive(MetaDataUpdate &x){
	unsigned ret = SUI::NOTHING;
	for (auto &child : this->children)
		ret |= child->receive(x);
	return ret;
}

unsigned GUIElement::receive(PlaybackStop &x){
	unsigned ret = SUI::NOTHING;
	for (auto &child : this->children)
		ret |= child->receive(x);
	return ret;
}

unsigned GUIElement::receive(RTPCQueueElement &x){
	return SUI::NOTHING;
}

void GUIElement::update(){
	for (auto &child : this->children)
		child->update();
}

SUI::SUI():
		GUIElement(this, nullptr),
		player(*this),
		current_total_time(-1),
		bounding_square(-1),
		max_square(-1),
		full_update_count(0),
		scc(*this),
		update_requested(0),
		ui_in_foreground(1),
		apply_blur(0){
	this->set_visualization_mode(application_settings.get_visualization_mode());
	this->set_display_fps(application_settings.get_display_fps());
	::get_dots_per_millimeter();
	this->true_resolution.x = 0;
	this->true_resolution.y = 0;
	this->true_resolution.w = get_screen_width();
	this->true_resolution.h = get_screen_height();

	__android_log_print(ANDROID_LOG_INFO, "C++SUI", "Determined resolution: %dx%d\n", this->true_resolution.w, this->true_resolution.h);

	this->screen = GPU_Init(1080 / 2, 1920 / 2, GPU_DEFAULT_INIT_FLAGS | GPU_INIT_ENABLE_VSYNC);
	if (!this->screen)
		throw UIInitializationException("Window creation failed.");

	this->tex_picture.set_target(this->screen);
	this->background_picture.set_target(this->screen);

	this->font.reset(new Font(this->screen));

	this->create_shaders();

	this->scc.start();
}

SUI::~SUI(){
	this->player.terminate_thread(*this);
}

unsigned SUI::handle_keys(const SDL_Event &e){
	unsigned ret = NOTHING;
	switch (e.key.keysym.scancode){
		case SDL_SCANCODE_C:
		case SDL_SCANCODE_ANDROID_AUDIOPLAYPAUSE:
			this->player.request_playpause();
			break;
		case SDL_SCANCODE_X:
			this->player.request_hardplay();
			break;
		case SDL_SCANCODE_ANDROID_AUDIOPLAY:
			this->player.request_play();
			break;
		case SDL_SCANCODE_ANDROID_AUDIOPAUSE:
			this->player.request_pause();
			break;
		case SDL_SCANCODE_AUDIOPLAY:
			this->player.request_play();
			break;
		case SDL_SCANCODE_V:
		case SDL_SCANCODE_AUDIOSTOP:
			this->player.request_stop();
			break;
		case SDL_SCANCODE_B:
		case SDL_SCANCODE_AUDIONEXT:
			this->player.request_next();
			break;
		case SDL_SCANCODE_Z:
		case SDL_SCANCODE_AUDIOPREV:
			this->player.request_previous();
			break;
#if defined WIN32 && 0
		case SDL_SCANCODE_F12:
			{
				boost::shared_array<unsigned char> pixels;
				auto screen = to_surface_t(SDL_GetWindowSurface(this->window.get()));
				if (!screen)
					break;
				auto size = screen->w * screen->h * screen->format->BytesPerPixel;
				pixels.reset(new unsigned char[size]);
				if (SDL_RenderReadPixels(this->renderer.get(), &screen->clip_rect, screen->format->format, pixels.get(), screen->pitch))
					break;
				auto surface = to_surface_t(SDL_CreateRGBSurfaceFrom(
					pixels.get(),
					screen->w,
					screen->h,
					screen->format->BitsPerPixel,
					screen->pitch,
					screen->format->Rmask,
					screen->format->Gmask,
					screen->format->Bmask,
					screen->format->Amask
				));
				//auto new_s = apply_gaussian_blur(surface, 5);
				//SDL_SaveBMP(new_s.get(), "screenshot1.bmp");
				//new_s = apply_gaussian_blur_double(surface, 5);
				//SDL_SaveBMP(new_s.get(), "screenshot2.bmp");
				//auto new_s = apply_box_blur(surface, 20);
				auto new_s = apply_gaussian_blur2(surface, 15);
				SDL_SaveBMP(new_s.get(), "screenshot3.bmp");
			}
			break;
#endif
	}
	return ret;
}

unsigned SUI::handle_event(const SDL_Event &e){
	//Note: Removing this object may incur in undefined behavior.
	auto temp = this->current_element;
	return temp->handle_event(e);
}
	
void SUI::on_switch_to_foreground(){
	this->ui_in_foreground = 1;
	if (this->dpla){
		this->dpla->perform();
		this->dpla.reset();
	}
}

unsigned SUI::handle_in_events(){
	SDL_Event e;
	unsigned ret = NOTHING;
	while (SDL_PollEvent(&e)){
		switch (e.type){
			case SDL_QUIT:
				return QUIT;
			case SDL_WINDOWEVENT:
				ret |= REDRAW;
				break;
			case SDL_KEYDOWN:
				ret |= this->handle_keys(e);
				break;
			case SDL_APP_DIDENTERBACKGROUND:
				this->ui_in_foreground = 0;
				break;
			case SDL_APP_WILLENTERFOREGROUND:
				this->on_switch_to_foreground();
				ret |= REDRAW;
				break;
		}
		ret |= this->handle_event(e);
	}
	return ret;
}

unsigned SUI::receive(TotalTimeUpdate &ttu){
	this->current_total_time = ttu.get_seconds();
	return REDRAW;
}

unsigned SUI::receive(PlaybackStop &x){
	this->tex_picture_source.clear();
	this->tex_picture.unload();
	this->background_picture.unload();
	this->metadata.clear();
	this->current_total_time = -1;
	return REDRAW;
}

unsigned SUI::receive(RTPCQueueElement &x){
	this->perform_internal(x.get_rtpc());
	return NOTHING;
}

unsigned SUI::handle_out_events(){
	boost::shared_ptr<ExternalQueueElement> eqe;
	unsigned ret = NOTHING;
	while (this->player.external_queue_out.try_pop(eqe)){
		ret |= eqe->receive(*this);
		for (auto &p : this->children)
			ret |= eqe->receive(*p);
	}
	return ret;
}

unsigned SUI::handle_finished_jobs(){
	boost::shared_ptr<SUIJob> job;
	unsigned ret = NOTHING;
	while (this->finished_jobs_queue.try_pop(job))
		ret |= job->finish(*this);
	return ret;
}

void SUI::load(bool load, bool file, const std::wstring &path){
	this->player.request_load(load, file, path);
}

#if 0
template <typename T>
void format_memory(std::basic_ostream<T> &stream, size_t size){
	double m = (double)size;
	static const char *units[]={
		" B",
		" KiB",
		" MiB",
		" GiB",
		" TiB",
		" PiB",
		" EiB",
		" ZiB",
		" YiB"
	};
	auto unit = units;
	while (m >= 1024.0){
		m *= 1.0 / 1024.0;
		unit++;
	}
	stream <<m<<*unit;
}
#endif

void SUI::draw_picture(){
	int sq = this->get_bounding_square();
	if (this->background_picture){
		SDL_Rect rect = { 0, 0, 0, 0 };
		this->background_picture.draw(rect);
	}
	if (this->tex_picture){
		auto rect = this->tex_picture.get_rect();
		SDL_Rect dst = { int((sq - rect.w) / 2), int((sq - rect.h) / 2), 0, 0 };
		this->tex_picture.draw(dst);
	}
}

int SUI::get_bounding_square(){
	if (this->bounding_square >= 0)
		return this->bounding_square;
	int w = this->screen->w,
		h = this->screen->h;
	return this->bounding_square = std::min(w, h);
}

int SUI::get_max_square(){
	if (this->max_square >= 0)
		return this->max_square;
	int w = this->screen->w,
		h = this->screen->h;
	return this->max_square = std::max(w, h);
}

SDL_Rect SUI::get_visible_region() const{
	int w = this->screen->w,
		h = this->screen->h;
	SDL_Rect ret = { 0, 0, w, h, };
	return ret;
}

void SUI::gui_signal(const GuiSignal &signal){
	this->scc.relay(signal);
}

void SUI::request_update(){
	this->update_requested = 1;
}

void SUI::perform(RemoteThreadProcedureCall *rtpc){
	this->player.external_queue_out.push(boost::shared_ptr<ExternalQueueElement>(new RTPCQueueElement(rtpc)));
}

SDL_Rect SUI::get_seekbar_region(){
	auto ret = this->get_visible_region();
	auto square = this->get_bounding_square();
	ret.y += square * 3 / 2;
	ret.h -= square * 3 / 2;
	return ret;
}

void SUI::set_visualization_mode(VisualizationMode mode){
	this->visualization_mode = mode;
}

void SUI::set_display_fps(bool dfps){
	this->display_fps = dfps;
}

int SUI::transform_mouse_x(int x) const{
	auto r = this->get_visible_region();
	return x * r.w / this->true_resolution.w;
}

int SUI::transform_mouse_y(int y) const{
	auto r = this->get_visible_region();
	return y * r.h / this->true_resolution.h;
}

double SUI::get_dots_per_millimeter() const{
	auto r = this->get_visible_region();
	return ::get_dots_per_millimeter() * r.w / this->true_resolution.w;
}

void SUI::loop(){
	Uint32 last = 0;
	std::deque<Uint32> fps_queue;
	unsigned status;
	const auto min_time = (Uint32)(1000.0 / 60.0);
	while (!check_flag(status = this->handle_in_events(), QUIT)){
		try{
			status |= this->handle_out_events();
		}catch (const DeviceInitializationException &e){
			__android_log_print(ANDROID_LOG_INFO, "C++Exception", "Fatal exception caught: %s\n", e.what());
			return;
		}catch (const CR_Exception &e){
			__android_log_print(ANDROID_LOG_INFO, "C++Exception", "Non-fatal exception caught: %s\n", e.what());
		}catch (...){
		}
		status |= this->handle_finished_jobs();
		bool do_redraw = 0;
		Uint32 now_ticks = 0;
		if (this->ui_in_foreground){
			now_ticks = SDL_GetTicks();
			auto delta_t = now_ticks - last;
			do_redraw = do_redraw || this->update_requested;
			do_redraw = do_redraw || delta_t >= 500;
			do_redraw = do_redraw || check_flag(status, REDRAW);
			do_redraw = do_redraw || this->full_update_count > 0;
			do_redraw = do_redraw || this->player.get_state() == PlayState::PLAYING && this->visualization_mode != VisualizationMode::NONE
#ifdef LIMIT_FPS
				&& delta_t >= min_time
#endif
				;
		}
		if (!do_redraw){
			SDL_Delay(min_time / 2);
			continue;
		}
		this->update_requested = 0;

		std::string display_string;
		fps_queue.push_back(now_ticks);
		while (now_ticks - fps_queue.front() > 1000)
			fps_queue.pop_front();
		if (fps_queue.size() > 1){
			this->current_framerate = (float)fps_queue.size() * 1000.f / (float)(now_ticks - fps_queue.front());
			if (this->display_fps){
				std::stringstream stream;
				stream << std::setprecision(3) << std::setw(4) << std::setfill(' ') << this->current_framerate << " fps";
				display_string = stream.str();
			}
		}else
			this->current_framerate = -1;

		GPU_Clear(this->screen);
		this->current_element->update();
		if (display_string.size())
			this->font->draw_text(display_string, 0, 0, INT_MAX, 2.0);
		GPU_Flip(this->screen);
		last = now_ticks;
	}
}
