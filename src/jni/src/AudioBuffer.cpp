#include "AudioBuffer.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <iomanip>

audio_buffer_t audio_buffer_t::alloc(unsigned bytes_per_sample, unsigned channels, memory_sample_count_t length, const char *msg){
	audio_buffer_t ret;
	ret.bps = bytes_per_sample * channels;
	size_t n = length * ret.bps;
	//TODO: Optimize this. Implement reusable buffers.
	ret.data = malloc(n);
	if (msg){
		std::cout <<msg<<" - 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<ret.data<<std::endl;
	}
	ret.sample_count = length;
	ret.channel_count = channels;
	ret.data_offset = 0;
#ifdef _DEBUG
	memset(ret.data, 0xCD, n);
#endif
	return ret;
}

void audio_buffer_t::free(const char *msg){
	//TODO: Optimize this. Implement reusable buffers.
	if (msg){
		std::cout <<msg<<" - 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<this->data<<std::endl;
	}
	::free(this->data);
	this->data = 0;
	this->data_offset = 0;
	this->sample_count = 0;
}

audio_buffer_t audio_buffer_t::clone_with_minimum_byte_length(size_t n, const AudioFormat *new_format) const{
	size_t length = std::max(this->full_byte_length(), n);
	audio_buffer_t ret = *this;
	if (new_format){
		ret.bps = new_format->bytes_per_sample();
		ret.channel_count = new_format->channels;
	}
	ret.data_offset = 0;
	ret.data = malloc(length);
	std::cout <<"clone_with_minimum_byte_length() - 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<ret.data<<std::endl;
	memcpy(ret.data, this->data, this->full_byte_length());
	return ret;
}
