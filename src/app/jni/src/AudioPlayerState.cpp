/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioPlayerState.h"
#include "AudioPlayer.h"
#include "SUI/MainScreen.h"

AudioPlayerState::AudioPlayerState(AudioPlayer &parent, PlayerState &player_state): playlist(player_state){
	this->parent = &parent;
	this->player_state = &player_state;
	this->internal_queue.max_size = 100;
	double time = this->player_state->get_playback().get_current_time();
	if (time < 0)
		return;
	this->state = PlayState::PAUSED;
	int success = 0;
	int index = this->playlist.get_current_track_index();
	while (success == 0){
		try{
			this->initialize_stream();
			success = 1;
		}catch (DecoderInitializationException &){
			if (!this->playlist.next() || this->playlist.get_current_track_index() == index)
				success = -1;
			time = 0;
		}
	}
	if (success > 0){
		time = std::max(time - 5, 0.0);
		this->execute_absolute_seek(time, 0);
		this->overriding_current_time = time;
	}else{
		this->playlist.clear();
	}
}

AudioPlayerState::~AudioPlayerState(){
	if (this->player_state)
		this->player_state->get_playback().set_current_time(this->state == PlayState::STOPPED ? -1 : this->get_current_time());
}

template <typename T>
struct is_basic_type{
	static const bool value =
		std::is_integral<T>::value ||
		std::is_pointer<T>::value ||
		std::is_floating_point<T>::value ||
		std::is_enum<T>::value ||
		std::is_pod<T>::value && std::is_trivially_move_constructible<T>::value;
};

template <typename T>
typename std::enable_if<is_basic_type<T>::value, void>::type
move(T &dst, T &src){
	dst = std::move(src);
	src = {};
}

template <typename T>
typename std::enable_if<!is_basic_type<T>::value, void>::type
move(T &dst, T &src){
	dst = std::move(src);
}

bool AudioPlayerState::initialize_stream(){
	if (this->now_playing || this->state == PlayState::STOPPED)
		return true;
	std::wstring next;
	if (!this->playlist.get_current_track(next)){
		this->on_end();
		return false;
	}
	auto &filename = next;
	this->now_playing.reset(new AudioStream(*this, filename, 44100, 2));
	this->current_total_time = -1;
	this->try_update_total_time();
	return true;
}

void AudioPlayerState::try_update_total_time(){
	if (this->current_total_time >= 0)
		return;
	this->current_total_time = this->now_playing->get_total_time();
	if (this->current_total_time < 0)
		return;
	this->push_maybe_to_internal_queue([this, t = this->current_total_time](){
		this->main_screen->on_total_time_update(t);
	});
}

void AudioPlayerState::on_end(){
	if (this->state == PlayState::ENDING)
		return;
	this->state = PlayState::ENDING;
	this->push_to_internal_queue(new PlaybackEnd(*this));
}

void AudioPlayerState::on_stop(){
	if (this->state == PlayState::STOPPED)
		return;
	this->state = PlayState::STOPPED;
	this->current_total_time = 0;
	{
		AutoMutex am(this->position_mutex);
		this->last_position_seen = 0;
		this->last_freq_seen = 0;
	}
	this->player_state->get_playback().set_current_time(-1);
	this->parent->sui->push_async_callback([this](){
		this->main_screen->on_playback_stop();
	});
}

void AudioPlayerState::on_pause(){
	this->time_of_last_pause = SDL_GetTicks();
	this->player_state->get_playback().set_current_time(this->get_current_time());
}

double AudioPlayerState::get_current_time(){
	AutoMutex am(this->position_mutex);
	if (!this->last_freq_seen){
		if (this->overriding_current_time >= 0)
			return this->overriding_current_time;
		return 0;
	}
	return (double)this->last_position_seen / (double)this->last_freq_seen;
}

