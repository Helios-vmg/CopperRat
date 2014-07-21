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

#include "stdafx.h"
#include "AudioPlayer.h"
#include "CommonFunctions.h"
#include "Settings.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <fstream>
#ifdef PROFILING
#if defined WIN32
#include <fstream>
#else
#include <sstream>
#include <android/log.h>
#endif
#endif
#endif

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
	unsigned &sample_rate,
	boost::shared_ptr<InternalQueueElement> pointer
){
	const size_t bytes_written = samples_written * bytes_per_sample;
	auto &buffer = this->buffer;
	last_position = buffer.position;
	sample_rate = this->stream_format.freq;
	size_t ctb_res = buffer.copy_to_buffer<Sint16, 2>(stream + bytes_written, len - samples_written * bytes_per_sample);
	samples_written += (memory_sample_count_t)(ctb_res / bytes_per_sample);
	return !buffer.samples();
}

void AudioPlayer::AudioCallback(void *udata, Uint8 *stream, int len){
	AudioPlayer *player = (AudioPlayer *)udata;
	AutoLocker<internal_queue_t> al(player->internal_queue);
	const unsigned bytes_per_sample = 2 * 2;

	memory_sample_count_t samples_written = 0;
	audio_position_t last_position;
	bool perform_final_pops = 0;
	unsigned last_sample_rate;
	while ((unsigned)len > samples_written * bytes_per_sample){
		const size_t bytes_written = samples_written * bytes_per_sample;
		auto element = player->internal_queue.unlocked_try_peek();
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
			last_sample_rate,
			*element
		);
		if (action)
			player->internal_queue.unlocked_simple_pop();
		perform_final_pops = action;
	}
	{
		AutoMutex am(player->position_mutex);
		player->last_freq_seen = last_sample_rate;
		player->last_position_seen = last_position;
	}
	if (!perform_final_pops)
		return;
	while (1){
		{
			auto element = player->internal_queue.unlocked_try_peek();
			if (!element)
				return;
			if ((*element)->is_buffer())
				break;
		}
		auto element = player->internal_queue.unlocked_simple_pop();
		auto eqe = static_cast<ExternalQueueElement *>(element.get());
		eqe->push(player, element);
	}
}

AudioPlayer::AudioPlayer(RemoteThreadProcedureCallPerformer &rtpcp):
		device(*this, rtpcp),
		overriding_current_time(-1){
	this->jumped_this_loop = 0;
	this->internal_queue.max_size = 100;
	this->last_position_seen = 0;
	this->last_freq_seen = 0;
	this->state = PlayState::STOPPED;
	double time = application_settings.get_current_time();
	if (time >= 0){
		this->state = PlayState::PAUSED;
		this->initialize_stream();
		time = std::max(time - 5, 0.0);
		this->execute_absolute_seek(time, 0);
		this->overriding_current_time = time;
	}
#ifndef PROFILING
	this->sdl_thread = SDL_CreateThread(_thread, "AudioPlayerThread", this);
#else
	this->thread();
#endif
}

void AudioPlayer::terminate_thread(UserInterface &ui){
#ifndef PROFILING
	this->request_exit();
	ExitAcknowledged *target = nullptr;
	while (1){
		auto el = this->external_queue_out.pop();
		target = dynamic_cast<ExitAcknowledged *>(&*el);
		if (!target)
			el->receive(ui);
		else
			break;
	}
#endif
}

AudioPlayer::~AudioPlayer(){
#ifndef PROFILING
	SDL_WaitThread(this->sdl_thread, 0);
#endif
	application_settings.set_current_time(this->state == PlayState::STOPPED ? -1 : this->get_current_time());
}

int AudioPlayer::_thread(void *p){
	((AudioPlayer *)p)->thread();
	return 0;
}

//#define OUTPUT_TO_FILE

bool AudioPlayer::initialize_stream(){
	if (this->now_playing || this->state == PlayState::STOPPED)
		return 1;
	std::wstring next;
	if (!this->playlist.get_current_track(next)){
		this->on_stop();
		return 0;
	}
	auto &filename = next;
	this->now_playing.reset(new AudioStream(*this, filename, 44100, 2));
	this->current_total_time = -1;
	this->try_update_total_time();
	return 1;
}

