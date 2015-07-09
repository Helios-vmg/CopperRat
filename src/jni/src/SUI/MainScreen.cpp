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
#include "MainScreen.h"
#include "../CommonFunctions.h"
#include "Button.h"
#include "../File.h"
#include "ListView.h"
#include "SeekBar.h"

enum class ButtonSignal{
	PLAY = 0,
	PAUSE,
	STOP,
	LOAD,
	PREVIOUS,
	SEEKBACK,
	SEEKFORTH,
	NEXT,
};

MainScreen::MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player):
		GUIElement(sui, parent),
		player(player){
	this->prepare_buttons();
	this->children.push_back(boost::shared_ptr<GUIElement>(new SeekBar(sui, this)));
}

unsigned MainScreen::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	if (event.type == SDL_KEYDOWN && (event.key.keysym.scancode == SDL_SCANCODE_MENU || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)){
		GuiSignal signal;
		signal.type = SignalType::MAINSCREEN_MENU;
		this->parent->gui_signal(signal);
	}
	return ret | GUIElement::handle_event(event);
}

void MainScreen::update(){
	Uint32 time = SDL_GetTicks();
	this->sui->draw_picture();
	GUIElement::update();
	//this->draw_oscilloscope(time);
	this->draw_spectrum(time);
	this->last_draw = time;
}

void MainScreen::draw_oscilloscope(Uint32 time){
	auto player_state = this->player.get_state();
	if (player_state == PlayState::STOPPED){
		this->last_buffer.unref();
		return;
	}
	auto buffer = this->player.get_last_buffer_played();
	auto visible_region = this->sui->get_visible_region();
	memory_sample_count_t length = this->last_buffer.samples();
	length = std::min(length, (memory_sample_count_t)visible_region.w);
	if (buffer)
		this->last_buffer = buffer;
	else{
		if (player_state != PlayState::PAUSED && this->last_draw){
			//this->last_buffer.advance_data_offset((time - this->last_draw) * 44100 / 1000);
			this->last_buffer.advance_data_offset(length);
			length = this->last_buffer.samples();
			length = std::min(length, (memory_sample_count_t)visible_region.w);
		}
	}
	if (!this->last_buffer)
		return;
	if (this->last_buffer.bytes_per_sample() != 2 * this->last_buffer.channels() || !this->last_buffer.samples())
		return;
	float last = 0;
	const float middle = (float)visible_region.w / 2;
	auto channels = this->last_buffer.channels();
	for (memory_audio_position_t i = 0; i != length; i++){
		float value = 0;
		auto sample = this->last_buffer.get_sample_use_channels<Sint16>(i);
		for (unsigned j = 0; j < channels; j++)
			value += s16_to_float(sample->values[j]);
		value /= channels;
		auto y = value * middle + middle;
		if (i)
			GPU_Line(this->sui->get_target(), (float)i - 1, last, (float)i, y, { 255, 255, 255, 255 });
		last = y;
	}
}

const double pi = 3.1415926535897932384626433832795;

void compute_dct(float *dst, float *time_domain, size_t n){
	for (size_t i = n; i--;){
		double sum = 0;
		for (size_t j = n; j--;)
			sum += time_domain[j] * cos(pi / (double)n * (j + 0.5) * i);
		dst[i] = sum < 0 ? -sum : sum;
	}
}

class dct_calculator{
	std::vector<float> dct_matrix;
public:
	std::vector<float> result_store;
	dct_calculator(unsigned short size) :
			dct_matrix(size * size),
			result_store(size){
		auto n = this->result_store.size();
		auto matrix = &this->dct_matrix[0];
		for (auto i = this->dct_matrix.size(); i--;){
			auto x = i % n;
			auto y = i / n;
			matrix[x + y * n] = cos(pi / (float)n * ((float)x + 0.5f) * (float)y);
		}
	}
	void compute_native_size_dct(float *time_domain){
		auto n = this->result_store.size();
		auto matrix = &this->dct_matrix[0];
		auto res = &this->result_store[0];
		for (size_t i = n; i--;){
			float sum = 0;
			auto base = matrix + i * n;
			for (size_t j = n; j--;)
				sum += time_domain[j] * base[j];
			res[i] = sum < 0 ? -sum : sum;
		}
	}
	void compute_arbitrary_size_dct(float *time_domain, size_t size){
		if (size == this->result_store.size()){
			this->compute_native_size_dct(time_domain);
			return;
		}
		size_t n = this->result_store.size();
		auto matrix = &this->dct_matrix[0];
		auto res = &this->result_store[0];
		float multiplier = 1; // 1.f / (float)((size + n - 1) / n);
		for (size_t i = 0; i < size; i += n){
			auto end = std::min(i + n, size);
			float conditional_multiplier = !i ? 0.f : 1.f;
			for (size_t j = n; j--;){
				float sum = res[j] * conditional_multiplier;
				auto base = matrix + j * n - i;
				for (size_t k = i; k != end; k++)
					sum += time_domain[k] * base[k];
				res[j] = sum * multiplier;
			}
		}
		for (auto &f : this->result_store)
			f = f < 0 ? -f : f;
	}
};

static dct_calculator dct(540);

