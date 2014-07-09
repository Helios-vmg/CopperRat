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
		boost::shared_ptr<InternalQueueElement> pointer
	) = 0;
	virtual bool is_buffer() const = 0;
};

class BufferQueueElement : public InternalQueueElement{
	audio_buffer_t buffer;
public:
	BufferQueueElement(audio_buffer_t buffer): buffer(buffer){}
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
		AudioPlayer &player;
		AudioLocker(AudioPlayer &player): player(player){
			player.device.pause_audio();
		}
		~AudioLocker(){
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
	Atomic<audio_position_t> last_position_seen;
	double current_total_time;
	bool jumped_this_loop;

	void thread();
	void try_update_total_time();
	bool initialize_stream();
	void push_to_command_queue(AudioPlayerAsyncCommand *p){
		boost::shared_ptr<AudioPlayerAsyncCommand> sp(p);
		this->external_queue_in.push(sp);
	}
	void push_to_internal_queue(InternalQueueElement *p){
		boost::shared_ptr<InternalQueueElement> sp(p);
		this->internal_queue.push(sp);
	}
	void eliminate_buffers(audio_position_t * = 0);
	bool handle_requests();
	void on_stop();
public:
	external_queue_out_t external_queue_out;
	AudioPlayer();
	~AudioPlayer();

	//request_* functions run in the caller thread!
	void request_hardplay();
	void request_playpause();
	void request_play();
	void request_pause();
	void request_stop();
	void request_seek(double seconds);
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
	bool execute_seek(double seconds);
	bool execute_previous();
	bool execute_next();
	bool execute_load(bool load, bool file, const std::wstring &path);
	/*
	bool execute_previous(bool seek_near_the_end = 0){
		throw NotImplementedException();
		return 1;
	}
	*/
	bool execute_exit(){
		return 0;
	}
	bool execute_metadata_update(boost::shared_ptr<GenericMetadata>);
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

class AsyncCommandSeek : public AudioPlayerAsyncCommand{
	double seconds;
public:
	AsyncCommandSeek(AudioPlayer *player, double seconds): AudioPlayerAsyncCommand(player), seconds(seconds){}
	bool execute(){
		return this->player->execute_seek(this->seconds);
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
