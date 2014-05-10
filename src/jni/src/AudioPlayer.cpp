#include "AudioPlayer.h"

void AudioPlayer::AudioCallback(void *udata, Uint8 *stream, int len){
	AudioPlayer *player = (AudioPlayer *)udata;
	const unsigned bytes_per_sample = 2 * 2;

	memory_sample_count_t samples_written = 0;
	while ((unsigned)len > samples_written * bytes_per_sample){
		audio_buffer_t *buffer_pointer;
		size_t bytes_written = samples_written * bytes_per_sample;
		buffer_pointer = player->queue.try_peek();
		if (!buffer_pointer){
			memset(stream + bytes_written, 0, len - bytes_written);
			return;
		}
		audio_buffer_t &buffer = *buffer_pointer;
		size_t ctb_res = buffer.copy_to_buffer<Sint16, 2>(stream + bytes_written, len - samples_written * bytes_per_sample);
		samples_written += (memory_sample_count_t)(ctb_res / bytes_per_sample);
		if (!buffer.samples()){
			buffer = player->queue.pop();
			buffer.free();
		}
	}
}

AudioPlayer::AudioPlayer(): device(*this){
#ifdef WIN32
	//Put your test tracks here when compiling for Windows.
	//TODO: Other systems.
	//this->track_queue.push("f:/Data/Music/Beethoven/CC-sharealike/large_file.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/01 - Bloodlines.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/02 - The Gears.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/03 - Burn The Earth.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/04 - Laser Cannon Deth Sentence.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/05 - Black Fire Upon Us.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/06 - Dethsupport.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/07 - The Cyborg Slayers.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/08 - I Tamper With The Evidence At The Murder Site Of Odin.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/09 - Murmaider II- The Water God.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/10 - Comet Song.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/11 - Symmetry.ogg");
	this->track_queue.push("f:/Data/Music/Dethklok/2009 - The Dethalbum II/12 - Volcano.ogg");
#else
	//Put your test tracks here when compiling for Android.
#endif
	this->queue.max_size = 100;
	this->now_playing = 0;
	this->run = 1;
	this->last_position_seen = 0;
#ifndef PROFILING
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
#else
	this->thread();
#endif
	this->device.start_audio();
}

AudioPlayer::~AudioPlayer(){
	this->run = 0;
#ifndef PROFILING
	SDL_WaitThread(this->sdl_thread, 0);
#endif
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

//#define OUTPUT_TO_FILE

void AudioPlayer::thread(){
	unsigned long long count = 0;
#ifdef PROFILING
	unsigned long long samples_decoded = 0;
	Uint32 t0 = SDL_GetTicks();
#ifdef OUTPUT_TO_FILE
	std::ofstream raw_file("output.raw", std::ios::binary);
#endif
#endif
	AudioPlayerAsyncCommand command;
	while (this->run){
		while (this->command_queue.try_pop(command))
			this->execute_command(command);
		if (!this->now_playing){
			if (!this->track_queue.size()){
#ifndef PROFILING
				SDL_Delay(50);
				continue;
#else
				break;
#endif
			}
			const char *filename = this->track_queue.front();
			this->now_playing = new AudioStream(filename, 44100, 2);
			this->track_queue.pop();
		}
		if (this->queue.is_full()){
			SDL_Delay(50);
			continue;
		}
		audio_buffer_t buffer = this->now_playing->read_new();
		this->last_position_seen = buffer.position;
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
#if !defined PROFILING
		buffer.switch_to_manual();
		this->queue.push(buffer);
#elif defined OUTPUT_TO_FILE
		raw_file.write((char *)buffer.raw_pointer(0), buffer.byte_length());
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

void AudioPlayer::execute_command(const AudioPlayerAsyncCommand &comm){
	switch (comm.comm){
		case AudioPlayerAsyncCommand::Command::SEEK:
			if (this->now_playing){
				this->device.pause_audio();
				audio_position_t pos;
				{
					AutoLocker<thread_safe_queue<audio_buffer_t> > am(this->queue);
					audio_buffer_t buffer;
					if (this->queue.unlocked_try_pop(buffer))
						pos = buffer.position;
					else
						pos = this->last_position_seen;
					this->queue.unlocked_clear();
				}
				this->last_position_seen = this->now_playing->seek(pos, comm.param);
			}
			this->device.start_audio();
			break;
		default:
			break;
	}
}

void AudioPlayer::relative_seek(float ms){
	AudioPlayerAsyncCommand apac;
	apac.comm = AudioPlayerAsyncCommand::Command::SEEK;
	apac.param = ms;
	this->command_queue.push(apac);
}
