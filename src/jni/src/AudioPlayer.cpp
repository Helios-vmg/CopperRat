#include "AudioPlayer.h"

AudioPlayer::AudioPlayer(){
#ifdef WIN32
	//Put your test tracks here when compiling for Windows.
	//TODO: Other systems.
#else
	//Put your test tracks here when compiling for Android.
#endif
	this->queue.max_size = 100;
	this->now_playing = 0;
	this->run = 1;
#ifndef PROFILING
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
#else
	this->thread();
#endif
}

AudioPlayer::~AudioPlayer(){
	this->run = 0;
#ifndef PROFILING
	SDL_WaitThread(this->sdl_thread, 0);
#endif
	while (!this->queue.is_empty())
		this->queue.pop().free();
}

int AudioPlayer::_thread(void *p){
	((AudioPlayer *)p)->thread();
	return 0;
}

#ifdef PROFILING
#if defined WIN32
#include <iostream>
#include <fstream>
#else
#include <fstream>
#include <sstream>
#include <android/log.h>
#endif
#endif
#include <fstream>

double playback_time = 0;

void AudioPlayer::thread(){
	unsigned long long count = 0;
#ifdef PROFILING
	unsigned long long samples_decoded = 0;
	Uint32 t0 = SDL_GetTicks();
#endif
	while (this->run){
		if (!this->now_playing){
			if (!this->playlist.size()){
#ifndef PROFILING
				SDL_Delay(50);
				continue;
#else
				break;
#endif
			}
			const char *filename = this->playlist.front();
			this->now_playing = new AudioStream(filename, 44100, 2, DEFAULT_BUFFER_SIZE);
			this->playlist.pop();
		}
		audio_buffer_t buffer = this->now_playing->read_new();
		playback_time = double(buffer.samples()) / (44.1 * 2.0);
#ifdef PROFILING
		samples_decoded += buffer.samples();
#endif
		if (!buffer){
			delete this->now_playing;
			this->now_playing = 0;
			continue;
		}
		count++;
#ifndef PROFILING
		this->queue.push(buffer);
#else
		buffer.free();
#endif
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
		__android_log_print(ANDROID_LOG_ERROR, "PERFORMANCE", "%fx", times);
#endif
	}
#endif
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
		if (!buffer){
			delete this->now_playing;
			this->now_playing = 0;
			continue;
		}else
			buffer.free();
		count++;
	}
}
