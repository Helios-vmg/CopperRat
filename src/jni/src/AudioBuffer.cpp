#include "AudioBuffer.h"
#include <cstring>

audio_buffer_t audio_buffer_t::alloc(unsigned channels, unsigned length){
	audio_buffer_t ret;
	ret.data = new sample_t[channels * length];
	ret.sample_count = length;
	ret.channel_count = channels;
	ret.samples_produced = 0;
	ret.samples_consumed = 0;
#ifdef _DEBUG
	memset(ret.data, 0xCD, sizeof(sample_t) * channels * length);
#endif
	return ret;
}

audio_buffer_t audio_buffer_t::alloc(){
	audio_buffer_t ret;
	//TODO: Optimize this. Implement reusable buffers.
	ret.data = new sample_t[this->channel_count * this->sample_count];
	ret.sample_count = this->sample_count;
	ret.channel_count = this->channel_count;
	ret.samples_produced = this->samples_produced;
	ret.samples_consumed = this->samples_consumed;
#ifdef _DEBUG
	memset(ret.data, 0xCD, sizeof(sample_t) * this->channel_count * this->sample_count);
#endif
	return ret;
}

void audio_buffer_t::free(){
	//TODO: Optimize this. Implement reusable buffers.
	delete[] this->data;
}
