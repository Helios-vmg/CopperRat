/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "../AudioBuffer.h"
#include "SUI.h"
//#include "ListView.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <list>
#include <functional>
#endif

class SeekBar;
class GenericMetadata;

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
	Uint32 last_draw;
	std::wstring metadata;
	//If the currently loaded picture came from a file, this string contains
	//the path. If no picture is currently loaded, or if the one that is loaded
	//came from somewhere other than a file, this string is empty.
	std::wstring tex_picture_source;
	Texture tex_picture;
	Texture background_picture;
	std::shared_ptr<WorkerThreadJobHandle> picture_job;
	
	void prepare_buttons();
	void on_button(int);
	void draw_oscilloscope(Uint32 time);
	void draw_spectrum(Uint32 time, SpectrumQuality, bool spectrogram);
	void draw_picture();
	void load_image(GenericMetadata &metadata, const std::wstring &original_source);
	void blur_image(surface_t, const std::wstring &);
public:
	AudioPlayerState *player;
	
	MainScreen(SUI *sui, GUIElement *parent, AudioPlayerState &player);
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
	AudioPlayerState &get_player() const{
		return *this->player;
	}
	double get_current_total_time() const{
		return this->current_total_time;
	}
	const std::wstring &get_metadata() const{
		return this->metadata;
	}
	void on_total_time_update(double t);
	void on_playback_stop();
	void on_metadata_update(const std::shared_ptr<GenericMetadata> &metadata);
	unsigned finish_picture_load(surface_t picture, const std::wstring &source, const std::string &hash, bool skip_loading);
	unsigned finish_background_load(surface_t picture);

};
