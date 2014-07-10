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
#ifndef HAVE_PRECOMPILED_HEADERS
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#endif

SUIControlCoroutine::SUIControlCoroutine(SUI &sui):
		sui(&sui),
		co([this](co_t::pull_type &pt){ this->antico = &pt; this->entry_point(); }){
}

void SUIControlCoroutine::start(){
	this->co(GuiSignal());
}

void SUIControlCoroutine::relay(const GuiSignal &s){
	this->co(s);
}

GuiSignal SUIControlCoroutine::display(boost::shared_ptr<GUIElement> el){
	this->sui->current_element = el;
	this->sui->request_update();
	(*this->antico)();
	return this->antico->get();
}

bool SUIControlCoroutine::load_file(std::wstring &dst, bool only_directories){
	boost::shared_ptr<FileBrowser> browser(new FileBrowser(this->sui, this->sui, !only_directories));
	while (1){
		auto signal = this->display(browser);
		switch (signal.type){
			case SignalType::BACK_PRESSED:
				return 0;
			case SignalType::FILE_BROWSER_DONE:
				break;
			default:
				continue;
		}
		if (!signal.data.file_browser_success)
			return 0;
		break;
	}
	dst = browser->get_selection();
	return 1;
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
	while (1){
		auto signal = this->display(lv);
		switch (signal.type){
			case SignalType::BACK_PRESSED:
				return;
			case SignalType::LISTVIEW_SIGNAL:
				break;
			default:
				continue;
		}
		if (signal.data.listview_signal.listview_name != 0)
			continue;
		signal = *signal.data.listview_signal.signal;
		if (signal.type != SignalType::BUTTON_SIGNAL)
			continue;
		bool load = signal.data.button_signal / 2 == 0;
		bool file = signal.data.button_signal % 2 == 0;
		std::wstring path;
		if (!this->load_file(path, !file))
			return;
		this->sui->load(load, file, path);
		break;
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
		full_update_count(0),
		scc(*this),
		update_requested(0){
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
	}
	return ret;
}

unsigned SUI::handle_event(const SDL_Event &e){
	//Note: Removing this object may incur in undefined behavior.
	auto temp = this->current_element;
	return temp->handle_event(e);
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
				this->picture = load_image_from_file(directory + L"/" + filename);
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

unsigned SUI::receive(PlaybackStop &x){
	this->tex_picture.unload();
	this->metadata.clear();
	this->current_total_time = -1;
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

void SUI::gui_signal(const GuiSignal &signal){
	this->scc.relay(signal);
}

void SUI::request_update(){
	this->update_requested = 1;
}

#include "../AudioBuffer.h"

void SUI::loop(){
	Uint32 last = 0;
	unsigned status;
	while (!check_flag(status = this->handle_in_events(), QUIT)){
		status |= this->handle_out_events();
		status |= this->handle_finished_jobs();
		Uint32 now_ticks = SDL_GetTicks();

		bool do_redraw = 0;
		do_redraw = do_redraw || this->update_requested;
		do_redraw = do_redraw || now_ticks - last >= 500;
		do_redraw = do_redraw || check_flag(status, REDRAW);
		do_redraw = do_redraw || this->full_update_count > 0;
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
