#include "Queue.h"
#include <cassert>
#include <algorithm>

RingBufferQueue::~RingBufferQueue(){
	for (char i = 0; i < this->size; i++)
		(*this)[i].free();
}

void RingBufferQueue::push(audio_buffer_t buffer){
	switch (this->size){
		case 0:
			this->buffers[this->start & 1]=buffer;
			this->size++;
			break;
		case 1:
			this->buffers[(this->start + 1) & 1]=buffer;
			this->size++;
			break;
		case 2:
			this->position += this->buffers[this->start].sample_count;
			this->buffers[this->start].free();
			this->buffers[this->start] = buffer;
			this->start = (this->start+1) & 1;
	}
}

audio_buffer_t &RingBufferQueue::operator[](char i){
	return this->buffers[(this->start + i) & 1];
}

const sample_t *RingBufferQueue::get_sample(audio_position_t p, bool &below){
	below = p<this->position;
	if (below)
		return 0;
	size_t diff = size_t(p - this->position);
	switch (this->size){
		case 0:
			return 0;
		case 1:
			if ((*this)[0].sample_count <= diff)
				return 0;
			return (*this)[0][diff];
		case 2:
			if ((*this)[0].sample_count > diff)
				return (*this)[0][diff];
			if ((*this)[1].sample_count <= diff)
				return 0;
			return (*this)[1][diff];
	}
	assert(0);
	return 0;
}

unsigned RingBufferQueue::copy_buffer(audio_buffer_t &buffer, audio_position_t &p, bool &below){
	below = p < this->position;
	if (below)
		return 0;
	unsigned diff = unsigned(p - this->position);
	unsigned samples_to_copy;
	unsigned bytes_to_copy;
	switch (this->size){
		case 0:
			return 0;
		case 1:
			if ((*this)[0].sample_count <= diff)
				return 0;
			samples_to_copy = std::min(buffer.sample_count, (*this)[0].sample_count - diff);
			bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
			memcpy(buffer.data, (*this)[0][diff], bytes_to_copy);
			return samples_to_copy;
		case 2:
			if ((*this)[0].sample_count>diff){
				samples_to_copy = std::min(buffer.sample_count, (*this)[0].sample_count - diff);
				bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
				memcpy(buffer.data, (*this)[0][diff], bytes_to_copy);
				return samples_to_copy;
			}
			diff -= (*this)[0].sample_count;
			if ((*this)[1].sample_count <= diff)
				return 0;
			samples_to_copy = std::min(buffer.sample_count, (*this)[1].sample_count - diff);
			bytes_to_copy = samples_to_copy * buffer.channel_count * 2;
			memcpy(buffer.data, (*this)[1][diff], bytes_to_copy);
			return samples_to_copy;
	}
	assert(0);
	return 0;
}
