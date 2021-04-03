/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "../stdafx.h"
#include "MainScreen.h"
#include "../CommonFunctions.h"
#include "Button.h"
#include "../File.h"
#include "ListView.h"
#include "SeekBar.h"
#include "../AudioPlayer.h"

enum class ButtonSignal{
	PLAY = 0,
	PAUSE,
	STOP,
	LOAD,
	PREVIOUS,
	SEEKBACK,
	SEEKFORTH,
	NEXT,
	PreviousPlayer,
	Options,
	NextPlayer,
};

MainScreen::MainScreen(SUI *sui, GUIElement *parent, AudioPlayerState &player):
		GUIElement(sui, parent),
		player(&player),
		spectrogram_data_w(0),
		spectrogram_data_h(0),
		spectrogram_data_head(0){
	this->prepare_buttons();
	this->children.push_back(std::shared_ptr<GUIElement>(new SeekBar(sui, this)));
	
	this->tex_picture.set_target(this->sui->get_target());
	this->background_picture.set_target(this->sui->get_target());
}

unsigned MainScreen::handle_event(const SDL_Event &event){
	unsigned ret = SUI::NOTHING;
	if (event.type == SDL_KEYDOWN && (event.key.keysym.scancode == SDL_SCANCODE_MENU || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)){
		if (this->on_menu_request)
			this->on_menu_request();
	}
	return ret | GUIElement::handle_event(event);
}

void MainScreen::update(){
	Uint32 time = SDL_GetTicks();
	this->draw_picture();
	GUIElement::update();
	switch (this->sui->get_visualization_mode()){
		case VisualizationMode::NONE:
			break;
		case VisualizationMode::OSCILLOSCOPE:
			this->draw_oscilloscope(time);
			break;
		case VisualizationMode::SPECTRUM_LOW:
			this->draw_spectrum(time, SpectrumQuality::Low, false);
			break;
		case VisualizationMode::SPECTRUM_MID:
			this->draw_spectrum(time, SpectrumQuality::Mid, false);
			break;
		case VisualizationMode::SPECTRUM_HIGH:
			this->draw_spectrum(time, SpectrumQuality::High, false);
			break;
		case VisualizationMode::SPECTRUM_MAX:
			this->draw_spectrum(time, SpectrumQuality::Max, false);
			break;
		default:
			assert(0);
	}
	this->last_draw = time;
}

void MainScreen::draw_oscilloscope(Uint32 time){
	auto player_state = this->player->get_state();
	if (player_state == PlayState::STOPPED){
		this->last_buffer.unref();
		return;
	}
	auto buffer = this->player->get_last_buffer_played();
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

	static std::pair<float, SDL_Color> times[] = {
		{ 3, { 0, 0, 0, 255 } },
		{ 1, { 255, 255, 255, 255 } },
	};
	for (auto &pair : times){
		GPU_SetLineThickness(pair.first);
		for (memory_audio_position_t i = 0; i != length; i++){
			float value = 0;
			auto sample = this->last_buffer.get_sample_use_channels<Sint16>(i);
			for (unsigned j = 0; j < channels; j++)
				value += s16_to_float(sample->values[j]);
			value /= channels;
			auto y = value * middle + middle;
			if (i)
				GPU_Line(this->sui->get_target(), (float)i - 1, last, (float)i, y, pair.second);
			last = y;
		}
	}
}

std::pair<float, float> loudness_curve_data[] = {
	{ 20, -42 },
	{ 180, -10 },
	{ 300, -5.5f },
	{ 600, -5.5f },
	{ 900, -9.5f },
	{ 1000, -10 },
	{ 2000, -7 },
	{ 3000, -1.5f },
	{ 3600, 0 },
	{ 5000, -4.5f },
	{ 8000, -15.5f },
	{ 10000, -15.f },
	{ 20000, -54 },
};

float equal_loudness_curve_db(float hz){
	if (hz < 20 || hz > 20000)
		return 1;
	if (hz == 20)
		return loudness_curve_data[0].second;
	int i = 0;
	for (auto &pair : loudness_curve_data){
		if (pair.first >= hz)
			break;
		i++;
	}
	auto left = loudness_curve_data[i - 1];
	auto right = loudness_curve_data[i];
	float lambda = (hz - left.first) / (right.first - left.first);
	return left.second * (1 - lambda) + right.second * lambda;
}


