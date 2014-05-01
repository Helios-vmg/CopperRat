#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(){
#ifdef WIN32
	//Put your test tracks here when compiling for Windows.
	//TODO: Other systems.
#else
	this->playlist.push("/sdcard/external_sd/Music/Evangelion-Cruel Angel.ogg");
	//Put your test tracks here when compiling for Android.
#endif
	this->queue.max_size = 100;
	this->now_playing = 0;
	this->run = 1;
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
}

AudioPlayer::~AudioPlayer(){
	this->run = 0;
	SDL_WaitThread(this->sdl_thread, 0);
	while (!this->queue.is_empty())
		this->queue.pop().free();
}

int AudioPlayer::_thread(void *p){
	((AudioPlayer *)p)->thread();
	return 0;
}

double playback_time = 0;

void AudioPlayer::thread(){
	unsigned long long count = 0;
	while (this->run){
#if 0
		{
			unsigned fullness = unsigned(this->queue.size() * 40 / this->queue.max_size);
			char display[41];
			display[40] = 0;
			for (unsigned i = 0; i < 40; i++)
				display[i] = (i < fullness) ? '#' : ' ';
			std::cerr <<display<<"]\n";
		} 
#endif

		if (!this->now_playing){
			if (!this->playlist.size()){
				SDL_Delay(50);
				continue;
			}
			const char *filename = this->playlist.front();
			this->now_playing = new AudioStream(filename, 44100, 2, DEFAULT_BUFFER_SIZE);
			this->playlist.pop();
		}
		audio_buffer_t buffer = this->now_playing->read_new();
		playback_time = double(buffer.sample_count) / (44.1 * 2.0);
		if (!buffer.data){
			delete this->now_playing;
			this->now_playing = 0;
			continue;
		}
		count++;
		this->queue.push(buffer);
	}
}

void AudioPlayer::test(){
	unsigned long long count = 0;
	while (this->run){
		if (!this->now_playing){
			if (!this->playlist.size())
				break;
			const char *filename = this->playlist.front();
			this->now_playing = new AudioStream(filename, 44100, 2, DEFAULT_BUFFER_SIZE);
			this->playlist.pop();
		}
		audio_buffer_t buffer = this->now_playing->read_new();
		if (!buffer.data){
			delete this->now_playing;
			this->now_playing = 0;
			continue;
		}else
			buffer.free();
		count++;
	}
}
