/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "../stdafx.h"
#include "SUI.h"
#include "../CommonFunctions.h"
#include "../Image.h"
#include "../File.h"
#include "MainScreen.h"
#include "FileBrowser.h"
#include "ListView.h"
#include "../ApplicationState.h"
#include "../AudioBuffer.h"
#include "../Rational.h"
#include "../AudioPlayer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#endif

//#define LIMIT_FPS

unsigned GUIElement::handle_event(const SDL_Event &e){
	unsigned ret = SUI::NOTHING;
	auto it = this->children.begin();
	auto end = this->children.end();
	for (; it != end; ++it)
		ret |= (*it)->handle_event(e);
	return ret;
}

void GUIElement::update(){
	for (auto &child : this->children)
		child->update();
}

SUI::SUI(AudioPlayer &player): GUIElement(this, nullptr), player(&player){}

SUI::~SUI(){}

void SUI::initialize(){
	this->set_visualization_mode(application_state->get_visualization_mode());
	this->set_display_fps(application_state->get_display_fps());
	::get_dots_per_millimeter();
	this->true_resolution.x = 0;
	this->true_resolution.y = 0;
	auto w = this->true_resolution.w = get_screen_width();
	auto h = this->true_resolution.h = get_screen_height();

	typedef Rational<int> Q;

    auto ratio = Q(w, h);
    const auto tall = Q(9, 16);

    w = 1080 / 2;
    h = 1920 / 2;

	__android_log_print(ANDROID_LOG_INFO, "C++SUI", "Determined resolution: %dx%d\n", this->true_resolution.w, this->true_resolution.h);

	if (ratio > tall)
	    h = (int)(Q(w) / ratio);
	else if (ratio < tall)
		w = (int)(Q(h) * ratio);

	__android_log_print(ANDROID_LOG_INFO, "C++SUI", "Using resolution: %dx%d\n", w, h);

	this->screen = GPU_Init(w, h, GPU_DEFAULT_INIT_FLAGS | GPU_INIT_ENABLE_VSYNC);
	if (!this->screen)
		throw UIInitializationException("Window creation failed.");

	this->set_geometry();
	
	this->font.reset(new Font(this->screen));

	this->create_shaders();

	auto &map = this->sui->player->get_players();
	auto &current = this->sui->player->get_current_player();
	this->stacks.reserve(map.size());
	size_t i = 0;
	for (auto &kv : map){
		if (&current == kv.second.get())
			this->active_stack = i;
		i++;
		std::vector<current_element_t> stack;
		this->start_gui(stack, *kv.second);
		this->stacks.emplace_back(std::move(stack));
	}
}

void SUI::set_geometry(){
	auto w = this->screen->w,
		h = this->screen->h;
	this->bounding_square = std::min(w, h);
	this->max_square = std::max(w, h);
}


