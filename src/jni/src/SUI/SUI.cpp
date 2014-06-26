#include "SUI.h"
#include "../CommonFunctions.h"
#include "../Image.h"
#include "../File.h"
#include "MainScreen.h"
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

unsigned GUIElement::handle_event(const SDL_Event &e){
	unsigned ret = SUI::NOTHING;
	for (auto &child : this->children)
		ret |= child->handle_event(e);
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

void GUIElement::update(){
	for (auto &child : this->children)
		child->update();
}

SUI::SUI(AudioPlayer &player):
		GUIElement(this, nullptr),
		player(player),
		window(nullptr, [](SDL_Window *w) { if (w) SDL_DestroyWindow(w); }),
		renderer(nullptr, SDL_Renderer_deleter()),
		current_total_time(-1),
		bounding_square(-1),
		full_update_count(0){
	get_dots_per_millimeter();

	this->window.reset(SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1080/2, 1920/2, 0));
	if (!this->window)
		throw UIInitializationException("Window creation failed.");

	this->renderer.reset(SDL_CreateRenderer(this->window.get(), -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC), SDL_Renderer_deleter());
	if (!this->renderer)
		throw UIInitializationException("Renderer creation failed.");

	this->tex_picture.set_renderer(this->renderer);

	this->font.reset(new Font(this->renderer));

	this->children.push_back(boost::shared_ptr<GUIElement>(new MainScreen(this, this, this->player)));

	SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
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
		}
		ret |= GUIElement::handle_event(e);
	}
	return ret;
}

std::string wide_to_narrow(const std::wstring &s){
	std::string ret;
	ret.reserve(s.size());
	for (auto wc : s)
		ret.push_back(wc < 128 ? wc : '?');
	return ret;
}

unsigned SUI::receive(TotalTimeUpdate &ttu){
	this->current_total_time = ttu.get_seconds();
	return REDRAW;
}

void PictureDecodingJob::sui_perform(WorkerThread &wt){
	unsigned char *buffer;
	size_t length;
	if (this->metadata->picture(buffer, length))
		this->picture = load_image_from_memory(buffer, length);
	else
		this->load_picture_from_filesystem();
	if (!!this->picture)
		this->picture = bind_surface_to_square(this->picture, this->target_square);
}

void PictureDecodingJob::load_picture_from_filesystem(){
	auto path = this->metadata->get_path();
	auto directory = get_contaning_directory(path);

	std::wstring patterns[5];
	for (auto &pattern : patterns)
		pattern = directory;

	{
		patterns[0] += L"front.*";
		patterns[1] += L"cover.*";
		{
			patterns[2] = path;
			auto last_dot = patterns[2].rfind('.');
			patterns[2] = patterns[2].substr(0, last_dot);
			patterns[2] += L".*";
		}
		{
			patterns[3] += this->metadata->album();
			patterns[3] += L".*";
		}
		patterns[4] = L"folder.*";
	}

	std::vector<std::wstring> files;
	list_files(files, directory);

	for (auto &pattern : patterns){
		for (auto &filename : files){
			if (glob(pattern.c_str(), filename.c_str(), [](wchar_t c){ return tolower(c == '\\' ? '/' : c); })){
				this->picture = load_image_from_file(filename);
				if (!!this->picture)
					return;
			}
		}
	}
}

unsigned PictureDecodingJob::finish(SUI &sui){
	return sui.finish(*this);
}

unsigned SUI::receive(MetaDataUpdate &mdu){
	auto metadata = mdu.get_metadata();
	this->metadata.clear();
	this->metadata += metadata->track_number();
	this->metadata += L" - ";
	this->metadata += metadata->track_artist();
	this->metadata += L" - ";
	this->metadata += metadata->track_title();

	boost::shared_ptr<PictureDecodingJob> job(new PictureDecodingJob(this->finished_jobs_queue, metadata, this->get_bounding_square()));
	this->picture_job = this->worker.attach(job);

	return REDRAW;
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

unsigned SUI::finish(PictureDecodingJob &job){
	unsigned ret = NOTHING;
	if (job.get_id() != this->picture_job->get_id())
		return ret;
	auto picture = job.get_picture();
	if (!picture)
		this->tex_picture.unload();
	else{
		this->tex_picture.load(picture);
		ret = REDRAW;
	}
	this->picture_job.reset();
	return ret;
}

unsigned SUI::handle_finished_jobs(){
	boost::shared_ptr<SUIJob> job;
	unsigned ret = NOTHING;
	while (this->finished_jobs_queue.try_pop(job))
		ret |= job->finish(*this);
	return ret;
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
	SDL_Rect dst = { 0, 0, 0, 0 };
	this->tex_picture.draw(dst);
}

int SUI::get_bounding_square(){
	if (this->bounding_square >= 0)
		return this->bounding_square;
	int w, h;
	SDL_GetWindowSize(this->window.get(), &w, &h);
	return this->bounding_square = std::min(w, h);
}

SDL_Rect SUI::get_visible_region(){
	int w, h;
	SDL_GetWindowSize(this->window.get(), &w, &h);
	SDL_Rect ret = { 0, 0, w, h, };
	return ret;
}

#include "../AudioBuffer.h"

void SUI::loop(){
	Uint32 last = 0;
	unsigned status;
	while (!check_flag(status = this->handle_in_events(), QUIT)){
		status |= this->handle_out_events();
		status |= this->handle_finished_jobs();
		Uint32 now_ticks = SDL_GetTicks();

		bool do_redraw = now_ticks - last >= 500 || check_flag(status, REDRAW) || this->full_update_count > 0;
		if (!do_redraw){
			SDL_Delay((Uint32)(1000.0/60.0));
			continue;
		}

		last = now_ticks;
		SDL_RenderClear(this->renderer.get());
		for (auto &p : this->children){
			p->update();
		}
		SDL_RenderPresent(this->renderer.get());
	}
}