void AudioPlayerState::eliminate_buffers(audio_position_t *pos){
	AutoLocker<internal_queue_t> am(this->internal_queue);
	std::queue<iqe_t> temp;
	bool set = false;
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
		set = true;
	}
	while (temp.size()){
		this->internal_queue.push(temp.front());
		temp.pop();
	}
}

bool AudioPlayerState::process(){
	/*if (this->state == PlayState::PAUSED && this->parent->device.is_open()){
		unsigned now = SDL_GetTicks();
		if (now >= this->time_of_last_pause + 5000){
			__android_log_print(ANDROID_LOG_INFO, "C++Audio", "%s", "Audio inactivity timeout. Closing device.\n");
			this->parent->device.close();
		}
	}*/
	audio_buffer_t buffer;
	std::shared_ptr<DecoderException> exc;
	try{
		if (!this->initialize_stream() || this->internal_queue.is_full() || this->state == PlayState::STOPPED)
			return false;
		buffer = this->now_playing->read();
		this->try_update_total_time();
	}catch (DecoderException &e){
		exc.reset(static_cast<DecoderException *>(e.clone()));
	}

	bool b_continue = false;
	if (!buffer){
		this->now_playing.reset();
		this->playlist.next();
		b_continue = true;
	}
	if (exc)
		throw *exc;
	if (b_continue)
		return true;

	//std::cout <<"Outputting "<<buffer.samples()<<" samples.\n";
	this->push_to_internal_queue(new BufferQueueElement(this, buffer, this->now_playing->get_stream_format()));
#if defined OUTPUT_TO_FILE
	raw_file.write((char *)buffer.raw_pointer(0), buffer.byte_length());
#endif
	return true;
}

void AudioPlayerState::audio_callback(Uint8 *stream, int len){
	AutoLocker<internal_queue_t> al(this->internal_queue);
	const unsigned bytes_per_sample = 2 * 2;

	memory_sample_count_t samples_written = 0;
	audio_position_t last_position;
	bool perform_final_pops = 0;
	unsigned last_sample_rate;
	audio_buffer_t last_buffer;
	AudioPlayerState *state;
	while ((unsigned)len > samples_written * bytes_per_sample){
		const size_t bytes_written = samples_written * bytes_per_sample;
		auto element = this->internal_queue.unlocked_try_peek();
		if (!element){
			memset(stream + bytes_written, 0, len - bytes_written);
			return;
		}
		auto action = (*element)->AudioCallback_switch(
			this->parent,
			state,
			stream,
			len,
			bytes_per_sample,
			samples_written,
			last_position,
			last_sample_rate,
			*element
		);
		if ((*element)->is_buffer())
			last_buffer = static_cast<BufferQueueElement &>(**element).get_buffer();
		if (action)
			this->internal_queue.unlocked_simple_pop();
		perform_final_pops = action;
	}
	{
		AutoMutex am(state->position_mutex);
		state->last_freq_seen = last_sample_rate;
		state->last_position_seen = last_position;
		if (last_buffer){
			this->last_buffer_played = audio_buffer_t(last_buffer.bytes_per_sample() / last_buffer.channels(), last_buffer.channels(), samples_written);
			memcpy(this->last_buffer_played.raw_pointer(0), stream, samples_written * bytes_per_sample);
			this->last_buffer_played.reset_offset();
		}
	}
	if (!perform_final_pops)
		return;
	while (true){
		{
			auto element = this->internal_queue.unlocked_try_peek();
			if (!element)
				return;
			if ((*element)->is_buffer())
				break;
		}
		auto element = this->internal_queue.unlocked_simple_pop();
		auto eqe = static_cast<ExternalQueueElement *>(element.get());
		eqe->push(this->parent, element);
	}
}

bool AudioPlayerState::execute_hardplay(){
	if (this->is_empty())
		return true;
	this->parent->device.start_audio();
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
	return true;
}

