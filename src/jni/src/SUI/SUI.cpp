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
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <boost/shared_array.hpp>
#endif

class DelayedPictureLoadStartAction : public DelayedPictureLoadAction{
	SUI *sui;
	boost::shared_ptr<PictureDecodingJob> job;
public:
	DelayedPictureLoadStartAction(SUI &sui, boost::shared_ptr<PictureDecodingJob> job): sui(&sui), job(job){}
	void perform(){
#ifdef _DEBUG
		std::string temp = "Resuming loading of ";
		temp += this->job->description;
		temp += " due to switch to foreground.";
		__android_log_print(ANDROID_LOG_DEBUG, "C++DPLA", "%s", temp.c_str());
#endif
		this->sui->start_picture_load(this->job);
	}
};

class DelayedTextureLoadAction : public DelayedPictureLoadAction{
	Texture *dst;
	surface_t src;
public:
	DelayedTextureLoadAction(Texture &dst, surface_t src): dst(&dst), src(src){}
	void perform(){
		__android_log_print(ANDROID_LOG_DEBUG, "C++DPLA", "%s", "Resuming texture creation due to switch to foreground.");
		if (!this->src)
			this->dst->unload();
		else
			this->dst->load(this->src);
	}
};

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
		std::vector<std::wstring> strings;
		strings.push_back(L"Playback mode: " + to_string(playback_mode));
		strings.push_back(std::wstring(L"Shuffling: O") + (shuffling ? L"N" : L"FF"));
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
		window(nullptr, [](SDL_Window *w) { if (w) SDL_DestroyWindow(w); }),
		renderer(nullptr, SDL_Renderer_deleter()),
		current_total_time(-1),
		bounding_square(-1),
		full_update_count(0),
		scc(*this),
		update_requested(0),
		ui_in_foreground(1){
	get_dots_per_millimeter();

	this->window.reset(SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1080/2, 1920/2, 0));
	if (!this->window)
		throw UIInitializationException("Window creation failed.");

	this->renderer.reset(SDL_CreateRenderer(this->window.get(), -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC), SDL_Renderer_deleter());
	if (!this->renderer)
		throw UIInitializationException("Renderer creation failed.");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	this->tex_picture.set_renderer(this->renderer);

	this->font.reset(new Font(this->renderer));

	this->scc.start();

	SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
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
#ifdef WIN32
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
				auto new_s = apply_gaussian_blur2(surface, 5);
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

	{
		patterns[0] = L"front.*";
		patterns[1] = L"cover.*";
		{
			patterns[2] = path;
			auto last_dot = patterns[2].rfind('.');
			patterns[2] = patterns[2].substr(0, last_dot);
			patterns[2] += L".*";
		}
		{
			patterns[3] = this->metadata->album();
			patterns[3] += L".*";
		}
		patterns[4] = L"folder.*";
	}

	std::vector<DirectoryElement> files;
	list_files(files, directory, FilteringType::RETURN_FILES);

	for (auto &pattern : patterns){
		for (auto &element : files){
			const auto &filename = element.name;
			if (glob(pattern.c_str(), filename.c_str(), [](wchar_t c){ return tolower(c == '\\' ? '/' : c); })){
				auto path = directory;
				path += L"/";
				path += filename;
				if (path == this->current_source)
					continue;
				this->picture = load_image_from_file(path);
				if (!!this->picture){
					this->source = path;
					return;
				}
			}
		}
	}
	this->skip_loading = !!this->current_source.size();
}

unsigned PictureDecodingJob::finish(SUI &sui){
	return sui.finish(*this);
}

void SUI::start_picture_load(boost::shared_ptr<PictureDecodingJob> job){
	this->picture_job = this->worker.attach(job);
}

unsigned SUI::receive(MetaDataUpdate &mdu){
	auto metadata = mdu.get_metadata();
	this->metadata.clear();
	auto track_number = metadata->track_number();
	auto track_artist = metadata->track_artist();
	auto track_title = metadata->track_title();
	auto size = this->metadata.size();
	this->metadata += track_number;
	if (size < this->metadata.size())
		this->metadata += L" - ";
	size = this->metadata.size();
	this->metadata += track_artist;
	if (size < this->metadata.size())
		this->metadata += L" - ";
	this->metadata += track_title;

	if (!this->metadata.size())
		this->metadata = get_filename(metadata->get_path());

	boost::shared_ptr<PictureDecodingJob> job(new PictureDecodingJob(this->finished_jobs_queue, metadata, this->get_bounding_square(), this->tex_picture_source));
	job->description = to_string(metadata->get_path());
	if (this->ui_in_foreground)
		this->start_picture_load(job);
	else{
#ifdef _DEBUG
		std::string temp = "Suspending loading of ";
		temp += job->description;
		temp += " due to switch to background.";
		__android_log_print(ANDROID_LOG_INFO, "C++DPLA", "%s", temp.c_str());
#endif
		this->dpla.reset(new DelayedPictureLoadStartAction(*this, job));
	}
	return NOTHING;
}

unsigned SUI::receive(PlaybackStop &x){
	this->tex_picture_source.clear();
	this->tex_picture.unload();
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

unsigned SUI::finish(PictureDecodingJob &job){
	unsigned ret = NOTHING;
	if (job.get_id() != this->picture_job->get_id())
		return ret;
	auto picture = job.get_picture();
	this->picture_job.reset();
	if (job.get_skip_loading()){
		__android_log_print(ANDROID_LOG_INFO, "C++AlbumArt", "%s", "Album art load optimized away.\n");
		return ret;
	}
	if (this->ui_in_foreground){
		if (!picture){
			this->tex_picture_source.clear();
			this->tex_picture.unload();
		}else{
			this->tex_picture_source = job.get_source();
			this->tex_picture.load(picture);
			ret = REDRAW;
		}
	}else{
		__android_log_print(ANDROID_LOG_INFO, "C++DPLA", "%s", "Suspending texture creation due to switch to background.");
		this->dpla.reset(new DelayedTextureLoadAction(this->tex_picture, picture));
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
	auto rect = this->tex_picture.get_rect();
	int sq = this->get_bounding_square();
	SDL_Rect dst = { (sq - rect.w) / 2, (sq - rect.h) / 2, 0, 0 };
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

void SUI::gui_signal(const GuiSignal &signal){
	this->scc.relay(signal);
}

void SUI::request_update(){
	this->update_requested = 1;
}

void SUI::perform(RemoteThreadProcedureCall *rtpc){
	this->player.external_queue_out.push(boost::shared_ptr<ExternalQueueElement>(new RTPCQueueElement(rtpc)));
}

#include "../AudioBuffer.h"

void SUI::loop(){
	Uint32 last = 0;
	unsigned status;
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
			do_redraw = do_redraw || this->update_requested;
			do_redraw = do_redraw || now_ticks - last >= 500;
			do_redraw = do_redraw || check_flag(status, REDRAW);
			do_redraw = do_redraw || this->full_update_count > 0;
		}
		if (!do_redraw){
			SDL_Delay((Uint32)(1000.0/60.0));
			continue;
		}
		this->update_requested = 0;

		last = now_ticks;
		SDL_RenderClear(this->renderer.get());
		this->current_element->update();
		SDL_RenderPresent(this->renderer.get());
	}
}