unsigned SUI::handle_keys(const SDL_Event &e){
	unsigned ret = NOTHING;
	switch (e.key.keysym.scancode){
		//case SDL_SCANCODE_ANDROID_AUDIOPLAYPAUSE:
		case SDL_SCANCODE_C:
			this->player->request_playpause();
			break;
		case SDL_SCANCODE_X:
			this->player->request_hardplay();
			break;
		/*case SDL_SCANCODE_ANDROID_AUDIOPLAY:
			this->player->request_play();
			break;
		case SDL_SCANCODE_ANDROID_AUDIOPAUSE:
			this->player->request_pause();
			break;*/
		case SDL_SCANCODE_AUDIOPLAY:
			this->player->request_play();
			break;
		case SDL_SCANCODE_V:
		case SDL_SCANCODE_AUDIOSTOP:
			this->player->request_stop();
			break;
		case SDL_SCANCODE_B:
		case SDL_SCANCODE_AUDIONEXT:
			this->player->request_next();
			break;
		case SDL_SCANCODE_Z:
		case SDL_SCANCODE_AUDIOPREV:
			this->player->request_previous();
			break;
		case SDL_SCANCODE_RIGHT:
			this->switch_to_next_player();
			break;
		case SDL_SCANCODE_LEFT:
			this->switch_to_previous_player();
			break;
#if defined WIN32 && 0
		case SDL_SCANCODE_F12:
			{
				std::unique_ptr<unsigned char[]> pixels;
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
	auto &stack = this->stacks[this->active_stack];
	auto temp = stack;
	auto ret = stack.back()->handle_event(e);
	return ret;
}
	
void SUI::on_switch_to_foreground(){
	this->ui_in_foreground = true;
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

void SUI::process(decltype(async_callbacks) &q){
	while (true){
		std::function<void()> f;
		{
			std::lock_guard<std::mutex> lg(this->async_callbacks_mutex);
			if (!q.size())
				break;
			f = pop_front(q);
		}
		if (f)
			f();
	}
}

void SUI::handle_out_events(){
	this->process(this->async_callbacks);
	if (this->ui_in_foreground)
		this->process(this->fg_async_callbacks);
}

void SUI::push_async_callback(std::function<void()> &&f, bool fg_only){
	std::lock_guard<std::mutex> lg(this->async_callbacks_mutex);
	(fg_only ? this->fg_async_callbacks : this->async_callbacks).emplace_back(std::move(f));
}

void SUI::push_parallel_work(std::function<void()> &&f){
	this->worker.attach(std::move(f));
}

void SUI::load(bool load, bool file, std::wstring &&path){
	this->player->request_load(load, file, std::move(path));
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

SDL_Rect SUI::get_visible_region() const{
	int w = this->screen->w,
		h = this->screen->h;
	SDL_Rect ret = { 0, 0, w, h, };
	return ret;
}

void SUI::request_update(){
	this->update_requested = true;
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
	SDL_Color black;
	black.r = 0;
	black.g = 0;
	black.b = 0;
	black.a = 255;
	while (!check_flag(status = this->handle_in_events(), QUIT)){
		try{
			this->handle_out_events();
		}catch (const DeviceInitializationException &e){
			__android_log_print(ANDROID_LOG_INFO, "C++Exception", "Fatal exception caught: %s\n", e.what());
			return;
		}catch (const CR_Exception &e){
			__android_log_print(ANDROID_LOG_INFO, "C++Exception", "Non-fatal exception caught: %s\n", e.what());
		}catch (...){
		}
		bool do_redraw = false;
		Uint32 now_ticks = 0;
		if (this->ui_in_foreground){
			now_ticks = SDL_GetTicks();
			auto delta_t = now_ticks - last;
			do_redraw = do_redraw || this->update_requested;
			do_redraw = do_redraw || delta_t >= 500;
			do_redraw = do_redraw || check_flag(status, REDRAW);
			do_redraw = do_redraw || this->full_update_count > 0;
			do_redraw = do_redraw || this->player->get_state() == PlayState::PLAYING && this->visualization_mode != VisualizationMode::NONE
#ifdef LIMIT_FPS
				&& delta_t >= min_time
#endif
				;
		}
		if (!do_redraw){
			SDL_Delay(min_time / 2);
			continue;
		}
		this->update_requested = false;

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

		GPU_ClearColor(this->screen, black);
		GPU_Clear(this->screen);
		this->stacks[this->active_stack].back()->update();
		if (display_string.size())
			this->font->draw_text(display_string, 0, 0, INT_MAX, 2.0);
		GPU_Flip(this->screen);
		last = now_ticks;
	}
}

void SUI::start_gui(std::vector<current_element_t> &dst, AudioPlayerState &state){
	auto main_screen = std::make_shared<MainScreen>(this->sui, this->sui, state);
	state.main_screen = main_screen.get();
	main_screen->set_on_load_request([this](){ this->load_file_menu(); });
	main_screen->set_on_menu_request([this](){
		this->options_menu();
		application_state->save();
	});
	this->display(dst, main_screen);
}

template <typename T, size_t N>
constexpr size_t array_length(const T (&)[N]){
	return N;
}

void SUI::load_file_menu(){
	static const wchar_t * const options[] = {
		L"Load file...",
		L"Load directory...",
		L"Enqueue file...",
		L"Enqueue directory...",
	};
	auto lv = std::make_shared<ListView>(this->sui, this->sui, options, options + array_length(options));
	this->display(lv);
	lv->set_on_cancel([this](){
		this->undisplay();
	});
	lv->set_on_selection([this](size_t button){
		this->undisplay();
		bool load = button / 2 == 0;
		bool file = button % 2 == 0;

		auto path = application_state->get_last_browse_directory();
		auto root = application_state->get_last_root();
		if (!root.size())
			root = get_external_storage_path();
		auto browser = std::make_shared<FileBrowser>(this->sui, this->sui, file, false, root, path);
		this->display(browser);
		browser->set_on_cancel([this](){
			this->undisplay();
		});
		browser->set_on_accept([this, load, file, root{std::move(root)}](std::wstring &&path){
			auto browser = std::static_pointer_cast<FileBrowser>(this->stacks[this->active_stack].back());
			this->undisplay();
			application_state->set_last_root(root);
			application_state->set_last_browse_directory(browser->get_new_initial_directory());
			this->load(load, file, std::move(path));
		});
	});
}

void SUI::options_menu(){
	std::vector<std::wstring> strings;
	{
		auto &playlist = this->get_player().get_playlist();
		auto playback_mode = playlist.get_playback_mode();
		bool shuffling = playlist.get_shuffle();
		auto visualization_mode = application_state->get_visualization_mode();
		auto display_fps = application_state->get_display_fps();
		strings.push_back(L"Playback mode: " + to_string(playback_mode));
		strings.push_back(std::wstring(L"Shuffling: O") + (shuffling ? L"N" : L"FF"));
		strings.push_back(L"Visualization mode: " + to_string(visualization_mode));
		strings.push_back(std::wstring(L"Display framerate: O") + (display_fps ? L"N" : L"FF"));
	}
	auto lv = std::make_shared<ListView>(this->sui, this->sui, strings.begin(), strings.end());
	this->display(lv);
	lv->set_on_cancel([this](){
		this->undisplay();
	});
	lv->set_on_selection([this](size_t button){
		this->undisplay();
		auto &playlist = this->get_player().get_playlist();
		auto &playback = application_state->get_current_player().get_playback();
		switch (button){
			case 0:
				playback.set_playback_mode(playlist.cycle_mode());
				break;
			case 1:
				playback.set_shuffle(playlist.toggle_shuffle());
				break;
			case 2:
				visualization_mode = (VisualizationMode)(((int)visualization_mode + 1) % (int)VisualizationMode::END);
				application_state->set_visualization_mode(visualization_mode);
				this->set_visualization_mode(visualization_mode);
				break;
			case 3:
				display_fps = !display_fps;
				application_state->set_display_fps(display_fps);
				this->set_display_fps(display_fps);
				break;
		}
		//Note: this is not a recursive call.
		this->options_menu();
	});
}


void SUI::switch_to_next_player(){
	this->switch_player(1);
}

void SUI::switch_to_previous_player(){
	this->switch_player(-1);
}

void SUI::switch_player(int direction){
	this->active_stack = this->active_stack + direction;
	if (this->active_stack >= this->stacks.size()){
		auto &ms = static_cast<MainScreen &>(*this->stacks.back().front());
		auto &player = ms.get_player();
		if (!player.is_empty()){
			auto &new_player = this->player->new_player();
			std::vector<current_element_t> stack;
			this->start_gui(stack, new_player);
			this->stacks.emplace_back(std::move(stack));
			this->active_stack = this->stacks.size() - 1;
		}else{
			this->player->erase(player);
			this->stacks.pop_back();
			this->active_stack = direction > 0 ? 0 : this->stacks.size() - 1;
		}
	}
	auto &screen = static_cast<MainScreen &>(*this->stacks[this->active_stack].front());
	this->player->switch_to_player(screen.get_player());
	this->request_update();
}
