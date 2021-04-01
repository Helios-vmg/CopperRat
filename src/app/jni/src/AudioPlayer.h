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

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H
#include "Threads.h"
#include "AudioDevice.h"
#include "AudioStream.h"
#include "AudioBuffer.h"
#include "Playlist.h"
#include "UserInterface.h"
#include "AudioPlayerState.h"
#include "QueueElements.h"
#include "AsyncCommands.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <memory>
#include <map>
#endif

struct NotImplementedException{};

class AudioPlayer{
	friend class AudioDevice;
	friend class AudioPlayerState;
	typedef std::shared_ptr<ExternalQueueElement> eqe_t;
	typedef std::shared_ptr<AudioPlayerAsyncCommand> command_t;
	typedef thread_safe_queue<command_t> external_queue_in_t;
	typedef thread_safe_queue<eqe_t> external_queue_out_t;

	struct AudioLocker{
		bool restore;
		AudioPlayer &player;
		AudioLocker(AudioPlayer &player): player(player){
			this->restore = player.device.is_open();
			player.device.pause_audio();
		}
		~AudioLocker(){
			if (restore)
				player.device.start_audio();
		}
	};

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
	void push_to_command_queue(AudioPlayerAsyncCommand *p){
		std::shared_ptr<AudioPlayerAsyncCommand> sp(p);
		this->external_queue_in.push(sp);
	}
	void push_to_external_queue(ExternalQueueElement *p){
		std::shared_ptr<ExternalQueueElement> sp(p);
		this->external_queue_out.push(sp);
	}
	bool handle_requests();
public:
	external_queue_out_t external_queue_out;
	AudioPlayer(bool start_thread = true);
	~AudioPlayer();

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
	void terminate_thread(UserInterface &ui);

	//request_* functions run in the caller thread!
	void request_hardplay();
	void request_playpause();
	void request_play();
	void request_pause();
	void request_stop();
	void request_absolute_scaling_seek(double scale);
	void request_relative_seek(double seconds);
	void request_previous();
	void request_next();
	void request_exit();
	void request_load(bool load, bool file, const std::wstring &path);
	double get_current_time();

	//nodify_* functions are designed to be called from the audio output thread.
	void notify_playback_end();
	
	Playlist &get_playlist();
	audio_buffer_t get_last_buffer_played();
	PlayState::Value get_state() const;
};

#endif
