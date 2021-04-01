/*
Copyright (c), Helios
All rights reserved.

Distributed under a permissive license. See COPYING.txt for details.
*/

#include "stdafx.h"
#include "AudioStream.h"
#include "AudioPlayer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#endif

AudioStream::AudioStream(AudioPlayerState &parent, const std::wstring &path, unsigned frequency, unsigned channels):
		parent(&parent),
		dst_format(true, 2, channels, frequency),
		multiplier(1){
	this->decoder.reset(Decoder::create(*this, path));
	if (!this->decoder)
		return;
	filter.reset(new AudioFilterManager(*this->decoder, dst_format, this->multiplier));
#ifdef DUMP_OUTPUT
	std::string s = filename;
	s.append(".raw");
	this->test_file.open(s.c_str(), std::ios::binary);
#endif
	this->position = 0;
}

audio_buffer_t AudioStream::read(){
	memory_sample_count_t samples_read;
	audio_buffer_t ret = this->filter->read(samples_read);
	if (!ret)
		return ret;
	ret.position = this->position;
	this->position += samples_read;
#ifdef DUMP_OUTPUT
	this->test_file.write((const char *)ret.raw_pointer(0), ret.byte_length());
#endif
	return ret;
}

void AudioStream::seek(audio_position_t &new_position, audio_position_t current_position, double param, bool scaling){
	if (this->position < current_position){
		//There was a track switch in the middle of the buffer queue. Do not seek.
		new_position = this->position;
		return;
	}
	audio_position_t target;
	if (scaling)
		target = audio_position_t(this->decoder->get_pcm_length() * param);
	else{
		auto freq = this->decoder->get_audio_format().freq;
		if (!freq){
			new_position = this->position;
			return;
		}
		target = audio_position_t(param * (double)freq);
	}
	if (target >= this->decoder->get_pcm_length()){
		this->parent->execute_next();
		return;
	}
	this->position = new_position = this->decoder->seek(target) ? target : current_position;
	this->filter->clear_saved_buffer();
}

void AudioStream::seek(audio_position_t &new_position, audio_position_t current_position, double seconds){
	if (this->position < current_position){
		//There was a track switch in the middle of the buffer queue. Do not seek.
		new_position = this->position;
		return;
	}
	audio_position_t target = audio_position_t(current_position + seconds * (double)this->decoder->get_audio_format().freq);
	if (target >= this->decoder->get_pcm_length()){
		if (seconds > 0)
			this->parent->execute_next();
		else
			this->parent->execute_previous();
		return;
	}
	this->position = new_position = this->decoder->seek(target) ? target : current_position;
	this->filter->clear_saved_buffer();
}

void AudioStream::metadata_update(std::shared_ptr<GenericMetadata> p){
	this->parent->execute_metadata_update(p);
	double multiplier = replaygain_get_multiplier(*p, ReplayGainSettings());
	this->multiplier = multiplier;
	if (multiplier != 1.0){
		__android_log_print(ANDROID_LOG_INFO, "C++ReplayGain", "ReplayGain applied: %f\n", multiplier);
		if (!!this->filter)
			this->filter->add_multiplication_filter(multiplier);
	}
}

bool AudioStream::reset(){
	bool ret;
	if (ret = this->decoder->seek(0))
		this->position = 0;
	this->filter->clear_saved_buffer();
	return ret;
}