void MainScreen::draw_spectrum(Uint32 time){
	auto player_state = this->player.get_state();
	auto framerate = this->sui->get_current_framerate();
	if (player_state == PlayState::STOPPED || framerate <= 0){
		this->last_buffer.unref();
		return;
	}
	auto buffer = this->player.get_last_buffer_played();
	auto visible_region = this->sui->get_visible_region();
	memory_sample_count_t length = this->last_buffer.samples();
	length = std::min(length, (memory_sample_count_t)visible_region.w);
	if (buffer){
		this->last_buffer = buffer;
		// (samples/buffer) / (samples/sec) * (frames/sec) = 
		// (1/buffer) / (1/sec) * (frames/sec) = 
		// (sec/buffer) * (frames/sec) = 
		// (1/buffer) * frames = 
		// frames/buffer 
		float frames_per_buffer = (float)this->last_buffer.samples() / 44100.f * framerate;
		this->last_length = (unsigned)(this->last_buffer.samples() / frames_per_buffer);
	}else{
		if (player_state != PlayState::PAUSED && this->last_draw){
			this->last_buffer.advance_data_offset(this->last_length);
		}
	}
	length = std::min(this->last_length, this->last_buffer.samples());
	if (!this->last_buffer || !length){
		//std::cerr << "Out of buffer!\n";
		return;
	}
	if (this->last_buffer.bytes_per_sample() != 2 * this->last_buffer.channels())
		return;
	auto channels = this->last_buffer.channels();
	std::vector<float> time_domain(length);
	float *aux = &time_domain[0];
	for (memory_audio_position_t i = length; i--;){
		float value = 0;
		auto sample = this->last_buffer.get_sample_use_channels<Sint16>(i);
		for (unsigned j = 0; j < channels; j++)
			value += s16_to_float(sample->values[j]);
		value /= channels;
		aux[i] = value;
	}
	dct.compute_arbitrary_size_dct(&time_domain[0], time_domain.size());
	const auto &frequency_domain = dct.result_store;
	float mul = (float)visible_region.w / (float)frequency_domain.size();
	float x0 = 0;
	for (auto val : frequency_domain){
		float value = val * (1.f / 10.f);
		float y0 = -value * visible_region.w + visible_region.w;
		float x1 = x0 + mul;
		float y1 = 0 * visible_region.w + visible_region.w;
		GPU_RectangleFilled(this->sui->get_target(), x0, y0, x1, y1, { 255, 255, 255, 255 });
		x0 = x1;
	}
}

void MainScreen::gui_signal(const GuiSignal &signal){
	if (signal.type != SignalType::BUTTON_SIGNAL)
		return;
	switch ((ButtonSignal)signal.data.button_signal){
		case ButtonSignal::PLAY:
			this->player.request_hardplay();
			break;
		case ButtonSignal::PAUSE:
			this->player.request_pause();
			break;
		case ButtonSignal::STOP:
			this->player.request_stop();
			break;
		case ButtonSignal::LOAD:
			{
				GuiSignal signal;
				signal.type = SignalType::MAINSCREEN_LOAD;
				this->parent->gui_signal(signal);
			}
			break;
		case ButtonSignal::PREVIOUS:
			this->player.request_previous();
			break;
		case ButtonSignal::SEEKBACK:
			this->player.request_relative_seek(-5);
			break;
		case ButtonSignal::SEEKFORTH:
			this->player.request_relative_seek(5);
			break;
		case ButtonSignal::NEXT:
			this->player.request_next();
			break;
	}
}

void MainScreen::prepare_buttons(){
	static const char *button_paths[] = {
		BASE_PATH "button_play.png",
		BASE_PATH "button_pause.png",
		BASE_PATH "button_stop.png",
		BASE_PATH "button_load.png",
		BASE_PATH "button_previous.png",
		BASE_PATH "button_seekback.png",
		BASE_PATH "button_seekforth.png",
		BASE_PATH "button_next.png",
	};
	const size_t n = sizeof(button_paths) / sizeof(*button_paths);
	surface_t button_surfaces[n];
	int i = 0;
	const int square = this->sui->get_bounding_square();
	for (auto p : button_paths){
		auto image = load_image_from_file(p);
		if (!image)
			throw UIInitializationException("Failed to load a button.");
		image = bind_surface_to_square(image, square / 4);
		if (!image)
			throw UIInitializationException("Failed to transform a button.");
		button_surfaces[i++] = image;
	}
	int w = button_surfaces[0]->w;
	int h = button_surfaces[0]->h;
	auto new_surface = create_rgba_surface(w * 4, h * 2);
	i = 0;
	SDL_Rect rects[n];
	for (auto &surface : button_surfaces){
		SDL_Rect dst = { w * (i % 4), h * (i / 4), w, h, };
		rects[i] = dst;
		SDL_BlitSurface(button_surfaces[i].get(), 0, new_surface.get(), &dst);
		i++;
	}
	this->tex_buttons.set_target(this->sui->get_target());
	this->tex_buttons.load(new_surface);
	this->tex_buttons.set_alpha(0.5);
	for (i = 0; i < n; i++){
		Subtexture st(this->tex_buttons, rects[i]);
		boost::shared_ptr<GraphicButton> button(new GraphicButton(this->sui, this));
		button->set_graphic(st);
		button->set_position(rects[i].x, rects[i].y + square);
		button->set_signal_value(i);
		this->children.push_back(button);
	}
}
