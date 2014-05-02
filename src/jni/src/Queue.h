#ifndef QUEUE_H
#define QUEUE_H
#include "AudioBuffer.h"

class RingBufferQueue{
	audio_buffer_t buffers[2];
	audio_position_t position;
	char start;
	char size;
public:
	RingBufferQueue(): start(0), size(0), position(0){}
	~RingBufferQueue();
	void push(audio_buffer_t buffer);
	audio_buffer_t &operator[](char i);
	char get_size(){
		return this->size;
	}
	const sample_t *get_sample(audio_position_t, bool &below);
	unsigned copy_buffer(audio_buffer_t &buffer, audio_position_t &sample, bool &below);
	weak_audio_buffer_t get_a_buffer(audio_position_t position, bool &below);
};

#endif
