#ifndef SUI_H
#define SUI_H

#include "../AudioPlayer.h"
#include "../UserInterface.h"
#include "../auto_ptr.h"
#include "../Deleters.h"
#include "../Threads.h"
#include "../Image.h"
#include "Font.h"
#include "Signal.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/coroutine/all.hpp>
#include <memory>
#include <list>
#endif

struct UIInitializationException{
	std::string desc;
	UIInitializationException(const std::string &desc): desc(desc){}
};

class SUIJob;
class SUI;

typedef thread_safe_queue<boost::shared_ptr<SUIJob> > finished_jobs_queue_t;

class SUIJob : public WorkerThreadJob{
	finished_jobs_queue_t &queue;
	virtual void sui_perform(WorkerThread &) = 0;
public:
	SUIJob(finished_jobs_queue_t &queue): queue(queue){}
	void perform(WorkerThread &wt){
		this->sui_perform(wt);
		this->queue.push(boost::static_pointer_cast<SUIJob>(wt.get_current_job()));
	}
	virtual unsigned finish(SUI &) = 0;
};

#define SDL_PTR_WRAPPER(T) CR_UNIQUE_PTR2(T, void(*)(T *))

class PictureDecodingJob : public SUIJob{
	boost::shared_ptr<GenericMetadata> metadata;
	unsigned target_square;
	surface_t picture;
	void sui_perform(WorkerThread &wt);
	void load_picture_from_filesystem();
public:
	PictureDecodingJob(finished_jobs_queue_t &queue, boost::shared_ptr<GenericMetadata> metadata, unsigned target_square):
		SUIJob(queue),
		metadata(metadata),
		target_square(target_square),
		picture(nullptr, SDL_Surface_deleter_func){}
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
	std::list<boost::shared_ptr<GUIElement> > children;

public:
	GUIElement(SUI *sui, GUIElement *parent): sui(sui), parent(parent){}
	virtual ~GUIElement(){}
	virtual void update();
	virtual unsigned handle_event(const SDL_Event &);
	virtual unsigned receive(TotalTimeUpdate &);
	virtual unsigned receive(MetaDataUpdate &);
	virtual unsigned receive(PlaybackStop &);
	virtual void gui_signal(const GuiSignal &){}
};

class SUI;

class SUIControlCoroutine{
	SUI *sui;
	typedef boost::coroutines::coroutine<GuiSignal> co_t;
	co_t::push_type co;
	co_t::pull_type *antico;

	GuiSignal display(boost::shared_ptr<GUIElement>);
	void entry_point();
	void load_file_menu();
	bool load_file(std::wstring &dst, bool only_directories);
public:
	SUIControlCoroutine(SUI &sui);
	void start();
	void relay(const GuiSignal &);
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
	AudioPlayer &player;
	finished_jobs_queue_t finished_jobs_queue;
	SDL_PTR_WRAPPER(SDL_Window) window;
	renderer_t renderer;
	boost::shared_ptr<Font> font;
	double current_total_time;
	std::wstring metadata;
	Texture tex_picture;
	int bounding_square;
	WorkerThread worker;
	boost::shared_ptr<WorkerThreadJobHandle> picture_job;
	int full_update_count;
	typedef boost::shared_ptr<GUIElement> current_element_t;
	current_element_t current_element;
	SUIControlCoroutine scc;
	bool update_requested;

	unsigned handle_event(const SDL_Event &e);
	unsigned handle_keys(const SDL_Event &e);
	unsigned handle_in_events();
	unsigned handle_out_events();
	unsigned handle_finished_jobs();
	//load: true for load, false for add
	//file: true for file, false for directory
	void load(bool load, bool file, const std::wstring &path);
public:
	SUI(AudioPlayer &player);
	void loop();

	unsigned receive(TotalTimeUpdate &);
	unsigned receive(MetaDataUpdate &);
	unsigned receive(PlaybackStop &x);
	unsigned finish(PictureDecodingJob &);
	void draw_picture();
	double get_current_total_time() const{
		return this->current_total_time;
	}
	const std::wstring &get_metadata() const{
		return this->metadata;
	}
	boost::shared_ptr<Font> get_font() const{
		return this->font;
	}
	renderer_t get_renderer() const{
		return this->renderer;
	}
	int get_bounding_square();
	SDL_Rect get_visible_region();
	void start_full_updating(){
		this->full_update_count++;
	}
	void end_full_updating(){
		this->full_update_count--;
	}
	void gui_signal(const GuiSignal &);
	void request_update();
};

#endif
