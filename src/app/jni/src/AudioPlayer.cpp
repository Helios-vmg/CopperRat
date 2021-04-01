/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioPlayer.h"
#include "CommonFunctions.h"
#include "ApplicationState.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <fstream>
#ifdef PROFILING
#if defined WIN32
#include <fstream>
#else
#include <sstream>
#include <android/log.h>
#endif
#endif
#endif

const char *PlayState::strings[] = {
	"STOPPED",
	"PLAYING",
	"ENDING",
	"PAUSED",
};

void AudioPlayer::AudioCallback(void *udata, Uint8 *stream, int len){
	auto player = ((AudioPlayer *)udata)->current_player.load();
	if (!player){
		memset(stream, 0, len);
		return;
	}
	player->audio_callback(stream, len);
}

AudioPlayer::AudioPlayer(bool start_thread): device(*this){
	{
		auto ul = application_state.lock();
		for (auto &kv : application_state.get_players())
			this->players[kv.first] = {*this, kv.second};
	}

	this->current_player = &this->players[application_state.get_current_player_index()];
	
	if (start_thread){
		this->running = true;
		this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
	}
}

void AudioPlayer::terminate_thread(UserInterface &ui){
#ifndef PROFILING
	this->request_exit();
	ExitAcknowledged *target = nullptr;
	while (1){
		auto el = this->external_queue_out.pop();
		target = dynamic_cast<ExitAcknowledged *>(&*el);
		if (!target)
			el->receive(ui);
		else
			break;
	}
#endif
}

AudioPlayer::~AudioPlayer(){
#ifndef PROFILING
	if (this->sdl_thread)
		SDL_WaitThread(this->sdl_thread, nullptr);
#endif
}

int AudioPlayer::_thread(void *p){
	((AudioPlayer *)p)->thread();
	return 0;
}

//#define OUTPUT_TO_FILE
#ifdef OUTPUT_TO_FILE
std::ofstream raw_file;
#endif

void AudioPlayer::thread_loop(){
	while (true){
		bool continue_loop = this->handle_requests();
		if (!continue_loop)
			break;
		bool nothing_was_done = true;
		for (auto &kv : this->players)
			nothing_was_done &= !kv.second.process();
		if (nothing_was_done)
			SDL_Delay(10);
	}
}

void AudioPlayer::thread(){
#ifdef PROFILING
	unsigned long long samples_decoded = 0;
	Uint32 t0 = SDL_GetTicks();
#endif
#ifdef OUTPUT_TO_FILE
	raw_file.open("output.raw", std::ios::binary|std::ios::trunc);
#endif
	bool fatal_error = 0;
	while (!fatal_error){
		try{
			this->thread_loop();
			break;
		}catch (const std::bad_alloc &){
			abort();
		}catch (const DeviceInitializationException &e){
			auto temp = e.clone();
			this->push_to_external_queue(new ExceptionTransport(*temp));
			delete temp;
			fatal_error = 1;
		}catch (CR_Exception &e){
			auto temp = e.clone();
			this->push_to_external_queue(new ExceptionTransport(*temp));
			delete temp;
		}
	}
#ifdef PROFILING
	Uint32 t1 = SDL_GetTicks();
	{
		double times = (samples_decoded / ((t1 - t0) / 1000.0)) / 44100.0;
#ifdef WIN32
		std::cout <<(t1 - t0)<<" ms\n";
		std::cout <<times<<"x\n";
#else
		std::ofstream file("/sdcard/external_sd/log.txt", std::ios::app);
		file <<times<<"x\n";
		__android_log_print(ANDROID_LOG_INFO, "PERFORMANCE", "%fx", times);
#endif
	}
#endif

	this->device.close();
	this->external_queue_out.push(eqe_t(new ExitAcknowledged));
}

bool AudioPlayer::handle_requests(){
	auto current_player = this->current_player.load();
	bool ret = true;
	for (command_t command; this->external_queue_in.try_pop(command) && ret;)
		ret = command(*current_player);
	return ret;
}

void AudioPlayer::request_absolute_scaling_seek(double scale){
	this->external_queue_in.push([scale](AudioPlayerState &state){ return state.execute_absolute_seek(scale, true); });
}

void AudioPlayer::request_relative_seek(double seconds){
	this->external_queue_in.push([seconds](AudioPlayerState &state){ return state.execute_relative_seek(seconds); });
}

void AudioPlayer::request_load(bool load, bool file, std::wstring &&path){
	this->external_queue_in.push([load, file, path = std::move(path)](AudioPlayerState &state){ return state.execute_load(load, file, path); });
}

void AudioPlayer::notify_playback_end(){
	this->external_queue_in.push([](AudioPlayerState &state){ return state.execute_playback_end(); });
}

double AudioPlayer::get_current_time(){
	return this->current_player.load()->get_current_time();
}

Playlist &AudioPlayer::get_playlist(){
	return this->current_player.load()->playlist;
}

audio_buffer_t AudioPlayer::get_last_buffer_played(){
	return this->current_player.load()->get_last_buffer_played();
}

PlayState::Value AudioPlayer::get_state() const{
	return this->current_player.load()->state;
}
