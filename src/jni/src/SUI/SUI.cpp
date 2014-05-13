#include "SUI.h"
#include "../CommonFunctions.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

void draw_string(SDL_Renderer *target, SDL_Texture *font, const char *string, int x, int y){
	SDL_Rect dst, src;
	dst.x = x;
	dst.y = y;
	dst.w = (src.w = 8) * 1;
	dst.h = (src.h = 8) * 1;
	for (; *string; string++){
		if (*string == '\n'){
			dst.x = x;
			dst.y += dst.h;
			continue;
		}
		src.x = src.w * (*string % 16);
		src.y = src.h * (*string / 16 - 2);
		int result = SDL_RenderCopy(target, font, &src, &dst);
		dst.x += dst.w;
	}
}

SUI::SUI(AudioPlayer &player):
		player(player),
		window(nullptr, [](SDL_Window *w) { if (w) SDL_DestroyWindow(w); }),
		renderer(nullptr, [](SDL_Renderer *r){ if (r) SDL_DestroyRenderer(r); }),
		font(nullptr, [](SDL_Texture *t){ if (t) SDL_DestroyTexture(t); }),
		current_total_time(-1){
	this->window.reset(SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0));
	if (!this->window)
		throw UIInitializationException("Window creation failed.");
	this->renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC));
	if (!this->renderer)
		throw UIInitializationException("Renderer creation failed.");
	{
		auto font = SDL_LoadBMP("font3.bmp");
		if (!font)
			throw UIInitializationException("Couldn't load font.");
		this->font.reset(SDL_CreateTextureFromSurface(renderer.get(), font));
		if (!this->font){
			SDL_FreeSurface(font);
			throw UIInitializationException("Font texture creation failed.");
		}
	}
	SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
}

bool SUI::handle_in_events(){
	SDL_Event e;
	while (SDL_PollEvent(&e) ){
		switch (e.type){
			case SDL_QUIT:
				return 0;
			case SDL_KEYDOWN:
				{
					switch (e.key.keysym.sym){
						case SDLK_RIGHT:
							this->player.request_seek(5);
							break;
						case SDLK_LEFT:
							this->player.request_seek(-5);
							break;
					}
				}
				break;
		}
	}
	return 1;
}

std::string wide_to_narrow(const std::wstring &s){
	std::string ret;
	ret.reserve(s.size());
	for (auto wc : s)
		ret.push_back(wc < 128 ? wc : '?');
	return ret;
}

void SUI::handle_out_events(){
	for (boost::shared_ptr<InternalQueueElement> eqe; player.external_queue_out.try_pop(eqe);){
		auto ttu = dynamic_cast<TotalTimeUpdate *>(eqe.get());
		if (ttu){
			current_total_time = ttu->get_seconds();
			continue;
		}
		auto mdu = dynamic_cast<MetaDataUpdate *>(eqe.get());
		if (mdu){
			auto metadata = mdu->get_metadata();
			this->metadata.clear();
			metadata->iterate([this](const std::wstring &key, const std::wstring &value){
				this->metadata += wide_to_narrow(key);
				this->metadata += '=';
				if (key == L"METADATA_BLOCK_PICTURE")
					this->metadata += "<picture data>";
				else
					this->metadata += wide_to_narrow(value);

				this->metadata += '\n';
			});
		}
	}
}

void format_memory(std::ostream &stream, size_t size){
	double m = size;
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
	while (this->handle_in_events()){
		this->handle_out_events();
		std::stringstream stream;
		double now = player.get_current_time();
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
		draw_string(this->renderer.get(), this->font.get(), stream.str().c_str(), 0, 0);
		SDL_RenderPresent(this->renderer.get());
	}
}
