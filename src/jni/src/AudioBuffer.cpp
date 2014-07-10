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
#include "AudioBuffer.h"
#include "CommonFunctions.h"
#ifndef HAVE_PRECOMPILED_HEADERS
#include <cstring>
#include <algorithm>
#endif

audio_buffer_instance_tracker audio_buffer_t::abit;

struct malloced_buffer{
	unsigned ref_count;
	char data[1];
};

void audio_buffer_t::alloc(size_t bytes){
	this->true_byte_length = sizeof(malloced_buffer) + bytes;
	malloced_buffer *buffer = (malloced_buffer *)malloc(this->true_byte_length);
	if (!buffer){
		__android_log_print(ANDROID_LOG_ERROR, "Memory", "malloc() failed!");
		throw std::bad_alloc();
	}
	if (buffer)
		abit.alloc(this->true_byte_length);
	this->true_pointer = buffer;
	this->ref_count = &buffer->ref_count;
	this->data = &buffer->data;
	*this->ref_count = 1;
}

void audio_buffer_t::alloc(unsigned bytes_per_sample, unsigned channels, memory_sample_count_t length){
	unsigned bps = bytes_per_sample * channels;
	size_t n = length * bps;
	this->alloc(n);
	this->bps = bps;
	this->sample_count = length;
	this->channel_count = channels;
	this->data_offset = 0;
#ifdef _DEBUG
	memset(this->data, 0xCD, n);
#endif
}

audio_buffer_t::audio_buffer_t(unsigned bytes_per_sample, unsigned channels, memory_sample_count_t length){
	this->alloc(bytes_per_sample, channels, length);
}

audio_buffer_t::audio_buffer_t(const audio_buffer_t &buffer){
	this->ref_count = 0;
	*this = buffer;
}

audio_buffer_t::audio_buffer_t(audio_buffer_t &&buffer){
	this->ref_count = 0;
	this->copy(buffer);
	buffer.ref_count = 0;
}

const audio_buffer_t &audio_buffer_t::operator=(const audio_buffer_t &buffer){
	this->copy(buffer);
	this->ref();
	return *this;
}

const audio_buffer_t &audio_buffer_t::operator=(audio_buffer_t &&buffer){
	this->copy(buffer);
	buffer.ref_count = 0;
	return *this;
}

void audio_buffer_t::copy(const audio_buffer_t &buffer){
	this->unref();
	this->data = buffer.data;
	this->true_pointer = buffer.true_pointer;
	this->ref_count = buffer.ref_count;
	this->data_offset = buffer.data_offset;
	this->sample_count = buffer.sample_count;
	this->channel_count = buffer.channel_count;
	this->true_byte_length = buffer.true_byte_length;
	this->bps = buffer.bps;
	this->position = buffer.position;
}

audio_buffer_t::~audio_buffer_t(){
	this->unref();
}

void audio_buffer_t::ref(){
	if (this->ref_count)
		++*this->ref_count;
}

void audio_buffer_t::unref(){
	if (!this->ref_count)
		return;
	--*this->ref_count;
	if (!*this->ref_count)
		this->free();
	this->true_pointer = 0;
	this->data = 0;
	this->ref_count = 0;
	this->data_offset = 0;
	this->sample_count = 0;
}

void audio_buffer_t::free(){
	if (this->true_pointer)
		abit.free(this->true_byte_length);
	::free(this->true_pointer);
	this->true_pointer = 0;
	this->data = 0;
	this->ref_count = 0;
	this->data_offset = 0;
	this->sample_count = 0;
}

audio_buffer_t audio_buffer_t::clone_with_minimum_byte_length(size_t n, const AudioFormat *new_format) const{
	size_t length = std::max(this->full_byte_length(), n);
	audio_buffer_t ret;
	if (new_format){
		ret.bps = new_format->bytes_per_sample();
		ret.channel_count = new_format->channels;
	}else{
		ret.bps = this->bps;
		ret.channel_count = this->channel_count;
	}
	ret.alloc(length);
	ret.sample_count = this->sample_count;
	ret.data_offset = this->data_offset;
	memcpy(ret.data, this->data, this->full_byte_length());
	return ret;
}
