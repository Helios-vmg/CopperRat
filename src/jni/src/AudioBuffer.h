#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BasicTypes.h"

struct audio_buffer_t{
	sample_t *data;
	unsigned channel_count;
	unsigned sample_count;
	unsigned samples_produced;
	unsigned samples_consumed;
	audio_buffer_t():
		data(0),
		channel_count(0),
		sample_count(0),
		samples_produced(0),
		samples_consumed(0){}
	sample_t *operator[](size_t i){
		return this->data + i * this->channel_count;
	}
	static audio_buffer_t alloc(unsigned channels, unsigned length);
	audio_buffer_t alloc();
	void free();
};

#endif
