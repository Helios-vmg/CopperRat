#include "AudioPlayer.h"

void ExternalQueueElement::push(AudioPlayer *player, boost::shared_ptr<InternalQueueElement> pointer){
	player->external_queue_out.push(boost::static_pointer_cast<ExternalQueueElement>(pointer));
}

bool BufferQueueElement::AudioCallback_switch(
	AudioPlayer *player,
	Uint8 *stream,
	int len,
	unsigned bytes_per_sample,
	memory_sample_count_t &samples_written,
	audio_position_t &last_position,
	boost::shared_ptr<InternalQueueElement> pointer
){
	const size_t bytes_written = samples_written * bytes_per_sample;
	auto &buffer = this->buffer;
	last_position = buffer.position;
	size_t ctb_res = buffer.copy_to_buffer<Sint16, 2>(stream + bytes_written, len - samples_written * bytes_per_sample);
	samples_written += (memory_sample_count_t)(ctb_res / bytes_per_sample);
	return !buffer.samples();
}

void AudioPlayer::AudioCallback(void *udata, Uint8 *stream, int len){
	AudioPlayer *player = (AudioPlayer *)udata;
	const unsigned bytes_per_sample = 2 * 2;

	memory_sample_count_t samples_written = 0;
	audio_position_t last_position;
	bool perform_final_pops = 0;
	while ((unsigned)len > samples_written * bytes_per_sample){
		const size_t bytes_written = samples_written * bytes_per_sample;
		auto element = player->internal_queue.try_peek();
		if (!element){
			memset(stream + bytes_written, 0, len - bytes_written);
			return;
		}
		auto action = (*element)->AudioCallback_switch(
			player,
			stream,
			len,
			bytes_per_sample,
			samples_written,
			last_position,
			*element
		);
		if (action)
			player->internal_queue.pop();
		perform_final_pops = action;
	}
	player->last_position_seen.set(last_position);
	if (!perform_final_pops)
		return;
	while (1){
		{
			auto element = player->internal_queue.try_peek();
			if ((*element)->is_buffer())
				break;
		}
		auto element = player->internal_queue.pop();
		auto eqe = static_cast<ExternalQueueElement *>(element.get());
		eqe->push(player, element);
	}
}

#include <fstream>

AudioPlayer::AudioPlayer(): device(*this){
#ifndef __ANDROID__
	//Put your test tracks here when compiling for desktop OSs.
	{
		std::ifstream file("test_tracks.txt");
		while (1){
			std::string line;
			std::getline(file, line);
			if (!file)
				break;
			if (!line.size())
				continue;
			this->track_queue.push(line);
		}
	}
#else
	this->track_queue.push("/sdcard/external_sd/Music/Ghost Riders In The Sky.mp3");
	//Put your test tracks here when compiling for Android.
#endif
	this->internal_queue.max_size = 100;
	this->last_position_seen.set(0);
	this->state = PlayState::STOPPED;
#ifndef PROFILING
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
#else
	this->thread();
#endif
	this->device.start_audio();
}