float equal_loudness_curve(float hz){
	auto db = equal_loudness_curve_db(hz);
	if (db > 0)
		return 0;
	return powf(10, db * 0.05f);
}

const float pi = 3.1415926535897932384626433832795f;

float exponential_function(float x, float n){
	return 1 - powf(n, -x);
}

float exponential_function2(float x, float n){
	return exponential_function(x, n) / exponential_function(1, n);
}

dct_calculator::dct_calculator(unsigned short size) :
		dct_matrix(size * size),
		result_store(size),
		extra_multipliers(size){
	auto n = this->result_store.size();
	auto matrix = &this->dct_matrix[0];
	for (auto i = this->dct_matrix.size(); i--;){
		auto x = i % n;
		auto y = i / n;
		matrix[x + y * n] = cosf(pi / (float)n * ((float)x + 0.5f) * (float)y);
	}
	float m = (float)size * (-3.f / 4050.f) + 0.5f;
	float freq_fractional = 22050.f / (float)size;
	float freq = 0;
	for (auto &f : this->extra_multipliers){
		auto middle = freq + freq_fractional * 0.5f;
		//f = m * equal_loudness_curve(middle);
		//f = m * 2 * (middle / 22050);
		//f = m * exponential_function2(middle / 22050.f, 1e+1);
		f = m * 2.5 * (middle /22050);
		freq += freq_fractional;
	}
}

void dct_calculator::compute_native_size_dct(float *time_domain){
	auto n = this->result_store.size();
	auto matrix = &this->dct_matrix[0];
	auto multipliers = &this->extra_multipliers[0];
	auto res = &this->result_store[0];
	for (size_t i = n; i--;){
		float sum = 0;
		auto base = matrix + i * n;
		for (size_t j = n; j--;)
			sum += time_domain[j] * base[j];
		sum *= multipliers[i];
		res[i] = sum < 0 ? -sum : sum;
	}
}

void dct_calculator::compute_arbitrary_size_dct(float *time_domain, size_t size){
	if (size == this->result_store.size()){
		this->compute_native_size_dct(time_domain);
		return;
	}
	size_t n = this->result_store.size();
	auto matrix = &this->dct_matrix[0];
	auto res = &this->result_store[0];
	auto multipliers = &this->extra_multipliers[0];
	for (size_t i = 0; i < size; i += n){
		auto end = std::min(i + n, size);
		float conditional_multiplier = !i ? 0.f : 1.f;
		for (size_t j = n; j--;){
			float sum = res[j] * conditional_multiplier;
			auto base = matrix + j * n - i;
			for (size_t k = i; k != end; k++)
				sum += time_domain[k] * base[k];
			res[j] = sum;
		}
	}
	size_t i = 0;
	for (auto &f : this->result_store)
		f = (f < 0 ? -f : f) * multipliers[i++];
}

