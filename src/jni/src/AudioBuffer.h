#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BasicTypes.h"
#include "Threads.h"
#include <algorithm>

class audio_buffer_instance_tracker{
	friend class audio_buffer_t;
	Mutex mutex;
	unsigned instances;
	size_t memory_used;
	void alloc(size_t n){
		AutoMutex am(this->mutex);
		this->memory_used += n;
		this->instances++;
	}
	void free(size_t n){
		AutoMutex am(this->mutex);
		this->memory_used -= n;
		this->instances--;
	}
public:
	audio_buffer_instance_tracker(): instances(0), memory_used(0){}
	void get_info(unsigned &instances, size_t &memory){
		AutoMutex am(this->mutex);
		instances = this->instances;
		memory = this->memory_used;
	}
};


class audio_buffer_t{
	void *data,
		*true_pointer;
	size_t true_byte_length;
	unsigned *ref_count;
	memory_audio_position_t data_offset;
	memory_sample_count_t sample_count;
	unsigned channel_count;
	unsigned bps;

	void ref();
	void alloc(size_t bytes);
	void alloc(unsigned bytes_per_channel, unsigned channels, memory_sample_count_t length);
	void copy(const audio_buffer_t &);
	void free();
public:
	static audio_buffer_instance_tracker abit;
	audio_position_t position;
	audio_buffer_t(): data(0), true_pointer(0), ref_count(0), data_offset(0), sample_count(0), channel_count(0), bps(0){}
	audio_buffer_t(unsigned bytes_per_sample, unsigned channels, memory_sample_count_t length);
	audio_buffer_t(const audio_buffer_t &);
	const audio_buffer_t &operator=(const audio_buffer_t &);
	audio_buffer_t(audio_buffer_t &&);
	const audio_buffer_t &operator=(audio_buffer_t &&);
	~audio_buffer_t();
	void unref();
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
