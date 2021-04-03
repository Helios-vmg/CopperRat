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
#include <optional>
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

void AudioPlayer::initialize(bool start_thread){
	{
		auto ul = application_state->lock();
		for (auto &kv : application_state->get_players())
			this->players.emplace(kv.first, std::make_unique<AudioPlayerState>(*this, kv.second));
	}

	this->current_player = this->players[application_state->get_current_player_index()].get();
	
	if (start_thread){
		this->running = true;
		this->sdl_thread = std::thread([this](){ this->thread(); });
	}
}

/*void AudioPlayer::terminate_thread(UserInterface &ui){
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
}*/

AudioPlayer::~AudioPlayer(){
#ifndef PROFILING
	if (this->sdl_thread.joinable())
		this->sdl_thread.join();
#endif
}

//#define OUTPUT_TO_FILE
#ifdef OUTPUT_TO_FILE
std::ofstream raw_file;
#endif

void AudioPlayer::thread_loop(){
	while (this->handle_requests()){
		auto now = SDL_GetTicks();
		if (this->device.update(now)){
			for (auto &kv : this->players)
				kv.second->save();
		}
		if (!this->current_player.load()->process(now))
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
			this->sui->push_async_callback([e = e.clone()](){
				CR_Exception cre = *e;
				delete e;
				throw cre;
			});
			fatal_error = true;
		}catch (CR_Exception &e){
			this->sui->push_async_callback([e = e.clone()](){
				CR_Exception cre = *e;
				delete e;
				throw cre;
			});
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
	/*
	this->sui->push_async_callback([](){
		//TODO
	});
	*/
}

bool AudioPlayer::handle_requests(){
	bool ret = true;
	for (command_t command; this->external_queue_in.try_pop(command) && ret;)
		ret = command();
	return ret;
}

void AudioPlayer::request_absolute_scaling_seek(double scale){
	this->external_queue_in.push([this, scale](){ return this->current_player.load()->execute_absolute_seek(scale, true); });
}

void AudioPlayer::request_relative_seek(double seconds){
	this->external_queue_in.push([this, seconds](){ return this->current_player.load()->execute_relative_seek(seconds); });
}

void AudioPlayer::request_load(bool load, bool file, std::wstring &&path){
	this->external_queue_in.push([this, load, file, path = std::move(path)](){ return this->current_player.load()->execute_load(load, file, path); });
}

void AudioPlayer::notify_playback_end(AudioPlayerState &state){
	this->external_queue_in.push([&state](){ return state.execute_playback_end(); });
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

AudioPlayerState &AudioPlayer::new_player(){
	Future<AudioPlayerState *> ret;
	this->external_queue_in.push([this, &ret](){
		auto &ps = application_state->new_player();
		auto &new_player = this->players.emplace(ps.get_playback().get_index(), std::make_unique<AudioPlayerState>(*this, ps)).first->second;
		ret = new_player.get();
		return true;
	});
	return **ret;
}

void AudioPlayer::switch_to_player(AudioPlayerState &state){
	this->external_queue_in.push([this, &state](){
		this->execute_switch_to_player(state);
		return true;
	});
}

void AudioPlayer::execute_switch_to_player(AudioPlayerState &state){
	auto index = state.player_state->get_playback().get_index();
	auto it = this->players.find(index);
	if (it == this->players.end()){
		assert(false);
		return;
	}
	auto old_active = this->current_player.load()->is_active();
	this->current_player = it->second.get();
	auto new_active = this->current_player.load()->is_active();
	if (!old_active && new_active)
		this->device.start_audio();
	else if (old_active && !new_active)
		this->device.pause_audio();
	application_state->set_current_player_index(index);
}

void AudioPlayer::erase(AudioPlayerState &state){
	this->external_queue_in.push([this, &state](){
		this->execute_erase(state);
		return true;
	});
}

void AudioPlayer::execute_erase(AudioPlayerState &state){
	auto index = state.player_state->get_playback().get_index();
	auto it = this->players.find(index);
	if (it == this->players.end()){
		assert(false);
		return;
	}
	state.erase();
	this->players.erase(it);
}
