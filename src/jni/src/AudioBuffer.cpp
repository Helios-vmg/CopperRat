#include "AudioBuffer.h"
#include <cstring>
#include <algorithm>

struct malloced_buffer{
	unsigned ref_count;
	char data[1];
};

void audio_buffer_t::alloc(size_t bytes){
	//TODO: Optimize this. Implement reusable buffers.
	malloced_buffer *buffer = (malloced_buffer *)malloc(sizeof(malloced_buffer) + bytes);
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
	this->move_copy(buffer);
}

const audio_buffer_t &audio_buffer_t::operator=(const audio_buffer_t &buffer){
	this->unref();
	this->data = buffer.data;
	this->true_pointer = buffer.true_pointer;
	this->ref_count = buffer.ref_count;
	this->data_offset = buffer.data_offset;
	this->sample_count = buffer.sample_count;
	this->channel_count = buffer.channel_count;
	this->bps = buffer.bps;
	this->ref();
	return *this;
}

const audio_buffer_t &audio_buffer_t::operator=(audio_buffer_t &&buffer){
	this->move_copy(buffer);
	return *this;
}

void audio_buffer_t::move_copy(audio_buffer_t &buffer){
	this->unref();
	this->data = buffer.data;
	this->true_pointer = buffer.true_pointer;
	this->ref_count = buffer.ref_count;
	this->data_offset = buffer.data_offset;
	this->sample_count = buffer.sample_count;
	this->channel_count = buffer.channel_count;
	this->bps = buffer.bps;
	buffer.ref_count = 0;
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
	::free(this->true_pointer);
	this->true_pointer = 0;
	this->data = 0;
	this->ref_count = 0;
	this->data_offset = 0;
	this->sample_count = 0;
}

void audio_buffer_t::switch_to_manual(){
	*this->ref_count = 0;
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
