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
#include "AudioStream.h"
#include "AudioPlayer.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <string>
#endif

AudioStream::AudioStream(AudioPlayer &parent, const std::wstring &path, unsigned frequency, unsigned channels):
		parent(parent),
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

void AudioStream::seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double param, bool scaling){
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
		player->execute_next();
		return;
	}
	this->position = new_position = this->decoder->seek(target) ? target : current_position;
}

void AudioStream::seek(AudioPlayer *player, audio_position_t &new_position, audio_position_t current_position, double seconds){
	if (this->position < current_position){
		//There was a track switch in the middle of the buffer queue. Do not seek.
		new_position = this->position;
		return;
	}
	audio_position_t target = audio_position_t(current_position + seconds * (double)this->decoder->get_audio_format().freq);
	if (target >= this->decoder->get_pcm_length()){
		if (seconds > 0)
			player->execute_next();
		else
			player->execute_previous(/*1*/);
		return;
	}
	this->position = new_position = this->decoder->seek(target) ? target : current_position;
}

void AudioStream::metadata_update(std::shared_ptr<GenericMetadata> p){
	this->parent.execute_metadata_update(p);
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
	return ret;
}
