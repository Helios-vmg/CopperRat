#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H
#include "Threads.h"
#include "AudioDevice.h"
#include "AudioStream.h"
#include "AudioBuffer.h"
#include <boost/shared_ptr.hpp>

class AudioPlayerAsyncCommand{
protected:
	AudioPlayer *player;
public:
	AudioPlayerAsyncCommand(AudioPlayer *player): player(player){}
	virtual ~AudioPlayerAsyncCommand(){}
	virtual void execute() = 0;
};

class InternalQueueElement{
public:
	virtual ~InternalQueueElement(){}
	enum class PostAction{
		NOTHING,
		POP,
		POP_AND_DELETE,
	};
	virtual PostAction AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		boost::shared_ptr<InternalQueueElement> pointer
	) = 0;
};

class ExternalQueueElement : public InternalQueueElement{
public:
	virtual ~ExternalQueueElement(){}
	void push(AudioPlayer *player, boost::shared_ptr<InternalQueueElement> pointer);
};

class BufferQueueElement : public ExternalQueueElement{
	audio_buffer_t buffer;
public:
	BufferQueueElement(audio_buffer_t buffer): buffer(buffer){}
	audio_buffer_t get_buffer(){
		return this->buffer;
	}
	PostAction AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		boost::shared_ptr<InternalQueueElement> pointer
	);
};

class TotalTimeUpdate : public ExternalQueueElement{
	double seconds;
public:
	TotalTimeUpdate(double seconds): seconds(seconds){}
	double get_seconds(){
		return this->seconds;
	}
	PostAction AudioCallback_switch(
		AudioPlayer *player,
		Uint8 *stream,
		int len,
		unsigned bytes_per_sample,
		memory_sample_count_t &samples_written,
		audio_position_t &last_position,
		boost::shared_ptr<InternalQueueElement> pointer
	){
		this->push(player, pointer);
		return PostAction::POP;
	}
};

struct NotImplementedException{};

class AudioPlayer{
	friend class AudioDevice;
	friend class ExternalQueueElement;
	typedef boost::shared_ptr<InternalQueueElement> iqe_t;
	typedef thread_safe_queue<iqe_t> internal_queue_t;
	typedef boost::shared_ptr<AudioPlayerAsyncCommand> command_t;
	typedef thread_safe_queue<command_t> external_queue_in_t;
	typedef internal_queue_t external_queue_out_t;

	AudioDevice device;
	internal_queue_t internal_queue;
	external_queue_in_t external_queue_in;
	SDL_Thread *sdl_thread;
	std::auto_ptr<AudioStream> now_playing;
	std::queue<const char *> track_queue;
	volatile bool run;
	static void AudioCallback(void *udata, Uint8 *stream, int len);
	static int _thread(void *);
	Atomic<audio_position_t> last_position_seen;
	double current_total_time;
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
	void eliminate_buffers(audio_position_t &);
public:
	external_queue_out_t external_queue_out;
	AudioPlayer();
	~AudioPlayer();

	//request_* functions run in the caller thread!
	void request_seek(double seconds);
	void request_next();
	double get_current_time();

	//execute_* functions run in the internal thread!
	void execute_seek(double seconds);
	void execute_next();
	void execute_previous(bool seek_near_the_end = 0){
		throw NotImplementedException();
	}
};

class AsyncCommandSeek : public AudioPlayerAsyncCommand{
	double seconds;
public:
	AsyncCommandSeek(AudioPlayer *player, double seconds): AudioPlayerAsyncCommand(player), seconds(seconds){}
	void execute(){
		player->execute_seek(this->seconds);
	}
};

class AsyncCommandNext : public AudioPlayerAsyncCommand{
public:
	AsyncCommandNext(AudioPlayer *player): AudioPlayerAsyncCommand(player){}
	void execute(){
		player->execute_next();
	}
};

#endif
