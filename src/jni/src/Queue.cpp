#if 0
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
			this->buffers[this->start & 1] = buffer;
			this->size++;
			break;
		case 1:
			this->buffers[(this->start + 1) & 1] = buffer;
			this->size++;
			break;
		case 2:
			this->position += this->buffers[this->start].samples();
			this->buffers[this->start].free();
			this->buffers[this->start] = buffer;
			this->start = (this->start + 1) & 1;
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
	char last = this->size - 1;
	switch (this->size){
		case 2:
			if ((*this)[0].samples() > diff)
				return (*this)[0][diff];
		case 1:
			if ((*this)[last].samples() <= diff)
				return 0;
			return (*this)[last][diff];
		case 0:
			return 0;
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
	char last = this->size - 1;
	switch (this->size){
		case 2:
			if ((*this)[0].samples() > diff){
				samples_to_copy = std::min(buffer.samples(), (*this)[0].samples() - diff);
				bytes_to_copy = samples_to_copy * buffer.channels() * 2;
				memcpy(buffer[0], (*this)[0][diff], bytes_to_copy);
				return samples_to_copy;
			}
			diff -= (*this)[0].samples();
		case 1:
			if ((*this)[last].samples() <= diff)
				return 0;
			samples_to_copy = std::min(buffer.samples(), (*this)[last].samples() - diff);
			bytes_to_copy = samples_to_copy * buffer.channels() * 2;
			memcpy(buffer[0], (*this)[last][diff], bytes_to_copy);
			return samples_to_copy;
		case 0:
			return 0;
	}
	assert(0);
	return 0;
}

weak_audio_buffer_t RingBufferQueue::get_a_buffer(audio_position_t position, bool &below){
	weak_audio_buffer_t buffer;
	below = position < this->position;
	if (below)
		return buffer;
	unsigned diff = unsigned(position - this->position);
	unsigned samples_to_copy;
	unsigned bytes_to_copy;
	char last = this->size - 1;
	switch (this->size){
		case 2:
			if ((*this)[0].samples() > diff){
				samples_to_copy = std::min(buffer.samples(), (*this)[0].samples() - diff);
				bytes_to_copy = samples_to_copy * buffer.channels() * 2;
				memcpy(buffer[0], (*this)[0][diff], bytes_to_copy);
				return samples_to_copy;
			}
			diff -= (*this)[0].samples();
		case 1:
			if ((*this)[last].samples() <= diff)
				return 0;
			samples_to_copy = std::min(buffer.samples(), (*this)[last].samples() - diff);
			bytes_to_copy = samples_to_copy * buffer.channels() * 2;
			memcpy(buffer[0], (*this)[last][diff], bytes_to_copy);
			return samples_to_copy;
		case 0:
			return 0;
	}
	assert(0);
	return 0;
}
#endif
