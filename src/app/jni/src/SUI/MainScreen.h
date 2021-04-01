/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "SUI.h"
//#include "ListView.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <list>
#include <functional>
#endif

class SeekBar;

class dct_calculator{
	std::vector<float> dct_matrix;
	std::vector<float> extra_multipliers;
public:
	std::vector<float> result_store;
	dct_calculator(unsigned short size);
	void compute_native_size_dct(float *time_domain);
	void compute_arbitrary_size_dct(float *time_domain, size_t size);
};

enum class SpectrumQuality{
	Low,
	Mid,
	High,
	Max,
};

class MainScreen : public GUIElement{
	double current_total_time;
	AudioPlayer &player;
	Texture tex_buttons;
	audio_buffer_t last_buffer;
	unsigned last_length;
	std::shared_ptr<dct_calculator> dct;
	std::vector<float> spectrogram_data;
	std::vector<float> spectrum_state;
	unsigned spectrogram_data_w,
		spectrogram_data_h;
	size_t spectrogram_data_head;
	std::function<void()> on_load_request;
	std::function<void()> on_menu_request;

	void prepare_buttons();
	void on_button(int);
	void draw_oscilloscope(Uint32 time);
	void draw_spectrum(Uint32 time, SpectrumQuality, bool spectrogram);
	Uint32 last_draw;
public:
	MainScreen(SUI *sui, GUIElement *parent, AudioPlayer &player);
	unsigned handle_event(const SDL_Event &);
	void update();
	SDL_Rect get_seekbar_region() const{
		return this->sui->get_seekbar_region();
	}
	void set_on_load_request(std::function<void()> &&f){
		this->on_load_request = std::move(f);
	}
	void set_on_menu_request(std::function<void()> &&f){
		this->on_menu_request = std::move(f);
	}
};

#endif