void AudioPlayer::push_maybe_to_internal_queue(ExternalQueueElement *p){
	boost::shared_ptr<ExternalQueueElement> sp(p);
	{
		AutoLocker<internal_queue_t> al(this->internal_queue);
		if (this->internal_queue.unlocked_is_empty()){
			this->external_queue_out.push(sp);
			return;
		}
	}
	this->internal_queue.push(sp);
}

void AudioPlayer::on_stop(){
	this->state = PlayState::STOPPED;
	this->current_total_time = 0;
	{
		AutoMutex am(this->position_mutex);
		this->last_position_seen = 0;
		this->last_freq_seen = 0;
	}
	application_settings.set_current_time(-1);
	this->device.close();
	this->external_queue_out.push(eqe_t(new PlaybackStop));
}

void AudioPlayer::on_pause(){
	this->time_of_last_pause = SDL_GetTicks();
	application_settings.set_current_time(this->get_current_time());
}

void AudioPlayer::thread(){
#ifdef PROFILING
	unsigned long long samples_decoded = 0;
	Uint32 t0 = SDL_GetTicks();
#endif
#ifdef OUTPUT_TO_FILE
	std::ofstream raw_file("output.raw", std::ios::binary);
#endif
	while (this->handle_requests()){
		if (this->state == PlayState::PAUSED && this->device.is_open()){
			unsigned now = SDL_GetTicks();
			if (now >= this->time_of_last_pause + 5000){
				__android_log_print(ANDROID_LOG_INFO, "C++Audio", "%s", "Audio inactivity timeout. Closing device.\n");
				this->device.close();
			}
		}
#if PROFILING
		if (!this->now_playing && !this->track_queue.size())
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
			this->playlist.next();
			continue;
		}
		this->jumped_this_loop = 0;
		//std::cout <<"Outputting "<<buffer.samples()<<" samples.\n";
#if !defined PROFILING
		this->push_to_internal_queue(new BufferQueueElement(buffer, this->now_playing->get_stream_format()));
#endif
#if defined OUTPUT_TO_FILE
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

	this->device.close();
	this->external_queue_out.push(eqe_t(new ExitAcknowledged));
}

bool AudioPlayer::handle_requests(){
	bool ret = 1;
	for (command_t command; this->external_queue_in.try_pop(command) && ret;)
		ret = command->execute();
	return ret;
}

void AudioPlayer::request_hardplay(){
	this->push_to_command_queue(new AsyncCommandHardPlay(this));
}

void AudioPlayer::request_playpause(){
	this->push_to_command_queue(new AsyncCommandPlayPause(this));
}

void AudioPlayer::request_play(){
	this->push_to_command_queue(new AsyncCommandPlay(this));
}

void AudioPlayer::request_pause(){
	this->push_to_command_queue(new AsyncCommandPause(this));
}

void AudioPlayer::request_stop(){
	this->push_to_command_queue(new AsyncCommandStop(this));
}

void AudioPlayer::request_absolute_scaling_seek(double scale){
	this->push_to_command_queue(new AsyncCommandAbsoluteSeek(this, scale, 1));
}

void AudioPlayer::request_relative_seek(double seconds){
	this->push_to_command_queue(new AsyncCommandRelativeSeek(this, seconds));
}

void AudioPlayer::request_previous(){
	this->push_to_command_queue(new AsyncCommandPrevious(this));
}

void AudioPlayer::request_next(){
	this->push_to_command_queue(new AsyncCommandNext(this));
}

void AudioPlayer::request_exit(){
	this->push_to_command_queue(new AsyncCommandExit(this));
}

