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
		ret = command->execute(*current_player);
	return ret;
}

void AudioPlayer::request_hardplay(){
	this->push_to_command_queue(new AsyncCommandHardPlay);
}

void AudioPlayer::request_playpause(){
	this->push_to_command_queue(new AsyncCommandPlayPause);
}

void AudioPlayer::request_play(){
	this->push_to_command_queue(new AsyncCommandPlay);
}

void AudioPlayer::request_pause(){
	this->push_to_command_queue(new AsyncCommandPause);
}

void AudioPlayer::request_stop(){
	this->push_to_command_queue(new AsyncCommandStop);
}

void AudioPlayer::request_absolute_scaling_seek(double scale){
	this->push_to_command_queue(new AsyncCommandAbsoluteSeek(scale, true));
}

void AudioPlayer::request_relative_seek(double seconds){
	this->push_to_command_queue(new AsyncCommandRelativeSeek(seconds));
}

void AudioPlayer::request_previous(){
	this->push_to_command_queue(new AsyncCommandPrevious);
}

void AudioPlayer::request_next(){
	this->push_to_command_queue(new AsyncCommandNext);
}

void AudioPlayer::request_exit(){
	this->push_to_command_queue(new AsyncCommandExit);
}

void AudioPlayer::request_load(bool load, bool file, const std::wstring &path){
	this->push_to_command_queue(new AsyncCommandLoad(load, file, path));
}

double AudioPlayer::get_current_time(){
	return this->current_player.load()->get_current_time();
}

void AudioPlayer::notify_playback_end(){
	this->push_to_command_queue(new AsyncCommandPlaybackEnd);
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
