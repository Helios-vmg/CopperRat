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

#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include "SUI.h"
//#include "ListView.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <list>
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

	void prepare_buttons();
	void gui_signal(const GuiSignal &);
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
};

#endif
