#include "AudioStream.h"

AudioStream::AudioStream(const char *filename, unsigned frequency, unsigned channels, unsigned dfl){
	this->frequency = frequency;
	this->channels = channels;
	this->default_buffer_length = dfl;
	this->position = 0;
	this->decoder = Decoder::create(filename);
	this->filter = 0;
	if (!this->decoder)
		return;
	this->filter = ResamplingFilter::create(*this->decoder, frequency);
}

AudioStream::~AudioStream(){
	delete this->filter;
	delete this->decoder;
}

audio_buffer_t AudioStream::read_new(){
	audio_buffer_t ret;
	if (!this->filter)
		return ret;
	ret = audio_buffer_t::alloc(this->channels, this->default_buffer_length);
	sample_count_t samples_read = this->filter->read(ret, this->position);
	if (!samples_read){
		ret.free();
		ret.data = 0;
	}else{
		this->position += samples_read;
		ret.samples_produced += (unsigned)samples_read;
	}
	return ret;
}
