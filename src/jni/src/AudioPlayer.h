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
#include "auto_ptr.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <boost/shared_ptr.hpp>
#endif

class AudioPlayerAsyncCommand{
protected:
	AudioPlayer *player;
public:
	AudioPlayerAsyncCommand(AudioPlayer *player): player(player){}
	virtual ~AudioPlayerAsyncCommand(){}
	virtual bool execute() = 0;
};

class InternalQueueElement{
public:
	virtual ~InternalQueueElement(){}
	virtual bool AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		unsigned &sample_rate,
		boost::shared_ptr<InternalQueueElement> pointer
	) = 0;
	virtual bool is_buffer() const = 0;
};

class BufferQueueElement : public InternalQueueElement{
	audio_buffer_t buffer;
	AudioFormat stream_format;
public:
	BufferQueueElement(audio_buffer_t buffer, const AudioFormat &stream_format): buffer(buffer), stream_format(stream_format){}
	audio_buffer_t get_buffer(){
		return this->buffer;
	}
	bool AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		unsigned &sample_rate,
		boost::shared_ptr<InternalQueueElement> pointer
	);
	bool is_buffer() const{
		return 1;
	}
};

class ExternalQueueElement : public InternalQueueElement{
public:
	virtual ~ExternalQueueElement(){}
	void push(AudioPlayer *player, boost::shared_ptr<InternalQueueElement> pointer);
	virtual bool AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		unsigned &sample_rate,
		boost::shared_ptr<InternalQueueElement> pointer
	){
		this->push(player, pointer);
		return 1;
	}
	bool is_buffer() const{
		return 0;
	}
	virtual unsigned receive(UserInterface &) = 0;
};

class ExceptionTransport : public ExternalQueueElement{
	CR_Exception e;
public:
	ExceptionTransport(const CR_Exception &e): e(e){}
	unsigned receive(UserInterface &ui){
		throw this->e;
		return 0;
	}
};

class TotalTimeUpdate : public ExternalQueueElement{
	double seconds;
public:
	TotalTimeUpdate(double seconds): seconds(seconds){}
	double get_seconds(){
		return this->seconds;
	}
	unsigned receive(UserInterface &ui){
		return ui.receive(*this);
	}
};

class MetaDataUpdate : public ExternalQueueElement{
	boost::shared_ptr<GenericMetadata> metadata;
public:
	MetaDataUpdate(boost::shared_ptr<GenericMetadata> metadata): metadata(metadata){}
	boost::shared_ptr<GenericMetadata> get_metadata(){
		return this->metadata;
	}
	unsigned receive(UserInterface &ui){
		return ui.receive(*this);
	}
};

class PlaybackStop : public ExternalQueueElement{
public:
	unsigned receive(UserInterface &ui){
		return ui.receive(*this);
	}
};

class ExitAcknowledged : public ExternalQueueElement{
public:
	unsigned receive(UserInterface &ui){
		return 0;
	}
};

struct NotImplementedException{};

class AudioPlayer{
	friend class AudioDevice;
	typedef boost::shared_ptr<InternalQueueElement> iqe_t;
	typedef boost::shared_ptr<ExternalQueueElement> eqe_t;
	typedef thread_safe_queue<iqe_t> internal_queue_t;
	typedef boost::shared_ptr<AudioPlayerAsyncCommand> command_t;
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

	enum class PlayState{
		STOPPED,
		PLAYING,
		PAUSED,
	} state;

	AudioDevice device;
	internal_queue_t internal_queue;
	external_queue_in_t external_queue_in;
	SDL_Thread *sdl_thread;
	CR_UNIQUE_PTR(AudioStream) now_playing;
	Playlist playlist;
	static void AudioCallback(void *udata, Uint8 *stream, int len);
	static int _thread(void *);
	Mutex position_mutex;
	unsigned last_freq_seen;
	audio_position_t last_position_seen;
	double overriding_current_time;
	double current_total_time;
	bool jumped_this_loop;
	unsigned time_of_last_pause;

	void thread();
	void thread_loop();
	void try_update_total_time();
	bool initialize_stream();
	void push_to_command_queue(AudioPlayerAsyncCommand *p){
		boost::shared_ptr<AudioPlayerAsyncCommand> sp(p);
		this->external_queue_in.push(sp);
	}
	void push_maybe_to_internal_queue(ExternalQueueElement *p);
	void push_to_external_queue(ExternalQueueElement *p){
		boost::shared_ptr<ExternalQueueElement> sp(p);
		this->external_queue_out.push(sp);
	}
	void push_to_internal_queue(InternalQueueElement *p){
		boost::shared_ptr<InternalQueueElement> sp(p);
		this->internal_queue.push(sp);
	}
	void eliminate_buffers(audio_position_t * = 0);
	bool handle_requests();
	void on_stop();
	void on_pause();
public:
	external_queue_out_t external_queue_out;
	AudioPlayer(RemoteThreadProcedureCallPerformer &rtpcp);
	~AudioPlayer();

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
	bool execute_exit(){
		return 0;
	}
	bool execute_metadata_update(boost::shared_ptr<GenericMetadata>);
	Playlist &get_playlist(){
		return this->playlist;
	}
};

class AsyncCommandHardPlay : public AudioPlayerAsyncCommand{
public:
	AsyncCommandHardPlay(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_hardplay();
	}
};

class AsyncCommandPlayPause : public AudioPlayerAsyncCommand{
public:
	AsyncCommandPlayPause(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_playpause();
	}
};

class AsyncCommandPlay : public AudioPlayerAsyncCommand{
public:
	AsyncCommandPlay(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_play();
	}
};

class AsyncCommandPause : public AudioPlayerAsyncCommand{
public:
	AsyncCommandPause(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_pause();
	}
};

class AsyncCommandStop : public AudioPlayerAsyncCommand{
public:
	AsyncCommandStop(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_stop();
	}
};

class AsyncCommandAbsoluteSeek : public AudioPlayerAsyncCommand{
	bool scaling;
	double param;
public:
	AsyncCommandAbsoluteSeek(AudioPlayer *player, double param, bool scaling = 1): AudioPlayerAsyncCommand(player), param(param), scaling(scaling){}
	bool execute(){
		return this->player->execute_absolute_seek(this->param, this->scaling);
	}
};

class AsyncCommandRelativeSeek : public AudioPlayerAsyncCommand{
	double seconds;
public:
	AsyncCommandRelativeSeek(AudioPlayer *player, double seconds): AudioPlayerAsyncCommand(player), seconds(seconds){}
	bool execute(){
		return this->player->execute_relative_seek(this->seconds);
	}
};

class AsyncCommandPrevious : public AudioPlayerAsyncCommand{
public:
	AsyncCommandPrevious(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_previous();
	}
};

class AsyncCommandNext : public AudioPlayerAsyncCommand{
public:
	AsyncCommandNext(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_next();
	}
};

class AsyncCommandExit : public AudioPlayerAsyncCommand{
public:
	AsyncCommandExit(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	bool execute(){
		return this->player->execute_exit();
	}
};

class AsyncCommandLoad : public AudioPlayerAsyncCommand{
	bool load,
		file;
	std::wstring path;
public:
	AsyncCommandLoad(AudioPlayer *player, bool load, bool file, const std::wstring &path): AudioPlayerAsyncCommand(player), load(load), file(file), path(path){}
	bool execute(){
		return this->player->execute_load(this->load, this->file, this->path);
	}
};

#endif
