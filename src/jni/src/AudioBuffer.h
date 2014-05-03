#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BasicTypes.h"
#include <algorithm>

class weak_audio_buffer_t{
	friend class audio_buffer_t;
	sample_t *data;
	unsigned data_offset,
		channel_count,
		sample_count;
	static weak_audio_buffer_t alloc(unsigned channels, unsigned length);
	weak_audio_buffer_t alloc();
	void free();
	size_t full_byte_length() const{
		return this->sample_count * this->channel_count * sizeof(sample_t);
	}
public:
	weak_audio_buffer_t():
		data_offset(0),
		data(0),
		channel_count(0),
		sample_count(0){}
	weak_audio_buffer_t(const audio_buffer_t &ab);
	sample_t *operator[](unsigned i){
		return this->data + (i + this->data_offset) * this->channel_count;
	}
	operator bool() const{
		return this->data != 0;
	}
	unsigned samples() const{
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
	audio_buffer_t clone() const;
	audio_buffer_t clone_with_minimum_length(size_t bytes) const;
	void coerce_into_length(size_t bytes);
	size_t copy_to_buffer(void *dst, size_t max_bytes){
		size_t bytes_to_copy = std::min<size_t>(this->byte_length(), max_bytes);
		memcpy(dst, (*this)[0], bytes_to_copy);
		size_t advance = bytes_to_copy / this->bytes_per_sample();
		this->data_offset += advance;
		return bytes_to_copy;
	}
};

class audio_buffer_t{
	friend class weak_audio_buffer_t;
	weak_audio_buffer_t buffer;
public:
	audio_buffer_t(){}
	audio_buffer_t(const weak_audio_buffer_t &buffer): buffer(buffer){}
	sample_t *operator[](unsigned i){
		return this->buffer[i];
	}
	static audio_buffer_t alloc(unsigned channels, unsigned length){
		return weak_audio_buffer_t::alloc(channels, length);
	}
	audio_buffer_t alloc(){
		audio_buffer_t ret = *this;
		ret.buffer.alloc();
		return ret;
	}
	void free(){
		this->buffer.free();
	}
	operator bool() const{
		return this->buffer;
	}
	unsigned samples() const{
		return this->buffer.samples();
	}
	unsigned channels() const{
		return this->buffer.channels();
	}
	size_t byte_length() const{
		return this->buffer.byte_length();
	}
	void set_sample_count(unsigned sample_count){
		this->buffer.sample_count = sample_count;
	}
	void coerce_into_length(size_t bytes){
		this->buffer.coerce_into_length(bytes);
	}
	audio_buffer_t clone_with_minimum_length(size_t bytes) const{
		return this->buffer.clone_with_minimum_length(bytes);
	}
	size_t copy_to_buffer(void *dst, size_t max_bytes){
		return this->buffer.copy_to_buffer(dst, max_bytes);
	}
};

#endif