void AudioPlayer::request_load(bool load, bool file, const std::wstring &path){
	this->push_to_command_queue(new AsyncCommandLoad(this, load, file, path));
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

bool AudioPlayer::execute_hardplay(){
	this->device.start_audio();
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			break;
		case PlayState::PLAYING:
			if (this->now_playing->reset())
				this->eliminate_buffers();
			break;
	}
	this->state = PlayState::PLAYING;
	return 1;
}

bool AudioPlayer::execute_playpause(){
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			this->device.start_audio();
			this->state = PlayState::PLAYING;
			break;
		case PlayState::PLAYING:
			this->device.pause_audio();
			this->state = PlayState::PAUSED;
			this->on_pause();
			break;
	}
	return 1;
}

bool AudioPlayer::execute_play(){
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			this->device.start_audio();
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
			this->on_pause();
			this->device.pause_audio();
			this->state = PlayState::PAUSED;
			break;
		case PlayState::PAUSED:
			this->device.start_audio();
			this->state = PlayState::PLAYING;
			break;
	}
	return 1;
}

bool AudioPlayer::execute_stop(){
	switch (this->state){
		case PlayState::STOPPED:
			break;
		case PlayState::PLAYING:
		case PlayState::PAUSED:
			this->device.pause_audio();
			this->eliminate_buffers();
			this->now_playing.reset();
			this->on_stop();
			break;
	}
	this->state = PlayState::STOPPED;
	return 1;
}

bool AudioPlayer::execute_absolute_seek(double param, bool scaling){
	//TODO: Find some way to merge this code with AudioPlayer::execute_relative_seek().
	if (!this->now_playing || this->jumped_this_loop)
		return 1;
	AudioLocker al(*this);
	audio_position_t pos = invalid_audio_position;
	this->eliminate_buffers(&pos);
	if (pos == invalid_audio_position){
		AutoMutex am(this->position_mutex);
		pos = this->last_position_seen;
	}
	audio_position_t new_pos;
	this->now_playing->seek(this, new_pos, pos, param, scaling);
	AutoMutex am(this->position_mutex);
	this->last_position_seen = new_pos;
	return 1;
}

bool AudioPlayer::execute_relative_seek(double seconds){
	if (!this->now_playing || this->jumped_this_loop)
		return 1;
	AudioLocker al(*this);
	audio_position_t pos = invalid_audio_position;
	this->eliminate_buffers(&pos);
	if (pos == invalid_audio_position){
		AutoMutex am(this->position_mutex);
		pos = this->last_position_seen;
	}
	audio_position_t new_pos;
	this->now_playing->seek(this, new_pos, pos, seconds);
	AutoMutex am(this->position_mutex);
	this->last_position_seen = new_pos;
	return 1;
}

bool AudioPlayer::execute_previous(){
	AudioLocker al(*this);
	this->eliminate_buffers();
	this->now_playing.reset();
	if (this->playlist.back())
		this->initialize_stream();
	return 1;
}

bool AudioPlayer::execute_next(){
	AudioLocker al(*this);
	this->eliminate_buffers();
	this->now_playing.reset();
	if (this->playlist.next())
		this->initialize_stream();
	return 1;
}

bool AudioPlayer::execute_load(bool load, bool file, const std::wstring &path){
	if (load){
		this->execute_stop();
		this->playlist.load(file, path);
		this->execute_play();
	}else
		this->playlist.append(file, path);
	return 1;
}

bool AudioPlayer::execute_metadata_update(boost::shared_ptr<GenericMetadata> metadata){
	this->push_maybe_to_internal_queue(new MetaDataUpdate(metadata));
	return 1;
}

double AudioPlayer::get_current_time(){
	AutoMutex am(this->position_mutex);
	if (!this->last_freq_seen){
		if (this->overriding_current_time >= 0)
			return this->overriding_current_time;
		return 0;
	}
	return (double)this->last_position_seen / (double)this->last_freq_seen;
}

void AudioPlayer::try_update_total_time(){
	if (this->current_total_time >= 0)
		return;
	this->current_total_time = this->now_playing->get_total_time();
	if (this->current_total_time < 0)
		return;
	this->push_maybe_to_internal_queue(new TotalTimeUpdate(this->current_total_time));
}
