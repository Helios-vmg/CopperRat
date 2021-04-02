/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "Threads.h"
#include "AudioDevice.h"
#include "AudioStream.h"
#include "AudioBuffer.h"
#include "Playlist.h"
#include "UserInterface.h"
#include "AudioPlayerState.h"
#include "QueueElements.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <memory>
#include <map>
#include <functional>
#endif

class SUI;

struct NotImplementedException{};

class AudioPlayer{
	friend class AudioDevice;
	friend class AudioPlayerState;
	typedef std::shared_ptr<ExternalQueueElement> eqe_t;
	typedef std::function<bool()> command_t;
	typedef thread_safe_queue<command_t> external_queue_in_t;

	bool running = false;
	AudioDevice device;
	external_queue_in_t external_queue_in;
	SDL_Thread *sdl_thread = nullptr;
	static void AudioCallback(void *udata, Uint8 *stream, int len);
	static int _thread(void *);
	std::map<int, AudioPlayerState> players;
	std::atomic<AudioPlayerState *> current_player = nullptr;

	void thread();
	void thread_loop();
	bool handle_requests();
public:
	SUI *sui = nullptr;
	
	AudioPlayer(): device(*this){}
	~AudioPlayer();

	void initialize(bool start_thread);

	void loop(){
		if (this->running)
			return;
		try{
			this->running = true;
			this->thread();
			this->running = false;
		}catch (...){
			this->running = false;
			throw;
		}
	}
	//void terminate_thread(UserInterface &ui);

	#define TRIVIAL_ASYNC_COMMAND(name) void request_##name(){ this->external_queue_in.push([this](){ return this->current_player.load()->execute_##name(); }); }
	//request_* functions run in the caller thread!
	TRIVIAL_ASYNC_COMMAND(hardplay)
	TRIVIAL_ASYNC_COMMAND(playpause)
	TRIVIAL_ASYNC_COMMAND(play)
	TRIVIAL_ASYNC_COMMAND(pause)
	TRIVIAL_ASYNC_COMMAND(stop)
	TRIVIAL_ASYNC_COMMAND(previous)
	TRIVIAL_ASYNC_COMMAND(next)
	TRIVIAL_ASYNC_COMMAND(exit)
	void request_absolute_scaling_seek(double scale);
	void request_relative_seek(double seconds);
	void request_load(bool load, bool file, std::wstring &&path);
	auto &get_players(){
		return this->players;
	}
	auto &get_players() const{
		return this->players;
	}
	AudioPlayerState &get_current_player(){
		return *this->current_player.load();
	}
	AudioPlayerState &new_player();
	void switch_to_player(AudioPlayerState &);
	void erase(AudioPlayerState &);

	//nodify_* functions are designed to be called from the audio output thread.
	void notify_playback_end(AudioPlayerState &);
	
	Playlist &get_playlist();
	audio_buffer_t get_last_buffer_played();
	PlayState::Value get_state() const;
};
