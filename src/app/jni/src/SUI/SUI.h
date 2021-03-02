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

#ifndef SUI_H
#define SUI_H

#include "../AudioPlayer.h"
#include "../UserInterface.h"
#include "../auto_ptr.h"
#include "../Deleters.h"
#include "../Threads.h"
#include "../Image.h"
#include "../Settings.h"
#include "Font.h"
#include "Signal.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#include <string>
#include <memory>
#include <list>
#include <functional>
#endif

class UIInitializationException : public CR_Exception{
public:
	UIInitializationException(const std::string &desc): CR_Exception(desc){}
	CR_Exception *clone() const{
		return new UIInitializationException(*this);
	}
};

class SUIJob;
class SUI;

typedef thread_safe_queue<std::shared_ptr<SUIJob> > finished_jobs_queue_t;

class SUIJob : public WorkerThreadJob{
	finished_jobs_queue_t &queue;
	virtual void sui_perform(WorkerThread &) = 0;
public:
	SUIJob(finished_jobs_queue_t &queue): queue(queue){}
	void perform(WorkerThread &wt){
		this->sui_perform(wt);
		this->queue.push(std::static_pointer_cast<SUIJob>(wt.get_current_job()));
	}
	virtual unsigned finish(SUI &) = 0;
};

#define SDL_PTR_WRAPPER(T) CR_UNIQUE_PTR2(T, void(*)(T *))

class PictureDecodingJob : public SUIJob{
	std::shared_ptr<GenericMetadata> metadata;
	unsigned target_square;
	surface_t picture;
	SDL_Rect trim_rect;
	std::wstring current_source,
		source,
		cached_source;
	std::string hash;
	bool skip_loading,
		skip_resize;
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
			const std::wstring &current_source):
		SUIJob(queue),
		metadata(metadata),
		target_square(target_square),
		trim_rect(trim_rect),
		picture(nullptr, SDL_Surface_deleter_func),
		current_source(current_source),
		skip_loading(0){}
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
};

class PictureBlurringJob : public SUIJob{
	unsigned target_square;
	surface_t picture;
	SDL_Rect trim_rect;
	std::wstring path;
	void sui_perform(WorkerThread &wt);
public:
	PictureBlurringJob(
			finished_jobs_queue_t &queue,
			unsigned target_square,
			const SDL_Rect &trim_rect,
			surface_t picture,
			const std::wstring &path):
		SUIJob(queue),
		target_square(target_square),
		trim_rect(trim_rect),
		picture(picture),
		path(path){}
	unsigned finish(SUI &);
	surface_t get_picture(){
		return this->picture;
	}
};

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
	virtual unsigned receive(TotalTimeUpdate &);
	virtual unsigned receive(MetaDataUpdate &);
	virtual unsigned receive(PlaybackStop &);
	virtual unsigned receive(RTPCQueueElement &);
	virtual void gui_signal(const GuiSignal &){}
};

class DelayedPictureLoadAction{
public:
	virtual ~DelayedPictureLoadAction(){}
	virtual void perform() = 0;
};

class RTPCQueueElement : public ExternalQueueElement{
	RemoteThreadProcedureCall *rtpc;
public:
	RTPCQueueElement(RemoteThreadProcedureCall *rtpc): rtpc(rtpc){}
	RemoteThreadProcedureCall *get_rtpc(){
		return this->rtpc;
	}
	unsigned receive(UserInterface &ui){
		return ui.receive(*this);
	}
};

class SUI : public GUIElement, public RemoteThreadProcedureCallPerformer{
	friend class SUIControlCoroutine;
public:
	enum InputStatus{
		NOTHING = 0,
		QUIT = 1,
		REDRAW = 2,
	};
	//If the currently loaded picture came from a file, this string contains
	//the path. If no picture is currently loaded, or if the one that is loaded
	//came from somewhere other than a file, this string is empty.
	std::wstring tex_picture_source;
private:
	AudioPlayer player;
	finished_jobs_queue_t finished_jobs_queue;
	GPU_Target *screen;
	std::shared_ptr<Font> font;
	double current_total_time;
	std::wstring metadata;
	Texture tex_picture;
	Texture background_picture;
	int bounding_square;
	int max_square;
	WorkerThread worker;
	std::shared_ptr<WorkerThreadJobHandle> picture_job;
	int full_update_count;
	typedef std::shared_ptr<GUIElement> current_element_t;
	std::vector<current_element_t> element_stack;
	bool update_requested;
	bool ui_in_foreground;
	std::shared_ptr<DelayedPictureLoadAction> dpla;
	bool apply_blur;
	ShaderProgram blur_h, blur_v;
	float current_framerate;
	VisualizationMode visualization_mode;
	bool display_fps;
	SDL_Rect true_resolution;

	unsigned handle_event(const SDL_Event &e);
	unsigned handle_keys(const SDL_Event &e);
	unsigned handle_in_events();
	unsigned handle_out_events();
	unsigned handle_finished_jobs();
	//load: true for load, false for add
	//file: true for file, false for directory
	void load(bool load, bool file, const std::wstring &path);
	void on_switch_to_foreground();
	void create_shaders();
	Texture blur_image(Texture tex);
	void start_gui();
	void load_file_menu();
	void options_menu();
	template <typename T>
	void display(const std::shared_ptr<T> &p){
		this->element_stack.emplace_back(p);
		this->request_update();
	}
	void undisplay(){
		this->element_stack.pop_back();
		this->request_update();
	}
public:
	SUI();
	~SUI();
	void loop();

	unsigned receive(TotalTimeUpdate &);
	unsigned receive(MetaDataUpdate &);
	unsigned receive(PlaybackStop &x);
	unsigned receive(RTPCQueueElement &);
	unsigned finish(PictureDecodingJob &);
	unsigned finish(PictureBlurringJob &);
	void draw_picture();
	AudioPlayer &get_player(){
		return this->player;
	}
	const AudioPlayer &get_player() const{
		return this->player;
	}
	double get_current_total_time() const{
		return this->current_total_time;
	}
	const std::wstring &get_metadata() const{
		return this->metadata;
	}
	std::shared_ptr<Font> get_font() const{
		return this->font;
	}
	GPU_Target *get_target() const{
		return this->screen;
	}
	int get_bounding_square();
	int get_max_square();
	SDL_Rect get_visible_region() const;
	void start_full_updating(){
		this->full_update_count++;
	}
	void end_full_updating(){
		this->full_update_count--;
	}
	void gui_signal(const GuiSignal &);
	void request_update();
	void start_picture_load(std::shared_ptr<PictureDecodingJob>);
	void start_picture_blurring(std::shared_ptr<PictureBlurringJob>);
	unsigned finish_picture_load(surface_t picture, const std::wstring &source, const std::string &hash, bool skip_loading);
	unsigned finish_background_load(surface_t picture);
	void perform(RemoteThreadProcedureCall *);
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
};

#endif
