#include "AudioBuffer.h"
#include <cstring>
#include <algorithm>

void audio_buffer_t::free(){
	//TODO: Optimize this. Implement reusable buffers.
	::free(this->data);
	this->data = 0;
	this->sample_count = 0;
}

audio_buffer_t audio_buffer_t::clone() const{
	return this->clone_with_minimum_length(this->byte_length());
}

audio_buffer_t audio_buffer_t::clone_with_minimum_length(size_t bytes) const{
	size_t length = std::max(this->byte_length(), bytes);
	audio_buffer_t ret = *this;
	ret.data = malloc(length);
	memcpy(ret.data, this->data, this->full_byte_length());
	return ret;
}
