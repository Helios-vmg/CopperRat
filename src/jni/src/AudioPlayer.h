#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H
#include "Threads.h"
#include "AudioStream.h"
#include <queue>

#define DEFAULT_BUFFER_SIZE 4096

class AudioPlayer{
	friend void AudioCallback(void *udata, Uint8 *stream, int len);
	friend int main(int argc, char **argv);
	thread_safe_queue<audio_buffer_t> queue;
	SDL_Thread *sdl_thread;
	AudioStream *now_playing;
	std::queue<const char *> playlist;
	volatile bool run;
	static int _thread(void *);
	void thread();
public:
	AudioPlayer();
	~AudioPlayer();
	void test();
};

#endif