void MainScreen::draw_spectrum(Uint32 time, SpectrumQuality quality, bool spectrogram){
	auto player_state = this->player->get_state();
	auto framerate = this->sui->get_current_framerate();
	if (player_state == PlayState::STOPPED || framerate <= 0){
		this->last_buffer.unref();
		return;
	}
	auto buffer = this->player->get_last_buffer_played();
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
	if (!(!this->last_buffer || !length)){
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
		int bands;
		switch (quality){
			case SpectrumQuality::Low:
				bands = 16;
				break;
			case SpectrumQuality::Mid:
				bands = 64;
				break;
			case SpectrumQuality::High:
				bands = 128;
				break;
			case SpectrumQuality::Max:
				bands = visible_region.w;
				break;
			default:
				assert(0);
		}
		bands = std::min(visible_region.w, bands);
		if (!dct || bands != dct->result_store.size()){
			dct.reset(new dct_calculator(bands));
		}
		dct->compute_arbitrary_size_dct(&time_domain[0], time_domain.size());
	}
	if (!dct)
		return;
	const auto &frequency_domain = dct->result_store;
	if (this->spectrum_state.size() != frequency_domain.size()){
		this->spectrum_state = frequency_domain;
	}else{
		for (size_t i = 0; i < this->spectrum_state.size(); i++){
			auto &a = this->spectrum_state[i];
			auto &b = frequency_domain[i];
			const float delta = 0.075f;
			if (a <= b)
				a = b;
			else if (a >= delta)
				a -= delta;
			else
				a = 0;
		}
	}
	float mul = (float)visible_region.w / (float)frequency_domain.size();
	auto target = this->sui->get_target();
	if (!spectrogram){
		float x0 = 0;
		for (auto val : this->spectrum_state){
			//float value = (log10(val) + 5) / 7;
			float value = val;
			if (value < 0)
				value = 0;
			//else if (value > 1)
			//	value = 1;
			float y0 = -value * visible_region.w + visible_region.w;
			float x1 = x0 + mul;
			float y1 = (float)visible_region.w;
			GPU_RectangleFilled(target, x0, y0, x1, y1, { 255, 255, 255, 255 });
			x0 = x1;
		}
	} else{
		if (!this->spectrogram_data.size() || frequency_domain.size() != this->spectrogram_data_h || visible_region.w != this->spectrogram_data_w){
			this->spectrogram_data_h = (unsigned)frequency_domain.size();
			this->spectrogram_data_w = visible_region.w;
			this->spectrogram_data.resize(this->spectrogram_data_w * this->spectrogram_data_h);
			memset(&this->spectrogram_data[0], 0, this->spectrogram_data.size() * sizeof(float));
		}
		auto n = this->spectrogram_data.size();
		auto pos = this->spectrogram_data_head + n - this->spectrogram_data_h;
		pos %= n;
		memcpy(&this->spectrogram_data[pos], &frequency_domain[0], this->spectrogram_data_h * sizeof(float));

		auto data = &this->spectrogram_data[0];
		auto head = this->spectrogram_data_head / this->spectrogram_data_h;
		float y00 = (float)visible_region.w - 1;
		for (unsigned x0 = 0; x0 < this->spectrogram_data_w; x0++){
			unsigned x = x0 + head;
			x %= this->spectrogram_data_w;
			float y0 = y00;
			auto base = data + x * this->spectrogram_data_h;
			for (unsigned y = 0; y < this->spectrogram_data_h; y++){
				float value = base[y];
				if (value > 1)
					value = 1;
				float y1 = y0 - mul;
				auto color = (Uint8)(value * 255);
				GPU_Pixel(target, (float)x0, y0, { color, color, color, 255 });
				y0 = y1;
			}
		}
		this->spectrogram_data_head += this->spectrogram_data_h;
	}
}

