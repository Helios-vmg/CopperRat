#include "AudioBuffer.h"
#include <cstring>
#include <algorithm>

audio_buffer_t audio_buffer_t::alloc(unsigned channels, memory_sample_count_t length){
	audio_buffer_t ret;
	//TODO: Optimize this. Implement reusable buffers.
	ret.data = (sample_t *)malloc(channels * length * sizeof(sample_t));
	ret.sample_count = length;
	ret.channel_count = channels;
	ret.data_offset = 0;
#ifdef _DEBUG
	memset(ret.data, 0xCD, sizeof(sample_t) * channels * length);
#endif
	return ret;
}

audio_buffer_t audio_buffer_t::alloc(){
	audio_buffer_t ret = audio_buffer_t::alloc(this->sample_count, this->channel_count);
#ifdef _DEBUG
	memset(ret.data, 0xCD, sizeof(sample_t) * this->channel_count * this->sample_count);
#endif
	return ret;
}

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
	audio_buffer_t ret;
	ret.data = (sample_t *)malloc(length);
	ret.sample_count = this->sample_count;
	ret.channel_count = this->channel_count;
	ret.data_offset = this->data_offset;
	memcpy(ret.data, this->data, this->full_byte_length());
	return ret;
}
