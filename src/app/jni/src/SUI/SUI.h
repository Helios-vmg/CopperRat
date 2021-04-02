/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "../UserInterface.h"
#include "../Deleters.h"
#include "../Threads.h"
#include "../Image.h"
#include "../ApplicationState.h"
#include "Font.h"
#include "Signal.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#include <string>
#include <memory>
#include <list>
#include <functional>
#include <deque>
#endif

class UIInitializationException : public CR_Exception{
public:
	UIInitializationException(const std::string &desc): CR_Exception(desc){}
	CR_Exception *clone() const{
		return new UIInitializationException(*this);
	}
};

class SUI;
class MainScreen;
class AudioPlayer;
class AudioPlayerState;

#if 0
class PictureDecodingJob : public SUIJob{
	std::shared_ptr<GenericMetadata> metadata;
	unsigned target_square;
	surface_t picture;
	SDL_Rect trim_rect;
	std::wstring current_source,
		source,
		cached_source;
	std::string hash;
	bool skip_loading = false,
		skip_resize;
	MainScreen *receiver;
	
	void sui_perform(WorkerThread &wt);
	void load_picture_from_filesystem();
	bool load_picture_from_cache(const std::wstring &);
public:
	std::string description;
	PictureDecodingJob(
			finished_jobs_queue_t &queue,
			std::shared_ptr<GenericMetadata> metadata,
			unsigned target_square,
			const SDL_Rect &trim_rect,
			const std::wstring &current_source,
			MainScreen &receiver):
		SUIJob(queue),
		metadata(metadata),
		target_square(target_square),
		trim_rect(trim_rect),
		picture(nullptr, SDL_Surface_deleter_func),
		current_source(current_source),
		receiver(&receiver){}
	unsigned finish(SUI &);
	surface_t get_picture(){
		return this->picture;
	}
	const std::wstring &get_source() const {
		return this->source;
	}
	bool get_skip_loading() const{
		return this->skip_loading;
	}
	const std::string &get_hash() const{
		return this->hash;
	}
	MainScreen &get_receiver() const{
		return *this->receiver;
	}
};

class PictureBlurringJob : public SUIJob{
	unsigned target_square;
	surface_t picture;
	SDL_Rect trim_rect;
	std::wstring path;
	MainScreen *receiver;
	void sui_perform(WorkerThread &wt);
public:
	PictureBlurringJob(
			finished_jobs_queue_t &queue,
			unsigned target_square,
			const SDL_Rect &trim_rect,
			surface_t picture,
			const std::wstring &path,
			MainScreen &receiver):
		SUIJob(queue),
		target_square(target_square),
		trim_rect(trim_rect),
		picture(picture),
		path(path),
		receiver(&receiver){}
	unsigned finish(SUI &);
	surface_t get_picture(){
		return this->picture;
	}
	MainScreen &get_receiver() const{
		return *this->receiver;
	}
};
#endif

class SUI;

class GUIElement : public UserInterface{
protected:
	SUI *sui;
	GUIElement *parent;
	std::list<std::shared_ptr<GUIElement> > children;

public:
	GUIElement(SUI *sui, GUIElement *parent): sui(sui), parent(parent){}
	virtual ~GUIElement(){}
	virtual void update();
	virtual unsigned handle_event(const SDL_Event &);
	SUI &get_ui() const{
		return *this->sui;
	}
};

class SUI : public GUIElement{
	friend class SUIControlCoroutine;
public:
	enum InputStatus{
		NOTHING = 0,
		QUIT = 1,
		REDRAW = 2,
	};
private:
	AudioPlayer *player;
	std::mutex async_callbacks_mutex;
	std::deque<std::function<void()>> async_callbacks;
	std::deque<std::function<void()>> fg_async_callbacks;
	GPU_Target *screen;
	std::shared_ptr<Font> font;
	int bounding_square = -1;
	int max_square = -1;
	WorkerThread worker;
	int full_update_count = 0;
	typedef std::shared_ptr<GUIElement> current_element_t;
	std::vector<std::vector<current_element_t>> stacks;
	size_t active_stack = 0;
	bool update_requested = false;
	bool ui_in_foreground = true;
	bool apply_blur = false;
	ShaderProgram blur_h, blur_v;
	float current_framerate;
	VisualizationMode visualization_mode;
	bool display_fps;
	SDL_Rect true_resolution;

	unsigned handle_event(const SDL_Event &e);
	unsigned handle_keys(const SDL_Event &e);
	unsigned handle_in_events();
	void handle_out_events();
	void process(decltype(async_callbacks) &);
	//load: true for load, false for add
	//file: true for file, false for directory
	void load(bool load, bool file, std::wstring &&path);
	void on_switch_to_foreground();
	void create_shaders();
	void start_gui(std::vector<current_element_t> &, AudioPlayerState &);
	void load_file_menu();
	void options_menu();
	template <typename T>
	void display(const std::shared_ptr<T> &p){
		this->display(this->stacks[this->active_stack], p);
	}
	template <typename T>
	void display(std::vector<current_element_t> &dst, const std::shared_ptr<T> &p){
		dst.emplace_back(p);
		this->request_update();
	}
	void undisplay(){
		this->stacks[this->active_stack].pop_back();
		this->request_update();
	}
	void switch_to_next_player();
	void switch_to_previous_player();
	void switch_player(int direction);
	void set_geometry();
public:
	SUI(AudioPlayer &player);
	~SUI();
	void initialize();
	void loop();
	AudioPlayer &get_player(){
		return *this->player;
	}
	const AudioPlayer &get_player() const{
		return *this->player;
	}
	std::shared_ptr<Font> get_font() const{
		return this->font;
	}
	GPU_Target *get_target() const{
		return this->screen;
	}
	int get_bounding_square() const{
		return this->bounding_square;
	}
	int get_max_square() const{
		return this->max_square;
	}
	SDL_Rect get_visible_region() const;
	void start_full_updating(){
		this->full_update_count++;
	}
	void end_full_updating(){
		this->full_update_count--;
	}
	void request_update();
	
	SDL_Rect get_seekbar_region();
	float get_current_framerate() const{
		return this->current_framerate;
	}
	void set_visualization_mode(VisualizationMode mode);
	void set_display_fps(bool);
	VisualizationMode get_visualization_mode() const{
		return this->visualization_mode;
	}
	int transform_mouse_x(int x) const;
	int transform_mouse_y(int y) const;
	double get_dots_per_millimeter() const;
	Texture blur_image(Texture tex);
	bool in_foreground() const{
		return this->ui_in_foreground;
	}
	void push_async_callback(std::function<void()> &&f, bool fg_only = false);
	void push_fg_async_callback(std::function<void()> &&f){
		this->push_async_callback(std::move(f), true);
	}
	void push_parallel_work(std::function<void()> &&);
};
