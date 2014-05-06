#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BasicTypes.h"
#include <algorithm>

class audio_buffer_t{
	void *data;
	memory_audio_position_t data_offset;
	memory_sample_count_t sample_count;
	unsigned channel_count;
	unsigned bps;
public:
	void *raw_pointer(memory_audio_position_t i){
		return (char *)this->data + (i + this->data_offset) * this->bps;
	}
	template <typename NumberT, unsigned Channels>
	sample_t<NumberT, Channels> *get_sample(memory_audio_position_t i){
		return (sample_t<NumberT, Channels> *)this->data + (i + this->data_offset);
	}
	template <typename NumberT>
	sample_t<NumberT, 1> *get_sample_use_channels(memory_audio_position_t i){
		return (sample_t<NumberT, 1> *)this->data + (i + this->data_offset) * this->channel_count;
	}
	template <typename NumberT>
	static audio_buffer_t alloc(unsigned channels, memory_sample_count_t length){
		audio_buffer_t ret;
		//TODO: Optimize this. Implement reusable buffers.
		ret.bps = sizeof(NumberT) * channels;
		size_t n = length * ret.bps;
		ret.data = malloc(n);
		ret.sample_count = length;
		ret.channel_count = channels;
		ret.data_offset = 0;
#ifdef _DEBUG
		memset(ret.data, 0xCD, n);
#endif
		return ret;
	}
	template <typename NumberT, unsigned Channels>
	static audio_buffer_t alloc(memory_sample_count_t length){
		audio_buffer_t ret;
		//TODO: Optimize this. Implement reusable buffers.
		ret.bps = sizeof(sample_t<NumberT, Channels>);
		size_t n = length * ret.bps;
		ret.data = malloc(n);
		ret.sample_count = length;
		ret.channel_count = Channels;
		ret.data_offset = 0;
#ifdef _DEBUG
		memset(ret.data, 0xCD, n);
#endif
		return ret;
	}
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
		return this->bps;
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
		this->sample_count = (memory_sample_count_t)((bytes + this->bps - 1) / this->bps);
	}
	audio_buffer_t clone() const{
		return this->clone_with_minimum_length(this->samples());
	}
	audio_buffer_t clone_with_minimum_length(memory_sample_count_t samples) const{
		return this->clone_with_minimum_length_in_target_format(samples, AudioFormat(0, this->bps / this->channel_count, this->channel_count, 0));
	}
	audio_buffer_t clone_with_minimum_length_in_target_format(memory_sample_count_t samples, const AudioFormat &af) const{
		return this->clone_with_minimum_byte_length(samples * af.bytes_per_sample(), &af);
	}
	audio_buffer_t clone_with_minimum_byte_length(size_t bytes, const AudioFormat *new_format = 0) const;
	template <typename NumberT, unsigned Channels>
	size_t copy_to_buffer(void *dst, size_t max_bytes){
		size_t bytes_to_copy = std::min<size_t>(this->byte_length(), max_bytes);
		memcpy(dst, this->get_sample<NumberT, Channels>(0), bytes_to_copy);
		size_t advance = bytes_to_copy / this->bytes_per_sample();
		this->data_offset += (memory_audio_position_t)advance;
		return bytes_to_copy;
	}
};

#endif