bool AudioPlayerState::execute_playpause(){
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			this->parent->device.start_audio();
			this->state = PlayState::PLAYING;
			break;
		case PlayState::PLAYING:
			this->parent->device.pause_audio();
			this->state = PlayState::PAUSED;
			this->on_pause();
			break;
	}
	return true;
}

bool AudioPlayerState::execute_play(){
	switch (this->state){
		case PlayState::STOPPED:
		case PlayState::PAUSED:
			this->parent->device.start_audio();
			this->state = PlayState::PLAYING;
			break;
		case PlayState::PLAYING:
			break;
	}
	return true;
}

bool AudioPlayerState::execute_pause(){
	switch (this->state){
		case PlayState::STOPPED:
			break;
		case PlayState::PLAYING:
			this->on_pause();
			this->parent->device.pause_audio();
			this->state = PlayState::PAUSED;
			break;
		case PlayState::PAUSED:
			this->parent->device.start_audio();
			this->state = PlayState::PLAYING;
			break;
	}
	return true;
}

bool AudioPlayerState::execute_stop(){
	switch (this->state){
		case PlayState::STOPPED:
			break;
		case PlayState::PLAYING:
		case PlayState::PAUSED:
		case PlayState::ENDING:
			this->parent->device.pause_audio();
			this->eliminate_buffers();
			this->now_playing.reset();
			this->on_stop();
			break;
	}
	this->state = PlayState::STOPPED;
	return true;
}

bool AudioPlayerState::execute_absolute_seek(double param, bool scaling){
	//TODO: Find some way to merge this code with AudioPlayerState::execute_relative_seek().
	if (!this->now_playing)
		return true;
	AudioDevice::AudioLocker al(this->parent->device);
	audio_position_t pos = invalid_audio_position;
	this->eliminate_buffers(&pos);
	if (pos == invalid_audio_position){
		AutoMutex am(this->position_mutex);
		pos = this->last_position_seen;
	}
	audio_position_t new_pos;
	this->now_playing->seek(new_pos, pos, param, scaling);
	AutoMutex am(this->position_mutex);
	this->last_position_seen = new_pos;
	return true;
}

bool AudioPlayerState::execute_relative_seek(double seconds){
	if (!this->now_playing)
		return true;
	AudioDevice::AudioLocker al(this->parent->device);
	audio_position_t pos = invalid_audio_position;
	this->eliminate_buffers(&pos);
	if (pos == invalid_audio_position){
		AutoMutex am(this->position_mutex);
		pos = this->last_position_seen;
	}
	audio_position_t new_pos;
	this->now_playing->seek(new_pos, pos, seconds);
	AutoMutex am(this->position_mutex);
	this->last_position_seen = new_pos;
	return true;
}

bool AudioPlayerState::execute_previous(){
	AudioDevice::AudioLocker al(this->parent->device);
	this->eliminate_buffers();
	this->now_playing.reset();
	if (this->playlist.back())
		this->initialize_stream();
	return true;
}

bool AudioPlayerState::execute_next(){
	AudioDevice::AudioLocker al(this->parent->device);
	this->eliminate_buffers();
	this->now_playing.reset();
	if (this->playlist.next(true))
		this->initialize_stream();
	return true;
}

bool AudioPlayerState::execute_load(bool load, bool file, const std::wstring &path){
	if (load){
		this->execute_stop();
		this->playlist.load(file, path);
		this->execute_play();
	}else
		this->playlist.append(file, path);
	return true;
}

bool AudioPlayerState::execute_metadata_update(const std::shared_ptr<GenericMetadata> &metadata){
	this->push_maybe_to_internal_queue([this, metadata](){
		this->main_screen->on_metadata_update(metadata);
	});
	return true;
}

bool AudioPlayerState::execute_playback_end(){
	this->execute_stop();
	return true;
}

bool AudioPlayerState::is_empty() const{
	return this->playlist.is_empty();
}

void AudioPlayerState::erase(){
	application_state->erase(*this->player_state);
	this->player_state = nullptr;
	this->playlist.forget_state();
}
