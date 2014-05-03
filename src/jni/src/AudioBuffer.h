#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BasicTypes.h"
#include <algorithm>

class audio_buffer_t{
	sample_t *data;
	memory_audio_position_t data_offset;
	memory_sample_count_t sample_count;
	unsigned channel_count;
public:
	sample_t *operator[](memory_audio_position_t i){
		return this->data + (i + this->data_offset) * this->channel_count;
	}
	static audio_buffer_t alloc(unsigned channels, memory_sample_count_t length);
	audio_buffer_t alloc();
	void free();
	operator bool() const{
		return this->data != 0;
	}
	memory_sample_count_t samples() const{
		return this->sample_count - this->data_offset;
	}
	unsigned channels() const{
		return this->channel_count;
	}
	size_t bytes_per_sample() const{
		return this->channel_count * sizeof(sample_t);
	}
	size_t byte_length() const{
		return this->samples() * this->bytes_per_sample();
	}
	size_t full_byte_length() const{
		return this->sample_count * this->bytes_per_sample();
	}
	void set_sample_count(unsigned sample_count){
		this->sample_count = sample_count;
	}
	void coerce_into_length(size_t bytes){
		this->sample_count = (memory_sample_count_t)((bytes + (this->channel_count * sizeof(sample_t) - 1)) / this->bytes_per_sample());
	}
	audio_buffer_t clone() const;
	audio_buffer_t clone_with_minimum_length(size_t bytes) const;
	size_t copy_to_buffer(void *dst, size_t max_bytes){
		size_t bytes_to_copy = std::min<size_t>(this->byte_length(), max_bytes);
		memcpy(dst, (*this)[0], bytes_to_copy);
		size_t advance = bytes_to_copy / this->bytes_per_sample();
		this->data_offset += (memory_audio_position_t)advance;
		return bytes_to_copy;
	}
};

#endif
