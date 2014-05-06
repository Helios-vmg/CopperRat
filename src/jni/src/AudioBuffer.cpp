#include "AudioBuffer.h"
#include <cstring>
#include <algorithm>

void audio_buffer_t::free(){
	//TODO: Optimize this. Implement reusable buffers.
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
	memcpy(ret.data, this->data, this->full_byte_length());
	return ret;
}