AudioPlayer::~AudioPlayer(){
#ifndef PROFILING
	this->device.close();
	this->request_exit();
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

//#define OUTPUT_TO_FILE

bool AudioPlayer::initialize_stream(){
	if (this->now_playing.get() || this->state == PlayState::STOPPED)
		return 1;
	if (!this->track_queue.size())
		return 0;
	auto filename = this->track_queue.front();
	this->now_playing.reset(new AudioStream(*this, filename.c_str(), 44100, 2));
	this->track_queue.pop();
	this->current_total_time = -1;
	this->try_update_total_time();
	return 1;
}

void AudioPlayer::thread(){
#ifdef PROFILING
	unsigned long long samples_decoded = 0;
	Uint32 t0 = SDL_GetTicks();
#ifdef OUTPUT_TO_FILE
	std::ofstream raw_file("output.raw", std::ios::binary);
#endif
#endif
	this->jumped_this_loop = 0;
	while (this->handle_requests()){
#if PROFILING
		if (!this->now_playing.get() && !this->track_queue.size())
			break;
#endif
		if (!this->initialize_stream() || this->internal_queue.is_full() || this->state == PlayState::STOPPED){
			SDL_Delay(50);
			continue;
		}
		audio_buffer_t buffer = this->now_playing->read();
		this->try_update_total_time();
#ifdef PROFILING
		samples_decoded += buffer.samples();
#endif
		if (!buffer){
			this->now_playing.reset();
			continue;
		}
		this->jumped_this_loop = 0;
#if !defined PROFILING
		this->push_to_internal_queue(new BufferQueueElement(buffer));
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

bool AudioPlayer::handle_requests(){
	bool ret = 1;
	for (command_t command; this->external_queue_in.try_pop(command) && ret;)
		ret = command->execute();
	return ret;
}

void AudioPlayer::request_play(){
	this->push_to_command_queue(new AsyncCommandPlay(this));
}

void AudioPlayer::request_pause(){
	this->push_to_command_queue(new AsyncCommandPause(this));
}

void AudioPlayer::request_seek(double seconds){
	this->push_to_command_queue(new AsyncCommandSeek(this, seconds));
}

void AudioPlayer::request_next(){
	this->push_to_command_queue(new AsyncCommandNext(this));
}

void AudioPlayer::request_exit(){
	this->push_to_command_queue(new AsyncCommandExit(this));
}

void AudioPlayer::eliminate_buffers(audio_position_t *pos){
	AutoLocker<internal_queue_t> am(this->internal_queue);
	std::queue<iqe_t> temp;
	bool set = 0;
	while (!this->internal_queue.unlocked_is_empty()){
		auto el = this->internal_queue.unlocked_simple_pop();
		auto bqe = dynamic_cast<BufferQueueElement *>(el.get());
		if (!bqe){
			temp.push(el);
			continue;
		}
		if (set)
			continue;
		auto buffer = bqe->get_buffer();
		if (pos)
			*pos = buffer.position;
		set = 1;
	}
	while (temp.size()){
		this->internal_queue.push(temp.front());
		temp.pop();
	}
}

bool AudioPlayer::execute_play(){
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			SDL_PauseAudio(0);
			this->state = PlayState::PLAYING;
			break;
		case PlayState::PLAYING:
			break;
	}
	return 1;
}

bool AudioPlayer::execute_pause(){
	switch (this->state){
		case PlayState::STOPPED:
			break;
		case PlayState::PLAYING:
			SDL_PauseAudio(1);
			this->state = PlayState::PAUSED;
			break;
		case PlayState::PAUSED:
			SDL_PauseAudio(0);
			this->state = PlayState::PLAYING;
			break;
	}
	return 1;
}

bool AudioPlayer::execute_seek(double seconds){
	if (!this->now_playing.get() || this->jumped_this_loop)
		return 1;
	AudioLocker al(*this);
	audio_position_t pos = invalid_audio_position;
	this->eliminate_buffers(&pos);
	if (pos == invalid_audio_position)
		pos = this->last_position_seen.get();
	audio_position_t new_pos;
	this->now_playing->seek(this, new_pos, pos, seconds);
	this->last_position_seen.set(new_pos);
	return 1;
}

bool AudioPlayer::execute_next(){
	AudioLocker al(*this);
	this->eliminate_buffers();
	this->now_playing.reset();
	this->initialize_stream();
	return 1;
}

bool AudioPlayer::execute_metadata_update(boost::shared_ptr<Metadata> metadata){
	this->push_to_internal_queue(new MetaDataUpdate(metadata));
	return 1;
}

double AudioPlayer::get_current_time(){
	return this->last_position_seen.get() / 44100.0;
}

void AudioPlayer::try_update_total_time(){
	if (this->current_total_time >= 0)
		return;
	this->current_total_time = this->now_playing->get_total_time();
	if (this->current_total_time < 0)
		return;
	this->push_to_internal_queue(new TotalTimeUpdate(this->current_total_time));
}