void MainScreen::on_button(int signal){
	auto &player = this->player->get_player();
	switch ((ButtonSignal)signal){
		case ButtonSignal::PLAY:
			player.request_hardplay();
			break;
		case ButtonSignal::PAUSE:
			player.request_pause();
			break;
		case ButtonSignal::STOP:
			player.request_stop();
			break;
		case ButtonSignal::LOAD:
			if (this->on_load_request)
				this->on_load_request();
			break;
		case ButtonSignal::PREVIOUS:
			player.request_previous();
			break;
		case ButtonSignal::SEEKBACK:
			player.request_relative_seek(-5);
			break;
		case ButtonSignal::SEEKFORTH:
			player.request_relative_seek(5);
			break;
		case ButtonSignal::NEXT:
			player.request_next();
			break;
		case ButtonSignal::PreviousPlayer:
			this->sui->switch_to_previous_player();
			break;
		case ButtonSignal::Options:
			if (this->on_menu_request)
				this->on_menu_request();
			break;
		case ButtonSignal::NextPlayer:
			this->sui->switch_to_next_player();
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
		SDL_SetSurfaceBlendMode(button_surfaces[i].get(), SDL_BLENDMODE_NONE);
		SDL_BlitSurface(button_surfaces[i].get(), 0, new_surface.get(), &dst);
		i++;
	}
	this->tex_buttons.set_target(this->sui->get_target());
	this->tex_buttons.load(new_surface);
	this->tex_buttons.set_alpha(0.5);
	for (i = 0; i < n; i++){
		Subtexture st(this->tex_buttons, rects[i]);
		auto button = std::make_shared<GraphicButton>(this->sui, this);
		button->set_graphic(st);
		button->set_position(rects[i].x, rects[i].y + square);
		button->set_on_click([this, i](){ this->on_button(i); });
		this->children.push_back(button);
	}

	{
		auto vr = this->sui->get_visible_region();
		auto bs = this->sui->get_bounding_square();
		const int divider = 10;
		auto k = vr.w / divider;
		SDL_Rect rect = { vr.x, vr.y, k, bs };
		
		auto button = std::make_shared<Button>(this->sui, this);
		button->set_bounding_box(rect);
		button->set_on_click([this, n](){ this->on_button(n); });
		this->children.emplace_back(std::move(button));

		rect.x += k;
		rect.w = k * (divider - 2);
		
		button = std::make_shared<Button>(this->sui, this);
		button->set_bounding_box(rect);
		button->set_on_click([this, n](){ this->on_button(n + 1); });
		this->children.emplace_back(std::move(button));

		rect.x += rect.w;
		rect.w = vr.w - rect.x;
		
		button = std::make_shared<Button>(this->sui, this);
		button->set_bounding_box(rect);
		button->set_on_click([this, n](){ this->on_button(n + 2); });
		this->children.emplace_back(std::move(button));
	}
}

void MainScreen::on_total_time_update(double t){
	this->current_total_time = t;
	this->sui->request_update();
}

void MainScreen::on_playback_stop(){
	this->tex_picture_source.clear();
	this->tex_picture.unload();
	this->background_picture.unload();
	this->metadata.clear();
	this->current_total_time = -1;
	this->sui->request_update();
}

void MainScreen::on_metadata_update(const std::shared_ptr<GenericMetadata> &metadata){
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

	this->sui->push_parallel_work([this, metadata, source = this->tex_picture_source](){
		this->load_image(*metadata, source);
	});
}

void MainScreen::draw_picture(){
	int sq = this->sui->get_bounding_square();
	if (this->background_picture){
		SDL_Rect rect = { 0, 0, 0, 0 };
		this->background_picture.draw(rect);
	}
	if (this->tex_picture){
		auto rect = this->tex_picture.get_rect();
		SDL_Rect dst = { int((sq - rect.w) / 2), int((sq - rect.h) / 2), 0, 0 };
		this->tex_picture.draw(dst);
	}
}

static std::wstring get_blurred_image_path_from_hash(const std::string &hash){
	return utf8_to_string(std::string(BASE_PATH) + hash + "-blur.webp");
}

//static std::wstring get_blurred_image_path(const std::wstring &path){
//	return get_blurred_image_path_from_hash(get_hash(path));
//}

unsigned MainScreen::finish_picture_load(surface_t picture, const std::wstring &source, const std::string &hash, bool skip_loading){
	if (skip_loading){
		__android_log_print(ANDROID_LOG_INFO, "C++AlbumArt", "%s", "Album art load optimized away.\n");
		return SUI::NOTHING;
	}

	if (!picture){
		this->tex_picture_source.clear();
		this->tex_picture.unload();
		this->background_picture.unload();
		return SUI::NOTHING;
	}

	this->tex_picture_source = source;
	this->tex_picture.load(picture);
	{
		//Try to load cached blurred background image.
		auto bg_path = get_blurred_image_path_from_hash(hash);
		bool loaded = false;
		if (file_exists(bg_path)){
			auto surface = load_image_from_file(bg_path);
			if (surface){
				loaded = true;
				this->background_picture.load(surface);
			}
		}
		if (!loaded){
			this->blur_image(picture, bg_path);
			this->background_picture = this->sui->blur_image(this->tex_picture);
		}
	}

	return SUI::REDRAW;
}

unsigned MainScreen::finish_background_load(surface_t picture){
	this->background_picture.load(picture);
	return SUI::REDRAW;
}
