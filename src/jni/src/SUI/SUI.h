#ifndef SUI_H
#define SUI_H

#include "../AudioPlayer.h"
#include "../UserInterface.h"
#include <SDL.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <memory>
#include "Font.h"
#include "../auto_ptr.h"
#include "../Deleters.h"
#include "../Threads.h"

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
	SDL_PTR_WRAPPER(SDL_Surface) picture;
	void sui_perform(WorkerThread &wt);
	void load_picture_from_filesystem();
public:
	PictureDecodingJob(finished_jobs_queue_t &queue, boost::shared_ptr<GenericMetadata> metadata, unsigned target_square):
		SUIJob(queue),
		metadata(metadata),
		target_square(target_square),
		picture(nullptr, SDL_Surface_deleter_func){}
	unsigned finish(SUI &);
	SDL_Surface *get_picture(){
		return this->picture.get();
	}
};

class SUI : public UserInterface{
	AudioPlayer &player;
	finished_jobs_queue_t finished_jobs_queue;
	SDL_PTR_WRAPPER(SDL_Window) window;
	boost::shared_ptr<SDL_Renderer> renderer;
	boost::shared_ptr<Font> font;
	double current_total_time;
	std::wstring metadata;
	SDL_PTR_WRAPPER(SDL_Texture) tex_picture;
	int bounding_square;
	WorkerThread worker;
	boost::shared_ptr<WorkerThreadJobHandle> picture_job;

	enum InputStatus{
		NOTHING = 0,
		QUIT = 1,
		REDRAW = 2,
	};

	unsigned handle_in_events();
	unsigned handle_out_events();
	unsigned handle_finished_jobs();
	void draw_picture();
	int get_bounding_square();
public:
	SUI(AudioPlayer &player);
	void loop();

	unsigned receive(TotalTimeUpdate &);
	unsigned receive(MetaDataUpdate &);
	unsigned finish(PictureDecodingJob &);
};

#endif
