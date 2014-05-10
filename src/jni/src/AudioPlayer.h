#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H
#include "Threads.h"
#include "AudioDevice.h"
#include "AudioStream.h"
#include "AudioBuffer.h"
#include <queue>

struct AudioPlayerAsyncCommand{
	enum class Command{
		PLAYPAUSE,
		STOP,
		NEXT,
		PREVIOUS,
		SEEK,
	};
	Command comm;
	float param;
};

class AudioPlayer{
	friend class AudioDevice;
	AudioDevice device;
	thread_safe_queue<audio_buffer_t> queue;
	thread_safe_queue<AudioPlayerAsyncCommand> command_queue;
	SDL_Thread *sdl_thread;
	AudioStream *now_playing;
	std::queue<const char *> track_queue;
	volatile bool run;
	static void AudioCallback(void *udata, Uint8 *stream, int len);
	static int _thread(void *);
	audio_position_t last_position_seen;
	void thread();
	void execute_command(const AudioPlayerAsyncCommand &);
public:
	AudioPlayer();
	~AudioPlayer();

	void relative_seek(float ms);
};

#endif
