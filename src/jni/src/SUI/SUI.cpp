#include "SUI.h"
#include "../CommonFunctions.h"
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

SUI::SUI(AudioPlayer &player):
		player(player),
		window(nullptr, [](SDL_Window *w) { if (w) SDL_DestroyWindow(w); }),
		renderer(nullptr, SDL_Renderer_deleter()),
		current_total_time(-1){
	this->window.reset(SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0));
	if (!this->window)
		throw UIInitializationException("Window creation failed.");
	this->renderer.reset(SDL_CreateRenderer(this->window.get(), -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC), SDL_Renderer_deleter());
	if (!this->renderer)
		throw UIInitializationException("Renderer creation failed.");
	this->font.reset(new Font(this->renderer));
	SDL_SetRenderDrawColor(renderer.get(), 255, 0, 255, 255);
}

bool p = 1;

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
				{
					switch (e.key.keysym.sym){
						case SDLK_RIGHT:
							this->player.request_seek(5);
							break;
						case SDLK_LEFT:
							this->player.request_seek(-5);
							break;
						case SDLK_x:
							this->player.request_play();
							break;
						case SDLK_c:
							this->player.request_pause();
							break;
						case SDLK_b:
							this->player.request_next();
							break;
					}
				}
				break;
		}
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

unsigned SUI::receive(MetaDataUpdate &mdu){
	auto metadata = mdu.get_metadata();
	this->metadata.clear();
	auto f = [this](const std::wstring &key, const std::wstring &value){
		this->metadata += wide_to_narrow(key);
		this->metadata += '=';
		if (key == L"METADATA_BLOCK_PICTURE")
			this->metadata += "<picture data>";
		else
			this->metadata += wide_to_narrow(value);

		this->metadata += '\n';
	};
	metadata->iterate(f);
	return REDRAW;
}

unsigned SUI::handle_out_events(){
	boost::shared_ptr<ExternalQueueElement> eqe;
	unsigned ret = 0;
	while (this->player.external_queue_out.try_pop(eqe))
		ret |= eqe->receive(*this);
	return ret;
}

void format_memory(std::ostream &stream, size_t size){
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

#include "../AudioBuffer.h"

void SUI::loop(){
	size_t max_memory = 0;
	Uint32 last = 0;
	unsigned status;
	while (!check_flag(status = this->handle_in_events(), QUIT)){
		status |= this->handle_out_events();
		Uint32 now_ticks = SDL_GetTicks();
		if (now_ticks - last >= 500 || check_flag(status, REDRAW)){
			last = now_ticks;
			std::stringstream stream;
			parse_into_hms(stream, player.get_current_time());
			stream <<" / ";
			parse_into_hms(stream, current_total_time);
			stream <<std::endl<<this->metadata<<"\n\n";
			{
				unsigned instances;
				size_t memory;
				audio_buffer_t::abit.get_info(instances, memory);
				max_memory = std::max(max_memory, memory);
				stream <<"Audio buffers: "<<instances<<"\n"
				         "Memory used:   ";
				format_memory(stream, memory);
				stream <<"\n"
				         "Peak memory:   ";
				format_memory(stream, max_memory);
			}
			SDL_RenderClear(this->renderer.get());
			this->font->draw_text(stream.str(), 0, 0);
			SDL_RenderPresent(this->renderer.get());
		}else
			SDL_Delay((Uint32)(1000.0/60.0));
	}
}
