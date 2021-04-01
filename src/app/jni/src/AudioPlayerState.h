/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#pragma once

#include "ApplicationState.h"
#include "Playlist.h"
#include "AudioTypes.h"
#include "QueueElements.h"
#include "CommonFunctions.h"

class AudioPlayer;
class AudioStream;

class PlayState{
public:
	enum Value{
		STOPPED = 0,
		PLAYING,
		ENDING,
		PAUSED,
	};
private:
	Value state;
	static const char *strings[];
public:
	PlayState(): state(STOPPED){}
	PlayState(const PlayState &b): state(b.state){}
	PlayState(const Value &v): state(v){}
	const PlayState &operator=(const PlayState &b){
		return *this = b.state;
	}
	const PlayState &operator=(Value v){
		if (this->state != v)
			__android_log_print(ANDROID_LOG_INFO, "C++Playback", "State change: %s => %s\n", this->strings[(int)this->state], this->strings[(int)v]);
		this->state = v;
		return *this;
	}
	bool operator==(const PlayState &b) const{
		return this->state == b.state;
	}
	bool operator!=(const PlayState &b) const{
		return this->state != b.state;
	}
	bool operator==(Value v) const{
		return this->state == v;
	}
	bool operator!=(Value v) const{
		return this->state != v;
	}
	operator Value() const{
		return this->state;
	}
};

class AudioPlayerState{
	friend class AudioPlayer;
	typedef std::shared_ptr<InternalQueueElement> iqe_t;
	typedef thread_safe_queue<iqe_t> internal_queue_t;
	AudioPlayer *parent;
	PlayerState *player_state = nullptr;
	PlayState state = PlayState::STOPPED;
	Playlist playlist;
	Mutex position_mutex;
	unsigned last_freq_seen = 0;
	audio_position_t last_position_seen = 0;
	double overriding_current_time = -1;
	std::unique_ptr<AudioStream> now_playing;
	double current_total_time;
	internal_queue_t internal_queue;
	unsigned time_of_last_pause;
	audio_buffer_t last_buffer_played;


	void on_end();
	void eliminate_buffers(audio_position_t * = 0);
	void on_stop();
	void on_pause();
	void push_to_internal_queue(InternalQueueElement *p){
		std::shared_ptr<InternalQueueElement> sp(p);
		this->internal_queue.push(sp);
	}
	void push_maybe_to_internal_queue(ExternalQueueElement *p);
public:
	AudioPlayerState() = default;
	AudioPlayerState(AudioPlayer &, PlayerState &);
	~AudioPlayerState();
	AudioPlayerState(const AudioPlayerState &) = delete;
	AudioPlayerState &operator=(const AudioPlayerState &) = delete;
	AudioPlayerState(AudioPlayerState &&other){
		*this = std::move(other);
	}
	AudioPlayerState &operator=(AudioPlayerState &&);
	bool initialize_stream();
	void try_update_total_time();
	double get_current_time();
	//Returns false if nothing (expensive) was done.
	bool process();
	void audio_callback(Uint8 *stream, int len);
	audio_buffer_t get_last_buffer_played(){
		audio_buffer_t ret;
		{
			AutoMutex am(this->position_mutex);
			if (this->last_buffer_played){
				ret = this->last_buffer_played;
				this->last_buffer_played.unref();
			}
		}
		return ret;
	}

	//execute_* functions run in the internal thread!
	bool execute_hardplay();
	bool execute_playpause();
	bool execute_play();
	bool execute_pause();
	bool execute_stop();
	bool execute_absolute_seek(double param, bool scaling);
	bool execute_relative_seek(double seconds);
	bool execute_previous();
	bool execute_next();
	bool execute_load(bool load, bool file, const std::wstring &path);
	bool execute_metadata_update(std::shared_ptr<GenericMetadata>);
	bool execute_exit(){
		return false;
	}
	bool execute_playback_end();
};
